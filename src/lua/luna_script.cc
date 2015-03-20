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

#include "luna_script.hh"

#include "luna_extension.hh"

#include "lua/proxies/luna_channel_proxy.hh"
#include "lua/proxies/luna_extension_proxy.hh"
#include "lua/proxies/luna_user_proxy.hh"

#include "config.hh"
#include "luna_user.hh"
#include "logging.hh"
#include "luna.hh"

#include <mond/mond.hh>

#include <irc/irc_core.hh>
#include <irc/irc_except.hh>

#include <sstream>
#include <string>
#include <chrono>
#include <stdexcept>
#include <regex>

luna_script::luna_script(luna& context, std::string file)
    : luna_extension{context},
      _file{std::move(file)}
{
    constexpr char const* extra_paths =
        "./scripts/?.lua;"
        "./scripts/?/init.lua;"
        "./scripts/lib/?.lua;"
        "./scripts/lib/?/init.lua;";

    _lua["package"]["path"].set(
        extra_paths + _lua["package"]["path"].get<std::string>());

    _lua.load_library(_file, "script");

    try {
        _script_name =
            _lua["script"]["info"]["name"].get<std::string>();

        _script_descr =
            _lua["script"]["info"]["description"].get<std::string>();

        _script_version =
            _lua["script"]["info"]["version"].get<std::string>();

    } catch (mond::error const& e) {
        std::throw_with_nested(mond::error{"invalid script info table"});
    }

    _logger = logger{name() + " (script)",
        logging_level::DEBUG,
        logging_flags::ANSI};

    setup_api();
    _lua.load_library("luna", "luna");
}


luna_script::~luna_script()
{
    destroy();
}


std::string luna_script::id() const
{
    return _file;
}


std::string luna_script::name() const
{
    return _script_name;
}

std::string luna_script::description() const
{
    return _script_descr;
}

std::string luna_script::version() const
{
    return _script_version;
}


bool operator==(luna_script const& a, luna_script const& b)
{
    return a._file == b._file;
}

bool operator!=(luna_script const& a, luna_script const& b)
{
    return !(a == b);
}


void luna_script::setup_api()
{
    _lua["os"]["time_ms"] = std::function<double (void)>{[] {
            return std::chrono::duration_cast<
                std::chrono::milliseconds>(
                    std::chrono::system_clock::now()
                        .time_since_epoch())
                            .count();
        }};

    _lua["os"]["exit"] = std::function<void (int)>{[] (int c) {
            throw mond::runtime_error{"nope"};
        }};

    _lua["string"]["rfc1459lower"] = std::function<std::string (std::string)>{
            [] (std::string str) {
                return irc::rfc1459_lower(std::move(str));
            }};

    _lua["string"]["rfc1459upper"] = std::function<std::string (std::string)>{
            [] (std::string str) {
                return irc::rfc1459_upper(std::move(str));
            }};

    _lua["log"] = mond::table{};
    _lua["log"]["log"] = std::function<int (lua_State*)>{
        [=, _logger = this->_logger] (lua_State* s) {
            std::string level = luaL_checkstring(s, 1);
            std::string msg = luaL_checkstring(s, 2);

            if (irc::rfc1459_equal(level, "debug")) {
                _logger.debug() << msg;
            } else if (irc::rfc1459_equal(level, "info")) {
                _logger.info() << msg;
            } else if (irc::rfc1459_equal(level, "warn")) {
                _logger.warn() << msg;
            } else if (irc::rfc1459_equal(level, "error")) {
                _logger.error() << msg;
            } else if (irc::rfc1459_equal(level, "wtf")) {
                _logger.wtf() << msg;
            }

            return 0;
        }};

    _lua[api] = mond::table{};

    register_environment();

    register_channel();
    register_channel_user();

    register_self();
    register_script();
    register_user();
}


