//
//    RMSInspector.cpp: description
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#include "RMSInspector.h"
#include "RMSViewTab.h"
#include "ui_RMSInspector.h"
#include "SuWidgetsHelpers.h"
#include "UIMediator.h"
#include "Default/FFT/FFTWidget.h"
#include "SigDiggerHelpers.h"
#include <sys/stat.h>
#include <QFileDialog>

// I will never accept this
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  define DirectoryOnly Directory
#endif

using namespace SigDigger;

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
RMSInspectorConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(integrate);
  LOAD(integrationTime);
  LOAD(dBscale);
  LOAD(autoFit);
  LOAD(timeAxisType);
  LOAD(autoScroll);
  LOAD(currScaleMin);
  LOAD(currScaleMax);
  LOAD(logData);
  LOAD(logDir);
  LOAD(logFormat);
  LOAD(host);
  LOAD(port);
}

Suscan::Object &&
RMSInspectorConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("RMSInspectorConfig");

  STORE(integrate);
  STORE(integrationTime);
  STORE(dBscale);
  STORE(autoFit);
  STORE(timeAxisType);
  STORE(autoScroll);
  STORE(currScaleMin);
  STORE(currScaleMax);
  STORE(logData);
  STORE(logDir);
  STORE(logFormat);
  STORE(host);
  STORE(port);

  return this->persist(obj);
}

QString
RMSInspector::getInspectorTabTitle() const
{

  QString result = " for "
      + SuWidgetsHelpers::formatQuantity(
        this->request().channel.fc + this->mediator()->getCurrentCenterFreq(),
        "Hz");

  return "RMS plot" + result;
}

void
RMSInspector::connectAll()
{
  connect(
        m_rmsTab,
        SIGNAL(integrationTimeChanged(qreal)),
        this,
        SLOT(onConfigChanged()));

  connect(
        ui->tabWidget,
        SIGNAL(currentChanged(int)),
        this,
        SLOT(onTabChanged()));

  connect(
        ui->passBandSpectrum,
        SIGNAL(pandapterRangeChanged(float, float)),
        this,
        SLOT(onRangeChanged(float, float)));

  connect(
        ui->freqLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onChangeLo(void)));

  connect(
        ui->bandwidthLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onChangeBandwidth(void)));

  connect(
        ui->logCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleDataLogger()));

  connect(
        ui->browseButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onBrowseDirectory()));

  connect(
        ui->formatCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onConfigChanged()));

  connect(
        ui->hostEdit,
        SIGNAL(textEdited(QString)),
        this,
        SLOT(onConfigChanged()));

  connect(
        ui->portSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onConfigChanged()));

  connect(
        m_rmsTab,
        SIGNAL(viewTypeChanged()),
        this,
        SLOT(onRMSTabViewChanged()));
}

RMSInspector::RMSInspector(
    InspectionWidgetFactory *factory,
    Suscan::AnalyzerRequest const &request,
    UIMediator *mediator,
    QWidget *parent) :
  InspectionWidget(factory, request, mediator, parent),
  ui(new Ui::RMSInspector())
{
  qreal intTime = RMS_INSPECTOR_DEFAULT_INTEGRATION_TIME_MS * 1e-3;
  ui->setupUi(this);

  m_rawMode = request.inspClass == "raw";

  m_rmsTab = new RMSViewTab(nullptr, nullptr);
  m_sampleRate = SCAST(qreal, request.equivRate);

  ui->freqLcd->setValue(SCAST(qint64, request.lo));
  ui->bandwidthLcd->setValue(SCAST(qint64, request.bandwidth));

  ui->freqLcd->setMin(-SCAST(int, request.basebandRate) / 2);
  ui->freqLcd->setMax(SCAST(int, request.basebandRate) / 2);

  ui->bandwidthLcd->setMin(0);
  ui->bandwidthLcd->setMax(SCAST(int, request.equivRate));

  m_rmsTab->setIntegrationTimeMode(1 / m_sampleRate, 24 * 3600);

  m_rmsTab->setSampleRate(1 / intTime);
  m_rmsTab->setIntegrationTimeHint(intTime);

  ui->tabWidget->insertTab(0, m_rmsTab, "Power plot");
  ui->tabWidget->setCurrentIndex(0);

  // Initialize data logger
  m_datasaverParams = hashlist_new();

  registerDataSaver(
        "Comma-separated values (CSV)",
        suscli_datasaver_params_init_csv);
  registerDataSaver(
        "Binary MAT v5 file",
        suscli_datasaver_params_init_mat5);
  registerDataSaver(
        "Matlab script",
        suscli_datasaver_params_init_matlab);
  registerDataSaver(
        "TCP socket forwarding",
        suscli_datasaver_params_init_tcp);
  connectAll();

  updateMaxSamples();
}

