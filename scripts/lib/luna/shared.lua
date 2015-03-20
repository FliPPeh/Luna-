--[[
-- Wrap up __luna.shared.get and __luna.shared.set inside a nice and intuitive
-- table.
--]]
local __shared = luna.shared

function luna.save_shared()
    return __shared.save()
end

function luna.reload_shared()
    return __shared.reload()
end

luna.shared = setmetatable({}, {
    __index = function(tab, key)
        return __shared.get(key)
    end,

    __newindex = function(tab, key, val)
        if val then
            __shared.set(key, val)
        else
            __shared.clear(key)
        end
    end,

    __pairs = function(tab)
        return pairs(__shared.list())
    end
})
