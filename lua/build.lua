
local lump_table = {}
function lump_load(name)
	if not lump_table[name] then
		lump_table[name] = dofile("data/lumps/" .. name .. ".lump.lua")
	end
	return lump_table[name]
end

--[[
function plan_load(name)
	return require("plans/" .. name)()
end
]]

return function (plan_name)
	--local plan = plan_load(plan_name)
	--print("hello, " .. plan)
	--print(lump_load("stress"))

	local lump = lump_load("t0")
	local lvl = require('lvl')()

	lvl:insert_lump(lump)

	return require('compile')(lvl)
end

