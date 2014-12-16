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

#include "luna.hh"

#include "config.hh"
#include "luna_user.hh"

#include "luna_extension.hh"
#include "lua/luna_script.hh"

#include "logging.hh"

#include <irc_core.hh>
#include <irc_except.hh>
#include <irc_helpers.hh>
#include <environment.hh>

#include <mond.hh>

#include <lua.hpp>

#include <cstring>
#include <cerrno>
#include <ctime>

#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>


luna::luna(std::string const& cfgfile)
    : client_base{"", "", ""}
{
    read_shared_vars(_varfile);
    read_users(_userfile);

    set_idle_interval(idle_interval);

    luna_extension::shared_vars["luna.version"]  = luna_version;
    luna_extension::shared_vars["luna.compiled"] = __DATE__ " " __TIME__;

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

    luna_extension::shared_vars["luna.compiler"] = compiler.str();

    if (luna_extension::shared_vars.find("luna.trigger") ==
            std::end(luna_extension::shared_vars)) {

        luna_extension::shared_vars["luna.trigger"] = "!";
    }

    read_config(cfgfile);
}


luna::~luna()
{
    save_users(_userfile);
    save_shared_vars(_varfile);
}


void luna::send_message(irc::message const& msg)
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
}


void luna::read_shared_vars(std::string const& filename)
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

        luna_extension::shared_vars[key] = rvalue;
    }
}

void luna::read_users(std::string const& filename)
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


void luna::save_shared_vars(std::string const& filename)
{
    std::ofstream shared{filename};

    for (auto const& i : luna_extension::shared_vars) {
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

void luna::save_users(std::string const& filename)
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


std::vector<std::unique_ptr<luna_extension>> const& luna::extensions()
{
    return _exts;
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


void luna::load_script(std::string const& script)
{
    try {
        std::unique_ptr<luna_script> s{new luna_script{*this, script}};

        _logger.info()
            << "  Loaded script `" << s->name() << "': " << s->description()
            << " (version " << s->version() << ")";

        _exts.push_back(std::move(s));
        _exts.back()->init();

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



void luna::on_raw(irc::message const& msg)
{
    client_base::on_raw(msg);

    std::size_t n = irc::to_string(msg).size();
    _bytes_recvd += n;
    _bytes_recvd_sess += n;

    dispatch_event(&luna_extension::on_message, msg);
}

void luna::on_idle()
{
    client_base::on_idle();

    dispatch_event(&luna_extension::on_idle);
}


void luna::on_invite(std::string const& source, std::string const& channel)
{
    dispatch_event(&luna_extension::on_invite, source, channel);
}

void luna::on_join(std::string const& source, std::string const& channel)
{
    dispatch_event(&luna_extension::on_join, source, channel);
}

void luna::on_part(
    std::string const& source,
    std::string const& channel,
    std::string const& reason)
{
    dispatch_event(&luna_extension::on_part, source, channel, reason);
}

void luna::on_quit(std::string const& source, std::string const& reason)
{
    dispatch_event(&luna_extension::on_quit, source, reason);
}

void luna::on_nick(std::string const& source, std::string const& new_nick)
{
    dispatch_event(&luna_extension::on_nick, source, new_nick);
}

void luna::on_kick(
    std::string const& source,
    std::string const& channel,
    std::string const& kicked,
    std::string const& reason)
{
    dispatch_event(&luna_extension::on_kick, source, channel, kicked, reason);
}

void luna::on_topic(
    std::string const& source,
    std::string const& channel,
    std::string const& new_topic)
{
    dispatch_event(&luna_extension::on_topic, source, channel, new_topic);
}

void luna::on_privmsg(
    std::string const& source,
    std::string const& target,
    std::string const& msg)
{
    dispatch_event(&luna_extension::on_privmsg, source, target, msg);
}

void luna::on_notice(
    std::string const& source,
    std::string const& target,
    std::string const& msg)
{
    dispatch_event(&luna_extension::on_notice, source, target, msg);
}

void luna::on_ctcp_request(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
    handle_core_ctcp(source, target, ctcp, args);

    dispatch_event(&luna_extension::on_ctcp_request,
        source, target, ctcp, args);
}

void luna::on_ctcp_response(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
    dispatch_event(&luna_extension::on_ctcp_response,
        source, target, ctcp, args);
}

void luna::on_mode(
    std::string const& source,
    std::string const& target,
    std::string const& mode,
    std::string const& arg)
{
    dispatch_event(&luna_extension::on_mode, source, target, mode, arg);
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

    dispatch_event(&luna_extension::on_connect);
}

void luna::on_disconnect()
{
    _logger.info() << "Disconnected.";

    dispatch_event(&luna_extension::on_disconnect);

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


void luna::handle_core_ctcp(
    std::string const& prefix,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
    std::string rtarget = prefix;

    if (irc::is_user_prefix(prefix)) {
        std::tie(rtarget, std::ignore, std::ignore) = irc::split_prefix(prefix);
    }


    if (irc::rfc1459_equal(ctcp, "VERSION")) {
        send_message(irc::ctcp_response(rtarget,
             "VERSION", "Luna++ " + std::string{luna_version}));

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
    // Not technically well defined or secure, but as long as this is not
    // running on a pacemaker...
    lref->disconnect("Ctrl-C :(");
    lref->stop();
}

int main(int argc, char** argv)
{
    luna cl{argc > 1 ? argv[1] : "config.lua"};
    lref = &cl;

    signal(SIGINT, cleanup);

    cl.run();
    cl.save_users("users.txt");

    return 0;
}
