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
#ifndef FOCUS_H
#define FOCUS_H

#include "macros.h"

#include <lua.hpp>

#include "lua_write.h"
#include "lua_read.h"
#include "lua_types.h"
#include "function.h"
#include "tape.h"
#include "state.h"

#include <sstream>
#include <string>

namespace mond {

template <typename T>
class metatable;

class DLL_PUBLIC focus {
public:
    focus(state& state, std::string const& idx);

    inline focus operator[](char const* cstr)
    {
        return (*this)[std::string{cstr}];
    }

    focus operator[](std::string const& idx);
    focus operator[](int idx);

    template <typename T>
    metatable<T> new_metatable()
    {
        return metatable<T>{_state, *this, T::metatable};
    }

    template <typename T>
    void export_metatable()
    {
        auto t = seek_init();

        luaL_newmetatable(_state, T::metatable);
        _set(_state, -1);
    }

    /*
    template <typename T, typename... Args>
    void new_constructor(std::string const& meta)
    {
        auto t = seek_init();

        _state._functions.emplace_back(
            std::move(write_constructor<T, Args...>(_state, meta)));

        _set(_state, -1);
    }
    */

    template <typename... Ret, typename... Args>
    auto call(Args const&... args)
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
    R get()
    {
        auto t = seek();

        return read<R>(_state, -1);
    }

    template <typename T>
    operator T()
    {
        return get<T>();
    }

    template <bool>
    operator bool()
    {
        auto t = seek();

        if (lua_isnil(_state, lua_gettop(_state))) {
            return false;
        }

        return read<bool>(_state, lua_gettop(_state));
    }


    template <typename T>
    void set(T const& v)
    {
        auto t = seek_init();

        write(_state, v);

        _set(_state, -1);
    }

    template <typename T, typename... Args>
    void set(std::string meta, Args&&... args)
    {
        auto t = seek_init();

        write_object<T>(_state, meta, std::forward<Args>(args)...);

        _set(_state, -1);
    }

    template <typename T>
    focus& operator=(T const& v)
    {
        set<T>(v);
        return *this;
    }

    template <typename Ret, typename... Args>
    focus& operator=(std::function<Ret (Args...)> fun)
    {
        auto t = seek_init();

        _state._functions.emplace_back(
            std::move(write_function(_state, fun, "")));

        _set(_state, -1);
        return *this;
    }


    tape seek();
    tape seek_init();

private:
    state& _state;
    bool _seeking;

    using walker_fun = std::function<void (state&)>;

    std::vector<walker_fun> _path;
    std::function<void (state&, int)> _set;
};

} // namespace mond

#endif // defined FOCUS_H
