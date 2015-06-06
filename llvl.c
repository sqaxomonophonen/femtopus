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

static int table_length(lua_State* L, const char* field)
{
	lua_getfield(L, -1, field);
	if (!lua_istable(L, -1)) arghf("\"%s\" is not a table", field);
	int length = lua_rawlen(L, -1);
	lua_pop(L, 1);
	return length;
}

static void populate_vec2(lua_State* L, union vec2* v)
{
	for (int i = 0; i < 2; i++) {
		lua_rawgeti(L, -1, i+1);
		v->s[i] = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

static void populate_vec3(lua_State* L, union vec3* v)
{
	for (int i = 0; i < 3; i++) {
		lua_rawgeti(L, -1, i+1);
		v->s[i] = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

static void populate_lvl(lua_State* L, struct lvl* lvl)
{
	char errstr1024[1024];

	if (!lua_istable(L, -1)) arghf("expected a table");

	int n_chunks = table_length(L, "chunks");
	int n_portals = table_length(L, "portals");
	int n_materials = table_length(L, "materials");

	lvl_init(lvl, n_chunks, n_portals, n_materials);

	{
		lua_getfield(L, -1, "chunks");
		for (int i = 0; i < n_chunks; i++) {
			lua_rawgeti(L, -1, i+1);
			int n_vertices = table_length(L, "vertices");
			int polygon_list_size = table_length(L, "polygon_list");
			int n_portal_indices = table_length(L, "portal_indices");
			struct lvl_chunk* chunk = lvl_init_chunk(lvl, i, n_vertices, polygon_list_size, n_portal_indices);

			// vertices
			lua_getfield(L, -1, "vertices");
			for (int j = 0; j < n_vertices; j++) {
				lua_rawgeti(L, -1, j+1);
				struct lvl_vertex* vertex = &chunk->vertices[j];

				lua_getfield(L, -1, "co");
				populate_vec3(L, &vertex->co);

				lua_getfield(L, -1, "uv");
				populate_vec2(L, &vertex->uv);

				lua_pop(L, 1);
			}
			lua_pop(L, 1);

			// polygon list
			lua_getfield(L, -1, "polygon_list");
			for (int j = 0; j < polygon_list_size; j++) {
				lua_rawgeti(L, -1, j+1);
				chunk->polygon_list[j] = lua_tointeger(L, -1);
				lua_pop(L, 1);
			}
			lua_pop(L, 1);

			{
				int err = lvl_chunk_validate_polygon_list(lvl, chunk, n_vertices, polygon_list_size, errstr1024);
				if (err) arghf("lvl_chunk_validate_polygon_list: %s (%d)", errstr1024, err);
			}

			// portal indices
			lua_getfield(L, -1, "portal_indices");
			for (int j = 0; j < n_portal_indices; j++) {
				lua_rawgeti(L, -1, j+1);
				chunk->portal_indices[j] = lua_tointeger(L, -1);
				lua_pop(L, 1);
			}
			lua_pop(L, 1);

			lua_pop(L, 1); // chunks[i]
		}
		lua_pop(L, 1); // chunks
	}

	{
		lua_getfield(L, -1, "portals");
		for (int i = 0; i < n_portals; i++) {
			lua_rawgeti(L, -1, i+1);

			int n_convex_vertex_pairs = table_length(L, "convex_vertex_pairs");
			int n_additional_vertex_pairs = table_length(L, "additional_vertex_pairs");

			struct lvl_portal* portal = lvl_init_portal(lvl, i, n_convex_vertex_pairs, n_additional_vertex_pairs);

			// chunk indices
			int n_chunk_indices = table_length(L, "chunk_indices");
			if (n_chunk_indices != 2) {
				arghf("portals[%d].chunk_indices must contain exactly 2 elements", i+1);
			}
			lua_getfield(L, -1, "chunk_indices");
			for (int j = 0; j < 2; j++) {
				lua_rawgeti(L, -1, j+1);
				portal->chunk_indices[j] = lua_tointeger(L, -1);
				lua_pop(L, 1);
			}
			lua_pop(L, 1);


			// vertex pairs
			int offset = 0;

			lua_getfield(L, -1, "convex_vertex_pairs");
			for (int j = 0; j < n_convex_vertex_pairs; j++) {
				lua_rawgeti(L, -1, j+1);
				portal->vertex_pairs[offset++] = lua_tointeger(L, -1);
				lua_pop(L, 1);
			}
			lua_pop(L, 1);

			lua_getfield(L, -1, "additional_vertex_pairs");
			for (int j = 0; j < n_additional_vertex_pairs; j++) {
				lua_rawgeti(L, -1, j+1);
				portal->vertex_pairs[offset++] = lua_tointeger(L, -1);
				lua_pop(L, 1);
			}
			lua_pop(L, 1);

			lua_pop(L, 1); // portals[i];
		}
		lua_pop(L, 1); // portals
	}

	{
		lua_getfield(L, -1, "materials");
		for (int i = 0; i < n_materials; i++) {
			lua_rawgeti(L, -1, i+1);

			struct lvl_material* material = lvl_get_material(lvl, i);

			lua_getfield(L, -1, "name");
			size_t len;
			const char* str = lua_tolstring(L, -1, &len);
			if (len > (LVL_MATERIAL_NAME_MAX_LENGTH-1)) {
				arghf("materials[%d].name length of %zd exceeds max length (%d)", i+1, len, LVL_MATERIAL_NAME_MAX_LENGTH-1);
			}
			strcpy(material->name, str);
			lua_pop(L, 1);

			lua_pop(L, 1); // materials[i]
		}
		lua_pop(L, 1); // materials
	}

	lua_pop(L, 1); // lvl

	{
		int err = lvl_validate_misc(lvl, errstr1024);
		if (err) arghf("lvl_validate_misc: %s (%d)", errstr1024, err);
	}
}

void llvl_build(const char* plan_name, struct lvl* lvl)
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	setup_package_path(L);

	lua_getglobal(L, "require");
	lua_pushstring(L, "build");
	pcall(L, 1, 1);
	if (!lua_isfunction(L, -1)) arghf("expected require('build') to yield a function");

	lua_pushstring(L, plan_name);
	pcall(L, 1, 1);

	populate_lvl(L, lvl);

	lua_close(L);
}

