#ifndef PLATFORM_H

#if BUILD_LINUX
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#elif BUILD_MINGW32
#include <GL/glew.h>
#elif BUILD_OSX
#include <gl.h>
#endif

#ifndef BUILD_MINGW32
#include <alloca.h>
#endif

#define PLATFORM_H
#endif
