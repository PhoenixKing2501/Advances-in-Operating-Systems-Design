#include <linux/compiler_types.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kconfig.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#define DEBUG

const char PROC_FILENAME[] = "partb_1_20CS30062_20CS30057";

// for functions like creating a deque/ allocating memory
#define SUCCESS 0
#define FAILURE 1

// max allowable read size in bytes for a read call
#define MAX_READ_SIZE 4

// max allowable write size in bytes for a write call
#define MAX_WRITE_SIZE 4

MODULE_LICENSE( "Dual BSD/GPL" );
MODULE_AUTHOR( "Swarup_Utsav" );
MODULE_VERSION( "0.1" );
MODULE_DESCRIPTION( "LKM for deque" );

static DEFINE_MUTEX( file_mutex );

static struct proc_dir_entry * proc_file;

// element of deq (implemented as linked list)
struct deq_element
{
	int data;
	struct deq_element * next;
};

// deque structure , front and rear pointer, size and capacity(max size allowed)
struct deque
{
	struct deq_element * front;
	struct deq_element * rear;
	int size;
	int capacity;
};

// states of the deque
#define PROC_FOPEN 0  // capacity not yet initialised
#define PROC_FWRITE 1 // capacity initialised, can write to deque

// stores information about a process working on this file such as pid, state, deque etc
struct proc_node
{
	// struct proc_dir_entry *proc_file;
	pid_t pid;
	int state;
	struct deque * deque;
	struct proc_node * next;
};

// list of procs having this file open/writing on file
static struct proc_node * proc_list;
// to copy a block of data from user space to kernel
static char ker_buf[ MAX_WRITE_SIZE ];
// to store the size of buffer
static ssize_t ker_buf_size = 0;

static int create_deque( struct proc_node * proc , int capacity )
{
	proc->deque = ( struct deque * ) kmalloc( sizeof( struct deque ) , GFP_KERNEL );
	if ( proc->deque == NULL )
	{
		return FAILURE;
	}
	proc->deque->front = NULL;
	proc->deque->rear = NULL;
	proc->deque->size = 0;
	proc->deque->capacity = capacity;
	return SUCCESS;
}

// finds a process from list of processes
static struct proc_node * find_proc_node( pid_t pid )
{
	struct proc_node * temp = proc_list;
	while ( temp != NULL )
	{
		if ( temp->pid == pid )
			return temp;
		temp = temp->next;
	}
	return NULL;
}

// inserts a proc_node into list, returns 0 on success and 1 on failure
static int insert_proc_node( pid_t pid )
{
	struct proc_node * new_proc = ( struct proc_node * ) kmalloc( sizeof( struct proc_node ) , GFP_KERNEL );
	if ( new_proc == NULL )
	{
		return FAILURE;
	}
	new_proc->pid = pid;
	new_proc->state = PROC_FOPEN;
	// modify list
	new_proc->next = proc_list;
	proc_list = new_proc;
	// the deque is NULL until this proc starts writing to the file
	return SUCCESS;
}

// delete proc /cleanup its resources
static void delete_proc( struct proc_node * proc )
{
	// delete the deque in proc
	if ( proc->deque )
	{
		struct deq_element * temp = proc->deque->front;
		while ( temp != NULL )
		{
			struct deq_element * temp2 = temp;
			temp = temp->next;
			kfree( temp2 );
		}
		kfree( proc->deque );
	}
	kfree( proc );
}

// delete process from list
static void delete_proc_from_list( struct proc_node * proc )
{
	struct proc_node * temp = proc_list;
	if ( temp == proc )
	{
		proc_list = proc_list->next;
	}
	else
	{
		while ( temp->next != proc )
		{
			temp = temp->next;
		}
		temp->next = proc->next;
	}
	delete_proc( proc );
}

static void print_deq( struct deque * deq )
{
#ifdef DEBUG
	struct deq_element * temp = deq->front;
	pr_info( "Deque: " );
	while ( temp != NULL )
	{
		pr_cont( "%d " , temp->data );
		temp = temp->next;
	}
	pr_info( "\n" );
#endif
}

