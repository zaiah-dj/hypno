#include "util.h"

//Print binary data (in hex) using name of variable as key
#define nbprintf(v, n) \
	fprintf (stderr,"%-30s: ", k); \
	for (int i=0; i < n; i++) fprintf( stderr, "%02x", v[i] ); \
	fprintf (stderr, "\n")


#if 0
//Use the time and a modulus to calculate some random numbers
int * mrand_init( void ) {
	//Generate 32 random numbers, back track and bitshift 3 times?
	const int count = 32;
	int *list = malloc( sizeof(int) * count );
	for ( int i = 0 ; i < count; i++ ) {
		fprintf( stderr, "ts: %d\n", urand_get( 128 ) );
	}	
	return list;
}
#endif

static int urand_get( int mod ) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	//fprintf( stderr, "ts: %ld\n", ts.tv_nsec % 9 );
	return ts.tv_nsec % mod;
}

uint8_t * srand_block( uint8_t *src, int srclen, uint8_t *buf, int buflen ) {
	memset( buf, 0, buflen );
	srclen --;
	uint8_t **b = &buf;
	while ( --buflen ) {
		char a = src[ urand_get( srclen ) ]; 
		fprintf( stderr, "%c\n", a );
		**b = a;
		b++;
	}
	return buf;
}


#if 0
char * srand_nums( char *buf, int buflen ) {
	char w[] = "0123456789";
	memset( buf, 0, buflen );
	while ( --buflen )
		*buf = urand_get( sizeof(w) ); 
		buf++;
	}
	return buf;
}

char *srand_chars( char *buf, int buflen, int range ) {
	return NULL;
}

unsigned char *srand_uint8t( uint8_t *buf, int buflen, int range ) {
	return NULL;
}
#endif

char * mrand_nums( int len ) {
	return NULL;
}

char *mrand_chars( int len ) {
	return NULL;
}

unsigned char *mrand_uint8t( int len ) {
	return NULL;
}

void mrand_free ( int **num ) {
	while ( *num ) {
		free( *num );
		num++;
	}
}



//TODO: None of these should take an error buffer.  They are just utilities...
uint8_t *read_file ( const char *filename, int *len, char *err, int errlen ) {
	//Check for and load whatever file
	int fd, fstat, bytesRead, fileSize;
	uint8_t *buf = NULL;
	struct stat sb;
	memset( &sb, 0, sizeof( struct stat ) );

	//Check for the file 
	if ( (fstat = stat( filename, &sb )) == -1 ) {
		//fprintf( stderr, "FILE STAT ERROR: %s\n", strerror( errno ) );
		snprintf( err, errlen, "FILE STAT ERROR: %s\n", strerror( errno ) );
		return NULL;	
	}

	//Check for the file 
	if ( (fd = open( filename, O_RDONLY )) == -1 ) {
		snprintf( err, errlen, "FILE OPEN ERROR: %s\n", strerror( errno ) );
		return NULL;	
	}

	//Allocate a buffer
	fileSize = sb.st_size + 1;
	if ( !(buf = malloc( fileSize )) || !memset(buf, 0, fileSize)) {
		snprintf( err, errlen, "COULD NOT OPEN VIEW FILE: %s\n", strerror( errno ) );
		close( fd );
		return NULL;	
	}

	//Read the entire file into memory, b/c we'll probably have space 
	if ( (bytesRead = read( fd, buf, sb.st_size )) == -1 ) {
		snprintf( err, errlen, "COULD NOT READ ALL OF VIEW FILE: %s\n", strerror( errno ) );
		free( buf );
		close( fd );
		return NULL;	
	}

	//This should have happened before...
	if ( close( fd ) == -1 ) {
		snprintf( err, errlen, "COULD NOT CLOSE FILE %s: %s\n", filename, strerror( errno ) );
		free( buf );
		return NULL;	
	}

	*len = sb.st_size;
	return buf;
}


//Safely convert numeric buffers...
int safeatoi( const char *value ) {
	//Copy to string
	char lc[ 128 ];
	memset( lc, 0, sizeof( lc ) );
	memcpy( lc, value, strlen( value ) );

	//Make sure that content-length numeric
	for ( int i=0; i < strlen(lc); i++ )  {
		if ( (int)lc[i] < 48 || (int)lc[i] > 57 ) {
			return 0;
		}
	}

	return atoi( lc );
}


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
		rr = malloc( r + 1 );
		memset( rr, 0, r );
		memcpy( rr, *str, r );	
		rr[ r ] = '\0';
		*str += r + 1;
		*lt -= r;
	}

	return rr;
}


//Extract value (a simpler code that can be used to grab values)
char *msg_get_value ( const char *value, const char *chrs, uint8_t *msg, int len ) {
	int start=0, end=0;
	char *bContent = NULL;

	if ((start = memstrat( msg, value, len )) > -1) {
		start += strlen( value );
		msg += start;

		//If chrs is more than one character, accept only the earliest match
		int pend = -1;
		while ( *chrs ) {
			end = memchrat( msg, *chrs, len - start );
			if ( end > -1 && pend > -1 && pend < end ) {
				end = pend;	
			}
			pend = end;
			chrs++;	
		}

		//Set 'end' if not already...	
		if ( end == -1 && pend == -1 ) {
			end = len - start; 
		}

		//Prepare for edge cases...
		if ((bContent = malloc( end + 1 )) == NULL ) {
			return ""; 
		}

		//Prepare the raw buffer..
		memset( bContent, 0, end + 1 );	
		memcpy( bContent, msg, end );
	}

	return bContent;
}


//Just copy the key
char *copystr ( uint8_t *src, int len ) {
	len++;
	char *dest = malloc( len );
	memset( dest, 0, len );
	memcpy( dest, src, len - 1 );
	return dest;
}


uint8_t *append_to_uint8t ( uint8_t **dest, int *len, uint8_t *src, int srclen ) {
	if ( !( *dest = realloc( *dest, (*len) + srclen ) ) ) {	
		return NULL;
	}

	if ( !memcpy( &(*dest)[ *len ], src, srclen ) ) {
		return NULL;
	}

	*len += srclen;
	return *dest;
}
 
