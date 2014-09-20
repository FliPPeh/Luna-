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

#pragma once
#ifndef IRC_EXCEPT_H
#define IRC_EXCEPT_H

#include "macros.h"

#include <stdexcept>
#include <string>

/*! \file
 *  \brief Exceptions used throughout the library.
 */

namespace irc {

//! \brief Base class of all IRC errors that will be thrown.
class DLL_PUBLIC error : public std::runtime_error {
public:
    error(std::string what) : std::runtime_error{std::move(what)}
    {
    }
};

template <typename T>
char const* typed_error_meaning(T const& dontcare)
{
    return "unknown error";
}

/*! \brief Specific exception type.
 *
 * `typed_error` is parameterized over a specific error type that is used to
 * identify the error that has been raised. Thus `typed_error` itself is
 * merely a category of error types, rather than a specific error in itself.
 *
 * The human readable error code translations are obtained via a function
 * template specialization of `typed_error_meaning` for the parameterized
 * type, or the default fallback.
 */
template <typename T>
class typed_error : public error {
public:
    typed_error(T code, std::string what)
        : error{what}, _code{code}
    {
        if (what.empty()) {
            _what = meaning();
        } else {
            _what = meaning() + ": " + error::what();
        }
    }

    //! \brief Returns the specific error that was raised.
    T code() const
    {
        return _code;
    }

    //! \brief Returns the human readable description of the error.
    std::string meaning() const
    {
        return typed_error_meaning<T>(_code);
    }

    char const* what() const noexcept
    {
        return _what.c_str();
    }

private:
    T _code;
    std::string _what;
};


//! Error types used to identify IRC protocol error.
enum class protocol_error_type {
    invalid_message,
    invalid_command,
    invalid_prefix,
    login_error,
    no_such_channel,
    no_such_user,
    no_such_mode,
    not_enough_arguments
};

//! Error types used to identify IRC connection errors.
enum class connection_error_type {
    connection_error,
    lookup_error,
    stream_error,
    io_error,
    cannot_change_security,
    not_connected
};


template <typename T>
char const* typed_error_meaning(protocol_error_type t)
{
    switch (t) {
    case protocol_error_type::invalid_message:
        return "invalid message";

    case protocol_error_type::invalid_command:
        return "invalid command";

    case protocol_error_type::invalid_prefix:
        return "invalid prefix";

    case protocol_error_type::login_error:
        return "login error";

    case protocol_error_type::no_such_channel:
        return "no such channel";

    case protocol_error_type::no_such_user:
        return "no such user";

    case protocol_error_type::no_such_mode:
        return "no such mode";

    case protocol_error_type::not_enough_arguments:
        return "not enough parameters";
    }

    return ""; // Not reached
}

template <typename T>
char const* typed_error_meaning(connection_error_type t)
{
    switch (t) {
    case connection_error_type::connection_error:
        return "connection error";

    case connection_error_type::lookup_error:
        return "hostname lookup error";

    case connection_error_type::stream_error:
        return "Stream I/O error";

    case connection_error_type::io_error:
        return "I/O error";

    case connection_error_type::cannot_change_security:
        return "Can not change security";

    case connection_error_type::not_connected:
        return "Not connected";
    }

    return ""; // Not reached
}

//! \brief IRC protocol error
using protocol_error   = typed_error<protocol_error_type>;

//! \brief IRC connection error
using connection_error = typed_error<connection_error_type>;

}

#endif // defined IRC_EXCEPT_H
