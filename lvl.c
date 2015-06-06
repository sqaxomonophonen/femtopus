#include <stdio.h>

#include "lvl.h"

void lvl_init(struct lvl* lvl, int n_chunks, int n_portals, int n_materials)
{
	memset(lvl, 0, sizeof(*lvl));

	scratch_init(&lvl->scratch, 1<<22);

	lvl->n_chunks = n_chunks;
	lvl->chunks = scratch_alloc(&lvl->scratch, sizeof(*lvl->chunks) * n_chunks);

	lvl->n_portals = n_portals;
	lvl->portals = scratch_alloc(&lvl->scratch, sizeof(*lvl->portals) * n_portals);

	lvl->n_materials = n_materials;
	lvl->materials = scratch_alloc(&lvl->scratch, sizeof(*lvl->materials) * n_materials);
}

void lvl_free(struct lvl* lvl)
{
	scratch_free(&lvl->scratch);
}

struct lvl_chunk* lvl_get_chunk(struct lvl* lvl, uint32_t chunk_index)
{
	ASSERT(chunk_index < lvl->n_chunks);
	return &lvl->chunks[chunk_index];
}

struct lvl_chunk* lvl_init_chunk(struct lvl* lvl, int chunk_index, int n_vertices, int polygon_list_size, int n_portal_indices)
{
	struct lvl_chunk* chunk = lvl_get_chunk(lvl, chunk_index);

	chunk->n_vertices = n_vertices;
	chunk->vertices = scratch_alloc(&lvl->scratch, sizeof(*chunk->vertices) * n_vertices);

	chunk->polygon_list = scratch_alloc(&lvl->scratch, sizeof(*chunk->polygon_list) * polygon_list_size);

	chunk->n_portal_indices = n_portal_indices;
	chunk->portal_indices = scratch_alloc(&lvl->scratch, sizeof(*chunk->portal_indices) * n_portal_indices);

	return chunk;
}

struct lvl_portal* lvl_get_portal(struct lvl* lvl, uint32_t portal_index)
{
	ASSERT(portal_index < lvl->n_portals);
	return &lvl->portals[portal_index];
}

struct lvl_portal* lvl_init_portal(struct lvl* lvl, int portal_index, int n_convex_vertex_pairs, int n_additional_vertex_pairs)
{
	struct lvl_portal* portal = lvl_get_portal(lvl, portal_index);
	portal->n_convex_vertex_pairs = n_convex_vertex_pairs;
	portal->n_additional_vertex_pairs = n_additional_vertex_pairs;
	int n = (n_convex_vertex_pairs + n_additional_vertex_pairs) * 2;
	portal->vertex_pairs = scratch_alloc(&lvl->scratch, sizeof(*portal->vertex_pairs) * n);
	return portal;
}

struct lvl_material* lvl_get_material(struct lvl* lvl, uint32_t material_index)
{
	ASSERT(material_index < lvl->n_materials);
	return &lvl->materials[material_index];
}

int lvl_chunk_validate_polygon_list(struct lvl* lvl, struct lvl_chunk* chunk, int n_vertices, int polygon_list_size, char* errstr1024)
{
	int state = 0;
	uint32_t vertices_remaining = 0;
	for (int i = 0; i < polygon_list_size; i++) {
		uint32_t value = chunk->polygon_list[i];

		if (state == 0) {
			if (value == 0) {
				if (i == (polygon_list_size-1)) {
					// list properly terminated
					return 0;
				} else {
					snprintf(errstr1024, 1024, "polygon list prematurely terminated at index %d/%d", i, polygon_list_size);
					return 1002;
				}
			} else {
				if (value < 3) {
					// polygons must have at least 3 vertices
					snprintf(errstr1024, 1024, "invalid polygon size %u at index %d/%d", value, i, polygon_list_size);
					return 1003;
				}
				vertices_remaining = value;
				state = 1;
			}
		} else if (state == 1) {
			if (value >= lvl->n_materials) {
				snprintf(errstr1024, 1024, "material index %u/%d out of bounds at polygon list index %d/%d", value, lvl->n_materials, i, polygon_list_size);
				return 1101;
			} else {
				state = 2;
			}
		} else if (state == 2) {
			if (value >= n_vertices) {
				snprintf(errstr1024, 1024, "vertex index %u/%d out of bounds at polygon list index %d/%d", value, n_vertices, i, polygon_list_size);
				return 1102;
			} else {
				vertices_remaining--;
				if (vertices_remaining == 0) {
					state = 0;
				}
			}
		}
	}

	snprintf(errstr1024, 1024, "terminator not found at end of list");
	return -1;
}

int lvl_validate_misc(struct lvl* lvl, char* errstr1024)
{
	// check that portal indices are within bounds
	for (int i = 0; i < lvl->n_chunks; i++) {
		struct lvl_chunk* chunk = lvl_get_chunk(lvl, i);
		for (int j = 0; j < chunk->n_portal_indices; j++) {
			uint32_t v = chunk->portal_indices[j];
			if (v >= lvl->n_portals) {
				snprintf(errstr1024, 1024, "portal index %u out of bounds at portal_indices[%d]", v, j);
				return 1001;
			}
		}
	}

	// check portal
	for (int i = 0; i < lvl->n_portals; i++) {
		struct lvl_portal* portal = lvl_get_portal(lvl, i);

		// check that chunk indices are within bounds
		struct lvl_chunk* chunks[2];
		for (int j = 0; j < 2; j++) {
			uint32_t chunk_index = portal->chunk_indices[j];
			if (chunk_index >= lvl->n_chunks) {
				snprintf(errstr1024, 1024, "chunk index %u/%d out of bounds in portal %d\n", chunk_index, lvl->n_chunks, i);
				return 2001;
			}
			chunks[j] = lvl_get_chunk(lvl, chunk_index);
		}

		// check that vertex indices are within bounds
		int n = (portal->n_convex_vertex_pairs + portal->n_additional_vertex_pairs) * 2;
		for (int j = 0; j < n; j++) {
			uint32_t vertex_index = portal->vertex_pairs[j];
			int side = j&1;
			struct lvl_chunk* chunk = chunks[side];
			if (vertex_index >= chunk->n_vertices) {
				snprintf(errstr1024, 1024, "vertex index %u out of bounds at portal_indices[%d]", vertex_index, j);
				return 2002;
			}
		}
	}

	return 0;
}
