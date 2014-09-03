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
#ifndef LUAH_FUNCTION_H
#define LUAH_FUNCTION_H

#include "lua_util.h"
#include "lua_read.h"
#include "lua_write.h"
#include "lua_types.h"

#include <lua.hpp>

#include <cstddef>

#include <string>
#include <tuple>
#include <functional>
#include <memory>
#include <utility>

namespace mond {

class function_base {
public:
    function_base(lua_State* state, std::string metatable)
        : _state{state},
          _metatable{metatable}
    {
    }

    virtual ~function_base()
    {
    }

    virtual int call() = 0;

protected:
    void validate_metatable(int index)
    {
        if (not _metatable.empty()) {
            luaL_checkudata(_state, index, _metatable.c_str());
        }
    }

    void validate_argc(std::size_t expect)
    {
        if (static_cast<std::size_t>(lua_gettop(_state)) < expect) {
            luaL_error(_state,
                "insufficent number of arguments, expected %d, got %d",
                    expect,
                    lua_gettop(_state));
        }
    }

protected:
    lua_State* _state;
    std::string _metatable;
};


template <typename Ret, typename... Args>
class function : public function_base {
public:
    function(
        lua_State* state,
        std::function<Ret (Args...)> fun,
        std::string meta = "")

        : function_base{state, meta},
          _fun{fun}
    {
    }

    function(
        lua_State* state,
        Ret (*funptr)(Args...),
        std::string const& meta = "")

        : function(state, std::function<Ret (Args...)>{funptr}, meta)
    {
    }

    virtual int call() override
    {
        validate_metatable(1);
        //validate_argc(sizeof...(Args));

        auto args = lift_result([this] {
            return check<Args...>(_state);
        });

        return write(_state, lift<Ret>(_fun, args));
    }

private:
    std::function<Ret (Args...)> _fun;
};

/*
 * Specialization for functions that return nothing.
 */
template <typename... Args>
class function<void, Args...> : public function_base {
public:
    function(
        lua_State* state,
        std::function<void (Args...)> fun,
        std::string meta = "")

        : function_base{state, meta},
          _fun{fun}
    {
    }

    function(
        lua_State* state,
        void (*funptr)(Args...),
        std::string const& meta = "")

        : function(state, std::function<void (Args...)>{funptr})
    {
    }

    virtual int call() override
    {
        validate_metatable(1);
        //validate_argc(sizeof...(Args));

        auto args = lift_result([this] {
            return check<Args...>(_state);
        });

        lift<void>(_fun, args);
        return 0;
    }

private:
    std::function<void (Args...)> _fun;
};


/*
 * Specialization for functions that want to manage their own stacks.
 * No argument and return value translation is done.
 */
template <>
class function<int, lua_State*> : public function_base {
public:
    function(
        lua_State* state,
        std::function<int (lua_State*)> fun,
        std::string const& meta = "")

        : function_base{state, meta},
          _fun{fun}
    {
    }

    function(
        lua_State* state,
        int (*funptr)(lua_State*),
        std::string const& meta = "")

        : function(state, std::function<int (lua_State*)>{funptr}, meta)
    {
    }

    virtual int call() override
    {
        return _fun(_state);
    }

private:
    std::function<int (lua_State*)> _fun;
};


namespace impl {

DLL_PUBLIC
extern int func_dispatcher(lua_State*);

} // namespace impl


/*
 * Pushes the functions to Lua's stack, wrapped within a wrapper for Lua's
 * comfort, and returns an owning pointer.
 */
template <typename Ret, typename... Args>
std::unique_ptr<function_base> write_function(
    lua_State* s,
    std::function<Ret (Args...)> f,
    std::string const& meta = "")
{
    std::unique_ptr<function_base> tmp{new function<Ret, Args...>{s, f, meta}};

    lua_pushlightuserdata(s, static_cast<void*>(tmp.get()));
    lua_pushcclosure(s, &impl::func_dispatcher, 1);

    return std::move(tmp);
}

template <typename T, typename... Args>
int write_object(lua_State* s, std::string const& meta, Args&&... args)
{
    T* ud = static_cast<T*>(lua_newuserdata(s, sizeof(*ud)));
    new (ud) T{std::forward<Args>(args)...};

    luaL_setmetatable(s, meta.c_str());
    return 1;
}

/*
 * Pushes a function to Lua's stack that will construct on object of type T
 * with whatever constructor matches the template parameter Args.
 */
template <typename T, typename... Args>
std::unique_ptr<function_base> write_constructor(
    lua_State* s,
    std::string const& meta)
{
    std::function<int (lua_State*)> ctor_wrapper = [meta](lua_State* s) {
        // Read arguments into tuple and pop off stack
        auto args = lift_result([s] {
            return check<Args...>(s);
        });

        lua_pop(s, lua_gettop(s));

        // Create userdata
        T* ud = static_cast<T*>(lua_newuserdata(s, sizeof(*ud)));

        // Set metatable
        luaL_setmetatable(s, meta.c_str());

        // Call constructor
        lift<void>([&] (auto&&... args) {
            new (ud) T{std::forward<decltype(args)>(args)...};
        }, args);

        return 1;
    };

    return write_function(s, ctor_wrapper, meta);
}

} // namespace mond

#endif // defined LUA_FUNCTION_H
