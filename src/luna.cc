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

#include "luna.h"
#include "luna_user.h"
#include "luna_script.h"
#include "logging.h"

#include <irc_core.h>
#include <irc_except.h>
#include <irc_helpers.h>
#include <environment.h>

#include <mond.h>

#include <lua.hpp>

#include <ctime>

#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>


luna::luna(
    std::string const& nick,
    std::string const& user,
    std::string const& realname,
    std::string const& password)
        : client_base{nick, user, realname, password},
          _logger{"luna", logging_level::DEBUG, logging_flags::ANSI},
          _server{""},
          _port{6667},
          _autojoin{},
          _scripts{},
          _users{},
          _userfile{"users.txt"},
          _varfile{"shared_vars.txt"},
          _started{std::time(nullptr)},
          _connected{0},
          _bytes_sent{0},
          _bytes_sent_sess{0},
          _bytes_recvd{0},
          _bytes_recvd_sess{0}
{
    read_shared_vars(_varfile);
    read_users(_userfile);

    luna_script::shared_vars["luna.version"]  = "6.283";
    luna_script::shared_vars["luna.compiled"] = __DATE__ " " __TIME__;
    luna_script::shared_vars["luna.trigger"] = "!";

    std::ostringstream compiler;

#if defined(__clang__)
    compiler << "clang v" << __clang_major__ << "."
                          << __clang_minor__ << "."
                          << __clang_patchlevel__;
#elif defined(__GNUC__)
    compiler << "GCC v" << __GNUC__       << "."
                        << __GNUC_MINOR__ << "."
                        << __GNUC_PATCHLEVEL__;
#else
    compiler << "(unknown)";
#endif

    luna_script::shared_vars["luna.compiler"] = compiler.str();

    read_config("config.lua");
}


luna::~luna()
{
    save_users(_userfile);
    save_shared_vars(_varfile);
}


void luna::send_message(const irc::message& msg)
{
    _logger.debug() << ">> " << msg;

    client_base::send_message(msg);

    std::size_t n = irc::to_string(msg).size();

    _bytes_sent += n;
    _bytes_sent_sess += n;
}



void luna::read_config(std::string const& filename)
{
    mond::state s;

    _logger.info() << "Reading configuration `" << filename << "'";

    s.load_file(filename);

    if (auto v = s["nick"])            { _nick   = v.get<std::string>(); }
    if (auto v = s["user"])            { _user   = v.get<std::string>(); }
    if (auto v = s["realname"])        { _real   = v.get<std::string>(); }
    if (auto v = s["server_addr"])     { _server = v.get<std::string>(); }
    if (auto v = s["server_password"]) { _pass   = v.get<std::string>(); }
    if (auto v = s["server_port"])     { _port   = v.get<   uint16_t>(); }
    if (auto v = s["ssl"])             { use_ssl(  v.get<      bool>()); }

    if (auto autojoin = s["autojoin"]) {
        _autojoin = autojoin.get<std::vector<std::string>>();
    }

    if (auto scripts = s["scripts"]) {
        _logger.info() << "Loading scripts...";

        for (std::string const& scr : scripts.get<std::vector<std::string>>()) {
            load_script(scr);
        }
    }

    _logger.info() << "Configuration: ";
    _logger.info() << "  nickname...: " << _nick;
    _logger.info() << "  username...: " << _user;
    _logger.info() << "  realname...: " << _real;
    _logger.info() << "  server host: " << _server;
    _logger.info() << "  server port: " << _port;
    _logger.info() << "  use SSL....: " << (use_ssl() ? "yes" : "no");

    dispatch_signal("configuration_loaded");
}


void luna::read_shared_vars(const std::string& filename)
{
    _logger.info() << "Reading shared variables";

    std::ifstream shared{filename};

    if (not shared) {
        _logger.error()
            << "  Error loading shared variables: " << std::strerror(errno);

        return;
    }

    std::string line;
    while (std::getline(shared, line)) {
        std::istringstream linestrm{line};

        std::string key;
        std::string value;

        linestrm >> std::quoted(key) >> std::quoted(value);

        std::string rvalue;

        rvalue.reserve(value.size());

        for (auto i  = std::begin(value);
                  i != std::end(value);
                ++i) {
            if (*i == '\\') {
                switch (*++i) {
                case 'n':  rvalue.push_back('\n'); break;
                case 'r':  rvalue.push_back('\r'); break;
                case '\\': rvalue.push_back('\\'); break;
                default:   rvalue.push_back('\\'); rvalue.push_back(*i);
                }
            } else {
                rvalue.push_back(*i);
            }
        }

        luna_script::shared_vars[key] = rvalue;
    }
}

