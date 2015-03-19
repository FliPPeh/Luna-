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
#ifndef LIBIRCCLIENT_CLIENT_HH_INCLUDED
#define LIBIRCCLIENT_CLIENT_HH_INCLUDED

#include "irc/macros.h"
#include "irc/irc_core.hh"
#include "irc/irc_utils.hh"

#include <cstddef>

#include <string>
#include <functional>
#include <array>
#include <chrono>
#include <queue>
#include <memory>

namespace irc {

struct message;
class environment;

class DLL_PUBLIC client {
public:
    // Wait at most `timeout' seconds before reconnecting after receiving the
    // last message from the server (to repair broken client side connections)
    static constexpr unsigned timeout = 300; // 5 minutes

    client(
        std::string nick,
        std::string user,
        std::string realname,
        std::string pass = "");

    client(client const&) = delete; // No copy

    virtual ~client();

    void run(std::string const& host, uint16_t port, bool ssl = false);
    void stop();
    void disconnect(std::string reason);

    // Always allowed
    void change_nick(std::string const& nick);

    // Only allowed before connecting.
    void change_user(std::string const& user);
    void change_realname(std::string const& realname);
    void change_password(std::string const& password);

    void set_idle_interval(int ms);

    std::string nick() const;
    std::string user() const;
    std::string realname() const;
    std::string password() const;

    std::string   server_host() const;
    std::string   server_addr() const;
    uint16_t      server_port() const;

    void use_ssl(bool setting);
    bool use_ssl() const;

    // May be overridden to implement flood throttling
    virtual void send_message(message const& msg);

    bool connected() const;

    irc::environment const& environment() const;

protected:
    bool is_me(std::string user) const;

    // Overridden by actual clients, noops in base.
    virtual void on_message(message const& msg);
    virtual void on_idle();
    virtual void on_connect();
    virtual void on_disconnect();

    virtual void pretty_print_exception(std::exception_ptr p, int lvl) const;
    virtual void report_error(std::exception_ptr p, int lvl = 0) const;


private:
    using handler = std::tuple<
        std::size_t,                           // minimum number of arguments
        bool,                                  // needs user prefix?
        bool,                                  // run *after* user handler?
        std::function<void (message const&)>>; // callback


    DLL_LOCAL void send_queue();

    DLL_LOCAL void handle_message(message const& msg);

    DLL_LOCAL void do_disconnect();
    DLL_LOCAL void do_idle();

    DLL_LOCAL void login_handler(message const& msg);
    DLL_LOCAL void main_handler(message const& msg);

    DLL_LOCAL void run_core_handler(handler const& handler, message const& msg);
    DLL_LOCAL void run_user_handler(message const& msg);

    DLL_LOCAL void init_core_handlers();

private:
    // Things the outside neither cares about, nor SHOULD be able to care about.
    //
    // A case could be made for hiding most of this class behind an
    // opaque pointer, but that gets messy real fast, so I only hide the bigger
    // dependencies.
    struct details;
    std::unique_ptr<details> _impl;

    // State of this client
    enum class session_state {
        start,
        login_sent,
        logged_in,
        stop
    };

    session_state _session_state = session_state::start;

    std::queue<irc::message> _write_queue;

    std::string _pass;
    std::string _nick;
    std::string _user;
    std::string _real;
    bool _use_ssl = false;

    unordered_rfc1459_map<std::string, handler> _core_handlers;

    void (client::*_current_handler)(message const& msg) =
        &client::login_handler;

    std::chrono::system_clock::time_point _last_contact;
};

}

#endif // defined LIBIRCCLIENT_CLIENT_HH_INCLUDED
