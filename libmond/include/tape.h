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
#ifndef TAPE_H
#define TAPE_H

#include "macros.h"

#include <lua.hpp>

class DLL_PUBLIC tape {
public:
    tape(int bottom, lua_State* state, bool* s);
    tape(tape&& other);

    ~tape();

private:
    int _bottom;
    lua_State* _state;
    bool* _seeking;
};

#endif // defined TAPE_H
