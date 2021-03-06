/* ------------------------------------------- * 
 * filesystem.c 
 * ============
 * 
 * Summary 
 * -------
 * Database primitives for Lua
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * - added shadow primitives
 * - added fs{read, write, stat & pwd}
 * 
 * TODO
 * ----
 * - add mkdir, rmdir, rm -rf, and ls
 * 
 * ------------------------------------------- */
#include "filesystem.h"

static char *sw_path( lua_State *L, const char *path ) {
	char *spath = NULL;
	int len = 0;

	//Get the shadow path if there is one
	lua_getglobal( L, "shadow" ); 
	if ( lua_isnil( L, -1 ) ) {
		lua_pop( L, 1 );
		return NULL;
	}

	//Translate to the right path
	if ( !( spath = malloc( 2048 ) ) || !memset( spath, 0, 2048 ) ) {
		return NULL;
	}

	len = snprintf( spath, 2047, "%s", lua_tostring( L, -1 ) );
	lua_pop( L, 1 );

	//Stop if the buffer is too large (this is bigger than PATH_MAX)
	if ( ( 2047 - len ) <= strlen( path ) ) {
		free( spath );
		return NULL;
	}

	snprintf( &spath[ len ], 2047 - len, "/%s", path );
	return spath;
}



static zTable * get_config_limits( lua_State *L ) {
	//Get the shadow path if there is one
	lua_getglobal( L, "config" ); 
	if ( lua_isnil( L, -1 ) ) {
		lua_pop( L, 1 );
		return NULL;
	}
	//lua_pushnumber( L, -1 );
	zTable *t = lt_make( 1024 );
	if ( !lua_to_ztable( L, 1, t ) ) {
		return NULL;
	} 

	lt_lock( t );
	lua_pop( L, 1 );
	return t;
} 



int fs_pwd ( lua_State *L ) {
	char *fspath = NULL, fp[ 2048 ] = {0}, rp[ 2048 ] = {0};
	struct stat sb;

	//Check for a string argument
	luaL_checktype( L, 1, LUA_TSTRING );

	//Seems like this should never happen
	if ( !( fspath = ( char * )sw_path( L, "." ) ) ) {
		return luaL_error( L, "Could not find shadow directory" );
	}

#if 1
	realpath( fspath, rp );
	lua_pushstring( L, rp );	
	free( fspath );
	return 1;
#else
	//Stat
	if ( stat( fspath, &sb ) == -1 ) {
		return luaL_error( L, "Could not find '%s': %s.", fspath, strerror( errno ) );
	}

	realpath( fspath, rp );
	lua_pushstring( L, rp );	
	free( fspath );
	return 1;
#endif
}



int fs_read ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TSTRING );
	struct stat sb;
	int fd, rlimit = 0;
	char *fspath, err[ 1024 ] = { 0 }; 
	const char *funct = "fs.read", *filename = lua_tostring( L, 1 );
	unsigned char *rb = NULL;

	//Seems like this should never happen
	if ( !( fspath = ( char * )sw_path( L, filename ) ) ) {
		return luaL_error( L, "Could not find shadow directory" );
	}

	//Do a stat and compare against a decent size
	if ( stat( fspath, &sb ) == -1 ) {
		return luaL_error( L, "Error opening '%s': %s.", fspath, strerror( errno ) );
	}

	//Pop and get limits, etc
	lua_pop( L, 1 );
	zTable * ct = get_config_limits( L );	
	if ( !ct )
		rlimit = 100000;
	else {
		int i = lt_geti( ct, "readlimit" );
		if ( !( rlimit = lt_int_at( ct, i ) ) ) {
			rlimit = 100000;
		}
	}

	//Free resources
	lt_free( ct ), free( ct );

	//Read a file if the sizes are right
	if ( sb.st_size > rlimit ) {
		return luaL_error( L, "Could not find shadow directory" );
	}

	if ( ( fd = open( fspath, O_RDONLY ) ) == -1 ) {
		free( fspath );
		return luaL_error( L, "%s: %s", funct, strerror( errno ) );
	}

	if ( !( rb = malloc( sb.st_size + 1 ) ) || !memset( rb, 0, sb.st_size + 1 ) ) {
		free( fspath );
		return luaL_error( L, "%s: %s", funct, strerror( errno ) );
	}

	if ( read( fd, rb, sb.st_size ) == -1 ) {
		free( rb ), free( fspath );
		return luaL_error( L, "%s: %s", funct, strerror( errno ) );
	}

	if ( close( fd ) == -1 ) {
		free( rb ), free( fspath );
		return luaL_error( L, "%s: %s", funct, strerror( errno ) );
	}

	//Add a table and just return info
	free( fspath );
	lua_newtable( L );
	lua_pushstring( L, "results" );
	lua_newtable( L );
	lua_pushstring( L, "size" );
	lua_pushinteger( L, sb.st_size );
	lua_settable( L, 3 );
	lua_pushstring( L, "content" );
	if ( !lua_pushlstring( L, (char *)rb, sb.st_size ) ) {
		return luaL_error( L, "%s: %s", funct, "Could not copy binary string." );
	}
	free( rb );
	lua_settable( L, 3 );
	lua_settable( L, 1 );

	//
	lua_pushstring( L, "status" );
	lua_pushboolean( L, 1 );
	lua_settable( L, 1 );
	return 1;
}


