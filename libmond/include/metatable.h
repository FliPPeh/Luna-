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
#ifndef LUA_METATABLE_H
#define LUA_METATABLE_H

#include "state.h"
#include "function.h"

#include <lua.hpp>

#include <string>
#include <memory>
#include <utility>
#include <functional>

namespace mond {

template <typename T>
class metatable;


template <typename T>
class meta_element {
    virtual void apply(metatable<T>&) = 0;
};


template <typename T, typename Ret, typename... Args>
class metamethod : public meta_element<T> {
public:
    metamethod(std::string name, Ret (T::*ptr)(Args...))
        : _name{std::move(name)},
          _memptr{std::mem_fun(ptr)}
    {
    }

    metamethod(std::string name, Ret (T::*ptr)(Args...) const)
        : _name{std::move(name)},
          _memptr{std::mem_fn(ptr)}
    {
    }

    virtual void apply(metatable<T>& mt) override
    {
        auto ptr = _memptr;

        std::function<Ret (T* self, Args...)> fun =
            [ptr](T* self, Args... args) {
                return ptr(self, args...);
            };

        lua_getfield(mt._state, mt._meta, "__index");
        mt._state._functions.emplace_back(
            std::move(write_function(mt._state, fun, mt._name)));

        lua_setfield(mt._state, -2, _name.c_str());
    }

private:
    template <typename U>
    friend class metatable;

    std::string _name;
    std::function<Ret (T*, Args...)> _memptr;
};

template <typename T>
class metamethod<T, int, lua_State*> : public meta_element<T> {
public:
    metamethod(std::string name, int (T::*ptr)(lua_State*))
        : _name{std::move(name)},
          _memptr{std::mem_fun(ptr)}
    {
    }

    metamethod(std::string name, int (T::*ptr)(lua_State*) const)
        : _name{std::move(name)},
          _memptr{std::mem_fn(ptr)}
    {
    }

    virtual void apply(metatable<T>& mt) override
    {
        auto ptr = _memptr;
        std::string meta = mt._name;

        std::function<int (lua_State*)> fun =
            [ptr, meta](lua_State* s) {
                T* self = static_cast<T*>(luaL_checkudata(s, 1, meta.c_str()));

                return ptr(self, s);
            };

        lua_getfield(mt._state, mt._meta, "__index");
        mt._state._functions.emplace_back(
            std::move(write_function(mt._state, fun, mt._name)));

        lua_setfield(mt._state, -2, _name.c_str());
    }

private:
    template <typename U>
    friend class metatable;

    std::string _name;
    std::function<int (T*, lua_State*)> _memptr;
};

/*
template <typename T, typename... Args>
class metaconstructor : public meta_element<T> {
public:
    metaconstructor(std::string name)
        : _name{name}
    {
    }

    virtual void apply(metatable<T>& mt) override
    {
        mt._focus[_name].template new_constructor<T, Args...>(mt._name);
    }

private:
    template <typename U>
    friend class metatable;

    std::string _name;
};
*/

template <typename T, typename R, typename... Args>
metamethod<T, R, Args...> method(
    std::string const& name,
    R (T::*ptr)(Args...) const)
{
    return metamethod<T, R, Args...>{name, ptr};
}

template <typename T, typename R, typename... Args>
metamethod<T, R, Args...> method(
    std::string const& name,
    R (T::*ptr)(Args...))
{
    return metamethod<T, R, Args...>{name, ptr};
}

/*
template <typename T, typename... Args>
metaconstructor<T, Args...> constructor(std::string const& name)
{
    return metaconstructor<T, Args...>{name};
};
*/

class state;
class focus;

template <typename T>
class metatable {
public:
    metatable(state& state, focus& focus, std::string const& name)
        : _state{state},
          _focus{focus},
          _name{name},
          _meta{-1}
          //_fields{}
    {
        if (luaL_newmetatable(_state, name.c_str()) != 1) {
            throw mond::runtime_error{
                "Unable to register metatable `" + name + "'"};
        }

        _meta = lua_gettop(_state);

        //lua_pushvalue(_state, _meta);
        lua_newtable(_state);
        lua_setfield(_state, _meta, "__index");

        std::function<void (T*)> dtor = [](T* self) {
            self->~T();
        };

        _state._functions.emplace_back(
            std::move(write_function(_state, dtor, _name)));

        lua_setfield(_state, _meta, "__gc");
    }

    template <typename Ret, typename... Args>
    metatable& operator<<(metamethod<T, Ret, Args...> meth)
    {
        meth.apply(*this);

        return *this;
    }

    /*
    template <typename... Args>
    metatable& operator<<(metaconstructor<T, Args...> ctor)
    {
        ctor.apply(*this);
        //_fields.push_back(ctor._name);

        return *this;
    }
    */

private:
    template <typename U, typename Ret, typename... Args>
    friend class metamethod;

    /*
    template <typename U, typename... Args>
    friend class metaconstructor;
    */

    state& _state;
    focus& _focus;

    std::string _name;
    int _meta;

    //std::vector<std::string> _fields;
};

} // namespace mond

#endif // defined LUA_METATABLE_H