void
RMSInspector::closeRequested()
{
  if (!m_uiConfig->autoFit) {
    m_uiConfig->currScaleMin = m_rmsTab->getMin();
    m_uiConfig->currScaleMax = m_rmsTab->getMax();
  }

  InspectionWidget::closeRequested();
}

RMSInspector::~RMSInspector()
{
  if (m_datasaver != nullptr)
    suscli_datasaver_destroy(m_datasaver);

  if (m_datasaverParams != nullptr)
    hashlist_destroy(m_datasaverParams);

  delete ui;
}


void
RMSInspector::registerDataSaver(
    QString const &desc,
    datasaver_param_init_cb initializer)
{
  auto index = m_datasaverList.size();

  m_datasaverList.resize(index + 1);

  (initializer) (&m_datasaverList[index], m_datasaverParams);

  ui->formatCombo->insertItem(SCAST(int, index), desc);
}

void
RMSInspector::pushPowerSample(qreal mean)
{
  if (m_analyzer != nullptr) {

    struct timeval currTv, diff;
    struct timeval tv = m_analyzer->getSourceTimeStamp();

    if (m_rmsTab->running()) {
      if (m_firstMeasurement) {
        if (!m_uiConfig->autoFit) {
          if (m_uiConfig->dBscale)
            m_rmsTab->setVerticalLimitsDb(
                  m_uiConfig->currScaleMin,
                  m_uiConfig->currScaleMax);
          else
            m_rmsTab->setVerticalLimitsLinear(
                  m_uiConfig->currScaleMin,
                  m_uiConfig->currScaleMax);
        }
        m_firstMeasurement = false;
      }

      m_rmsTab->feed(
            SCAST(qreal, tv.tv_sec) + 1e-6 * SCAST(qreal, tv.tv_usec),
            mean);
    }

    ++m_updates;

    if (m_datasaver != nullptr) {
      gettimeofday(&currTv, nullptr);
      timersub(&currTv, &m_lastUpdate, &diff);

      if (diff.tv_sec >= 1) {
        struct stat sbuf;
        m_lastUpdate = currTv;

        if (stat(m_fullPathStd.c_str(), &sbuf) != -1) {
          ui->sizeLabel->setText(
                SuWidgetsHelpers::formatBinaryQuantity(sbuf.st_size));
        } else {
          ui->sizeLabel->setText(QString::number(m_updates) + " points");
        }

      }

      if (!suscli_datasaver_write_timestamp(
            m_datasaver,
            &tv,
            SU_ASFLOAT(mean))) {
        // Delete datalogger
        suscli_datasaver_destroy(m_datasaver);
        m_datasaver = nullptr;

        ui->currentFileLabel->setText("N/A");
        ui->sizeLabel->setText("0 bytes");

        ui->logCheck->setChecked(false);
        m_uiConfig->logData = false;

        refreshUi();

        QMessageBox::warning(
              this,
              "Datasaver closed",
              "The current data logger closed unexpectedly. Open the log window for details");
      }
    }
  }
}

void
RMSInspector::checkMaxSamples()
{
  if (m_count >= m_maxSamples) {
    if (m_count > 0) {
      qreal mean = m_kahanAcc / SCAST(qreal, m_count);
      pushPowerSample(mean);
    }

    m_kahanC = m_kahanAcc = 0;
    m_count = 0;
  }
}

