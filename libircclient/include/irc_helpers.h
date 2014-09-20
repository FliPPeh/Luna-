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
#ifndef HELPERS_H
#define HELPERS_H

#include "macros.h"

#include <string>

/*! \file
 *  \brief Message creator functions for commonly used messages.
 */

namespace irc {

struct message;



extern DLL_PUBLIC
message pass(std::string password);

extern DLL_PUBLIC
message user(std::string username, std::string mode, std::string realname);

extern DLL_PUBLIC
message nick(std::string nickname);

extern DLL_PUBLIC
message join(std::string channel, std::string key = "");

extern DLL_PUBLIC
message topic(std::string channel, std::string new_topic);

extern DLL_PUBLIC
message mode(std::string channel, char mode);

extern DLL_PUBLIC
message mode(std::string channel, char mode, std::string argument);

extern DLL_PUBLIC
message privmsg(std::string target, std::string message);

extern DLL_PUBLIC
message notice(std::string target, std::string message);

extern DLL_PUBLIC
message response(std::string target, std::string channel, std::string message);

extern DLL_PUBLIC
message ctcp_request(
    std::string target,
    std::string ctcp,
    std::string args = "");

extern DLL_PUBLIC
message ctcp_response(
    std::string target,
    std::string ctcp,
    std::string args = "");

}

#endif // defined HELPERS_H