void luna_script::register_environment()
{
    _lua[api]["environment"] = mond::table{};
    _lua[api]["environment"]["server_supports"] =
        std::function<int (lua_State*)>{
            [=] (lua_State* s) {
                lua_newtable(s);

                for (auto& v : context().environment().capabilities()) {
                    lua_pushstring(s, v.second.c_str());
                    lua_setfield(s, -2, v.first.c_str());
                }

                return 1;
            }};

    _lua[api]["shared"] = mond::table{};

    _lua[api]["shared"]["save"] = std::function<void ()>{[=] {
            context().save_shared_vars(varfile);
        }};

    _lua[api]["shared"]["reload"] = std::function<void ()>{[=] {
            luna_script::shared_vars.clear();

            context().read_shared_vars(varfile);
        }};

    _lua[api]["shared"]["get"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            std::string key = luaL_checkstring(s, 1);

            if (shared_vars.find(key) != std::end(shared_vars)) {
                return mond::write(s, shared_vars[key]);
            } else {
                return mond::write(s, mond::nil{});
            }
        }};

    _lua[api]["shared"]["set"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            std::string key = luaL_checkstring(s, 1);

            std::size_t n;
            char const* val = luaL_checklstring(s, 2, &n);

            shared_vars[key] = std::string{val, n};

            return 0;
        }};

    _lua[api]["shared"]["clear"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            std::string key = luaL_checkstring(s, 1);

            shared_vars.erase(key);
            context().save_shared_vars(varfile);

            return 0;
        }};

    _lua[api]["shared"]["list"] =
        std::function<int (lua_State*)>{[=] (lua_State* s) {
            lua_newtable(s);

            for (auto const& kv : shared_vars) {
                mond::write(s, kv.second);
                lua_setfield(s, -2, kv.first.c_str());
            }

            return 1;
        }};
}

void luna_script::register_self()
{
    _lua[api]["send_message"] = std::function<int (lua_State* s)>{
        [=] (lua_State* s) {
            irc::message msg;

            std::string cmd = luaL_checkstring(s, 1);

            try {
                msg.command = irc::rfc1459_upper(cmd);
            } catch (irc::protocol_error const& pe) {
                std::throw_with_nested(mond::runtime_error{
                    "invalid command: " + cmd});
            }

            int n = lua_gettop(s) - 1;

            for (int i = 2; i < (2 + n); ++i) {
                if (not lua_isnil(s, i)) {
                    msg.args.push_back(luaL_checkstring(s, i));
                }
            }

            context().send_message(msg);

            return 0;
        }};

    _lua[api]["runtime_info"] =
        std::function<std::tuple<std::time_t, std::time_t> ()>{[=] {
            return std::make_tuple(context()._started, context()._connected);
        }};

    _lua[api]["user_info"] =
        std::function<std::tuple<std::string, std::string> ()>{[=] {
            return std::make_tuple(context().nick(), context().user());
        }};

    _lua[api]["server_info"] =
        std::function<std::tuple<std::string, std::string, uint16_t> ()>{[=] {
            return std::make_tuple(
                context().server_host(),
                context().server_addr(),
                context().server_port());
        }};

    _lua[api]["traffic_info"] =
        std::function<
            std::tuple<
                std::size_t,
                std::size_t,
                std::size_t,
                std::size_t> ()>{[=] {

            return std::make_tuple(
                context()._bytes_sent,
                context()._bytes_sent_sess,
                context()._bytes_recvd,
                context()._bytes_recvd_sess);
        }};
}

namespace {

bool strcaseequal(std::string const& a, std::string const& b)
{
    if (a.size() != b.size()) {
        return false;
    }

    for (std::size_t i = 0; i < a.size(); ++i) {
        if (::tolower(a[i]) != ::tolower(b[i])) {
            return false;
        }
    }

    return true;
}

} // namespace

