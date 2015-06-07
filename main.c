#include <stdio.h>

#include <SDL.h>

#include "llvl.h"
#include "render.h"
#include "a.h"

static void gldbg(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* usr)
{
	printf("<gldbg> %d %d %d %d %s\n", source, type, id, severity, message);
}

int main(int argc, char** argv)
{
	int enable_opengl_debug = 0;

	SAZ(SDL_Init(SDL_INIT_EVERYTHING));
	atexit(SDL_Quit);

	int bitmask = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL;
	SDL_Window* window = SDL_CreateWindow(
			"femtopus",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			0, 0,
			bitmask);
	SAN(window);

	if (enable_opengl_debug) SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	SDL_GLContext glctx = SDL_GL_CreateContext(window);
	SAN(glctx);

	if (enable_opengl_debug) {
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(gldbg, NULL);
	}

	#ifdef BUILD_MINGW32
	glewInit();
	#endif

	SAZ(SDL_GL_SetSwapInterval(1)); // or -1, "late swap tearing"?

	struct render render;
	render_init(&render, window);

	struct lvl lvl;
	llvl_build("thing", &lvl);

	int ctrl_forward = 0;
	int ctrl_backward = 0;
	int ctrl_left = 0;
	int ctrl_right = 0;

	SDL_SetRelativeMouseMode(SDL_TRUE);

	struct lvl_entity view_entity;
	memset(&view_entity, 0, sizeof(view_entity));

	int exiting = 0;
	while (!exiting) {
		int mdx = 0;
		int mdy = 0;

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				exiting = 1;
			}

			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					exiting = 1;
				}
			}

			struct push_key {
				SDL_Keycode sym;
				int* intptr;
			} push_keys[] = {
				{SDLK_w, &ctrl_forward},
				{SDLK_s, &ctrl_backward},
				{SDLK_a, &ctrl_left},
				{SDLK_d, &ctrl_right},
				{-1, NULL}
			};

			for (struct push_key* tkp = push_keys; tkp->intptr != NULL; tkp++) {
				if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && e.key.keysym.sym == tkp->sym) {
					*(tkp->intptr) = (e.type == SDL_KEYDOWN);
				}
			}

			if (e.type == SDL_MOUSEMOTION) {
				mdx += e.motion.xrel;
				mdy += e.motion.yrel;
			}
		}

		{
			float sensitivity = 0.1f;
			float dyaw = (float)mdx * sensitivity;
			float dpitch = (float)mdy * sensitivity;
			lvl_entity_dlook(&view_entity, dyaw, dpitch);
		}

		{
			float speed = 0.1f;
			float forward = (float)(ctrl_forward - ctrl_backward) * speed;
			float right = (float)(ctrl_right - ctrl_left) * speed;
			lvl_entity_flymove(&lvl, &view_entity, forward, right);
			//lvl_entity_clipmove(&lvl, &view_entity, forward, right);
		}

		render_lvl(&render, &lvl, &view_entity);
		render_flip(&render);
	}

	lvl_free(&lvl);

	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(glctx);

	return EXIT_SUCCESS;
}
