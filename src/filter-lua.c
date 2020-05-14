#include "filter-lua.h"
#include "filter-static.h"

#define FILTER_LUA_DEBUG

#ifndef FILTER_LUA_DEBUG
 #define FILTER_LUA_PRINT(...)
#else
 #define FILTER_LUA_PRINT(...) \
	fprintf( stderr, "[%s:%d]", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ )
#endif

#define lua_pushustrings(STATE,KEY,VAL,VLEN) \
	lua_pushstring( STATE, KEY ) && lua_pushlstring( STATE, (char *)VAL, VLEN )

#define lua_pushstrings(STATE,KEY,VAL) \
	lua_pushstring( STATE, KEY ) && lua_pushlstring( STATE, (char *)VAL, strlen( VAL ))


static const char lconfname[] = "/config.lua";
static const char modelfmt[] = "%s/app/%s.lua";
static const char viewfmt[] = "%s/view/%s.%s"; //tpl


//Destroy the Lua configuration
void destroy_luaconf ( struct luaconf *luaconf ) {
	free_routeh( luaconf->routes );
	free( luaconf->db );
	free( luaconf->fqdn );
	free( luaconf->title );
	free( luaconf->root_default );
	free( luaconf->spath );
	free( luaconf );
}


//Load Lua libraries
static lua_State * lua_load_libs( lua_State **L ) {
	if ( !( *L = luaL_newstate() ) ) {
		return NULL;
	}	
	luaL_openlibs( *L );
	return *L;
}


//All of the keys I need to parse will go here for now...
struct luaconf * build_luaconf ( const char *dir, char *err, int errlen ) {
	struct luaconf *conf = NULL;
	char path[2048] = {0};
	Table *t = NULL;
	lua_State *L = NULL;

	//
	if ( !( L = luaL_newstate() ) ) {
		return NULL;
	}

	//Build a thing
	if ( snprintf( path, sizeof( path ), "%s%s", dir, lconfname ) == -1 ) {
		//return http_set_error( res, 500, "Could not get full config path." );
		lua_close( L );
		return NULL;
	}

	//Allocate
	FPRINTF( "config path is: %s\n", path );
	if ( !( t = malloc( sizeof(Table) ) ) || !lt_init( t, NULL, 1024 ) ) {
		lua_close( L );
		snprintf( err, errlen, "Failed to allocate heap for configuration." );
		return NULL; //http_set_error( res, 500, "Failed to allocate config table." );
	}

	//Execute
	if ( !lua_exec_file( L, path, err, errlen ) ) {
		lt_free( t );
		free( t );
		lua_close( L );
		return NULL;
	}

	//Check the stack and make sure that it's a table.
	if ( !lua_istable( L, 1 ) ) {
		lt_free( t );
		free( t );
		lua_close( L );
		snprintf( err, errlen, "Configuration is not a table." );
		return NULL; // http_set_error( res, 500, "Configuration is not a table.\n" );
	}

	//Convert it
	if ( !lua_to_table( L, 1, t ) ) {
		lt_free( t );
		free( t );
		lua_close( L );
		snprintf( err, errlen, "Failed to convert config from Lua to table." );
		return NULL; //http_set_error( res, 500, "Failed to convert config table." );
	}

	//Clean up the stack after loading config file...
	lua_pop(L, 1);

	//Set everything
	if ( !( conf = malloc( sizeof( struct luaconf ) ) ) || !memset( conf, 0, sizeof( struct luaconf )) ) {
		lt_free( t );
		free( t );
		lua_close( L );
		snprintf( err, errlen, "Failed to allocate heap for configuration." );
		return NULL; //http_set_error( res, 500, "Failed to convert config table." );
	}

	//Define the rules
	const struct rule rules[] = {
		{ "db", "s", .v.s = &conf->db },
		{ "fqdn", "s", .v.s = &conf->fqdn },
		{ "title", "s", .v.s = &conf->title },
		{ "root_default", "s", .v.s = &conf->root_default },
		{ "static", "s", .v.s = &conf->spath },
		//{ "routes", "s", .v.s = &w->mvc }, //Build these seperately
		{ NULL }
	};

	//Run the rules and build the route list
	loader_run( t, rules );
	conf->routes = build_mvc( t );
	conf->dir = strdup( dir );

	//Debug dump
	FPRINTF( "db: %s\n", conf->db );
	FPRINTF( "fqdn: %s\n", conf->fqdn );
	FPRINTF( "title: %s\n", conf->title );
	FPRINTF( "root_default: %s\n", conf->root_default );
	FPRINTF( "routes:\n" );

	//Destroy things that aren't needed.
	lt_free( t );
	free( t );
	lua_close( L );

	//Any required things that aren't set should cause us to stop...
	return conf;
}