void
RMSInspector::feedSpectrum(
    const SUFLOAT *data,
    SUSCOUNT len,
    SUSCOUNT rate,
    uint32_t)
{
#define WATERFALL_CALL(x) ui->passBandSpectrum->x
  if (m_lastRate != rate) {
    WATERFALL_CALL(setSampleRate(SCAST(float, rate)));
    m_lastRate = rate;
  }

  m_fftData.resize(len);
  m_fftData.assign(data, data + len);

  WATERFALL_CALL(setNewFftData(
        static_cast<float *>(m_fftData.data()),
        SCAST(int, len)));

  if (false) {
    SUFLOAT min = +INFINITY;
    SUFLOAT max = -INFINITY;

    for (SUSCOUNT i = 0; i < len; ++i) {
      if (min > data[i])
        min = data[i];
      if (max < data[i])
        max = data[i];
    }

    if (!isinf(min) && !isinf(max)) {
      unsigned int updates;
      SUFLOAT range = max - min;
      SUFLOAT spacing = .1f * range;
      WATERFALL_CALL(setPandapterRange(min - max - spacing, spacing));
      WATERFALL_CALL(setWaterfallRange(min - max - spacing, spacing));

      // The inspector spectrum size is len, with 100 ms update period.
      // If the sample rate is fs, len / (fs * .1) is the number of intermediate
      // updates we need to wait for the spectrum to stabilize.

      updates = static_cast<unsigned>(len / (m_sampleRate * .1f));

      if (m_spectrumAdjustCounter++ > updates)
        m_haveSpectrumLimits = true;
    }
  }

  if (m_lastLen != len) {
    int res = SCAST(int,
          round(SCAST(qreal, rate) / SCAST(qreal, len)));
    if (res < 1)
      res = 1;

    m_lastLen = len;
    WATERFALL_CALL(resetHorizontalZoom());
    WATERFALL_CALL(setClickResolution(res));
    WATERFALL_CALL(setFilterClickResolution(res));
  }
#undef WATERFALL_CALL
}

void
RMSInspector::updateMaxSamples()
{
  qreal maxTime = m_rmsTab->getIntegrationTimeHint();

  if (m_uiConfig != nullptr)
    m_uiConfig->integrationTime = maxTime;
  m_maxSamples = SCAST(quint64, maxTime * m_sampleRate);

  if (m_rawMode)
    checkMaxSamples();
  else if (m_analyzer != nullptr) {
    m_config.set("power.integrate-samples", SCAST(uint64_t, m_maxSamples));
    m_analyzer->setInspectorConfig(m_request.handle, m_config);
  }
}

/////////////////////////// Overriden methods /////////////////////////////////
void
RMSInspector::attachAnalyzer(Suscan::Analyzer *analyzer)
{
  m_analyzer = analyzer;

  ui->passBandSpectrum->setCenterFreq(
        SCAST(qint64, m_analyzer->getFrequency()) + ui->freqLcd->getValue());

  connect(
        analyzer,
        SIGNAL(source_info_message(Suscan::SourceInfoMessage const &)),
        this,
        SLOT(onSourceInfoMessage(Suscan::SourceInfoMessage const &)));

  onToggleDataLogger();

  if (!m_rawMode) {
    m_config.set("power.integrate-samples", SCAST(uint64_t, m_maxSamples));
    m_analyzer->setInspectorConfig(m_request.handle, m_config);
  }
}

void
RMSInspector::detachAnalyzer()
{
  m_analyzer = nullptr;

  onToggleDataLogger();
}

void
RMSInspector::setProfile(Suscan::Source::Config &profile)
{
  m_tunerFreq = profile.getFreq();

  ui->passBandSpectrum->setCenterFreq(m_tunerFreq + ui->freqLcd->getValue());
}

void
RMSInspector::setTimeStamp(struct timeval const &)
{
}

void
RMSInspector::setQth(Suscan::Location const &)
{

}

