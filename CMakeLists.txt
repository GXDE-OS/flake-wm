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
project(flakewm LANGUAGES CXX)
include(cmake/wlroots.cmake)

add_executable(flakewm
  src/main.cc
)

target_include_directories(flakewm ./)
target_compile_features(flakewm PRIVATE cxx_std_20)
target_link_libraries(flakewm PRIVATE Wlroots::Wlroots)
