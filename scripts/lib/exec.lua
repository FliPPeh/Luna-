local M = {}

local function mkcmdline(args)
    local program = table.remove(args, 1):gsub(' ', '\\ ')

    for i, arg in ipairs(args) do
        args[i] = "'" .. arg:gsub("'", "\\'") .. "'"
    end

    return table.concat({program, table.concat(args, ' ')}, ' ')
end

local exec_meta = {
    __index = {
        read  = function(self, arg) return self.fp:read(arg) end,
        write = function(self, ...) return self.fp:write(...) end,
        lines = function(self) return self.fp:lines() end,
        exit_code = function(self)
            local done = tonumber(
                io.popen(string.format('%s; echo $?',
                    mkcmdline{'test', '-f', self.tmpfile})):read('*l')) == 0

            if done then
                return tonumber(io.open(self.tmpfile):read('*l'))
            else
                return nil
            end
        end,
    },

    __gc = function(self)
        if self.tmpfile then
            os.remove(self.tmpfile)
        end
    end
}

function M.exec(args, mode)
    local tmpfile = os.tmpname()
    local cmdline = string.format('%s; echo $? >%s', mkcmdline(args), tmpfile)

    mode = mode or 'r'

    return setmetatable({
        fp      = io.popen(cmdline, mode),
        tmpfile = tmpfile
    }, exec_meta)
end

return M
