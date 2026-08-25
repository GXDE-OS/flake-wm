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

#include <stdlib.h>

#include "src/utils/args_handler/args_handler.h"

int main(int argc, char** argv) {
  // Parse command line arguments to basic configurations.
  Flake::Utils::ArgsHandler* args_helper = new Flake::Utils::ArgsHandler(argv);
  Flake::Utils::StartupArgs configs = args_helper->GetArgs();

  // If the exit flag is set, just exit.
  if (configs.exit_flag) {
    delete args_helper;
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}
