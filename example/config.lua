-------------------------------------------------
-- config.lua
-- ==========
--
-- @summary
-- Server config file for example htdocs of libhypno. Running the target 
-- `make examples` will start a server with these four sites loaded.  
--
-- NOTE: The names below will need to be listed in your /etc/hosts file to
-- resolve on your system.
--
-- @usage
-- Sites can be added by simply creating the directory you want, and 
-- adding it to the list below.
--
-- @changelog
-- nothing yet...
-- 
-------------------------------------------------
return {
	wwwroot = "example",
	hosts = {
		-- Default host in case no domain is specified
		["localhost"] = { 
			root_default = "/index.html",
			dir = "localhost",
			filter = "static"
		},
		
		-- Redirect does nothing of real importance
		["redirect.hypno"] = { 
			-- Where to redirect?
			filter = "redirect"
		},

		-- See the dirent filter in action 
		["dir.hypno"] = { 
			dir = "dir",
			root_default = "/index.html",
			filter = "dirent"
		},

		-- See the echo filter in action
		["echo.hypno"] = { 
			root_default = "/index.html", -- Technically, no root is needed
			filter = "echo"
		},

		-- See the static filter in action
		["html.hypno"] = { 
			dir = "html",
			root_default = "/index.html",
			filter = "static"
		},
	
		-- See the static filter in action
		["lua.hypno"] = { 
			dir = "lua",
			root_default = "/index.html",
			filter = "lua"
		},
	
		-- SSL capable hosts look like this
		["supergreatwok.xyz"] = { 
			dir = "ssl",
			root_default = "/index.html",
			ca_bundle = "misc/ssl/supergreatwok.xyz/supergreatwok_xyz.ca-bundle",
			cert_file = "misc/ssl/supergreatwok.xyz/supergreatwok_xyz.crt",
			key_file = "misc/ssl/supergreatwok.xyz/supergreatwok_xyz.pem",
			filter = "static"
		},
	}
}
