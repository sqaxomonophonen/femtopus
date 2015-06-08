#ifndef MAT_H

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "a.h"

#ifndef M_PI
#define M_PI (3.141592653589793)
#endif




#define DEG2RAD(x) ((x)/180.0f*M_PI)
#define I2RAD(x) ((x)*M_PI*2.0f)



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

union vec4 {
	struct { float x; float y; float z; float w; };
	struct { float r; float g; float b; float a; };
	float s[4];
};

struct mat33 {
	float s[3*3];
};

struct mat44 {
	float s[4*4];
};

struct aabb {
	union vec3 center;
	union vec3 extent;
};



////////////////////////////////////////////////////
// vec2_*

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



////////////////////////////////////////////////////
// vec3_*

inline static union vec3 vec3_xyz(float x, float y, float z)
{
	union vec3 v = {{x,y,z}};
	return v;
}

inline static void vec3_dump(union vec3 v)
{
	printf("[");
	for (int i = 0; i < 3; i++) printf("%.4f%s", v.s[i], i < 2 ? ", " : "");
	printf("]\n");
}

inline static union vec3 vec3_add(union vec3 a, union vec3 b)
{
	union vec3 r;
	for (int i = 0; i < 3; i++) {
		r.s[i] = a.s[i] + b.s[i];
	}
	return r;
}


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

inline static union vec3 vec3_move(float yaw, float pitch, float forward, float right)
{
	union vec3 move;

	float yaw_s = sinf(DEG2RAD(yaw));
	float yaw_c = cosf(DEG2RAD(yaw));

	float pitch_s = sinf(DEG2RAD(pitch));
	float pitch_c = cosf(DEG2RAD(pitch));

	union vec3 fv = {{ yaw_s * pitch_c, -pitch_s, -yaw_c * pitch_c }};
	move = vec3_scale(fv, forward);

	union vec3 rv = {{ yaw_c, 0, yaw_s, }};
	move = vec3_add(move, vec3_scale(rv, right));

	return move;
}


////////////////////////////////////////////////////
// vec4_*

inline static float vec4_dot(union vec4 a, union vec4 b)
{
	float dot = 0;
	for (int i = 0; i < 4; i++) dot += a.s[i] * b.s[i];
	return dot;
}



////////////////////////////////////////////////////
// mat44_*

inline static int mat44_ati(int col, int row)
{
	ASSERT(col >= 0 && col < 4 && row >= 0 && row < 4);
	return row + col * 4;
}

inline static float mat44_at(struct mat44 m, int col, int row)
{
	return m.s[mat44_ati(col, row)];
}

inline static float* mat44_atp(struct mat44* m, int col, int row)
{
	return &m->s[mat44_ati(col, row)];
}

inline static union vec4 mat44_get_row(struct mat44 m, int row)
{
	union vec4 v;
	for (int col = 0; col < 4; col++) {
		v.s[col] = mat44_at(m, col, row);
	}
	return v;
}

inline static union vec4 mat44_get_col(struct mat44 m, int col)
{
	union vec4 v;
	for (int row = 0; row < 4; row++) {
		v.s[row] = mat44_at(m, col, row);
	}
	return v;
}

inline static struct mat44 mat44_multiply(struct mat44 a, struct mat44 b)
{
	struct mat44 m;
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			*(mat44_atp(&m, col, row)) = vec4_dot(
				mat44_get_row(a, row),
				mat44_get_col(b, col)
			);
		}
	}
	return m;
}

inline static struct mat44 mat44_zero()
{
	struct mat44 m;
	memset(&m, 0, sizeof(m));
	return m;
}

inline static struct mat44 mat44_identity()
{
	struct mat44 m;
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			*(mat44_atp(&m, col, row)) = col == row ? 1 : 0;
		}
	}
	return m;
}

inline static struct mat44 mat44_translation(union vec3 delta)
{
	struct mat44 m = mat44_identity();
	for (int row = 0; row < 3; row++) {
		*(mat44_atp(&m, 3, row)) = delta.s[row];
	}
	return m;
}

inline static struct mat44 mat44_translate(struct mat44 m, union vec3 d)
{
	return mat44_multiply(m, mat44_translation(d));
}

inline static struct mat44 mat44_rotation(float angle, union vec3 axis)
{
	struct mat44 m = mat44_identity();

	float c = cosf(DEG2RAD(angle));
	float s = sinf(DEG2RAD(angle));
	float c1 = 1 - c;

	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++) {
			float value = axis.s[col] * axis.s[row] * c1;
			if (col == row) {
				value += c;
			} else {
				int i = 3 - row - col;
				ASSERT(i >= 0 && i < 3);
				float sgn1 = ((col+row)&1) ? 1 : -1;
				float sgn2 = row>col ? 1 : -1;
				value += sgn1 * sgn2 * axis.s[i] * s;

			}
			*(mat44_atp(&m, col, row)) = value;
		}
	}

	return m;
}

