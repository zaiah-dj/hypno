//config.c
//Test parsing Lua config files.
//Each key should return something
//Compile me with: 
//gcc -ldl -llua -o config vendor/single.o config.c luabind.c && ./config
#include "config.h"

#define DUMPTYPE(NUM) \
	( NUM == BD_VIEW ) ? "BD_VIEW" : \
	( NUM == BD_MODEL ) ? "BD_MODEL" : \
	( NUM == BD_QUERY ) ? "BD_QUERY" : \
	( NUM == BD_CONTENT_TYPE ) ? "BD_CONTENT_TYPE" : \
	( NUM == BD_RETURNS ) ? "BD_RETURNS" : "UNKNOWN" 


//struct route { char *routename, **elements; };
//struct config { char *item; int (*fp)( char * ); };
struct config { char *item; int (*fp)( LiteKv *, int, void * ); };
struct routehandler { char *filename; int type; };
struct route { char *routename; char *parent; int elen; struct routehandler **elements; };
struct routeset { int len; struct route **routes; };

struct config conf[] = {
	{ "engine", NULL }
,	{ "db"    , NULL }
,	{ "routes", NULL }
,	{ NULL, NULL }
};

const char *keys[] = {
	"returns"
,	"content-type"
,	"query"
,	"model"
,	"view"
,	"routes"
,	"hint"
,	"auth"
,	NULL
};

const char *keysstr = 
	"returns" \
	"content-type" \
	"query" \
	"model" \
	"view" \
	"routes" \
 	"hint" \
	"auth"
;

int c = 0;
int b = 0;
char *parent[100] = { NULL };
char *handler[100] = { NULL };
const int BD_VIEW = 41;
const int BD_MODEL = 42;
const int BD_QUERY = 43;
const int BD_CONTENT_TYPE = 44;
const int BD_RETURNS = 45;


void *get_key ( Table *t, const char *key ) {
	int index = lt_geti( t, key );
	int count = lt_counti( t, index );
	FPRINTF( "Key '%s' = %d.  Contains %d element(s).\n", key, index, count );
	//Check what is on the other side if it's not -1
	//If it's string, extract and return it
	//If it's number, ?
	//If it's table, ?
	//Extraction will probably be custom...
	return NULL;
}


void dump_routes ( struct routeset r ) {
	
}

#if 0
int get_routes ( Table *t ) {
	//Find the routes index, use that as start and move?
	//struct route **routelist = NULL; //This could be static too... less to clean
	struct routeset r = { 0, NULL };
	int routesIndex = lt_geti( t, "routes" );
	int count = lt_counti( t, routesIndex );
	FPRINTF( "routes key at: %d\n", routesIndex );

	//Combine the routes and save each combination (strcmbd?)
	if ( !lt_exec_complex( t, routesIndex, t->count, (void *)&r, buildRoutes ) ) {
		FPRINTF( "At end of route data.\n" );
	}

	for ( int i=0; i < r.len; i++ ) {
		struct route *rr = r.routes[i];
		fprintf( stderr, "[%d] %s => ", i, rr->routename );
		fprintf( stderr, "route composed of %d files.\n", rr->elen );
		for ( int ii=0; ii < rr->elen; ii++ ) {
			struct routehandler *t = rr->elements[ ii ];
			fprintf( stderr, "\t{ %s=%s }\n", DUMPTYPE(t->type), t->filename );
		}
	}
return 0;
}
#endif

int route_table_iterator ( LiteKv *kv, int i, void *p ) {
	struct routeset *r = (struct routeset *)p;

	//Right side should be a table.
	//FPRINTF( "Index: %d\n", i );
	char *name = NULL;
        char nbuf[ 64 ] = { 0 };	
	if ( kv->key.type == LITE_TXT ) {
		//FPRINTF( "Left: %s => ", kv->key.v.vchar );
		name = kv->key.v.vchar;
	}
	//If it's a number, it could be something else...
	else if ( kv->key.type == LITE_TRM ) {
		//Safest to add a null member to the end of elements
		//b--;
		if ( (--b) == 0 )
			return 0;	
		else { 
			if ( b == 1 ) {
				for ( int i=0; i < sizeof( parent ) / sizeof( char * ); i++ ) {
					parent[ i ] = NULL;
					handler[ i ] = NULL;
				}	
			}
		}
	}
	else {
		FPRINTF( "got some other type of key: %s\n", lt_typename( kv->key.type ) ); 
		//snprintf( nbuf, sizeof(nbuf) - 1, "%d", 
	}

	if ( kv->value.type == LITE_TXT ) {
		//The other side should be either string or function
		char *type = handler[ b - 1];
		FPRINTF( "filename: %s, type: %s\n", kv->value.v.vchar, type ); 
		if ( type ) {
			struct route *rr = r->routes[ r->len - 1 ];
			struct routehandler *t = malloc( sizeof( struct routehandler ) );
			t->filename = strdup( kv->value.v.vchar ); 
			if ( memcmp( "model", type, 3 ) == 0 )
				t->type = BD_MODEL;
			else if ( memcmp( "view", type, 3 ) == 0 )
				t->type = BD_VIEW;
			else if ( memcmp( "query", type, 3 ) == 0 )
				t->type = BD_QUERY;
			else if ( memcmp( "content", type, 3 ) == 0 )
				t->type = BD_CONTENT_TYPE;
			else if ( memcmp( "returns", type, 3 ) == 0 )
				t->type = BD_RETURNS;
			else {
				//This isn't valid, so drop it...
			}
			ADDITEM( t, struct routetype *, rr->elements, rr->elen, 0 ); 
		}
	}

	else if ( kv->value.type == LITE_TBL ) {
		//FPRINTF( "Right: %s\n", lt_typename( kv->value.type ) );
		//Only add certain keys, other wise, they're routes...
		if ( name ) {
			if( !memstr( keysstr, name, strlen(keysstr) ) ) {
				struct route *rr = malloc( sizeof(struct route) );
				char buf[ 1024 ] = {0};
				char *par = parent[ b - 1 ];
				rr->elen = 0;
				rr->elements = NULL;
				if ( !par ) {
					buf[ 0 ] = '/';
					memcpy( &buf[ 1 ], name, strlen( name ) );
					rr->routename = strdup( buf );
				}
				else {
					int slen = 0;
					memcpy( buf, par, strlen( par ) );
					slen += strlen( par );
					memcpy( &buf[ slen ], "/", 1 );
					slen += 1;
					memcpy( &buf[ slen ], name, strlen( name ) );
					slen += strlen( name );
					rr->routename = strdup( buf );
				}
				parent[ b ] = rr->routename;
				//FPRINTF( "Got route name (%s), c. parent (%s), n.parent (%s)\n", name, par, parent[ b ] );
				ADDITEM( rr, struct route *, r->routes, r->len, 0 );  
				c = 0;
			}
			else {
				FPRINTF( "Got prepared key: %s\n", name );
				handler[ b ] = strdup( name );
				//Get the last set one
				//Mark which one you got somehow...
				c = 1;
			}
			b++;
		}
	}
	//Keep track of all the sentinels....
	//FPRINTF( "Depth: %d\n", b );
	//getchar();
	return 1;
}

