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

struct lvl_chunk* lvl_get_chunk(struct lvl* lvl, int chunk_index)
{
	ASSERT(chunk_index > 0);
	ASSERT(chunk_index < lvl->n_chunks);
	return &lvl->chunks[chunk_index];
}

struct lvl_chunk* lvl_init_chunk(struct lvl* lvl, int chunk_index, int n_vertices, int polygon_list_size)
{
	struct lvl_chunk* chunk = lvl_get_chunk(lvl, chunk_index);
	chunk->vertices = scratch_alloc(&lvl->scratch, sizeof(*chunk->vertices) * n_vertices);
	chunk->polygon_list = scratch_alloc(&lvl->scratch, sizeof(*chunk->polygon_list) * polygon_list_size);
	return chunk;
}

struct lvl_portal* lvl_get_portal(struct lvl* lvl, int portal_index)
{
	ASSERT(portal_index > 0);
	ASSERT(portal_index < lvl->n_portals);
	return &lvl->portals[portal_index];
}

struct lvl_portal* lvl_init_portal(struct lvl* lvl, int portal_index, int n_convex_vertex_pairs, int n_additional_vertex_pairs)
{
	struct lvl_portal* portal = lvl_get_portal(lvl, portal_index);
	portal->n_convex_vertex_pairs = n_convex_vertex_pairs;
	portal->n_additional_vertex_pairs = n_additional_vertex_pairs;
	int n_total = n_convex_vertex_pairs + n_additional_vertex_pairs;
	portal->vertex_pairs = scratch_alloc(&lvl->scratch, sizeof(*portal->vertex_pairs) * n_total);
	return portal;
}

struct lvl_material* lvl_get_material(struct lvl* lvl, int material_index)
{
	ASSERT(material_index > 0);
	ASSERT(material_index < lvl->n_materials);
	return &lvl->materials[material_index];
}

