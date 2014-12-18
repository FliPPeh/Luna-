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

#include "luna_user_proxy.hh"

#include "config.hh"
#include "luna.hh"
#include "luna_user.hh"

#include <mond/mond.hh>

#include <lua.hpp>

#include <string>
#include <utility>


luna_user_proxy::luna_user_proxy(luna& ref, std::string id)
    : _ref{&ref},
      _id{std::move(id)}
{
}


std::string luna_user_proxy::id() const
{
    return lookup().id();
}

std::string luna_user_proxy::hostmask() const
{
    return lookup().hostmask();
}

std::string luna_user_proxy::title() const
{
    return lookup().title();
}

std::string luna_user_proxy::flags() const
{
    return flags_to_string(lookup().flags());
}


void luna_user_proxy::set_id(std::string id)
{
    luna_user& user = lookup();

    user.set_id(id);
    _id = id;
    _ref->save_users(userfile);
}

void luna_user_proxy::set_hostmask(std::string hostmask)
{
    luna_user& user = lookup();

    user.set_hostmask(hostmask);
    _ref->save_users(userfile);
}

void luna_user_proxy::set_title(std::string title)
{
    luna_user& user = lookup();

    user.set_title(title);
    _ref->save_users(userfile);
}

void luna_user_proxy::set_flags(std::string flags)
{
    luna_user& user = lookup();

    user.set_flags(flags_from_string(flags));
    _ref->save_users(userfile);
}


luna_user& luna_user_proxy::lookup() const
{
    auto u = std::find_if(std::begin(_ref->users()), std::end(_ref->users()),
        [this] (luna_user& u2) {
            return u2.id() == _id;
        });

    if (u == std::end(_ref->users())) {
        throw mond::error{"no such user: " + _id};
    }

    return *u;
}
