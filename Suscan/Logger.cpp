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

#include <Suscan/Logger.h>

using namespace Suscan;

Logger *Logger::instance = nullptr;

void
Logger::log_func(void *privdata, const struct sigutils_log_message *message)
{
  Logger *logger = static_cast<Logger *>(privdata);

  logger->push(message);
}

Logger::Logger(void)
{
  struct sigutils_log_config config;

  config.priv = this;
  config.exclusive = SU_FALSE;
  config.log_func = log_func;

  su_log_init(&config);
}

void
Logger::push(const struct sigutils_log_message *message)
{
  struct LoggerMessage msg;

  msg.severity = message->severity;
  msg.time     = message->time;
  msg.line     = message->line;

  msg.domain   = std::string(message->domain);
  msg.function = std::string(message->function);
  msg.message  = std::string(message->message);

  std::lock_guard<std::mutex>(this->mutex);
  this->messages.push_back(std::move(msg));
}

void
Logger::flush(void)
{
  std::lock_guard<std::mutex>(this->mutex);
  this->messages.clear();
}

void
Logger::lock(void)
{
  this->mutex.lock();
}

void
Logger::unlock(void)
{
  this->mutex.unlock();
}

Logger *
Logger::getInstance(void)
{
  if (instance == nullptr)
    instance = new Logger();

  return instance;
}

std::vector<LoggerMessage>::const_iterator
Logger::begin(void)
{
  return this->messages.begin();
}

std::vector<LoggerMessage>::const_iterator
Logger::end(void)
{
  return this->messages.end();
}
