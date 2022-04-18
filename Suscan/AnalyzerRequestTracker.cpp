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
  assert(this->m_analyzer != nullptr);

  this->m_pendingRequests[req.requestId] = req;

  try {
    this->m_analyzer->openEx(
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
  assert(this->m_analyzer != nullptr);

  try {
    this->m_analyzer->setInspectorId(req.handle, req.inspectorId, req.requestId);
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

  if (this->m_analyzer == nullptr)
    return false;

  request.requestId   = this->m_analyzer->allocateRequestId();
  request.inspectorId = this->m_analyzer->allocateInspectorId();
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
  for (auto &r : this->m_pendingRequests) {
    if (this->m_analyzer != nullptr && r.opened)
      this->m_analyzer->closeInspector(r.handle, r.requestId);

    emit cancelled(r);
  }

  this->m_pendingRequests.clear();
}

void
AnalyzerRequestTracker::setAnalyzer(Analyzer *analyzer)
{
  // Cancel all requests
  this->cancelAll();

  this->m_pendingRequests.clear();

  this->m_analyzer = analyzer;
}

void
AnalyzerRequestTracker::onInspectorMessage(
    const Suscan::InspectorMessage &message)
{
  auto it = this->m_pendingRequests.find(message.getRequestId());

  if (it != this->m_pendingRequests.end()) {
    switch (message.getKind()) {
      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_OPEN:
        it->handle = message.getHandle();
        it->opened = true;
        this->executeSetInspectorId(*it);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SET_ID:
        it->idSet = true;
        emit opened(*it, message.getCConfig());
        this->m_pendingRequests.remove(it->requestId);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_HANDLE:
        emit error(*it, "Wrong handle (server desync?)");
        this->m_pendingRequests.remove(it->requestId);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_OBJECT:
        emit error(*it, "Wrong object");
        this->m_pendingRequests.remove(it->requestId);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_KIND:
        emit error(*it, "Invalid message kind");
        this->m_pendingRequests.remove(it->requestId);
        break;

      default:
        break;
    }
  }
}

AnalyzerRequestTracker::~AnalyzerRequestTracker()
{

}
