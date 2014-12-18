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
#ifndef LIBIRCCLIENT_ENVIRONMENT_HH_INCLUDED
#define LIBIRCCLIENT_ENVIRONMENT_HH_INCLUDED

#include "irc/irc_utils.hh"

#include <ctime>

#include <tuple>
#include <unordered_map>
#include <string>
#include <memory>

namespace irc {

class channel;

/*! \brief The different classifications of channel modes */
enum class channel_mode_argument_type {
    required_user_list,    //!< Address list mode (ban list, ban exceptions)
    required_user,         //!< User mode (operator, voice)
    required,              //!< Mode with required argument (channel key)
    required_when_setting, //!< Mode with required argument when setting only
    no_argument            //!< Mode without argument
};

/*! \brief An IRC environment.
 *
 * Contains information about supported server features and details of
 * implementation. Used by the client to create, find and remove channels.
 */
class DLL_PUBLIC environment {
public:
    // use pointers to channels so we can expose an immutable _channels, with
    // mutable channel elements.
    using channel_list =
        unordered_rfc1459_map<std::string, std::unique_ptr<channel>>;

    using channel_prefixes = std::unordered_map<char, char>;

    using channel_mode_types =
        std::tuple<std::string,  // A) parameter = always (list)
                   std::string,  // B) parameter = always (includes prefix)
                   std::string,  // C) parameter = when setting
                   std::string>; // D) parameter = never (and default)

    using channel_mode_changes =
        std::vector<
            std::tuple<
                bool,          // true = '+', false = '-'
                char,          // flag
                std::string>>; // argument (which may be empty)

    environment();

    environment(environment const& rhs);
    environment(environment&&)      = default;

    environment& operator=(environment const&) = delete;
    environment& operator=(environment&&)      = default;

    void        set_capability(std::string const& cap, std::string const& val);
    std::string get_capability(std::string const& cap) const;

    unordered_rfc1459_map<std::string, std::string> const& capabilities() const;

    // {{'@', 'o'}, {'+', 'v'}, ...}
    channel_prefixes const&   prefixes()  const;
    channel_mode_types const& chanmodes() const;

    std::string channel_types() const;
    bool is_channel(std::string const& subj) const;

    // Distributes channel modes with their respective flags
    channel_mode_changes partition_mode_changes(
        std::string const& modes,
        std::vector<std::string> const& args) const;

    channel_list const& channels() const;

    bool has_channel(std::string const& channel) const;
    channel& find_channel(std::string const& channel) const;

    channel_mode_argument_type get_mode_argument_type(char mode) const;

private:
    // anything a client can do to us
    friend class client;

    channel& create_channel(std::string name);
    void     remove_channel(channel& channel);

private:
    DLL_LOCAL void init_channel_modes(std::string const& chanmodes);
    DLL_LOCAL void init_channel_prefixes(std::string const& prefix);

private:
    unordered_rfc1459_map<std::string, std::string> _capabilities;

    // Initialize with sensible default values until the server updates it.
    channel_mode_types _channel_modes =
        channel_mode_types{"beI", "k", "l", "imnpstaqr"};

    channel_prefixes _channel_prefixes = {{'@', 'o'}, {'%', 'h'}, {'+', 'v'}};
    std::string _prefix_modes  = "ovh";
    std::string _channel_types = "#&";

    channel_list _channels;
};

}

#endif // defined LIBIRCCLIENT_ENVIRONMENT_HH_INCLUDED
