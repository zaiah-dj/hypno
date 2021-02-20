/* ------------------------------------------- * 
 * filter-lua.h
 * ===========
 * 
 * Summary 
 * -------
 * Header file for functions comprising the Lua filter for interpreting HTTP 
 * messages. 
 *
 * Usage
 * -----
 * filter-lua.c enables Hypno to use Lua as a scripting language for creating
 * HTTP responses.
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
#include "mime.h"
#include "util.h"
#include "luabind.h"
#include "configs.h"
#include "loader.h"
#include "router.h"
#include "mvc.h"
#include "server.h"
#include "../vendor/zhttp.h"
#include "../vendor/zrender.h"

#ifndef FILTER_LUA_H
#define FILTER_LUA_H

const int filter_lua( int, struct HTTPBody *, struct HTTPBody *, struct cdata * );

struct luaconf {
	char *db;	
	char *fqdn;
	char *title;
	char *root_default;
	char *spath;
	char *dir;
	struct mvc *mvc;
	struct routeh **routes;
};
#endif
