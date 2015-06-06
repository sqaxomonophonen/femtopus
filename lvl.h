#ifndef LVL_H

#include <stdint.h>

#include "mat.h"
#include "scratch.h"

struct lvl_vertex {
	union vec3 co;
	union vec2 uv;
};

struct lvl_chunk {
	int n_vertices;
	struct lvl_vertex* vertices;

	/*
	polygon list encoded as:
	  vertex count (0 terminates list)
	  material index
	  vertex indices...
	  ...
	  (sort by material index if you're a nice guy)
	*/
	uint32_t* polygon_list;

	int n_portal_indices;
	uint32_t* portal_indices;
};


struct lvl_portal {
	uint32_t chunk_indices[2];

	/*
	vertex pairs link vertices in two chunks together. each pair in
	vertex_pairs is encoded as 2 x uint32_t; a vertex index for each chunk.
	total length is n_convex_vertex_pairs + n_additional_vertex_pairs.
	convex pairs first, then comes additional pairs. convex pairs must
	enclose the convex set of the portal. the additional pairs are meant
	for welding vertices within the convex set.
	*/
	int n_convex_vertex_pairs;
	int n_additional_vertex_pairs;
	uint32_t* vertex_pairs;
};

#define LVL_MATERIAL_NAME_MAX_LENGTH (64)
struct lvl_material {
	char name[LVL_MATERIAL_NAME_MAX_LENGTH];
};

#if 0
struct lvl_entity {
	uint32_t inside_chunk;
	union vec3 position;
};
#endif

struct lvl {
	struct scratch scratch;

	int n_chunks;
	struct lvl_chunk* chunks;

	int n_portals;
	struct lvl_portal* portals;

	int n_materials;
	struct lvl_material* materials;
};


void lvl_init(struct lvl* lvl, int n_chunks, int n_portals, int n_materials);

struct lvl_chunk* lvl_get_chunk(struct lvl* lvl, int chunk_index);
struct lvl_chunk* lvl_init_chunk(struct lvl* lvl, int chunk_index, int n_vertices, int polygon_list_size, int n_portal_indices);

struct lvl_portal* lvl_get_portal(struct lvl* lvl, int portal_index);
struct lvl_portal* lvl_init_portal(struct lvl* lvl, int portal_index, int n_convex_vertex_pairs, int n_additional_vertex_pairs);

struct lvl_material* lvl_get_material(struct lvl* lvl, int material_index);

int lvl_chunk_validate_polygon_list(struct lvl* lvl, struct lvl_chunk* chunk, int n_vertices, int polygon_list_size, char* errstr1024);
int lvl_validate_misc(struct lvl* lvl, char* errstr1024);

#define LVL_H
#endif
