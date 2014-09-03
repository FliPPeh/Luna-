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
#ifndef TOKENBUCKET_H
#define TOKENBUCKET_H

#include <cstdint>
#include <chrono>

/*! \brief A simple token bucket implementation for rate limiting.
 *
 * What exactly a token represents is up to the user. It could be actual tokens,
 * or it could be a number of bytes for throttling network traffic.
 */
class tokenbucket {
public:
    //! The numeric type used in the implementation.
    using num_type = uint_fast32_t;

    /*!
     * \param cap  Maximum token capacity.
     * \param rate Token refill rate per second.
     * \param minc The minimum amount of tokens that will be consumed.
     */
    tokenbucket(num_type cap, num_type rate, num_type minc = 0);

    /*! \brief Consumes tokens. Triggers generation.
     *
     * Attempts to consume at most \p tokens, but at least as many as set as
     * minimum) tokens.
     *
     * \param tokens The maximum amount of tokens to consume
     * \return Amount of tokens actually consumed, or `0` if not enough were
     *         available for consumption (or `0` was both set as minimum
     *         consumption and the value of \p tokens).
     */
    num_type consume(num_type tokens);

    /*! \brief Return amount of consumable tokens left. Triggers generation.
     *
     * \return Amount of usable tokens.
     */
    num_type available();

private:
    //! \brief Generates new tokens based upon the last update time.
    void generate();

private:
    //! Current amount of tokens left to use
    num_type _tokens;

    num_type _capacity;    //!< Maximum capacity of tokens.
    num_type _fill_rate;   //!< Token refill rate per second.
    num_type _min_consume; //!< Minimum amount of tokens that will be consumed.

    std::chrono::time_point<std::chrono::system_clock>
        _last_update;      //!< Last update.
};

#endif // defined TOKENBUCKET_H
