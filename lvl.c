#include <stdio.h>

#include "lvl.h"

static void lvl_set_gravity(struct lvl* lvl, union vec3 v)
{
	lvl->gravity = v;
	lvl->gravity_normalized = vec3_normalize(lvl->gravity);
}

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

	lvl_set_gravity(lvl, vec3_xyz(0, -10, 0));
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

int lvl_get_material_index(struct lvl* lvl, const char* name)
{
	// TODO can use binary search if materials are sorted by name
	for (int i = 0; i < lvl->n_materials; i++) {
		struct lvl_material* m = lvl_get_material(lvl, i);
		if (strcmp(m->name, name) == 0) return i;
	}
	return -1;
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

static float lvl_entity_max_step_up(struct lvl_entity* e)
{
	return 0.4f;
}

struct lvl_aabb_mtv_iterator {
	// setup
	struct lvl* lvl;
	struct aabb aabb;
	uint32_t origin_chunk_index;

	// state
	int polygon_list_cursor;

	// result
	uint32_t material_index;
	union vec3 mtv;
};

inline static void lvl_aabb_mtv_iterator_init(struct lvl_aabb_mtv_iterator* it, struct lvl* lvl, struct aabb aabb, uint32_t origin_chunk_index)
{
	memset(it, 0, sizeof(*it));
	it->lvl = lvl;
	it->aabb = aabb;
	it->origin_chunk_index = origin_chunk_index;
}

inline static void lvl_aabb_mtv_iterator_init_from_entity(struct lvl_aabb_mtv_iterator* it, struct lvl* lvl, struct lvl_entity* e)
{
	lvl_aabb_mtv_iterator_init(it, lvl, lvl_entity_aabb(e), e->chunk_index);
}

inline static void lvl_aabb_mtv_iterator_init_from_other_iterator(struct lvl_aabb_mtv_iterator* it, struct lvl_aabb_mtv_iterator* other)
{
	lvl_aabb_mtv_iterator_init(it, other->lvl, other->aabb, other->origin_chunk_index);
}

inline static void lvl_aabb_mtv_iterator_init_from_entity_and_offset(struct lvl_aabb_mtv_iterator* it, struct lvl* lvl, struct lvl_entity* e, union vec3 offset)
{
	// TODO must consider that offset may push entity into another chunk;
	// should use the same logic as entities crossing portals
	struct aabb aabb = lvl_entity_aabb(e);
	aabb.center = vec3_add(aabb.center, offset);
	lvl_aabb_mtv_iterator_init(it, lvl, aabb, e->chunk_index);
}

inline static int lvl_aabb_mtv_iterator_next(struct lvl_aabb_mtv_iterator* it)
{
	struct lvl_chunk* chunk = lvl_get_chunk(it->lvl, it->origin_chunk_index); // FIXME aabb may intersect portals into other chunks

	AN(chunk);
	AN(chunk->polygon_list);

	while (1) {
		uint32_t vertex_count = chunk->polygon_list[it->polygon_list_cursor++];
		if (vertex_count == 0) break;

		it->material_index = chunk->polygon_list[it->polygon_list_cursor++];

		ASSERT(vertex_count <= 32);

		union vec3 polygon[32];
		for (int i = 0; i < vertex_count; i++) {
			struct lvl_vertex lv = chunk->vertices[chunk->polygon_list[it->polygon_list_cursor++]];
			polygon[i] = lv.co;
		}

		if (polygon_aabb_mtv(it->aabb, polygon, vertex_count, &it->mtv)) return 1;
	}

	// TODO check against portals (need a smallish stack?)

	return 0;
}

inline static int dot_is_ground(float dot)
{
	return dot < -0.707; // cos(45deg) ~= 0.707
}

static void lvl_entity_clipmove(struct lvl* lvl, struct lvl_entity* e, union vec3 r)
{
	float rlensqr = vec3_dot(r, r);
	if (rlensqr < 1e-5) return;

	int n_steps = 8; // TODO determine based on length of r?
	union vec3 rstep = vec3_scale(r, 1.0 / (float)n_steps);

	float max_step_up = lvl_entity_max_step_up(e);

	for (int i = 0; i < n_steps; i++) {
		e->position = vec3_add(e->position, rstep);
		struct lvl_aabb_mtv_iterator it;
		lvl_aabb_mtv_iterator_init_from_entity(&it, lvl, e);
		while (lvl_aabb_mtv_iterator_next(&it)) {
			float rmtv = vec3_length(it.mtv);
			if (rmtv > 1e-8) {
				union vec3 mtv_normalized = vec3_normalize(it.mtv);
				float cn = vec3_dot(lvl->gravity_normalized, mtv_normalized);

				int stepped_up = 0;

				if (fabsf(cn) < 0.174f) { // ~80deg
					const int N = 6;
					union vec3 nudge = vec3_scale(vec3_normalize(r), (max_step_up / (float)(1 << (N-1))));
					union vec3 s = vec3_normalize(vec3_cross(vec3_cross(lvl->gravity_normalized, it.mtv), it.mtv));
					float t = 0.5f;
					float tinc = 0.25f;
					for (int i = 0; i < N; i++) {
						struct lvl_aabb_mtv_iterator it2;
						lvl_aabb_mtv_iterator_init_from_other_iterator(&it2, &it);
						union vec3 step_up_probe = vec3_add(vec3_scale(s, max_step_up * t), nudge);
						it2.aabb.center = vec3_add(it2.aabb.center, step_up_probe);
						union vec3 best_mtv = {{0,0,0}};
						float best_mtv_sqrlen = 0.0f;
						while (lvl_aabb_mtv_iterator_next(&it2)) {
							float mtv_sqrlen = vec3_dot(it2.mtv, it2.mtv);
							if (mtv_sqrlen > best_mtv_sqrlen) {
								best_mtv = it2.mtv;
								best_mtv_sqrlen = mtv_sqrlen;
							}
						}

						if (best_mtv_sqrlen > 0) {
							float ground_dot = vec3_dot(vec3_normalize(best_mtv), lvl->gravity_normalized);
							if (dot_is_ground(ground_dot)) {
								union vec3 step_up_offset = vec3_add(step_up_probe, best_mtv);
								e->position = vec3_add(e->position, step_up_offset);
								stepped_up = 1;
								break;
							} else {
								t += tinc;
							}
						} else {
							t -= tinc;
						}

						tinc *= 0.5f;
					}
				}

				// normal resolution if not handling a step
				if (!stepped_up) {
					union vec3 normal = vec3_scale(it.mtv, 1.0 / rmtv);
					e->position = vec3_add(e->position, it.mtv);
					e->velocity = vec3_sub(e->velocity, vec3_scale(normal, vec3_dot(e->velocity, normal)));
				} else {
					break;
				}
			}
		}
	}
}

void lvl_entity_update(struct lvl* lvl, struct lvl_entity* e, float dt)
{
	// ground check
	union vec3 ground_offset = vec3_scale(lvl->gravity_normalized, 3e-3);
	struct lvl_aabb_mtv_iterator it;
	lvl_aabb_mtv_iterator_init_from_entity_and_offset(&it, lvl, e, ground_offset);
	union vec3 dominant_ground_mtv = {{0,0,0}};
	float dominant_ground_mtv_sqrlen = 0;
	while (lvl_aabb_mtv_iterator_next(&it)) {
		float sqrlen = vec3_dot(it.mtv, it.mtv);
		if (sqrlen > dominant_ground_mtv_sqrlen) {
			dominant_ground_mtv_sqrlen = sqrlen;
			dominant_ground_mtv = it.mtv;
		}
	}

	e->grounded = 0;
	union vec3 dominant_ground_mtv_direction = {{0,0,0}};
	float ground_dot = 0;
	if (dominant_ground_mtv_sqrlen > 0) {
		dominant_ground_mtv_direction = vec3_normalize(dominant_ground_mtv);
		ground_dot = vec3_dot(lvl->gravity_normalized, dominant_ground_mtv_direction);
		if (dot_is_ground(ground_dot)) {
			e->grounded = 1;
			e->velocity = vec3_scale(e->velocity, powf(5e-04, dt));
		}
	}

	// gravity acceleration
	if (!e->grounded) lvl_entity_accelerate(e, lvl->gravity, dt);

	// movement acceleration
	if (e->grounded && dominant_ground_mtv_sqrlen > 0) {
		// ground acceleration
		if (e->move_forward != 0 || e->move_right != 0) {
			union vec3 a = vec3_move(e->yaw, 0, e->move_forward, e->move_right);
			a = vec3_normalize(vec3_cross(vec3_cross(dominant_ground_mtv, a), dominant_ground_mtv));
			float bf = 50;
			float f = bf + vec3_dot(a, lvl->gravity_normalized) * bf;
			lvl_entity_accelerate(e, vec3_scale(a, f), dt);
		}
	} else {
		// air acceleration
		lvl_entity_accelerate(
			e,
			vec3_scale(vec3_move(e->yaw, 0, e->move_forward, e->move_right), 2),
			dt);
	}

	// allow wall jumps so long as angle is less than ~80deg
	if (ground_dot < -0.17) {
		// TODO "jump potential energy"? like something that
		// replenishes? to make quick consequtive jump actions have
		// less effect? try it out!
		lvl_entity_impulse(e, vec3_scale(dominant_ground_mtv_direction, e->move_jump * 6));
	}

	e->move_forward = 0;
	e->move_right = 0;
	e->move_jump = 0;

	lvl_entity_clipmove(lvl, e, vec3_scale(e->velocity, dt));
}

struct mat44 lvl_entity_view(struct lvl_entity* e)
{
	struct mat44 m = mat44_identity();
	m = mat44_rotate_x(m, e->pitch);
	m = mat44_rotate_y(m, e->yaw);
	m = mat44_translate(m, vec3_scale(e->position, -1));
	return m;
}
