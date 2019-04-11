#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>

#define	QLEN			5
#define	BUFSIZE			4096

int passivesock( char *service, char *protocol, int qlen, int *rport );

char		*readerstr[20];
char		*writerstr[20];
char		rtempbuffer[BUFSIZE];
char		wtempbuffer[BUFSIZE];
char 		rfilename[20];
char		wfilename[20];
sem_t 		resource;
sem_t 		rmutex;
int 		readcount = 0;
int 		writecount = 0;
sem_t   	wprmutex, wpwmutex, readTry, wpresource; 


/////////////////////////////////////READER PREFERENCE///////////////////////////////////////
void *rwriter( void *s )
{
	sem_wait(&resource);

	char buf[BUFSIZE];
	char tempbuf[BUFSIZE];
	int cc, x;
	int ssock = (int) s;
	//printf("%s\n", filename);

	strcpy(buf, "GO");
	strcat(buf, " ");
	strcat(buf, wfilename);

	if ( write( ssock, buf, strlen(buf) ) < 0 )
	{
		printf("I am busy\n");
		close(ssock);
		pthread_exit(0);
	}
	printf("%s\n", buf);
	/* start working for this guy */
	/* ECHO what the client says */

	//printf("I am here\n");
	

	//printf("%s\n", buffer);
	FILE *fp;

	fp = fopen("random.txt", "w");

	//printf("And here\n");

	if(fp == NULL){
		printf("Cannot open file \n");
		close(ssock);
	}
	read(ssock, tempbuf, 9);

	printf("%s\n", tempbuf);


	for( x = 0; x < 4000; x++ ){

		cc = read(ssock, buf, 500);		

		fprintf(fp, "%s\n", buf);

		//printf( "The client says: %s\n", buf );
		if ( write( ssock, buf, cc ) < 0 )
		{
			/* This guy is dead */
			close( ssock );		
		}
	}
	buf[cc] = '\0';

	fclose(fp);

	//printf("I am here\n");

	if ( (cc = read( ssock, buf, BUFSIZE )) <= 0 )
	{
		printf( "The client has gone.\n" );
		close(ssock);
	}

	close(ssock);

	sem_post(&resource);
	
	pthread_exit(0);
}

//**********THREAD FOR READER*************//
void *rreader( void *s )
{

	sem_wait(&rmutex);

	readcount++;

	if(readcount == 1){
		sem_wait(&resource);
	}

	sem_post(&rmutex);


	char buf[BUFSIZE];
	int  cc;
	char temp[256];
	int  i;
	int  ssock = (int) s;
	//int	 leng;
	/* start working for this guy */
	/* ECHO what the client says */
	//buf[cc] = '\0';

	
	FILE *fp;
	
	//printf("%s\n", filename);
	//leng = strlen(readerstr[0]) + strlen(readerstr[1]);
	//printf("%i\n", leng);
	//memset(readerstr, leng, 0);

	fp = fopen("random.txt", "r");

	if(fp != NULL){
		printf("File opened successfully \n");
	}else{
		printf("Failed to open file \n");
		close(ssock);
	}
	/*fseek( fp, 0, SEEK_END);
	printf("%ld\n", ftell(fp));
	fflush(stdout);
	fseek( fp, 0, SEEK_SET);*/

	strcpy(buf, "SIZE:");
    strcat(buf, " ");
    strcat(buf, "2MB");

    write(ssock, buf, strlen(buf));
    printf("%s\n", buf);

	// char str[9];
	// fscanf(fp, "%s", str);

	fgets(temp, 500, fp);	

	//printf("GG\n");

	for( i = 0; i < 4000; i++ ){

		if(write( ssock, temp, 500 ) < 0){
			printf("Error\n");
		}

	}

	fclose(fp);

	sem_wait(&rmutex);

	readcount--;

	if(readcount == 0){
		sem_post(&resource);
	}

	sem_post(&rmutex);
	
	pthread_exit(0);
}

//////////////////////////////WRITER PREFERENCE//////////////////////////////////

