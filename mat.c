#include "mat.h"
#include "a.h"

int polygon_aabb_mtv(
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

union vec3 vec3_move(float yaw, float pitch, float forward, float right)
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

