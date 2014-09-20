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
#ifndef LUNA_PROXY_H
#define LUNA_PROXY_H

#include <lua.hpp>

#include <cstddef>
#include <ctime>

#include <tuple>
#include <string>

class luna;

class luna_proxy {
public:
    static constexpr char const* metatable = "luna";

    luna_proxy(luna& ref);

    std::tuple<std::time_t, std::time_t>          runtime_info() const;
    std::tuple<std::string, std::string>             user_info() const;
    std::tuple<std::string, std::string, uint16_t> server_info() const;

    std::tuple<
        std::size_t,
        std::size_t,
        std::size_t,
        std::size_t> traffic_info() const;

    int send_message(lua_State* s);

private:
    luna* _ref;
};

#endif // defined LUNA_PROXY_H
