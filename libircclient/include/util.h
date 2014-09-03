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

#ifndef UTIL_H
#define UTIL_H

#include "macros.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <iosfwd>

/*! \brief Splits a string on a delimiter.
 *
 * Splits the string \p src among the delimiter \p sep. Empty section are left
 * out of the results and delimiters are not included.
 *
 * \param src Source string to split.
 * \param sep Delimiter to split at.
 * \return Non-Empty parts that were split out.
 */
extern DLL_PUBLIC
std::vector<std::string> split_noempty(
    std::string const& src,
    std::string const& sep);

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

}

#endif
