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

#include "core.h"
#include "irc_utils.h"

#include <cstddef>

#include <algorithm>
#include <sstream>

namespace irc {

static char const* command_names[command::COMMAND_MAX] = {
    // 3.1
    "PASS", "NICK", "USER", "OPER", "SERVICE", "QUIT", "SQUIT",

    // 3.2
    "JOIN", "PART", "MODE", "TOPIC", "NAMES", "LIST", "INVITE", "KICK",
    // 3.3
    "PRIVMSG", "NOTICE",

    // 3.4
    "MOTD", "LUSERS", "VERSION", "STATS", "LINKS", "TIME",
    "CONNECT", "TRACE", "ADMIN", "INFO",

    // 3.5
    "SERVLIST", "SQUERY",

    // 3.6
    "WHO", "WHOIS", "WHOWAS",

    // 3.7
    "KILL", "PING", "PONG", "ERROR",

    // 4.0
    "AWAY", "REHASH", "DIE", "RESTART", "SUMMON", "USERS", "WALLOPS",
    "USERHOST", "ISON"
};


std::string to_string(enum command cmd)
{
    std::ostringstream strm;

    if (is_numeric(cmd)) {
        strm.fill('0');
        strm.width(3);

        strm << static_cast<int>(cmd);
    } else {
        strm << command_names[cmd - ERR_LAST_ERR_MSG - 1];
    }

    return strm.str();
}

enum command command_from_string(std::string const& cmd)
{
    bool is_numeric = (cmd.size() == 3) // "000" - "999"
       and (cmd.find_first_not_of("0123456789") == std::string::npos);

    if (is_numeric) {
        return static_cast<enum command>(std::stoi(cmd));
    } else {
        for (std::size_t i = ERR_LAST_ERR_MSG + 1;
                         i < command::COMMAND_MAX;
                       ++i) {
            enum command rcmd = static_cast<enum command>(i);

            if (cmd == to_string(rcmd)) {
                return rcmd;
            }
        }
    }

    throw protocol_error{protocol_error_type::invalid_command, cmd};
}

std::string to_string(message const& msg)
{
    std::ostringstream strm;

    if (!msg.prefix.empty()) {
        strm << ':' << msg.prefix << ' ';
    }

    strm << msg.command;

    for (std::string const& param : msg.args) {
        strm << ' ';

        if (param.find(' ') != std::string::npos) {
            strm << ':';
        }

        strm << param;
    }

    return strm.str();
}


message message_from_string(std::string str)
{
    std::string msgcpy{str};
    message msg;

    // Preserve spaces in the trailing message, split it out now if it exists.
    std::size_t trailpos;
    std::string trail;

    if ((trailpos = str.find(" :")) != std::string::npos) {
        trail = str.substr(trailpos + 2);
        str.erase(trailpos);
    }

    // command, [args...]
    msg.args = split_noempty(str, " ");

    // Need at least a command
    if (!msg.args.size()) {
        throw protocol_error{protocol_error_type::invalid_message, msgcpy};
    }

    // Push trailing arg back
    msg.args.push_back(trail);

    // Fetch prefix and remove it from args if it exists
    if (msg.args[0][0] == ':') {
        msg.prefix = msg.args[0].substr(1);

        msg.args.erase(begin(msg.args));
    }

    std::string cmd = msg.args[0];
    msg.args.erase(begin(msg.args));
    msg.command = command_from_string(cmd);

    return msg;
}

}
