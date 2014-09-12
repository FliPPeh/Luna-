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

#include "channel_user.h"

#include "irc_utils.h"

#include <cstdint>
#include <string>
#include <tuple>

namespace irc {

channel_user::channel_user(
    class channel& channel,
    uint64_t uid,
    std::string prefix)

    : _channel{&channel},
      _uid{uid},
      _nick{},
      _user{},
      _host{},
      _modes{""}
{
    std::tie(_nick, _user, _host) = split_prefix(prefix);
}

channel_user::channel_user(
    class channel& channel,
    uint64_t uid,
    std::string nick,
    std::string user,
    std::string host)
        : _channel{&channel},
          _uid{uid},
          _nick{nick},
          _user{user},
          _host{host},
          _modes{""}
{
}


channel& channel_user::channel() const
{
    return *_channel;
}


uint64_t channel_user::uid() const
{
    return _uid;
}

std::string channel_user::nick() const
{
    return _nick;
}

std::string channel_user::user() const
{
    return _user;
}

std::string channel_user::host() const
{
    return _host;
}

std::string channel_user::modes() const
{
    return _modes;
}

bool channel_user::has_mode(char modefl) const
{
    return _modes.find(modefl) != std::string::npos;
}


void channel_user::rename(std::string new_nick)
{
    _nick = std::move(new_nick);
}

void channel_user::set_mode(char modefl)
{
    if (not has_mode(modefl)) {
        _modes += modefl;
    }
}

void channel_user::unset_mode(char modefl)
{
    if (has_mode(modefl)) {
        _modes.erase(_modes.find(modefl));
    }
}

}
