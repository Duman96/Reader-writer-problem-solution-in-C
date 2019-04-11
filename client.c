#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>
#include <math.h>

#define _GNU_SOURCE
#define BUFSIZE		4096
#define THREADS		20

int connectsock( char *host, char *service, char *protocol );

/*
**	Client
*/
	char		buf[BUFSIZE];
	char		*service;		
	char		*host = "localhost";
	char		*name;
	char		*directory;
	double		rate;
	double		ttimeout;
	int			cc;
	int			csock;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
	int 		total_timeouts = 0;


double poissonRandomInterarrivalDelay( double L )
{
    return (log((double) 1.0 - ((double) rand())/((double) RAND_MAX)))/-L;
}


void *reader_c ( void *s ){

	char filename[30];
	int  i = (int) s;
	int  n;
	int  sret;
	char tempbuf[BUFSIZE];
	struct timeval timeout;
	double limit = ttimeout;
	fd_set readfds;

	timeout.tv_sec = limit;
	timeout.tv_usec = 0;

	/*	Create the socket to the controller  */

	if ( ( csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}

	printf( "The server is ready, please start sending to the server.\n" );
	fflush( stdout );
		
	strcpy(buf, "READ");
	strcat(buf, " ");
	strcat(buf, name);

	// write( csock, buf, strlen(buf) );

	//strcpy(buf, name);

	//write( csock, buf, strlen(buf) );

	if ( write( csock, buf, strlen(buf) ) < 0 )
	{
		fprintf( stderr, "client write: %s\n", strerror(errno) );
		exit( -1 );
	}
	// Read the input and print it out to the screen

    //buf[cc] = '\0';
    //printf( "%s\n", buf );

    FILE *fd;
    ///////STORE THE FILENAME
    sprintf(filename, "./reader/random%d.txt", i);                        

	fd = fopen(filename, "w");

	if(fd == NULL){
		printf("Failed to open file \n");
		exit(-1);
		close(csock);
	}

	FD_ZERO(&readfds);
	FD_SET(csock, &readfds);

	sret = select(8, &readfds, NULL, NULL, &timeout);

	if(sret == 0){
		printf("Timeout of the reader: %ld\n", pthread_self());
		total_timeouts++;
		close(csock);
		pthread_exit(0);
	}

	read( csock, tempbuf, 9);
	printf("%s\n", tempbuf);

	if(tempbuf[0] == 'S' && tempbuf[1] == 'I' && tempbuf[2] == 'Z' && tempbuf[3] == 'E'){

		for( n = 0; n < 4000; n++ ){

			read( csock, buf, 500 );
			fprintf(fd, "%s\n", buf);
		}

		fclose(fd);

		printf("Mission accomplished \n\n"); 
	}	

	close(csock);

	pthread_exit(0);
}

void *writer_c ( void *s ){

	pthread_mutex_lock(&mutex1);

	int i = (int) s;
	int x, y;
	char str[500];
	char tempstr[5];
	int  sret;
	struct timeval timeout;
	double limit = ttimeout;
	fd_set readfds;

	timeout.tv_sec = limit;
	timeout.tv_usec = 0;

	if ( ( csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}

	printf( "The server is ready, please start sending to the server.\n" );
	fflush( stdout );

	strcpy(buf, "WRITE ");
	strcat(buf, name);

	write( csock, buf, strlen(buf) );

	FD_ZERO(&readfds);
	FD_SET(csock, &readfds);

	sret = select(8, &readfds, NULL, NULL, &timeout);

	if(sret == 0){
		printf("Timeout of the reader: %ld\n", pthread_self());
		total_timeouts++;
		close(csock);
		pthread_exit(0);
	}


	if ( (cc = read( csock, buf, BUFSIZE )) <= 0 )
    {
    	printf( "The server has gone.\n" );
        close(csock);
    }

    if(buf[0] != 'G' && buf[1] != 'O'){
    	close(csock);
    }

    strcpy(buf, "SIZE:");
    strcat(buf, " ");
    strcat(buf, "2MB");

	// Send to the server
	write(csock, buf, strlen(buf));

	for( y = 0; y < 500; y++){

		sprintf(tempstr, "%d", i);
		strcat(str, tempstr);

	}

	for( x = 0; x < 4000; x++ ){

		write( csock, str, strlen(str) );

	}

	//printf("I am here\n");

	if ( write( csock, str, strlen(str) ) < 0 )
	{
		fprintf( stderr, "client write: %s\n", strerror(errno) );
		exit( -1 );
	}
	//printf("Also here\n");

	// Read the echo and print it out to the screen
	if ( (cc = read( csock, buf, BUFSIZE )) <= 0 )
    {
    	printf( "The server has gone.\n" );
        close(csock);
    }
    //printf("And here\n");

    pthread_mutex_unlock(&mutex1);

    pthread_exit(0);
            
}



int
main( int argc, char *argv[] )
{
	int status, i, j;
	pthread_t threads[THREADS];


	if(pthread_mutex_init(&mutex1, NULL) != 0){
		printf("\nMutex1 init has failed\n");
		return 1;
	}

	if(pthread_mutex_init(&mutex2, NULL) != 0){
		printf("\nMutex2 init has failed\n");
		return 1;
	}

	/**********************************READER*********************************/
	if(strcmp(argv[1], "rclient") == 0){

		printf("Reader is connecting\n");

			switch( argc ) 
		{
			case    7:
				service = argv[2];
				rate = atof(argv[3]);
				name = argv[4];
				directory = argv[5];
				ttimeout = atof(argv[6]);

				break;
			case    8:
				host = argv[2];
				service = argv[3];
				rate = atof(argv[4]);
				name = argv[5];
				directory = argv[6];
				ttimeout = atof(argv[7]);
				break;
			case 	2:
				fprintf( stderr, "usage: chat [host] port\n" );
				exit(-1);
			case	4:
				service = argv[2];
				name = argv[3];
		}



		for(i = 0, j = 0; i < THREADS; i++){

			int h = poissonRandomInterarrivalDelay(rate);
			//printf("%i\n", h);

			//printf("Created0\n");
			status = pthread_create( &threads[j++], NULL, reader_c, (void *) i );
			//printf("Created1\n");
			if ( status != 0 )
			{
				printf( "pthread_create error %d\n", status );
				exit( -1 );
			}
			//printf("Created2\n");
			sleep(3);
		}
		for ( j = 0; j < THREADS; j++ ){
			pthread_join( threads[j], NULL );
		}

	}

	/**********************************WRITER*********************************/
	if (strcmp(argv[1], "wclient") == 0){

		printf("Writer is connecting \n");
		
		switch( argc ) 
		{
			case    6:
				service = argv[2];
				rate = atof(argv[3]);
				name = argv[4];
				ttimeout = atof(argv[5]);

				break;
			case    7:
				host = argv[2];
				service = argv[3];
				rate = atof(argv[4]);
				name = argv[5];
				ttimeout = atof(argv[6]);
				break;
			case    4:
				name = argv[3];
				service = argv[2];
				break;
			case 	2:
				fprintf( stderr, "usage: chat [host] port\n" );
				exit(-1);
		}
		for(i = 0, j = 0; i < THREADS; i++){

			int w = poissonRandomInterarrivalDelay(rate);

			//printf("Created0\n");
			status = pthread_create( &threads[j++], NULL, writer_c, (void *) i );
			//printf("Created1\n");
			if ( status != 0 )
			{
				printf( "pthread_create error %d\n", status );
				exit( -1 );
			}
			//printf("Created2\n");
			sleep(w);
		}
		for ( j = 0; j < THREADS; j++ ){
			pthread_join( threads[j], NULL );
		}
		pthread_mutex_destroy(&mutex1);
		//pthread_mutex_destroy(&mutex2);
	}
}


