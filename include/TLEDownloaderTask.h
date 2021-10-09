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

#define TLE_DOWNLOADER_MAX_MEMORY_SIZE (1 << 24) // 16 MiB

namespace SigDigger {
  class TLEDownloaderTask : public Suscan::CancellableTask {
    Q_OBJECT

    CURLM *multi = nullptr;
    CURL  *curl = nullptr;
    FILE  *fp = nullptr;
    bool   ok = false;
    std::string data;

    static size_t curl_save_data(
        void *ptr,
        size_t,
        size_t nmemb,
        TLEDownloaderTask *self);

    static int curl_progress(
        void *self,
        double dltotal,
        double dlnow,
        double ultotal,
        double ulnow);

    void extractTLEs(void);
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
