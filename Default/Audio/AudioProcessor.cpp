//
//    AudioProcessor.cpp: Audio processor
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
#include "AudioProcessor.h"
#include "AudioPlayback.h"
#include "UIMediator.h"

#include <AppConfig.h>
#include <SuWidgetsHelpers.h>
#include <Suscan/AnalyzerRequestTracker.h>
#include <cassert>

using namespace SigDigger;

AudioProcessor::AudioProcessor(UIMediator *mediator, QObject *parent)
  : QObject(parent)
{
  m_mediator = mediator;

  try {
    m_playBack = new AudioPlayback("default", m_sampleRate);
    m_tracker = new Suscan::AnalyzerRequestTracker(this);

    // Connects the tracker
    this->connectAll();
  } catch (std::runtime_error &e) {
    m_audioError = e.what();
    m_playBack = nullptr;
  }
}

AudioProcessor::~AudioProcessor()
{
  if (m_audioCfgTemplate != nullptr)
    suscan_config_destroy(m_audioCfgTemplate);

  if (m_playBack != nullptr)
    delete m_playBack;
}

void
AudioProcessor::connectAll()
{
  connect(
        this->m_tracker,
        SIGNAL(opened(Suscan::AnalyzerRequest const &)),
        this,
        SLOT(onOpened(Suscan::AnalyzerRequest const &)));

  connect(
        this->m_tracker,
        SIGNAL(cancelled(Suscan::AnalyzerRequest const &)),
        this,
        SLOT(onCancelled(Suscan::AnalyzerRequest const &)));

  connect(
        this->m_tracker,
        SIGNAL(error(Suscan::AnalyzerRequest const &, const std::string &)),
        this,
        SLOT(onError(Suscan::AnalyzerRequest const &, const std::string &)));
}

bool
AudioProcessor::openAudio()
{
  // Opening audio is a multi-step, asynchronous process, that involves:
  // 1. Performing the request through the request tracker
  // 2. Signaling the completion of the request
  // 3. Setting channel properties asynchronously and waiting for its
  //    completion
  // 4. Signal audio open back to the user

  bool opening = false;

  assert(m_analyzer != nullptr);

  if (m_opening)
    return true;

  if (!m_opened) {
    if (m_playBack != nullptr) {
      Suscan::Channel ch;
      unsigned int reqRate = m_requestedRate;

      m_maxAudioBw =
          SU_MIN(
            SCAST(SUFREQ, m_analyzer->getSampleRate() / 2),
            2e5);

      // FIXME: Find a sample rate that better matches this
      if (reqRate > m_maxAudioBw)
        reqRate = SCAST(unsigned int, floor(m_maxAudioBw));

      // Configure sample rate
      m_playBack->setVolume(m_volume);
      m_playBack->setSampleRate(reqRate);
      m_playBack->start();

      m_sampleRate = m_playBack->getSampleRate();

      // Prepare channel
      ch.bw    = m_maxAudioBw;
      ch.ft    = 0;
      ch.fc    = this->calcTrueLoFreq();
      ch.fLow  = -.5 * m_maxAudioBw;
      ch.fHigh = +.5 * m_maxAudioBw;

      if (ch.fc > m_maxAudioBw || ch.fc < -m_maxAudioBw)
        ch.fc = 0;

      // Async step 1: track request
      opening = m_tracker->requestOpen("audio", ch);

      if (!opening) {
        emit audioError("Internal Suscan error while opening audio inspector");
        m_playBack->stop();
      }
    } else {
      emit audioError("Cannot enable audio, playback support failed to start");
    }

    m_opening = opening;
  }

  m_mediator->setUIBusy(opening);

  return opening;
}

bool
AudioProcessor::closeAudio()
{
  m_mediator->setUIBusy(false);

  assert(m_analyzer != nullptr);

  if (m_opening || m_opened) {
    // Inspector opened: close it
    if (m_audioInspectorOpened)
      m_analyzer->closeInspector(m_audioInspHandle);

    if (!m_opened)
      m_tracker->cancelAll();

    m_playBack->stop();
  }

  // Just in case
  this->stopRecording();

  m_opening = false;
  m_opened  = false;
  m_settingRate = false;
  m_audioInspectorOpened = false;

  return true;
}

SUFREQ
AudioProcessor::calcTrueBandwidth() const
{
  SUFREQ bw = m_bw;

  if (m_demod == AudioDemod::USB || m_demod == AudioDemod::LSB)
    bw *= .5;

  if (bw > m_maxAudioBw)
    bw = m_maxAudioBw;
  else if (bw < 1)
    bw = 1;

  return bw;
}

