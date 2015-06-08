import bpy
from bpy_extras import mesh_utils
from mathutils import Vector, Matrix
import sys
import os
import math

# relative imports
sys.path.append(os.path.dirname(__file__))
import lson

src = bpy.data.filepath
dst = sys.argv[sys.argv.index('--') + 1]

print("exporting lump %s => %s ..." % (src, dst))

lump = {"polygons": [], "dummies": []}

def get_custom_properties(bo):
	props = {}
	for k in bo.keys():
		if k[0] == "_": continue
		props[k] = bo[k]
	return props

def flatten_matrix(m):
	s = []
	for v in list(m):
		s += list(v)
	return s

opengl_tx = Matrix.Rotation(-math.pi/2, 4, "X")

for bo in bpy.data.objects:
	if bo.type == "MESH":
		mesh = bo.to_mesh(bpy.context.scene, True, "PREVIEW")
		for polygon in mesh.polygons:
			lump_polygon = {"vs":[]}
			for i in range(polygon.loop_start, polygon.loop_start + polygon.loop_total):
				co = opengl_tx * bo.matrix_world * mesh.vertices[mesh.loops[i].vertex_index].co
				uv = Vector((0,0))
				uv_layer = mesh.uv_layers.active
				if uv_layer:
					uv = uv_layer.data[i].uv
				lump_polygon["vs"].append({"co": tuple(co), "uv": tuple(uv)})
			try:
				lump_polygon["mt"] = mesh.materials[polygon.material_index].name
			except IndexError:
				lump_polygon["mt"] = "null"

			assert(len(lump_polygon["vs"]) >= 3)

			vs = [Vector(v['co']) for v in lump_polygon['vs']]
			e0 = vs[1] - vs[0]
			e1 = vs[1] - vs[2]
			bnormal = opengl_tx * polygon.normal
			onormal = e1.cross(e0).normalized()
			if bnormal.dot(onormal) < 0:
				lump_polygon['vs'].reverse()
				print("WARNING had to reverse vs!")

			lump["polygons"].append(lump_polygon)
	elif bo.type == "EMPTY":
		dummy = {}
		dummy["props"] = get_custom_properties(bo)
		dummy["tx"] = flatten_matrix(bo.matrix_world)
		lump["dummies"].append(dummy)

output = lson.dumps(lump)
with open(dst, "wb") as f: f.write(lson.dumps(lump).encode('ascii'))

