local mt = {}; mt.__index = mt

local function new ()
	local lvl = {
		chunks = {},
		portals = {}
	}
	return setmetatable(lvl, mt)
end

mt.insert_lump = function (self, lump)
	table.insert(self.chunks, lump)
	return chunk
end

return new
