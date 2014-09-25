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

#include "environment.h"

#include "irc_utils.h"
#include "irc_except.h"
#include "channel.h"
#include "channel_user.h"

#include <algorithm>
#include <sstream>
#include <vector>
#include <memory>
#include <tuple>

namespace irc {

environment::environment()
    : _capabilities{},
      _channel_modes{channel_mode_types{"beI", "k", "l", "imnpstaqr"}},
      _channel_prefixes{channel_prefixes{{'@', 'o'}, {'%', 'h'}, {'+', 'v'}}},
      _prefix_modes{"ohv"},
      _channel_types{"#&"},
      _channels{}
{
}

environment::environment(const environment& rhs)
    : _capabilities{rhs._capabilities},
      _channel_modes{rhs._channel_modes},
      _channel_prefixes{rhs._channel_prefixes},
      _prefix_modes{rhs._prefix_modes},
      _channel_types{rhs._channel_types},
      _channels{}
{
    for (auto& c : rhs._channels) {
        _channels[c.first] = std::make_unique<channel>(*(c.second));
    }
}


void environment::set_capability(std::string const& cap, std::string const& val)
{
    _capabilities[cap] = val;

    if (rfc1459_equal(cap, "CHANMODES")) {
        init_channel_modes(val);
    } else if (rfc1459_equal(cap, "PREFIX")) {
        init_channel_prefixes(val);
    } else if (rfc1459_equal(cap, "CHANTYPES")) {
        _channel_types = val;
    }
}

std::string environment::get_capability(std::string const& cap) const
{
    return _capabilities.at(cap);
}


unordered_rfc1459_map<std::string, std::string > const&
environment::capabilities() const
{
    return _capabilities;
}



environment::channel_prefixes const& environment::prefixes() const
{
    return _channel_prefixes;
}

environment::channel_mode_types const& environment::chanmodes() const
{
    return _channel_modes;
}


bool environment::is_channel(std::string const& subj) const
{
    return _channel_types.find(subj.front()) != std::string::npos;
}


environment::channel_mode_changes environment::partition_mode_changes(
    std::string const& modes,
    std::vector<std::string> const& args) const
{
    auto args_iter = std::begin(args);
    bool setting = true;

    channel_mode_changes res;

    for (char c : modes) {
        if ((c == '+') or (c == '-')) {
            setting = (c == '+');
            continue;
        }

        enum channel_mode_argument_type type = get_mode_argument_type(c);

        std::string mode_arg;

        if ((setting &&
                (type == channel_mode_argument_type::required_when_setting))
             or (type == channel_mode_argument_type::required)
             or (type == channel_mode_argument_type::required_user_list)
             or (type == channel_mode_argument_type::required_user)) {

            if (args_iter == std::end(args)) {
                std::ostringstream err;

                err << "not enough arguments for mode `" << c << "' in "
                    << "`" << modes << "'";

                throw protocol_error{
                    protocol_error_type::not_enough_arguments, err.str()};
            }

            mode_arg = *args_iter++;
        }

        res.emplace_back(setting, c, mode_arg);
    }

    return res;
}


environment::channel_list const& environment::channels() const
{
    return _channels;
}


channel& environment::find_channel(std::string channel) const
{
    if (_channels.find(channel) == std::end(_channels)) {
        throw protocol_error{protocol_error_type::no_such_channel, channel};
    }

    return *_channels.find(channel)->second;
}


channel_mode_argument_type environment::get_mode_argument_type(char mode) const
{
    if (_prefix_modes.find(mode) != std::string::npos) {
        return channel_mode_argument_type::required_user;
    } else if (std::get<0>(_channel_modes).find(mode) != std::string::npos) {
        return channel_mode_argument_type::required_user_list;
    } else if (std::get<1>(_channel_modes).find(mode) != std::string::npos) {
        return channel_mode_argument_type::required;
    } else if (std::get<2>(_channel_modes).find(mode) != std::string::npos) {
        return channel_mode_argument_type::required_when_setting;
    } else {
        return channel_mode_argument_type::no_argument;
    }
}


channel& environment::create_channel(std::string name)
{
    _channels[name] = std::make_unique<channel>(name);

    return *_channels[name];
}

void environment::remove_channel(channel& channel)
{
    _channels.erase(channel.name());
}


void environment::init_channel_modes(std::string const& chanmodes)
{
    if (std::count(std::begin(chanmodes), end(chanmodes), ',') != 3) {
        return;
    }

    std::vector<std::string> parts = split_noempty(chanmodes, ",");

    _channel_modes = channel_mode_types{
        parts[0] + _prefix_modes,
        parts[1],
        parts[2],
        parts[3]};
}

void environment::init_channel_prefixes(std::string const& prefix)
{
    _channel_prefixes.clear();
    _prefix_modes.clear();

    auto flag_pos = std::begin(prefix) + 1;
    auto pref_pos = std::begin(prefix) + ((prefix.size() / 2) + 1);

    while ((*flag_pos != ')') and (pref_pos != std::end(prefix))) {
        _channel_prefixes[*pref_pos] = *flag_pos;
        _prefix_modes += *flag_pos;

        ++flag_pos;
        ++pref_pos;
    }
}

}
