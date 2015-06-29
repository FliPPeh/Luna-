/*
 * Copyright 2014 Lukas Niederbremer
 *
 * This file is part of libmond.
 *
 * libmond is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * libmond is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libmond.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef LIBMOND_STATE_HH_INCLUDED
#define LIBMOND_STATE_HH_INCLUDED

#include "mond/macros.h"
#include "mond/lua_write.hh"
#include "mond/lua_read.hh"
#include "mond/lua_types.hh"
#include "mond/tape.hh"
#include "mond/function.hh"

#include <lua.hpp>

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <sstream>

namespace mond {

constexpr char const* meta_call     = "__call";
constexpr char const* meta_tostring = "__tostring";

class focus;

template <typename T>
class metatable;


class DLL_PUBLIC state {
public:
    state();
    ~state();

    // There isn't really a valid state after moving from a state.
    state(state const&) = delete;
    state(state&&)      = delete;

    state& operator=(state const&) = delete;
    state& operator=(state&&)      = delete;

    bool valid() const;

    void load_file(std::string const& name);
    void load_library(std::string const& name, std::string const& mod = "");

    bool eval(std::string const& eval);

    operator lua_State*() const;

    focus operator[](std::string const& idx);

private:
    lua_State *_l;

private:
    friend class focus;

    template <typename T>
    friend class metatable;

    template <typename T, typename Ret, typename... Args>
    friend class metamethod;

    template <typename T, typename Ret, typename... Args>
    friend class metametamethod;

    std::vector<std::unique_ptr<function_base>> _functions;

    void register_function(std::unique_ptr<function_base> fn)
    {
        _functions.push_back(std::move(fn));
    }
};


class DLL_PUBLIC focus {
public:
    focus(state& lstate, std::string const& idx);

    focus operator[](char const* cstr);
    focus operator[](std::string const& idx);
    focus operator[](int idx);

    template <typename T>
    metatable<T> new_metatable();

    template <typename T>
    void export_metatable();

    template <typename... Ret, typename... Args>
    auto call(Args const&... args);

    template <typename R>
    R get();

    template <typename T>
    operator T();

    operator bool();

    template <typename T>
    void set(T const& v);

    template <typename T, typename... Args>
    void set(std::string meta, Args&&... args);

    template <typename T>
    focus& operator=(T const& v);

    template <typename Ret, typename... Args>
    focus& operator=(std::function<Ret (Args...)> fun);

    template <typename Ret, typename... Args>
    focus& operator=(Ret (*fun)(Args...));

    tape seek();
    tape seek_init();

private:
    state& _state;
    bool _seeking;

    using walker_fun = std::function<void (state&)>;

    std::vector<walker_fun> _path;
    std::function<void (state&, int)> _set;
};

/*
 * state implementation
 */
inline state::state()
    : _l{luaL_newstate()}
{
    luaL_openlibs(_l);
}

inline state::~state()
{
    if (_l) {
        // If we let _function get destroyed automatically, it would have
        // happened *after* lua_close and all the destructors would point
        // to bad memory after Lua freed all their userdata.
        lua_close(_l);
    }
}

inline bool state::valid() const
{
    return _l;
}

inline void state::load_file(std::string const& name)
{
    if (luaL_dofile(_l, name.c_str())) {
        throw mond::error{lua_tostring(_l, -1)};
    }
}

inline void state::load_library( std::string const& name, std::string const& mod)
{
    // In lua: name = require('name'). Load the module using require and
    // push it as a global.

    lua_getglobal(_l, "require");
    lua_pushstring(_l, name.c_str());

    if (lua_pcall(_l, 1, 1, 0) != LUA_OK) {
        std::string err = std::string{lua_tostring(_l, -1)};

        lua_pop(_l, 1);

        throw mond::runtime_error{err};
    }

    lua_setglobal(_l, (mod.empty() ? name : mod).c_str());
}

inline bool state::eval(std::string const& eval)
{
    return luaL_dostring(_l, eval.c_str());
}

inline state::operator lua_State*() const
{
    return _l;
}

inline focus state::operator[](std::string const& idx)
{
    return focus{*this, idx};
}


/*
 * focus implementation
 */
inline focus::focus(state& lstate, std::string const& idx)
    : _state{lstate},
      _seeking{false},
      _path{}
{
    _path.emplace_back([idx](state& l) {
        lua_getglobal(l, idx.c_str());
    });

    _set = [idx](state& l, int i) {
        lua_setglobal(l, idx.c_str());
    };
}

