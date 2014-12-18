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
#ifndef LUNA_LUA_LUNA_CHANNEL_PROXY_HH_INCLUDED
#define LUNA_LUA_LUNA_CHANNEL_PROXY_HH_INCLUDED

#include <lua.hpp>

#include <ctime>
#include <cstdint>

#include <string>

class luna;

namespace irc {
    class channel;
    class channel_user;
}


class luna_unknown_user_proxy {
public:
    static constexpr char const* metatable = "luna.unknown_user";

    luna_unknown_user_proxy(luna& ref, std::string prefix);

    std::string repr() const;

    std::tuple<std::string, std::string, std::string> user_info() const;
    int match(lua_State* s) const;

private:
    luna* _ref;

    std::string _prefix;
};


class luna_channel_proxy {
public:
    static constexpr char const* metatable = "luna.channel";

    luna_channel_proxy(luna& ref, std::string name);

    std::string name() const;
    std::time_t created() const;
    std::tuple<
        std::string,
        std::string,
        std::time_t> topic() const;

    int     users(lua_State* s) const;
    int find_user(lua_State* s) const;

    int modes(lua_State* s) const;

private:
    irc::channel& lookup() const;

private:
    luna* _ref;
    std::string _name;
};


class luna_channel_user_proxy {
public:
    static constexpr char const* metatable = "luna.channel.user";

    luna_channel_user_proxy(luna& ref, std::string channel, uint64_t uid);

    std::string repr() const;

    std::tuple<std::string, std::string, std::string> user_info() const;
    int match(lua_State* s) const;

    std::string modes() const;
    int channel(lua_State* s) const;


private:
    irc::channel_user& lookup() const;

private:
    luna* _ref;

    std::string _channel;
    uint64_t _uid;

};

#endif // defined LUNA_LUA_LUNA_CHANNEL_PROXY_HH_INCLUDED
