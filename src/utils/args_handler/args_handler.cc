/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <stdio.h>

# include "src/utils/args_handler/args_handler.h"

namespace Flake {
namespace Utils {

void ArgsHandler::PrintHelp() {
  std::cout << "Usage: flakewm [options] [command]\n";
  std::cout << "  -h, --help               Show help message and quit.\n";
  std::cout << "  -d, --debug              "
    << "Enables full logging, including debug information.\n";
  std::cout << "  -D, --debug <options>    "
    << "noxwayland, logtostdout, logtofile or loginmtime.\n";
  std::cout << "  -s, --session <process>  Run session on startup\n";
  std::cout << "  -v, --version            Show the version number & quit.\n";
  std::cout << "  -V, --verbose            Enables more verbose logging.\n";
  std::cout << "-----------------------------------------------------------\n";
  std::cout << "Debug options:\n";
  std::cout << "  -Dnoxwayland    Disable XWayland support\n";
  std::cout << "  -Dlogtostdout   Log to stdout "
    << "(by default, kept for compatibility)\n";
  std::cout << "  -Dlogtofile     Log to $HOME/.log/flakewm.log\n";
  std::cout << "  -Dloginmtime    Log with monotonic time\n";
}

}  // namespace Utils
}  // namespace Flake
