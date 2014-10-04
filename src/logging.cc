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

#include "logging.hh"

#include <ctime>

#include <string>
#include <sstream>
#include <iostream>
#include <utility>
#include <chrono>


logger::logger_helper::logger_helper(logger const& parent, logging_level level)
    : _parent{&parent},
      _level{level},
      _msg{}
{
}

logger::logger_helper::~logger_helper()
{
    if (_level >= _parent->_minlevel) {
        _parent->do_log(_msg, _level);
    }
}



logger::logger(std::string name, logging_level minlevel, int flags)
    : _name{std::move(name)},
      _minlevel{minlevel},
      _flags{flags},

      // TODO: meh..
      _col_timestamp{},
      _col_name{},
      _col_level{},
      _col_debug{},
      _col_info{},
      _col_warn{},
      _col_error{},
      _col_wtf{},
      _col_reset{}
{
    // TODO: meh..
    if (_flags & logging_flags::ANSI) {
        _col_level = "\033[0;36m";
        _col_name  = "\033[1;32m";
        _col_debug = "\033[1m";
        _col_info  = "";
        _col_warn  = "\033[0;31m";
        _col_error = "\033[1;31m";
        _col_wtf   = "\033[1;36m";
        _col_reset = "\033[0m";
    }
}


logger::logger_helper logger::debug() const
{
    return logger_helper{*this, logging_level::DEBUG};
}

logger::logger_helper logger::info() const
{
    return logger_helper{*this, logging_level::INFO};
}

logger::logger_helper logger::warn() const
{
    return logger_helper{*this, logging_level::WARN};
}

logger::logger_helper logger::error() const
{
    return logger_helper{*this, logging_level::ERROR};
}

logger::logger_helper logger::wtf() const
{
    return logger_helper{*this, logging_level::WTF};
}



void logger::do_log(const std::string& msg, logging_level lvl) const
{
    std::ostream& out = lvl > logging_level::INFO ? std::cerr : std::cout;

    out << _col_timestamp << make_timestamp() << _col_reset << ": ";

    std::string col;

    out << _col_level;

    switch (lvl) {
        case logging_level::DEBUG: out << "(DEBUG) "; col = _col_debug; break;
        case logging_level::INFO:  out << "(INFO ) "; col = _col_info;  break;
        case logging_level::WARN:  out << "(WARN ) "; col = _col_warn;  break;
        case logging_level::ERROR: out << "(ERROR) "; col = _col_error; break;
        case logging_level::WTF:   out << "( WTF ) "; col = _col_wtf;   break;
    }

    out << _col_reset;

    if (not _name.empty()) {
        out << _col_name << "[" << _name << "]" << _col_reset<< ": ";
    }

    out << col << msg << _col_reset << std::endl;
}


std::string logger::make_timestamp() const
{
    std::ostringstream tso;

    std::chrono::duration<double> ts =
        std::chrono::system_clock::now().time_since_epoch();

    std::time_t secs = static_cast<std::time_t>(ts.count());
    unsigned msecs = (ts.count() - secs) * 1000;

    char timestamp[128];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S",
        std::localtime(&secs));

    tso << timestamp << ",";
    tso.width(3);
    tso.fill('0');
    tso << msecs;

    return tso.str();
}

