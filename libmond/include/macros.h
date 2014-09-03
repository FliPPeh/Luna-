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
#ifndef MACRO_H
#define MACRO_H

#if __GNUC__ >= 4
  #define HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
  #define HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
  #define HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
  #define HELPER_DLL_IMPORT
  #define HELPER_DLL_EXPORT
  #define HELPER_DLL_LOCAL
#endif

#define DLL_PUBLIC HELPER_DLL_EXPORT
#define DLL_LOCAL  HELPER_DLL_LOCAL

#endif // defined MACRO_H