int fs_write ( lua_State *L ) {
	//Need to write to a file
	luaL_checktype( L, 1, LUA_TSTRING );
	luaL_checktype( L, 2, LUA_TSTRING );
	if ( lua_gettop( L ) == 3 ) { 
		luaL_checktype( L, 3, LUA_TNUMBER );
	}

	//
	char *fspath = NULL;
	const char *src = lua_tostring( L, 1 ); 
	const unsigned char *data;
	unsigned long size = 0;
	int fd, mode = 0644;

	if ( !( fspath = sw_path( L, src ) ) ) {
		return luaL_error( L, "Could not get new shadow path" );
	}

	if ( lua_gettop( L ) < 3 ) {
		data = (unsigned char *)lua_tostring( L, 2 );
		size = strlen( (char *)data );
	}
	else {
		size = lua_tointeger( L, 3 );
		data = (unsigned char *)lua_tolstring( L, 2, &size );
	}

	lua_pop( L, lua_gettop( L ) );
	if ( ( fd = open( fspath, O_RDWR | O_CREAT | O_TRUNC, mode ) ) == -1 ) {
		return luaL_error( L, "Could not create new file" );
	}

	if ( write( fd, data, size ) == -1 ) {
		return luaL_error( L, "Could not write data to file" );
	}

	if ( close( fd ) == -1 ) {
		return luaL_error( L, "Could not close file" );
	}

	//
	lua_newtable( L );
	lua_pushstring( L, "status" );
	lua_pushstring( L, "true" );
	lua_settable( L, 1 );
	lua_pushstring( L, "message" );
	lua_pushfstring( L, "Wrote %d bytes to file %s", size, fspath );
	lua_settable( L, 1 );
	free( fspath );
	return 1;
}



int fs_stat ( lua_State *L ) {
	struct stat sb;
	char *fspath = NULL;
	luaL_checktype( L, 1, LUA_TSTRING );
	//What's on the stack?
	//lua_istack( L ); getchar ();

	//Seems like this should never happen
	if ( !( fspath = ( char * )sw_path( L, lua_tostring( L, 1 ) ) ) ) {
		return luaL_error( L, "Could not find shadow directory" );
	}

	if ( stat( fspath, &sb ) == -1 ) {
		return luaL_error( L, "Error opening '%s': %s.", fspath, strerror( errno ) );
	}

	//Add a table and just return info
	lua_pop( L, 1 );
	lua_newtable( L );
	
	struct fs_item { const char *key; int d, tostr; } fs_items[] = {
		{ "device", sb.st_dev, 0 }	
	,	{ "inode", sb.st_ino, 0 }	
	,	{ "mode", sb.st_mode, 0 }	
	,	{ "linkcount", sb.st_nlink, 0 }	
	,	{ "uid", sb.st_uid, 0 }	
	,	{ "gid", sb.st_gid, 0 }	
	,	{ "dev_id", sb.st_rdev, 0 }	
	,	{ "size", sb.st_size, 0 }	
	,	{ "blocksize", sb.st_blksize, 0 }	
	,	{ "blockcount", sb.st_blocks, 0 }	
	,	{ "atime", sb.st_atime, 0 }	
	,	{ "mtime", sb.st_mtime, 0 }	
	,	{ "ctime", sb.st_ctime, 0 }	
	,	{ NULL }
	};

	for ( struct fs_item *item = fs_items; item->key; item++ ) {
		lua_pushstring( L, item->key );
		if ( !item->tostr )
			lua_pushnumber( L, item->d );	
		else {
			char *str = "convert_number_to_string";
			lua_pushstring( L, str );	
		}
		lua_settable( L, 1 );
	}
	
	return 1;
}


#if 0
int fs_open ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TSTRING );
	return 0;
}


int fs_close ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TSTRING );
	return 0;
}
#endif

struct luaL_Reg fs_set[] = {
 	{ "read", fs_read }
,	{ "write", fs_write }
,	{ "stat", fs_stat }
,	{ "pwd", fs_pwd }

#if 0
,	{ "list", fs_list }
,	{ "mkdir", fs_mkdir }
,	{ "rmdir", fs_rmdir }
,	{ "delete", fs_remove }
#endif
#if 0
	//The standard library already handles these pretty well...
	//Why repeat it here?
,	{ "open", fs_open }
,	{ "close", fs_close }
#endif
,	{ NULL }
};