inline static struct mat44 mat44_rotate(struct mat44 m, float angle, union vec3 axis)
{
	return mat44_multiply(m, mat44_rotation(angle, axis));
}

inline static struct mat44 mat44_rotate_x(struct mat44 m, float angle)
{
	union vec3 axis = {{1,0,0}};
	return mat44_rotate(m, angle, axis);
}

inline static struct mat44 mat44_rotate_y(struct mat44 m, float angle)
{
	union vec3 axis = {{0,1,0}};
	return mat44_rotate(m, angle, axis);
}

inline static struct mat44 mat44_rotate_z(struct mat44 m, float angle)
{
	union vec3 axis = {{0,0,1}};
	return mat44_rotate(m, angle, axis);
}

inline static struct mat44 mat44_frustum(float left, float right, float bottom, float top, float znear, float zfar)
{
	// http://docs.gl/gl2/glFrustum

	struct mat44 r = mat44_zero();

	*(mat44_atp(&r,0,0)) = 2 * znear / (right - left);
	*(mat44_atp(&r,1,1)) = 2 * znear / (top - bottom);

	*(mat44_atp(&r,2,0)) = (right+left)/(right-left); // A
	*(mat44_atp(&r,2,1)) = (top+bottom)/(top-bottom); // B
	*(mat44_atp(&r,2,2)) = -((zfar+znear)/(zfar-znear)); // C
	*(mat44_atp(&r,2,3)) = -1;

	*(mat44_atp(&r,3,2)) = -(2 * zfar * znear / (zfar - znear)); // D

	return r;
}


inline static struct mat44 mat44_perspective(float fovy, float aspect, float znear, float zfar)
{
	float dy = znear * tanf(DEG2RAD(fovy)/2);
	float dx = dy * aspect;
	return mat44_frustum(-dx, dx, -dy, dy, znear, zfar);
}



////////////////////////////////////////////////////
// other

/*
(x,y,z) x (1,0,0) = ( 0  z, -y)
(x,y,z) x (0,1,0) = (-z, 0,  x)
(x,y,z) x (0,0,1) = (y, -x,  0)
*/


// find minimal translation vector (mtv) using separating axis test for convex
// polygon vs aabb.
// returns 0 if no intersection
// returns 1 if intersection and *mtv will be populated with minimal translation vector
inline static int polygon_aabb_mtv(
	struct aabb aabb,
	union vec3* polygon,
	int polygon_n,
	union vec3* mtv)
{
	ASSERT(polygon_n >= 3);

	float best_distance = 1e10f;
	union vec3 best_axis;
	int ret = 0;

	// perform SAT using edge cross products as separating axis
	int pi_prev = polygon_n - 1;
	for (int pi = 0; pi < polygon_n; pi++) {
		union vec3 polygon_edge = vec3_sub(polygon[pi], polygon[pi_prev]);
		int ai_prev = 2;
		for (int ai = 0; ai < 3; ai++) {
			// find separating axis in 2d
			union vec2 x = {{
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
				union vec2 v = {{
					polygon[i].s[ai] - aabb.center.s[ai],
					polygon[i].s[ai_prev] - aabb.center.s[ai_prev]
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
			float e = aabb.extent.s[ai] * fabsf(x.s[0]) + aabb.extent.s[ai_prev] * fabsf(x.s[1]);

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
				union vec3 axis = {{0,0,0}};
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
			float v = polygon[i].s[ai] - aabb.center.s[ai];
			if (i == 0) {
				min = max = v;
			} else if (v < min) {
				min = v;
			} else if (v > max) {
				max = v;
			}
		}
		float e = aabb.extent.s[ai];

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
			union vec3 axis = {{0,0,0}};
			axis.s[ai] = 1;
			best_axis = axis;
			ret = 1;
		}
	}

	// perform SAT using polygon face normal as separating axis
	{
		union vec3 e0 = vec3_sub(polygon[0], polygon[1]);
		union vec3 e1 = vec3_sub(polygon[2], polygon[1]);
		union vec3 x = vec3_normalize(vec3_cross(e0, e1));
		float d = vec3_dot(x, vec3_sub(polygon[0], aabb.center));
		float e = 0;
		for (int i = 0; i < 3; i++) e += aabb.extent.s[i] * fabsf(x.s[i]);
		if (fabs(d) > e) {
			return 0;
		} else {
			// XXX there's probably value in always pushing along
			// the face normal, but then I'd have to make sure the
			// normal faces the right direction.
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
