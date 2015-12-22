local base = {}

local dump = require "dumplib"
local exec = require "exec"
local hrnums = require "hrnums"
local time = require "time"

base.info = {
    name         = "Base",
    description  = "Do basey things",
    version      = "0.1"
}


function base.script_load()
    local nsusr = luna.shared["base.nickserv.user"]
    local nspwd = luna.shared["base.nickserv.passwd"]

    if nsusr and nspwd then
        luna.add_signal_handler("connect", function()
            luna.send_message("NICKSERV",
                string.format("identify %s %s", nsusr, nspwd))
        end)
    end

    luna.add_command("sitrep", function(who, where, what, args)
        local user = who:match_reguser()
        if not user or not user:flags():find("o") then
            return
        end

        local start = luna.started()
        local connect = luna.connected()
        local now = os.time()

        local fmt = "%Y-%m-%d %H:%M:%S"

        local ds, dc = now - start, now - connect
        local ts, tr = luna.bytes_sent(), luna.bytes_received()

        local tin, ui  = hrnums.humanize_binary(tr, "B")
        local tout, uo = hrnums.humanize_binary(ts, "B")

        local msgs = {
            ("Started on %s (%s ago), connected on %s (%s ago); "):format(
                os.date(fmt, start),
                time.pretty_delta(start, now),
                os.date(fmt, connect),
                time.pretty_delta(connect, now)),

            ("Traffic: %.2f %s/s in (%.2f %s total), "):format(
                tin / dc, ui,
                tin, ui),

            ("%.2f %s/s out (%.2f %s total)"):format(
                tout / dc, uo,
                tout, uo)
        }

        who:respond(table.concat(msgs))
    end)

    luna.add_command("sh", "*l", function(who, where, what, args)
        local user = who:match_reguser()
        if user and user:flags():find("a") then
            local e = exec.exec{"/bin/bash", "-c", args}
            local t = {}

            for l in e:lines() do
                table.insert(t, l)
            end

            who:respond(table.concat(t, "; "))
        end
    end)

    luna.add_command("eval", "*l", function(who, where, what, args)
        local function do_load(s)
            return load(s, "(eval)", "bt", setmetatable({
                who = who,
                where = where,
                what = what,
                args = args,
                user = user
            }, { __index = _ENV }))
        end

        local user = who:match_reguser()
        if user and user:flags():find("a") then
            local f = do_load(args)
            if not f then
                f, e = do_load("return " .. args)

                if not f then
                    who:respond(e)
                    return
                end
            end

            local r = {pcall(f)}
            local ok = table.remove(r, 1)

            if ok then
                if #r > 1 then
                    for i, v in ipairs(r) do
                        r[i] = dump.dump_inline(r[i])
                    end

                    who:respond(table.concat(r, ", "))
                else
                    who:respond(dump.dump_inline(r[1]))
                end
            else
                log.err(r[1])

                local errs = tostring(r[1]):split("\n")

                if #errs > 1 then
                    who:respond(string.format("Error: %s (+%d lines)",
                     errs[1], #errs - 1))
                else
                    who:respond(string.format("Error: %s", errs[1]))
                end
            end
        end
    end)

    luna.add_command("ping", function(who, ...)
        who:respond("pong")
    end)

    luna.add_command("version", function(who, ...)
        who:respond(string.format("Luna++ %s (built %s, %s)",
            luna.shared["luna.version"],
            luna.shared["luna.compiled"],
            luna.shared["luna.compiler"]))
    end)

    luna.add_command("settrigger", "*l", function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find("o") then
            local a, b, target, newtrigger = args:find("(#[^%s]+)%s*(.+)")

            if not a then
                target, newtrigger = where:name(), args
            end

            luna.set_channel_trigger(target, newtrigger)

            if #newtrigger > 0 then
                who:respond(string.format("Set trigger for %q to: %q",
                    target, newtrigger))
            else
                who:respond(
                    string.format("Disabled non-highlight trigger for %q",
                        target))
            end

            luna.save_shared()
        end
    end)

    luna.add_command("unsettrigger", function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find("o") then
            local target = args[1] or where:name()

            luna.set_channel_trigger(target, nil)
            who:respond(string.format("Reset trigger for %q", target))

            luna.save_shared()
        end
    end)

    luna.add_command("getvar", function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find("o") then
            local r = luna.shared[args[1]]

            if r then
                who:respond(string.format("%q -> %q.", args[1], r))
            else
                who:respond(string.format("%q is not set.", args[1]))
            end
        end
    end)

    luna.add_command("setvar", "*l", function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find("o") then
            print(args)
            local a, b, key, val = args:find("([^%s]+)%s*(.*)")

            print(key, val)

            if val and val ~= "" then
                luna.shared[key] = val
                who:respond(string.format("%q -> %q.", key, val))
            else
                luna.shared[key] = nil
                who:respond(string.format("cleared %q.", key))
            end

            luna.save_shared()
        end
    end)

    luna.add_command("reloadusers", function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find("a") then
            luna.users.reload()
            local n = 0

            for _, _ in pairs(luna.users.list()) do
                n = n + 1
            end

            who:respond(string.format("Reloaded %d users", n))
        end
    end)

    luna.add_command("reloadvars", function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find("a") then
            luna.reload_shared()
            local n = 0

            for _, _ in pairs(luna.shared) do
                n = n + 1
            end

            who:respond(string.format("Reloaded %d variables", n))
        end
    end)

    luna.add_command("join", function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find("o") then
            who:respond("OK!")
            luna.join(args[1])
        end
    end)

    luna.add_command("part", function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find("o") then
            who:respond("OK!")
            luna.part(
                args[1] or where:name(),
                "Because " .. who:nick() .. " said so :(")
        end
    end)
end

return base
