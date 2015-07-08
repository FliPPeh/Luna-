--[[
-- Shared functions
--]]
local function nick(self) return ({self:user_info()})[1] end
local function user(self) return ({self:user_info()})[2] end
local function host(self) return ({self:user_info()})[3] end

local function mask(self, style, mtype)
    return luna.util.mask(string.format("%s!%s@%s",
        self:nick(), self:user(), self:host()), style, mtype)
end

local function is_me(self)
    return self:nick():rfc1459lower() == luna.own_nick():rfc1459lower()
end

local function privmsg(self, msg) return luna.privmsg(self:nick(), msg) end
local function notice(self, msg)  return luna.notice(self:nick(), msg) end
local function action(self, msg)  return luna.action(self:nick(), msg) end

local function request_ctcp(self, ctcp, arg)
    return luna.request_ctcp(self:nick(), ctcp, arg)
end

local function respond_ctcp(self, ctcp, arg)
    return luna.respond_ctcp(self:nick(), ctcp, arg)
end

--[[
-- Augmented unknown user class
--]]
local unknown_user_aux = {
        nick  = nick,
        user  = user,
        host  = host,

        mask  = mask,
        is_me = is_me,

        privmsg = privmsg,
        notice  = notice,
        action  = action,

        request_ctcp = request_ctcp,
        respond_ctcp = respond_ctcp
    }

setmetatable(luna.unknown_user_meta.__index, {
        __index = unknown_user_aux
    })


function unknown_user_aux:respond(msg)
    if luna.__response_template then
        return self:privmsg(luna.__response_template:template{
            nick = self:nick(),
            user = self:user(),
            host = self:host(),
            response = msg
        })
    else
        return self:privmsg(msg)
    end
end


--[[
-- Augmented channel user class
--]]
local channel_user_aux = {
        nick  = nick,
        user  = user,
        host  = host,

        mask  = mask,
        is_me = is_me,

        privmsg = privmsg,
        notice  = notice,
        action  = action,

        request_ctcp = request_ctcp,
        respond_ctcp = respond_ctcp
    }

setmetatable(luna.channel_user_meta.__index, {
        __index = channel_user_aux
    })


function channel_user_aux:respond(msg, lvl)
    if luna.__response_template then
        return self:channel():privmsg(luna.__response_template:template{
            nick = self:nick(),
            user = self:user(),
            host = self:host(),
            response = msg
        }, lvl)
    else
        return self:channel():privmsg(self:nick() .. ": " .. msg, lvl)
    end
end

--[[
-- Helpers
--]]
function luna.with_reguser(user, fn)
    local reguser = user:match_reguser(user)

    if reguser then
        return fn(true, reguser)
    else
        return fn(false)
    end
end
