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
    uint32_t    requestId;
    uint32_t    inspectorId;

    std::string inspClass;
    Channel     channel;
    bool        precise;
    Handle      parent;

    Handle      handle;

    QVariant    data;

    bool opened = false;
    bool idSet  = false;
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
    void opened(AnalyzerRequest const &req, const suscan_config_t *config);
    void cancelled(AnalyzerRequest const &req);
    void error(AnalyzerRequest const &req, const std::string &);

  public slots:
    void onInspectorMessage(const Suscan::InspectorMessage &message);
  };

}

#endif // SUSCAN_ANALYZERREQUESTTRACKER_H
