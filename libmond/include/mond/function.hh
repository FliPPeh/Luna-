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
#ifndef LIBMOND_FUNCTION_HH_INCLUDED
#define LIBMOND_FUNCTION_HH_INCLUDED

#include "mond/lua_util.hh"
#include "mond/lua_read.hh"
#include "mond/lua_write.hh"
#include "mond/lua_types.hh"

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
    function_base(std::string const& meta)
        : _meta{meta}
    {
    }

    virtual ~function_base()
    {
    }

    virtual int call(lua_State* state)
    {
        if (not _meta.empty()) {
            luaL_checkudata(state, 1, _meta.c_str());
        }

        return 0;
    }

protected:
    std::string _meta;
};

template <typename Fun, typename Ret, typename... Args>
class function : public function_base {
public:
    function(Fun fun, std::string const& meta)
        : function_base{meta},
          _fun{fun}
    {
    }

    virtual int call(lua_State* state) override
    {
        function_base::call(state);

        auto args = lift_result([state] {
            return check<Args...>(state);
        });

        return write(state, lift<Ret>(_fun, args));
    }

private:
    Fun _fun;
};

/*
 * Specialization for functions that return nothing.
 */
template <typename Fun, typename... Args>
class function<Fun, void, Args...> : public function_base {
public:
    function(Fun fun, std::string const& meta)
        : function_base{meta},
          _fun{fun}
    {
    }

    virtual int call(lua_State* state) override
    {
        function_base::call(state);

        auto args = lift_result([state] {
            return check<Args...>(state);
        });

        lift<void>(_fun, args);
        return 0;
    }

private:
    Fun _fun;
};


/*
 * Specialization for functions that want to manage their own stacks.
 * No argument and return value translation is done.
 */
template <typename Fun>
class function<Fun, int, lua_State*> : public function_base {
public:
    function(Fun fun, std::string const& meta)
        : function_base{meta},
          _fun{fun}
    {
    }

    virtual int call(lua_State* state) override
    {
        return _fun(state);
    }

private:
    Fun _fun;
};


namespace impl {

inline DLL_LOCAL int func_dispatcher(lua_State* state)
{
    function_base* fun = static_cast<function_base*>(
        lua_touserdata(state, lua_upvalueindex(1)));

    try {
        return fun->call(state);
    } catch (mond::error const& ex) {
        return luaL_error(state, ex.what());
    } catch (std::exception const& ex) {
        return luaL_error(state, ex.what());
    } catch (...) {
        return luaL_error(state, "unknown error");
    }
}

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
    auto tmp = std::make_unique<
        function<std::function<Ret (Args...)>, Ret, Args...>>(
            std::move(f), meta);

    lua_pushlightuserdata(s, static_cast<void*>(tmp.get()));
    lua_pushcclosure(s, &impl::func_dispatcher, 1);

    return std::move(tmp);
}

template <typename Ret, typename... Args>
std::unique_ptr<function_base> write_function(
    lua_State* s,
    Ret (*f)(Args...),
    std::string const& meta = "")
{
    auto tmp = std::make_unique<
        function<Ret (*)(Args...), Ret, Args...>>(
            f, meta);

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

    return write_function(s, std::move(ctor_wrapper), meta);
}

} // namespace mond

#endif // defined LIBMOND_FUNCTION_HH_INCLUDED
