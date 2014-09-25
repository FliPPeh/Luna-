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
#ifndef CHANNEL_H
#define CHANNEL_H

#include "macros.h"

#include "irc_utils.h"

#include <ctime>

#include <tuple>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

namespace irc {

class environment;
class channel_user;


/*! \brief An IRC channel.
 *
 * Used by the client to update information about an active IRC channel. Can be
 * used by anyone to fetch details about, or to use it as a target for
 * commands.
 */
class DLL_PUBLIC channel {
public:
    using topic_info =
        std::tuple<
            std::string,  // The topic itself
            std::string,  // User that set it
            std::time_t>; // Time when set

    // Value may be empty (simple modes). One mode can map to multiple arguments
    // due to user list modes. The interpretation of the contents in this map
    // depends on the mode type of the value (see
    // environment::get_mode_argument_type()).
    //
    // channel::[un]set_mode(), channel::change_mode() and channel::get_mode()
    //  will do the correct thing according to the mode in question.
    using mode_list = std::unordered_multimap<char, std::string>;
    using user_list = unordered_user_map<std::unique_ptr<channel_user>>;

    channel(std::string name);

    channel(channel const& c);
    channel& operator=(channel const& c);

    channel(channel&&)            = default;
    channel& operator=(channel&&) = default;

    // Accessors
    std::string      name()    const;
    std::time_t      created() const;
    topic_info       topic()   const;
    user_list const& users()   const;
    mode_list const& modes()   const;

    // Operations
    bool           has_user(std::string const& user) const;
    channel_user& find_user(std::string const& user) const;

    std::vector<std::string> get_mode(       char modefl) const;
    std::string              get_mode_simple(char modefl) const;

    bool is_mode_set(char modefl) const;

private:
    // anything the client has to call
    friend class client;

    // Meta-Management
    void set_topic(std::string topic);
    void set_topic_meta(std::string setter, std::time_t settime);

    void set_created(time_t created);

    // Mode management
    void apply_modes(
        std::string const& modes,
        std::vector<std::string> const& args,
        environment const& env);

    // User management
    channel_user& create_user(std::string prefix);
    channel_user& create_user(
        std::string nick,
        std::string user,
        std::string host);

    void rename_user(std::string old_nick, std::string new_nick);

    void remove_user(channel_user& user);

private:
    DLL_LOCAL void set_mode(
        char modefl,
        std::string const& argument,
        environment const& env);

    DLL_LOCAL void unset_mode(
        char modefl,
        std::string const& argument,
        environment const& env);

    DLL_LOCAL void set_list_mode(char modefl, std::string argument);
    DLL_LOCAL void set_simple_mode(char modefl, std::string argument);

    DLL_LOCAL void unset_list_mode(char modefl, std::string const& argument);
    DLL_LOCAL void unset_simple_mode(char modefl);

private:
    mode_list _modes;
    user_list _users;

    std::string _name;
    std::time_t _created;
    topic_info  _topic;

    uint64_t _next_uid;
};

}

#endif // defined CHANNEL_H
