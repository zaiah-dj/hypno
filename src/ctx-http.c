/* ------------------------------------------- * 
 * ctx-http.c
 * ========
 * 
 * Summary 
 * -------
 * Functions for dealing with HTTP contexts.
 *
 * Usage
 * -----
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include "ctx-http.h"

//Define an interval for polling 
static const struct timespec __interval__ = { 0, 100000000 };


//No-op
void create_notls ( void **p ) { ; }

static void dumpconn( struct cdata *conn, const char *fname ) {
	if ( conn ) {	

		fprintf( stderr, "SERVER INFO\n" );
		fprintf( stderr, "[%s] count: %d\n", fname, conn->count );
		fprintf( stderr, "[%s] status: %d\n", fname, conn->status );
		fprintf( stderr, "[%s] ipv4: %s\n", fname, conn->ipv4 );
		fprintf( stderr, "[%s] ipv6: %s\n", fname, conn->ipv6 );

		fprintf( stderr, "SERVER CONFIG\n" );
		fprintf( stderr, "[%s] ptr: %p\n", fname, conn->config );
		if ( conn->config ) {
			fprintf( stderr, "[%s] wwwroot: %s\n", fname, conn->config->wwwroot );
			fprintf( stderr, "[%s] hosts: %p\n", fname, conn->config->hosts );
		}

#if 0
		fprintf( stderr, "LOCAL CONFIG\n" );
		fprintf( stderr, "[%s] ptr : %p\n", fname, conn->hconfig );
		if ( conn->hconfig ) {
			fprintf( stderr, "[%s] name	: %s\n", fname, conn->hconfig->name	 );
			fprintf( stderr, "[%s] alias: %s\n", fname, conn->hconfig->alias );
			fprintf( stderr, "[%s] dir	: %s\n", fname, conn->hconfig->dir	 );
			fprintf( stderr, "[%s] filter	: %s\n", fname, conn->hconfig->filter	 );
			fprintf( stderr, "[%s] root_default	: %s\n", fname, conn->hconfig->root_default	 );
			fprintf( stderr, "[%s] ca_bundle: %s\n", fname, conn->hconfig->ca_bundle );
			fprintf( stderr, "[%s] certfile: %s\n", fname, conn->hconfig->certfile );
			fprintf( stderr, "[%s] keyfile: %s\n", fname, conn->hconfig->keyfile );
		}
#endif
	}
}


const int pre_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {
dumpconn( conn, __func__ );
	return 1;
}


//Read a message that the server will use later.
const int read_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {
	FPRINTF( "Read started...\n" );
dumpconn( conn, __func__ );
	int mult = 1, size = 1024; 
	char err[ 2048 ] = {0};

	//Get the time at the start
	struct timespec timer = {0};
	clock_gettime( CLOCK_REALTIME, &timer );	

	//Allocate space for the first call
	if ( ( rq->msg = malloc( size ) ) == NULL ) {
		return http_set_error( rs, 500, "Could not allocate initial read buffer." ); 
	} 

	//Read first
	for ( ;; ) {

		int flags, rd, nsize = size * mult;
		unsigned char *ptr = rq->msg;
		ptr += ( nsize - size );

		if ( ( rd = recv( fd, ptr, size, MSG_DONTWAIT ) ) == 0 ) {
			conn->count = -2; //most likely resources are unavailable
			return 0;		
		}
		else if ( rd < 1 ) {
			struct timespec n = {0};
			clock_gettime( CLOCK_REALTIME, &n );

			if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
				conn->count = -2;
				return 0;
			}

			if ( ( n.tv_sec - timer.tv_sec ) > 5 ) {
				conn->count = -3;
				return http_set_error( rs, 408, "Timeout reached." );
			}

			FPRINTF("Trying again to read from socket. Got %d bytes.\n", rd );
			nanosleep( &__interval__, NULL );
		}
		else {
			rq->mlen += rd;
			struct HTTPBody *tmp = http_parse_request( rq, err, sizeof(err) ); 
	
			//TODO: Is this handling everything?
			if ( tmp->error == ZHTTP_NONE ) { 
				FPRINTF( "All data received\n" );
				break;
			}
			else if ( tmp->error != ZHTTP_INCOMPLETE_HEADER ) { // && tmp->error !=	ZHTTP_INCOMPLETE_REQUEST
				FPRINTF( "Got fatal HTTP parser error: %s\n", err );
				conn->count = -3;
				return http_set_error( rs, 500, err ); 
			}

			if ( !( rq->msg = realloc( rq->msg, nsize ) ) || !memset( &rq->msg[ nsize - size ], 0, size ) ) {
				return http_set_error( rs, 500, "Could not allocate read buffer." ); 
			}

			FPRINTF( "Received %d bytes on fd %d\n", rd, fd ); 
			clock_gettime( CLOCK_REALTIME, &timer );	
			mult++;
		}
	}
dumpconn( conn, __func__ );
	return 1;
}


//Write
const int write_notls ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {
	FPRINTF( "Write started...\n" );
dumpconn( conn, __func__ );
	int sent = 0, pos = 0, try = 0;
	int total = rs->mlen;
	unsigned char *ptr = rs->msg;

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
