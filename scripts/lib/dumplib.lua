----
-- Dumper library for easy dumping of Lua values
-- and basic serialization to and from files.
--
local dump = {}

-- Determine if a table is an array
local function is_array(t)
    local nkeys = 0
    for k, v in pairs(t) do
        nkeys = nkeys + 1
    end

    local nidx = 0
    for i, v in ipairs(t) do
        nidx = nidx + 1
    end

    return nkeys == nidx
end

function dump.make_dumper(args)
    local args = args or {}

    local indent_width = args.indent_width or 3
    local indent_char  = args.indent_char  or ' '
    local indent_level = 0
    local depth        = args.depth  or math.huge
    local inline       = args.inline or false

    local indent = ''

    local d = depth

    local ignore = {}


    local function dumper(v)
        if     type(v) == 'number'
            or type(v) == 'boolean'
            or type(v) == 'nil' then

                return tostring(v)

        elseif type(v) == 'function' then
            return '<' .. tostring(v) .. '>'

        elseif type(v) == 'string' then
            return string.format('%q', v)

        elseif type(v) == 'table' then
            for _, val in ipairs(ignore) do
                if v == val then
                    return '<recursion: ' .. tostring(v) .. '>'
                end
            end

            table.insert(ignore, v)


            local list = is_array(v)

            depth = depth - 1
            if inline then
                local dump = '{ '

                if depth > 0 then
                    for k, v in pairs(v) do
                        if list then
                            dump = dump .. string.format('%s, ', dumper(v))
                        else
                            dump = dump .. string.format('%s = %s, ',
                                k, dumper(v))
                        end
                    end
                else
                    dump = dump .. '... '
                end

                depth = depth + 1
                return dump .. '}'
            else
                local dump = '{\n'

                indent_level = indent_level + 1
                indent = string.rep(indent_char, indent_width * indent_level)

                if depth > 0 then
                    for k, v in pairs(v) do
                        if list then
                            dump = dump .. string.format('%s%s,\n',
                                indent, dumper(v))
                        else
                            dump = dump .. string.format('%s%s = %s,\n',
                                indent, k, dumper(v))
                        end
                    end
                else
                    dump = dump .. indent .. '...\n'
                end

                indent_level = indent_level - 1
                indent = string.rep(indent_char, indent_width * indent_level)

                depth = depth + 1
                return dump .. indent .. '}'
            end
        else
            return '<unknown: ' .. type(v) .. '>'
        end
    end

    return dumper
end

dump.dump        = dump.make_dumper{}
dump.dump_inline = dump.make_dumper{inline = true}

----
-- File serializer, save and restore lua values to and from files.
----
dump.file_serializer = {}
dump.file_serializer_meta = { __index = dump.file_serializer }

function dump.file_serializer.new(file)
    return setmetatable({ file = file }, dump.file_serializer_meta)
end

function dump.file_serializer:write(v)
    d = dump.make_dumper{inline = false}

    f = io.open(self.file, 'w')
    f:write('return ' .. d(v) .. '\n')
    f:close()
end

function dump.file_serializer:read()
    ok, ret = pcall(dofile, self.file)
    if ok then
        return ret
    else
        error(ret)
    end
end

return dump
