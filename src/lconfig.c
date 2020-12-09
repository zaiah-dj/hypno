//lconfig.c - Put all the config stuff into the same place
#include "lconfig.h"

//Free config tables
static void free_t( zTable *t ) {
	if ( t ) {
		lt_free( t );
		free( t );
	}
}


//A hosts handler
static int hosts_iterator ( zKeyval * kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct lconfig ***hosts = f->userdata;
	zTable *st = NULL, *nt = NULL;

	//If current index is a table
	if ( kv->key.type == LITE_TXT && kv->value.type == LITE_TBL && f->depth == 2 ) {
		struct lconfig *w = malloc( sizeof( struct lconfig ) );
		int count = lt_counti( ( st = ((struct fp_iterator *)p)->source ), i );
		//FPRINTF( "NAME: %s, COUNT OF ELEMENTS: %d\n", kv->key.v.vchar, count ); 
		nt = loader_shallow_copy( st, i+1, i+count );
		memset( w, 0, sizeof( struct lconfig ) );
		const struct rule rules[] = {
			{ "alias", "s", .v.s = &w->alias },
			{ "dir", "s", .v.s = &w->dir },
			{ "filter", "s", .v.s = &w->filter },
			{ "root_default", "s", .v.s = &w->root_default },
			{ "ca_bundle", "s", .v.s = &w->ca_bundle },
			{ "certfile", "s", .v.s = &w->certfile },
			{ "keyfile", "s", .v.s = &w->keyfile },
			{ NULL }
		};

		w->name = kv->key.v.vchar;
		if ( !loader_run( nt, rules ) ) {
		}
		lt_free( nt );
		free( nt );
		add_item( hosts, w, struct lconfig *, &f->len );
	}
	return 1;
}


//Find a host
struct lconfig * find_host ( struct lconfig **hosts, char *hostname ) {
	char host[ 2048 ] = { 0 };
	int pos = memchrat( hostname, ':', strlen( hostname ) );
	memcpy( host, hostname, ( pos > -1 ) ? pos : strlen(hostname) );
	while ( hosts && *hosts ) {
		struct lconfig *req = *hosts;
		if ( req->name && strcmp( req->name, host ) == 0 )  {
			return req;	
		}
		if ( req->alias && strcmp( req->alias, host ) == 0 ) {
			return req;	
		}
		hosts++;
	}
	return NULL;
}


//Build a list of valid hosts
static struct lconfig ** build_hosts ( zTable *t ) {
	struct lconfig **hosts = NULL;
	const struct rule rules[] = {
		{ "hosts", "t", .v.t = (void ***)&hosts, hosts_iterator }, 
		{ NULL }
	};

	loader_run( t, rules );
	return hosts;
}


//Free hosts list
void free_hosts ( struct lconfig ** hlist ) {
	struct lconfig **hosts = hlist;
	while ( hosts && (*hosts) ) {
		static int i;
		free( (*hosts)->alias );
		free( (*hosts)->dir );
		free( (*hosts)->filter );
		free( (*hosts)->root_default );
		free( (*hosts)->ca_bundle );
		free( (*hosts)->certfile );
		free( (*hosts)->keyfile );
		free( (*hosts) );
		hosts++;
	}
	free( hlist );
}


//Dump the host list
void dump_hosts ( struct lconfig **hosts ) {
	struct lconfig **r = hosts;
	fprintf( stderr, "Hosts:\n" );
	while ( r && *r ) {
		fprintf( stderr, "\t%p => ", *r );
		fprintf( stderr, "%s =>\n", (*r)->name );
		fprintf( stderr, "\t\tdir = %s\n", (*r)->dir );
		fprintf( stderr, "\t\talias = %s\n", (*r)->alias );
		fprintf( stderr, "\t\tfilter = %s\n", (*r)->filter );
		r++;
	}
}

//build_server_config or get_server_config
struct sconfig * build_server_config ( const char *file, char *err, int errlen ) {
	FPRINTF( "Configuration parsing started...\n" );

	struct sconfig *config = NULL; 
	zTable *t = NULL;
	lua_State *L = NULL;

	//Allocate Lua
	if ( ( L = luaL_newstate() ) == NULL ) {
		snprintf( err, errlen, "Could not initialize Lua environment.\n" );
		return NULL;
	}

	//Allocate config
	if ( ( config = malloc( sizeof( struct sconfig ) ) ) == NULL ) {
		snprintf( err, errlen, "Could not initialize memory when parsing config at: %s\n", file );
		return NULL;
	}

	//After this conversion takes place, destroy the environment
	if ( !lua_exec_file( L, file, err, errlen ) ) {
		free( config );
		lua_close( L );
		return NULL;
	}

	//Allocate a table for the configuration
	if ( !(t = malloc(sizeof(zTable))) || !lt_init( t, NULL, 2048 ) ) {
		snprintf( err, errlen, "Could not initialize table when parsing config at: %s\n", file );
		free_t( t );
		free( config );
		lua_close( L );
		return NULL;
	}

	//Check the stack and make sure that it's a table.
	if ( !lua_istable( L, 1 ) ) {
		snprintf( err, errlen, "Configuration is not a table.\n" );
		free_t( t );
		free( config );
		lua_close( L );
		return NULL;
	}

	//Convert configuration into a table
	if ( !lua_to_table( L, 1, t ) ) {
		snprintf( err, errlen, "Failed to convert Lua config data to table.\n" );
		free_t( t );
		free( config );
		lua_close( L );
		return NULL;
	}

	//Build hosts
	if ( ( config->hosts = build_hosts( t ) ) == NULL ) {
		//Build hosts fails with null, I think...
		snprintf( err, errlen, "Failed to bulid hosts table from: %s\n", file );
		free_t( t );
		free( config );
		lua_close( L );
		return NULL;
	}

#if 0
	//This is the global root default
	if ( ( config->root_default = get_char_value( t, "root_default" ) ) ) {
		config->root_default = strdup( config->root_default );
	} 
#endif

	//Destroy lua_State and the tables...
	//free_t( t );
	config->src = t;	
	lua_close( L );
	FPRINTF( "Configuration parsing complete.\n" );
	return config;
}

//build_site_config or get_site_config

//Destroy the server oonfig file.
void free_server_config( struct sconfig *config ) {
#if 0
	if ( config->hosts ) {
		free_hosts( config->hosts );	
	}
	if ( config->routes ) {
		free_routes( config->routes );
	}
#endif
	//free( config->path );
	//free( config->root_default );
	lt_free( config->src );
	free( config->src );
	free( config );
}
