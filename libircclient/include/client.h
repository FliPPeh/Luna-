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

#ifndef CLIENT_H
#define CLIENT_H

#include "macros.h"

#include "irc_core.h"
#include "irc_utils.h"

#include <boost/asio.hpp>

#include <cstddef>

#include <string>
#include <functional>
#include <array>
#include <chrono>
#include <queue>

namespace irc {

struct message;

class environment;
class async_connection;

class DLL_PUBLIC client {
public:
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

    void change_nick(std::string const& nick);

    // May be overridden to implement flood throttling
    virtual void send_message(message const& msg);

    bool connected() const;

    std::string   server_host() const;
    std::string   server_addr() const;
    uint16_t      server_port() const;

    irc::environment const& environment() const;

    std::string nick() const;
    std::string user() const;
    std::string realname() const;
    std::string password() const;

    void use_ssl(bool setting);
    bool use_ssl() const;

protected:
    bool is_me(std::string user) const;

    // Overridden by actual clients, noops in base.
    virtual void on_message(message const& msg);
    virtual void on_idle();
    virtual void on_connect();
    virtual void on_disconnect();

    virtual void pretty_print_exception(std::exception_ptr p, int lvl) const;
    virtual void report_error(std::exception_ptr p, int lvl = 0) const;

    void set_idle_interval(int ms);

    std::string _pass;

    // Identity
    std::string _nick;
    std::string _user;
    std::string _real;

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

    DLL_LOCAL void init_core_handlers();

private:
    enum session_state {
        START,
        LOGIN_SENT,
        LOGGED_IN,
        STOP
    };

    boost::asio::io_service         _io_service;
    boost::asio::deadline_timer     _idle_timer;
    boost::posix_time::milliseconds _idle_interval;

    std::unique_ptr<irc::async_connection> _irccon;
    std::unique_ptr<irc::environment>      _ircenv;

    std::queue<irc::message> _write_queue;

    bool _use_ssl;

    session_state _session_state;

    unordered_rfc1459_map<std::string, handler> _core_handlers;

    void (client::*_current_handler)(message const& msg);
};

}

#endif
