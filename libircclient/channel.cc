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

#include "channel.h"

#include "channel_user.h"

#include "irc_except.h"
#include "environment.h"

#include <tuple>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <memory>
#include <utility>

namespace irc {

channel::channel(std::string name)
    : _modes{},
      _users{},
      _name{std::move(name)},
      _created{},
      _topic{},
      _next_uid{0}
{
}

channel::channel(const channel& c)
    : _modes{c._modes},
      _users{},
      _name{c._name},
      _created{c._created},
      _topic{c._topic},
      _next_uid{c._next_uid}
{
    for (auto& cu : c._users) {
        _users[cu.first] = std::make_unique<channel_user>(*(cu.second));
        _users[cu.first]->_channel = this;
    }
}

channel& channel::operator=(const channel& c)
{
    channel cpy{*this};

    std::swap(*this, cpy);
    return *this;
}


std::string channel::name() const
{
    return _name;
}

std::time_t channel::created() const
{
    return _created;
}

channel::topic_info channel::topic() const
{
    return _topic;
}

channel::user_list const& channel::users() const
{
    return _users;
}

channel::mode_list const& channel::modes() const
{
    return _modes;
}


channel_user& channel::find_user(std::string user) const
{
    if (_users.find(user) == std::end(_users)) {
        throw protocol_error{protocol_error_type::no_such_user, user};
    }

    return *_users.find(user)->second;
}

std::vector<std::string> channel::get_mode(char modefl) const
{
    std::vector<std::string> res;

    res.reserve(_modes.count(modefl));

    auto iters = _modes.equal_range(modefl);

    std::for_each(iters.first, iters.second, [&](auto& iter) {
        res.push_back(iter.second);
    });

    return res;
}

std::string channel::get_mode_simple(char modefl) const
{
    if (_modes.find(modefl) == std::end(_modes)) {
        throw protocol_error{
            protocol_error_type::no_such_mode, std::string{modefl}};
    }

    return _modes.find(modefl)->second;
}

bool channel::is_mode_set(char modefl) const
{
    return _modes.count(modefl) > 0;
}


void channel::set_topic(std::string topic)
{
    std::get<0>(_topic) = std::move(topic);
}

void channel::set_topic_meta(std::string setter, std::time_t settime)
{
    std::get<1>(_topic) = std::move(setter);
    std::get<2>(_topic) = settime;
}


void channel::set_created(std::time_t created)
{
    _created = created;
}


void channel::apply_modes(
    std::string const& modes,
    std::vector<std::string> const& args,
    environment const& env)
{
    auto mode_changes = env.partition_mode_changes(modes, args);

    for (auto& m : mode_changes) {
        if (std::get<0>(m)) {
            set_mode(std::get<1>(m), std::get<2>(m), env);
        } else {
            unset_mode(std::get<1>(m), std::get<2>(m), env);
        }
    }
}

channel_user& channel::create_user(std::string prefix)
{
    std::string nick;
    std::string user;
    std::string host;

    std::tie(nick, user, host) = split_prefix(prefix);

    return create_user(std::move(nick), std::move(user), std::move(host));
}

channel_user& channel::create_user(
    std::string nick,
    std::string user,
    std::string host)
{
    _users[nick] =
        std::make_unique<channel_user>(
            *this, _next_uid++, nick, std::move(user), std::move(host));

    return *_users[nick];
}


void channel::rename_user(std::string old_nick, std::string new_nick)
{
    // Have it throw an exception if user not found
    find_user(old_nick);

    _users[new_nick] = std::move(_users[old_nick]);
    _users[new_nick]->rename(new_nick);

    _users.erase(old_nick);
}


void channel::remove_user(channel_user& user)
{
    if (_users.find(user.nick()) == std::end(_users))  {
        throw protocol_error{protocol_error_type::no_such_user, user.nick()};
    }

    _users.erase(user.nick());
}


void channel::set_mode(
    char modefl,
    std::string const& argument,
    environment const& env)
{
    switch (env.get_mode_argument_type(modefl)) {
    case channel_mode_argument_type::required_user_list:
        set_list_mode(modefl, argument);
        break;

    case channel_mode_argument_type::required_user: {
        channel_user& u = find_user(argument);
        u.set_mode(modefl);

        break;
    }

    case channel_mode_argument_type::required:
    case channel_mode_argument_type::required_when_setting:
        set_simple_mode(modefl, argument);
        break;

    default:
        set_simple_mode(modefl, "");;
        break;
    }
}

void channel::unset_mode(
    char modefl,
    std::string const& argument,
    environment const& env)
{
    switch (env.get_mode_argument_type(modefl)) {
    case channel_mode_argument_type::required_user_list:
        unset_list_mode(modefl, argument);
        break;

    case channel_mode_argument_type::required_user: {
        channel_user& u = find_user(argument);
        u.unset_mode(modefl);

        break;
    }

    default:
        unset_simple_mode(modefl);
        break;
    }
}


void channel::set_list_mode(char modefl, std::string argument)
{
    // Try not to set "+<mode> <arg>" if arg is already set for that mode
    auto iters = _modes.equal_range(modefl);

    for (auto it = iters.first; it != iters.second; ++it) {
        if (rfc1459_equal(it->second, argument)) {
            // duplicate found, abort mission
            return;
        }
    }

    auto elem = mode_list::value_type{modefl, std::move(argument)};
    _modes.insert(elem);
}

void channel::set_simple_mode(char modefl, std::string argument)
{
    _modes.erase(modefl);

    auto elem = mode_list::value_type{modefl, std::move(argument)};
    _modes.insert(elem);
}


void channel::unset_list_mode(char modefl, std::string const& argument)
{
    auto iters = _modes.equal_range(modefl);

    for (auto it = iters.first; it != iters.second; ++it) {
        if (rfc1459_equal(it->second, argument)) {
            _modes.erase(it);
            return;
        }
    }
}

void channel::unset_simple_mode(char modefl)
{
    _modes.erase(modefl);
}

}