void *wwriter( void *s )
{
	sem_wait(&wpwmutex);

	writecount++;
	if(writecount == 1){
		sem_wait(&readTry);
	}

	sem_post(&wpwmutex);

	sem_wait(&wpresource);

	char buf[BUFSIZE];
	char tempbuf[BUFSIZE];
	int cc, x;
	int ssock = (int) s;
	//printf("%s\n", filename);

	strcpy(buf, "GO");
	strcat(buf, " ");
	strcat(buf, wfilename);

	if ( write( ssock, buf, strlen(buf) ) < 0 )
	{
		printf("I am busy\n");
		close(ssock);
		pthread_exit(0);
	}
	printf("%s\n", buf);
	/* start working for this guy */
	/* ECHO what the client says */

	//printf("I am here\n");
	

	//printf("%s\n", buffer);
	FILE *fp;

	fp = fopen("random.txt", "w");

	//printf("And here\n");

	if(fp == NULL){
		printf("Cannot open file \n");
		close(ssock);
	}
	read(ssock, tempbuf, 9);

	printf("%s\n", tempbuf);


	for( x = 0; x < 4000; x++ ){

		cc = read(ssock, buf, 500);		

		fprintf(fp, "%s\n", buf);

		//printf( "The client says: %s\n", buf );
		if ( write( ssock, buf, cc ) < 0 )
		{
			/* This guy is dead */
			close( ssock );		
		}
	}
	buf[cc] = '\0';

	fclose(fp);

	//printf("I am here\n");

	if ( (cc = read( ssock, buf, BUFSIZE )) <= 0 )
	{
		printf( "The client has gone.\n" );
		close(ssock);
	}

	close(ssock);
	sem_post(&wpresource);

	sem_wait(&wpwmutex);

	writecount--;
	if(writecount == 0){
		sem_post(&readTry);
	}

	sem_post(&wpwmutex);
	
	pthread_exit(0);
}

//**********THREAD FOR READER*************//
void *wreader( void *s )
{

	sem_wait(&readTry);
	sem_wait(&wprmutex);

	readcount++;

	if(readcount == 1){
		sem_wait(&wpresource);
	}

	sem_post(&wprmutex);
	sem_post(&readTry);


	char buf[BUFSIZE];
	int  cc;
	char temp[256];
	int  i;
	int  ssock = (int) s;
	//int	 leng;
	/* start working for this guy */
	/* ECHO what the client says */
	//buf[cc] = '\0';

	
	FILE *fp;
	
	//printf("%s\n", filename);
	//leng = strlen(readerstr[0]) + strlen(readerstr[1]);
	//printf("%i\n", leng);
	//memset(readerstr, leng, 0);

	fp = fopen("random.txt", "r");

	if(fp != NULL){
		printf("File opened successfully \n");
	}else{
		printf("Failed to open file \n");
		close(ssock);
	}
	/*fseek( fp, 0, SEEK_END);
	printf("%ld\n", ftell(fp));
	fflush(stdout);
	fseek( fp, 0, SEEK_SET);*/

	strcpy(buf, "SIZE:");
    strcat(buf, " ");
    strcat(buf, "2MB");

    write(ssock, buf, strlen(buf));
    printf("%s\n", buf);

	// char str[9];
	// fscanf(fp, "%s", str);

	fgets(temp, 500, fp);	

	//printf("GG\n");

	for( i = 0; i < 4000; i++ ){

		if(write( ssock, temp, 500 ) < 0){
			printf("Error\n");
		}

	}

	fclose(fp);

	sem_wait(&wprmutex);

	readcount--;

	if(readcount == 0){
		sem_post(&wpresource);
	}

	sem_post(&wprmutex);
	
	pthread_exit(0);
}