void
RMSInspector::setColorConfig(ColorConfig const &cfg)
{
  QString styleSheet =
      "background-color: " + cfg.lcdBackground.name() + "; \
      color: " + cfg.lcdForeground.name() + "; \
      font-size: 12px; \
      font-family: Monospace; \
      font-weight: bold;";

  m_rmsTab->setColorConfig(cfg);

  ui->freqLcd->setBackgroundColor(cfg.lcdBackground);
  ui->freqLcd->setForegroundColor(cfg.lcdForeground);

  ui->bandwidthLcd->setBackgroundColor(cfg.lcdBackground);
  ui->bandwidthLcd->setForegroundColor(cfg.lcdForeground);

  ui->freqLabel->setStyleSheet(styleSheet);
  ui->bandwidthLabel->setStyleSheet(styleSheet);
  ui->padLabel->setStyleSheet(styleSheet);

#define WATERFALL_CALL(x) ui->passBandSpectrum->x
  WATERFALL_CALL(setFftPlotColor(cfg.spectrumForeground));
  WATERFALL_CALL(setFftBgColor(cfg.spectrumBackground));
  WATERFALL_CALL(setFftAxesColor(cfg.spectrumAxes));
  WATERFALL_CALL(setFftTextColor(cfg.spectrumText));
  WATERFALL_CALL(setFilterBoxColor(cfg.filterBox));
#undef WATERFALL_CALL
}

void
RMSInspector::inspectorMessage(Suscan::InspectorMessage const &msg)
{
  SUFLOAT *data;
  SUSCOUNT len, p;
  float x;

  switch (msg.getKind()) {
    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SPECTRUM:
      data = msg.getSpectrumData();
      len = msg.getSpectrumLength();
      p = len / 2;

      for (auto i = 0u; i < len; ++i)
        data[i] = SU_POWER_DB(data[i]);

      for (auto i = 0u; i < len / 2; ++i) {
        x = data[i];
        data[i] = data[p];
        data[p] = x;

        if (++p == len)
          p = 0;
      }

      feedSpectrum(
            data,
            len,
            msg.getSpectrumRate(),
            msg.getSpectrumSourceId());
      break;
    default:
      break;
  }
}

void
RMSInspector::samplesMessage(Suscan::SamplesMessage const &samplesMsg)
{
  unsigned int count, i;
  qreal input, y, t;
  const SUCOMPLEX *samples = samplesMsg.getSamples();
  count = samplesMsg.getCount();

  if (m_rawMode) {
    // TODO: We can make this actually a little better. Look for the max
    // number of samples we can process in a row before calling
    // checkMaxSamples and then deliver to the rmstab

    for (i = 0; i < count; ++i) {
      input = SCAST(qreal, SU_C_REAL(samples[i] * SU_C_CONJ(samples[i])));
      y = input - m_kahanC;
      t = m_kahanAcc + y;

      m_kahanC = (t - m_kahanAcc) - y;
      m_kahanAcc = t;

      ++m_count;
      checkMaxSamples();
    }
  } else {
    for (i = 0; i < count; ++i)
      pushPowerSample(SU_C_REAL(samples[i]));
  }
}

void
RMSInspector::refreshUi()
{
  bool enabled = ui->logCheck->isChecked();
  bool haveDataLogger;
  int page = 0;

  if (m_analyzer == nullptr)
    enabled = false;

  if (ui->formatCombo->currentIndex() == 3)
    page = 1;

  ui->stackedWidget->setCurrentIndex(page);

  haveDataLogger = m_datasaver != nullptr;

  ui->tabWidget->setTabText(
        ui->tabWidget->indexOf(ui->loggingTab),
        haveDataLogger ? "Data logger [logging]" : "Data logger");

  ui->formatCombo->setEnabled(!enabled);
  ui->stackedWidget->setEnabled(!enabled);
  ui->page->setEnabled(!enabled);
  ui->page_2->setEnabled(!enabled);
}

Suscan::Serializable *
RMSInspector::allocConfig()
{
  m_uiConfig = new RMSInspectorConfig();

  return m_uiConfig;
}

