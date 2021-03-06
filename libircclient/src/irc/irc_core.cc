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

#include "irc/irc_core.hh"
#include "irc/irc_except.hh"
#include "irc/irc_utils.hh"

#include <cstddef>
#include <cctype>

#include <algorithm>
#include <sstream>

namespace irc {

std::string to_string(message const& msg)
{
    std::ostringstream strm;

    if (!msg.prefix.empty()) {
        strm << ':' << msg.prefix << ' ';
    }

    strm << msg.command;

    for (std::string const& param : msg.args) {
        strm << ' ';
        bool trailing = false;

        if ((param.find(' ') != std::string::npos)
            or (param.find(':') != std::string::npos)) {

                trailing = true;

                strm << ':';
        }

        // Filter out all control characters
        for (char c : param) {
            if (not (std::iscntrl(c) and std::isspace(c))) {
                strm << c;
            }
        }

        if (trailing) {
            break;
        }
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
    if (not trail.empty()) {
        msg.args.push_back(trail);
    }

    // Fetch prefix and remove it from args if it exists
    if (msg.args[0][0] == ':') {
        msg.prefix = msg.args[0].substr(1);

        msg.args.erase(std::begin(msg.args));
    }

    msg.command = std::move(msg.args[0]);
    msg.args.erase(std::begin(msg.args));

    return msg;
}

}
