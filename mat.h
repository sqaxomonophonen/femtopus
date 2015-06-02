#ifndef MAT_H

#include <math.h>

#include "a.h"

struct vec2 {
	float s[2];
};

static float vec2_dot(struct vec2 a, struct vec2 b)
{
	float sum = 0;
	for (int i = 0; i < 2; i++) sum += a.s[i] * b.s[i];
	return sum;
}

#if 0
static float vec2_length(struct vec2 v)
{
	return sqrtf(vec2_dot(v, v));
}
#endif

static struct vec2 vec2_scale(struct vec2 v, float scalar)
{
	for (int i = 0; i < 2; i++) v.s[i] *= scalar;
	return v;
}

#if 0
static struct vec2 vec2_normalize(struct vec2 v)
{
	return vec2_scale(v, 1.0f / vec2_length(v));
}
#endif

struct vec3 {
	float s[3];
};

static struct vec3 vec3_sub(struct vec3 a, struct vec3 b)
{
	struct vec3 r;
	for (int i = 0; i < 3; i++) {
		r.s[i] = a.s[i] - b.s[i];
	}
	return r;
}

static struct vec3 vec3_scale(struct vec3 v, float scalar)
{
	for (int i = 0; i < 3; i++) v.s[i] *= scalar;
	return v;
}

static struct vec3 vec3_cross(struct vec3 a, struct vec3 b)
{
	struct vec3 r = {{
		a.s[1]*b.s[2] - a.s[2]*b.s[1],
		a.s[2]*b.s[0] - a.s[0]*b.s[2],
		a.s[0]*b.s[1] - a.s[1]*b.s[0]
	}};
	return r;
}

static float vec3_dot(struct vec3 a, struct vec3 b)
{
	float sum = 0;
	for (int i = 0; i < 3; i++) sum += a.s[i] * b.s[i];
	return sum;
}

static float vec3_length(struct vec3 v)
{
	return sqrtf(vec3_dot(v, v));
}

static struct vec3 vec3_normalize(struct vec3 v)
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
static int polygon_aabb_mtv(
	struct vec3 aabb_center, struct vec3 aabb_extent,
	struct vec3* polygon,
	int polygon_n,
	struct vec3* mtv)
{
	ASSERT(polygon_n >= 3);

	float best_distance = 1e10f;
	struct vec3 best_axis;
	int ret = 0;

	// perform SAT using edge cross products as separating axis
	int pi_prev = polygon_n - 1;
	for (int pi = 0; pi < polygon_n; pi++) {
		struct vec3 polygon_edge = vec3_sub(polygon[pi], polygon[pi_prev]);
		int ai_prev = 2;
		for (int ai = 0; ai < 3; ai++) {
			// find separating axis in 2d
			struct vec2 x = {{
				-polygon_edge.s[ai_prev],
				polygon_edge.s[ai]
			}};
			float xlsqr = vec2_dot(x, x);
			if (xlsqr == 0.0f) continue;
			x = vec2_scale(x, 1.0f / sqrtf(xlsqr));

			// project polygon onto axis; find min/max interval
			float min = 0;
			float max = 0;
			for (int i = 0; i < polygon_n; i++) {
				struct vec2 v = {{
					polygon[i].s[ai] - aabb_center.s[ai],
					polygon[i].s[ai_prev] - aabb_center.s[ai_prev]
				}};
				float d = vec2_dot(x, v);
				if (i == 0) {
					min = max = d;
				} else if (d < min) {
					min = d;
				} else if (d > max) {
					max = d;
				}
			}

			// project aabb onto axis
			float e = aabb_extent.s[ai] * fabsf(x.s[ai]) + aabb_extent.s[ai_prev] * fabsf(x.s[ai_prev]);

			float ld = 0;
			if (min > e || max < -e) {
				// no overlap
				return 0;
			} else if (min < -e) {
				ld = -max - e;
			} else {
				ld = e - min;
			}

			// overlap found; check if it's better than previous
			// results
			if (fabsf(ld) < fabsf(best_distance)) {
				best_distance = ld;
				struct vec3 axis = {{0,0,0}};
				axis.s[ai] = x.s[0];
				axis.s[ai_prev] = x.s[1];
				best_axis = axis;
				ret = 1;
			}

			ai_prev = ai;
		}
		pi_prev = pi;
	}

	// perform SAT using AABB face normals as separating axis
	for (int ai = 0; ai < 3; ai++) {
		float min = 0;
		float max = 0;
		for (int i = 0; i < polygon_n; i++) {
			float v = polygon[i].s[ai] - aabb_center.s[ai];
			if (i == 0) {
				min = max = v;
			} else if (v < min) {
				min = v;
			} else if (v > max) {
				max = v;
			}
		}
		float e = aabb_extent.s[ai];

		float ld = 0;
		if (min > e || max < -e) {
			// no overlap
			return 0;
		} else if (min < -e) {
			ld = -max - e;
		} else {
			ld = e - min;
		}

		// overlap found; check if it's better than previous results
		if (fabsf(ld) < fabsf(best_distance)) {
			best_distance = ld;
			struct vec3 axis = {{0,0,0}};
			axis.s[ai] = 1;
			best_axis = axis;
			ret = 1;
		}
	}

	// perform SAT using polygon face normal as separating axis
	{
		struct vec3 e0 = vec3_sub(polygon[0], polygon[1]);
		struct vec3 e1 = vec3_sub(polygon[2], polygon[1]);
		struct vec3 x = vec3_normalize(vec3_cross(e0, e1));
		float d = vec3_dot(x, vec3_sub(polygon[0], aabb_center));
		float e = 0;
		for (int i = 0; i < 3; i++) e += aabb_extent.s[i] * fabsf(x.s[i]);
		if (fabs(d) > e) {
			return 0;
		} else {
			float ld = (d>0 ? e : -e) - d;
			// overlap found; check if it's better than previous results
			if (fabsf(ld) < fabsf(best_distance)) {
				best_distance = ld;
				best_axis = x;
				ret = 1;
			}
		}
	}

	if (mtv) *mtv = vec3_scale(best_axis, -best_distance);
	return ret;
}


#define MAT_H
#endif