static int my_open( struct inode * inode , struct file * file )
{
	pid_t pid;
	int ret;
	mutex_lock( &file_mutex );
	pid = current->pid;
	struct proc_node * cur;
	if ( pid < 0 )
	{
		pr_err( "Invalid pid\n" );
		ret = -EINVAL;
	}
	else
	{
		pr_info( "proc_open() invoked by Process pid %d\n" , pid );
		// check if pid already in list
		cur = find_proc_node( pid );
		if ( cur != NULL )
		{
			pr_err( "Process pid %d already has this file open\n" , pid );
			ret = -EACCES;
		}
		else
		{
			// insert proc node
			if ( insert_proc_node( pid ) == FAILURE )
			{
				pr_err( "Failed to open file\n" );
				ret = -ENOMEM;
			}
			else
			{
				pr_info( "Process pid %d successfully opened the file" , pid );
				ret = 0;
			}
		}
	}

	mutex_unlock( &file_mutex );
	return ret;
}

static ssize_t helper_write( struct proc_node * cur )
{
	// if state of cur FOPEN accept only an integer of 1 byte denoting capacity of deque else accept integer of 4 bytes to insert into deque
	int capacity;
	if ( cur->state == PROC_FOPEN )
	{
		// allow only single byte buffer size
		if ( ker_buf_size != 1 )
		{
			pr_err( "Error: invalid write\n" );
			return -EINVAL;
		}
		capacity = ( int ) ker_buf[ 0 ];
		if ( capacity < 1 || capacity > 100 )
		{
			pr_err( "Error: invalid capacity\n" );
			return -EINVAL;
		}
		if ( create_deque( cur , capacity ) == FAILURE )
		{
			pr_err( "Error: failed to create deque\n" );
			return -ENOMEM;
		}
		cur->state = PROC_FWRITE;
		return 1;
	}
	else if ( cur->state == PROC_FWRITE )
	{
		// allow only 4 byte buffer size
		if ( ker_buf_size != 4 )
		{
			pr_err( "Error: invalid write\n" );
			return -EINVAL;
		}
		int data = *( int * ) ker_buf;
		if ( cur->deque->size == cur->deque->capacity )
		{
			pr_err( "Error: deque full\n" );
			return -EACCES;
		}
		struct deq_element * new_element = ( struct deq_element * ) kmalloc( sizeof( struct deq_element ) , GFP_KERNEL );
		if ( new_element == NULL )
		{
			pr_err( "Error: failed to create new element\n" );
			return -ENOMEM;
		}
		// insert element at front if integer is odd or at rear if integer is even
		new_element->data = data;
		new_element->next = NULL;
		if ( cur->deque->size == 0 )
		{
			cur->deque->front = new_element;
			cur->deque->rear = new_element;
		}
		else if ( data % 2 == 1 ) // odd
		{
			new_element->next = cur->deque->front;
			cur->deque->front = new_element;
		}
		else // even
		{
			cur->deque->rear->next = new_element;
			cur->deque->rear = new_element;
		}
		cur->deque->size++;
		pr_info( "Process pid %d wrote %d to deque\n" , cur->pid , data );

		return 4;
	}
	return -EINVAL;
}

static ssize_t my_write( struct file * file , const char __user * user_buffer ,
						 size_t count , loff_t * position )
{
	// Write logic
	pid_t pid;
	int ret;
	// stores the the node belonging to proc who is using file
	static struct proc_node * cur;
	mutex_lock( &file_mutex );
	pid = current->pid;
	// check if pid valid
	if ( pid < 0 )
	{
		pr_err( "Invalid pid\n" );
		ret = -EINVAL;
	}
	else
	{
		pr_info( "proc_write() invoked by Process pid %d\n" , pid );
		// check if pid in list
		cur = find_proc_node( pid );
		if ( cur == NULL )
		{
			pr_err( "Process pid %d has not opened this file\n" , pid );
			ret = -EACCES;
		}
		else
		{
			if ( user_buffer == NULL || count == 0 )
			{
				pr_err( "Error: empty write\n" );
				ret = -EINVAL;
			}
			else
			{
				ker_buf_size = count;
				if ( copy_from_user( ker_buf , user_buffer , ker_buf_size ) != 0 )
				{
					pr_err( "Error: copy_from_user failed\n" );
					ret = -EFAULT;
				}
				else
				{
					// write to deque
					ret = helper_write( cur );
				}
				print_deq( cur->deque );
				// debug printing of deque
			}
		}
	}
	mutex_unlock( &file_mutex );
	return ret;
}

