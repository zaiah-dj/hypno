#!/usr/bin/env lua
------------------------------------------------------
-- @name
-- ------
-- bench - Run benchmarks against Lua code. 
--
-- @synopsis 
-- ----------
-- None so far.
--
-- @description
-- -------------
-- Runs benchmarks on the core to gauge speed. 
--
-- @options
-- ---------
-- -f, --file <file>     
-- 	Run a benchmark against a file.
--
-- -a, --versus <file>   
-- 	Run a benchmark using <file> as a reference.
--
-- @examples
-- ----------
--
-- @caveats
-- ---------
-- Benchmark targets are similar to test targets.  
-- Carefully named comments can be spread out through
-- files and preloaded that way to see exactly how
-- long it takes to run parts of code. 
--
-- The other way modifies Lua's normal run time and
-- logs the time it took to run items.
--
-- @copyright
-- ----------
-- <mit>
--
-- @author
-- -------
-- <author> 
--
-- @see-also
-- ---------
-- <see>
--
-- @todo
-- -----
-- 	- Indicate whether or not to support dashes
-- 
-- @end
-- ----
------------------------------------------------------