/*
*/
int
main( int argc, char *argv[] )
{
	char		*service;
	struct sockaddr_in	fsin;
	int			alen;
	int			msock;
	int			rport = 0;
	int			maincc;
	char		*tokenw;
	int			t = 0;
	char 		buffer[BUFSIZE];
	sem_init(&resource, 0, 1);
	sem_init(&rmutex, 0, 1);
	char		*pref;
	sem_init(&wprmutex, 0, 1);
	sem_init(&wpwmutex, 0, 1);
	sem_init(&readTry, 0, 1);
	sem_init(&wpresource, 0, 1);
	
	switch (argc) 
	{
		case	2:
			// No args? let the OS choose a port and tell the user
			rport = 1;
			pref = argv[1];

			break;
		case	3:
			// User provides a port? then use it
			service = argv[2];
			pref = argv[1];
			break;
		default:
			fprintf( stderr, "usage: server [port]\n" );
			exit(-1);
	}

	msock = passivesock( service, "tcp", QLEN, &rport );
	if (rport)
	{
		//	Tell the user the selected port
		printf( "server: port %d\n", rport );	
		fflush( stdout );
	}

	
	for (;;)
	{
		int	ssock;
		pthread_t	thr;

		alen = sizeof(fsin);
		ssock = accept( msock, (struct sockaddr *)&fsin, &alen );
		if (ssock < 0)
		{
			fprintf( stderr, "accept: %s\n", strerror(errno) );
			break;
		}

		if(strcmp(pref, "rp")){

			maincc = read(ssock, buffer, BUFSIZE);

			buffer[maincc] = '\0';

			if(buffer[0] == 'R' && buffer[1] == 'E' && buffer[2] == 'A' && buffer[3] == 'D')
			{

				/*strcpy(rtempbuffer, buffer);
				readerstr[0] = strtok(rtempbuffer, " ");

				while(readerstr[t] != NULL){

					readerstr[++t] = strtok(NULL, " ");
					
				}
				strcpy( rfilename, readerstr[1] );*/
				//while (buffer[i+5] != '\0')
				
				/*printf("#%s#\n", filename);
				fflush( stdout );*/

				printf( "A client has arrived for reading file.\n" );
				fflush( stdout );
				pthread_create( &thr, NULL, rreader, (void *)ssock );

			} 
			else if (buffer[0] == 'W' && buffer[1] == 'R' && buffer[2] == 'I' && buffer[3] == 'T' && buffer[4] == 'E')
			{

				/*strcpy(wtempbuffer, buffer);
				writerstr[0] = strtok(wtempbuffer, " ");

				while(writerstr[t] != NULL){

					writerstr[++t] = strtok(NULL, " ");
					
				}
				strcpy( wfilename, writerstr[1] );*/

				printf( "A client has arrived for writing to a file.\n" );
				fflush( stdout );
				pthread_create( &thr, NULL, rwriter, (void *)ssock );

			
			} /*else {
				printf("Unknown command");
				write(ssock, "Unknown command. Try again", 56);
			}*/
		}
		if(strcmp(pref, "wp")){

			maincc = read(ssock, buffer, BUFSIZE);

			buffer[maincc] = '\0';

			if(buffer[0] == 'R' && buffer[1] == 'E' && buffer[2] == 'A' && buffer[3] == 'D')
			{

				/*strcpy(rtempbuffer, buffer);
				readerstr[0] = strtok(rtempbuffer, " ");

				while(readerstr[t] != NULL){

					readerstr[++t] = strtok(NULL, " ");
					
				}
				strcpy( rfilename, readerstr[1] );*/
				//while (buffer[i+5] != '\0')
				
				/*printf("#%s#\n", filename);
				fflush( stdout );*/

				printf( "A client has arrived for reading file.\n" );
				fflush( stdout );
				pthread_create( &thr, NULL, wreader, (void *)ssock );

			} 
			else if (buffer[0] == 'W' && buffer[1] == 'R' && buffer[2] == 'I' && buffer[3] == 'T' && buffer[4] == 'E')
			{

				/*strcpy(wtempbuffer, buffer);
				writerstr[0] = strtok(wtempbuffer, " ");

				while(writerstr[t] != NULL){

					writerstr[++t] = strtok(NULL, " ");
					
				}
				strcpy( wfilename, writerstr[1] );*/

				printf( "A client has arrived for writing to a file.\n" );
				fflush( stdout );
				pthread_create( &thr, NULL, wwriter, (void *)ssock );

			
			} 

		}

	}
	pthread_exit(0);
}

