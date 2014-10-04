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

#include "tokenbucket.hh"

#include <cstdint>

#include <chrono>
#include <algorithm>


tokenbucket::tokenbucket(
    num_type cap,
    num_type rate,
    num_type minc)
        : _tokens{cap},
          _capacity{cap},
          _fill_rate{rate},
          _min_consume{minc},
          _last_update{std::chrono::system_clock::now()}
{
}

tokenbucket::num_type tokenbucket::consume(num_type tokens)
{
    generate();

    tokens = std::max(tokens, _min_consume);

    if (tokens <= _tokens) {
        _tokens -= tokens;

        return tokens;
    } else {
        return 0;
    }
}

tokenbucket::num_type tokenbucket::available()
{
    generate();

    return _tokens;
}

void tokenbucket::generate()
{
    auto now = std::chrono::system_clock::now();

    if (_tokens < _capacity) {
        std::chrono::duration<double> dur = now - _last_update;

        // Only update last update time if the amount of newly generated tokens
        // is at least 1 to prevent getting stuck on 0.
        if (num_type ntoks = _fill_rate * dur.count()) {
            _tokens = std::min(_capacity, _tokens + ntoks);
            _last_update = std::chrono::system_clock::now();
        }
    }

}
