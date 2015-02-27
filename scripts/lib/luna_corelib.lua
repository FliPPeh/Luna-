---
-- Luna core support library, bridges the interface between C++ and actual
-- scripts.
--
-- Although it is located in scripts/lib/, it is not intended for require()ing
-- by scripts.
--
local luna = {}

local base64 = require 'base64'

--
-- Merge __luna native library into corelib
for k, v in pairs(__luna) do
    luna[k] = v
end

luna.util = {}

math.randomseed(os.time())


local function splitmask(prefix)
    nick = prefix:sub(1, prefix:find('!') - 1)
    user = prefix:sub(prefix:find('!') + 1, prefix:find('@') - 1)
    addr = prefix:sub(prefix:find('@') + 1)

    if addr:match('(%d+.%d+.%d+.%d+)') then
        host   = addr:sub(addr:find('^%d+.%d+.%d'))
        domain = addr:sub(addr:find('%d+$'))
    else
        -- special case for no-host addresses
        if addr:match('^%w+.%w+$') then
            host = addr
            domain = ''
        else
            host   = addr:sub(addr:find('^(%w+)'))
            domain = addr:sub(addr:find('%w+.%w+$'))
        end
    end

    user = user:gsub('^~', '')

    return nick, user, host, domain
end

luna.util.default_ban_mask  = 4
luna.util.default_user_mask = 2

-- Mask a hostname
function luna.util.mask(prefix, style, mtype)
    local style = style or 'irc'
    local mtype = mtype or 4
    local nick, user, host, domain = splitmask(prefix)

    styles = {
        irc  = {  '*',     '*' },
        emca = {'(.+?)', '(.?)'},
        lua  = {'(.-)',  '(.?)'},
    }

    if not styles[style:lower()] then
        error('unsupported hostmask style', 2)
    end

    local many1 = styles[style:lower()][1]
    local any0  = styles[style:lower()][2]

    masks = {
        {many1,         user, host,   domain},
        {many1, any0 .. user, host,   domain},
        {many1, many1,        host,   domain},
        {many1, any0 .. user, many1,  domain},
        {many1, many1,        many1,  domain},
        {nick,          user, host,   domain},
        {nick,  any0 .. user, host,   domain},
        {nick,  many1,        host,   domain},
        {nick,  any0 .. user, many1,  domain},
        {nick,  many1,        many1,  domain},
    }

    if mtype > #masks or mtype < 1 then
        error('unsupported hostmask type',  2)
    end

    return string.format(domain ~= '' and '%s!%s@%s.%s'
                                       or '%s!%s@%s', unpack(masks[mtype]))
end

-- Collects mode changes
--   e.g.:
--     luna.util.collect_modechanges{
--        '+o user1',       -- as plain string
--        {'+o', 'user2'},  -- as separated mode and argument
--        '-o user3',
--        '+t'}             -- as plain string without argument
--     => '+oot-o user1 user2 user3'
function luna.util.collect_modechanges(modes)
    table.sort(modes, function(a, b)
        if type(a) == 'table' then
            a = a[1]
        end

        if type(b) == 'table' then
            b = b[1]
        end

        return a:sub(2,2) > b:sub(2,2)
    end)

    local   set,   seta = {}, {}
    local unset, unseta = {}, {}

    for i, modeset in ipairs(modes) do
        local m, a

        if type(modeset) == 'table' then
            if #modeset > 1 then
                m, a = unpack(modeset)
            else
                m, a = modeset[1], ''
            end
        else
            _, _, m, a = modeset:find('([%+%-]%a)%s*(%w*)')
        end


        m = m:gsub('^%s*(.-)%s*$', '%1')
        a = a:gsub('^%s*(.-)%s*$', '%1')

        if m:sub(1, 1) == '+' then
            table.insert(set, m:sub(2, 2))

            if #a > 0 then
                table.insert(seta, a)
            end
        else
            table.insert(unset, m:sub(2, 2))

            if #a > 0 then
                table.insert(unseta, a)
            end
        end
    end

    rv = ''

    if #set > 0 then
        rv = '+' .. table.concat(set)
    end

    if #unset > 0 then
        rv = rv .. '-' .. table.concat(unset)
    end

    return rv .. ' ' .. table.concat(seta,   ' ')
              .. ' ' .. table.concat(unseta, ' ')
end

-- Logging functionality
function print(...)
    log.info(...)
end

local function merge(vals)
    for i, _ in ipairs(vals) do
        vals[i] = tostring(vals[i])
    end

    return table.concat(vals, '\t')