SUFREQ
AudioProcessor::calcTrueLoFreq() const
{
  SUFREQ delta = 0;
  SUFREQ bw = this->calcTrueBandwidth();

  if (m_demod == AudioDemod::USB)
    delta += .5 * bw;
  else if (m_demod == AudioDemod::LSB)
    delta -= .5 * bw;

  return m_lo + delta;
}

void
AudioProcessor::setTrueLoFreq()
{
  assert(m_analyzer != nullptr);
  assert(m_audioInspectorOpened);

  m_analyzer->setInspectorFreq(m_audioInspHandle, this->calcTrueLoFreq());
}

void
AudioProcessor::setTrueBandwidth()
{
  assert(m_analyzer != nullptr);
  assert(m_audioInspectorOpened);

  m_analyzer->setInspectorBandwidth(
        m_audioInspHandle,
        this->calcTrueBandwidth());
}

void
AudioProcessor::setParams()
{
  assert(m_audioCfgTemplate != nullptr);
  assert(m_analyzer != nullptr);
  assert(m_audioInspectorOpened);

  Suscan::Config cfg(m_audioCfgTemplate);
  cfg.set("audio.cutoff", m_cutOff);
  cfg.set("audio.volume", 1.f); // We handle this at UI level
  cfg.set("audio.sample-rate", SCAST(uint64_t, m_sampleRate));
  cfg.set("audio.demodulator", SCAST(uint64_t, m_demod + 1));
  cfg.set("audio.squelch", m_squelch);
  cfg.set("audio.squelch-level", m_squelchLevel);

  // Set audio inspector parameters
  m_analyzer->setInspectorConfig(m_audioInspHandle, cfg);
}

void
AudioProcessor::disconnectAnalyzer()
{
  m_mediator->setUIBusy(false);

  disconnect(m_analyzer, nullptr, this, nullptr);
}

void
AudioProcessor::connectAnalyzer()
{
  connect(
        m_analyzer,
        SIGNAL(inspector_message(const Suscan::InspectorMessage &)),
        this,
        SLOT(onInspectorMessage(const Suscan::InspectorMessage &)));

  connect(
        m_analyzer,
        SIGNAL(samples_message(const Suscan::SamplesMessage &)),
        this,
        SLOT(onInspectorSamples(const Suscan::SamplesMessage &)));
}

void
AudioProcessor::connectAudioFileSaver()
{
  connect(
        m_audioFileSaver,
        SIGNAL(stopped()),
        this,
        SLOT(stopRecording()));

  connect(
        m_audioFileSaver,
        SIGNAL(stopped()),
        this,
        SIGNAL(recStopped()));

  connect(
        m_audioFileSaver,
        SIGNAL(swamped()),
        this,
        SLOT(stopRecording()));

  connect(
        m_audioFileSaver,
        SIGNAL(swamped()),
        this,
        SIGNAL(recSwamped()));

  connect(
        m_audioFileSaver,
        SIGNAL(dataRate(qreal)),
        this,
        SIGNAL(recSaveRate(qreal)));

  connect(
        m_audioFileSaver,
        SIGNAL(commit()),
        this,
        SIGNAL(recCommit()));
}


void
AudioProcessor::setAnalyzer(Suscan::Analyzer *analyzer)
{
  if (m_analyzer != nullptr) {
    this->disconnectAnalyzer();
    this->closeAudio();
  }

  m_analyzer = analyzer;
  m_tracker->setAnalyzer(analyzer);

  // Was audio enabled? Open it back
  if (m_analyzer != nullptr) {
    this->connectAnalyzer();
    if (m_enabled)
      this->openAudio();
  }
}

void
AudioProcessor::setEnabled(bool enabled)
{
  if (m_enabled != enabled) {
    m_enabled = enabled;

    if (m_analyzer != nullptr) {
      if (enabled) {
        if (!m_opened && !m_opening)
          this->openAudio();
      } else {
        if (m_opened || m_opening)
          this->closeAudio();
      }
    }
  }
}

void
AudioProcessor::setSquelchEnabled(bool enabled)
{
  if (m_squelch != enabled) {
    m_squelch = enabled;

    if (m_audioInspectorOpened)
      this->setParams();
  }
}

void
AudioProcessor::setSquelchLevel(float level)
{
  if (!sufeq(m_squelchLevel, level, 1e-8f)) {
    m_squelchLevel = level;

    if (m_audioInspectorOpened)
      this->setParams();
  }
}

