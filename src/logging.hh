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
#ifndef LUNA_LOGGING_HH_INCLUDED
#define LUNA_LOGGING_HH_INCLUDED

#include <string>
#include <sstream>
#include <functional>

enum logging_level : int {
    DEBUG, INFO, WARN, ERROR, WTF
};

enum logging_flags {
    ANSI = 0x01
};

class logger {
public:
    class logger_helper {
    public:
        logger_helper(logger const& parent, logging_level level);
        ~logger_helper();

        template <typename T>
        logger_helper& operator<<(T&& e)
        {
            if (_level >= _parent->_minlevel) {
                std::ostringstream strm;
                strm << e;

                _msg += strm.str();
            }

            return *this;
        }

    private:
        logger const* _parent;
        logging_level _level;
        std::string _msg;
    };


    logger(
        std::string name = "",
        logging_level minlevel = logging_level::INFO,
        int flags = 0);

    logger(logger const&) = default;
    logger(logger&&) = default;

    void change_level(logging_level l);
    logging_level level() const;

    logger& operator=(logger const&) = default;
    logger& operator=(logger&&) = default;

    logger_helper debug() const;
    logger_helper info()  const;
    logger_helper warn()  const;
    logger_helper error() const;
    logger_helper wtf()   const;


private:
    void do_log(std::string const& msg, logging_level lvl) const;

    std::string make_timestamp() const;

private:
    std::string _name;
    logging_level _minlevel;
    int           _flags;

    // TODO: meh..
    std::string _col_timestamp;
    std::string _col_name;
    std::string _col_level;

    std::string _col_debug;
    std::string _col_info;
    std::string _col_warn;
    std::string _col_error;
    std::string _col_wtf;
    std::string _col_reset;

};

#endif // defined LUNA_LOGGING_HH_INCLUDED
