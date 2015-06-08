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

	lvl->gravity = vec3_xyz(0, -10, 0);
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

void lvl_entity_dlook(struct lvl_entity* e, float dyaw, float dpitch)
{
	e->yaw += dyaw;
	e->pitch += dpitch;
	float pitch_limit = 90;
	if (e->pitch > pitch_limit) e->pitch = pitch_limit;
	if (e->pitch < -pitch_limit) e->pitch = -pitch_limit;
}

#if 0
void lvl_entity_flymove(struct lvl* lvl, struct lvl_entity* e, float forward, float right)
{
	e->position = vec3_add(e->position, vec3_move(e->yaw, e->pitch, forward, right));
}
#endif

void lvl_entity_move(struct lvl_entity* e, float forward, float right, float jump)
{
	e->move_forward = forward;
	e->move_right = right;
	e->move_jump = jump;
}

void lvl_entity_accelerate(struct lvl_entity* e, union vec3 a, float dt)
{
	e->velocity = vec3_add(e->velocity, vec3_scale(a, dt));
}

void lvl_entity_impulse(struct lvl_entity* e, union vec3 imp)
{
	e->velocity = vec3_add(e->velocity, imp);
}

static struct aabb lvl_entity_aabb(struct lvl_entity* e)
{
	struct aabb aabb;

	aabb.center = e->position;

	float height = 1;
	float r = 0.5;
	union vec3 extent = {{r, height, r}};
	aabb.extent = extent;

	return aabb;
}

static void lvl_entity_clipmove(struct lvl* lvl, struct lvl_entity* e, union vec3 r, int n_steps)
{
	union vec3 rstep = vec3_scale(r, 1.0 / (float)n_steps);

	for (int i = 0; i < n_steps; i++) {
		e->position = vec3_add(e->position, rstep);
		struct aabb aabb = lvl_entity_aabb(e);

		struct lvl_chunk* chunk = lvl_get_chunk(lvl, 0); // XXX
		AN(chunk);
		AN(chunk->polygon_list);
		int cursor = 0;

		while (1) {
			int vertex_count = chunk->polygon_list[cursor++];
			if (vertex_count == 0) break;

			cursor++; // skip material index

			ASSERT(vertex_count <= 32);

			union vec3 polygon[32];
			for (int i = 0; i < vertex_count; i++) {
				struct lvl_vertex lv = chunk->vertices[chunk->polygon_list[cursor++]];
				polygon[i] = lv.co;
			}

			union vec3 mtv;
			int intersect = polygon_aabb_mtv(
				aabb,
				polygon,
				vertex_count,
				&mtv);

			if (intersect) {
				float rmtv = vec3_length(mtv);
				if (rmtv > 1e-8) {
					union vec3 normal = vec3_scale(mtv, 1.0 / rmtv);
					e->position = vec3_add(e->position, mtv);
					e->velocity = vec3_sub(e->velocity, vec3_scale(normal, vec3_dot(e->velocity, normal)));
				}
			}
		}
	}
}

void lvl_entity_update(struct lvl* lvl, struct lvl_entity* e, float dt)
{
	union vec3 moveacc = vec3_scale(vec3_move(e->yaw, e->pitch, e->move_forward, e->move_right), 10);
	lvl_entity_accelerate(e, vec3_add(lvl->gravity, moveacc), dt);
	union vec3 jump_vector = {{0,0.4,0}};
	lvl_entity_impulse(e, vec3_scale(jump_vector, e->move_jump));
	e->move_forward = 0;
	e->move_right = 0;
	e->move_jump = 0;

	lvl_entity_clipmove(lvl, e, vec3_scale(e->velocity, dt), 8);
}

struct mat44 lvl_entity_view(struct lvl_entity* e)
{
	struct mat44 m = mat44_identity();
	m = mat44_rotate_x(m, e->pitch);
	m = mat44_rotate_y(m, e->yaw);
	m = mat44_translate(m, vec3_scale(e->position, -1));
	return m;
}
