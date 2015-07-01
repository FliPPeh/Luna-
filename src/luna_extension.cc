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

#include "luna_extension.hh"

irc::unordered_rfc1459_map<std::string, std::string>
    luna_extension::shared_vars{};


luna_extension::luna_extension(luna& context)
    : _context{&context}
{
}

luna_extension::~luna_extension() { }

void luna_extension::init() { }
void luna_extension::destroy() { }

void luna_extension::on_connect() { }
void luna_extension::on_disconnect() { }
void luna_extension::on_idle() { }

void luna_extension::on_message(irc::message const& msg) { }

void luna_extension::on_invite(
    std::string const& source,
    std::string const& channel) { }

void luna_extension::on_channel_sync(
    std::string const& channel,
    sync_type type) { }

void luna_extension::on_join(
    std::string const& source,
    std::string const& channel) { }

void luna_extension::on_part(
    std::string const& source,
    std::string const& channel,
    std::string const& reason) { }

void luna_extension::on_quit(
    std::string const& source,
    std::string const& reason) { }

void luna_extension::on_nick(
    std::string const& source,
    std::string const& new_nick) { }

void luna_extension::on_kick(
    std::string const& source,
    std::string const& channel,
    std::string const& kicked,
    std::string const& reason) { }

void luna_extension::on_topic(
    std::string const& source,
    std::string const& channel,
    std::string const& new_topic) { }

void luna_extension::on_privmsg(
    std::string const& source,
    std::string const& target,
    std::string const& msg) { }

void luna_extension::on_notice(
    std::string const& source,
    std::string const& target,
    std::string const& msg) { }

void luna_extension::on_ctcp_request(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args) { }

void luna_extension::on_ctcp_response(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args) { }

void luna_extension::on_mode(
    std::string const& source,
    std::string const& target,
    std::string const& mode,
    std::string const& arg) { }

luna& luna_extension::context() const
{
    return *_context;
}
