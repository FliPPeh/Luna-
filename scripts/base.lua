local base = {}

base.info = {
    name         = 'Base',
    description  = 'Do basey things',
    version      = '0.1'
}


function base.script_load()
    local nsusr = luna.shared['base.nickserv.user']
    local nspwd = luna.shared['base.nickserv.passwd']

    if nsusr and nspwd then
        luna.add_signal_handler('connect', function()
            luna.send_message('NICKSERV',
                string.format('identify %s %s', nsusr, nspwd))
        end)
    end


    luna.add_command('eval', '*l', function(who, where, what, args)
        local user = who:match_reguser()
        if user and user:flags():find('a') then
            local f = loadstring(args)

            local ok, r = pcall(f)

            if ok then
                who:respond(tostring(r))
            else
                log.err(r)

                local errs = tostring(r):split('\n')

                if #errs > 1 then
                    who:respond(string.format('Error: %s (+%d lines)',
                     errs[1], #errs - 1))
                else
                    who:respond(string.format('Error: %s', errs[1]))
                end
            end
        end
    end)

    luna.add_command('ping', function(who, ...)
        who:respond('pong')
    end)

    luna.add_command('version', function(who, ...)
        who:respond(string.format('Luna++ %s (built %s, %s)',
            luna.shared['luna.version'],
            luna.shared['luna.compiled'],
            luna.shared['luna.compiler']))
    end)

    luna.add_command('getvar', function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find('o') then
            local r = luna.shared[args[1]]

            if r then
                who:respond(string.format('%q -> %q.', args[1], r))
            else
                who:respond(string.format('%q is not set.', args[1]))
            end
        end
    end)

    luna.add_command('setvar', '*l', function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find('o') then
            print(args)
            local a, b, key, val = args:find('([^%s]+)%s*(.*)')

            print(key, val)

            if val and val ~= '' then
                luna.shared[key] = val
                who:respond(string.format('%q -> %q.', key, val))
            else
                luna.shared[key] = nil
                who:respond(string.format('cleared %q.', key))
            end

            luna.save_shared()
        end
    end)

    luna.add_command('join', function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find('o') then
            who:respond('OK!')
            luna.join(args[1])
        end
    end)

    luna.add_command('part', function(who, where, what, args)
        local u = who:match_reguser()

        if u and u:flags():find('o') then
            who:respond('OK!')
            luna.part(
                args[1] or where:name(),
                'Because ' .. who:nick() .. ' said so :(')
        end
    end)
end

return base
