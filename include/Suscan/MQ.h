//
//    MQ.h: Message queuing API
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


#ifndef CPP_MQ_H
#define CPP_MQ_H

#include <Suscan/Compat.h>

#include <analyzer/mq.h>

// This class must be thread safe
namespace Suscan {
  class MQ {
  private:
    struct suscan_mq mq;
    bool mq_initialized;

    friend class Analyzer;

  public:
    void *read(uint32_t &type);

    MQ();
    ~MQ();
  };
};

#endif // CPP_MQ_H