end

function log.debug(...) log.log('debug', merge{...}) end
function log.info(...)  log.log('info',  merge{...}) end
function log.warn(...)  log.log('warn',  merge{...}) end
function log.err(...)   log.log('error', merge{...}) end
function log.wtf(...)   log.log('wtf',   merge{...}) end

---
-- Signal handling
--
local __callbacks = {}
local __current_handler = nil
local __nextid = 0

function luna.add_signal_handler(signal, id, fn)
    if not fn then
        fn = id

        id = string.format('__unique_%s_%s',
                tostring(__nextid),
                tostring(math.random(999)))

        __nextid = __nextid + 1
    end

    if signal == 'channel_message' then
        local wrapped = fn

        fn = function(who, where, what, lvl)
            local ru = who:match_reguser()
            local ai = luna.shared['luna.auto_ignore'] or false

            if ru and ru:flags():find('i') and ai then
                return
            end

            local filter = where:incoming_filter()

            wrapped(who, where, filter and filter(where, what) or what, lvl)
        end
    end


    __callbacks[id] = {
            signal = signal,
            callback = fn,
            id = id
        }

    return id
end

function luna.current_signal_handler()
    if __current_handler.id == __command_handler then
        return __current_command
    else
        return __current_handler.id
    end
end

function luna.remove_signal_handler(tid)
    for id, _ in pairs(__callbacks) do
        if id == tid then
            __callbacks[id] = nil
            return
        end
    end

    error(string.format('no signal handler with id %q found', tid), 2)
end


---
-- Higher level signal handling
-- TODO: private command?
--
local __command_handler = nil
local __current_command = nil
local __commands = {}

local function dupkeys(t)
    local keys = {}

    for k, v in pairs(t) do
        table.insert(keys, k)
    end

    return keys
end

