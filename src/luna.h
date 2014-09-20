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
#ifndef LUNA_H
#define LUNA_H

#include <channel.h>
#include <channel_user.h>
#include <environment.h>

#include "proxies/luna_channel_proxy.h"

#include "client_base.h"
#include "luna_script.h"
#include "logging.h"

#include <mond.h>

#include <string>

class luna_user;

class luna : public client_base
{
public:
    luna(
        std::string const& nick,
        std::string const& user,
        std::string const& realname,
        std::string const& password = "");

    ~luna();

    virtual void send_message(irc::message const& msg) override;

    void read_config(std::string const& filename);

    void read_shared_vars(std::string const& filename);
    void read_users(std::string const& filename);

    void save_shared_vars(std::string const& filename);
    void save_users(std::string const& filename);

    std::vector<luna_script>& scripts();
    std::vector<luna_user>&   users();

    using client_base::run;

    void run();

protected:
    void load_script(std::string const& script);

    virtual void on_message(irc::message const& msg) override;

    virtual void on_raw(irc::message const& msg) override;
    virtual void on_idle() override;

    virtual void on_connect() override;
    virtual void on_disconnect() override;

    virtual void on_invite(
        std::string const& source,
        std::string const& channel) override;

    virtual void on_join(
        std::string const& source,
        std::string const& channel) override;

    virtual void on_part(
        std::string const& source,
        std::string const& channel,
        std::string const& reason) override;

    virtual void on_quit(
        std::string const& source,
        std::string const& reason) override;

    virtual void on_nick(
        std::string const& source,
        std::string const& new_nick) override;

    virtual void on_kick(
        std::string const& source,
        std::string const& channel,
        std::string const& kicked,
        std::string const& reason) override;

    virtual void on_topic(
        std::string const& source,
        std::string const& channel,
        std::string const& new_topic) override;

    virtual void on_privmsg(
        std::string const& source,
        std::string const& target,
        std::string const& msg) override;

    virtual void on_notice(
        std::string const& source,
        std::string const& target,
        std::string const& msg) override;

    virtual void on_ctcp_request(
        std::string const& source,
        std::string const& target,
        std::string const& ctcp,
        std::string const& args) override;

    virtual void on_ctcp_response(
        std::string const& source,
        std::string const& target,
        std::string const& ctcp,
        std::string const& args) override;

    virtual void on_mode(
        std::string const& source,
        std::string const& target,
        std::string const& mode,
        std::string const& arg) override;

    virtual void pretty_print_exception(
        std::exception_ptr p,
        int lvl) const override;

private:
    void handle_core_commands(
        std::string const& prefix,
        std::string const& channel,
        std::string const& msg);

    void handle_core_ctcp(
        std::string const& prefix,
        std::string const& target,
        std::string const& ctcp,
        std::string const& args);

    template <typename... Args>
    void dispatch_signal(std::string const& signal, Args const&... args)
    {
        for (auto& script : _scripts) {
            script.emit_signal(signal, args...);
        }
    }

    template <typename... Args>
    void broadcast_signal(
        luna_script const& source,
        std::string const& signal,
        Args const&... args)
    {
        for (auto& script : _scripts) {
            if (script != source) {
                script.emit_signal(signal, args...);
            }
        }
    }

    auto get_channel_proxy(std::string const& channel)
    {
        (void)environment().find_channel(channel);
        return mond::object<luna_channel_proxy>(*this, channel);
    }

    auto get_channel_user_proxy(
        std::string const& user,
        std::string const& channel)
    {
        irc::channel& c = environment().find_channel(channel);
        irc::channel_user& u = c.find_user(user);

        return mond::object<luna_channel_user_proxy>(*this, channel, u.uid());
    }

private:
    logger _logger;

    std::string _server;
    uint16_t _port;

    std::vector<std::string> _autojoin;
    std::vector<luna_script> _scripts;
    std::vector<luna_user>   _users;

    std::string _userfile;
    std::string _varfile;

private:
    friend class luna_script;
    friend class luna_user_proxy;
    friend class luna_proxy;

    std::time_t _started;
    std::time_t _connected;

    std::size_t _bytes_sent;
    std::size_t _bytes_sent_sess;
    std::size_t _bytes_recvd;
    std::size_t _bytes_recvd_sess;
};

#endif // defined LUNA_H
