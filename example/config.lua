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
--[[
		-- See the dirent filter in action 
		["dir.hypno"] = { 
			dir = "dir",
			root_default = "/index.html",
			filter = "dirent"
		},
--]]

		-- See the C filter in action
		["app.hypno"] = { 
			root_default = "/index.html", -- Technically, no root is needed
			filter = "c"
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
	
		--[[	
		-- See LuaMVC in action
		["luamvc.hypno"] = { 
			dir = "luamvc",
			root_default = "/index.html",
			filter = "lua"
		},
		--]]

		-- SSL hosts look like this
		--[[
		-- A static host (I'm GUESSING, that we're crashing because no host points anywhere)
		["ssl.hypno"] = { 
			dir = "/home/ramar/prj/site/hypno-www/collinshosting.com",
			root_default = "/index.html",
			ca_bundle = "collinshosting_com.ca-bundle",
			cert_file = "collinshosting_com.crt",
			keyfile = "x509-collinshosting-key.pem",
			filter = "static"
		},
		--]]
	}
}