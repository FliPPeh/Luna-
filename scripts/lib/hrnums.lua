local hrnums = {}

--[[
-- Metric SI unit prefixes
--]]
local si_prefixes = {
    {1e-24, 'y', 'yocto'},
    {1e-21, 'z', 'zepto'},
    {1e-18, 'a', 'atto' },
    {1e-15, 'f', 'femto'},
    {1e-12, 'p', 'pico' },
    {1e-9,  'n', 'nano' },
    {1e-6,  'µ', 'micro'},
    {1e-3,  'm', 'milli'},
    {1e0,   '',  '',    },
    {1e3,   'k', 'kilo' },
    {1e6,   'M', 'mega' },
    {1e9,   'G', 'giga' },
    {1e12,  'T', 'tera' },
    {1e15,  'P', 'peta' },
    {1e18,  'E', 'exa'  },
    {1e21,  'Z', 'zetta'},
    {1e24,  'Y', 'yotta'},
}

--[[
-- Binary metric SI unit prefixes
--]]
local binary_si_prefixes = {
    {math.pow(1024, 0), '',  '',   },
    {math.pow(1024, 1), 'K', 'kibi'},
    {math.pow(1024, 2), 'M', 'mebi'},
    {math.pow(1024, 3), 'G', 'gibi'},
    {math.pow(1024, 4), 'T', 'tebi'},
    {math.pow(1024, 5), 'P', 'pebo'},
    {math.pow(1024, 6), 'E', 'exbi'},
    {math.pow(1024, 7), 'Z', 'zebi'},
    {math.pow(1024, 8), 'Y', 'yobi'},
}

--[[
-- Minimize a number and return it, along with the according SI prefixes
--]]
function hrnums.si_factorize(n)
    -- start in the middle at factor
    local i = math.ceil(#si_prefixes / 2)

    local sign = n < 0 and -1 or 1
    local n  = math.abs(n)

    -- Only a fraction? Go down the prefix list, make number bigger
    if n < 1 then
        while i > 1 and (n / si_prefixes[i][1]) < 1 do
            i = i - 1
        end
    else
        -- Go up the prefix list, make number smaller
        while i < #si_prefixes and (n / si_prefixes[i][1]) >= 1000 do
            i = i + 1
        end
    end

    return n / si_prefixes[i][1] * sign,
               si_prefixes[i][2],
               si_prefixes[i][3],
               si_prefixes[i][1]
end

--[[
-- Minimize a number and return it, along with the according binary SI prefixes
--]]
function hrnums.si_binary_factorize(n)
    -- start at factor 1
    local i = 1

    local sign = n < 0 and -1 or 1
    local n  = math.abs(n)

    if n < 1 then
        error('number must be at least 1', 2)
    end

    -- Go up the prefix list, make number smaller
    while i < #binary_si_prefixes and (n / binary_si_prefixes[i][1]) >= 1024 do
        i = i + 1
    end

    return n / binary_si_prefixes[i][1] * sign,
               binary_si_prefixes[i][2],
               binary_si_prefixes[i][3],
               binary_si_prefixes[i][1]
end

hrnums.default_opts = {
    number_separator  = ',',
    decimal_separator = '.',
    decimal_precision = 2
}

--[[
-- Pretty-formats a number according to the passed in (or default) options,
-- with thousands-separators and decimal precision
--]]
function hrnums.humanize_number(num, opts)
    local parts = {}
    local numstr = tostring(num)

    local dec = ''
    local frac = ''

    local opts = opts or hrnums.default_opts

    local numsep = opts.number_separator  or ','
    local decsep = opts.decimal_separator or '.'
    local decpre = opts.decimal_precision or 2

    local sign = num < 0 and '-' or ''

    if numstr:find('%.') then
        dec  = numstr:sub(1, numstr:find('%.') - 1)

        fracnum = tonumber('0.' .. numstr:sub(numstr:find('%.') + 1, #numstr))
        frac = string.format('%.' .. tostring(decpre) .. 'f', fracnum):sub(3)
    else
        dec = numstr
        frac = string.rep('0', decpre)
    end

    for part in dec:reverse():gmatch('%d%d?%d?') do
        table.insert(parts, 1, part:reverse())
    end

    return table.concat({sign .. table.concat(parts, numsep), frac}, decsep)
end

--[[
-- Pretty-formats a binary number and prefixes the unit with the
-- binary SI prefix
--]]
function hrnums.humanize_binary(num, unit, opts)
    opts = opts or hrnums.default_opts
    local long = opts.long_prefix or false

    local rnum, pref, longpref, fac = hrnums.si_binary_factorize(num)
    local res = hrnums.humanize_number(rnum, opts)

    if long then
        return res, longpref .. unit, fac
    else
        return res, (fac > 1) and (pref .. "i" .. unit) or unit, fac
    end
end

--[[
-- Pretty-formats a number in metric units according to how a human would read
-- and write them, e.g. tons instead of megagrams or kilometers instead of
-- megameters.
--]]
function hrnums.humanize_metric(num, unit, opts)
    opts = opts or hrnums.default_opts
    local long = opts.long_prefix or false

    local props = {
        ['g']  = { map = { ['kg'] = 1000,
                           ["t"]  = 1000*1000 } },

        ['m']  = { map = { ['cm'] = 0.01,
                         --['dm'] = 0.10,
                           ['m']  = 1.00,
                           ['km'] = 1000      } },

        ['l']  = { map = { ['cl'] = 0.01,
                           ['dl'] = 0.10,
                           ['l']  = 1.00      } },

        -- Don't prefix °C at all
        ['°C'] = { map = { ['°C'] = 0         } },
    }

    local function map_unit(num, unit, map)
        local factor  = 0
        local newunit = unit
        local found   = false

        for unit, from in pairs(map) do
            if num >= from and factor <= from then
                factor = from == 0 and 1 or from
                newunit = unit

                found = true
            end
        end

        if not found then
            local snum, sunit, slunit, fac = hrnums.si_factorize(num)

            return snum, (long and slunit or sunit) .. unit, fac
        end

        return num / factor, newunit, factor
    end

    local fac
    local snum, sunit, slunit

    if props[unit] and props[unit].map then
        local sign = num < 0 and -1 or 1

        num, unit, fac = map_unit(math.abs(num), unit, props[unit].map)
        num = num * sign
    else
        snum, sunit, slunit, fac = hrnums.si_factorize(num)
        num, unit = snum, (long and slunit or sunit) .. unit
    end

    return hrnums.humanize_number(num, opts), unit, fac
end

--[[
-- Convenience function for humanizing imperial united numbers, simply returns
-- the unit alongside it
--]]
function hrnums.humanize_imperial(num, unit, opts)
    return hrnums.humanize_number(num, opts), unit, 1
end

return hrnums
