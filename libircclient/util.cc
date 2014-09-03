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

#include "util.h"

#include <cstddef>

#include <string>
#include <vector>

std::vector<std::string> split_noempty(
    std::string const& src,
    std::string const& sep)
{
    std::size_t seppos = 0;

    std::vector<std::string> results;

    while (seppos != std::string::npos) {
        std::size_t old = seppos ? seppos + sep.size() : 0;

        seppos = src.find(sep, seppos + 1);

        std::string sub = src.substr(old, seppos - old);

        if (!sub.empty()) {
            results.push_back(sub);
        }
    }

    return results;
}
