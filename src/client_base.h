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
#ifndef CLIENT_BASE_H
#define CLIENT_BASE_H

#include <client.h>

#include "tokenbucket.h"

#include <string>
#include <queue>

class client_base : public irc::client {
public:
    client_base(
        std::string const& nick,
        std::string const& user,
        std::string const& realname,
        std::string const& password = "");

    virtual void send_message(irc::message const& msg) override;

protected:
    virtual void on_message(irc::message const& msg) override;
    virtual void on_idle() override;

    virtual void on_raw(irc::message const& msg);

    virtual void on_connect()    override;
    virtual void on_disconnect() override;

    virtual void on_invite(
        std::string const& source,
        std::string const& channel);

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

private:
    void handle_direct_message(
        std::string const& prefix,
        std::string const& target,
        std::string const& msg,
        void (client_base::*message_handler)(
            std::string const&,
            std::string const&,
            std::string const&),
        void (client_base::*ctcp_handler)(
            std::string const&,
            std::string const&,
            std::string const&,
            std::string const&));

    void work_through_queue();

private:
    tokenbucket _bucket;
    std::queue<irc::message> _message_queue;
};

#endif // defined CLIENT_BASE_H
