bl_info = {
	"name": "Link from Matbrowse",
	"category": "Material"
}

import bpy
import http.client
import json

class LinkFroMatbrowse(bpy.types.Operator):
	bl_idname = "material.link_from_matbrowse"
	bl_label = "Link material from Matbrowse"
	bl_options = {'REGISTER', 'UNDO'}

	def error(self, msg):
		self.report({'ERROR'}, msg)
		return {'CANCELLED'}

	# path can be relative or absolute; blender resolves it to an relative
	# path in any case
	def import_material(self, context, path, name):
		with bpy.data.libraries.load(path, link = True, relative = True) as (data_from, data_to):
			if name not in data_from.materials:
				self.error("fuck! expected to find material '%s' in %s" % (name, path))
			data_to.materials += [name]
		return [m for m in context.blend_data.materials if m.name == name][0]

	def execute(self, context):
		try:
			host = 'localhost:6510'
			http_client = http.client.HTTPConnection(host)
			http_client.request('GET', '/api/get_selection')
			http_response = http_client.getresponse()
			if http_response.status != 200:
				return self.error("expected response status 200, got %s" % http_response.status)
			response = json.loads(http_response.read().decode('utf-8'))
		except ConnectionRefusedError as e:
			return self.error("connection refused @ %s; is matbrowse running?" % host)

		o = context.active_object

		if o.mode != 'EDIT' or o.type != 'MESH':
			return self.error("must be in mesh edit mode")

		# have to leave edit mode to be able to commit changes
		bpy.ops.object.mode_set()

		mesh = o.data
		o.update_from_editmode()

		selected_polygons = [p for p in mesh.polygons if p.select]
		if len(selected_polygons) > 0:
			#material = self.import_material(context, '/home/kaare/priv/git/femtopus/workbench/materials/m0.blend', 'tex')
			material = self.import_material(context, response['path'], response['name'])

			if len([mm for mm in mesh.materials if mm == material]) == 0:
				mesh.materials.append(material)

			mi = mesh.materials.find(material.name)
			for p in selected_polygons:
				p.material_index = mi

			mesh.update()

		# "commit"
		bpy.ops.object.mode_set(mode='EDIT')

		return {'FINISHED'}

def register():
	bpy.utils.register_class(LinkFroMatbrowse)

def unregister():
	bpy.utils.unregister_class(LinkFroMatbrowse)

