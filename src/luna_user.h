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

#ifndef LUNA_USER_H
#define LUNA_USER_H

#include <string>

class luna_user {
public:
    enum flag {
        flag_ignore = 0x01, // i
        flag_friend = 0x02, // f
        flag_oper   = 0x04, // o
        flag_owner  = 0x08  // a
    };

    luna_user(
        std::string id,
        std::string hostmask,
        std::string title,
        int flags);

    std::string id()       const;
    std::string hostmask() const;
    std::string title()    const;
    int         flags()    const;

    void set_id(std::string newid);
    void set_hostmask(std::string newhostmask);
    void set_title(std::string newtitle);
    void set_flags(int newflags);

    bool matches(std::string const& prefix) const;

    bool operator==(luna_user const& other) const;
    bool operator!=(luna_user const& other) const;

private:
    std::string _id;
    std::string _hostmask;
    std::string _title;
    int         _flags;
};

int flags_from_string(std::string const& flags);
std::string flags_to_string(int flags);

#endif // defined LUNA_USER_H