void luna_script::register_script()
{
    _lua[api].new_metatable<luna_extension_proxy>()
        << mond::meta_method(mond::meta_tostring, &luna_extension_proxy::id)

        << mond::method("id",          &luna_extension_proxy::id)
        << mond::method("name",        &luna_extension_proxy::name)
        << mond::method("description", &luna_extension_proxy::description)
        << mond::method("version",     &luna_extension_proxy::version)
        << mond::method("is_self",     &luna_extension_proxy::is_self);

    _lua[api]["extensions"] = mond::table{};
    _lua[api]["extensions"]["self"] = mond::object<luna_extension_proxy>(
        context(), _file);

    _lua[api]["extensions"]["list"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            lua_newtable(s);

            for (std::unique_ptr<luna_extension> const& scr :
                    context().extensions()) {

                if (scr) {
                    mond::write(s,
                        mond::object<luna_extension_proxy>(
                            context(), scr->id()));

                    lua_setfield(s, -2, scr->id().c_str());
                }
            }

            return 1;
        }};

    _lua[api]["extensions"]["load"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            std::string scr = luaL_checkstring(s, 1);

            for (std::unique_ptr<luna_extension> const& sc :
                    context().extensions()) {

                if (sc and strcaseequal(scr, sc->id())) {
                    throw mond::error{"script `" + scr + "' already loaded"};
                }
            }

            std::unique_ptr<luna_extension> script{new luna_script{
                context(), scr}};

            context()._exts.push_back(std::move(script));
            context()._exts.back()->init();

            return mond::write(s,
                mond::object<luna_extension_proxy>(context(), scr));
        }};

    _lua[api]["extensions"]["unload"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            std::string scr = luaL_checkstring(s, 1);

            if (strcaseequal(scr, this->_file)) {
                throw mond::error{"can not unload self"};
            }

            for (auto it  = std::begin(context()._exts);
                      it != std::end(context()._exts);
                    ++it) {
                if (*it and strcaseequal(scr, (*it)->id())) {
                    it->reset(nullptr);

                    return 0;
                }
            }

            throw mond::error{"script `" + scr + "' not loaded"};
        }};

    _lua[api]["extension_meta"].export_metatable<luna_extension_proxy>();
}


void luna_script::register_user()
{
    _lua[api].new_metatable<luna_user_proxy>()
        << mond::meta_method(mond::meta_tostring, &luna_user_proxy::id)

        << mond::method("id",           &luna_user_proxy::id)
        << mond::method("hostmask",     &luna_user_proxy::hostmask)
        << mond::method("flags",        &luna_user_proxy::flags)
        << mond::method("title",        &luna_user_proxy::title)
        << mond::method("set_id",       &luna_user_proxy::set_id)
        << mond::method("set_hostmask", &luna_user_proxy::set_hostmask)
        << mond::method("set_flags",    &luna_user_proxy::set_flags)
        << mond::method("set_title",    &luna_user_proxy::set_title);

    _lua[api]["users"] = mond::table{};
    _lua[api]["users"]["list"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            lua_newtable(s);

            for (auto const& u : context().users()) {
                mond::write(s,
                    mond::object<luna_user_proxy>(context(), u.id()));

                lua_setfield(s, -2, u.id().c_str());
            }

            return 1;
        }};

    _lua[api]["users"]["save"] = std::function<void ()>{[=] {
            context().save_users(userfile);
        }};

    _lua[api]["users"]["reload"] = std::function<void ()>{[=] {
            context()._users.clear();
            context().read_users(userfile);
        }};

    _lua[api]["users"]["create"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            std::string id    = luaL_checkstring(s, 1);
            std::string mask  = luaL_checkstring(s, 2);
            std::string flags = luaL_checkstring(s, 3);
            std::string title = luaL_checkstring(s, 4);

            auto i = std::find_if(
                std::begin(context().users()), std::end(context().users()),
                [&] (luna_user& u2) {
                    return irc::rfc1459_equal(u2.id(), id);
                });

            if (i != std::end(context().users())) {
                throw mond::runtime_error{"user `" + id + "' already exists"};
            }

            context().users().emplace_back(
                id, mask, title, flags_from_string(flags));

            return mond::write(s, mond::object<luna_user_proxy>(context(), id));
        }};


    _lua[api]["users"]["remove"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            std::string id = luaL_checkstring(s, 1);

            auto i = std::find_if(
                std::begin(context().users()),
                std::end(context().users()),
                [&] (luna_user& u2) {
                    return irc::rfc1459_equal(u2.id(), id);
                });

            if (i == std::end(context().users())) {
                throw mond::runtime_error{"user `" + id + "' does not exist"};
            }

            context().users().erase(i);
            context().save_users(userfile);

            return 0;
        }};

    _lua[api]["user_meta"].export_metatable<luna_user_proxy>();
}

