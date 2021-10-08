//
//    TLEDownloaderTask.cpp: download TLE files from the Internet
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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

#include <TLEDownloaderTask.h>

using namespace SigDigger;

int
TLEDownloaderTask::curl_progress(
    void *self,
    double dltotal,
    double dlnow,
    double,
    double)
{
  TLEDownloaderTask *task = reinterpret_cast<TLEDownloaderTask *>(self);

  emit task->progress(dlnow / dltotal, "Downloading data");

  return 0;
}


TLEDownloaderTask::TLEDownloaderTask(
    QString url,
    QObject *parent) : Suscan::CancellableTask(parent)
{
  this->multi = curl_multi_init();
  this->curl  = curl_easy_init();

  if (this->curl != nullptr && this->multi) {
    // curl_easy_setopt(curl, CURLOPT_PROXY, proxy);

    curl_easy_setopt(this->curl, CURLOPT_USERAGENT, "SigDigger TLE Downloader/curl");
    curl_easy_setopt(this->curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(this->curl, CURLOPT_URL, url.data());
    curl_easy_setopt(this->curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(this->curl, CURLOPT_PROGRESSFUNCTION, TLEDownloaderTask::curl_progress);
    curl_easy_setopt(this->curl, CURLOPT_PROGRESSDATA, this);
  }

  curl_multi_add_handle(this->multi, this->curl);

  this->ok = true;
}

bool
TLEDownloaderTask::work(void)
{
  int running = 0;
  bool done = false;
  CURLMcode mc;
  CURLMsg *msg;
  int left;

  if (!this->ok) {
    emit error("CURL initialization failed");
    return false;
  } else {
    mc = curl_multi_perform(this->multi, &running);
    if (running) {
      mc = curl_multi_poll(this->multi, nullptr, 0, 1000, nullptr);
      if (mc != CURLM_OK) {
        emit error("CURL poll error: " + QString(curl_multi_strerror(mc)));
        return false;
      }
    }

    while ((msg = curl_multi_info_read(this->multi, &left)))
      if (msg->msg == CURLMSG_DONE)
        done = true;
  }

  if (!running) {
    if (!done) {
      emit error("Download aborted");
    } else {
      // Process data. Don't care to block
    }
  }
  return running != 0;
}

void
TLEDownloaderTask::cancel(void)
{
  emit cancelled();
}

TLEDownloaderTask::~TLEDownloaderTask()
{
  if (this->multi != nullptr)
    curl_multi_cleanup(this->multi);

  if (this->curl != nullptr)
    curl_easy_cleanup(this->curl);

  if (this->fp != nullptr)
    fclose(this->fp);
}