void
RMSInspector::applyConfig()
{
  FFTWidgetConfig config;
  QString dir = QString::fromStdString(m_uiConfig->logDir);

  bool blocked = m_rmsTab->blockSignals(true);
  m_rmsTab->setIntegrationTimeHint(SCAST(qreal, m_uiConfig->integrationTime));
  m_rmsTab->setAutoFit(m_uiConfig->autoFit);
  m_rmsTab->setAutoScroll(m_uiConfig->autoScroll);
  m_rmsTab->setLogScale(m_uiConfig->dBscale);
  m_rmsTab->setTimeScaleSelection(m_uiConfig->timeAxisType);
  m_rmsTab->blockSignals(blocked);

  if (dir == "")
    dir = QDir::currentPath();

  ui->hostEdit->blockSignals(true);
  ui->portSpin->blockSignals(true);
  ui->formatCombo->blockSignals(true);
  ui->logCheck->blockSignals(true);
  ui->directoryEdit->blockSignals(true);

  ui->logCheck->setChecked(m_uiConfig->logData);
  ui->directoryEdit->setText(dir);


  if (m_uiConfig->logFormat == "csv")
    ui->formatCombo->setCurrentIndex(0);
  else if (m_uiConfig->logFormat == "mat5")
    ui->formatCombo->setCurrentIndex(1);
  else if (m_uiConfig->logFormat == "matlab")
    ui->formatCombo->setCurrentIndex(2);
  else if (m_uiConfig->logFormat == "tcp")
    ui->formatCombo->setCurrentIndex(3);

  ui->hostEdit->setText(m_uiConfig->host.c_str());
  ui->portSpin->setValue(m_uiConfig->port);

  ui->hostEdit->blockSignals(false);
  ui->portSpin->blockSignals(false);
  ui->formatCombo->blockSignals(false);
  ui->logCheck->blockSignals(false);
  ui->directoryEdit->blockSignals(false);

  refreshUi();
  updateMaxSamples();

  try {
    config.deserialize(
          mediator()->getAppConfig()->getComponentConfig("FFTWidget"));
  } catch (Suscan::Exception &) {}

  auto palette = SigDiggerHelpers::instance()->getPalette(config.palette);

  if (palette != nullptr)
    ui->passBandSpectrum->setPalette(palette->getGradient());
}


void
RMSInspector::showEvent(QShowEvent *)
{

}

void
RMSInspector::floatStart()
{

}

void
RMSInspector::floatEnd()
{

}


std::string
RMSInspector::getLabel() const
{
  return this->getInspectorTabTitle().toStdString();
}

//////////////////////////////// Slots /////////////////////////////////////////
void
RMSInspector::onConfigChanged()
{
  switch (ui->formatCombo->currentIndex()) {
    case 0:
      m_uiConfig->logFormat = "csv";
      break;

    case 1:
      m_uiConfig->logFormat = "mat5";
      break;

    case 2:
      m_uiConfig->logFormat = "matlab";
      break;

    case 3:
      m_uiConfig->logFormat = "tcp";
      break;
  }

  m_uiConfig->host = ui->hostEdit->text().toStdString();
  m_uiConfig->port = ui->portSpin->value();

  refreshUi();
  updateMaxSamples();
}

void
RMSInspector::onTabChanged()
{
  if (m_analyzer != nullptr) {
    if (ui->tabWidget->currentIndex() == 1) {
      m_analyzer->setSpectrumSource(
              this->request().handle,
              1,
              static_cast<Suscan::RequestId>(rand()));
    } else {
      m_analyzer->setSpectrumSource(
              this->request().handle,
              0,
              static_cast<Suscan::RequestId>(rand()));
    }
  }
}

void
RMSInspector::onRangeChanged(float min, float max)
{
  bool blocked = ui->passBandSpectrum->blockSignals(true);

  ui->passBandSpectrum->setWaterfallRange(min, max);
  ui->passBandSpectrum->blockSignals(blocked);
}

void
RMSInspector::onChangeLo()
{
  if (m_analyzer != nullptr) {
    int lo = SCAST(int, this->ui->freqLcd->getValue());
    m_analyzer->setInspectorFreq(
        this->request().handle,
        lo,
        0);

    if (m_haveNamedChannel) {
      m_namedChannel.value()->frequency = m_tunerFreq + lo;
      ui->passBandSpectrum->setCenterFreq(m_namedChannel.value()->frequency);
      refreshNamedChannel();
    }
  }
}

void
RMSInspector::onChangeBandwidth()
{
  if (m_analyzer != nullptr) {
    int bw = SCAST(int, this->ui->bandwidthLcd->getValue());
    m_analyzer->setInspectorBandwidth(
        this->request().handle,
        bw,
        0);
    if (m_haveNamedChannel) {
      auto halfBw = bw / 2;
      m_namedChannel.value()->lowFreqCut  = -halfBw;
      m_namedChannel.value()->highFreqCut = +halfBw;
      refreshNamedChannel();
    }
  }
}