void luna_script::register_channel()
{
    _lua[api].new_metatable<luna_channel_proxy>()
        << mond::meta_method(mond::meta_tostring, &luna_channel_proxy::name)

        << mond::method("name",              &luna_channel_proxy::name)
        << mond::method("created",           &luna_channel_proxy::created)
        << mond::method("topic",             &luna_channel_proxy::topic)
        << mond::method("users",             &luna_channel_proxy::users)
        << mond::method("find_user",         &luna_channel_proxy::find_user)
        << mond::method("modes",             &luna_channel_proxy::modes);


    _lua[api]["channels"] = mond::table{};
    _lua[api]["channels"]["find"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            try {
                return mond::write(s, mond::object<luna_channel_proxy>(
                    context(),
                    context().environment()
                        .find_channel(luaL_checkstring(s, 1)).name()));

            } catch (irc::protocol_error& pe) {
                return mond::write(s, mond::nil{});
            }
        }};

    _lua[api]["channels"]["list"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            lua_newtable(s);

            for (auto const& entry : context().environment().channels()) {
                mond::write(s, mond::object<luna_channel_proxy>(
                    context(), entry.second->name()));

                lua_setfield(s, -2, entry.second->name().c_str());
            }

            return 1;
        }};

    _lua[api]["channel_meta"].export_metatable<luna_channel_proxy>();
}

void luna_script::register_channel_user()
{
    _lua[api].new_metatable<luna_unknown_user_proxy>()
        << mond::meta_method(mond::meta_tostring,
            &luna_unknown_user_proxy::repr)

        << mond::method("user_info",     &luna_unknown_user_proxy::user_info)
        << mond::method("match_reguser", &luna_unknown_user_proxy::match);


    _lua[api].new_metatable<luna_channel_user_proxy>()
        << mond::meta_method(mond::meta_tostring,
            &luna_channel_user_proxy::repr)

        << mond::method("user_info",     &luna_channel_user_proxy::user_info)
        << mond::method("match_reguser", &luna_channel_user_proxy::match)
        << mond::method("modes",         &luna_channel_user_proxy::modes)
        << mond::method("channel",       &luna_channel_user_proxy::channel);

    _lua[api]["unknown_user_meta"].export_metatable<luna_unknown_user_proxy>();
    _lua[api]["channel_user_meta"].export_metatable<luna_channel_user_proxy>();
}


void luna_script::init()
{
    luna_extension::init();

    _lua["luna"]["init_script"].call();
}

void luna_script::destroy()
{
    if (_lua.valid()) {
        try {
            _lua["luna"]["deinit_script"].call();
        } catch (...) {
            ;
        }
    }

    luna_extension::destroy();
}


void luna_script::on_connect()
{
    luna_extension::on_connect();

    emit_signal("connect");
}

void luna_script::on_disconnect()
{
    luna_extension::on_disconnect();

    emit_signal("disconnect");
}

void luna_script::on_idle()
{
    luna_extension::on_idle();

    emit_signal("tick");
}

void luna_script::on_message(irc::message const& msg)
{
    luna_extension::on_message(msg);

    if (msg.command == irc::command::RPL_ENDOFWHO) {
        emit_signal_helper("user_join", msg.args[0], msg.args[1]);
    }

    emit_signal("raw", msg.prefix, msg.command, msg.args);
}

void luna_script::on_invite(
    std::string const& source,
    std::string const& channel)
{
    luna_extension::on_invite(source, channel);

    emit_signal("invite", get_unknown_user_proxy(source), channel);
}

void luna_script::on_join(
    std::string const& source,
    std::string const& channel)
{
    luna_extension::on_join(source, channel);

    if (not context().is_me(source)) {
        emit_signal_helper("user_join", source, channel);
    }
}

void luna_script::on_part(
    std::string const& source,
    std::string const& channel,
    std::string const& reason)
{
    luna_extension::on_part(source, channel, reason);

    emit_signal_helper("user_part", source, channel, reason);
}

