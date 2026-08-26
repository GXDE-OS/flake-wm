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
 * This is the logging utility of FlakeWM.
 */

#include "src/utils/log/log.h"

#include <atomic>
#include <iostream>
#include <ostream>
#include <string>

#include "src/utils/log/ansi_escape_code_defs.h"

namespace Flake {
namespace Utils {
namespace {

std::atomic<LogLevel>& MinimumLogLevel() {
  static std::atomic<LogLevel> level{LogLevel::kError};
  return level;
}

constexpr int LogPriority(LogLevel level) {
  switch (level) {
    case LogLevel::kInfo:
    case LogLevel::kOk:
      return 0;
    case LogLevel::kWarning:
      return 1;
    case LogLevel::kError:
      return 2;
  }

  return 0;
}

bool ShouldLog(LogLevel level) {
  return LogPriority(level) >= LogPriority(GetMinimumLogLevel());
}

/**
 * @brief Print the colored log level to stdout/stderr.
 *
 * This is an internal function. It prints the log header (e.g. @c [INFO]) to
 * @c stdout (or @c stderr, if the log level is @c LogLevel::kError.) These
 * headers
 * are colored using ANSI escape codes
 *
 * @see src/utils/log/ansi_escape_code_def.h for the color definitions.
 *
 * @param level The log level.
 */
std::ostream& LogStream(LogLevel level) {
  return level == LogLevel::kError ? std::cerr : std::cout;
}

void PrintLevelPrefix(LogLevel level) {
  std::ostream& output = LogStream(level);

  switch (level) {
    case LogLevel::kInfo:
      output << BOLD_BLUE_FORMAT << "[INFO]" << RESET_FORMAT << ' ';
      break;
    case LogLevel::kOk:
      output << BOLD_GREEN_FORMAT << "[ OK ]" << RESET_FORMAT << ' ';
      break;
    case LogLevel::kWarning:
      output << BOLD_YELLOW_FORMAT << "[WARNING]" << RESET_FORMAT << ' ';
      break;
    case LogLevel::kError:
      output << BOLD_RED_FORMAT << "[ERROR]" << RESET_FORMAT << ' ';
      break;
  }
}

}  // namespace

void SetMinimumLogLevel(LogLevel level) {
  MinimumLogLevel().store(level, std::memory_order_relaxed);
}

LogLevel GetMinimumLogLevel() {
  return MinimumLogLevel().load(std::memory_order_relaxed);
}

/**
 * @brief Print the log content to stdout/stderr.
 *
 * This function outputs the log message with the appropriate header. e.g.
 * [INFO] Hello.
 *
 * If the log level is @c LogLevel::kError, the message will be printed to
 * @c stderr.
 * Otherwise the log is printed through @c stdout.
 *
 * @param level The log level.
 * @param content The log message.
 */
void WriteLog(LogLevel level, const std::string& content) {
  if (!ShouldLog(level)) {
    return;
  }

  PrintLevelPrefix(level);
  LogStream(level) << content << '\n';
}

/**
 * @brief Print the log title and content to stdout/stderr.
 *
 * This function outputs the log title and message with the appropriate header.
 * e.g. [INFO] Greetings: Hello.
 *
 * In the example above, "Greetings" is the title and "Hello." is the message
 * body.
 *
 * If the log level is @c LogLevel::kError, the message will be printed to
 * @c stderr.
 * Otherwise the log is printed through @c stdout.
 *
 * @param level The log level.
 * @param title The log title.
 * @param content The log message.
 */
void WriteLogWithTitle(LogLevel level, const std::string& title,
    const std::string& content) {
  if (!ShouldLog(level)) {
    return;
  }

  PrintLevelPrefix(level);
  LogStream(level) << BOLD_FORMAT << title << RESET_FORMAT << ": " << content
    << '\n';
}

/**
 * @brief Print the log tag, title and content to stdout/stderr.
 *
 * This function outputs the log tag, title and message with the appropriate
 * header. e.g. [INFO] Main - Greetings: Hello.
 *
 * In the example above, "Main" is the tag, "Greetings" is the title and
 * "Hello." is the message body.
 *
 * If the log level is @c LogLevel::kError, the message will be printed to
 * @c stderr.
 * Otherwise the log is printed through @c stdout.
 *
 * @param level The log level.
 * @param tag The log tag.
 * @param title The log title.
 * @param content The log message.
 */
void WriteLogWithTag(LogLevel level, const std::string& tag,
    const std::string& title, const std::string& content) {
  if (!ShouldLog(level)) {
    return;
  }

  PrintLevelPrefix(level);
  LogStream(level) << BOLD_UNDERLINE_FORMAT << tag << " - " << title
    << RESET_FORMAT << ": " << content << '\n';
}

}  // namespace Utils
}  // namespace Flake
