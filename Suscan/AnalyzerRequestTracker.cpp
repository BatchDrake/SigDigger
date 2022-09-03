//
//    AnalyzerRequestTracker.cpp: Track state of synchronous requests
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
#include <Suscan/AnalyzerRequestTracker.h>
#include <Suscan/Analyzer.h>
#include <cassert>

using namespace Suscan;

Q_DECLARE_METATYPE(Suscan::AnalyzerRequest);

static bool g_registered = false;

AnalyzerRequestTracker::AnalyzerRequestTracker(QObject *parent) :
  QObject(parent)
{

}

bool
AnalyzerRequestTracker::executeOpenRequest(AnalyzerRequest const &req)
{
  assert(m_analyzer != nullptr);

  m_pendingRequests[req.requestId] = req;

  try {
    m_analyzer->openEx(
          req.inspClass,
          req.channel,
          req.precise,
          req.parent,
          req.requestId);
  } catch (Suscan::Exception &) {
    return false;
  }

  return true;
}

bool
AnalyzerRequestTracker::executeSetInspectorId(AnalyzerRequest const &req)
{
  assert(m_analyzer != nullptr);

  try {
    m_analyzer->setInspectorId(req.handle, req.inspectorId, req.requestId);
  } catch (Suscan::Exception &) {
    return false;
  }

  return true;
}

bool
AnalyzerRequestTracker::requestOpen(
    std::string const &inspClass,
    Channel const &channel,
    QVariant data,
    bool precise,
    Handle parent)
{
  AnalyzerRequest request;

  if (!g_registered) {
    qRegisterMetaType<Suscan::AnalyzerRequest>();
    g_registered = true;
  }

  if (m_analyzer == nullptr)
    return false;

  request.requestId   = m_analyzer->allocateRequestId();
  request.inspectorId = m_analyzer->allocateInspectorId();
  request.inspClass   = inspClass;
  request.channel     = channel;
  request.precise     = precise;
  request.parent      = parent;
  request.data        = data;

  return this->executeOpenRequest(request);
}

void
AnalyzerRequestTracker::cancelAll()
{
  for (auto &r : m_pendingRequests) {
    if (m_analyzer != nullptr && r.opened)
      m_analyzer->closeInspector(r.handle, r.requestId);

    emit cancelled(r);
  }

  m_pendingRequests.clear();
}

void
AnalyzerRequestTracker::setAnalyzer(Analyzer *analyzer)
{
  // Cancel all requests
  this->cancelAll();

  m_pendingRequests.clear();

  m_analyzer = analyzer;

  if (m_analyzer != nullptr)
    connect(
          m_analyzer,
          SIGNAL(inspector_message(const Suscan::InspectorMessage &)),
          this,
          SLOT(onInspectorMessage(const Suscan::InspectorMessage &)));
}

void
AnalyzerRequestTracker::onInspectorMessage(
    const Suscan::InspectorMessage &message)
{
  auto it = m_pendingRequests.find(message.getRequestId());

  if (it != m_pendingRequests.end()) {
    switch (message.getKind()) {
      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_OPEN:
        if (it->config == nullptr) {
          it->config = suscan_config_dup(message.getCConfig());
        }
        it->basebandRate = message.getBasebandRate();
        it->equivRate    = message.getEquivSampleRate();
        it->bandwidth    = message.getBandwidth();
        it->lo           = message.getLo();
        it->handle       = message.getHandle();
        it->spectSources = message.getSpectrumSources();
        it->estimators   = message.getEstimators();
        it->opened       = true;
        this->executeSetInspectorId(*it);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SET_ID:
        it->idSet = true;
        emit opened(*it);
        m_pendingRequests.remove(it->requestId);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_INVALID_CHANNEL:
        emit error(*it, "Invalid channel specification (invalid bandwidth / frequency)");
        m_pendingRequests.remove(it->requestId);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_HANDLE:
        emit error(*it, "Wrong handle (server desync?)");
        m_pendingRequests.remove(it->requestId);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_OBJECT:
        emit error(*it, "Wrong object");
        m_pendingRequests.remove(it->requestId);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_KIND:
        emit error(*it, "Invalid message kind");
        m_pendingRequests.remove(it->requestId);
        break;

      default:
        break;
    }
  }
}

AnalyzerRequestTracker::~AnalyzerRequestTracker()
{

}

AnalyzerRequest::AnalyzerRequest()
{

}

AnalyzerRequest::AnalyzerRequest(AnalyzerRequest const &prev)
{
  *this = prev;
}

AnalyzerRequest &
AnalyzerRequest::operator=(AnalyzerRequest const &prev)
{
  if (this->config != nullptr)
    free(this->config);

  this->requestId    = prev.requestId;
  this->inspectorId  = prev.inspectorId;
  this->inspClass    = prev.inspClass;
  this->channel      = prev.channel;
  this->precise      = prev.precise;
  this->parent       = prev.parent;
  this->handle       = prev.handle;
  this->data         = prev.data;
  this->opened       = prev.opened;
  this->idSet        = prev.idSet;
  this->basebandRate = prev.basebandRate;
  this->equivRate    = prev.equivRate;
  this->bandwidth    = prev.bandwidth;
  this->lo           = prev.lo;
  this->spectSources = prev.spectSources;
  this->estimators   = prev.estimators;

  this->config = nullptr;
  if (prev.config != nullptr)
    SU_ATTEMPT(this->config = suscan_config_dup(prev.config));

  return *this;
}

AnalyzerRequest::~AnalyzerRequest()
{
  if (this->config != nullptr)
    suscan_config_destroy(this->config);
}
