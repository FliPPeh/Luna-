--[[
-- Logging functionality
--]]
function print(...)
    log.info(...)
end

local function merge(vals)
    for i, _ in ipairs(vals) do
        vals[i] = tostring(vals[i])
    end

    return table.concat(vals, "\t")
end

function log.debug(...) log.log("debug", merge{...}) end
function log.info(...)  log.log("info",  merge{...}) end
function log.warn(...)  log.log("warn",  merge{...}) end
function log.err(...)   log.log("error", merge{...}) end
function log.wtf(...)   log.log("wtf",   merge{...}) end