//Find the active route
static struct mvc * find_active_route ( struct luaconf *luaconf, const char *path ) {
	struct routeh *r, **routes = luaconf->routes;

#if 1
	if ( !( r = resolve_routeh( routes, path ) ) ) {
		FPRINTF( "Resolve routes failed to find anything...\n" );
		return NULL;
	}
#else
	while ( routes && *routes ) {
		FPRINTF( "Route: '%s', Path: '%s'\n", (*routes)->name, path );
		if ( resolve_routes( (*routes)->name, path ) ) {
			break;
		}
		routes++;
	}
#endif
	struct mvc * mvc = r->mvc;
	FPRINTF( "models: %p\n", mvc->models );
	FPRINTF( "views: %p\n", mvc->views );
	FPRINTF( "queries: %p\n", mvc->queries );
	FPRINTF( "returns: %s\n", mvc->returns );
	FPRINTF( "auth: %s\n", mvc->auth );
	FPRINTF( "content: %s\n", mvc->content );
	return mvc; 
}


//Build Lua environment
static int build_luaenv ( lua_State *L, struct HTTPBody *req, char *err, int errlen ) {
	//Add the table for our environment.
	lua_newtable( L ); 

	//Put all of these things in something
	struct n { const char *name; struct HTTPRecord **records; } **ttt = 
	(struct n *[]){
		&(struct n){ "headers", req->headers },
		&(struct n){ "get", req->url },
		&(struct n){ "post", req->body },
		NULL
	};

	//Loop through all of the headers
	while ( ttt && *ttt ) {
		FPRINTF( "Name: %s\n", (*ttt)->name );
		lua_pushstring( L, (*ttt)->name ); 
		//TODO: Nils should probably be nils, not empty tables...
		lua_newtable( L );
		while ( (*ttt)->records && (*(*ttt)->records)->field ) {
			struct HTTPRecord *r = (*(*ttt)->records);
		#if 0
			lua_pushustrings( L, r->field, r->value, r->size );
		#else
			FPRINTF( "\t[%s] => %p\n", r->field, r->value );
			lua_pushstring( L, r->field );
			lua_pushlstring( L, (char *)r->value, r->size );
		#endif
			lua_settable( L, 3 );
			(*ttt)->records++;
		}
		lua_settable( L, 1 );
		ttt++;
	}

	//Push the path as well
#if 0
	lua_pushstrings( L, "url", req->path );
#else
	lua_pushstring( L, "url" );
	lua_pushstring( L, req->path );
#endif
	lua_settable( L, 1 );
	lua_setglobal( L, "newenv" );	
	return 1;
}


//Combine everything that came from the call
static Table * combine_models ( lua_State *L, char *err, int errlen ) {
	int top = lua_gettop( L );
	Table *t = malloc( sizeof( Table ) );
	if ( !t || !lt_init( t, NULL, 2048 ) ) {
		snprintf( err, errlen, "Failed to initialize model table." );
		return NULL;
	}
	
	for ( int i = 0; i < top; i++ ) {
		FPRINTF( "%d element(s) left on stack...\n", i );
		//lua_stackdump( L );

		//Check the value of each
		if ( !lua_istable( L, i ) ) {
			snprintf( err, errlen, "Value at stack index %d is not a table.\n", i );
			return NULL;
		}

		//...
		if ( !lua_to_table( L, i, t ) ) {
			snprintf( err, errlen, "Conversion to Table failed.\n" );
			return NULL;
		}

		lua_pop( L, 1 );
	}

	lt_dump( t );
	return t;
}


//Execute all models
Table * execute_models ( struct luaconf *luaconf, lua_State *L, char *err, int errlen ) {
	FPRINTF( "Beginning model execution...\n" );
	struct mvc *mvc = luaconf->mvc;
	char **model = mvc->models;
	Table *t = malloc( sizeof( Table ) );
		
	if ( !t || !lt_init( t, NULL, 2048 ) ) {
		snprintf( err, errlen, "Could not allocate space for tables." );
		return NULL;
	}

	//FPRINTF( "mvc: %p\n", mvc );FPRINTF( "mvc.models: %p\n", mvc->models );
	FPRINTF( "dir: %s\n", luaconf->dir );
	while ( model && *model )  {
		char filename[ 2048 ] = { 0 };
		snprintf( filename, sizeof(filename), modelfmt, luaconf->dir, *model );
		FPRINTF( "Executing model: %s\n", filename );
		if ( !lua_exec_file( L, filename, err, errlen ) ) {
			FPRINTF( err );
			return NULL;
		}

		if ( !lua_istable( L, 1 ) ) {
			snprintf( err, errlen, "Model returned from file '%s' is not a table.\n", filename );
			return NULL;
		}

		if ( !lua_to_table( L, 1, t ) ) {
			snprintf( err, errlen, "Conversion of model from file '%s' failed.\n", filename );
			return NULL;
		}

		lua_pop( L, 1 );
		model++;	
	}

	FPRINTF( "Done with model execution...\n" );
	return t;
}