void luna::read_users(const std::string& filename)
{
    _logger.info() << "Reading userlist `" << filename << "'";

    std::ifstream userlist{filename};

    if (not userlist) {
        _logger.error() << "  Error loading userlist: " << std::strerror(errno);
        return;
    }

    int lineno = 0;

    std::string line;
    while (std::getline(userlist, line)) {
        ++lineno;

        if (line.empty()) {
            continue;
        }

        std::istringstream linestrm{line};

        std::string id;
        std::string hostmask;
        std::string flags;
        std::string title;

        linestrm >> id >> hostmask >> flags >> std::quoted(title);

        if (id.empty() or hostmask.empty() or flags.empty() or title.empty()) {
            _logger.warn()
                << "  Bad user entry on line " << lineno << ", skipping";
            continue;
        }

        _logger.info() << "  Loaded user: " << "`" << id << "': " << hostmask;

        _users.emplace_back(id, hostmask, title, flags_from_string(flags));
    }
}


void luna::save_shared_vars(const std::string& filename)
{
    std::ofstream shared{filename};

    for (auto const& i : luna_script::shared_vars) {
        std::string val;

        val.reserve(i.second.size());

        for (char c : i.second) {
            switch (c) {
            case '\r': val.append("\\r");  break;
            case '\n': val.append("\\n");  break;
            case '\\': val.append("\\\\"); break;
            default:   val.push_back(c);
            }
        }

        shared << std::quoted(i.first) << " "
               << std::quoted(val)     << std::endl;
    }
}

void luna::save_users(const std::string& filename)
{
    std::ofstream userlist{filename};

    for (luna_user const& user : _users) {
        userlist << user.id()                     << " "
                 << user.hostmask()               << " "
                 << flags_to_string(user.flags()) << " "
                 << std::quoted(user.title())
                 << std::endl;
    }
}


std::vector<luna_script>& luna::scripts()
{
    return _scripts;
}

std::vector<luna_user>& luna::users()
{
    return _users;
}



void luna::run()
{
    if (_server.empty()) {
        throw std::runtime_error{"no server hostname configured"};
    }

    run(_server, _port);
}


void luna::load_script(const std::string& script)
{
    try {
        luna_script s{*this, script};

        _logger.info()
            << "  Loaded script `" << s.name() << "': " << s.description()
            << " (version " << s.version() << ", priority "
            << s.priority() << ")";

        _scripts.push_back(std::move(s));
        _scripts.back().init_script();

        std::sort(std::begin(_scripts), std::end(_scripts));

    } catch (mond::runtime_error const& e) {
        _logger.error() << "  Could not load script `" << script << "': "
                        << "Lua error: " << e.what();
    } catch (mond::error const& e) {
        _logger.error() << "  Could not load script `" << script << "': "
                        << e.what();
    }
}


void luna::on_message(irc::message const& msg)
{
    _logger.debug() << "<< " << msg;

    client_base::on_message(msg);
}



void luna::on_raw(const irc::message& msg)
{
    client_base::on_raw(msg);

    std::size_t n = irc::to_string(msg).size();
    _bytes_recvd += n;
    _bytes_recvd_sess += n;

    dispatch_signal("raw", msg.prefix, irc::to_string(msg.command), msg.args);

    if (msg.command == irc::command::RPL_ENDOFWHO) {
        dispatch_signal("self_join",
            get_channel_user_proxy(msg.args[0], msg.args[1]),
            get_channel_proxy(msg.args[1]));
    }
}

void luna::on_idle()
{
    client_base::on_idle();

    dispatch_signal("tick");
}


void luna::on_invite(std::string const& source, std::string const& channel)
{
    dispatch_signal("invite", source, channel);
}

void luna::on_join(std::string const& source, std::string const& channel)
{
    if (not is_me(source)) {
        dispatch_signal("user_join",
            get_channel_user_proxy(source, channel),
            get_channel_proxy(channel));
    }
}

