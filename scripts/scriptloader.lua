local scriptloader = {}

scriptloader.info = {
    name        = 'Script Loader',
    description = 'Load, unload, and reload scripts',
    version     = '1.0'
}


local function truncate(msg)
    local lines = msg:split('\n')

    if #lines > 1 then
        return string.format('%s ... (+%d lines)', lines[1], #lines - 1)
    else
        return msg
    end
end

function scriptloader.script_load()
    luna.add_command('load', function(who, where, what, args)
        local user = who:match_reguser()

        if user and user:flags():find('a') then
            if not args[1] then
                who:respond('usage: load <script>')
            else
                local status, res = pcall(luna.scripts.load, args[1])

                if status then
                    who:respond(string.format('Loaded script: %q.', res:name()))
                else
                    log.err(res)

                    who:respond('Error: ' .. truncate(res) .. ' :(')
                end
            end
        end
    end)

    luna.add_command('unload', function(who, where, what, args)
        local user = who:match_reguser()

        if user and user:flags():find('a') then
            if not args[1] then
                who:respond('usage: unload <script>')
            else
                local status, res = pcall(luna.scripts.unload, args[1])

                if status then
                    who:respond(string.format('Unloaded script: %q.', args[1]))
                else
                    log.err(res)

                    who:respond('Error: ' .. truncate(res) .. ' :(')
                end
            end
        end
    end)

    luna.add_command('reload', function(who, where, what, args)
        local user = who:match_reguser()

        if user and user:flags():find('a') then
            if not args[1] then
                who:respond('usage: reload <script>')
            else
                local status, res = pcall(function()
                    luna.scripts.unload(args[1])
                    return luna.scripts.load(args[1])
                end)

                if status then
                    who:respond(string.format('Reloaded script: %q.',
                        res:name()))
                else
                    log.err(res)

                    who:respond('Error: ' .. truncate(res) .. ' :(')
                end
            end
        end
    end)
end

return scriptloader
