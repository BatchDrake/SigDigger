//
//    InspectorMessage.h: Inspector Message
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

#ifndef MESSAGES_INSPECTOR_MESSAGE_H
#define MESSAGES_INSPECTOR_MESSAGE_H

#include <Suscan/Compat.h>
#include <Suscan/Message.h>
#include <Suscan/Channel.h>
#include <Suscan/Config.h>
#include <Suscan/SpectrumSource.h>
#include <Suscan/Estimator.h>

#include <analyzer/analyzer.h>

namespace Suscan {
  class InspectorMessage: public Message {
  private:
    struct suscan_analyzer_inspector_msg *message = nullptr; // Convenience reference
    std::vector<SpectrumSource> sources;
    std::vector<Estimator> estimators;

    Config config;

  public:
    enum suscan_analyzer_inspector_msgkind getKind(void) const;
    suscan_config_t const *getCConfig(void) const;
    RequestId getRequestId(void) const;
    InspectorId getInspectorId(void) const;
    Handle getHandle(void) const;
    SUFLOAT *getSpectrumData(void) const;
    SUSCOUNT getSpectrumLength(void) const;
    SUSCOUNT getSpectrumRate(void) const;
    unsigned int getBasebandRate(void) const;
    SUFLOAT  getEquivSampleRate(void) const;
    SUFLOAT  getBandwidth(void) const;
    SUFLOAT  getLo(void) const;
    SUFLOAT  getEstimation(void) const;
    EstimatorId getEstimatorId(void) const;
    std::string getClass(void) const;
    std::vector<SpectrumSource> const &getSpectrumSources(void) const;
    std::vector<Estimator> const &getEstimators(void) const;
    Channel getChannel(void) const;

    InspectorMessage();
    InspectorMessage(struct suscan_analyzer_inspector_msg *msg);
  };
};

#endif // MESSAGES_INSPECTOR_MESSAGE_H
