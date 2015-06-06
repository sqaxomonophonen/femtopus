return function (lvl)
	local clvl = {chunks = {}, portals = {}, materials = {}}
	local matmap = {}
	for _,chunk in ipairs(lvl.chunks) do
		local compiled_chunk = {vertices = {}, polygon_list = {}, portal_indices = {}}
		for _,p in ipairs(chunk.polygons) do
			if not matmap[p.mt] then
				table.insert(clvl.materials, {name = p.mt})
				matmap[p.mt] = #clvl.materials
			end

			table.insert(compiled_chunk.polygon_list, #p.vs)
			table.insert(compiled_chunk.polygon_list, p.mt)
			for _,v in ipairs(p.vs) do
				table.insert(compiled_chunk.vertices, v)
				table.insert(compiled_chunk.polygon_list, #compiled_chunk.vertices - 1)
			end
		end
		table.insert(compiled_chunk.polygon_list, 0)
		table.insert(clvl.chunks, compiled_chunk)
	end

	return clvl
end

