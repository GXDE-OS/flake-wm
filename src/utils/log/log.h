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

#ifndef SRC_UTILS_LOG_LOG_H_
#define SRC_UTILS_LOG_LOG_H_

#include <string>

namespace Flake {
namespace Utils {

/**
 * @brief Log level used to control output style and destination stream.
 */
enum class LogLevel {
  kInfo,
  kOk,
  kWarning,
  kError,
};

/**
 * @brief Set the minimum log level printed by all logging functions.
 *
 * @param level The minimum log level.
 */
void SetMinimumLogLevel(LogLevel level);

/**
 * @brief Return the process-wide minimum log level.
 */
LogLevel GetMinimumLogLevel();

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
void WriteLog(LogLevel level, const std::string& content);

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
  const std::string& content);

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
  const std::string& title, const std::string& content);

}  // namespace Utils
}  // namespace Flake

#endif  // SRC_UTILS_LOG_LOG_H_
