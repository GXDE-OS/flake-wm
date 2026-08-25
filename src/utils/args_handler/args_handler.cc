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
#include <string>
#include <unordered_set>

# include "src/utils/args_handler/args_handler.h"

namespace Flake {
namespace Utils {
namespace {

const std::unordered_set<std::string> kValidOptions = {
  "-h", "--help",
  "-d", "-D", "--debug",
  "-s", "--session",
  "-v", "--version",
  "-V", "--verbose"
};

const std::unordered_set<std::string> kValidDebugOptions = {
  "noxwayland", "logtostdout", "logtofile", "loginmtime"
};

const std::unordered_set<std::string> kValidDebugArgs = {
  "-Dnoxwayland", "-Dlogtostdout", "-Dlogtofile", "-Dloginmtime"
};

}  // namespace

// Initializes Argument handler by parsing the command line arguments
// to configurations.
ArgsHandler::ArgsHandler(char** argv) {
  if (argv[1] == nullptr) {
    PrintHelp();
    args_.exit_flag = true;
    return;
  }

  for (int i = 1; argv[i] != nullptr; ++i) {
    std::string cur = argv[i];
    if (cur == "-h" || cur == "--help") {
      PrintHelp();
      args_.exit_flag = true;
      return;
    }

    if (cur == "-d" || cur == "-D" || cur == "--debug") {
      args_.info_level = 2;
      for (int j = i + 1; argv[j] != nullptr; ++j) {
        std::string cur_option = argv[j];
        if (kValidDebugOptions.find(cur_option) != kValidDebugOptions.end()) {
          if (cur_option == "noxwayland") {
            args_.disable_wayland = true;
            continue;
          }

          if (cur_option == "logtostdout") {
            args_.log_to_stdout = true;
            continue;
          }

          if (cur_option == "logtofile") {
            args_.log_to_file = true;
            continue;
          }

          if (cur_option == "loginmtime") {
            args_.log_monotonic_time = true;
            continue;
          }

          i += 1;
          break;
        }
        break;
      }
      continue;
    }

    if (kValidDebugArgs.find(cur) != kValidDebugArgs.end()) {
      if (cur == "-Dnoxwayland") {
        args_.disable_wayland = true;
        continue;
      }

      if (cur == "-Dlogtostdout") {
        args_.log_to_stdout = true;
        continue;
      }

      if (cur == "-Dlogtofile") {
        args_.log_to_file = true;
        continue;
      }

      if (cur == "-Dloginmtime") {
        args_.log_monotonic_time = true;
        continue;
      }
    }

    if (cur == "-s" || cur == "--session") {
      if (argv[i + 1] != nullptr) {
        args_.process = argv[i + 1];
        i += 1;
      } else {
        std::cerr << "E: Missing argument for session.\n";
        args_.exit_flag = true;
        return;
      }
      continue;
    }

    if (cur == "-v" || cur == "--version") {
      PrintVersion();
      args_.exit_flag = true;
      return;
    }

    if (cur == "-V" || cur == "--verbose") {
      args_.info_level = 1;
      continue;
    }
  }
}

// Simply returns the parsed arguments as a StartupArgs struct.
StartupArgs ArgsHandler::GetArgs() {
  return args_;
}

// Prints usage help.
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

// Prints the version number of FlakeWM.
void ArgsHandler::PrintVersion() {
  std::cout << "FlakeWM version 0.1.0\n";
}

}  // namespace Utils
}  // namespace Flake
