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

#include "mond/state.hh"
#include "mond/focus.hh"

#include <lua.hpp>

#include <string>

namespace mond {

state::state()
    : _l{luaL_newstate()}
{
    luaL_openlibs(_l);
}

state::~state()
{
    if (_l) {
        // If we let _function get destroyed automatically, it would have
        // happened *after* lua_close and all the destructors would point
        // to bad memory after Lua freed all their userdata.
        lua_close(_l);
    }
}

/*
state::state(state&& other)
    : _l{other._l},
      _functions{std::move(other._functions)}
{
    other._l = nullptr;
}

state& state::operator=(state&& other)
{
    _l = other._l;

    other._l = nullptr;

    return *this;
}
*/

bool state::valid() const
{
    return _l;
}


void state::load_file(std::string const& name)
{
    if (luaL_dofile(_l, name.c_str())) {
        throw mond::error{lua_tostring(_l, -1)};
    }
}

void state::load_library(std::string const& name, std::string const& mod)
{
    // In lua: name = require('name'). Load the module using require and
    // push it as a global.

    lua_getglobal(_l, "require");
    lua_pushstring(_l, name.c_str());

    if (lua_pcall(_l, 1, 1, 0) != LUA_OK) {
        std::string err = std::string{lua_tostring(_l, -1)};

        lua_pop(_l, 1);

        throw mond::runtime_error{err};
    }

    lua_setglobal(_l, (mod.empty() ? name : mod).c_str());
}


bool state::eval(std::string const& eval)
{
    return luaL_dostring(_l, eval.c_str());
}

state::operator lua_State*() const
{
    return _l;
}


focus state::operator[](std::string const& idx)
{
    return focus{*this, idx};
}


lua_Debug state::debug(int level)
{
    lua_Debug debug;

    if (lua_getstack(_l, level, &debug) == 1) {
        lua_getinfo(_l, "nSl", &debug);

        return debug;
    } else {
        throw mond::error{"error getting function stack info"};
    }
}


namespace impl {

int func_dispatcher(lua_State* state)
{
    function_base* fun = static_cast<function_base*>(
        lua_touserdata(state, lua_upvalueindex(1)));

    try {
        return fun->call();
    } catch (mond::error const& ex) {
        return luaL_error(state, ex.what());
    } catch (std::exception const& ex) {
        return luaL_error(state, ex.what());
    } catch (...) {
        return luaL_error(state, "unknown error");
    }
}

} // namespace impl

} // namespace mond
