--[[
-- Signal handling
--]]
local d = require("dumplib")

local __callbacks = {}
local __current_handler = nil
local __nextid = 0

local function dupkeys(t)
    local keys = {}

    for k, v in pairs(t) do
        table.insert(keys, k)
    end

    return keys
end

function luna.dispatch_signal(signal, ...)
    local args = {...}

    -- Make a copy of all keys here so we can add and remove signal handlers
    -- while handling signals.
    local __callbacks_keys = dupkeys(__callbacks)

    for _, id in ipairs(__callbacks_keys) do
        local handler = __callbacks[id]
        __current_handler = handler

        if handler and handler.signal == signal then
            status, err = xpcall(handler.callback, function(err)
                log.warn("[CORE]",
                    debug.traceback(
                        string.format(
                            "handler for \"%s\" error: %s", signal, err),
                        2))
            end, unpack(args))
        end
    end
end

function luna.add_signal_handler(signal, id, fn)
    if not fn then
        fn = id

        id = string.format("__unique_%s_%s",
                tostring(__nextid),
                tostring(math.random(999)))

        __nextid = __nextid + 1
    end

    if signal == "channel_message" then
        local wrapped = fn

        fn = function(who, where, what, lvl)
            local ru = who:match_reguser()
            local ai = luna.shared["luna.auto_ignore"] or false

            if ru and ru:flags():find("i") and ai then
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

    error(string.format("no signal handler with id %q found", tid), 2)
end


--[[
-- Higher level signal handling
--
-- TODO: private command?
--]]
local __command_handler = nil
local __current_command = nil
local __commands = {}

local function command_handler(who, where, what)
    local triggers = {
        where:trigger(),                               -- !command
        luna.own_nick():literalpattern() .. "[:,]?%s*" -- mynick: command
    }

    local disabled = where:disabled_contexts()

    for _, ctx in ipairs(luna.command_contexts()) do
        for i, dis in ipairs(disabled) do
            if dis:lower() == ctx.name:lower() then
                goto continue
            end
        end

        local trigger = ctx.trigger:template{
            trigger = where:trigger(),
            own_nick = luna.own_nick():literalpattern():icasepattern()
        }

        -- TODO: not this
        luna.__response_template = ctx.response

        -- To allow per-channel or global disabling of non-highlight trigger
        if not (#trigger > 0) then
            goto continue
        end

        local a, b, rcmd, args = what:find("^" .. trigger .. "([^%s]+)%s*(.*)")

        if a then
            luna.dispatch_signal("channel_command", who, where, rcmd, args)

            for _, cmd in ipairs(dupkeys(__commands)) do
                __current_command = cmd

                local cmdopts = __commands[cmd]

                if cmdopts and cmd:lower() == rcmd:lower() then
                    if cmdopts.argtype == "*w" then
                        args = args:split(" ")
                    elseif cmdopts.argtype == "*s" then
                        args = args:shlex()
                    end

                    cmdopts.func(who, where, cmd, args)
                end
            end

            break

        end

        ::continue::
    end
end

__command_handler = luna.add_signal_handler("channel_message", command_handler)

function luna.add_command_context(name, trigger, response)
    local cur = luna.command_contexts()

    for i, v in ipairs(cur) do
        if v.name:lower() == name:lower() then
            error("context with the same name already registered", 2)
        end
    end

    table.insert(cur, {
        name = name,
        trigger = trigger,
        response = response
    })

    luna.shared["luna.cmdcontexts"] = d.serialize(cur)
end

function luna.remove_command_context(name)
    local cur = luna.command_contexts()

    for i, v in ipairs(cur) do
        if v.name:lower() == name:lower() then
            table.remove(cur, i)
            break
        end
    end

    luna.shared["luna.cmdcontexts"] = d.serialize(cur)
end

function luna.command_contexts()
    if not luna.shared["luna.cmdcontexts"] then
        luna.shared["luna.cmdcontexts"] = d.serialize{
            {
                name     = "simple",
                trigger  = "${trigger}", -- default channel trigger
                response = "${response}" -- default response
            },
            {
                name     = "hilight_simple",
                trigger  = "${own_nick}[:,]?%s*",
                response = "${nick}: ${response}"
            }
        }
    end

    return d.unserialize(luna.shared["luna.cmdcontexts"])
end

function luna.add_command(command, argtype, fn)
    if type(argtype) == "function" then
        -- only 2 args, use default
        fn = argtype
        argtype = "*s"
    end

    __commands[command] = {
            argtype = argtype,
            func    = fn
        }

    if not __command_handler then
        __command_handler =
            luna.add_signal_handler("channel_message", command_handler)
    end
end

function luna.remove_command(command)
    __commands[command] = nil
end


--[[
-- Message and IRC command watchers
--]]
function luna.add_message_watcher(pattern, fn)
    return luna.add_signal_handler("channel_message", function(who, where, what)
        local matches = {what:find(pattern)}

        if matches[1] and matches[2] then
            fn(who, where, what, unpack(matches))
        end
    end)
end

function luna.add_command_watcher(cmd, fn)
    return luna.add_signal_handler("raw", function(prefix, command, args)
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