void luna::on_part(
    std::string const& source,
    std::string const& channel,
    std::string const& reason)
{
    if (not is_me(source)) {
        dispatch_signal("user_part",
            source,
            get_channel_proxy(channel),
            reason);
    } else {
        dispatch_signal("self_part",
            source,
            channel,
            reason);
    }
}

void luna::on_quit(std::string const& source, std::string const& reason)
{
    dispatch_signal("user_quit", source, reason);
}

void luna::on_nick(std::string const& source, std::string const& new_nick)
{
    dispatch_signal("nick_change", source, new_nick);
}

void luna::on_kick(
    std::string const& source,
    std::string const& channel,
    std::string const& kicked,
    std::string const& reason)
{
    dispatch_signal("user_kick",
        get_channel_user_proxy(source, channel),
        get_channel_proxy(channel),
        kicked,
        reason);
}

void luna::on_topic(
    std::string const& source,
    std::string const& channel,
    std::string const& new_topic)
{
    dispatch_signal("topic_change",
        get_channel_user_proxy(source, channel),
        get_channel_proxy(channel),
        new_topic);
}

void luna::on_privmsg(
    std::string const& source,
    std::string const& target,
    std::string const& msg)
{
    if (environment().is_channel(target)) {
        std::string trigger = nick() + ": ";

        if (irc::rfc1459_equal(msg.substr(0, trigger.size()), trigger)) {
            handle_core_commands(source, target, msg.substr(trigger.size()));

            dispatch_signal("channel_command",
                get_channel_user_proxy(source, target),
                get_channel_proxy(target),
                msg.substr(trigger.size()));
        } else {
            dispatch_signal("channel_message",
                get_channel_user_proxy(source, target),
                get_channel_proxy(target),
                msg);
       }
    } else {
        dispatch_signal("message", source, msg);
    }
}

void luna::on_notice(
    std::string const& source,
    std::string const& target,
    std::string const& msg)
{
    if (environment().is_channel(target)) {
        dispatch_signal("channel_notice",
            get_channel_user_proxy(source, target),
            get_channel_proxy(target),
            msg);
    } else {
        dispatch_signal("notice", source, msg);
    }
}

void luna::on_ctcp_request(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
    handle_core_ctcp(source, target, ctcp, args);

    if (environment().is_channel(target)) {
        dispatch_signal("channel_ctcp_request",
            get_channel_user_proxy(source, target),
            get_channel_proxy(target),
            ctcp,
            args);
    } else {
        dispatch_signal("ctcp_request", source, ctcp, args);
    }
}

void luna::on_ctcp_response(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
    if (environment().is_channel(target)) {
        dispatch_signal("channel_ctcp_response",
            get_channel_user_proxy(source, target),
            get_channel_proxy(target),
            ctcp,
            args);
    } else {
       dispatch_signal("ctcp_response", source, ctcp, args);
    }
}

void luna::on_mode(
    std::string const& source,
    std::string const& target,
    std::string const& mode,
    std::string const& arg)
{
    if (environment().is_channel(target)) {
        dispatch_signal("channel_mode",
            get_channel_user_proxy(source, target),
            get_channel_proxy(target),
            mode,
            arg);
    }
}

void luna::on_connect()
{
    client_base::on_connect();

    _logger.info()
        << "Connected to " << server_host() << ":" << server_port() << " "
        <<             "(" << server_addr() << ":" << server_port() << ")";

    for (auto& channel : _autojoin) {
        send_message(irc::join(channel));
    }

    _connected = std::time(nullptr);
    dispatch_signal("connect");
}

void luna::on_disconnect()
{
    _logger.info() << "Disconnected.";

    dispatch_signal("disconnect");
    _connected = 0;

    _bytes_recvd_sess = 0;
    _bytes_sent_sess = 0;

    client_base::on_disconnect();
}


void luna::pretty_print_exception(std::exception_ptr p, int lvl) const
{
    std::string prefix;

    if (lvl > 0) {
        prefix = " `- ";
    }

    try {
        std::rethrow_exception(p);
    } catch (irc::protocol_error const& pe) {
        _logger.error() << prefix << "[Protocol error] " << pe.what();
    } catch (irc::connection_error const& ce) {
        _logger.error() << prefix << "[Connection error] " << ce.what();
    } catch (mond::error const& e) {
        _logger.error() << prefix << "[Lua error] " << e.what();
    } catch (std::runtime_error const& r) {
        _logger.error() << prefix << "[Runtime Error] " << r.what();
    } catch (std::exception const& e) {
        _logger.error() << prefix << "[Exception] " << e.what();
    } catch (...) {
        _logger.error() << prefix << "[???] ?";
    }
}


