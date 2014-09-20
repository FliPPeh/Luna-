/*
 * Copyright 2014 Lukas Niederbremer
 *
 * This file is part of libircclient.
 *
 * libircclient is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * libircclient is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libircclient.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "irc_helpers.h"

#include "irc_core.h"
#include "irc_utils.h"

#include <sstream>
#include <string>

namespace irc {


message pass(std::string password)
{
    return message{"", command::PASS, {std::move(password)}};
}

message user(std::string username, std::string mode, std::string realname)
{
    return message{"", command::USER, {
        std::move(username),
        std::move(mode),
        "*",
        std::move(realname)
    }};
}

message nick(std::string nickname)
{
    return message{"", command::NICK, {std::move(nickname)}};
}

message join(std::string channel, std::string key)
{
    if (key.empty()) {
        return message{"", command::JOIN, {std::move(channel)}};
    } else {
        return message{"", command::JOIN, {std::move(channel), std::move(key)}};
    }
}

message topic(std::string channel, std::string new_topic)
{
    return message{"", command::TOPIC, {std::move(new_topic)}};
}

message mode(std::string channel, char mode)
{
    return message{"", command::MODE, {}};
}

message mode(std::string channel, char mode, std::string argument)
{
    return message{"", command::MODE, {std::move(argument)}};
}

message privmsg(std::string target, std::string msg)
{
    return message{"", command::PRIVMSG, {std::move(target), std::move(msg)}};
}

message notice(std::string target, std::string msg)
{
    return message{"", command::NOTICE, {std::move(target), std::move(msg)}};
}

message response(std::string target, std::string channel, std::string msg)
{
    return message{"", command::PRIVMSG,
        {std::move(channel),
         normalize_nick(target) + ": " + msg}};
}

message ctcp_request(
    std::string target,
    std::string ctcp,
    std::string args)
{
    std::ostringstream cmd;

    cmd << '\x01' << ctcp;

    if (not args.empty()) {
        cmd << " " << args;
    }

    cmd << '\x01';

    return message{"", command::PRIVMSG, {std::move(target), cmd.str()}};
}

message ctcp_response(
    std::string target,
    std::string ctcp,
    std::string args)
{
    std::ostringstream cmd;

    cmd << '\x01' << ctcp;

    if (not args.empty()) {
        cmd << " " << args;
    }

    cmd << '\x01';

    return message{"", command::NOTICE, {std::move(target), cmd.str()}};
}


}
