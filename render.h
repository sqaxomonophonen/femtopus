#ifndef RENDER_H

#include <SDL.h>

#include "lvl.h"
#include "vtxbuf.h"
#include "shader.h"

struct render {
	SDL_Window* window;

	struct vtxbuf vtxbuf;
	struct shader nullmat_shader;
};

void render_init(struct render* render, SDL_Window* window);
void render_lvl(struct render* render, struct lvl* lvl, struct lvl_entity* entity);
void render_flip(struct render* render);

#define RENDER_H
#endif
