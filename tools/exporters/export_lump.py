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

lump = {"polygons": []}

for bo in bpy.data.objects:
	if bo.type == "MESH":
		mesh = bo.to_mesh(bpy.context.scene, True, "PREVIEW")
		for polygon in mesh.polygons:
			lump_polygon = {"vs":[]}
			for i in range(polygon.loop_start, polygon.loop_start + polygon.loop_total):
				co = bo.matrix_world * mesh.vertices[mesh.loops[i].vertex_index].co
				uv = Vector((0,0))
				uv_layer = mesh.uv_layers.active
				if uv_layer:
					uv = uv_layer.data[i].uv
				lump_polygon["vs"].append({"co": tuple(co), "uv": tuple(uv)})
			try:
				lump_polygon["mt"] = mesh.materials[polygon.material_index].name
			except IndexError:
				lump_polygon["mt"] = "null"
			lump["polygons"].append(lump_polygon)

output = lson.dumps(lump)
with open(dst, "wb") as f: f.write(lson.dumps(lump).encode('ascii'))

