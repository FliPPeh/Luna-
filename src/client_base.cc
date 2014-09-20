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

#include "client_base.h"

#include <environment.h>

#include "tokenbucket.h"

#include <iostream>

client_base::client_base(
    std::string const& nick,
    std::string const& user,
    std::string const& realname,
    std::string const& password)
        : irc::client{nick, user, realname, password},
          _bucket{512, 64, 128},
          _message_queue{}
{
}

void client_base::send_message(const irc::message& msg)
{
    _message_queue.push(msg);

    work_through_queue();
}


void client_base::on_message(irc::message const& msg)
{
    switch (msg.command) {
    case irc::command::INVITE:
        if (msg.args.size() > 0) {
            on_invite(msg.prefix, msg.args[0]);
        }

        break;

    case irc::command::JOIN:
        if (msg.args.size() > 0) {
            on_join(msg.prefix, msg.args[0]);
        }

        break;

    case irc::command::PART:
        if (msg.args.size() > 1) {
            on_part(msg.prefix, msg.args[0], msg.args[1]);
        }

        break;

    case irc::command::QUIT:
        if (msg.args.size() > 0) {
            on_quit(msg.prefix, msg.args[0]);
        }

        break;

    case irc::command::NICK:
        if (msg.args.size() > 0) {
            on_nick(msg.prefix, msg.args[0]);
        }

        break;

    case irc::command::KICK:
        if (msg.args.size() > 2) {
            on_kick(msg.prefix, msg.args[0], msg.args[1], msg.args[2]);
        }

        break;

    case irc::command::TOPIC:
        if (msg.args.size() > 1) {
            on_topic(msg.prefix, msg.args[0], msg.args[1]);
        }

        break;

    case irc::command::PRIVMSG:
        if (msg.args.size() > 1) {
            handle_direct_message(
                msg.prefix,
                msg.args[0],
                msg.args[1],
                &client_base::on_privmsg,
                &client_base::on_ctcp_request);
        }

        break;

    case irc::command::NOTICE:
        if (msg.args.size() > 1) {
            handle_direct_message(
                msg.prefix,
                msg.args[0],
                msg.args[1],
                &client_base::on_notice,
                &client_base::on_ctcp_response);
        }

        break;

    case irc::command::MODE:
        if (msg.args.size() > 1 and environment().is_channel(msg.args[0])) {
            auto changes = environment().partition_mode_changes(
                msg.args[0],
                {std::begin(msg.args) + 1, std::end(msg.args)});

            for (auto& mc : changes) {
                std::ostringstream m;
                m << (std::get<0>(mc) ? '+' : '-') << std::get<1>(mc);

                on_mode(msg.prefix, msg.args[0], m.str(), std::get<2>(mc));
            }
        }

        break;

    default:
        break;
    }

    on_raw(msg);
}

void client_base::on_idle()
{
    work_through_queue();
}


void client_base::on_raw(irc::message const& msg)
{
}


void client_base::on_connect()
{
}

void client_base::on_disconnect()
{
}


void client_base::on_invite(
    std::string const& source,
    std::string const& channel)
{
}

void client_base::on_join(
    std::string const& source,
    std::string const& channel)
{
}

void client_base::on_nick(
    std::string const& source,
    std::string const& new_nick)
{
}

void client_base::on_part(
    std::string const& source,
    std::string const& channel,
    std::string const& reason)
{
}

void client_base::on_quit(
    std::string const& source,
    std::string const& reason)
{
}

void client_base::on_kick(
    std::string const& source,
    std::string const& channel,
    std::string const& kicked,
    std::string const& reason)
{
}

void client_base::on_topic(
    std::string const& source,
    std::string const& channel,
    std::string const& new_topic)
{
}

void client_base::on_privmsg(
    std::string const& source,
    std::string const& target,
    std::string const& msg)
{
}

void client_base::on_notice(
    std::string const& source,
    std::string const& target,
    std::string const& msg)
{
}

void client_base::on_ctcp_request(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
}

void client_base::on_ctcp_response(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
}

void client_base::on_mode(
    std::string const& source,
    std::string const& target,
    std::string const& mode,
    std::string const& arg)
{
}


namespace {

bool is_ctcp(std::string const& msg)
{
    return msg.front() == '\x01'
       and msg.back()  == '\x01';
}

void split_ctcp(
    std::string const& msg,
    std::string& ctcp,
    std::string& ctcp_args)
{
    if (msg.find(' ') != std::string::npos) {
        ctcp = msg.substr(1, msg.find(' ') - 1);
        ctcp_args = msg.substr(msg.find(' ') + 1, msg.size() - msg.find(' '));
    } else {
        ctcp = msg.substr(1, msg.size() - 2);
        ctcp_args = "";
    }
}

}

void client_base::handle_direct_message(
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
        std::string const&))
{
    if (is_ctcp(msg)) {
        std::string ctcp, ctcp_args;

        split_ctcp(msg, ctcp, ctcp_args);
        (this->*ctcp_handler)(prefix, target, ctcp, ctcp_args);
    } else {
        (this->*message_handler)(prefix, target, msg);
    }
}

void client_base::work_through_queue()
{
    while (not _message_queue.empty()) {
        irc::message& msg = _message_queue.front();

        std::string msgstr = irc::to_string(msg);

        if (_bucket.consume(msgstr.size() + 2)) {
            irc::client::send_message(msg);
            _message_queue.pop();
        } else {
            break;
        }
    }
}

