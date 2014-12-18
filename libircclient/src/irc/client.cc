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

#include "irc/client.hh"

#include "irc/irc_core.hh"
#include "irc/irc_except.hh"
#include "irc/irc_helpers.hh"
#include "irc/irc_utils.hh"
#include "irc/connection.hh"
#include "irc/environment.hh"
#include "irc/channel.hh"
#include "irc/channel_user.hh"

#include <ctime>
#include <cstddef>

#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <tuple>
#include <vector>
#include <functional>
#include <array>
#include <future>
#include <exception>
#include <chrono>
#include <utility>


namespace irc {

client::client(
    std::string nick,
    std::string user,
    std::string realname,
    std::string pass)

    : _pass{std::move(pass)},
      _nick{std::move(nick)},
      _user{std::move(user)},
      _real{std::move(realname)},

      _idle_timer{_io_service},
      _irccon{nullptr},
      _ircenv{nullptr}
{
    init_core_handlers();
}

client::~client()
{
    if (connected()) {
        disconnect("So long, and thanks for all the fish.");
    }
}

void client::run(std::string const& host, uint16_t port, bool ssl)
{
    if (connected()) {
        return;
    }

    for (;;) {
        _session_state = session_state::start;
        _current_handler = &client::login_handler;

        int flags = _use_ssl ? connection_flags::SSL : 0;

        _irccon.reset(new irc::async_connection{_io_service, flags});
        _ircenv.reset(new irc::environment{});

        _irccon->connect(host, port, [this] (auto ep) {
            _idle_timer.expires_from_now(_idle_interval);
            _idle_timer.async_wait(
                [this] (boost::system::error_code const& err) {
                    if (not err) {
                        // GCC wants `this'
                        this->do_idle();
                    }
                });

            _irccon->read_message([this] (message const& msg) {
                // GCC wants `this'
                this->handle_message(msg);
            });

            // prefix irc:: to these helpers to disambiguate from our
            // member functions
            if (not _pass.empty()) {
                this->send_message(irc::pass(_pass));
            }

            this->send_message(irc::nick(_nick));
            this->send_message(irc::user(_user, "0", _real));

            this->_session_state = session_state::login_sent;
        });


        try {
            _io_service.run();

        } catch (std::exception const& e)  {
            _idle_timer.cancel();
            report_error(std::current_exception());
        }

        _io_service.reset();

        if (_session_state == session_state::stop) {
            return;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void client::stop()
{
    _session_state = session_state::stop;
}


void client::disconnect(std::string reason)
{
    if (connected()) {
        send_message(message{"", command::QUIT, {reason}});
    }
}

void client::change_nick(std::string const& nick)
{
    // If we're connected, we can't change the nick nilly-willy.
    if (connected()) {
        send_message(message{"", command::NICK, {nick}});

        // If we're not logged yet, we won't get the nick confirmation message
        // that we normally use to update the nickname
        if (_session_state != session_state::logged_in) {
            _nick = nick;
        }
    } else {
        _nick = nick;
    }
}

void client::send_message(message const& msg)
{
    if (not connected()) {
        throw connection_error{connection_error_type::not_connected,
            "send_message"};
    }

    _write_queue.push(msg);

    // If there is now only one element in the queue, start the sending process
    if (_write_queue.size() == 1) {
        _irccon->send_message(msg, [this] (std::size_t s) {
            _write_queue.pop();

            send_queue();
        });
    }
}

bool client::connected() const
{
    return _irccon and _irccon->connected();
}


std::string client::server_host() const
{
    if (not connected()) {
        throw connection_error{connection_error_type::not_connected,
            "server_host"};
    }

    return _irccon->server_host();
}

std::string client::server_addr() const
{
    if (not connected()) {
        throw connection_error{connection_error_type::not_connected,
            "server_addr"};
    }

    return _irccon->server_addr();
}

uint16_t client::server_port() const
{
    if (not connected()) {
        throw connection_error{connection_error_type::not_connected,
            "server_port"};
    }

    return _irccon->server_port();
}


environment const& client::environment() const
{
    return *_ircenv;
}


std::string client::nick() const
{
    return _nick;
}

std::string client::user() const
{
    return _user;
}

std::string client::realname() const
{
    return _real;
}

std::string client::password() const
{
    return _pass;
}


void client::use_ssl(bool setting)
{
    _use_ssl = setting;
}

bool client::use_ssl() const
{
    return _use_ssl;
}


bool client::is_me(std::string user) const
{
    return rfc1459_equal(normalize_nick(std::move(user)), _nick);
}


void client::on_message(message const& msg) {}
void client::on_idle()                      {}
void client::on_connect()                   {}
void client::on_disconnect()                {}


void client::pretty_print_exception(std::exception_ptr p, int lvl) const
{
    if (lvl > 0) {
        std::cerr << " `- ";
    }

    try {
        std::rethrow_exception(p);
    } catch (protocol_error const& pe) {
        std::cerr << "[Protocol error] " << pe.what() << std::endl;
    } catch (connection_error const& ce) {
        std::cerr << "[Connection error] " << ce.what() << std::endl;
    } catch (std::runtime_error const& r) {
        std::cerr << "[Runtime Error] " << r.what() << std::endl;
    } catch (std::exception const& e) {
        std::cerr << "[Exception] " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[Unknown ?" << std::endl;
    }
}

void client::report_error(std::exception_ptr p, int lvl) const
{
    pretty_print_exception(p, lvl);

    try {
        try {
            std::rethrow_exception(p);
        } catch (std::exception const& e) {
            std::rethrow_if_nested(e);
        }
    } catch (std::exception const&  e) {
        report_error(std::current_exception(), lvl + 1);
    }
}


void client::set_idle_interval(int ms)
{
    _idle_interval = boost::posix_time::milliseconds(ms);
}


void client::send_queue()
{
    if (_write_queue.empty() or not connected()) {
        return;
    }

    _irccon->send_message(_write_queue.front(), [this] (std::size_t siz) {
            _write_queue.pop();

            send_queue();
        });
}


void client::handle_message(message const& msg)
{
    try {
        (this->*_current_handler)(msg);

        if (_irccon) {
            _irccon->read_message([this] (message const& msg) {
                handle_message(msg);
            });
        }
    } catch (protocol_error& pe) {
        report_error(std::current_exception());

    } catch (connection_error& ce) {
        std::throw_with_nested(connection_error{ce.code(),
            "handler for: `" + to_string(msg) + "'"});
    }
}


void client::do_disconnect()
{
    _idle_timer.cancel();

    if (_irccon) {
        _irccon->disconnect();
        _irccon.reset(nullptr);
    }

    if (_session_state >= session_state::logged_in) {
        on_disconnect();
    }
}

void client::do_idle()
{
    on_idle();

    _idle_timer.expires_from_now(_idle_interval);
    _idle_timer.async_wait([this] (boost::system::error_code const& err) {
        if (not err) {
            do_idle();
        }
    });
}


void client::login_handler(message const& msg)
{
    if (rfc1459_equal(msg.command, command::ERR_NICKNAMEINUSE)) {
        change_nick(nick() + "_");
    } else if (rfc1459_equal(msg.command, command::RPL_WELCOME)) {
        _current_handler = &client::main_handler;

        if (_session_state != session_state::stop) {
            _session_state = session_state::logged_in;
        }

        (this->*_current_handler)(msg);
    } else if (rfc1459_equal(msg.command, command::PING)) {
        send_message(message{"", command::PONG, {msg.args[0]}});
    } else if (rfc1459_equal(msg.command, command::ERROR)) {
        do_disconnect();
    }
}

void client::main_handler(message const& msg)
{
    auto const& res = _core_handlers.find(msg.command);
    if (res != std::end(_core_handlers) and not std::get<2>(res->second)) {
        run_core_handler(res->second, msg);
    }

    try {
        on_message(msg);
    } catch (protocol_error& pe) {
        report_error(std::current_exception());

    } catch (connection_error& ce) {
        std::throw_with_nested(
            connection_error{ce.code(),
                "error in user handler `" + msg.command + "'"});

    } catch (...) {
        report_error(std::current_exception());
    }


    if (res != std::end(_core_handlers) and std::get<2>(res->second)) {
        run_core_handler(res->second, msg);
    }
}


void client::run_core_handler(
    client::handler const& handler,
    message const& msg)
{
    if (auto& fun = std::get<3>(handler)) {
        bool requires_user = std::get<1>(handler);

        if (not (requires_user and not is_user_prefix(msg.prefix))) {
            std::string err =
                "error in core handler for `" + msg.command + "'";

            try {
                auto args_req = std::get<0>(handler);

                if (msg.args.size() < args_req) {
                    std::ostringstream errs;

                    errs << "expected " << args_req << " arguments "
                            << "for `" << msg.command << "' handler, "
                            << "got " << msg.args.size();

                    // Just throw it here so we can print a pretty message
                    // down there.
                    throw protocol_error{
                        protocol_error_type::not_enough_arguments,
                        errs.str()};
                }

                fun(msg);
            } catch (protocol_error& p) {
                // protocol errors are not fatal, report and move on
                report_error(std::current_exception());

            } catch (connection_error& c) {
                // but connection errors and unknown errors are fatal!
                std::throw_with_nested(connection_error{c.code(), err});

            } catch (...) {
                std::throw_with_nested(std::runtime_error{err});

            }
        }
    }
}


void client::init_core_handlers()
{
    // Core event handlers responsible for state and housekeeping
    _core_handlers[command::RPL_WELCOME] = handler{ 1, false, false,
        [this](message const& msg) {
            _nick = msg.args[0];
            on_connect();
        }
    };

    _core_handlers[command::RPL_ISUPPORT] = handler{ 1, false, false,
        // me, _core_handlers[args]
        [this](message const& msg) {
            for (auto iter  = std::begin(msg.args) + 1;
                      iter != std::end(msg.args)   - 1;
                    ++iter) {
                if (iter->find('=') == std::string::npos) {
                    _ircenv->set_capability(*iter, "");
                } else {
                    _ircenv->set_capability(
                        iter->substr(0, iter->find('=')),
                        iter->substr(iter->find('=') + 1));
                }
            }
        }
    };

    _core_handlers[command::RPL_TOPIC] = handler{ 3, false, false,
        // me, channel, topic
        [this](message const& msg) {
            auto& channel = _ircenv->find_channel(msg.args[1]);
            channel.set_topic(msg.args[2]);
        }
    };

    _core_handlers[command::RPL_TOPICWHOTIME] = handler{ 4, false, false,
        // me, channel, creator, time
        [this](message const& msg) {
            auto& channel = _ircenv->find_channel(msg.args[1]);
            channel.set_topic_meta(
                normalize_nick(msg.args[2]),
                std::stoll(msg.args[3]));
        }
    };

    _core_handlers[command::RPL_CHANNELMODEIS] = handler{ 3, false, false,
        // me, channel, mode, _core_handlers[mode_args]
        [this](message const& msg) {
            auto& channel = _ircenv->find_channel(msg.args[1]);

            channel.apply_modes(
                msg.args[2],
                {std::begin(msg.args) + 3, std::end(msg.args)},
                *_ircenv);
        }
    };

    _core_handlers[command::RPL_CREATIONTIME] = handler{ 3, false, false,
        // me, channel, ctime
        [this](message const& msg) {
            auto& channel = _ircenv->find_channel(msg.args[1]);
            channel.set_created(std::stoll(msg.args[2]));
        }
    };

    _core_handlers[command::RPL_WHOREPLY] = handler{ 7, false, false,
        // me, channel, user, host, server, nick, mode, <...>
        [this](message const& msg) {
            auto& channel = _ircenv->find_channel(msg.args[1]);

            auto& user = channel.create_user(
                msg.args[5],
                msg.args[2],
                msg.args[3]);

            auto prefixes = _ircenv->prefixes();

            for (char c : msg.args[6]) {
                if (prefixes.find(c) != std::end(prefixes)) {
                    channel.apply_modes(
                        "+" + std::string{prefixes.at(c)},
                        {user.nick()}, *_ircenv);
                }
            }
        }
    };

    _core_handlers[command::RPL_BANLIST] = handler{ 3, false, false,
        // me, channel, entry
        [this](message const& msg) {
            auto& channel = _ircenv->find_channel(msg.args[1]);

            channel.apply_modes("+b", {msg.args[2]}, *_ircenv);
        }
    };

    _core_handlers[command::PING] = handler{ 1, false, false,
        // server
        [this](message const& msg) {
            send_message(message{"", command::PONG, {msg.args[0]}});
        }
    };

    // Channel user events
    _core_handlers[command::JOIN] = handler{ 1, true, false,
        // channel
        [this](message const& msg) {
            // me? add channel. not me? add user to channel.
            if (is_me(msg.prefix)) {
                auto& channel = _ircenv->create_channel(msg.args[0]);

                channel.create_user(msg.prefix);

                send_message(message{"", command::WHO,  {msg.args[0]}});
                send_message(message{"", command::MODE, {msg.args[0]}});
                send_message(message{"", command::MODE, {msg.args[0], "+b"}});
            } else {
                _ircenv->find_channel(msg.args[0]).create_user(msg.prefix);
            }
        }
    };

    _core_handlers[command::PART] = handler{ 1, true, true,
        // channel, [reason]
        [this](message const& msg) {
            // me? drop channel. not me? remove user from channel.
            auto& channel = _ircenv->find_channel(msg.args[0]);

            if (is_me(msg.prefix)) {
                _ircenv->remove_channel(channel);
            } else {
                channel.remove_user(channel.find_user(msg.prefix));
            }
        }
    };

    _core_handlers[command::KICK] = handler{ 2, false, true,
        // channel, kicked, reason
        [this](message const& msg) {
            // me? drop channel. not me? remove user from channel.
            auto& channel = _ircenv->find_channel(msg.args[0]);

            if (is_me(msg.args[1])) {
                _ircenv->remove_channel(channel);
            } else {
                channel.remove_user(channel.find_user(msg.args[1]));
            }
        }
    };

    _core_handlers[command::QUIT] = handler{ 0, true, true,
        // [reason]
        [this](message const& msg) {
            // me? unlikely. not me? remove user from all channels.
            if (not is_me(msg.prefix)) {
                auto& channels = _ircenv->channels();

                for (auto& i : channels) {
                    if (i.second->has_user(msg.prefix)) {
                        i.second->remove_user(
                            i.second->find_user(msg.prefix));
                    }
                }
            } else {
                do_disconnect();
            }
        }
    };

    _core_handlers[command::ERROR] = handler{ 1, false, false,
        [this](message const& msg) {
            do_disconnect();
        }
    };

    _core_handlers[command::NICK] = handler{ 1, true, true,
        // new nick
        [this](message const& msg) {
            // me? rename. not me or me? rename user in all channels.
            if (is_me(msg.prefix)) {
                _nick = msg.args[0];
            }

            auto& channels = _ircenv->channels();

            for (auto& i : channels) {
                if (i.second->has_user(msg.prefix)) {
                    i.second->rename_user(msg.prefix, msg.args[0]);
                }
            }
        }
    };

    // Channel events
    _core_handlers[command::TOPIC] = handler{ 2, false, false,
        // channel, new topic
        [this](message const& msg) {
            // set topic info in channel
            auto& channel = _ircenv->find_channel(msg.args[0]);

            channel.set_topic(msg.args[1]);
            channel.set_topic_meta(
                normalize_nick(msg.prefix),
                std::time(nullptr));
        }
    };

    _core_handlers[command::MODE] = handler{ 2, false, false,
        // channel, modestring, [args]
        [this](message const& msg) {
            if (not _ircenv->is_channel(msg.args[0])) {
                return;
            }

            // set modes in channel
            // msg.args is guaranteed to be at least of size 2, so
            // adding 2 to begin() would put us at end() if args were empty,
            // which is legal as long as we don't deref it.
            _ircenv->find_channel(msg.args[0]).apply_modes(
                msg.args[1],
                {std::begin(msg.args) + 2, std::end(  msg.args)},
                *_ircenv);
        }
    };
}

}
