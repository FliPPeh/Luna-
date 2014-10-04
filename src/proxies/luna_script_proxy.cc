/*
 * Copyright 2014 Lukas Niederbremer
 *
 * This file is part of Luna++.
 *
 * Luna++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Luna++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Luna++.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "luna_script_proxy.h"

#include "luna.h"
#include "luna_script.h"

#include <string>
#include <utility>

luna_script_proxy::luna_script_proxy(luna& ref, std::string fname)
    : _ref{&ref},
      _fname{std::move(fname)}
{
}


std::string luna_script_proxy::file() const
{
    return lookup().file();
}


std::string luna_script_proxy::name() const
{
    return lookup().name();
}

std::string luna_script_proxy::description() const
{
    return lookup().description();
}

std::string luna_script_proxy::version() const
{
    return lookup().version();
}


int luna_script_proxy::is_self(lua_State* s) const
{
    lua_pushboolean(s, s == static_cast<lua_State*>(lookup()._lua));
    return 1;
}


luna_script& luna_script_proxy::lookup() const
{
    auto s = std::find_if(
        std::begin(_ref->scripts()), std::end(_ref->scripts()),
        [this] (std::unique_ptr<luna_script> const& s2) {
            return s2 and (s2->file() == _fname);
        });

    if (s == std::end(_ref->scripts())) {
        throw mond::error{"no such script: " + _fname};
    }

    return **s;

}
