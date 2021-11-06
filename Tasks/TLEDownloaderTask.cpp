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
#include <Suscan/Library.h>

using namespace SigDigger;

#if LIBCURL_VERSION_MAJOR <= 7 && LIBCURL_VERSION_MINOR < 66
#  define curl_multi_poll curl_multi_wait
#endif

size_t
TLEDownloaderTask::curl_save_data(
    void *ptr,
    size_t size,
    size_t nmemb,
    TLEDownloaderTask *self)
{
  size_t chunksize = size * nmemb;

  if (self->data.size() + chunksize > TLE_DOWNLOADER_MAX_MEMORY_SIZE)
    chunksize = TLE_DOWNLOADER_MAX_MEMORY_SIZE - self->data.size();

  if (chunksize > 0) {
    const char *asStr = reinterpret_cast<const char *>(ptr);
    self->data.append(asStr, chunksize);
  }

  return nmemb;
}

int
TLEDownloaderTask::curl_progress(
    void *self,
    double dltotal,
    double dlnow,
    double,
    double)
{
  TLEDownloaderTask *task = reinterpret_cast<TLEDownloaderTask *>(self);

  task->setStatus("Downloading data");
  task->setProgress(dlnow / dltotal);

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
    curl_easy_setopt(this->curl, CURLOPT_URL, url.toStdString().c_str());
    curl_easy_setopt(this->curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(this->curl, CURLOPT_PROGRESSFUNCTION, TLEDownloaderTask::curl_progress);
    curl_easy_setopt(this->curl, CURLOPT_PROGRESSDATA, this);
    curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, TLEDownloaderTask::curl_save_data);
    curl_easy_setopt(this->curl, CURLOPT_WRITEDATA, this);
  }

  curl_multi_add_handle(this->multi, this->curl);

  this->setProgress(0);
  this->setStatus("Performing request...");

  this->ok = true;
}

void
TLEDownloaderTask::extractTLEs(void)
{
  SUSDIFF got;
  const char *data = this->data.data();
  size_t size = this->data.size();
  orbit_t orbit = orbit_INITIALIZER;
  auto sus = Suscan::Singleton::get_instance();

  while ((got = orbit_init_from_data(&orbit, data, size)) > 0) {
    std::string chunk;
    orbit_finalize(&orbit);
    chunk.append(data, static_cast<size_t>(got));
    (void) sus->registerTLE(chunk);
    data += got;
    size -= static_cast<size_t>(got);
  }
}

bool
TLEDownloaderTask::work(void)
{
  int running = 0;
  bool downloadFinished = false;
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
        downloadFinished = true;
  }

  if (!running) {
    if (!downloadFinished) {
      emit error("Download aborted");
    } else {
      this->extractTLEs();
      emit done();
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

