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
#ifndef LIBMOND_TAPE_HH_INCLUDED
#define LIBMOND_TAPE_HH_INCLUDED

#include "mond/macros.h"

#include <lua.hpp>

#include <algorithm>

class DLL_PUBLIC tape {
public:
    tape(int bottom, lua_State* state, bool* s)
        : _bottom{bottom},
          _state{state},
          _seeking{s}
    {
        *_seeking = true;
    }

    tape(tape&& other)
        : _bottom{other._bottom},
          _state{other._state},
          _seeking{other._seeking}
    {
        other._state   = nullptr;
        other._seeking = nullptr;
    }

    ~tape()
    {
        if (_state) {
            lua_pop(_state, std::max(0, lua_gettop(_state) - _bottom));

            *_seeking = false;
        }
    }

private:
    int _bottom;
    lua_State* _state;
    bool* _seeking;
};

#endif // defined LIBMOND_TAPE_HH_INCLUDED
