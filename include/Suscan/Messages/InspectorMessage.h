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
  struct OrbitReport {
    bool loan = false;
    struct suscan_orbit_report local_info;
    struct suscan_orbit_report *c_info = nullptr;

    OrbitReport()
    {
      this->c_info = &this->local_info;
    }

    OrbitReport(struct suscan_orbit_report *ptr, bool loan = false)
    {
      this->loan = loan;

      if (loan) {
        this->c_info = ptr;
      } else {
        this->local_info = *ptr;
        this->c_info = &this->local_info;
      }
    }

    struct timeval const &
    getRxTime() const
    {
      return this->c_info->rx_time;
    }

    xyz_t const &
    getAzel() const
    {
      return this->c_info->satpos;
    }

    SUFLOAT
    getFrequencyCorrection() const
    {
      return this->c_info->freq_corr;
    }

    SUDOUBLE
    getVlosVelocity() const
    {
      return this->c_info->vlos_vel;
    }
  };


  class InspectorMessage: public Message {
  private:
    struct suscan_analyzer_inspector_msg *message = nullptr; // Convenience reference
    std::vector<SpectrumSource> sources;
    std::vector<Estimator> estimators;
    OrbitReport report;
    Config config;

  public:
    enum suscan_analyzer_inspector_msgkind getKind() const;
    suscan_config_t const *getCConfig() const;
    RequestId getRequestId() const;
    InspectorId getInspectorId() const;
    uint32_t getSpectrumSourceId() const;
    Handle getHandle() const;
    SUFLOAT *getSpectrumData() const;
    SUSCOUNT getSpectrumLength() const;
    SUSCOUNT getSpectrumRate() const;
    unsigned int getBasebandRate() const;
    SUFLOAT  getEquivSampleRate() const;
    SUFLOAT  getBandwidth() const;
    SUFLOAT  getLo() const;
    SUFLOAT  getEstimation() const;
    EstimatorId getEstimatorId() const;
    std::string getClass() const;
    std::vector<SpectrumSource> const &getSpectrumSources() const;
    std::vector<Estimator> const &getEstimators() const;
    Channel getChannel() const;
    OrbitReport const &getOrbitReport() const;
    bool isTLEEnabled() const;
    std::string getSignalName() const;
    SUDOUBLE    getSignalValue() const;

    InspectorMessage();
    InspectorMessage(struct suscan_analyzer_inspector_msg *msg);
  };
};

#endif // MESSAGES_INSPECTOR_MESSAGE_H
