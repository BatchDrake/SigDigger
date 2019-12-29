//
//    ConfigDialog.h: Configuration dialog window
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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <ui_Config.h>
#include <Suscan/AnalyzerParams.h>
#include <Suscan/Source.h>
#include <ColorConfig.h>
#include <SaveProfileDialog.h>

namespace SigDigger {
  class ConfigDialog : public QDialog
  {
    Q_OBJECT

  private:
    Suscan::Source::Config profile;
    Suscan::AnalyzerParams analyzerParams;
    ColorConfig colors;

    bool accepted;
    bool refreshing = false;

    // UI elements
    Ui_Config *ui = nullptr;
    SaveProfileDialog saveProfileDialog;

    void connectAll(void);
    void populateCombos(void);
    void refreshAntennas(void);
    void refreshColorUi(void);
    void refreshAnalyzerParamsUi(void);
    void refreshProfileUi(void);
    void refreshUi(void);
    void refreshUiState(void);
    void updateBwStep(void);
    void loadProfile(Suscan::Source::Config &config);
    void saveAnalyzerParams(void);
    void saveColors(void);

    static QString getBaseName(const QString &string);

  public:
    void setProfile(const Suscan::Source::Config &profile);
    void setAnalyzerParams(const Suscan::AnalyzerParams &params);
    void setColors(const ColorConfig &config);
    void setGain(std::string const &name, float value);
    void setFrequency(qint64 freq);
    void notifySingletonChanges(void);

    float getGain(std::string const &name);
    Suscan::Source::Config getProfile(void);
    ColorConfig getColors(void);
    Suscan::AnalyzerParams getAnalyzerParams(void);

    bool run(void);
    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog();

    static void
    populateAntennaCombo(
        Suscan::Source::Config &profile,
        QComboBox *combo)
    {
      int index = 0;
      combo->clear();

      for (auto i = profile.getDevice().getFirstAntenna();
           i != profile.getDevice().getLastAntenna();
           ++i) {
        combo->addItem(QString::fromStdString(*i));

        if (profile.getAntenna() == *i)
          index = static_cast<int>(
                i - profile.getDevice().getFirstAntenna());
      }

      combo->setCurrentIndex(index);
    }

  public slots:
    void onLoadProfileClicked(void);
    void onToggleSourceType(bool);
    void onDeviceChanged(int);
    void onFormatChanged(int);
    void onCheckButtonsToggled(bool);
    void onLineEditsChanged(const QString &);
    void onBandwidthChanged(double);
    void onBrowseCaptureFile(void);
    void onAccepted(void);
    void onSaveProfile(void);
  };
};


#endif // CONFIGDIALOG_H