local function command_handler(who, where, what)
    local triggers = {
        where:trigger(),                               -- !command
        luna.own_nick():literalpattern() .. '[:,]?%s*' -- mynick: command
    }

    for _, trigger in ipairs(triggers) do
        -- To allow per-channel or global disabling of non-highlight trigger
        if not (#trigger > 0) then
            goto continue
        end

        local a, b, rcmd, args = what:find('^' .. trigger .. '([^%s]+)%s*(.*)')

        if a then
            for _, cmd in ipairs(dupkeys(__commands)) do
                __current_command = cmd

                local cmdopts = __commands[cmd]

                if cmdopts and cmd:lower() == rcmd:lower() then
                    if cmdopts.argtype == '*w' then
                        args = args:split(' ')
                    end

                    cmdopts.func(who, where, cmd, args)
                end
            end

            break

        end

        ::continue::
    end
end


function luna.add_command(command, argtype, fn)
    if type(argtype) == 'function' then
        -- only 2 args, use default
        fn = argtype
        argtype = '*w'
    end

    __commands[command] = {
            argtype = argtype,
            func    = fn
        }

    if not __command_handler then
        __command_handler =
            luna.add_signal_handler('channel_message', command_handler)
    end
end

function luna.remove_command(command)
    __commands[command] = nil
end


---
-- Message and IRC command watchers
--
function luna.add_message_watcher(pattern, fn)
    return luna.add_signal_handler('channel_message', function(who, where, what)
        local matches = {what:find(pattern)}

        if matches[1] and matches[2] then
            fn(who, where, what, unpack(matches))
        end
    end)
end

function luna.add_command_watcher(cmd, fn)
    return luna.add_signal_watcher('raw', function(prefix, command, args)
        if command:lower() == cmd:lower() then
            fn(prefix, command, args)
        end
    end)
end


function luna.remove_current_handler()
    if __current_handler.id == __command_handler then
        luna.remove_command(__current_command)
    else
        luna.remove_signal_handler(__current_handler.id)
    end
end

---
-- Entry functions from C++
--

-- Event handler
function luna.handle_signal(signal, ...)
    local args = {...}

    -- Make a copy of all keys here so we can add and remove signal handlers
    -- while handling signals.
    local __callbacks_keys = dupkeys(__callbacks)

    for _, id in ipairs(__callbacks_keys) do
        local handler = __callbacks[id]
        __current_handler = handler

        if handler and handler.signal == signal then
            status, err = xpcall(handler.callback, function(err)
                log.warn('[CORE]',
                    debug.traceback(
                        string.format(
                            "handler for `%s' error: %s", signal, err),
                        2))
            end, unpack(args))
        end
    end
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

---
-- Higher leveled wrappers

-- Wrap up __luna.shared.get and __luna.shared.set inside a nice and intuitive
-- table.
local __shared = luna.shared

function luna.save_shared()
    __shared.save()
end

function luna.reload_shared()
    __shared.reload()
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


---
-- High level command helpers
--
function luna.privmsg(tar, msg) luna.send_message('PRIVMSG', tar, msg) end
function luna.notice(tar, msg)  luna.send_message('NOTICE', tar, msg)  end

function luna.request_ctcp(tar, ctcp, arg)
    if arg then
        luna.privmsg(tar, string.format('\x01%s %s\x01', ctcp:upper(), arg))
    else
        luna.privmsg(tar, string.format('\x01%s\x01', ctcp:upper()))
    end
end

function luna.respond_ctcp(tar, ctcp, arg)
    if arg then
        luna.notice(tar, string.format('\x01%s %s\x01', ctcp:upper(), arg))
    else
        luna.notice(tar, string.format('\x01%s\x01', ctcp:upper()))
    end
end

function luna.action(tar, msg)
    luna.request_ctcp(tar, 'ACTION', msg)
end


function luna.join(channel, key)
    luna.send_message('JOIN', channel, key)
end

function luna.part(channel, reason)
    luna.send_message('PART', channel, reason or '')
end

---
-- Info wrappers
--

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

---
-- Trigger and filter management
--
local function trigger_key(channel)
    return string.format('luna.channel.%s.trigger', channel:lower())
end

function luna.set_channel_trigger(channel, trigger)
    luna.shared[trigger_key(channel)] = trigger
end

function luna.channel_trigger(channel)
    return luna.shared[trigger_key(channel)] or luna.shared['luna.trigger']
end


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

function luna.set_channel_outgoing_filter(channel, fun)
    luna.shared[filter_out_key(channel)] =
        fun and base64.encode(string.dump(fun)) or nil
end

function luna.channel_incoming_filter(channel)
    return luna.shared[filter_in_key(channel)]
       and loadstring(base64.decode(luna.shared[filter_in_key(channel)]))
end

function luna.channel_outgoing_filter(channel)
    return luna.shared[filter_out_key(channel)]
       and loadstring(base64.decode(luna.shared[filter_out_key(channel)]))
end

---
-- Augment basic types
---

---
-- Augmented channel class
--
local channel_meta_aux = {}

setmetatable(luna.channel_meta.__index, {
        __index = channel_meta_aux
    })


function channel_meta_aux:privmsg(msg, lvl)
    local filter = self:outgoing_filter()

    luna.privmsg((lvl or '') .. self:name(),
        filter and filter(self, msg) or msg)
end

function channel_meta_aux:notice(msg, lvl)
    luna.notice((lvl or '') .. self:name(), msg)
end

function channel_meta_aux:action(msg, lvl)
    luna.action((lvl or '') .. self:name(), msg)
end

function channel_meta_aux:request_ctcp(ctcp, arg, lvl)
    luna.request_ctcp((lvl or '') .. self:name(), ctcp, arg)
end

function channel_meta_aux:respond_ctcp(ctcp, arg, lvl)
    luna.respond_ctcp((lvl or '') .. self:name(), ctcp, arg)
end


function channel_meta_aux:set_incoming_filter(fun)
    luna.set_channel_incoming_filter(self:name(), fun)
end

function channel_meta_aux:set_outgoing_filter(fun)
    luna.set_channel_outgoing_filter(self:name(), fun)
end


function channel_meta_aux:incoming_filter(fun)
    return luna.channel_incoming_filter(self:name())
end

function channel_meta_aux:outgoing_filter(fun)
    return luna.channel_outgoing_filter(self:name())
end

function channel_meta_aux:set_trigger(trigger)
    luna.set_channel_trigger(self:name(), trigger)
end

function channel_meta_aux:trigger()
    return luna.channel_trigger(self:name())
end


---
-- Augmented user types (known and unknown
--

-- Shared functions
local function nick(self) return ({self:user_info()})[1] end
local function user(self) return ({self:user_info()})[2] end
local function host(self) return ({self:user_info()})[3] end

local function mask(self, style, mtype)
    return luna.util.mask(string.format('%s!%s@%s',
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

---
-- Augmented unknown user class
--
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
    return self:privmsg(msg)
end


---
-- Augmented channel user class
--
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
    return self:channel():privmsg(self:nick() .. ': ' .. msg, lvl)
end



-- Also augment core types for fun and profit
function string:split(sep)
    local parts = {}
    local l = 1

    -- While there is a seperator within the string
    while self:find(sep, l) do
        local sep_start, sep_end = self:find(sep, l)

        -- Unless the substring between the last seperators was empty, add
        -- it to the results
        if sep_start ~= l then
            -- Add the part between l (last seperator end or string start) and
            -- sep_start
            table.insert(parts, self:sub(l, sep_start - 1))
        end

        -- put l after the seperator end
        l = sep_end + 1
    end

    if self:len() >= l then
        table.insert(parts, self:sub(l))
    end

    return parts
end


function string:fcolour(f, b)
    if not b then
        return string.format('\x03%02d%s\x03', f, self)
    else
        return string.format('\x03%02d,%02d%s\x03', f, b, self)
    end
end

function string:fbold()
    return string.format('\x02%s\x02', self)
end

function string:funderline()
    return string.format('\x1f%s\x1f', self)
end

function string:freverse()
    return string.format('\x16%s\x16', self)
end

function string:stripformat()
    return ({
        self:gsub('\x03%d%d?,%d%d?',      '')
            :gsub('\x03%d%d?',            '')
            :gsub('([\x02\x1f\x16\x03])', '')})[1]

end

local function map_irc_color(f, b)
    local irc_to_ansi = {
        -- white, black, dark blue, dark green, red, brown, purple, orange
           17,    0,     4,         2,          11,  1,     5,      3,
        -- yellow, green, teal, aqua, blue, margenta, dark grey, grey
           13,     12,    4,    16,   14,   15,       10,        7
    }

    local function genseq(ansi, class)
        class = class or 3

        if ansi >= 10 then
            return string.format('\027[1;%d%dm', class, ansi - 10)
        else
            return string.format('\027[0;%d%dm', class, ansi)
        end
    end


    res = ''

    if (f+1) <= #irc_to_ansi then
        res = res .. genseq(irc_to_ansi[f + 1])
    end

    if b and (b+1) <= #irc_to_ansi then
        res = res .. genseq(irc_to_ansi[b + 1], 4)
    end

    return res
end

-- Not a perfect translation.
function string:irctoansi()
    bold = false
    underline = false

    return ({
        self:gsub('\x03(%d%d?),(%d%d?)', function(f, g)
                    return map_irc_color(tonumber(f), tonumber(g))
                end)
            :gsub('\x03(%d%d?)', function(f)
                    return map_irc_color(tonumber(f))
                end)

            :gsub('\x03', function() return '\027[0;39m' end)
            :gsub('\x02', function()
                    if not bold then
                        bold = true
                        return '\027[1m'
                    else
                        bold = false
                        return '\027[0m'
                    end
                end)
            :gsub('\x1f', function()
                    if not bold then
                        bold = true
                        return '\027[4m'
                    else
                        bold = false
                        return '\027[0m'
                    end
                end)
        })[1]
end

-- Turns the given string into a literal pattern of itself (by escaping all
-- characters that have a special meaning in pattern matching)
function string:literalpattern()
    return self:gsub('[^%w%s]', '%%%1')
end

-- Given a map of keys to values, format a string using interpolations, e.g.
--
--   str = "Hello ${SUBJECT}"
--   print(str:template{SUBJECT = "World"}) # Hello World
--
-- Keys can refer to keys inside the given table "fmt", or to environment
-- variables ("env:HOME") or shared Luna variables ("var:luna.version").
--
-- Replacements can be escaped with "$${VAR}".
--
-- If a variable can not be located, it is replaced with the value of "rep"
-- unless a special replacement is specified ("${VAR/<VAR not found>")
function string:template(fmt, rep)
    rep = rep or '(?)'

    return self:gsub("(.?)($%b{})", function(p, m)
        local k = m:sub(3, #m - 1)

        p = p or ''

        if p == '$' then
            return m
        end

        local i = k:find('/')
        if i then
            rep = k:sub(i + 1)
            k = k:sub(1, i)
        end

        if k:find('^env:') then
            return p .. (os.getenv(k:sub(5)) or rep)
        elseif k:find('^var:') then
            return p .. (luna.shared[k:sub(5)] or rep)
        elseif fmt[k] then
            return p .. fmt[k]
        else
            return p .. rep
        end
    end)
end

---
-- Helpers
--

function luna.with_reguser(user, fn)
    local reguser = user:match_reguser(user)

    if reguser then
        return fn(true, reguser)
    else
        return fn(false)
    end
end


return luna
