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

#include "luna_channel_proxy.hh"

#include "luna_user_proxy.hh"

#include "luna.hh"
#include "luna_user.hh"

#include <irc/channel.hh>
#include <irc/channel_user.hh>
#include <irc/irc_utils.hh>
#include <irc/environment.hh>

#include <mond/mond.hh>

#include <lua.hpp>

#include <ctime>
#include <string>
#include <sstream>


///
// Unknown users
luna_unknown_user_proxy::luna_unknown_user_proxy(luna& ref, std::string prefix)
    : _ref{&ref},
      _prefix{std::move(prefix)}
{
}


std::string luna_unknown_user_proxy::repr() const
{
    return std::get<0>(user_info());
}


std::tuple<std::string,
           std::string,
           std::string > luna_unknown_user_proxy::user_info() const
{
    return irc::split_prefix(_prefix);
}

int luna_unknown_user_proxy::match(lua_State* s) const
{
    auto i = std::find_if(std::begin(_ref->users()), std::end(_ref->users()),
        [this] (luna_user& u) {
            return u.matches(_prefix);
        });

    if (i != std::end(_ref->users())) {
        return mond::write(s, mond::object<luna_user_proxy>(*_ref, i->id()));
    }

    return mond::write(s, mond::nil{});
}


///
// Channels
luna_channel_proxy::luna_channel_proxy(luna& ref, std::string name)
    : _ref{&ref},
      _name{std::move(name)}
{
}


std::string luna_channel_proxy::name() const
{
    return lookup().name();
}

std::time_t luna_channel_proxy::created() const
{
    return lookup().created();
}

std::tuple<
    std::string,
    std::string,
    std::time_t> luna_channel_proxy::topic() const
{
    return lookup().topic();
}


int luna_channel_proxy::users(lua_State* s) const
{
    lua_newtable(s);
    int table = lua_gettop(s);

    for (auto const& u : lookup().users()) {
        std::ostringstream prefix;

        prefix << u.second->nick() << '!'
            << u.second->user() << '@'
            << u.second->host();

        mond::write(s,
            mond::object<luna_channel_user_proxy>(
                *_ref, _name, u.second->uid()));

        lua_setfield(s, table, prefix.str().c_str());
    }

    return 1;
}

int luna_channel_proxy::find_user(lua_State* s) const
{
    char const* qry = luaL_checkstring(s, 2);

    auto u = std::find_if(
        std::begin(lookup().users()),
        std::end(lookup().users()),
            [this, qry] (auto const& u2) {
                return irc::rfc1459_equal(
                    irc::normalize_nick(qry), u2.second->nick());
            });

    if (u == std::end(lookup().users())) {
        lua_pushnil(s);
    } else {
        mond::write(s,
            mond::object<luna_channel_user_proxy>(
                *_ref, _name, u->second->uid()));
    }

    return 1;
}


int luna_channel_proxy::modes(lua_State* s) const
{
    /* mode_set['a'] ..., let's waste some space for the sake of simplicity */
    bool mode_set[128] = {false};

    lua_newtable(s);
    int table = lua_gettop(s);

    for (auto& i : lookup().modes()) {
        char mode              = i.first;
        std::string const& arg = i.second;

        std::string modestr{mode};

        irc::channel_mode_argument_type type =
            _ref->environment().get_mode_argument_type(mode);

        if (type == irc::channel_mode_argument_type::required_user_list) {
            // Add to or create list
            if (not mode_set[static_cast<int>(mode)]) {
                mode_set[static_cast<int>(mode)] = true;

                lua_newtable(s);
                lua_pushvalue(s, -1);
                lua_setfield(s, table, modestr.c_str());
            } else {
                lua_getfield(s, table, modestr.c_str());
            }

            lua_pushstring(s, arg.c_str());
            lua_rawseti(s, -2, lua_rawlen(s, -2) + 1);

            lua_pop(s, 1);
        } else {
            // Set
            lua_pushstring(s, arg.c_str());
            lua_setfield(s, table, modestr.c_str());
        }
    }

    return 1;
}


irc::channel& luna_channel_proxy::lookup() const
{
    auto& channels = _ref->environment().channels();

    if (channels.find(_name) == std::end(channels)) {
        throw mond::error{"no such channel: " + _name};
    }

    return *channels.at(_name);
}


///
// Known channel users
luna_channel_user_proxy::luna_channel_user_proxy(
    luna& ref,
    std::string channel,
    uint64_t uid)

    : _ref{&ref},
      _channel{std::move(channel)},
      _uid{uid}
{
}


std::string luna_channel_user_proxy::repr() const
{
    return std::get<0>(user_info());
}


std::tuple<
    std::string,
    std::string,
    std::string> luna_channel_user_proxy::user_info() const
{
    irc::channel_user const& user = lookup();

    return std::make_tuple(user.nick(), user.user(), user.host());
}


std::string luna_channel_user_proxy::modes() const
{
    return lookup().modes();
}


int luna_channel_user_proxy::channel(lua_State* s) const
{
    return mond::write(s, mond::object<luna_channel_proxy>(*_ref, _channel));
}


int luna_channel_user_proxy::match(lua_State* s) const
{
    std::ostringstream pref;

    irc::channel_user const& cu = lookup();

    pref << cu.nick() << '!' << cu.user() << '@' << cu.host();

    auto i = std::find_if(std::begin(_ref->users()), std::end(_ref->users()),
        [&pref] (luna_user& u) {
            return u.matches(pref.str());
        });

    if (i != std::end(_ref->users())) {
        return mond::write(s, mond::object<luna_user_proxy>(*_ref, i->id()));
    }

    return mond::write(s, mond::nil{});
}



irc::channel_user& luna_channel_user_proxy::lookup() const
{
    auto& channels = _ref->environment().channels();

    if (channels.find(_channel) == std::end(channels)) {
        throw mond::error{"no such channel: " + _channel};
    }

    for (auto& iter : channels.at(_channel)->users()) {
        if (iter.second->uid() == _uid) {
            return *(iter.second);
        }
    }

    std::ostringstream err;

    err << "user " << _uid << " not found in channel " << _channel;
    throw mond::error{err.str()};
}

