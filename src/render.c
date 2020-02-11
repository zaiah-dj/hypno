/* ---------------------------------------------------
render.c 

Test out rendering...

TODO / TASKS
------------
- Get proper renders to work...
- Make it work with nested anything


Stuck on COMPLEX_EXTRACT.
- !!! Keeping the hashes still helps, so don't get rid of that...

1.
- Extend single.c to return a short key or full key from a location
(this way all of the hashes can be used)
2.
- Or just allocate individual strings with what you need at the parent
	2a.
	- Generate full strings at the COMPLEX_EXTRACT part
	- Full strings also have to be generated at LOOP_START

//{{ # xxx }} - LOOP START
//{{ / xxx }} - LOOP END 
//{{ x     }} - SIMPLE EXTRACT
//{{ .     }} - COMPLEX EXTRACT
//{{ $     }} - EACH KEY OR VALUE IN A TABLE 
//{{ `xxx` }} - EXECUTE 
//{{ !xxx  }} - BOOLEAN? 
//{{ xxx ? y : z }} - TERNARY

 * --------------------------------------------------- */
#include "render.h"

#define DUMPACTION( NUM ) \
	( NUM == LOOP_START ) ? "LOOP_START" : \
	( NUM == LOOP_END ) ? "LOOP_END" : \
	( NUM == COMPLEX_EXTRACT ) ? "COMPLEX_EXTRACT" : \
	( NUM == SIMPLE_EXTRACT ) ? "SIMPLE_EXTRACT" : \
	( NUM == EACH_KEY ) ? "EACH_KEY" : \
	( NUM == EXECUTE ) ? "EXECUTE" : \
	( NUM == BOOLEAN ) ? "BOOLEAN" : \
	( NUM == RAW ) ? "RAW" : "UNKNOWN" 

static const int LOOP_START = 30;
static const int LOOP_END = 31;
static const int SIMPLE_EXTRACT = 32;
static const int COMPLEX_EXTRACT = 33;
static const int EACH_KEY = 34;
static const int EXECUTE = 35;
static const int BOOLEAN = 36;
static const int RAW = 37;
static const int BLOCK_START = 0;
static const int BLOCK_END = 0;

