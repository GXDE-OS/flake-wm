/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
 *
 * This file is part of FlakeWM.
 *
 * FlakeWM is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * FlakeWM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * FlakeWM. If not, see <https://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 * This file provides handles the startup arguments.
 */

#include <string>

#include "src/utils/log/log.h"

#ifndef SRC_UTILS_ARGS_HANDLER_ARGS_HANDLER_H_
#define SRC_UTILS_ARGS_HANDLER_ARGS_HANDLER_H_

namespace Flake {
namespace Utils {

struct StartupArgs {
  LogLevel info_level = LogLevel::kError;
  std::string process = "";
  bool disable_wayland = false;
  bool log_to_stdout = true;
  bool log_to_file = true;
  bool log_monotonic_time = false;
  bool nested = false;
  bool exit_flag = false;
};

class ArgsHandler {
 public:
  explicit ArgsHandler(char** argv);
  ~ArgsHandler() = default;

  static void PrintHelp();
  static void PrintVersion();
  StartupArgs GetArgs();

 private:
  StartupArgs args_;
};

}  // namespace Utils
}  // namespace Flake

#endif  // SRC_UTILS_ARGS_HANDLER_ARGS_HANDLER_H_
