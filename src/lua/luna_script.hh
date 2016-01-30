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
#ifndef LUNA_LUA_LUNA_SCRIPT_HH_INCLUDED
#define LUNA_LUA_LUNA_SCRIPT_HH_INCLUDED

#include "luna.hh"
#include "logging.hh"
#include "luna_extension.hh"

#include "lua/proxies/luna_channel_proxy.hh"
#include "lua/proxies/luna_user_proxy.hh"
#include "lua/proxies/luna_extension_proxy.hh"

#include <irc/irc_utils.hh>
#include <irc/channel.hh>
#include <irc/channel_user.hh>
#include <irc/environment.hh>

#include <mond/mond.hh>

#include <string>

class luna;

class luna_script : public luna_extension {
public:
    static constexpr char const* api = "__luna";

    luna_script(luna& context, std::string file);
    virtual ~luna_script();

    virtual std::string id()          const override;
    virtual std::string name()        const override;
    virtual std::string description() const override;
    virtual std::string version()     const override;

    friend bool operator==(luna_script const& me, luna_script const& other);
    friend bool operator!=(luna_script const& me, luna_script const& other);

    // Aaaaand all the event handlers
    virtual void init() override;
    virtual void destroy() override;

    virtual void on_connect() override;
    virtual void on_disconnect() override;
    virtual void on_idle() override;

    virtual void on_message_send(irc::message const& msg) override;

    virtual void on_message(irc::message const& msg) override;

    virtual void on_invite(
        std::string const& source,
        std::string const& channel) override;

    virtual void on_channel_sync(
           std::string const& channel,
           sync_type type) override;

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

private:
    void setup_api();

    void register_environment();
    void register_self();
    void register_script();
    void register_user();
    void register_channel();
    void register_channel_user();

    auto get_channel_proxy(irc::channel const& channel)
    {
        return mond::object<luna_channel_proxy>(
            this->context(), channel.name());
    }

    auto get_unknown_user_proxy(std::string prefix)
    {
        return mond::object<luna_unknown_user_proxy>(
            this->context(), std::move(prefix));
    }

    auto get_channel_user_proxy(
        irc::channel_user const& user,
        irc::channel const& channel)
    {
        return mond::object<luna_channel_user_proxy>(
            this->context(), channel.name(), user.uid());
    }

    template <typename... Args>
    void emit_signal(std::string const& signal, Args&&... args)
    {
        try {
            _lua["luna"]["handle_signal"].call(
                signal, std::forward<Args>(args)...);
        } catch (mond::error const& e) {
            _logger.warn() << "Signal handler failed: " << e.what();
        }
    }

    template <typename... Args>
    auto emit_signal_helper(
        std::string const& signal,
        std::string const& user,
        std::string const& target,
        Args&&... args)
    {
        if (context().environment().is_channel(target)) {
            irc::channel const& chan =
                context().environment().find_channel(target);

            if (chan.has_user(user)) {
                emit_signal("channel_" + signal,
                    get_channel_user_proxy(chan.find_user(user), chan),
                    get_channel_proxy(chan),
                    std::forward<Args>(args)...);
            } else {
                emit_signal("channel_" + signal,
                    get_unknown_user_proxy(user),
                    get_channel_proxy(chan),
                    std::forward<Args>(args)...);
            }
        } else {
            emit_signal(signal,
                get_unknown_user_proxy(user), std::forward<Args>(args)...);
        }
    }

    bool split_message_level(std::string& target, std::string& level);

private:
    friend class luna_extension_proxy;

    logger _logger;

    std::string _file;
    mond::state _lua;

    std::string _script_name;
    std::string _script_descr;
    std::string _script_version;
};

#endif // defined LUNA_LUA_LUNA_SCRIPT_HH_INCLUDED