//Bitmasking will tell me a lot...
uint8_t *table_to_template ( Table *t, const uint8_t *src, int srclen, int * newlen ) {
	//Constants for now, b/c I forgot how to properly bitmask
	uint8_t *dest = NULL;
	int destlen = 0;
	int ACTION = 0;
	int BLOCK = 0;
	int SKIP = 0;
	int INSIDE = 0;
	Mem r;
	memset( &r, 0, sizeof( Mem ) );

	//Hmm... this structure could use a little help
	//1) A union is probably best considering memory and type-safety.
	//2) void pointers work, but they are very ugly and error prone
	//3) A specific member for each type could work too

	//Hmm. len is not super necessary, only because the hash can be used to pull things
	//so, we're looking at:
	//{ int hash, action;  uint8_t *text;  int **hashlist; }
	//hash is the source hash if it's a simple replace, or the parent if hashlist is used
	//action is what to do, this cna just be an integer constant
	//text is original text if its there, a replacement if it's just one
	//hashlist is a list of hashes when we reach bigger tables
	//
	//NOTE:
	//this could probably be a two column table if I really think about it...
	//{ int **hashlist; uint8_t *block; }
	//if hashlist is null, assume that block is raw content
	//if hashlist is not null, pull each hash as you go through

	//NOTE:
	//Yet another way to do it, is to make the data structure much bigger.
	//Then you don't have to move backwards in a list of pointers...
	//If this is really a two-column list, then it won't take much space... 
//#define RBDEF
	struct rb { int action, len, **hashList; void *ptr; } **rr = NULL ; 
	struct parent { 
	#if 0
		struct parent *parent; 
		struct rb *parent;
	#else
	#endif
		uint8_t *text; 
		int len, pos, childCount; 
	} **pp = NULL;
	int rrlen = 0;
	int pplen = 0;

	//TODO: Maps really should come from outside of the function.
	//char *mapchars = "#/.$`!";
	//struct map { int action; char a; } **maps = NULL;
	int maps[] = {
		//['#'] = SIMPLE_EXTRACT,
		['#'] = LOOP_START,
		['/'] = LOOP_END,
		['.'] = COMPLEX_EXTRACT,
		['$'] = EACH_KEY, 
		['`'] = EXECUTE, //PAIR_EXTRACT
		['!'] = BOOLEAN,
		[254] = RAW,
		[255] = 0
	};

	//Allocate a new block to copy everything to
	if (( dest = malloc( 8 ) ) == NULL ) {
		return NULL;
	}

	//Allocating a list of characters to elements is easiest.
	while ( memwalk( &r, (uint8_t *)src, (uint8_t *)"{}", srclen, 2 ) ) {
		//More than likely, I'll always use a multi-byte delimiter
		if ( r.size == 0 ) { /*&& r.pos > 0 ) {*/
			//Check if there is a start or end block
			if ( r.chr == '{' ) {
				BLOCK = BLOCK_START;
			}
		}
		else if ( r.chr == '}' ) {
			if ( src[ r.pos + r.size + 1 ] == '}' ) {
				//Start extraction...
				BLOCK = BLOCK_END;
				int nlen = 0;	
				int **hashList = NULL;
				int hashListLen = 0;
				uint8_t *p = trim( (uint8_t *)&src[r.pos], " \t", r.size, &nlen );
			#if 0
				struct rb rbb = { 0 };
			#else
				struct rb *rp = malloc( sizeof( struct rb ) );
				if ( !rp ) {
					//Free and destroy things
					return NULL;
				}
			#endif

				//Extract the first character
				if ( !maps[ *p ] ) {
					rp->action = SIMPLE_EXTRACT; 
					int hash = -1;
					if ( (hash = lt_get_long_i(t, p, nlen) ) > -1 ) {
						int *h = malloc( sizeof(int) );
						memcpy( h, &hash, sizeof( int ) );
						ADDITEM( h, int *, hashList, hashListLen, NULL ); 
					}
				}
				else {
					//Advance and reset p b/c we need just the text...
					int alen = 0;
					rp->action = maps[ *p ];
					p = trim( p, ". #/$`!\t", nlen, &alen );
					FPRINTF("GOT ACTION %s, and TEXT = ", DUMPACTION(rp->action));
					ENCLOSE( p, 0, alen );

					//Figure some things out...
					if ( rp->action == LOOP_START ) {

						//We could be just about anywhere, so anticipate that the parent could be here
						FPRINTF( "@LOOP_START - " );	
						int hash = -1;
						int blen = 0;
						int eCount = 0;
						uint8_t bbuf[ 2048 ] = { 0 };
						struct parent *cp = NULL;

						//If a parent should exist, copy the parent's text 
						//TODO: Eventually, numbers shouldn't be necessary on this check
						if ( !INSIDE ) {
							//Copy the data
							memcpy( &bbuf[ blen ], p, alen );
							blen += alen;
						
							FPRINTF( "Checking level[0] table " );	
							ENCLOSE( bbuf, 0, blen );
							//This is the only thing, get the hash and end it
							if ( (hash = lt_get_long_i( t, bbuf, blen ) ) > -1 ) {
								int *h = malloc( sizeof( int ) );
								memcpy( h, &hash, sizeof( int ) );
								ADDITEM( h, int *, hashList, hashListLen, NULL ); 
								eCount = lt_counti( t, hash );
							}
						}
						else {
							//If there are 3 parents, you need to start at the beginning and come out
							//Find the MAX count of all the rows that are there... This way you'll have the right count everytime...
							FPRINTF( "Checking level[n+1] table " );	

							//Get a count of the number of elements in the parent.
							int maxCount = 0;
							cp = pp[ pplen - 1 ];
							FPRINTF( "containing %d members.\n", cp->childCount );
							FPRINTF( "\tParent strings are:\n" ); 

							for ( int i=0, cCount=0; i<cp->childCount; i++ ) {
								char num[ 64 ] = { 0 };
								uint8_t nbuf[ 2048 ] = { 0 };
								int numlen = snprintf( num, sizeof( num ) - 1, ".%d.", i );	

								//Copy to static buffer
								memcpy( bbuf, cp->text, cp->len );
								blen = cp->len;
								memcpy( &bbuf[ blen ], num, numlen );
								blen += numlen;
								memcpy( &bbuf[ blen ], p, alen );
								blen += alen;
						
								//DEBUG: See the string to check for...	
								write( 2, "\t\t'", 3 ); write( 2, bbuf, blen ); write( 2, "'", 1 );

								//Check for the hash
								hash = lt_get_long_i(t, bbuf, blen ); 
								int *h = malloc( sizeof(int) );
								memcpy( h, &hash, sizeof( int ) );
								ADDITEM( h, int *, hashList, hashListLen, NULL ); 
								FPRINTF( "; hash is: %3d, ", hash );	
							
								if ( hash > -1 && (cCount = lt_counti( t, hash )) > maxCount ) {
									maxCount = cCount;	
									FPRINTF( " child count is: %3d\n", maxCount );	
								}

							}
							eCount = maxCount;
						}

						//Find the hash
						if ( hashListLen ) {
							//Set up the parent structure
							struct parent *np = NULL; 
							if (( np = malloc( sizeof(struct parent) )) == NULL ) {
								//TODO: Cut out and free things
							}

							//NOTE: len will contain the number of elements to loop
							np->childCount = eCount;
							np->pos = 0;
							np->len = alen;
							np->text = p; 
							ADDITEM( np, struct parent, pp, pplen, NULL );
							INSIDE++;
						}
					}
					else if ( rp->action == LOOP_END ) {
						//If inside is > 1, check for a period, strip it backwards...
						FPRINTF( "@LOOP_END - " );
						//TODO: Check that the hashes match instead of just pplen
						//rp->hash = lt_get_long_i( t, p, alen );
						if ( !INSIDE )
							;
						else if ( pplen == INSIDE ) {
							free( pp[ pplen - 1 ] );
							pplen--;
							INSIDE--;
						}
					}
					else if ( rp->action == COMPLEX_EXTRACT ) {
						FPRINTF( "@COMPLEX_EXTRACT - " );
						if ( pplen ) {
							struct parent **w = pp;
							int c = 0;
							while ( (*w)->pos < (*w)->childCount ) {
						FPRINTF( "@IS THIS RUNNING AT ALL?\n" ); getchar();
								//Move to the next block or build a sequence
								if ( c < (pplen - 1) ) {
									w++, c++;
									continue;
								}
								
								//Generate the hash strings
								if ( 1 ) {
									struct parent **xx = pp;
									uint8_t tr[ 2048 ] = { 0 };
									int trlen = 0;
									
									for ( int ii=0; ii < pplen; ii++ ) {
										memcpy( &tr[ trlen ], (*xx)->text, (*xx)->len );
										trlen += (*xx)->len;
										trlen += sprintf( (char *)&tr[ trlen ], ".%d.", (*xx)->pos );
										xx++;
									}
									memcpy( &tr[ trlen ], p, alen );
									trlen += alen;

									//Check for this hash, save each and dump the list...
									int hh = lt_get_long_i(t, tr, trlen ); 
									int *h = malloc( sizeof(int) );
									memcpy( h, &hh, sizeof(int) );	
									ADDITEM( h, int *, hashList, hashListLen, NULL ); 
									FPRINTF( "string = %s, hash = %3d, ", tr, hh );	
								}

								//Increment the number 
								while ( 1 ) {
									(*w)->pos++;
									//printf( "L%d %d == %d, STOP", c, (*w)->a, (*w)->b );
									if ( c == 0 )
										break;
									else { // ( c > 0 )
										if ( (*w)->pos < (*w)->childCount ) 
											break;
										else {
											(*w)->pos = 0;
											w--, c--;
										}
									}
								}
							}
							(*w)->pos = 0;
						}
					}
					else if ( rp->action == EACH_KEY ) {
						FPRINTF( "@EACH_KEY :: Nothing yet...\n" );
					}
					else if ( rp->action == EXECUTE ) {
						FPRINTF( "@EXECUTE :: Nothing yet...\n" );
					}
					else if ( rp->action == BOOLEAN ) {
						FPRINTF( "@BOOLEAN :: Nothing yet...\n" );
					}
				}

				//Create a new row with what we found.
				FPRINTF( "\n@END: Adding new row to template set.  rrlen: %d, pplen: %d.  Got ", rrlen, pplen );
				//ENCLOSE( rp->ptr, 0, rp->len );

				//struct rb *rp = malloc( sizeof( struct rb ) );
				//memcpy( rp, &rbb, sizeof( struct rb ) ); 
				if ( !hashListLen )
					rp->hashList = NULL;
				else {
					rp->hashList = hashList; 
					ADDITEM( NULL, int *, rp->hashList, hashListLen, NULL );
				}
				
				ADDITEM(rp, struct rb, rr, rrlen, NULL);
			}
		}
#if 1
		else {
			FPRINTF( "@RAW BLOCK COPY" ); 
			//We can simply copy if ACTION & BLOCK are 0 
			if ( !ACTION && !BLOCK ) {
				//struct rb rbb = { 0 };
				struct rb *rp = malloc( sizeof( struct rb ) );
				if ( !rp ) {
					//Teardown and destroy
					return NULL;
				}
				fprintf( stderr, "DO RAW COPY OF: " );
				write( 2, &src[ r.pos ], r.size );
				write( 2, "\n", 1 );
		
				//Set defaults
				rp->len = r.size;	
				//rp->hash = -2;
				rp->action = RAW;
				rp->hashList = NULL;
				rp->ptr = (uint8_t *)&src[ r.pos ];	
				
				//Save a new record
#if 0
				struct rb *rp = malloc( sizeof( struct rb ) );
				memcpy( rp, &rbb, sizeof( struct rb ) ); 
#endif
				ADDITEM(rp, struct rb, rr, rrlen, NULL);
			}	
		}
#endif
	}

#if 1
	//DEBUG: Show me what's on the list...
	for ( int i=0; i<rrlen; i++ ) {
		struct rb *item = rr[ i ];

		//Dump the unchanging elements out...
		fprintf( stderr, "[%3d] => action: %-16s", i, DUMPACTION( item->action ) );

		if ( item->action == RAW || item->action == EXECUTE ) {
			fprintf( stderr, " len: %3d, ", item->len ); 
			uint8_t *pp = (uint8_t *)item->ptr;
			ENCLOSE( pp, 0, item->len );
		}
		else {
			fprintf( stderr, " list: %p => ", item->hashList );
			int **ii = item->hashList;
			if ( !ii ) 
				fprintf( stderr , "NULL" );
			else {
				int d=0;
				while ( *ii ) {
					fprintf( stderr, "%c%d", d++ ? ',' : ' ',  **ii );
					ii++;
				}
			}
			fprintf( stderr, "\n" );
		}
	}
#endif

#if 1
	//This other way will involve generating a longer data structure
	//The code can just move through the list and not worry about
	//backreferencing (moving things around is slightly easier)
#else
	//Start the writes, by using the structure as is
	uint8_t *block = NULL;
	int blockLen = 0;
	for ( int **ii=NULL, i=0; i<rrlen; i++ ) {
		struct rb *item = rr[ i ];
		if ( item->action == RAW || item->action == EXECUTE ) {
			//fprintf( stderr, " len: %3d, ", item->len ); 
			//ENCLOSE( item->ptr, 0, item->len );
			blockLen += item->len;
			if ( (block = realloc( block, blockLen )) == NULL ) {
				//NOTE: Teardown properly or you will cry...
				return NULL;
			}
			//If I do a memset, where at?
			memcpy( &block[ blockLen - item->len ], item->ptr, item->len ); 
		}
		else {
			//LOOP_START has a childCount, so go through that.  
			//Is this what controls the looping though? 
			//If it's -1, skip this iteration and its children (who would have that?)
			//This is easier if *ptr is either void or a union
			//union is probably safer... but more troublesome
			//on LOOP_START, *ptr = parent (refernce to what's there)
			//on LOOP_END    *ptr = rb of LOOP_START (you can check equality?)
			//All pos should be at zero
			//on COMPLEX_EX  *ptr = NULL
			if ( item->action == LOOP_START ) {
				fprintf( stderr, "len: %d", item->len );
				fprintf( stderr, "ptr: %p", item->ptr );
			} 
			else if ( item->action == LOOP_END ) {
				//Access the parent from ptr and "rewind"
			}
			else if ( item->action == COMPLEX_EXTRACT ) {
				//If there is a pointer, it does not move until I get through all three
			}
		#if 0
			if ( (ii = item->hashList) != NULL ) {
				int d=0;
				while ( *ii ) {
					fprintf( stderr, "%c%d", d++ ? ',' : ' ',  **ii );
					ii++;
				}
			}
		#endif
			fprintf( stderr, "\n" );
		}
	}
#endif

#if 1
	//Start the writes
#endif

	//The final step is to assemble everything...
	return NULL;
}
