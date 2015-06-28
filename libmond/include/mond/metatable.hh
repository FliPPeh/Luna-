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
#ifndef LIBMOND_METATABLE_HH_INCLUDED
#define LIBMOND_METATABLE_HH_INCLUDED

#include "mond/state.hh"
#include "mond/function.hh"

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
    virtual void apply(metatable<T>&) const = 0;
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

    virtual void apply(metatable<T>& mt) const override
    {
        auto ptr = _memptr;

        std::function<Ret (T*, Args...)> fun =
            [ptr](T* self, Args... args) {
                return ptr(self, args...);
            };

        lua_getfield(mt._state, mt._meta, "__index");
        mt._state.register_function(
            std::move(write_function(mt._state, std::move(fun), mt._name)));

        lua_setfield(mt._state, -2, _name.c_str());
    }

protected:
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

    virtual void apply(metatable<T>& mt) const override
    {
        auto ptr = _memptr;
        std::string meta = mt._name;

        std::function<int (lua_State*)> fun =
            [ptr, meta](lua_State* s) {
                T* self = static_cast<T*>(luaL_checkudata(s, 1, meta.c_str()));

                return ptr(self, s);
            };

        lua_getfield(mt._state, mt._meta, "__index");
        mt._state.register_function(
            std::move(write_function(mt._state, fun, mt._name)));

        lua_setfield(mt._state, -2, _name.c_str());
    }

protected:
    template <typename U>
    friend class metatable;

    std::string _name;
    std::function<int (T*, lua_State*)> _memptr;
};

template <typename T, typename R, typename... Args>
metamethod<T, R, Args...> method(
    std::string const& name,
    R (T::*ptr)(Args...))
{
    return metamethod<T, R, Args...>{name, ptr};
}

template <typename T, typename R, typename... Args>
metamethod<T, R, Args...> method(
    std::string const& name,
    R (T::*ptr)(Args...) const)
{
    return metamethod<T, R, Args...>{name, ptr};
}


template <typename T, typename Ret, typename... Args>
class metametamethod : public metamethod<T, Ret, Args...> {
    using metamethod<T, Ret, Args...>::_name;
    using metamethod<T, Ret, Args...>::_memptr;

public:
    using metamethod<T, Ret, Args...>::metamethod;

    virtual void apply(metatable<T>& mt) const override
    {
        auto ptr = _memptr;

        std::function<Ret (T* self, Args...)> fun =
            [ptr](T* self, Args... args) {
                return ptr(self, args...);
            };

        mt._state.register_function(
            std::move(write_function(mt._state, std::move(fun), mt._name)));

        lua_setfield(mt._state, mt._meta, _name.c_str());
    }
};

template <typename T>
class metametamethod<T, int, lua_State*> : public metamethod<T, int, lua_State*> {
    using metamethod<T, int, lua_State*>::_name;
    using metamethod<T, int, lua_State*>::_memptr;

public:
    using metamethod<T, int, lua_State*>::metamethod;

    virtual void apply(metatable<T>& mt) const override
    {
        auto ptr = _memptr;
        std::string meta = mt._name;

        std::function<int (lua_State*)> fun =
            [ptr, meta](lua_State* s) {
                T* self = static_cast<T*>(luaL_checkudata(s, 1, meta.c_str()));

                return ptr(self, s);
            };

        mt._state.register_function(
            std::move(write_function(mt._state, fun, mt._name)));

        lua_setfield(mt._state, mt._meta, _name.c_str());
    }
};

template <typename T, typename R, typename... Args>
metametamethod<T, R, Args...> meta_method(
    std::string const& name,
    R (T::*ptr)(Args...))
{
    return metametamethod<T, R, Args...>{name, ptr};
}

template <typename T, typename R, typename... Args>
metametamethod<T, R, Args...> meta_method(
    std::string const& name,
    R (T::*ptr)(Args...) const)
{
    return metametamethod<T, R, Args...>{name, ptr};
}


class state;

namespace impl {

template <typename T>
void run_dtor(T* self)
{
    self->~T();
}

}; // namespace impl

template <typename T>
class metatable {
public:
    metatable(state& state, focus& focus, std::string const& name)
        : _state{state},
          _name{name},
          _meta{-1}
    {
        if (luaL_newmetatable(_state, name.c_str()) != 1) {
            throw mond::runtime_error{
                "Unable to register metatable `" + name + "'"};
        }

        _meta = lua_gettop(_state);

        lua_newtable(_state);
        lua_setfield(_state, _meta, "__index");

        _state.register_function(
            std::move(write_function(_state, &impl::run_dtor<T>, _name)));

        lua_setfield(_state, _meta, "__gc");
    }

    template <typename Ret, typename... Args>
    metatable& operator<<(metamethod<T, Ret, Args...> const& meth)
    {
        meth.apply(*this);

        return *this;
    }

private:
    template <typename U, typename Ret, typename... Args>
    friend class metamethod;

    template <typename U, typename Ret, typename... Args>
    friend class metametamethod;

    state& _state;

    std::string _name;
    int _meta;
};

} // namespace mond

#endif // defined LIBMOND_METATABLE_HH_INCLUDED
