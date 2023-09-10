#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PROC_FILENAME "partb_1_20CS30062_20CS30057"
/*checks passing of invalid arguments (2 bytes instead of 4 bytes)*/

int main()
{
	int fd = open( "/proc/" PROC_FILENAME , O_RDWR );

	if ( fd < 0 )
	{
		perror( "open" );
		exit( 1 );
	}

	char ch = ( char ) 10;

	int ret = write( fd , &ch , 1 );

	if ( ret < 0 )
	{
		perror( "write" );
		close( fd );
		exit( 1 );
	}

	srand( 42 );

	int n = ch + 10;

	int * arr = ( int * ) malloc( n * sizeof( int ) );
	int last = 0;

	for ( size_t i = 0; i < n; ++i )
	{
		int num = rand() % 100;
		printf( "Writing %d\n" , num );
		ret = write( fd , &num , /* sizeof( int ) */ 2 );
		if ( ret < 0 )
		{
			perror( "write" );
		}

		else
		{
			if ( num % 2 == 1 ) // if odd insert left in arr
			{
				for ( size_t j = last; j > 0; --j )
				{
					arr[ j ] = arr[ j - 1 ];
				}

				arr[ 0 ] = num;

				++last;
			}
			else // if even insert right in arr
			{
				arr[ last++ ] = num;
			}
		}
		// sleep( 1 );
	}

	for ( size_t i = 0; i < last; ++i )
	{
		printf( "%d " , arr[ i ] );
	}
	printf( "\n" );

	free( arr );

	for ( size_t i = 0; i < n; ++i )
	{
		int num;
		ret = read( fd , &num , sizeof( int ) );
		if ( ret < 0 )
		{
			perror( "read" );
		}
		else
			printf( "Read %d\n" , num );
	}

	close( fd );
}

