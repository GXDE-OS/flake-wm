# Copyright (C) 2026 CharOfString <root@charofstring.cc>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.24)
project(flakewm VERSION 0.1.0 LANGUAGES CXX)
include(cmake/wlroots.cmake)

add_executable(flakewm
  src/main.cc
  src/utils/args_handler/args_handler.h
  src/utils/args_handler/args_handler.cc
  src/utils/log/ansi_escape_code_defs.h
  src/utils/log/log.h
  src/utils/log/log.cc
)

target_include_directories(flakewm PRIVATE ./)
target_compile_definitions(flakewm PRIVATE
  FLAKEWM_VERSION="${PROJECT_VERSION}"
)
target_compile_features(flakewm PRIVATE cxx_std_20)
target_link_libraries(flakewm PRIVATE Wlroots::Wlroots)
