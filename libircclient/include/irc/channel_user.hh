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

#pragma once
#ifndef LIBIRCCLIENT_CHANNEL_USER_HH_INCLUDED
#define LIBIRCCLIENT_CHANNEL_USER_HH_INCLUDED

#include "irc/macros.h"

#include <cstdint>

#include <string>

namespace irc {

class channel;

/*! \brief An IRC channel user.
 *
 * Used by the channel to update information about an active IRC channel user.
 * Can be used by anyone to fetch details about, or to use it as a target for
 * commands.
 */
class DLL_PUBLIC channel_user {
public:
    channel_user(class channel& channel, uint64_t uid, std::string prefix);
    channel_user(
        class channel& channel,
        uint64_t uid,
        std::string nick,
        std::string user,
        std::string host);

    channel_user(channel_user const&)            = default;
    channel_user& operator=(channel_user const&) = default;

    channel_user(channel_user&&)            = default;
    channel_user& operator=(channel_user&&) = default;

    irc::channel& channel() const;

    uint64_t     uid() const;
    std::string nick() const;
    std::string user() const;
    std::string host() const;

    std::string modes() const;
    bool has_mode(char modefl) const;

private:
    // anything a channel can do to us
    friend class channel;

    void rename(std::string new_nick);

    void set_mode(char modefl);
    void unset_mode(char modefl);

private:
    irc::channel* _channel;
    uint64_t _uid;

    std::string _nick;
    std::string _user;
    std::string _host;
    std::string _modes;
};

}

#endif // defined LIBIRCCLIENT_CHANNEL_USER_HH_INCLUDED