void
AudioProcessor::setVolume(float volume)
{
  if (!sufeq(m_volume, volume, 1e-1f)) {
    m_volume = volume;

    if (m_playBack != nullptr)
      m_playBack->setVolume(volume);
  }
}

void
AudioProcessor::setAudioCorrection(Suscan::Orbit const &orbit)
{
  m_orbit = orbit;

  if (m_correctionEnabled && m_audioInspectorOpened)
    m_analyzer->setInspectorDopplerCorrection(m_audioInspHandle, m_orbit);
}

void
AudioProcessor::setCorrectionEnabled(bool enabled)
{
  if (m_correctionEnabled != enabled) {
    m_correctionEnabled = enabled;

    if (m_audioInspectorOpened) {
      if (m_correctionEnabled)
        m_analyzer->setInspectorDopplerCorrection(m_audioInspHandle, m_orbit);
      else
        m_analyzer->disableDopplerCorrection(m_audioInspHandle);
    }
  }
}

void
AudioProcessor::setDemod(AudioDemod demod)
{
  if (m_demod != demod) {
    m_demod = demod;

    if (m_audioInspectorOpened) {
      this->setTrueLoFreq();
      this->setTrueBandwidth();
      this->setParams();
    }

    if (m_audioFileSaver != nullptr) {
      this->stopRecording();
      this->startRecording(m_savedPath);
    }
  }
}

void
AudioProcessor::setSampleRate(unsigned rate)
{
  if (m_requestedRate != rate) {
    m_requestedRate = rate;
    m_sampleRate = rate;

    // We temptatively set the corresponding parameter and wait for its
    // acknowledgment to call setSampleRate
    if (m_audioInspectorOpened) {
      m_settingRate = true;
      this->setParams();
      m_analyzer->setInspectorWatermark(
            m_audioInspHandle,
            PlaybackWorker::calcBufferSizeForRate(m_sampleRate) / 2);
    } else {
      m_playBack->setSampleRate(rate);
    }

    if (m_audioFileSaver != nullptr) {
      this->stopRecording();
      this->startRecording(m_savedPath);
    }
  }
}

void
AudioProcessor::setCutOff(float cutOff)
{
  if (!sufeq(m_cutOff, cutOff, 1e-8f)) {
    m_cutOff = cutOff;

    if (m_audioInspectorOpened)
      this->setParams();
  }
}

void
AudioProcessor::setTunerFreq(SUFREQ tuner)
{
  // No need to signal anything
  m_tuner = tuner;
}

void
AudioProcessor::setLoFreq(SUFREQ lo)
{
  // If changed, set frequency
  if (!sufeq(m_lo, lo, 1e-8f)) {
    m_lo = lo;

    if (m_audioInspectorOpened)
      this->setTrueLoFreq();
  }
}

void
AudioProcessor::setBandwidth(SUFREQ bw)
{
  // If changed, set frequency
  if (bw > m_maxAudioBw)
    bw = m_maxAudioBw;

  if (!sufeq(m_bw, bw, 1e-8f)) {
    SUFREQ trueLo = this->calcTrueLoFreq();
    SUFREQ newLo;

    m_bw = bw;

    newLo = this->calcTrueLoFreq();

    if (m_audioInspectorOpened) {
      this->setTrueBandwidth();

      if (!sufeq(trueLo, newLo, 1e-8f))
        this->setTrueLoFreq();
    }
  }
}

SUFREQ
AudioProcessor::getTrueChannelFreq() const
{
  return calcTrueLoFreq() + m_tuner;
}


SUFREQ
AudioProcessor::getChannelFreq() const
{
  return m_lo + m_tuner;
}

SUFREQ
AudioProcessor::getChannelBandwidth() const
{
  return m_bw;
}

bool
AudioProcessor::isAudioAvailable() const
{
  return m_playBack != nullptr;
}

QString
AudioProcessor::getAudioError() const
{
  return m_audioError;
}

bool
AudioProcessor::isRecording() const
{
  return m_audioFileSaver != nullptr;
}

bool
AudioProcessor::isOpened() const
{
  return m_opened;
}

size_t
AudioProcessor::getSaveSize() const
{
  return m_audioFileSaver == nullptr ? 0 : m_audioFileSaver->getSize();
}