inline focus focus::operator[](char const* cstr)
{
    return (*this)[std::string{cstr}];
}

inline focus focus::operator[](std::string const& idx)
{
    focus next = *this;

    next._path.emplace_back([idx](state& l) {
        if (lua_isnil(l, -1)) {
            throw mond::error{"attempted to index nil"};
        }

        // Protect us from __index metamethods that could throw.
        auto protected_getfield = [](lua_State* s) {
            char const* idx = lua_tostring(s, lua_upvalueindex(1));

            lua_getfield(s, -2, idx);
            return 1;
        };

        lua_pushstring(l, idx.c_str());
        lua_pushcclosure(l, protected_getfield, 1);

        if (lua_pcall(l, 0, 1, 0) != LUA_OK) {
            throw mond::runtime_error{lua_tostring(l, -1)};
        }
    });

    next._set = [idx](state& l, int i) {
        lua_setfield(l, -2, idx.c_str());
    };

    return next;
}

inline focus focus::operator[](int idx)
{
    focus next = *this;

    next._path.emplace_back([idx](state& l) {
        if (lua_isnil(l, -1)) {
            throw mond::error{"attempted to index nil"};
        }

        lua_rawgeti(l, -1, idx);
    });

    next._set = [idx](state& l, int i) {
        lua_rawseti(l, i, idx);
    };

    return next;
}

template <typename T>
inline metatable<T> focus::new_metatable()
{
    return metatable<T>{_state, *this, T::metatable};
}

template <typename T>
inline void focus::export_metatable()
{
    auto t = seek_init();

    luaL_newmetatable(_state, T::metatable);
    _set(_state, -1);
}

template <typename... Ret, typename... Args>
inline auto focus::call(Args const&... args)
{
    auto t = seek();

    if (lua_type(_state, -1) != LUA_TFUNCTION) {
        std::ostringstream err;
        err << "error calling function: "
            << lua_typename(_state, LUA_TFUNCTION) << " expected, got "
            << luaL_typename(_state, -1);

        throw type_mismatch_error{err.str()};
    }

    write(_state, args...);

    if (lua_pcall(_state, sizeof...(args), sizeof...(Ret), 0) == 0) {
        return read<Ret...>(_state);
    } else {
        throw runtime_error{lua_tostring(_state, -1)};
    }
}

template <typename R>
inline R focus::get()
{
    auto t = seek();

    return read<R>(_state, -1);
}

template <typename T>
inline focus::operator T()
{
    return get<T>();
}

inline focus::operator bool()
{
    auto t = seek();

    if (lua_isnil(_state, lua_gettop(_state))) {
        return false;
    }

    return read<bool>(_state, lua_gettop(_state));
}


template <typename T>
inline void focus::set(T const& v)
{
    auto t = seek_init();

    write(_state, v);

    _set(_state, -1);
}

template <typename T, typename... Args>
inline void focus::set(std::string meta, Args&&... args)
{
    auto t = seek_init();

    write_object<T>(_state, meta, std::forward<Args>(args)...);

    _set(_state, -1);
}

template <typename T>
inline focus& focus::operator=(T const& v)
{
    set<T>(v);
    return *this;
}

template <typename Ret, typename... Args>
inline focus& focus::operator=(std::function<Ret (Args...)> fun)
{
    auto t = seek_init();

    _state.register_function(
        std::move(write_function(_state, fun, "")));
    _set(_state, -1);

    return *this;
}

template <typename Ret, typename... Args>
inline focus& focus::operator=(Ret (*fun)(Args...))
{
    auto t = seek_init();

    _state.register_function(
        std::move(write_function(_state, fun)));
    _set(_state, -1);

    return *this;
}


inline tape focus::seek()
{
    if (_seeking) {
        throw mond::error{"already seeking"};
    }

    tape t{lua_gettop(_state), _state, &_seeking};

    for (auto& breadcrumb : _path) {
        breadcrumb(_state);
    }

    return std::move(t);
}

inline tape focus::seek_init()
{
    if (_seeking) {
        throw mond::error{"already seeking"};
    }

    tape t{lua_gettop(_state), _state, &_seeking};

    for (auto breadcrumb =  std::begin(_path);
              breadcrumb != std::end(_path) - 1;
            ++breadcrumb) {
        (*breadcrumb)(_state);
    }

    return std::move(t);
}

} // namespace mond

#endif // defined LIBMOND_STATE_HH_INCLUDED
