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

#include "luna_extension_proxy.hh"

#include "luna.hh"
#include "lua/luna_script.hh"

#include <string>
#include <utility>

luna_extension_proxy::luna_extension_proxy(luna& ref, std::string id)
    : _ref{&ref},
      _id{std::move(id)}
{
}


std::string luna_extension_proxy::id() const
{
    return lookup().id();
}


std::string luna_extension_proxy::name() const
{
    return lookup().name();
}

std::string luna_extension_proxy::description() const
{
    return lookup().description();
}

std::string luna_extension_proxy::version() const
{
    return lookup().version();
}


int luna_extension_proxy::is_self(lua_State* s) const
{
    luna_extension const* ext = &lookup();

    if (luna_script const* scr = dynamic_cast<luna_script const*>(ext)) {
        lua_pushboolean(s, s == static_cast<lua_State const*>(scr->_lua));
    } else {
        lua_pushboolean(s, false);
    }

    return 1;
}


luna_extension& luna_extension_proxy::lookup() const
{
    auto s = std::find_if(
        std::begin(_ref->extensions()), std::end(_ref->extensions()),
        [this] (std::unique_ptr<luna_extension> const& s2) {
            return s2 and (s2->id() == _id);
        });

    if (s == std::end(_ref->extensions())) {
        throw mond::error{"no such extension: " + _id};
    }

    return **s;

}
