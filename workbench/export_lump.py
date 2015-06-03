import bpy
from bpy_extras import mesh_utils
from mathutils import Vector, Matrix
import sys
import os
import struct
import math

src = bpy.data.filepath
dst = sys.argv[sys.argv.index('--') + 1]

print("exporting lump %s => %s ..." % (src, dst))

class Lump:
	def __init__(self):
		self.polygons = []

	def add_polygon(self, polygon):
		assert(polygon.is_valid())
		self.polygons.append(polygon)

class LumpPolygon:
	def __init__(self):
		self.vertices = []

	def is_valid(self):
		return len(self.vertices) >= 3

	def add_vertex(self, co, uv):
		self.vertices.append((co, uv))

	def set_material(self, material):
		self.material = material
		print(material)

lump = Lump()
for bo in bpy.data.objects:
	if bo.type == "MESH":
		mesh = bo.to_mesh(bpy.context.scene, True, "PREVIEW")
		print()
		for polygon in mesh.polygons:
			lump_polygon = LumpPolygon()
			for i in range(polygon.loop_start, polygon.loop_start + polygon.loop_total):
				co = bo.matrix_world * mesh.vertices[mesh.loops[i].vertex_index].co
				uv = Vector((0,0))
				uv_layer = mesh.uv_layers.active
				if uv_layer:
					uv = uv_layer.data[i].uv
				lump_polygon.add_vertex(co, uv)
			lump_polygon.set_material(mesh.materials[polygon.material_index])
			lump.add_polygon(lump_polygon)

with open(dst, "wb") as f:
	f.write(b"")
