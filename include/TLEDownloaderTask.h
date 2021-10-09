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
#ifndef TLEDOWNLOADERTASK_H
#define TLEDOWNLOADERTASK_H

#include <Suscan/CancellableTask.h>
#include <curl/curl.h>

namespace SigDigger {
  class TLEDownloaderTask : public Suscan::CancellableTask {
    Q_OBJECT

    CURLM *multi = nullptr;
    CURL  *curl = nullptr;
    FILE  *fp = nullptr;
    bool   ok = false;

    static int curl_progress(
        void *self,
        double dltotal,
        double dlnow,
        double ultotal,
        double ulnow);

  public:
    TLEDownloaderTask(
        QString url,
        QObject *parent = nullptr);

    ~TLEDownloaderTask() override;

    virtual bool work(void) override;
    virtual void cancel(void) override;
  };
}


#endif // TLEDOWNLOADERTASK_H
