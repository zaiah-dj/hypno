/*let's try this again.  It seems never to work like it should...
*/
#include "vendor/single.h"

struct HTTPBody {
	int clen;  //content length
	int mlen;  //message length (length of the entire received message)
	int	hlen;  //header length
	int status; //what was this?
	char      *stext; //status text ptr
	char      *ctype; //content type ptr
#if 0	
	char      method[HTTP_METHOD_MAX];  //one of 7 methods
	char      protocol[HTTP_PROTO_MAX]; //
	char      path[HTTP_URL_MAX];       //The requested path
	char      host[1024];               //safe bet for host length
	char      boundary[128];            //The boundary
#else
	char      *method;
	char      *protocol;
	char      *path;
	char      *host;
	char      *boundary;
#endif
 	uint8_t   *msg;

	//Simple data structures.  like headers on both sides...
	//Can't think if you need hash tables or not...
	Table     table;
	//char **headers;
};



struct sockAbstr {
	int addrsize;
	int buffersize;
	int opened;
	int backlog;
	int waittime;
	int protocol;
	int socktype;
	int fd;
	int iptype; //ipv4 or v6
	int reuse;
	int family;
	int *port;
	struct sockaddr_in *sin;	
	void *ssl_ctx;
};


/*???*/
struct Loop {
//	Type type;	//what other type would there be?
	struct pollfd   *client;  //Pointer to currently being served client
	int bytes;  //bytes received or sent
	int *fd;  //???
	int retries;
  unsigned char *msg;
	void          *userdata;
	//struct timespec start, end;
	int      connNo; 
};




typedef enum {
	HTTP_100 = 100,
	HTTP_101 = 101,
	HTTP_200 = 200,
	HTTP_201 = 201,
	HTTP_202 = 202,
	HTTP_204 = 204,
	HTTP_206 = 206,
	HTTP_300 = 300,
	HTTP_301 = 301,
	HTTP_302 = 302,
	HTTP_303 = 303,
	HTTP_304 = 304,
	HTTP_305 = 305,
	HTTP_307 = 307,
	HTTP_400 = 400,
	HTTP_401 = 401,
	HTTP_403 = 403,
	HTTP_404 = 404,
	HTTP_405 = 405,
	HTTP_406 = 406,
	HTTP_407 = 407,
	HTTP_408 = 408,
	HTTP_409 = 409,
	HTTP_410 = 410,
	HTTP_411 = 411,
	HTTP_412 = 412,
	HTTP_413 = 413,
	HTTP_414 = 414,
	HTTP_415 = 415,
	HTTP_416 = 416,
	HTTP_417 = 417,
	HTTP_418 = 418,
	HTTP_500 = 500,
	HTTP_501 = 501,
	HTTP_502 = 502,
	HTTP_503 = 503,
	HTTP_504 = 504
} HTTP_Status;


static const char *http_status[] = {
	[HTTP_100] = "Continue",
	[HTTP_101] = "Switching Protocols", 
	[HTTP_200] = "OK",
	[HTTP_201] = "Created",
	[HTTP_202] = "Accepted",
	[HTTP_204] = "No Content",
	[HTTP_206] = "Partial Content",
	[HTTP_300] = "Multiple Choices",
	[HTTP_301] = "Moved Permanently",
	[HTTP_302] = "Found",
	[HTTP_303] = "See Other",
	[HTTP_304] = "Not Modified",
	[HTTP_305] = "Use Proxy",
	[HTTP_307] = "Temporary Redirect",
	[HTTP_400] = "Bad Request",
	[HTTP_401] = "Unauthorized",	
	[HTTP_403] = "Forbidden",			
	[HTTP_404] = "Not Found",				
	[HTTP_405] = "Method Not Allowed",
	[HTTP_406] = "Not Acceptable",
	[HTTP_407] = "Proxy Authentication Required",
	[HTTP_408] = "Request Timeout",
	[HTTP_409] = "Conflict",
	[HTTP_410] = "Gone",
	[HTTP_411] = "Length Required",
	[HTTP_412] = "Precondition Failed",
	[HTTP_413] = "Request Entity Too Large",
	[HTTP_414] = "Request URI Too Long",
	[HTTP_415] = "Unsupported Media Type",
	[HTTP_416] = "Requested Range",
	[HTTP_417] = "Expectation Failed",
	[HTTP_418] = "I'm a teapot",
	[HTTP_500] = "Internal Server Error",
	[HTTP_501] = "Not Implemented",
	[HTTP_502] = "Bad Gateway",
	[HTTP_503] = "Service Unavailable",
	[HTTP_504] = "Gateway Timeout"
};


//TODO: I want no static global variables anywhere
static const char content_string[] = "content";


