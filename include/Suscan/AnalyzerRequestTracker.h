//
//    AnalyzerRequestTracker.h: Track state of synchronous requests
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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
#ifndef SUSCAN_ANALYZERREQUESTTRACKER_H
#define SUSCAN_ANALYZERREQUESTTRACKER_H

#include <QObject>
#include <Suscan/Message.h>
#include <Suscan/Messages/InspectorMessage.h>
#include <Suscan/Channel.h>
#include <QVariant>
#include <QMap>

namespace Suscan {
  class Analyzer;

  struct AnalyzerRequest {
    // Request fields
    uint32_t    requestId = 0;
    uint32_t    inspectorId = 0;
    std::string inspClass;
    Channel     channel;
    bool        precise = false;
    Handle      parent = -1;
    QVariant    data;

    // Request state
    bool opened = false;
    bool idSet  = false;

    // Response fields
    Handle      handle = -1;
    unsigned    basebandRate = 0;
    float       equivRate = 0;
    float       bandwidth = 0;
    float       lo = 0;

    std::vector<SpectrumSource> spectSources;
    std::vector<Estimator> estimators;

    suscan_config_t *config = nullptr;

    AnalyzerRequest();
    AnalyzerRequest(AnalyzerRequest const &);
    AnalyzerRequest &operator= (AnalyzerRequest const &);

    ~AnalyzerRequest();
  };

  class AnalyzerRequestTracker: public QObject {
    Q_OBJECT

    Analyzer *m_analyzer = nullptr;

    QMap<uint32_t, AnalyzerRequest> m_pendingRequests;

    bool executeOpenRequest(AnalyzerRequest const &);
    bool executeSetInspectorId(AnalyzerRequest const &);

  public:
    bool requestOpen(
        std::string const &inspClass,
        Channel const &ch,
        QVariant v = QVariant(),
        bool precise = true,
        Handle parent = -1);
    void setAnalyzer(Analyzer *);
    void cancelAll();

    AnalyzerRequestTracker(QObject *parent = nullptr);
    ~AnalyzerRequestTracker() override;

  signals:
    // Always remember to put full namespaces here
    void opened(Suscan::AnalyzerRequest const &);
    void cancelled(Suscan::AnalyzerRequest const &);
    void error(Suscan::AnalyzerRequest const &, const std::string &);

  public slots:
    void onInspectorMessage(const Suscan::InspectorMessage &message);
  };

}

#endif // SUSCAN_ANALYZERREQUESTTRACKER_H