bool
AudioProcessor::startRecording(QString path)
{
  bool opened = false;

  if (m_audioFileSaver == nullptr && m_opened) {
    AudioFileSaver::AudioFileParams params;
    m_savedPath       = path;

    params.sampRate   = m_sampleRate;
    params.savePath   = path.toStdString();
    params.frequency  = m_tuner + m_lo;
    params.modulation = m_demod;

    m_audioFileSaver = new AudioFileSaver(params, nullptr);
    this->connectAudioFileSaver();

    opened = true;
  }

  return opened;
}

void
AudioProcessor::stopRecording(void)
{
  if (m_audioFileSaver != nullptr) {
    m_audioFileSaver->deleteLater();
    m_audioFileSaver = nullptr;
  }
}

///////////////////////////// Analyzer slots //////////////////////////////////
void
AudioProcessor::onInspectorMessage(Suscan::InspectorMessage const &msg)
{
  if (m_audioInspectorOpened && msg.getInspectorId() == m_audioInspId) {
    // This refers to us!

    switch (msg.getKind()) {
      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SET_CONFIG:
        // Async step 4: analyzer acknowledged config, emit audio open
        if (!m_opened) {
          m_opened = true;
          emit audioOpened();
        }

        // Check if this is the acknowledgement of a "Setting rate" message
        if (m_settingRate) {
          const suscan_config_t *cfg = msg.getCConfig();
          const struct suscan_field_value *value;

          value = suscan_config_get_value(cfg, "audio.sample-rate");

          // Value is the same as requested? Go ahead
          if (value != nullptr) {
            if (m_sampleRate == value->as_int) {
              m_settingRate = false;
              m_playBack->setSampleRate(m_sampleRate);
            }
          } else {
            // This should never happen, but just in case the server is not
            // behaving as expected
            m_settingRate = false;
          }
        }
        break;


      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_KIND:
      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_OBJECT:
      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_HANDLE:
        if (!m_opened) {
          this->closeAudio();
          emit audioError("Unexpected error while opening audio channel");
        }

        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SET_TLE:
        emit setTLE(msg);
        break;

      case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_ORBIT_REPORT:
        emit orbitReport(msg);
        break;

      default:
        break;
    }
  }
}

void
AudioProcessor::onInspectorSamples(Suscan::SamplesMessage const &msg)
{
  // Feed samples, only if the sample rate is right
  if (m_opened && msg.getInspectorId() == m_audioInspId) {
    const SUCOMPLEX *samples = msg.getSamples();
    unsigned int count = msg.getCount();

    m_playBack->write(samples, count);

    if (m_audioFileSaver != nullptr)
      m_audioFileSaver->write(samples, count);
  }
}

////////////////////////// Request tracker slots ///////////////////////////////
void
AudioProcessor::onOpened(Suscan::AnalyzerRequest const &req)
{
  // Async step 2: update state
  m_opening = false;

  m_mediator->setUIBusy(false);

  if (m_analyzer != nullptr) {
    // We do a lazy initialization of the audio channel parameters. Instead of
    // creating our own audio configuration template in the constructor, we
    // wait for the channel to provide the current configuration and
    // duplicate that one.

    if (m_audioCfgTemplate != nullptr) {
      suscan_config_destroy(m_audioCfgTemplate);
      m_audioCfgTemplate = nullptr;
    }

    m_audioCfgTemplate = suscan_config_dup(req.config);

    if (m_audioCfgTemplate == nullptr) {
      m_analyzer->closeInspector(req.handle);
      emit audioError("Failed to duplicate audio configuration");
      return;
    }

    // Async step 3: set parameters
    m_audioInspHandle      = req.handle;
    m_audioInspId          = req.inspectorId;
    m_audioInspectorOpened = true;

    this->setTrueBandwidth();
    this->setTrueLoFreq();
    this->setParams();

    m_analyzer->setInspectorWatermark(
          m_audioInspHandle,
          PlaybackWorker::calcBufferSizeForRate(m_sampleRate) / 2);

    if (m_correctionEnabled)
      m_analyzer->setInspectorDopplerCorrection(m_audioInspHandle, m_orbit);
  }
}

void
AudioProcessor::onCancelled(Suscan::AnalyzerRequest const &)
{
  m_mediator->setUIBusy(false);

  m_opening = false;
  m_settingRate = false;
  m_playBack->stop();
}

void
AudioProcessor::onError(Suscan::AnalyzerRequest const &, std::string const &err)
{
  m_mediator->setUIBusy(false);

  m_opening = false;
  m_settingRate = false;
  m_playBack->stop();

  emit audioError(
        "Failed to open audio channel: " + QString::fromStdString(err));
}