//Get space between
char *get_lstr( char **str, char chr, int *lt ) {
	//find string, clone string and increment ptr
	int r;
	char *rr = NULL;
	if (( r = memchrat( *str, chr, *lt ) ) == -1 ) {
		rr = malloc( *lt );
		memset( rr, 0, *lt );
		memcpy( rr, *str, *lt );	
		rr[ *lt - 1 ] = '\0';
	}	
	else {
		rr = malloc( r );
		memset( rr, 0, r );
		memcpy( rr, *str, r );	
		rr[ r ] = '\0';
		*str += r + 1;
		*lt -= r;
	}

	return rr;
}



#define REMOVE_ME
#ifdef REMOVE_ME 
//Get the method, version and location
int msg_parse_first_line (struct HTTPBody *h, uint8_t *msg, int len) {
	char *a[] = { h->method, h->path, h->protocol };
	//Define stuff
	Parser p = { .words={{" "},{"\r"},{NULL}} }; 
	pr_prepare( &p );
	int tlen = 0;
#if 0

	if ((tlen = memchrat( &msg[0], '\n', len )) == -1)
		return 0;  //This was a fail...  specify the error somewhere....

#endif
	for ( int i=0; i<3 && pr_next( &p, &msg[0], tlen + 1 ); i++ ) {
	#if 0
		memset( tt[i].ptr, 0, tt[i].maxlen );
		if ( p.size + 1 > tt[i].maxlen ) {
			return 0; //Also was a fail, specify error...
		}
		memcpy( tt[i].ptr, &msg[ p.prev ], p.size );
		tt[i].ptr[ p.size ] = '\0';	
	#else
		memcpy(	(a[i] = malloc( p.size + 1 )), &msg[ p.prev ], p.size );
		a[i][p.size] = '\0';
	#endif
	}

	return 1;
}
#endif


//Set the content length
int msg_get_content_length (uint8_t *msg, int len) {
	int a, b;
	char lc[ 128 ];
	if ((a = memstrat( msg, "Content-Length", len )) > -1) {
		//Catch any misses...
		msg += 16;
		if ((b = memchrat( msg, '\r', len - a )) > 127 || b == -1 ) return 0; 

		//Copy to string
		memset( lc, 0, b );
		memcpy( lc, msg, b );//&msg[a], b ); //lc[ b ] = '\0';

		//Make sure that content-length numeric
		for ( int i=0; i < strlen(lc); i++ )  {
			if ( (int)lc[i] < 48 || (int)lc[i] > 57 ) {
				return 0;
			}
		}

		//Set the content-length
		return atoi( lc );
	}
	return 0;
}


//Extract value (a simpler code that can be used to grab values)
char *msg_get_value ( const char *value, const char *end, uint8_t *msg, int len ) {
	int bStart=0, bEnd=0;
	char *bContent = NULL;
	//Ugly boundary extraction
	if ( (bStart = memstrat( msg, "boundary=", len )) > -1) {
		bStart += 9;
		msg += bStart;
		bEnd = memchrat( msg, '\r', len - bStart); 
		bEnd = (bEnd > -1) ? bEnd : len - bStart; 
		bContent = malloc( bEnd + 1 );
		memset( bContent, 0, bEnd + 1 );	
		memcpy( bContent, msg, bEnd );
	}
}


//Get boundary
char *msg_get_boundary ( uint8_t *msg, int len ) {
	int bStart=0, bEnd=0;
	char *bContent = NULL;
	//Ugly boundary extraction
	if ( (bStart = memstrat( msg, "boundary=", len )) > -1) {
		bStart += 9;
		msg += bStart;
		bEnd = memchrat( msg, '\r', len - bStart); 
		bEnd = (bEnd > -1) ? bEnd : len - bStart; 
		bContent = malloc( bEnd + 1 );
		memset( bContent, 0, bEnd + 1 );	
		memcpy( bContent, msg, bEnd );
	}
	return bContent; 
}


//Get the distance from the headers on
int msg_get_header_length (struct HTTPBody *h, uint8_t *msg, int32_t len) {
	//return ((h->hlen = memstrat(msg, "\r\n\r\n", len)) == -1) ? 0 : 1;
return 0;
}



//Figure out the message length
int msg_get_message_length (struct HTTPBody *h, uint8_t *msg, int32_t len) {
	//if these are equal, then we haven't gotten the full thing yet...
#if 0
	if ( h->hlen + 4 == len )
		return 0;
	r->mlen = len - (r->hlen + 4);	
#endif
	return 1;	
}



