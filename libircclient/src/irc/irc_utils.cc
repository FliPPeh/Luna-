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

#include "irc/irc_utils.hh"

#include "irc/irc_core.hh"
#include "irc/irc_except.hh"

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
    std::string user = prefix.substr(
        prefix.find('!') + 1,
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

    std::transform(std::begin(str), std::end(str), std::begin(out), [](char c) {
        return rfc1459_lower(c);
    });

    return out;
}

std::string rfc1459_upper(std::string const& str)
{
    std::string out;

    out.resize(str.size());

    std::transform(std::begin(str), std::end(str), std::begin(out), [](char c) {
        return rfc1459_upper(c);
    });

    return out;
}

}

std::vector<std::string> split_noempty(
    std::string const& src,
    std::string const& sep)
{
    std::size_t seppos = 0;

    std::vector<std::string> results;

    while (seppos != std::string::npos) {
        std::size_t old = seppos ? seppos + sep.size() : 0;

        seppos = src.find(sep, seppos + 1);

        std::string sub = src.substr(old, seppos - old);

        if (!sub.empty()) {
            results.push_back(sub);
        }
    }

    return results;
}

