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
#ifndef LUAH_STATE_H
#define LUAH_STATE_H

#include "macros.h"

#include <lua.hpp>

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace mond {

constexpr char const* meta_call     = "__call";
constexpr char const* meta_tostring = "__tostring";

class focus;
class function_base;

class DLL_PUBLIC state {
public:
    state();
    ~state();

    // There isn't really a valid state after moving from a state.
    state(state const&) = delete;
    state(state&&);

    state& operator=(state const&) = delete;
    state& operator=(state&&);

    bool valid() const;

    void load_file(std::string const& name);
    void load_library(std::string const& name, std::string const& mod = "");
    bool eval(std::string const& eval);

    operator lua_State*();

    focus operator[](std::string const& idx);

    lua_Debug debug(int level);

private:
    lua_State *_l;

private:
    friend class focus;

    template <typename T>
    friend class metatable;

    template <typename T, typename Ret, typename... Args>
    friend class metamethod;

    std::vector<std::unique_ptr<function_base>> _functions;
};

} // namespace mond

#endif // defined LUA_STATE_H
