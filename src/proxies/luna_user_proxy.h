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
#ifndef LUNA_USER_PROXY_H
#define LUNA_USER_PROXY_H

#include <string>

class luna;
class luna_user;

class luna_user_proxy {
public:
    static constexpr char const* metatable = "luna.user";

    luna_user_proxy(luna& ref, std::string id);

    std::string id() const;
    std::string hostmask() const;
    std::string title() const;
    std::string flags() const;

    void set_id(std::string id);
    void set_hostmask(std::string hostmask);
    void set_title(std::string title);
    void set_flags(std::string flags);

private:
    luna_user& lookup() const;

private:
    luna* _ref;
    std::string _id;
};

#endif // defined LUNA_USER_PROXY_H