//Trim whitespace
unsigned char *httpvtrim (uint8_t *msg, int len, int *nlen) {
	//Define stuff
	uint8_t *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr("\r\n\t ", *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr("\r\n\t ", *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}


//Trim any characters 
unsigned char *httptrim (uint8_t *msg, const char *trim, int len, int *nlen) {
	//Define stuff
	uint8_t *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr(trim, *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr(trim, *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}



//Create the rest of the request (then send it on)
int msg_get_remaining (struct HTTPBody *r, uint8_t *msg, int len) {
	//Define
	//HTTP_Request *r = &h->request;
	LiteBlob *host = NULL;
	int runner = 0;
#if 1
	struct HS { 
		char *name, a, s; 
		Parser p; 
		/*LiteKv *kv;*/ 
		uint8_t *msg; 
		int ml, type, inc; 
	} hs[] = 
	{
		//URL
		{ "URL",'u', 0  ,{ .words = {{"/"}, {"?"}, {"\0"}, {NULL}} },
			/*r->URL,*/ (uint8_t *)r->path, strlen(r->path), 1, 0 },
		//Headers
		{ "headers",'h','\n',{ .words = {{"\r\n"}, {":"}, {"="}, {";"}, {NULL}} },
			/*r->headers, */  msg, r->hlen    , 1, 1 },
		/*GET*/
		{ "GET",'g','?' ,{ .words = {{"?"}, {"&"}, {"="}, {NULL}} },
			/*r->GET,*/ (uint8_t *)r->path, strlen(r->path), 1, 1 },
		/*MPP*/
		{ "POST",'m', 0  ,{ .words = {{r->boundary},{"\r\n\r\n"},{"\r\n"},{":"},{"="},{";"},{NULL}} },
			/*r->body,*/ &msg[r->hlen + 4], r->clen        , 1, 0 },
		/*APP*/
		{ "POST",'a', 0  ,{ .words = {{"\r\n"}, {":"},{"="},{"&"},{NULL}} },
			/*r->body,*/ &msg[r->hlen + 4], r->clen        , 1 }
	};

	//Initialize here
	lt_init( &r->table, NULL, 1027 );

	//...
	if ( memcmp( "HEAD", r->method, 4 ) == 0 )
		runner = 1;
	else if ( memcmp( "GET", r->method, 3 ) == 0 )
		runner = memchr( r->path, '?', strlen(r->path) ) ? 3 : 2;
	else if ( memcmp( "POST", r->method, 4 ) == 0 || memcmp( "PUT", r->method, 3 ) == 0 ) {
		runner = memchr( r->path, '?', strlen(r->path) ) ? 3 : 2;
		hs[ runner++ ] = hs [ (*r->boundary) ? 3 : 4 ];
	}


	//...
	for (int i=0; i < runner; i++) {
		unsigned char *blob = NULL;
		struct HS *h = &hs[ i ];	
		Parser *ps   = &h->p;
		int bw       = 0;
		int ii       = 0;
		int jj       = 0;
		int kk       = 0;
		int root     = 0;
		int colon    = 0;
		int cstart   = 0,
			  cend     = 0;

		//Add a new key
		lt_addblobkey( &r->table, (unsigned char *)h->name, strlen(h->name));
		lt_descend( &r->table );

		//Find the starting character.
		if ( h->s ) {
			if ((ii = memchrat(h->msg, h->s, h->ml)) == -1) continue;
			h->msg = memchr(h->msg, h->s, h->ml);
			h->ml -= ii;
		}

		h->msg += h->inc;
		pr_prepare( ps );

		while ( pr_next( ps, h->msg, h->ml ) ) {
			int adjust=0;
			if ( ps->word == NULL ) {
				//unsigned char *blob = NULL;//httpvtrim( &h->msg[ ps->prev - 1 ], ps->size, &adjust );
				if ( strcmp("URL", h->name) == 0 ) {
					lt_addintkey( &r->table, ++bw );
					if ( h->msg[ ps->prev - 1 ] == '?' ) break;
					blob = httpvtrim( &h->msg[ ps->prev - 1 ], ps->size + 1, &adjust );
				}
				else {
					blob = httpvtrim( &h->msg[ ps->prev ], ps->size, &adjust );
				}
				lt_addblobvalue( &r->table, blob, adjust );
				lt_finalize( &r->table );
				break;
			}
			else if ( *ps->word == '-' ) {
				bw = 1; //Find http boundary
				unsigned char *ret = NULL;
				if ((ii = memstrat( &h->msg[ ps->prev ], "; name=", h->ml - ps->prev )) == -1)
					continue;
				int begin = ii + strlen("; name=") + ps->prev;
				int jj = memstrat( &h->msg[ begin ], "\r", h->ml - begin );
				int kk = memstrat( &h->msg[ begin ], ";", h->ml - begin );
				static const char whitespace[]     = "\t\r\n\" ";
				
				if ( jj == -1 && kk == -1 )
					continue;
				if ( jj == -1 )
					ret = httptrim( &h->msg[ begin ], whitespace, kk, &adjust );
				else if (kk == -1)
					ret = httptrim( &h->msg[ begin ], whitespace, jj, &adjust );
				else {
					ret = httptrim( &h->msg[ begin ], whitespace, (jj < kk) ? jj : kk, &adjust );
				}
				lt_addblobkey( &r->table, ret, adjust );
				lt_descend( &r->table );
			}
			else if ( *ps->word == '/' ) { /*memcmp( "/", w, 1 ) == 0 )*/
				//root is ALWAYS first...
				if ( !root ) {
					lt_addintkey( &r->table, bw );
					lt_addblobvalue( &r->table, (uint8_t *)"/", 1 );
					lt_finalize( &r->table );
					root = 1;
					if (h->ml == 1)
						break;
					else {
						continue;
					}
				}

				lt_addintkey( &r->table, ++bw );
				lt_addblobvalue( &r->table, &h->msg[ ps->prev - 1 ], ps->size + 1 );
				lt_finalize( &r->table );
			}
			else if ( *ps->word == '?' ) { /*memcmp( "?", w, 1 ) == 0 ) {*/
				lt_addintkey( &r->table, ++bw );
				lt_addblobvalue( &r->table, &h->msg[ ps->prev - 1 ], ps->size + 1 );
				lt_finalize( &r->table ); 
			}	
			else if ( memchr( ":=", *ps->word, 2 ) ) {
				if ( h->a == 'h' ) {
					if ( colon ) continue;
					else { 
						colon = 1;
						cstart = ps->next;
					}
					blob = httpvtrim( &h->msg[ ps->prev ], ps->size, &adjust ); 
					lt_addblobkey( &r->table, blob, adjust );
					//write( 2, blob, adjust ); write( 2, " => ", 4 );
				}
				else {
					blob = httpvtrim( &h->msg[ ps->prev ], ps->size, &adjust ); 
					lt_addblobkey( &r->table, blob, adjust );
				}
			}
			else if ( memchr( ";\r&", *ps->word, 3 ) ) {
				//unsigned char *blob = NULL;
				if (bw) {
					bw=0;  
					continue; 
				}

				//Unset the colon
				if ( h->a == 'h' ) {
					if ( *ps->word == '\r' && colon ) {	
						colon = 0;
						//write( 2, &h->msg[cstart], ps->next - cstart  );
						blob = httpvtrim( &h->msg[ cstart ], ps->next - cstart, &adjust ); 
					}
					else if ( *ps->word == ';' && colon ) {
						continue;
					} 
				} 
				else {
					blob = httpvtrim( &h->msg[ ps->prev ], ps->size, &adjust ); 
				}

				//Value fudging... cuz Multipart post is the stupidest protocol ever written
				lt_addblobvalue( &r->table, blob, adjust ); 
				lt_finalize( &r->table );

				//Handle multipart values...
				if ( memcmp( "\r\n\r\n", ps->word, 4 ) == 0 ) {
					SHOWDATA( "%d, %s\n", ps->next, r->boundary );
					int aa = memstrat( &h->msg[ ps->next ], r->boundary, h->ml - ps->next ); 
					if ( aa == -1 )
						continue;	
					aa -= 2;
					lt_addblobkey (&r->table, (uint8_t *)content_string, strlen(content_string)); 
					lt_addblobvalue( &r->table, &h->msg[ ps->next ], aa - 2);
					lt_finalize(&r->table);
					lt_ascend(&r->table);
					ps->next += aa;
				}
			}
		}

		lt_ascend( &r->table );
	}

	lt_lock( &r->table );
#if 0
	//TODO: This should affect the hostname field
	//Now extract the hostname and save it to a buffer
	if ( !(host = &lt_blob( &r->table, "headers.Host" ))->size )
		h->host = NULL;
 	else {
		int p=0;
		int pos = (( p = memchrat( host->blob, ':', host->size )) == -1) ? host->size : p;
		h->host = malloc( pos + 1 );
		memset( h->host, 0, pos + 1 ); 
		memcpy( h->host, host->blob, pos );
		h->host[ pos ] = '\0';		
	}
#endif
#endif
	return 1;
}


//pre
int http_pre( ) {
	return 0;
}


//post (these are for ssl teardown and creation)
int http_post( ) {
	return 0;
}


//tell me the error returned.  I don't know what happened...
void whatsockerr( int e ) {
	if ( e == EBADF  ) fprintf( stderr, "Got sockerr: %s", "EBADF " );
	else if ( e == ECONNREFUSED  ) fprintf( stderr, "Got sockerr: %s", "ECONNREFUSED " );
	else if ( e == EFAULT  ) fprintf( stderr, "Got sockerr: %s", "EFAULT " );
	else if ( e == EINTR  ) fprintf( stderr, "Got sockerr: %s", "EINTR " );
	else if ( e == EINVAL  ) fprintf( stderr, "Got sockerr: %s", "EINVAL " );
	else if ( e == ENOMEM  ) fprintf( stderr, "Got sockerr: %s", "ENOMEM " );
	else if ( e == ENOTCONN  ) fprintf( stderr, "Got sockerr: %s", "ENOTCONN " );
	else if ( e == ENOTSOCK  ) fprintf( stderr, "Got sockerr: %s", "ENOTSOCK " );
	else if ( e == EAGAIN || e == EWOULDBLOCK  ) fprintf( stderr, "Got sockerr: %s", "EAGAIN || EWOULDBLOCK " );
}


void print_httpbody ( struct HTTPBody *r ) {
	fprintf( stderr, "r->mlen: %d\n", r->mlen );
	fprintf( stderr, "r->clen: %d\n", r->clen );
	fprintf( stderr, "r->hlen: %d\n", r->hlen );
	fprintf( stderr, "r->status: %d\n", r->status );
	fprintf( stderr, "r->stext: %d\n", r->stext );
	fprintf( stderr, "r->ctype: %d\n", r->ctype );
	fprintf( stderr, "r->method: %s\n", r->method );
	fprintf( stderr, "r->path: %s\n", r->path );
	fprintf( stderr, "r->protocol: %s\n", r->protocol );
	fprintf( stderr, "r->host: %s\n", r->host );
	fprintf( stderr, "r->boundary: %s\n", r->boundary );
}


//send (sends a message, may take many times)
int t_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//write (write all the data in one call if you fork like this) 
	const char http_200[] = ""
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 11\r\n"
		"Content-Type: text/html\r\n\r\n"
		"<h2>Ok</h2>";
	if ( write( fd, http_200, strlen(http_200)) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


//read (reads a message)
int t_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//read (read all the data in one call if you fork like this)
	const int size = 100000;
	unsigned char *rqb = malloc( size );
	memset( rqb, 0, size );	
	if (( rq->mlen = read( fd, rqb, size )) == -1 ) {
		fprintf(stderr, "Couldn't read all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


int h_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//if ( 1 ) write( 2, rq->msg, rq->mlen );
	int sent = 0;
	int total = rs->mlen;
	int pos = 0;
	int try = 0;

	while ( 1 ) { 
	if (( sent = send( fd, &rs->msg[ pos ], total, MSG_DONTWAIT )) == -1 ) {
		if ( errno == EBADF )
			0; //TODO: Can't close a most-likely closed socket.  What do you do?
		else if ( errno == ECONNREFUSED )
			close(fd);
		else if ( errno == EFAULT )
			close(fd);
		else if ( errno == EINTR )
			close(fd);
		else if ( errno == EINVAL )
			close(fd);
		else if ( errno == ENOMEM )
			close(fd);
		else if ( errno == ENOTCONN )
			close(fd);
		else if ( errno == ENOTSOCK )
			close(fd);
		else if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			if ( ++try == 2 ) {
			 #ifdef HTTP_VERBOSE
				fprintf(stderr, "Tried three times to read from socket. We're done.\n" );
			 #endif
				fprintf(stderr, "rs->mlen: %d\n", rs->mlen );
				//rq->msg = buf;
				break;
			}
		 #ifdef HTTP_VERBOSE
			fprintf(stderr, "Tried %d times to read from socket. Trying again?.\n", try );
		 #endif
		}
		else {
			//this would just be some uncaught condition...
		}
	}
	else if ( sent == 0 ) {

	}
	else {
		//continue resending...
		pos += sent;
		total -= sent;	
	}
	}
	return 0;
}

int h_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	
	//...
	unsigned char *buf = malloc( 1 );
	int mult = 0;
	int try=0;
	const int size = 32767;

#if 0
	//I may need to zero everything here...
	rq->mlen = 0;
	rq->clen = 0;
	rq->hlen = 0;
	rq->status = 0;
	rq->stext = NULL;
	rq->ctype = NULL;
	rq->method = NULL;
	rq->path = NULL; 
	rq->protocol = NULL;
	rq->host = NULL;
	rq->boundary = NULL;
#endif

	//Read first
	while ( 1 ) {
		int rd=0;
		int bfsize = size * (++mult); 
		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );

		//read into new buffer
		//if (( rd = read( fd, &buf[ rq->mlen ], size )) == -1 ) {
		//TODO: Yay!  This works great on Arch!  But let's see what about Win, OSX and BSD
		if (( rd = recv( fd, buf2, size, MSG_DONTWAIT )) == -1 ) {
			//A subsequent call will tell us a lot...
			fprintf(stderr, "Couldn't read all of message..." );
			//whatsockerr(errno);
			if ( errno == EBADF )
				0; //TODO: Can't close a most-likely closed socket.  What do you do?
			else if ( errno == ECONNREFUSED )
				close(fd);
			else if ( errno == EFAULT )
				close(fd);
			else if ( errno == EINTR )
				close(fd);
			else if ( errno == EINVAL )
				close(fd);
			else if ( errno == ENOMEM )
				close(fd);
			else if ( errno == ENOTCONN )
				close(fd);
			else if ( errno == ENOTSOCK )
				close(fd);
			else if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
				 #ifdef HTTP_VERBOSE
					fprintf(stderr, "Tried three times to read from socket. We're done.\n" );
				 #endif
					fprintf(stderr, "rq->mlen: %d\n", rq->mlen );
					fprintf(stderr, "%p\n", buf );
					//rq->msg = buf;
					break;
			}
			 #ifdef HTTP_VERBOSE
				fprintf(stderr, "Tried %d times to read from socket. Trying again?.\n", try );
			 #endif
			}
			else {
				//this would just be some uncaught condition...
			}
		}
		else if ( rd == 0 ) {
			//will a zero ALWAYS be returned?
			rq->msg = buf;
			break;
		}
		else {
			//realloc manually and read
			if ((buf = realloc( buf, bfsize )) == NULL ) {
				fprintf(stderr, "Couldn't allocate buffer..." );
				close(fd);
				return 0;
			}

			//Copy new data and increment bytes read
			memset( &buf[ bfsize - size ], 0, size ); 
			fprintf(stderr, "pos: %d\n", bfsize - size );
			memcpy( &buf[ bfsize - size ], buf2, rd ); 
			rq->mlen += rd;
			rq->msg = buf; //TODO: You keep resetting this, only needs to be done once...

			//show read progress and data received, etc.
			if ( 1 ) {
				fprintf( stderr, "Recvd %d bytes on fd %d\n", rd, fd ); 
			}
		}
	}

	//Show what I received so far...
	if ( 1 ) {
		write( 2, rq->msg, rq->mlen );
	}

#if 0
	//Let's just make this an echo server.  Because it's hard to get right.
	char echomsg[] = ""
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 11\r\n"
		"Content-Type: text/html\r\n\r\n"
		"Gotbobross!";
	unsigned char ebuf[ 100000 ];
	memset( ebuf, 0, sizeof(ebuf));
	snprintf( (char *)ebuf, rq->mlen, echomsg, strlen(echomsg) - 4, buf );
	//write(2,ebuf,strlen((char*)ebuf)); exit(0);
	rs->mlen = strlen( (char *)ebuf );	
	rs->msg = malloc( rs->mlen );
	memset( rs->msg, 0, rs->mlen );
	memcpy( rs->msg, ebuf, rs->mlen );	
#else
	char *header = (char *)rq->msg;
	int flLen = memchrat( rq->msg, '\n', rq->mlen ) - 1;
	int hdLen = memstrat( rq->msg, "\r\n\r\n", rq->mlen );

	rq->method = get_lstr( &header, ' ', &flLen );
	rq->path = get_lstr( &header, ' ', &flLen );
	rq->protocol = get_lstr( &header, ' ', &flLen ); 

	//The protocol parsing can happen here...
	//if ( memstrat( rq->msg, "GET", rq->mlen ) > -1 ) {
	if ( strcmp( rq->method, "GET" ) == 0 ) {
	 //#ifdef HTTP_VERBOSE
		fprintf(stderr, "Got GET\n" );
	 //#endif
	}
	else if ( strcmp( rq->method, "POST" ) == 0 ) {
	//else if ( memstrat( rq->msg, "POST", rq->mlen ) > -1 ) {
	 //#ifdef HTTP_VERBOSE
		//fprintf(stderr, "POST received %d bytes.\n", rq->recvd);
	 //#endif

	 #if 1
		//this can be -1
		rq->hlen = hdLen; 
		rq->clen = msg_get_content_length( rq->msg, hdLen );
		rq->boundary = msg_get_boundary( rq->msg, hdLen );
		//if rq->mlen and rq->hlen + 4 are the same, I need to keep going...
		//rq->mlen = hdLen; 
	 #else
		//Parse the line
		if ( !strlen( request->path ) && !http_parse_first_line(h, r->request, r->recvd) )
			return 0;

		//Get content-length (and reject if it's not there)
		if ( !request->clen && !http_get_content_length(h, r->request, r->recvd) )
			return 0;

		//Get the distance to the end of the headers
		if ( !request->hlen && !http_get_header_length(h, r->request, r->recvd) )
			return 0;

		//Get the message body length. 
		if ( !http_get_message_length(h, r->request, r->recvd) )
			return 0; /*We need to try reading again*/

		//Get the distance to the end of the headers
		if ( !http_get_boundary(h, r->request, r->recvd) )
			return 0;
	 #endif
	}

	if ( 1 )  {
		print_httpbody( rq );	
	}
#endif
close(fd);

	//msg_get_remaining( rq, rq->msg, rq->mlen );
	return 0;
}


int ssl_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//write (write all the data in one call if you fork like this) 
	const char http_200[] = ""
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 11\r\n"
		"Content-Type: text/html\r\n\r\n"
		"<h2>Ok</h2>";
	if ( write( fd, http_200, strlen(http_200)) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


//read (reads a message)
//int http_read( ) {
int ssl_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//read (read all the data in one call if you fork like this)
	const int size = 100000;
	unsigned char *rqb = malloc( size );
	memset( rqb, 0, size );	
	if (( rq->mlen = read( fd, rqb, size )) == -1 ) {
		fprintf(stderr, "Couldn't read all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


struct senderrecvr { 
	int (*read)( int, struct HTTPBody *, struct HTTPBody * );
	int (*write)( int, struct HTTPBody *, struct HTTPBody * ); 
	int (*pre)( int, struct HTTPBody *, struct HTTPBody * );
	int (*post)( int, struct HTTPBody *, struct HTTPBody * ); 
} sr[] = {
	{ h_read, h_write, http_pre, http_post }
, { t_read, t_write, http_pre, http_post }
,	{ NULL }
};



#if 1


int main (int argc, char *argv[]) {

	struct values {
		int port;
		int ssl;
		int start;
		int kill;
		int fork;
		char *user;
	} values = { 0 };

	//Process all your options...
	if ( argc < 2 ) {
		fprintf( stderr, "No options received.\n" );
		const char *fmt = "  --%-10s       %-30s\n";
		fprintf( stderr, fmt, "start", "start new servers" );
		fprintf( stderr, fmt, "kill", "test killing a server" );
		fprintf( stderr, fmt, "fork", "daemonize the server" );
		fprintf( stderr, fmt, "port <arg>", "set a differnt port" );
		fprintf( stderr, fmt, "ssl", "use ssl or not.." );
		fprintf( stderr, fmt, "user <arg>", "choose a user to start as" );
		return 0;	
	}	
	else {
		while ( *argv ) {
			if ( strcmp( *argv, "--start" ) == 0 ) 
				values.start = 1;
			else if ( strcmp( *argv, "--kill" ) == 0 ) 
				values.kill = 1;
			else if ( strcmp( *argv, "--port" ) == 0 ) 
				values.port = atoi( *argv );
			else if ( strcmp( *argv, "--ssl" ) == 0 ) 
				values.ssl = 1;
			else if ( strcmp( *argv, "--daemonize" ) == 0 ) 
				values.fork = 1;
			else if ( strcmp( *argv, "--user" ) == 0 ) {
				argv++;
				if ( !*argv ) {
					fprintf( stderr, "Expected argument for --user!" );
					return 0;
				} 
				values.user = strdup( *argv );
			}
			argv++;
		}
	}	

	if ( 1 ) {
		const char *fmt = "%-10s: %s\n";
		fprintf( stderr, "Invoked with options:\n" );
		fprintf( stderr, "%10s: %d\n", "start", values.start );	
		fprintf( stderr, "%10s: %d\n", "kill", values.kill );	
		fprintf( stderr, "%10s: %d\n", "port", values.port );	
		fprintf( stderr, "%10s: %d\n", "fork", values.fork );	
		fprintf( stderr, "%10s: %s\n", "user", values.user );	
		fprintf( stderr, "%10s: %s\n", "ssl", values.ssl ? "true" : "false" );	
	}


	//Set all of the socket stuff
	const int defport = 2000;
	struct sockAbstr su;
	su.addrsize = sizeof(struct sockaddr);
	su.buffersize = 1024;
	su.opened = 0;
	su.backlog = 500;
	su.waittime = 5000;
	su.protocol = IPPROTO_TCP;
	su.socktype = SOCK_STREAM;
	//su.protocol = IPPROTO_UDP;
	//su.sockettype = SOCK_DGRAM;
	su.iptype = PF_INET;
	su.reuse = SO_REUSEADDR;
	su.port = !values.port ? (int *)&defport : &values.port;
	su.ssl_ctx = NULL;

	if ( 1 ) {
		const char *fmt = "%-10s: %s\n";
		FILE *e = stderr;
		fprintf( e, "Socket opened with options:\n" );
		fprintf( e, "%10s: %d\n", "addrsize", su.addrsize );	
		fprintf( e, "%10s: %d\n", "buffersize", su.buffersize );	
		fprintf( e, "%10s: %d connections\n", "backlog", su.backlog );	
		fprintf( e, "%10s: %d microseconds\n", "waittime", su.waittime );	
		fprintf( e, "%10s: %s\n", "protocol", su.protocol == IPPROTO_TCP ? "tcp" : "udp" );	
		fprintf( e, "%10s: %s\n", "socktype", su.socktype == SOCK_STREAM ? "stream" : "datagram" );	 
		fprintf( e, "%10s: %s\n", "IPv6?", su.iptype == PF_INET ? "no" : "yes" );	
		fprintf( e, "%10s: %d\n", "port", *su.port );	
		//How to dump all the socket info?
	}

	//Create the socket body
	struct sockaddr_in t;
	memset( &t, 0, sizeof( struct sockaddr_in ) );
	struct sockaddr_in *sa = &t;
	sa->sin_family = su.iptype; 
	sa->sin_port = htons( *su.port );
	(&sa->sin_addr)->s_addr = htonl( INADDR_ANY );

	//Open the socket (non-blocking, preferably)
	int status;
	if (( su.fd = socket( su.iptype, su.socktype, su.protocol )) == -1 ) {
		fprintf( stderr, "Couldn't open socket! Error: %s\n", strerror( errno ) );
		return 0;
	}

	#if 0
	//Set timeout, reusable bit and any other options 
	struct timespec to = { .tv_sec = 2 };
	if (setsockopt(su.fd, SOL_SOCKET, SO_REUSEADDR, &to, sizeof(to)) == -1) {
		// su.free(sock);
		su.err = errno;
		return (0, "Could not reopen socket.");
	}
	#endif
	if ( fcntl( su.fd, F_SETFD, O_NONBLOCK ) == -1 ) {
		fprintf( stderr, "fcntl error: %s\n", strerror(errno) ); 
		return 0;
	}

	if (( status = bind( su.fd, (struct sockaddr *)&t, sizeof(struct sockaddr_in))) == -1 ) {
		fprintf( stderr, "Couldn't bind socket to address! Error: %s\n", strerror( errno ) );
		return 0;
	}

	if (( status = listen( su.fd, su.backlog) ) == -1 ) {
		fprintf( stderr, "Couldn't listen for connections! Error: %s\n", strerror( errno ) );
		return 0;
	}

	//Mark open flag.
	su.opened = 1;

	//If I could open a socket and listen successfully, then write the PID
	#if 0
	if ( values.fork ) {
		pid_t pid = fork();
		if ( pid == -1 ) {
			fprintf( stderr, "Failed to daemonize server process: %s", strerror(errno) );
			return 0;
		}
		else if ( !pid ) {
			//Close the parent?
			return 0;
		}
		else if ( pid ) {
			int len, fd = 0;
			char buf[64] = { 0 };

			if ( (fd = open( pidfile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR )) == -1 )
				return ERRL( "Failed to access PID file: %s.", strerror(errno));

			len = snprintf( buf, 63, "%d", pid );

			//Write the pid down
			if (write( fd, buf, len ) == -1)
				return ERRL( "Failed to log PID: %s.", strerror(errno));
		
			//The parent exited successfully.
			if ( close(fd) == -1 )
				return ERRL( "Could not close parent socket: %s", strerror(errno));
			return SUC_PARENT;
		}
	}
	#endif

	//Let's start the accept loop here...
	for ( ;; ) {
		//Client address and length?
		struct sockaddr addrinfo;	
		socklen_t addrlen = sizeof (struct sockaddr);	
		int fd;	

		//Accept a connection if possible...
		if (( fd = accept( su.fd, &addrinfo, &addrlen )) == -1 ) {
			//TODO: Need to check if the socket was non-blocking or not...
			if ( 0 )
				; 
			else {
				fprintf( stderr, "Accept ran into trouble: %s\n", strerror( errno ) );
				continue;
			}
		}
#if 0
		//Make the new socket non-blocking too...
		if ( fcntl( fd, F_SETFD, O_NONBLOCK ) == -1 ) {
			fprintf( stderr, "fcntl error at child socket: %s\n", strerror(errno) ); 
			return 0;
		}
#endif

		//Dump the client info and the child fd
		if ( 1 ) {
			struct sockaddr_in *cin = (struct sockaddr_in *)&addrinfo;
			char *ip = inet_ntoa( cin->sin_addr );
			fprintf( stderr, "Got request from: %s on new file: %d\n", ip, fd );	
		}

		//Fork and go crazy
		pid_t cpid = fork();
		if ( cpid == -1 ) {
			//TODO: There is most likely a reason this didn't work.
			fprintf( stderr, "Failed to setup new child connection. %s\n", strerror(errno) );
		}
		else if ( cpid == 0 ) {
			//TODO: The parent should probably log some important info here.	
			fprintf(stderr, "in parent...\n" );
		}
		else if ( cpid ) {
			//All the processing occurs here.
			struct HTTPBody rq, rs;	
			struct senderrecvr *f = &sr[ 0 ]; 
			memset( &rq, 0, sizeof( struct HTTPBody ) );
			memset( &rs, 0, sizeof( struct HTTPBody ) );

			//TODO: Somewhere in here, a signal needs to run that allows this thing to die.
			//...

			//Read the message	
			if (( status = f->read( fd, &rq, &rs )) == -1 ) {
				//TODO: Besides handling errors, a lot of what comes out here will define
				//what to do with the response...
			}

			//Write a new message	
			if (( status = f->write( fd, &rq, &rs )) == -1 ) {
				//...
			}
				
			if ( close( fd ) == -1 ) {
				fprintf( stderr, "Couldn't close child socket. %s\n", strerror(errno) );
				return 0;
			}	
		}

	}

	//Close the server process.
	if ( close( su.fd ) == -1 ) {
		fprintf( stderr, "Couldn't close socket! Error: %s\n", strerror( errno ) );
		return 0;
	}

	return 0;
}

#endif
