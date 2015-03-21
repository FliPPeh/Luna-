local base64 = require 'base64'

--[[
-- Trigger helpers
--]]
local function trigger_key(channel)
    return string.format('luna.channel.%s.trigger', channel:lower())
end


function luna.set_channel_trigger(channel, trigger)
    luna.shared[trigger_key(channel)] = trigger
end

function luna.channel_trigger(channel)
    return luna.shared[trigger_key(channel)] or luna.shared['luna.trigger']
end

--[[
-- Context helpers
--]]
local function disabled_contexts_key(channel)
    return string.format('luna.channel.%s.disabled_ctx', channel:lower())
end

function luna.channel_disabled_contexts(channel)
    local r = luna.shared[disabled_contexts_key(channel)]

    return r and r:split(';') or {}
end

function luna.channel_enable_context(channel, ctx)
    local cur = luna.channel_disabled_contexts(channel)

    for i, v in ipairs(cur) do
        if v:lower() == ctx:lower() then
            table.remove(cur, i)

            local r
            if #cur ~= 0 then
                r = table.concat(cur, ";")
            end

            luna.shared[disabled_contexts_key(channel)] = r
            return
        end
    end
end

function luna.channel_disable_context(channel, ctx)
    local cur = luna.channel_disabled_contexts(channel)

    for i, v in ipairs(cur) do
        if v:lower() == ctx:lower() then
            return
        end
    end

    table.insert(cur, ctx:lower())
    luna.shared[disabled_contexts_key(channel)] = table.concat(cur, ";")
    return
end

--[[
-- Filter helpers
--]]
local function filter_in_key(channel)
    return string.format('luna.channel.%s.filter_in', channel:lower())
end

local function filter_out_key(channel)
    return string.format('luna.channel.%s.filter_out', channel:lower())
end


function luna.set_channel_incoming_filter(channel, fun)
    luna.shared[filter_in_key(channel)] =
        fun and base64.encode(string.dump(fun)) or nil
end

function luna.channel_incoming_filter(channel)
    return luna.shared[filter_in_key(channel)]
       and loadstring(base64.decode(luna.shared[filter_in_key(channel)]))
end


function luna.set_channel_outgoing_filter(channel, fun)
    luna.shared[filter_out_key(channel)] =
        fun and base64.encode(string.dump(fun)) or nil
end

function luna.channel_outgoing_filter(channel)
    return luna.shared[filter_out_key(channel)]
       and loadstring(base64.decode(luna.shared[filter_out_key(channel)]))
end

--[[
-- Augmented channel class
--]]
local channel_meta_aux = {}

setmetatable(luna.channel_meta.__index, {
        __index = channel_meta_aux
    })


function channel_meta_aux:privmsg(msg, lvl)
    local filter = self:outgoing_filter()

    return luna.privmsg((lvl or '') .. self:name(),
        filter and filter(self, msg) or msg)
end

function channel_meta_aux:notice(msg, lvl)
    return luna.notice((lvl or '') .. self:name(), msg)
end

function channel_meta_aux:action(msg, lvl)
    return luna.action((lvl or '') .. self:name(), msg)
end

function channel_meta_aux:request_ctcp(ctcp, arg, lvl)
    return luna.request_ctcp((lvl or '') .. self:name(), ctcp, arg)
end

function channel_meta_aux:respond_ctcp(ctcp, arg, lvl)
    return luna.respond_ctcp((lvl or '') .. self:name(), ctcp, arg)
end


function channel_meta_aux:set_trigger(trigger)
    return luna.set_channel_trigger(self:name(), trigger)
end

function channel_meta_aux:trigger()
    return luna.channel_trigger(self:name())
end


function channel_meta_aux:disabled_contexts()
    return luna.channel_disabled_contexts(self:name())
end

function channel_meta_aux:disable_context(ctx)
    return luna.channel_disable_context(self:name(), ctx)
end

function channel_meta_aux:enable_context(ctx)
    return luna.channel_enable_context(self:name(), ctx)
end


function channel_meta_aux:set_incoming_filter(fun)
    return luna.set_channel_incoming_filter(self:name(), fun)
end

function channel_meta_aux:incoming_filter(fun)
    return luna.channel_incoming_filter(self:name())
end


function channel_meta_aux:set_outgoing_filter(fun)
    return luna.set_channel_outgoing_filter(self:name(), fun)
end

function channel_meta_aux:outgoing_filter(fun)
    return luna.channel_outgoing_filter(self:name())
end

