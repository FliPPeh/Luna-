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
#ifndef IRC_UTILS_H
#define IRC_UTILS_H

/*! \file
 *  \brief Common, but not cruicial, IRC helpers.
 */

#include "irc_core.h"

#include <cstddef>

#include <string>
#include <tuple>
#include <vector>
#include <algorithm>
#include <unordered_map>

namespace irc {

//! Returns whether or not the command is a numeric one (i.e. server reply)
extern DLL_PUBLIC
inline bool is_numeric(std::string const& cmd)
{
    return (cmd.size() == 3)
       and (cmd.find_first_not_of("0123456789") == std::string::npos);
}

//! Returns whether or not the prefix is a user prefix or single nickname.
extern DLL_PUBLIC
inline bool is_user_prefix(std::string const& pref)
{
    std::string req = "!@";

    // Shortcut: no '.' => can't be single hostname (server) or part of
    // user prefix (user hostname), in which case it's simply a nickname.
    if (pref.find('.') == std::string::npos) {
        return true;
    }

    return not std::any_of(begin(req), end(req), [&pref](char c) {
        return pref.find(c) == std::string::npos;
    });
}

//! Returns whether or not the prefix is a server name.
extern DLL_PUBLIC
inline bool is_server_prefix(std::string const& pref)
{
    // If it's not a user prefix, but contains a '.', it should be a hostname.
    return not is_user_prefix(pref);
}


//! Splits a user prefix into a tuple of nickname, username and hostname.
extern DLL_PUBLIC
std::tuple<std::string, std::string, std::string> split_prefix(
    std::string const& prefix);

//! Normalizes a user prefix into a nickname and leaves nicknames untouched.
extern DLL_PUBLIC
inline std::string normalize_nick(std::string user)
{
    if (is_user_prefix(user)) {
        return std::get<0>(split_prefix(user));
    } else {
        return user;
    }
}

//! \brief ASCII-Lowercase
extern DLL_PUBLIC
inline char rfc1459_lower(char c)
{
    if (c >= 'A' and c <= '^') {
        return c | (1 << 5);
    } else {
        return c;
    }
}

//! \brief ASCII-Uppercase
extern DLL_PUBLIC
inline char rfc1459_upper(char c)
{
    if (c >= 'a' and c <= '~') {
        return c & ~(1 << 5);
    } else {
        return c;
    }
}

//! \brief Compare two strings for quality using rfc1459 case mapping
extern DLL_PUBLIC
bool rfc1459_equal(std::string const& a, std::string const& b);

//! \brief ASCII-Lowercase
extern DLL_PUBLIC
std::string rfc1459_lower(std::string const& src);

//! \brief ASCII-Uppercase
extern DLL_PUBLIC
std::string rfc1459_upper(std::string const& src);

/*! \brief Hashing object using ASCII case mapping.
 *
 * Hashing object for std::unordered_map that hashes keys regardeless of their
 * ASCII case mappings.
 */
template <typename T>
struct rfc1459_key_hash {
    std::size_t operator()(T const& key) const
    {
        return std::hash<T>{}(rfc1459_lower(key));
    }
};

/*! \brief Comparator object using ASCII case mapping.
 *
 * Comparator object for std::unordered_map that compares keys using
 * ASCII case mapping.
 */
template <typename T>
struct rfc1459_key_equal {
    bool operator()(T const& key1, T const& key2) const
    {
        return rfc1459_equal(key1, key2);
    }
};

/*! \brief Hashing object with nickname normalization.
 *
 * Specialized hashing object for std::unordered_map that, in addition to
 * hashing the same for equal std::strings with different casing, normalizes
 * prefixes before comparison.
 */
struct DLL_PUBLIC nick_hash {
    std::size_t operator()(std::string const& key) const
    {
        return hsh(normalize_nick(key));
    }

    rfc1459_key_hash<std::string> hsh;
};

/*! \brief Comparator object using nickname normalization.
 *
 * Specialized comparator object for std::unordered_map that, in addition to
 * comparing with ASCII case mapping, normalizes the compared std::strings.
 */
struct DLL_PUBLIC nick_equal {
    bool operator()(std::string const& key1, std::string const& key2) const
    {
        return rfc1459_equal(normalize_nick(key1), normalize_nick(key2));
    }
};

//! Alias for std::unordered_map using rfc1459_hash and rfc1459_equal for
//! insertion.
template <typename K, typename V>
using unordered_rfc1459_map =
    std::unordered_map<K, V, rfc1459_key_hash<K>, rfc1459_key_equal<K>>;

//! Alias for std::unordered_map using nick_hash and nick_equal for insertion.
template <typename V>
using unordered_user_map =
    std::unordered_map<std::string, V, nick_hash, nick_equal>;

}


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

#endif // defined IRC_UTILS_H
