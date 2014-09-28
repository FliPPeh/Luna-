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

#ifndef LUNA_SCRIPT_H
#define LUNA_SCRIPT_H

#include "logging.h"

#include "irc_utils.h"

#include <mond.h>

#include <string>

class luna;

class luna_script {
public:
    static constexpr char const* api = "__luna";

    luna_script(luna& context, std::string file);
    ~luna_script();

    luna_script(luna_script const&) = delete;
    luna_script(luna_script&&) = default;

    luna_script& operator=(luna_script const&) = delete;
    luna_script& operator=(luna_script&&) = default;

    void init_script();

    template <typename... Args>
    void emit_signal(std::string const& signal, Args&&... args)
    {
        try {
            _lua["luna"]["handle_signal"].call(
                signal, std::forward<Args>(args)...);
        } catch (mond::error const& e) {
            _logger.warn() << "Signal handler failed: " << e.what();
        }
    }

    std::string file() const;

    std::string name()        const;
    std::string description() const;
    std::string version()     const;

    friend bool operator==(luna_script const& me, luna_script const& other);
    friend bool operator!=(luna_script const& me, luna_script const& other);

    static irc::unordered_rfc1459_map<std::string, std::string> shared_vars;

private:
    void setup_api();

    void register_environment();
    void register_self();
    void register_script();
    void register_user();
    void register_channel();
    void register_channel_user();

private:
    friend class luna_script_proxy;

    luna* _context;

    logger _logger;

    std::string _file;
    mutable mond::state _lua;

    std::string _script_name;
    std::string _script_descr;
    std::string _script_version;
};

#endif // defined LUNA_SCRIPT_H
