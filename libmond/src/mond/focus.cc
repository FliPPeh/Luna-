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

#include "mond/focus.hh"

#include "mond/state.hh"
#include "mond/tape.hh"
#include "mond/lua_types.hh"

#include <lua.hpp>

#include <string>
#include <utility>

namespace mond {

focus::focus(state& lstate, std::string const& idx)
    : _state{lstate},
      _seeking{false},
      _path{}
{
    _path.emplace_back([idx](state& l) {
        lua_getglobal(l, idx.c_str());
    });

    _set = [idx](state& l, int i) {
        lua_setglobal(l, idx.c_str());
    };
}

focus focus::operator[](std::string const& idx)
{
    focus next = *this;

    next._path.emplace_back([idx](state& l) {
        if (lua_isnil(l, -1)) {
            throw mond::error{"attempted to index nil"};
        }

        // Protect us from __index metamethods that could throw.
        auto protected_getfield = [](lua_State* s) {
            char const* idx = lua_tostring(s, lua_upvalueindex(1));

            lua_getfield(s, -2, idx);
            return 1;
        };

        lua_pushstring(l, idx.c_str());
        lua_pushcclosure(l, protected_getfield, 1);

        if (lua_pcall(l, 0, 1, 0) != LUA_OK) {
            throw mond::runtime_error{lua_tostring(l, -1)};
        }
    });

    next._set = [idx](state& l, int i) {
        lua_setfield(l, -2, idx.c_str());
    };

    return next;
}

focus focus::operator[](int idx)
{
    focus next = *this;

    next._path.emplace_back([idx](state& l) {
        if (lua_isnil(l, -1)) {
            throw mond::error{"attempted to index nil"};
        }

        lua_rawgeti(l, -1, idx);
    });

    next._set = [idx](state& l, int i) {
        lua_rawseti(l, i, idx);
    };

    return next;
}


tape focus::seek()
{
    if (_seeking) {
        throw mond::error{"already seeking"};
    }

    tape t{lua_gettop(_state), _state, &_seeking};

    for (auto& breadcrumb : _path) {
        breadcrumb(_state);
    }

    return std::move(t);
}

tape focus::seek_init()
{
    if (_seeking) {
        throw mond::error{"already seeking"};
    }

    tape t{lua_gettop(_state), _state, &_seeking};

    for (auto breadcrumb =  std::begin(_path);
              breadcrumb != std::end(_path) - 1;
            ++breadcrumb) {
        (*breadcrumb)(_state);
    }

    return std::move(t);
}

}
