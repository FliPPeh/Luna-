---
-- Luna core support library, bridges the interface between C++ and actual
-- scripts.
--
-- Although it is located in scripts/lib/, it is not intended for require()ing
-- by scripts.
--
luna = {}

local base64 = require 'base64'

--[[
-- Merge __luna native library into corelib
--]]
for k, v in pairs(__luna) do
    luna[k] = v
end

math.randomseed(os.time())

require("luna.logging")
require("luna.util")
require("luna.channel")
require("luna.user")
require("luna.shared")
require("luna.signals")


--[[
-- Entry functions from C++
--]]

-- Event handler
function luna.handle_signal(signal, ...)
    return luna.dispatch_signal(signal, ...)
end

-- Script init
function luna.init_script()
    -- Then initialize script
    ok, res = xpcall(script.script_load, function(err)
        log.warn('[CORE]',
            debug.traceback(
                string.format("`script.load' error: %s", err),
                2))

            return false
    end)
end

-- Script deinit
function luna.deinit_script()
    if not script.script_unload then
        return true
    end

    ok, res = xpcall(script.script_unload, function(err)
        log.warn('[CORE]',
            debug.traceback(
                string.format("`script_unload' error: %s", err),
                2))

        return false
    end)
end

--[[
-- High level command helpers
--]]
function luna.privmsg(tar, msg)
    return luna.send_message('PRIVMSG', tar, msg)
end

function luna.notice(tar, msg)
    return luna.send_message('NOTICE', tar, msg)
end


local function make_ctcp(ctcp, arg)
    if arg then
        return string.format('\x01%s %s\x01', ctcp:upper(), arg)
    else
        return string.format('\x01%s\x01', ctcp:upper())
    end
end

function luna.request_ctcp(tar, ctcp, arg)
    return luna.privmsg(tar, make_ctcp(ctcp, arg))
end

function luna.respond_ctcp(tar, ctcp, arg)
    return luna.notice(tar, make_ctcp(ctcp, arg))
end


function luna.action(tar, msg)
    return luna.request_ctcp(tar, 'ACTION', msg)
end


function luna.join(channel, key)
    return luna.send_message('JOIN', channel, key)
end

function luna.part(channel, reason)
    return luna.send_message('PART', channel, reason or '')
end

--[[
-- Info wrappers
--]]

-- Wrap up luna.runtime_info()
function luna.started()   return ({luna.runtime_info()})[1] end
function luna.connected() return ({luna.runtime_info()})[2] end

-- Wrap up luna.user_info()
function luna.own_nick() return ({luna.user_info()})[1] end
function luna.own_user() return ({luna.user_info()})[2] end

-- Wrap up luna.server_info()
function luna.server_host() return ({luna.server_info()})[1] end
function luna.server_addr() return ({luna.server_info()})[2] end
function luna.server_port() return ({luna.server_info()})[3] end

-- Wrap up luna.traffic_info()
function luna.bytes_sent_total()     return ({luna.traffic_info()})[1] end
function luna.bytes_sent()           return ({luna.traffic_info()})[2] end
function luna.bytes_received_total() return ({luna.traffic_info()})[3] end
function luna.bytes_received()       return ({luna.traffic_info()})[4] end

return luna
