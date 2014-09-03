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

#include "core.h"
#include "irc_utils.h"
#include "util.h"

#include <algorithm>
#include <sstream>
#include <iostream>
#include <utility>

namespace irc {

std::tuple<std::string, std::string, std::string> split_prefix(
    std::string const& prefix)
{
    if (not is_user_prefix(prefix)) {
        throw protocol_error{protocol_error_type::invalid_prefix, prefix};
    }

    std::string nick = prefix.substr(0, prefix.find('!'));
    std::string user = prefix.substr(prefix.find('!') + 1,
                                prefix.find('@') - prefix.find('!') - 1);

    std::string host = prefix.substr(prefix.find('@') + 1);

    return std::make_tuple(nick, user, host);
}

bool rfc1459_equal(std::string const& a, std::string const& b)
{
    if (a.size() != b.size()) {
        return false;
    }

    for (std::size_t i = 0; i < a.size(); ++i) {
        if (rfc1459_lower(a[i]) != rfc1459_lower(b[i])) {
            return false;
        }
    }

    return true;
}

std::string rfc1459_lower(std::string const& str)
{
    std::string out;

    out.resize(str.size());

    transform(begin(str), end(str), begin(out), [](char c) {
         return rfc1459_lower(c);
     });

    return out;
}

std::string rfc1459_upper(std::string const& str)
{
    std::string out;

    out.resize(str.size());

    transform(begin(str), end(str), begin(out), [](char c) {
        return rfc1459_upper(c);
     });

    return out;
}


message pass(std::string password)
{
    return message{"", command::PASS, {std::move(password)}};
}

message user(std::string username, std::string mode, std::string realname)
{
    return message{"", command::USER, {
        std::move(username),
        std::move(mode),
        std::move("*"),
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
