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

#pragma once
#ifndef LUNA_LUNA_EXTENSION_HH_INCLUDED
#define LUNA_LUNA_EXTENSION_HH_INCLUDED

#include <irc/irc_utils.hh>

#include <string>

class luna;

class luna_extension {
public:
    static irc::unordered_rfc1459_map<std::string, std::string> shared_vars;

    enum class sync_type {
        users,
        bans
    };

    luna_extension(luna& context);
    virtual ~luna_extension() = 0;

    luna_extension(luna_extension const&) = delete;
    luna_extension(luna_extension&&) = default;

    luna_extension& operator=(luna_extension const&) = delete;
    luna_extension& operator=(luna_extension&&) = default;

    virtual std::string id()          const = 0;
    virtual std::string name()        const = 0;
    virtual std::string description() const = 0;
    virtual std::string version()     const = 0;

    virtual void init();
    virtual void destroy();

    virtual void on_connect();
    virtual void on_disconnect();
    virtual void on_idle();

    // Message sent from this client
    virtual void on_message_send(irc::message const& msg);

    // Raw handler
    virtual void on_message(irc::message const& msg);

    // Cooked events
    virtual void on_invite(
        std::string const& source,
        std::string const& channel);

    virtual void on_channel_sync(
        std::string const& channel,
        sync_type type);

    virtual void on_join(
        std::string const& source,
        std::string const& channel);

    virtual void on_part(
        std::string const& source,
        std::string const& channel,
        std::string const& reason);

    virtual void on_quit(
        std::string const& source,
        std::string const& reason);

    virtual void on_nick(
        std::string const& source,
        std::string const& new_nick);

    virtual void on_kick(
        std::string const& source,
        std::string const& channel,
        std::string const& kicked,
        std::string const& reason);

    virtual void on_topic(
        std::string const& source,
        std::string const& channel,
        std::string const& new_topic);

    virtual void on_privmsg(
        std::string const& source,
        std::string const& target,
        std::string const& msg);

    virtual void on_notice(
        std::string const& source,
        std::string const& target,
        std::string const& msg);

    virtual void on_ctcp_request(
        std::string const& source,
        std::string const& target,
        std::string const& ctcp,
        std::string const& args);

    virtual void on_ctcp_response(
        std::string const& source,
        std::string const& target,
        std::string const& ctcp,
        std::string const& args);

    virtual void on_mode(
        std::string const& source,
        std::string const& target,
        std::string const& mode,
        std::string const& arg);

protected:
    luna& context() const;

private:
    luna* _context;
};

#endif // LUNA_LUNA_EXTENSION_HH_INCLUDED
