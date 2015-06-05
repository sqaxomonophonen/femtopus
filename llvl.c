#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "llvl.h"

#include "a.h"

static void setup_package_path(lua_State* L)
{
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "path");
	lua_pushstring(L, ";lua/?.lua"); // TODO new location eventually? relative to game root? something?
	lua_concat(L, 2);
	lua_setfield(L, -2, "path");
	lua_pop(L, 1);
}

static void pcall(lua_State* L, int nargs, int nresults)
{
	int status = lua_pcall(L, nargs, nresults, 0);
	if (status != 0) {
		const char* err = lua_tostring(L, -1);
		switch (status) {
			case LUA_ERRRUN:
				arghf("(lua runtime error) %s", err);
				break;
			case LUA_ERRMEM:
				arghf("(lua memory error) %s", err);
				break;
			case LUA_ERRERR:
				arghf("(lua error handler error) %s", err);
				break;
			default:
				arghf("(lua unknown error) %s", err);
				break;
		}
	}
}

void llvl_build(const char* plan, struct lvl* lvl)
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	setup_package_path(L);

	lua_getglobal(L, "require");
	lua_pushstring(L, "build");
	pcall(L, 1, 1);
	if (!lua_isfunction(L, -1)) arghf("expected require('build') to yield a function");

	lua_pushstring(L, plan);
	pcall(L, 1, 1);
	if (!lua_istable(L, -1)) arghf("expected require('build')(\"%s\") to yield a table", plan);

	// TODO

	lua_close(L);
}

