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

#include "tape.h"

#include <algorithm>

tape::tape(int bottom, lua_State* state, bool* s)
    : _bottom{bottom},
      _state{state},
      _seeking{s}
{
    *_seeking = true;
}


tape::tape(tape && other)
    : _bottom{other._bottom},
      _state{other._state},
      _seeking{other._seeking}
{
    other._state   = nullptr;
    other._seeking = nullptr;
}

tape::~tape()
{
    if (_state) {
        lua_pop(_state, std::max(0, lua_gettop(_state) - _bottom));

        *_seeking = false;
    }
}

