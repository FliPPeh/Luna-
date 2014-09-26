---
-- Luna core support library, bridges the interface between C++ and actual
-- scripts.
--
-- Although it is located in scripts/lib/, it is not intended for require()ing
-- by scripts.
--
local luna = {}
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

-- Logging functionality
function print(...)
    log.info(...)
end

function log.debug(...)
    log.log('info', table.concat({...}, '\t'))
end

function log.info(...)
    log.log('info', table.concat({...}, '\t'))
end

function log.warn(...)
    log.log('warn', table.concat({...}, '\t'))
end

function log.err(...)
    log.log('error', table.concat({...}, '\t'))
end

function log.wtf(...)
    log.log('wtf', table.concat({...}, '\t'))
end

---
-- Signal handling
local __callbacks = {}
local __nextid = 0

function luna.add_signal_handler(signal, id, fn)
    if not fn then
        fn = id

        id = '__unique_' .. tostring(__nextid)
                  .. '_' .. tostring(math.random(999))

        __nextid = __nextid + 1
    end

    __callbacks[id] = {
            signal = signal,
            callback = fn
        }

    return id
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
local function splitcmdline(line, splittype)
    if splittype == '*w' then
        local parts = line:split(' ')

        local cmd = table.remove(parts, 1)
        local args = parts

        return cmd, args
    else
        local _, _, cmd, args = line:find('(%w+)(%s*(.*))')

        return cmd, args
    end
end

-- TODO: private command?

function luna.add_command(command, argtype, fn)
    if type(argtype) == 'function' then
        -- only 2 args, use default
        fn = argtype
        argtype = '*w'
    end

    return luna.add_signal_handler('channel_command', function(who, where, what)
        if what and what == '' then
            return
        end

        local cmd, args = splitcmdline(what, argtype)

        if cmd:lower() == command:lower() then
            fn(who, where, cmd, args)
        end
    end)
end

function luna.add_trigger_command(command, argtype, fn)
    if type(argtype) == 'function' then
        -- only 2 args, use default
        fn = argtype
        argtype = '*w'
    end

    return luna.add_signal_handler('channel_message', function(who, where, what)
        if what and what == '' then
            return
        end

        local cmd, args = splitcmdline(what, argtype)
        local trigger = luna.shared['luna.trigger'] or '!'

        if      cmd:sub(1, 1) == trigger
            and cmd:sub(1):lower() == command:lower() then
                fn(who, where, cmd, args)
        end
    end)
end

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


-- Entry function from C++
function luna.handle_signal(signal, ...)
    local args = {...}

    for id, handler in pairs(__callbacks) do
        if handler.signal == signal then
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

-- Entry function from C++
function luna.init_script()
    ok, res = xpcall(script.script_load, function(err)
        log.warn('[CORE]',
            debug.traceback(
                string.format("`script.load' error: %s", err),
                2))

            return false
    end)
end

-- Entry function from C++
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

-- Wrap up __luna.shared.get and __luna.shared.set inside a nice and intuitive
-- table.
local __shared = luna.shared

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


function luna.privmsg(target, msg) luna.send_message('PRIVMSG', target, msg) end
function luna.notice(target, msg)  luna.send_message('NOTICE', target, msg)  end

function luna.join(channel, key)
    if key then
        luna.send_message('JOIN', channel, key or '')
    else
        luna.send_message('JOIN', channel)
    end
end

function luna.part(channel, reason)
    luna.send_message('PART', channel, reason or '')
end

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



-- Augment basic types
channelX = {}
setmetatable(luna.channel_meta.__index, { __index = channelX })

function channelX:privmsg(msg) luna.privmsg(self:name(), msg) end
function channelX:notice(msg)  luna.notice(self:name(),  msg) end

channel_userX = {}
setmetatable(luna.channel_user_meta.__index, { __index = channel_userX })

function channel_userX:nick() return ({self:user_info()})[1] end
function channel_userX:user() return ({self:user_info()})[2] end
function channel_userX:host() return ({self:user_info()})[3] end

function channel_userX:privmsg(msg) luna.privmsg(self:nick(), msg) end
function channel_userX:notice(msg)  luna.notice(self:nick(),  msg) end

function channel_userX:mask(style, mtype)
    return luna.util.mask(string.format('%s!%s@%s',
        self:nick(), self:user(), self:host()), style, mtype)
end

function channel_userX:respond(msg)
    return self:channel():privmsg(self:nick() .. ': ' .. msg)
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


return luna
