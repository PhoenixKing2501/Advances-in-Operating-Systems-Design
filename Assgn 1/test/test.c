#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main()
{
	int n = 2;

	for ( int i = 0; i < n; ++i )
	{
		pid_t pid = fork();

		if ( pid < 0 )
		{
			perror( "fork" );
		}

		else if ( pid == 0 )
		{
			char * args[] = { "./main" , NULL };
			execvp( args[ 0 ] , args );
		}
	}

	for ( int i = 0; i < n; ++i )
	{
		wait( NULL );
	}


}
