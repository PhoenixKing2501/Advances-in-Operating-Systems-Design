#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*checks synchronization and exclusion between processes*/

#define PROC_FILENAME "partb_1_20CS30062_20CS30057"

int main()
{
	int n = 2;
	
	for ( size_t i = 0; i < n; ++i )
	{
		pid_t pid = fork();

		if ( pid < 0 )
		{
			perror( "fork" );
			exit( 1 );
		}

		else if ( pid == 0 )
		{
			execl( "./test_01" , "./test_01" , NULL );
			perror( "execvp" );
		}
	}

	for ( size_t i = 0; i < 5; ++i )
	{
		wait( NULL );
	}
}

