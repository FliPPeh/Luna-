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

#pragma once
#ifndef LUNA_LUA_LUNA_EXTENSION_PROXY_HH_INCLUDED
#define LUNA_LUA_LUNA_EXTENSION_PROXY_HH_INCLUDED

#include <lua.hpp>

#include <string>

class luna;
class luna_extension;

class luna_extension_proxy {
public:
    static constexpr char const* metatable = "luna.ext";

    luna_extension_proxy(luna& ref, std::string id);

    std::string id() const;

    std::string name() const;
    std::string description() const;
    std::string version() const;

    int is_self(lua_State* s) const;

private:
    luna_extension& lookup() const;

private:
    luna* _ref;
    std::string _id;
};

#endif // defined LUNA_LUA_LUNA_EXTENSION_PROXY_HH_INCLUDED
