#include <string.h>

#include "platform.h"
#include "render.h"

void render_init(struct render* render, SDL_Window* window)
{
	AN(render);
	AN(window);

	memset(render, 0, sizeof(*render));
	render->window = window;

	vtxbuf_init(&render->vtxbuf, 1<<18);

	{
		#include "nullmat.glsl.inc"
		struct shader_attr_spec specs[] = {
			{"a_position", SHADER_ATTR_VEC3},
			{"a_normal", SHADER_ATTR_VEC3},
			{"a_uv", SHADER_ATTR_VEC2},
			{NULL}
		};
		shader_init(
			&render->nullmat_shader,
			nullmat_vert_src,
			nullmat_frag_src,
			specs);
	}
}

void render_lvl(struct render* render, struct lvl* lvl, struct lvl_entity* entity)
{
	AN(render);
	AN(lvl);
	AN(entity);

	if (entity->grounded) {
		glClearColor(1,1,0,1);
	} else {
		glClearColor(1,0,1,1);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	int window_width;
	int window_height;
	SDL_GetWindowSize(render->window, &window_width, &window_height);

	glViewport(0, 0, window_width, window_height); CHKGL;

	float aspect = (float)window_width / (float)window_height;

	struct mat44 view = lvl_entity_view(entity);
	struct mat44 projection = mat44_perspective(65, aspect, 0.1, 409.6);

	vtxbuf_begin(&render->vtxbuf, &render->nullmat_shader, GL_TRIANGLES);
	shader_uniform_mat44(&render->nullmat_shader, "u_view", view);
	shader_uniform_mat44(&render->nullmat_shader, "u_projection", projection);

	struct lvl_chunk* chunk = lvl_get_chunk(lvl, 0);
	AN(chunk);
	AN(chunk->polygon_list);
	int cursor = 0;

	while (1) {
		int vertex_count = chunk->polygon_list[cursor++];
		if (vertex_count == 0) break;

		uint32_t material_index = chunk->polygon_list[cursor++];
		(void) material_index;

		struct lvl_vertex vs[3];
		vs[0] = chunk->vertices[chunk->polygon_list[cursor++]];

		int tri_count = vertex_count - 2;
		for (int i = 0; i < tri_count; i++) {
			vs[1] = chunk->vertices[chunk->polygon_list[cursor++]];
			vs[2] = chunk->vertices[chunk->polygon_list[cursor]];

			// FIXME normal should not change, so don't calculate
			// it more than once
			union vec3 normal = vec3_normalize(vec3_cross(vec3_sub(vs[1].co, vs[0].co), vec3_sub(vs[1].co, vs[2].co)));

			float triangle[8*3];
			int ti = 0;
			for (int j = 0; j < 3; j++) {
				for (int k = 0; k < 3; k++) triangle[ti++] = vs[j].co.s[k];
				for (int k = 0; k < 3; k++) triangle[ti++] = normal.s[k];
				for (int k = 0; k < 2; k++) triangle[ti++] = vs[j].uv.s[k];
			}
			vtxbuf_element(&render->vtxbuf, triangle, sizeof(triangle));
		}

		cursor++;
	}

	vtxbuf_end(&render->vtxbuf);

}

void render_flip(struct render* render)
{
	AN(render);
	SDL_GL_SwapWindow(render->window);
}

