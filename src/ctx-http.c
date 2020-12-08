#include "ctx-http.h"

//Define an interval for waiting
static const struct timespec __interval__ = { 0, 100000000 };

//No-op
void create_notls ( void **p ) { ; }

//Read a message that the server will use later.
int read_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Read started...\n" );
	const int size = 1024; 
	int mult = 1;
	char err[ 2048 ] = {0};
	struct cdata *conn = (struct cdata *)p;

	//Read first
	for ( ;; ) {

		int flags, rd, nsize = size * mult;
		unsigned char *ptr = NULL;

		if ( ( ptr = rq->msg = realloc( rq->msg, nsize ) ) == NULL ) { 
			return http_set_error( rs, 500, "Could not allocate read buffer." ); 
		}

		if ( !memset( ptr += (nsize - size), 0, size ) ) {
			return http_set_error( rs, 500, "Could not zero out new read buffer." ); 
		}

		//Read a message
		rd = recv( fd, ptr, size, MSG_DONTWAIT );
		//FPRINTF( "const bsize = %d, msgbuf size = %d, start pos = %d, recvd = %d, recvd so far = %d\n", size, nsize, nsize - size, rd, rq->mlen );
	
		if ( rd == 0 ) {
			//you get a few times to try?  then just cut out and tell the server to try again?
			conn->count = -2;
			return 0;		
		}
		else if ( rd < 1 ) {
			//Sleep before your next try	
			nanosleep( &__interval__, NULL ); 

			//A subsequent call will tell us a lot...
			FPRINTF( "Couldn't read all of message...\n" );
			//whatsockerr( errno );
			if ( errno == EAGAIN || errno == EWOULDBLOCK )
				FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			else {
				//Any of these are probably fatal
				//EBADF || ECONNREFUSED || EFAULT || EINTR || EINVAL || ENOMEM || ENOTCONN || ENOTSOCK )
				conn->count = -2;
				return 0;
			}
		}
		else {
			//Define a temporary body
			struct HTTPBody *tmp;

			//Increment message length
			rq->mlen += rd;

			//We have to try to parse, the first go MAY not work
			tmp = http_parse_request( rq, err, sizeof(err) ); 
			print_httpbody( tmp );

			if ( tmp->error == ZHTTP_NONE ) { 
				FPRINTF( "All data received\n" );
				break;
			}
			else if ( tmp->error == ZHTTP_INCOMPLETE_HEADER ) {
				FPRINTF( "Got non-fatal HTTP parser error: %s.  Try reading again...\n", err );
			}
			else {
				//send a 500 with what went wrong
				FPRINTF( "Got fatal HTTP parser error: %s\n", err );
				conn->count = -1;
				return http_set_error( rs, 500, err ); 
			}
			mult++;
			//show read progress and data received, etc.
			FPRINTF( "Received %d bytes on fd %d\n", rd, fd ); 
		}
	}

	return 1;
}



//Write
int write_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Write started...\n" );
	int sent = 0, pos = 0, try = 0;
	int total = rs->mlen;
	unsigned char *ptr = rs->msg;
	struct cdata *conn = (struct cdata *)p;

	for ( ;; ) {	
		sent = send( fd, ptr, total, MSG_DONTWAIT );
		FPRINTF( "Bytes sent: %d\n", sent );

		if ( sent == 0 ) {
			FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
			break;	
		}
		else if ( sent > -1 ) {
			pos += sent, total -= sent, ptr += sent;	
			FPRINTF( "sent == %d, %d bytes remain to be sent...\n", sent, total );
			if ( total == 0 ) {
				FPRINTF( "sent == 0, assuming all %d bytes have been sent...\n", rs->mlen );
				break;
			}
		}
		else if ( sent == -1 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
			FPRINTF( "Tried %d times to write to socket. Trying again?\n", try );
		}
		else {
			//TODO: Can't close a most-likely closed socket.  What do you do?
			if ( sent == -1 && ( errno == EAGAIN || errno == EWOULDBLOCK ) )
				FPRINTF( "Tried %d times to write to socket. Trying again?\n", try );
			else {
				//EBADF|ECONNREFUSED|EFAULT|EINTR|EINVAL|ENOMEM|ENOTCONN|ENOTSOCK
				FPRINTF( "Got socket write error: %s\n", strerror( errno ) );
				conn->count = -2;
				return 0;	
			}
		}
		try++;
		FPRINTF( "Bytes sent: %d, leftover: %d\n", pos, total );
	}
	FPRINTF( "Write complete.\n" );
	return 1;
}


#if 0
//Destroy anything
void free_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *p ) {
	FPRINTF( "Deallocation started...\n" );

	//Free the HTTP body 
	http_free_body( rs );
	http_free_body( rq );

	//Close the file
	if ( close( fd ) == -1 ) {
		FPRINTF( "Couldn't close child socket. %s\n", strerror(errno) );
	}

	FPRINTF( "Deallocation complete.\n" );
}
#endif