//
uint8_t * execute_views ( struct luaconf *luaconf, Table *t, int *i, char *err, int errlen ) {
	struct mvc *mvc = luaconf->mvc;
	char **view = mvc->views;
	uint8_t *buf = NULL;
	int buflen = 0;

	while ( view && *view ) {
		uint8_t *fbuf = NULL;
		uint8_t *ren = NULL;
		int flen = 0;
		int renlen = 0;
		char fpath[ 2048 ] = { 0 };
		snprintf( fpath, sizeof( fpath ), "%s/views/%s.%s", luaconf->dir, *view, "tpl" );
		FPRINTF( "%s\n", fpath );

		//Read the entire file to render in and fail...
		if ( !( fbuf = read_file( fpath, &flen, err, sizeof( err ) ) ) ) {
			snprintf( err, errlen, "Failed to read entire view file: %s\n", fpath );
			return NULL; 
		}

		//Then do the render ( model.? )
		if ( !( ren = table_to_uint8t( t, fbuf, flen, &renlen ) ) ) {
			snprintf( err, errlen, "Failed to render model according to view file: %s\n", fpath );
			return NULL; 
		}

#if 1
		//Append to the whole message
		append_to_uint8t( &buf, &buflen, ren, renlen );
#endif
		view++;	
	}
	*i = buflen;
	return buf;
}


static int set_framework_methods() {
#if 1
	//Set framework methods here...
	//lua_set_methods( L, "newenv", libs );
#else
	lua_setglobal( L, "newenv" );	
	lua_stackdump( L );
#endif
	return 1;
}



//filter-lua.c - Run HTTP messages through a Lua handler
//Any processing can be done in the middle.  Since we're in another "thread" anyway
int filter_lua ( struct HTTPBody *req, struct HTTPBody *res, struct config *config, struct host *host ) {

	struct luaconf *luaconf = NULL;
	lua_State *L = NULL;
	uint8_t *buf = NULL;
	int buflen = 0;
	char err[2048] = {0}, lconfpath[2048] = {0};
	Table *model;

	//Check for config (though this is probably already done...)
	if ( !config )
		return http_set_error( res, 500, "No global config is present." );

	if ( !host->dir )
		return http_set_error( res, 500, "No host directory specified." );

	//Try parsing
	if ( !( luaconf = build_luaconf( host->dir, err, sizeof(err) )) )
		return http_set_error( res, 500, "Failed to parse lua config." );

	//Check for and serve any static files 
	//TODO: This should be able to serve a list of files matching a specific type
	if ( check_static_prefix( luaconf->spath, req->path ) ) {
		destroy_luaconf( luaconf );
		return filter_static( req, res, config, host );
	}

	//...
	if ( !( luaconf->mvc = find_active_route( luaconf, req->path ) ) ) {
		destroy_luaconf( luaconf );
		snprintf( err, sizeof(err), "Path %s does not resolve.", req->path );
		return http_set_error( res, 404, err );
	} 	

	//Load Lua libraries...
	if ( !lua_load_libs( &L ) )
		return http_set_error( res, 500, "Could not allocate Lua environment." );

	//Build an environment
	if ( !build_luaenv( L, req, err, sizeof(err) ) ) {
		snprintf( err, sizeof(err), "Set error with build_luaenv." );
		return http_set_error( res, 500, err );
	}

	if ( !set_framework_methods() )
		return http_set_error( res, 500, "No errors occurred." );

	//At this point, server data and libraries are available to Lua, so now run models...
	if ( !( model = execute_models( luaconf, L, err, sizeof(err) ) ) )
		return http_set_error( res, 500, err );

	//Generate some views
	if ( !( buf = execute_views( luaconf, model, &buflen, err, sizeof(err) ) ) )
		return http_set_error( res, 500, err );

#if 1
	//Set all of this
	http_set_ctype( res, mmtref( "text/html" ) );
	http_set_content( res, buf, buflen );
#else
 	uint8_t bug[] = "<h2>Everything is fine.</h2>";
	http_set_content( res, bug, strlen((char *)bug) ); //, sizeof(bug) );
#endif

#if 0
	if ( !http_finalize_response( res, err, sizeof(err) ) )
		return http_set_error( res, 500, err );
#endif

	//Destroy everything
	free( buf );
	lt_free( model );
	free( model );
	destroy_luaconf( luaconf );
	lua_close( L );
	return 1;
}
