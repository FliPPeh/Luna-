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
#ifndef LIBMOND_LUA_UTIL_HH_INCLUDED
#define LIBMOND_LUA_UTIL_HH_INCLUDED

#include "mond/lua_types.hh"

#include <lua.hpp>

#include <cstddef>

#include <tuple>
#include <type_traits>

namespace mond {
namespace impl {

// Apparently GCC doesn't like it when read here is called with an empty
// parameter pack for `Ns' and complains about `args' being set but
// unused. Clang is less shouty.
#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif

template <typename Ret, typename Fun, typename... Args, std::size_t... Ns>
Ret _lift(Fun f, std::tuple<Args...> args, std::index_sequence<Ns...>)
{
    return f(std::get<Ns>(args)...);
}

#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic pop
#endif

template <bool b> struct _bool {};

template <typename Fun, typename... Args>
auto _lift_result(Fun fun, Args&&... args, _bool<true>)
{
    fun(std::forward<Args>(args)...);

    return std::tuple<>{};
}

template <typename... Args>
std::tuple<Args...> _lift_result_helper(std::tuple<Args...> tup)
{
    return tup;
}

template <typename T>
std::tuple<T> _lift_result_helper(T&& t)
{
    return std::make_tuple(std::forward<T>(t));
}

template <typename Fun, typename... Args>
auto _lift_result(Fun fun, Args&&... args, _bool<false>)
{
    return _lift_result_helper(fun(std::forward<Args>(args)...));
}

} // namespace impl


/*
 * "Lifts" a function into tuple-land, where all its arguments are inside
 * the tuple.
 */
template <typename Ret, typename Fun, typename... Args>
Ret lift(Fun f, std::tuple<Args...> args)
{
    return impl::_lift<Ret>(
        f, args, std::make_index_sequence<sizeof...(Args)>{});
}

/*
 * Promotes atomic values, and void values, to tuples.
 */

template <typename Fun, typename... Args>
auto lift_result(Fun f, Args&&... args)
{
    return _lift_result(f, args..., typename
        impl::_bool<std::is_void<std::result_of_t<Fun (Args...)>>::value>{});
}

} // namespace mond


#endif // defined LIBMOND_LUA_UTIL_HH_INCLUDED
