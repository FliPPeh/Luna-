local time = {}

function time.is_leap_year(y)
    if y % 4 == 0 then
        if y % 100 == 0 then
            return y % 400 == 0
        else
            return true
        end
    else
        return false
    end
end

function time.month_length(m, y)
    local lens = {
        31, -- Jan
        time.is_leap_year(y) and 29 or 28, -- Feb
        31, -- Mar
        30, -- Apr
        31, -- May
        30, -- Jun
        31, -- Jul
        31, -- Aug
        30, -- Sep
        31, -- Oct
        30, -- Nov
        31, -- Dec
    }

    assert(m >= 1 and m <= 12, "month must be between 1 and 12")

    return lens[m]
end

local function factorize_seconds(s)
    local res = {}
    local facts = {
        31535999.9999648, -- years
        2628000, -- months
        604800, -- weeks
        86400, -- days
        3600, -- hours
        60, -- minutes
        1 -- remaining seconds
    }

    s = math.max(s, 0)

    for i, f in ipairs(facts) do
        table.insert(res, math.floor(f == 1 and s or s / f))
        s = s % f
    end

    return res
end

time.long_names = {
    {" year", " years"},
    {" month", " months"},
    {" week", " weeks"},
    {" day",  " days"},
    {" hour",  " hours"},
    {" minute",  " minutes"},
    {" second",  " seconds"}}

time.short_names = {
    {"yr", "yrs"},
    {"mo", "mos"},
    {"wk", "wks"},
    {"d",  "ds"},
    {"h",  "hrs"},
    {"m",  "mns"},
    {"s",  "s"}}

function time.pretty_print_delta(parts, units, sep)
    local units = units or time.short_names
    local i = 1

    -- Display at least the seconds part, even if 0
    while i < #parts and parts[i] == 0 do
        i = i + 1
    end

    local reprs = {}

    for j = i, #parts do
        table.insert(reprs, string.format("%d%s",
            parts[j],
            (parts[j] == 1) and units[j][1] or units[j][2]))
    end

    return table.concat(reprs, sep or " ")
end

function time.delta(a, b)
    return factorize_seconds(b - a)
end

function time.pretty_delta(a, b, units, sep)
    return time.pretty_print_delta(factorize_seconds(b - a), units, sep)
end
return time
