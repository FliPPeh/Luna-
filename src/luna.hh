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
#ifndef LUNA_LUNA_HH_INCLUDED
#define LUNA_LUNA_HH_INCLUDED

#include "client_base.hh"
#include "logging.hh"

#include <channel.hh>
#include <channel_user.hh>
#include <environment.hh>

#include <mond.hh>

#include <string>

class luna_user;
class luna_extension;

class luna final : public client_base {
public:
    luna(std::string const& cfgfile);
    ~luna();

    virtual void send_message(irc::message const& msg) override;

    void read_config(std::string const& filename);

    void read_shared_vars(std::string const& filename);
    void read_users(std::string const& filename);

    void save_shared_vars(std::string const& filename);
    void save_users(std::string const& filename);

    std::vector<std::unique_ptr<luna_extension>> const& extensions();
    std::vector<luna_user>& users();

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
    void handle_core_ctcp(
        std::string const& prefix,
        std::string const& target,
        std::string const& ctcp,
        std::string const& args);

    template <typename Ret, typename... Args>
    void dispatch_event(Ret (luna_extension::*fn)(Args...), Args&&... args)
    {
        std::size_t i = 0;

        // Since the list can grow while we iterate it, keep updating size.
        while (i < _exts.size()) {
            if (_exts[i]) {
                (_exts[i].get()->*fn)(std::forward<Args>(args)...);
            }

            ++i;
        }

        // Unloadind a script only resets the unique_ptr to nullptr. Drop all
        // unloaded scripts from the list when it's safe to shrink.
        _exts.erase(
            std::remove_if(std::begin(_exts), std::end(_exts),
                [] (std::unique_ptr<luna_extension> const& scr) {
                    return not scr;
                }),
            std::end(_exts));
    }

private:
    logger _logger{"luna", logging_level::DEBUG, logging_flags::ANSI};

    std::string _server = "";
    uint16_t _port      = 6667;

    std::vector<std::unique_ptr<luna_extension>> _exts;
    std::vector<std::string> _autojoin;
    std::vector<luna_user>   _users;

    std::string _userfile = "users.txt";
    std::string _varfile  = "shared_vars.txt";

private:
    friend class luna_script;
    friend class luna_user_proxy;
    friend class luna_proxy;

    std::time_t _started   = std::time(nullptr);
    std::time_t _connected = 0;

    std::size_t _bytes_sent       = 0;
    std::size_t _bytes_sent_sess  = 0;
    std::size_t _bytes_recvd      = 0;
    std::size_t _bytes_recvd_sess = 0;
};

#endif // defined LUNA_LUNA_HH_INCLUDED
