#ifndef MAT_H

#include <math.h>

union vec2 {
	struct { float x; float y; };
	struct { float u; float v; };
	float s[2];
};

union vec3 {
	struct { float x; float y; float z; };
	struct { float u; float v; float w; };
	struct { float r; float g; float b; };
	float s[3];
};

inline static float vec2_dot(union vec2 a, union vec2 b)
{
	float sum = 0;
	for (int i = 0; i < 2; i++) sum += a.s[i] * b.s[i];
	return sum;
}

#if 0
static float vec2_length(union vec2 v)
{
	return sqrtf(vec2_dot(v, v));
}
#endif

inline static union vec2 vec2_scale(union vec2 v, float scalar)
{
	for (int i = 0; i < 2; i++) v.s[i] *= scalar;
	return v;
}

#if 0
inline static union vec2 vec2_normalize(union vec2 v)
{
	return vec2_scale(v, 1.0f / vec2_length(v));
}
#endif

inline static union vec3 vec3_sub(union vec3 a, union vec3 b)
{
	union vec3 r;
	for (int i = 0; i < 3; i++) {
		r.s[i] = a.s[i] - b.s[i];
	}
	return r;
}

inline static union vec3 vec3_scale(union vec3 v, float scalar)
{
	for (int i = 0; i < 3; i++) v.s[i] *= scalar;
	return v;
}

inline static union vec3 vec3_cross(union vec3 a, union vec3 b)
{
	union vec3 r = {{
		a.s[1]*b.s[2] - a.s[2]*b.s[1],
		a.s[2]*b.s[0] - a.s[0]*b.s[2],
		a.s[0]*b.s[1] - a.s[1]*b.s[0]
	}};
	return r;
}

inline static float vec3_dot(union vec3 a, union vec3 b)
{
	float sum = 0;
	for (int i = 0; i < 3; i++) sum += a.s[i] * b.s[i];
	return sum;
}

inline static float vec3_length(union vec3 v)
{
	return sqrtf(vec3_dot(v, v));
}

inline static union vec3 vec3_normalize(union vec3 v)
{
	return vec3_scale(v, 1.0f / vec3_length(v));
}

/*
(x,y,z) x (1,0,0) = ( 0  z, -y)
(x,y,z) x (0,1,0) = (-z, 0,  x)
(x,y,z) x (0,0,1) = (y, -x,  0)
*/


// find minimal translation vector (mtv) using separating axis test for convex
// polygon vs aabb.
// returns 0 if no intersection
// returns 1 if intersection and *mtv will be populated with minimal translation vector
int polygon_aabb_mtv(
	union vec3 aabb_center, union vec3 aabb_extent,
	union vec3* polygon,
	int polygon_n,
	union vec3* mtv);

#define MAT_H
#endif
