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

#pragma once
#ifndef ITERATOR_H
#define ITERATOR_H

#include <lua.hpp>

#include <cstddef>

#include "lua_read.h"

#include <iterator>

namespace mond {

template <typename K, typename V>
struct iterator;

template <typename V>
struct iterator<std::size_t, V>
    // TODO: No reason this couldn't be random access
    : public std::iterator<std::bidirectional_iterator_tag, V const> {

    iterator()
        : _state{nullptr},
          _i{static_cast<std::size_t>(-1)},
          _max{static_cast<std::size_t>(-1)}
    {
    }

    iterator(lua_State* s)
        : _state{s},
          _i{1},
          _max{lua_rawlen(s, -1) + 1}
    {
    }

    iterator(iterator const& rhs)
        : _state{rhs._state},
          _i{rhs._i},
          _max{rhs._max}
    {
    }

    iterator& operator=(iterator const& rhs)
    {
        // No need for self assign check
        _state = rhs._state;
        _i     = rhs._i;
        _max   = rhs._max;
    }

    V const& operator*()
    {
        lua_rawgeti(_state, -1, _i);
        _val = read<V>(_state, -1);

        lua_pop(_state, 1);
        return _val;
    }

    V const* operator->()
    {
        lua_rawgeti(_state, -1, _i);
        _val = read<V>(_state, -1);

        lua_pop(_state, 1);
        return &_val;
    }

    iterator& operator++()
    {
        ++_i;
        return *this;
    }

    iterator& operator--()
    {
        --_i;
        return *this;
    }


    iterator operator++(int)
    {
        iterator r(*this);
        ++(*this);

        return r;
    }

    iterator operator--(int)
    {
        iterator r(*this);
        --(*this);

        return r;
    }


    bool operator==(iterator const& rhs) const
    {
        // If rhs is end iterator and we're at the end, compare true.
        if (rhs._max == static_cast<std::size_t>(-1) and (_i == _max)) {
            return true;
        }

        return _i == rhs._i;
    }

    bool operator!=(iterator const& rhs) const
    {
        return !(*this == rhs);
    }

private:
    lua_State* _state;
    std::size_t _i;
    std::size_t _max;
    V _val;
};

template <typename T>
using array_iterator = iterator<std::size_t, T>;

} // namespace mond

#endif // defined ITERATOR_H
