//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this program.  If not, see
//    <http://www.gnu.org/licenses/>
//

#ifndef LOG_H
#define LOG_H

#include <vector>
#include <mutex>

#include <sigutils/log.h>

namespace Suscan {
  struct LoggerMessage {
    enum sigutils_log_severity severity;
    struct timeval time;
    std::string domain;
    std::string function;
    unsigned int line;
    std::string message;
  };

  class Logger {
  private:
    static Logger *instance; // Singleton instance
    std::mutex mutex;
    std::vector<LoggerMessage> messages;

    static void log_func(
        void *privdata,
        const struct sigutils_log_message *message);

    Logger(void);
    void push(const struct sigutils_log_message *message);

  public:
    static Logger *getInstance(void);

    void flush(void);

    void lock(void);
    void unlock(void);

    std::vector<LoggerMessage>::const_iterator begin(void);
    std::vector<LoggerMessage>::const_iterator end(void);

  };
};

#endif // LOG_H
