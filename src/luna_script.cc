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

#include "luna_script.h"

#include "proxies/luna_proxy.h"
#include "proxies/luna_script_proxy.h"
#include "proxies/luna_channel_proxy.h"
#include "proxies/luna_user_proxy.h"

#include "luna_user.h"
#include "logging.h"
#include "luna.h"

#include "mond.h"

#include <irc_except.h>

#include <sstream>
#include <string>
#include <stdexcept>
#include <regex>

irc::unordered_rfc1459_map<std::string, std::string>
luna_script::shared_vars{};

luna_script::luna_script(luna& context, std::string file)
    : _context{&context},
      _logger{},
      _file{std::move(file)},
      _lua{},
      _script_name{},
      _script_descr{},
      _script_version{},
      _script_priority{0} // Arbitrarily define priority around zero
{
    _lua.load_library(_file, "script");

    try {
        _script_name =
            _lua["script"]["info"]["name"].get<std::string>();

        _script_descr =
            _lua["script"]["info"]["description"].get<std::string>();

        _script_version =
            _lua["script"]["info"]["version"].get<std::string>();

        if (auto prio = _lua["script"]["info"]["priority"]) {
            _script_priority = prio.get<int>();
        }

    } catch (mond::error const& e) {
        std::throw_with_nested(mond::error{"invalid script info table"});
    }

    _logger = logger{
        name() + " (script)",
        logging_level::DEBUG,
        logging_flags::ANSI};

    setup_api();
    _lua.load_library("luna_corelib", "luna");
}


luna_script::~luna_script()
{
    if (_lua.valid()) {
        try {
            _lua["luna"]["deinit_script"].call();
        } catch (...) {
            ;
        }
    }
}


void luna_script::init_script()
{
    _lua["luna"]["init_script"].call();
}


std::string luna_script::file() const
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

int luna_script::priority() const
{
    return _script_priority;
}


bool operator==(luna_script const& a, luna_script const& b)
{
    return a._file == b._file;
}

bool operator!=(luna_script const& a, luna_script const& b)
{
    return !(a == b);
}

bool operator<(luna_script const& a, luna_script const& b)
{
    return !(a.priority() < b.priority());
}


void luna_script::setup_api()
{
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
            [=, _context = this->_context] (lua_State* s) {
                lua_newtable(s);

                for (auto& v : _context->environment().capabilities()) {
                    lua_pushstring(s, v.second.c_str());
                    lua_setfield(s, -2, v.first.c_str());
                }

                return 1;
            }};

    _lua[api]["shared"] = mond::table{};
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
        [=, _context = this->_context] (lua_State* s) {
            std::string key = luaL_checkstring(s, 1);
            std::string val = luaL_checkstring(s, 2);

            shared_vars[key] = val;
            _context->save_shared_vars(_context->_varfile);

            return 0;
        }};

    _lua[api]["shared"]["clear"] = std::function<int (lua_State*)>{
        [=, _context = this->_context] (lua_State* s) {
            std::string key = luaL_checkstring(s, 1);

            shared_vars.erase(key);
            _context->save_shared_vars(_context->_varfile);

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
    _lua[api].new_metatable<luna_proxy>()
        << mond::method("send_message", &luna_proxy::send_message)
        << mond::method("runtime_info", &luna_proxy::runtime_info)
        << mond::method("user_info",    &luna_proxy::user_info)
        << mond::method("server_info",  &luna_proxy::server_info)
        << mond::method("traffic_info", &luna_proxy::traffic_info);

    _lua[api]["self"] = mond::object<luna_proxy>(*_context);

    _lua[api]["self_meta"].export_metatable<luna_proxy>();
}

void luna_script::register_script()
{
    _lua[api].new_metatable<luna_script_proxy>()
        << mond::method("file",        &luna_script_proxy::file)
        << mond::method("name",        &luna_script_proxy::name)
        << mond::method("description", &luna_script_proxy::description)
        << mond::method("version",     &luna_script_proxy::version)
        << mond::method("priority",    &luna_script_proxy::priority)
        << mond::method("is_self",     &luna_script_proxy::is_self);

    _lua[api]["scripts"] = mond::table{};
    _lua[api]["scripts"]["self"] = mond::object<luna_script_proxy>(
        *_context, _file);

    _lua[api]["scripts"]["list"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            lua_newtable(s);

            for (std::unique_ptr<luna_script> const& scr : _context->scripts()) {
                mond::write(s,
                    mond::object<luna_script_proxy>(*_context, scr->file()));

                lua_setfield(s, -2, scr->file().c_str());
            }

            return 1;
        }};

    _lua[api]["script_meta"].export_metatable<luna_script_proxy>();
}


void luna_script::register_user()
{
    _lua[api].new_metatable<luna_user_proxy>()
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

            for (auto const& u : _context->users()) {
                mond::object<luna_user_proxy>(
                    *_context, u.id());

                lua_setfield(s, -2, u.id().c_str());
            }

            return 1;
        }};

    _lua[api]["users"]["create"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            std::string id    = luaL_checkstring(s, 1);
            std::string mask  = luaL_checkstring(s, 2);
            std::string flags = luaL_checkstring(s, 3);
            std::string title = luaL_checkstring(s, 4);

            auto i = std::find_if(
                std::begin(_context->users()), std::end(_context->users()),
                [&] (luna_user& u2) {
                    return irc::rfc1459_equal(u2.id(), id);
                });

            if (i != std::end(_context->users())) {
                throw mond::runtime_error{"user `" + id + "' already exists"};
            }

            _context->users().emplace_back(
                id, mask, title, flags_from_string(flags));

            _context->save_users(_context->_userfile);
            return mond::write(s, mond::object<luna_user_proxy>(*_context, id));
        }};


    _lua[api]["users"]["remove"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            std::string id = luaL_checkstring(s, 1);

            auto i = std::find_if(
                std::begin(_context->users()), std::end(_context->users()),
                [&] (luna_user& u2) {
                    return irc::rfc1459_equal(u2.id(), id);
                });

            if (i == std::end(_context->users())) {
                throw mond::runtime_error{"user `" + id + "' does not exist"};
            }

            _context->users().erase(i);
            _context->save_users(_context->_userfile);

            return 0;
        }};

    _lua[api]["user_meta"].export_metatable<luna_user_proxy>();
}

void luna_script::register_channel()
{
    _lua[api].new_metatable<luna_channel_proxy>()
        << mond::method(mond::meta_tostring, &luna_channel_proxy::name)
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
                    *_context,
                    _context->environment()
                        .find_channel(luaL_checkstring(s, 1)).name()));

            } catch (irc::protocol_error& pe) {
                return mond::write(s, mond::nil{});
            }
        }};

    _lua[api]["channels"]["list"] = std::function<int (lua_State*)>{
        [=] (lua_State* s) {
            lua_newtable(s);

            for (auto const& entry : _context->environment().channels()) {
                mond::write(s, mond::object<luna_channel_proxy>(
                    *_context, entry.second->name()));

                lua_setfield(s, -2, entry.second->name().c_str());
            }

            return 1;
        }};

    _lua[api]["channel_meta"].export_metatable<luna_channel_proxy>();
}

void luna_script::register_channel_user()
{
    _lua[api].new_metatable<luna_channel_user_proxy>()
        << mond::method("user_info",     &luna_channel_user_proxy::user_info)
        << mond::method("modes",         &luna_channel_user_proxy::modes)
        << mond::method("channel",       &luna_channel_user_proxy::channel)
        << mond::method("match_reguser", &luna_channel_user_proxy::match);

    _lua[api]["channel_user_meta"].export_metatable<
        luna_channel_user_proxy>();
}
