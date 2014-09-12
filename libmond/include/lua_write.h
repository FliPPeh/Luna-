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
#ifndef LUAH_WRITE_H
#define LUAH_WRITE_H

#include "lua_types.h"
#include "lua_util.h"

#include <lua.hpp>

#include <cstddef>

#include <tuple>
#include <string>
#include <type_traits>
#include <functional>

namespace mond {
namespace impl {

// Write helpers
inline int _write(lua_State* l, std::string const& str)
{
    lua_pushlstring(l, str.c_str(), str.size());
    return 1;
}

inline int _write(lua_State* l, bool b)
{
    lua_pushboolean(l, b);
    return 1;
}

inline int _write(lua_State* l, void* v)
{
    lua_pushlightuserdata(l, v);
    return 1;
}

inline int _write(lua_State* l, lua_Integer i)
{
    lua_pushinteger(l, i);
    return 1;
}

inline int _write(lua_State* l, lua_Number n)
{
    lua_pushnumber(l, n);
    return 1;
}

template <typename T, typename... Args, std::size_t... Is>
int _write_ctor(
    lua_State* l,
    std::string const& meta,
    std::tuple<Args...> const& args,
    seq<Is...>)
{
    T* ud = static_cast<T*>(lua_newuserdata(l, sizeof(*ud)));
    new (ud) T{std::get<Is>(args)...};

    luaL_setmetatable(l, meta.c_str());
    return 1;
}

template <typename T, typename... Args>
int _write(lua_State* l, _object<T, Args...> const& obj)
{
    return _write_ctor<T>(l, obj.meta, obj.args, seq_gen<sizeof...(Args)>{});
}

inline int _write(lua_State* l, nil)
{
    lua_pushnil(l);
    return 1;
}

inline int _write(lua_State* l, table)
{
    lua_newtable(l);
    return 1;
}

template <typename T>
int _write(lua_State* l, std::vector<T> const& vec)
{
    lua_newtable(l);

    for (size_t i = 0; i < vec.size(); ++i) {
        _write(l, vec[i]);
        lua_rawseti(l, -2, i+1);
    }

    return 1;
}

} // namespace impl


// Write "dispatcher", give the compiler a little push if it couldn't resolve
// one of the above directly.

// zero values
inline int write(lua_State* l)
{
    return 0;
}

// one value
template <typename T>
int write(lua_State* l, T const& val)
{
    return impl::_write(l,
        static_cast<impl::mapped_type_t<T> const&>(val));
}

// many values
template <typename T, typename... Ts>
int write(lua_State* l, T const& head, Ts const&... tail)
{
    int a = write(l, head);
    return a + write(l, tail...);
}

namespace impl {

template <typename... Args, std::size_t... Ns>
int _write_tuple(lua_State* l, std::tuple<Args...> const& args, seq<Ns...>)
{
    return write<Args...>(l, std::get<Ns>(args)...);
}

} // namespace impl

template <typename... Args>
int write(lua_State* l, std::tuple<Args...> const& args)
{
    return impl::_write_tuple(l, args, impl::seq_gen<sizeof...(Args)>{});
}

} // namespace mond

#endif // defined LUAH_WRITE_H