void luna::handle_core_commands(
    std::string const& prefix,
    std::string const& channel,
    std::string const& msg)
{
    auto r = std::find_if(std::begin(_users), std::end(_users),
        [&] (luna_user const& u) {
            return u.matches(prefix)
               and u.flags() & luna_user::flag::flag_owner;
        });

    if (r == std::end(_users)) {
        return;
    }

    auto do_load = [this] (std::string const& script) {
        luna_script s{*this, script};

        _scripts.push_back(std::move(s));
        _scripts.back().init_script();

        std::sort(std::begin(_scripts), std::end(_scripts));
    };

    auto do_unload = [this] (std::string const& script) {
        for (auto it  = std::begin(_scripts);
                  it != std::end(_scripts);
                ++it) {

            if (irc::rfc1459_equal(it->file(), script)) {
                _scripts.erase(it);
                std::sort(std::begin(_scripts), std::end(_scripts));

                return;
            }
        }

        throw std::runtime_error{"no such script"};
    };


    std::vector<std::string> args = split_noempty(msg, " ");

    if (irc::rfc1459_equal(args[0], "load")) {
        if (args.size() < 2) {
            return send_message(
                irc::response(prefix, channel, "usage: load <script>."));
        }

        try {
            do_load(args[1]);

            send_message(irc::response(prefix, channel, "Script loaded."));
        } catch (std::runtime_error const& e) {
            report_error(std::current_exception());

            send_message(irc::response(prefix, channel,
                "Error loading script :("));
        }
    } else if (irc::rfc1459_equal(args[0], "unload")) {
        if (args.size() < 2) {
            return send_message(
                irc::response(prefix, channel, "usage: unload <script>."));
        }

        try {
            do_unload(args[1]);

            send_message(irc::response(prefix, channel, "Script unloaded."));
        } catch (std::runtime_error const& e) {
            report_error(std::current_exception());

            send_message(irc::response(prefix, channel,
                "Error unloading script. :("));
        }
    } else if (irc::rfc1459_equal(args[0], "reload")) {
        if (args.size() < 2) {
            return send_message(
                irc::response(prefix, channel, "usage: reload <script>."));
        }

        try {
            do_unload(args[1]);
            do_load(args[1]);

            send_message(irc::response(prefix, channel, "Script reloaded."));
        } catch (std::runtime_error const& e) {
            report_error(std::current_exception());

            send_message(irc::response(prefix, channel,
                "Error reloading script. :("));
        }
    } else if (irc::rfc1459_equal(args[0], "reloadusers")) {
        _users.clear();

        read_users(_userfile);

        std::ostringstream rsp;

        rsp << "reloaded " << _users.size() << " users.";
        send_message(irc::response(prefix, channel, rsp.str()));
    }
}


void luna::handle_core_ctcp(
    std::string const& prefix,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
    std::string const& rtarget = environment().is_channel(target)
        ? target
        : irc::normalize_nick(prefix);

    if (irc::rfc1459_equal(ctcp, "VERSION")) {
        send_message(irc::ctcp_response(rtarget, "VERSION", "Luna++ vTau"));
    } else if (irc::rfc1459_equal(ctcp, "PING")) {
        // :)
        send_message(irc::ctcp_response(rtarget, "PING", "0"));
    } else if (irc::rfc1459_equal(ctcp, "TIME")) {
        std::time_t now = std::time(nullptr);
        char timestamp[128] = {};

        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S",
            std::localtime(&now));

        send_message(irc::ctcp_response(rtarget, "TIME", timestamp));
    }
}



luna* lref = nullptr;

void cleanup(int sig)
{
    lref->disconnect("Ctrl-C :(");
    lref->stop();
}

#include <cassert>

int main(int argc, char** argv)
{
    luna cl{"Luna^", "Luna", "I am a robot beep boop"};
    lref = &cl;

    signal(SIGINT, cleanup);

    cl.run();
    cl.save_users("users.txt");

    return 0;
}
