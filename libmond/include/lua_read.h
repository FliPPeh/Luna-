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
#ifndef LUAH_READ_H
#define LUAH_READ_H

#include "lua_types.h"

#include <lua.hpp>

#include <cstddef>

#include <string>
#include <tuple>
#include <type_traits>
#include <iostream>

namespace mond {
namespace impl {

// Read helpers for generic stack values
inline std::string _read(tag<std::string>, lua_State* l, int i)
{
    std::size_t n;
    char const* str = lua_tolstring(l, i, &n);

    return std::string{str, n};
}

inline bool _read(tag<bool>, lua_State* l, int i)
{
    return lua_toboolean(l, i);
}

inline void* _read(tag<void*>, lua_State* l, int i)
{
    if (not lua_isuserdata(l, i) and not lua_islightuserdata(l, i)) {
        throw type_mismatch_error{
            "expected `" + std::string{lua_typename(l, LUA_TUSERDATA)} + "'"
            + ", got: `" + std::string{lua_typename(l, lua_type(l, i))} + "'."};
    }

    return lua_touserdata(l, i);
}

inline lua_Integer _read(tag<lua_Integer>, lua_State* l, int i)
{
    return lua_tointeger(l, i);
}

inline lua_Number _read(tag<lua_Number>, lua_State* l, int i)
{
    return lua_tonumber(l, i);
}

template <typename T>
std::vector<T> _read(tag<std::vector<T>>, lua_State* l, int i)
{
    std::vector<T> ret;

    for (size_t j = 1; j <= lua_rawlen(l, i); ++j) {
        lua_rawgeti(l, -1, j);

        ret.emplace_back(_read(tag<T>{}, l, -1));

        lua_pop(l, 1);
    }

    return ret;
}

} // namespace impl

// Read "dispatcher"
template <typename T>
T read(lua_State* l, int i)
{
    /*
     * When reading, don't allow references
     */
    static_assert(not std::is_reference<T>::value, "T may not be a reference");

    return static_cast<T>(_read(impl::tag<impl::mapped_type_t<T>>{}, l, i));
}

// Helpers

// Apparently GCC doesn't like it when read here is called with an empty
// parameter pack for `Ns' and complains about `l' and `offset' being set but
// unused. Clang is less shouty.
#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif

template <typename... Ts, std::size_t... Ns>
auto read(lua_State* l, int offset, impl::seq<Ns...>)
{
    return std::make_tuple(read<Ts>(l, offset + Ns + 1)...);
}

#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic pop
#endif

template <typename T, std::size_t N>
auto read(lua_State* l, int offset, impl::seq<N>)
{
    return read<T>(l, offset + N + 1);
}

template <typename... Ts>
auto read(lua_State* l)
{
    std::size_t constexpr n = sizeof...(Ts);
    int top = lua_gettop(l);

    // TODO argc count check (throw exception)

    return read<Ts...>(l, top - n, impl::seq_gen<n>{});
}

namespace impl {

/*
 * Read helpers for parameters (Includes type checking and signalling errors)
 */
inline std::string _check(tag<std::string>, lua_State* l, int i)
{
    std::size_t n;
    char const* str = luaL_checklstring(l, i, &n);

    return std::string{str, n};
}

inline bool _check(tag<bool>, lua_State* l, int i)
{
    return lua_toboolean(l, i);
}

inline void* _check(tag<void*>, lua_State* l, int i)
{
    luaL_argcheck(l, lua_isuserdata(l, i) || lua_islightuserdata(l, i), i,
        "expected user data");

    return lua_touserdata(l, i);
}

inline lua_Integer _check(tag<lua_Integer>, lua_State* l, int i)
{
    return luaL_checkinteger(l, i);
}

inline lua_Number _check(tag<lua_Number>, lua_State* l, int i)
{
    return luaL_checknumber(l, i);
}

} // namespace impl

// Check "dispatcher"
template <typename T>
T check(lua_State* l, int i)
{
    /*
     * When reading, don't allow references
     */
    static_assert(not std::is_reference<T>::value, "T may not be a reference");

    return static_cast<T>(_check(impl::tag<impl::mapped_type_t<T>>{}, l, i));
}

// Apparently GCC doesn't like it when read here is called with an empty
// parameter pack for `Ns' and complains about `l' and `offset' being set but
// unused. Clang is less shouty.
#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif

template <typename... Ts, std::size_t... Ns>
auto check(lua_State* l, int offset, impl::seq<Ns...>)
{
    return std::make_tuple(check<Ts>(l, offset + Ns + 1)...);
}

#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic pop
#endif

template <typename T, std::size_t N>
auto check(lua_State* l, int offset, impl::seq<N>)
{
    return check<T>(l, offset + N + 1);
}

template <typename... Ts>
auto check(lua_State* l)
{
    std::size_t constexpr n = sizeof...(Ts);

    // No need to check argument count, luaL_check* does the job for us.
    return check<Ts...>(l, 0, impl::seq_gen<n>{});
}

} // namespace mond

#endif // defined LUAH_READ_H
