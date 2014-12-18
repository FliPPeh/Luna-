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
#ifndef LIBMOND_LUA_TYPES_HH_INCLUDED
#define LIBMOND_LUA_TYPES_HH_INCLUDED

#include <lua.hpp>

#include <cstddef>

#include <utility>
#include <string>
#include <vector>
#include <stdexcept>
#include <functional>

namespace mond {

template <typename T, typename... Args>
struct _object {
    _object(std::string meta, Args&&... args)
        : meta{meta},
          args{std::forward<Args>(args)...}
    {
    }

    std::string meta;
    std::tuple<Args...> args;
};

template <typename T, typename... Args>
_object<T, Args...> object(Args&&... args)
{
    return _object<T, Args...>{T::metatable, std::forward<Args>(args)...};
}

struct table {
};

struct nil {
};


namespace impl {

// Simple type tagging to allow us to overload on
template <typename T>
struct tag {
};

/*
 * mapped_type and specializations return, for every (or most) valid types T
 * the type used to represent it.
 */
template <typename T, typename Enable = void>
struct mapped_type {
};

/*
 * bool maps to itself
 */
template <>
struct mapped_type<bool> {
    using type = bool;
};

/*
 * std::string maps to itself
 */
template <>
struct mapped_type<std::string> {
    using type = std::string;
};

/*
 * String literals
 */
template <std::size_t N>
struct mapped_type<char[N]> {
    using type = std::string;
};

/*
 * Map integrals to whatever lua_Integer may be
 */
template <typename T>
struct mapped_type<T, std::enable_if_t<std::is_integral<T>::value>> {
    using type = lua_Integer;
};

/*
 * Map floating points to whatever lua_Number may be
 */
template <typename T>
struct mapped_type<T, std::enable_if_t<std::is_floating_point<T>::value>> {
    using type = lua_Number;
};

/*
 * Pointers (light user data)
 */
template <typename T>
struct mapped_type<T*> {
    using type = void*;
};

/*
 * Objects (fat user data)
 */
template <typename T, typename... Args>
struct mapped_type<_object<T, Args...>> {
    using type = _object<T, Args...>;
};

/*
 * Nil and table
 */
template <>
struct mapped_type<nil> {
    using type = nil;
};

template <>
struct mapped_type<table> {
    using type = table;
};

/*
 * Vectors
 */
template <typename T>
struct mapped_type<std::vector<T>> {
    using type = std::vector<T>;
};

template <typename T>
using remove_qualifiers = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T, typename... Ts>
using mapped_type_t = typename mapped_type<remove_qualifiers<T>, Ts...>::type;

} // namespace impl


class error : public std::runtime_error {
public:
    error(std::string const& what) : std::runtime_error{what} {}
    error(std::string&& what)      : std::runtime_error{std::move(what)} {}
};

class runtime_error : public error {
public:
    runtime_error(std::string const& what) : error{what} {}
    runtime_error(std::string&& what)      : error{std::move(what)} {}
};

class type_mismatch_error : public error {
public:
    type_mismatch_error(std::string const& what) : error{what} {}
    type_mismatch_error(std::string&& what)      : error{std::move(what)} {}
};

} // namespace mond

#endif // defined LIBMOND_LUA_TYPES_HH_INCLUDED
