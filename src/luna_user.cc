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

#include "luna_user.h"

#include <string>
#include <regex>

luna_user::luna_user(
    std::string id,
    std::string hostmask,
    std::string title,
    int flags)

    : _id{id},
      _hostmask{hostmask},
      _title{title},
      _flags{flags}
{
}

std::string luna_user::id() const
{
    return _id;
}

std::string luna_user::hostmask() const
{
    return _hostmask;
}

std::string luna_user::title() const
{
    return _title;
}

int luna_user::flags() const
{
    return _flags;
}


void luna_user::set_id(std::string newid)
{
    _id = newid;
}

void luna_user::set_hostmask(std::string newhostmask)
{
    _hostmask = newhostmask;
}

void luna_user::set_title(std::string newtitle)
{
    _title = newtitle;
}

void luna_user::set_flags(int newflags)
{
    _flags = newflags;
}


bool luna_user::matches(const std::string& prefix) const
{
    try {
        return std::regex_search(prefix, std::regex{_hostmask});
    } catch (std::regex_error const& e) {
        return false;
    }
}


bool luna_user::operator==(luna_user const& other) const
{
    return _id == other._id;
}


bool luna_user::operator!=(luna_user const& other) const
{
    return !(*this == other);
}


int flags_from_string(std::string const& flags)
{
    return
        ((flags.find('i') != std::string::npos) ? luna_user::flag_ignore : 0)
      | ((flags.find('f') != std::string::npos) ? luna_user::flag_friend : 0)
      | ((flags.find('o') != std::string::npos) ? luna_user::flag_oper   : 0)
      | ((flags.find('a') != std::string::npos) ? luna_user::flag_owner  : 0);
}

std::string flags_to_string(int flags)
{
    std::ostringstream strf;

    strf << ((flags & luna_user::flag_ignore) ? "i" : "")
         << ((flags & luna_user::flag_friend) ? "f" : "")
         << ((flags & luna_user::flag_oper)   ? "o" : "")
         << ((flags & luna_user::flag_owner)  ? "a" : "");

    return strf.str();
}