void
RMSInspector::onSourceInfoMessage(Suscan::SourceInfoMessage const &msg)
{
  m_tunerFreq = SCAST(qint64, msg.info()->getFrequency());

  ui->passBandSpectrum->setCenterFreq(m_tunerFreq + ui->freqLcd->getValue());
}

const suscli_datasaver_params *
RMSInspector::currentDataSaverParams()
{
  int index = ui->formatCombo->currentIndex();
  if (index == -1)
    return nullptr;

  return &m_datasaverList[index];
}

void
RMSInspector::onToggleDataLogger()
{
  bool enabled = ui->logCheck->isChecked();
  bool haveDataLogger = m_datasaver != nullptr;
  bool hintFailed = false;
  bool haveHint = false;

  if (m_analyzer == nullptr)
    enabled = false;

  if (enabled != haveDataLogger) {
    if (enabled) {
      const suscli_datasaver_params *params = currentDataSaverParams();
      if (params != nullptr) {
        haveHint = true;
        char *file = nullptr;
        if (params->fname != nullptr) {
          file = suscli_datasaver_get_filename(params);

          if (file != nullptr) {
            m_dataFile = file;
            m_fullPathStd = (ui->directoryEdit->text() + "/" + m_dataFile).toStdString();
            free(file);
          } else {
            hintFailed = true;
          }
        }

        if (hintFailed) {
          QMessageBox::critical(
                this,
                "Internal error",
                "Selected datasaver failed to provide a filename hint");
        } else {
          std::string host = ui->hostEdit->text().toStdString();
          std::string port = std::to_string(ui->portSpin->value());
          std::string interval = std::to_string(1e3 * m_uiConfig->integrationTime);

          m_t0 = m_analyzer->getSourceTimeStamp();

          if (haveHint) {
            hashlist_set(m_datasaverParams, "path", const_cast<char *>(m_fullPathStd.c_str()));
            ui->currentFileLabel->setText(m_dataFile);
          }

          hashlist_set(m_datasaverParams, "host", const_cast<char *>(host.c_str()));
          hashlist_set(m_datasaverParams, "port", const_cast<char *>(port.c_str()));
          hashlist_set(m_datasaverParams, "interval", const_cast<char *>(interval.c_str()));

          hashlist_set(m_datasaverParams, "_t0", &m_t0);

          m_updates   = 0;
          m_datasaver = suscli_datasaver_new(params);

          if (m_datasaver == nullptr) {
            QMessageBox::critical(
                  this,
                  "Internal error",
                  "Failed to create datasaver object. See log messages for details.");
          }


          gettimeofday(&m_lastUpdate, NULL);
        }
      }

    } else {
      // Delete datalogger
      suscli_datasaver_destroy(m_datasaver);
      m_datasaver = nullptr;

      ui->currentFileLabel->setText("N/A");
      ui->sizeLabel->setText("0 bytes");
    }
  }

  haveDataLogger = m_datasaver != nullptr;

  if (m_analyzer != nullptr)
    ui->logCheck->setChecked(haveDataLogger);

  enabled = ui->logCheck->isChecked();
  m_uiConfig->logData = enabled;

  refreshUi();
}

void
RMSInspector::onBrowseDirectory()
{
  QFileDialog dialog(this->ui->browseButton);

  dialog.setFileMode(QFileDialog::DirectoryOnly);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setWindowTitle(QString("Select current save directory"));

  if (dialog.exec()) {
    QString path = dialog.selectedFiles().first();
    this->ui->directoryEdit->setText(path);
    m_uiConfig->logDir = path.toStdString();
  }
}

void
RMSInspector::onRMSTabViewChanged()
{
  m_uiConfig->autoFit = m_rmsTab->isAutoFit();
  m_uiConfig->autoScroll = m_rmsTab->isAutoScroll();
  m_uiConfig->dBscale = m_rmsTab->isLogScale();
  m_uiConfig->timeAxisType = m_rmsTab->getTimeScaleSelection();
}