// deletes first element from already verified non-empty deque
static void delete_from_front( struct deque * deq )
{
	struct deq_element * temp = deq->front;
	deq->front = deq->front->next;
	deq->size--;
	kfree( temp );
}

static int helper_read( struct proc_node * cur )
{
	// keep reading integer from left of deque and delete them
	// it may be that the deque is not yet created so it will be null
	int data;
	if ( cur->state == PROC_FOPEN )
	{
		pr_err( "Error:Deque not yet intialised for Process pid %d\n" , cur->pid );
		return -EACCES;
	}
	if ( cur->deque->size == 0 )
	{
		pr_err( "Error: deque empty\n" );
		return -EACCES;
	}
	data = cur->deque->front->data;

	// copy this data to ker_buf
	strncpy( ker_buf , ( const char * ) &data , ker_buf_size );
	pr_info( "Process pid %d read %d from deque\n" , cur->pid , data );

	delete_from_front( cur->deque );
	return ker_buf_size;
}

static ssize_t my_read( struct file * file , char __user * user_buffer ,
						size_t count , loff_t * position )
{
	// Read logic
	pid_t pid;
	int ret;
	struct proc_node * cur;
	mutex_lock( &file_mutex );
	pid = current->pid;
	if ( pid < 0 )
	{
		pr_err( "Invalid pid\n" );
		ret = -EINVAL;
	}
	else
	{
		pr_info( "my_read() invoked by Process pid %d\n" , pid );
		cur = find_proc_node( pid );
		if ( cur == NULL )
		{
			pr_err( "Process pid %d has not opened this file\n" , pid );
			ret = -EINVAL;
		}
		else
		{
			ker_buf_size = min( count , ( ssize_t ) MAX_READ_SIZE );
			ret = helper_read( cur );

			if ( ret < 0 )
			{
				pr_err( "Error: failed to read from deque\n" );
			}
			else
			{
				if ( copy_to_user( user_buffer , ker_buf , ker_buf_size ) != 0 )
				{
					pr_err( "Error: copy_to_user failed\n" );
					ret = -EFAULT;
				}
			}
		}
		print_deq( cur->deque );
		// debug printing of deque
	}
	mutex_unlock( &file_mutex );
	return ret;
}

static int my_release( struct inode * inode , struct file * file )
{
	// Release logic and resource cleanup
	pid_t pid;
	int ret;
	struct proc_node * cur;

	mutex_lock( &file_mutex );
	pid = current->pid;
	if ( pid < 0 )
	{
		pr_err( "Invalid pid\n" );
		ret = -EINVAL;
	}
	else
	{
		pr_info( "my_release() invoked by Process pid %d\n" , pid );
		cur = find_proc_node( pid );
		if ( cur == NULL )
		{
			pr_err( "Process pid %d has not opened this file\n" , pid );
			ret = -EACCES;
		}
		else
		{
			// delete proc node
			delete_proc_from_list( cur );
			pr_info( "Process pid %d successfully closed the file\n" , pid );
			ret = 0;
		}
	}
	mutex_unlock( &file_mutex );
	return ret;
}

static struct proc_ops my_ops = {
	.proc_open = my_open,
	.proc_release = my_release,
	.proc_read = my_read,
	.proc_write = my_write,
};

static int __init my_init( void )
{
	proc_file = proc_create( PROC_FILENAME , 0666 , NULL , &my_ops );
	pr_info( "LKM for deque support loaded\n" );
	if ( !proc_file )
	{
		pr_err( "Failed to create /proc/%s entry\n" , PROC_FILENAME );
		return -ENOMEM;
	}

	pr_info( "LKM loaded\n" );
	return 0;
}

// this function deletes all processes in the process list and makes them close this file,i.e clean up their resources
static void delete_process_list( void )
{
	struct proc_node * temp = proc_list;
	while ( temp != NULL )
	{
		struct proc_node * temp2 = temp;
		temp = temp->next;
		delete_proc( temp2 );
	}
}

static void __exit my_exit( void )
{
	// Cleanup resources
	delete_process_list();
	remove_proc_entry( PROC_FILENAME , NULL );
	pr_info( "/proc/%s deleted\n" , PROC_FILENAME );
	pr_info( "LKM unloaded\n" );
}

module_init( my_init );
module_exit( my_exit );