void luna_script::on_quit(
    std::string const& source,
    std::string const& reason)
{
    luna_extension::on_quit(source, reason);

    emit_signal("user_quit", get_unknown_user_proxy(source), reason);
}

void luna_script::on_nick(
    std::string const& source,
    std::string const& new_nick)
{
    luna_extension::on_nick(source, new_nick);

    emit_signal("nick_change", get_unknown_user_proxy(source), new_nick);
}

void luna_script::on_kick(
    std::string const& source,
    std::string const& channel,
    std::string const& kicked,
    std::string const& reason)
{
    luna_extension::on_kick(source, channel, kicked, reason);

    irc::channel const& chan = context().environment().find_channel(channel);

    // Could be chanserv kicking
    if (chan.has_user(source)) {
        emit_signal("channel_user_kick",
            get_channel_user_proxy(chan.find_user(source), chan),
            get_channel_proxy(chan),
            get_channel_user_proxy(chan.find_user(kicked), chan),
            reason);
    } else {
        emit_signal("channel_user_kick",
            get_unknown_user_proxy(source),
            get_channel_proxy(chan),
            get_channel_user_proxy(chan.find_user(kicked), chan),
            reason);
    }
}

void luna_script::on_topic(
    std::string const& source,
    std::string const& channel,
    std::string const& new_topic)
{
    luna_extension::on_topic(source, channel, new_topic);

    emit_signal_helper("topic_change", source, channel, new_topic);
}

bool luna_script::split_message_level(std::string& target, std::string& level)
{
    std::size_t chanstart = target.find_first_of(
        context().environment().channel_types());

    if (chanstart != std::string::npos) {
        if (chanstart != 0) {
            // +#channel, @#channel, @+#channel, ...
            level  = target.substr(0, chanstart);
            target = target.substr(chanstart);
        } else {
            level = "";
        }
    }

    return chanstart != std::string::npos;
}

void luna_script::on_privmsg(
    std::string const& source,
    std::string const& target,
    std::string const& msg)
{
    luna_extension::on_privmsg(source, target, msg);

    std::string rtarget = target, rlevel;

    if (split_message_level(rtarget, rlevel)) {
        emit_signal_helper("message", source, rtarget, msg, rlevel);
    } else {
        emit_signal_helper("message", source, target, msg);
    }
}

void luna_script::on_notice(
    std::string const& source,
    std::string const& target,
    std::string const& msg)
{
    luna_extension::on_notice(source, target, msg);

    std::string rtarget = target, rlevel;

    if (split_message_level(rtarget, rlevel)) {
        emit_signal_helper("notice", source, rtarget, msg, rlevel);
    } else {
        emit_signal_helper("notice", source, target, msg);
    }
}

void luna_script::on_ctcp_request(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
    luna_extension::on_ctcp_request(source, target, ctcp, args);

    std::string rtarget = target, rlevel;

    if (irc::rfc1459_equal(ctcp, "ACTION")) {
        if (split_message_level(rtarget, rlevel)) {
            emit_signal_helper("action", source, rtarget, args, rlevel);
        } else {
            emit_signal_helper("action", source, target, args);
        }
    } else {
        if (split_message_level(rtarget, rlevel)) {
            emit_signal_helper("ctcp_request",
                source, rtarget, ctcp, args, rlevel);
        } else {
            emit_signal_helper("ctcp_request", source, target, ctcp, args);
        }
    }
}

void luna_script::on_ctcp_response(
    std::string const& source,
    std::string const& target,
    std::string const& ctcp,
    std::string const& args)
{
    luna_extension::on_ctcp_response(source, target, ctcp, args);

    std::string rtarget = target, rlevel;

    if (split_message_level(rtarget, rlevel)) {
        emit_signal_helper("ctcp_response",
             source, rtarget, ctcp, args, rlevel);
    } else {
        emit_signal_helper("ctcp_response", source, target, ctcp, args);
    }
}

void luna_script::on_mode(
    std::string const& source,
    std::string const& target,
    std::string const& mode,
    std::string const& arg)
{
    luna_extension::on_mode(source, target, mode, arg);

    emit_signal_helper("mode", source, target, mode, arg);
}
