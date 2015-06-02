luna.util = {}

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
        irc  = {'*',   '*' },
        ecma = {'.+?', '.?'},
        lua  = {'.-',  '.?'},
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

    return (rv .. ' ' .. table.concat(seta,   ' ')
               .. ' ' .. table.concat(unseta, ' ')):split(" ")
end

-- Augment core types for fun and profit
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
-- Replacements can also take the form of "${lua:expr}", where "expr" is any
-- valid Lua expression that will be evaluated, e.g. "${lua: return 1 + 4}"
-- will yield "5".
--
-- Replacements can be escaped with "$${VAR}".
--
-- If a variable can not be located, it is replaced with the value of "rep"
-- unless a special replacement is specified ("${VAR/<VAR not found>")
--
-- The "enable" parameter controls which kinds of replacements are enabled,
-- where 'e' = environment, 'v' = shared variables, 'l' = Lua expressions.
-- By default it allows all replacements.
--
-- Due to the information that can be queried with this function, and the
-- support for evaluating arbitrary expressions, it should naturally not be
-- run on untrusted input unless the "enable" field is restricted.
function string:template(fmt, rep, enable)
    rep = rep or '(?)'
    enable = enable or "evl" -- env, shared vars, lua expr

    return self:gsub("(.?)($%b{})", function(p, m)
        local k = m:sub(3, #m - 1)

        p = p or ''

        if p == '$' then
            return m
        end

        local i = k:find('/')
        if i and not k:find('^lua:') then
            rep = k:sub(i + 1)
            k = k:sub(1, i)
        end

        if k:find('^env:') and enable:find('e') then
            return p .. (os.getenv(k:sub(5):trim()) or rep)

        elseif k:find('^var:') and enable:find('v') then
            return p .. (luna.shared[k:sub(5):trim()] or rep)

        elseif k:find('^lua:') and enable:find('l') then
            return p .. tostring(load(k:sub(5))() or nil)
        elseif fmt[k] then
            return p .. fmt[k]
        else
            return p .. rep
        end
    end)
end

function string:capitalize()
    return self:sub(1, 1):upper() .. self:sub(2)
end

function string:titlecase()
    return ({self:gsub("(%w+)", function(w) return w:capitalize() end)})[1]
end

function string:trim()
    return select(3, self:find("^%s*(.-)%s*$"))
end

return luna.util
