//
//    SigDiggerHelpers.h: Various helping functions
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#ifndef SIGDIGGERHELPERS_H
#define SIGDIGGERHELPERS_H

#include <vector>
#include <Suscan/Library.h>
#include <Palette.h>
#include <QStyledItemDelegate>
#include <QItemDelegate>
#include <list>
#include <time.h>
#include <sys/time.h>

class QComboBox;

namespace SigDigger {
  class MultitaskController;

  enum AudioDemod {
    AM,
    FM,
    USB,
    LSB
  };

  struct CaptureFileParams {
    bool haveFc = false;
    bool haveFs = false;
    bool haveDate = false;
    bool haveTime = false;
    bool isUTC = false;
    bool haveTm = false;
    bool isRaw = false;

    SUFREQ fc = 0;
    unsigned int fs = 0;

    struct tm tm;
    struct timeval tv = {0, 0};
  };

  class SigDiggerHelpers
  {
    std::vector<Palette> palettes;
    Palette *gqrxPalette = nullptr;

    std::list<std::string> tzs;
    std::list<const std::string *> tzStack;

    bool haveTZvar = false;
    std::string tzVar;

    static SigDiggerHelpers *currInstance;

    SigDiggerHelpers();

    Palette *getGqrxPalette(void);

  public:
    static unsigned int abiVersion(void);
    static QString version(void);
    static QString pkgversion(void);
    static void timerdup(struct timeval *);

    // Demod helpers
    static AudioDemod strToDemod(std::string const &str);
    static std::string demodToStr(AudioDemod);

    static void openSaveSamplesDialog(
        QWidget *root,
        const SUCOMPLEX *data,
        size_t len,
        qreal fs,
        int start,
        int end,
        Suscan::MultitaskController *);

    static SigDiggerHelpers *instance(void);
    int getPaletteIndex(std::string const &) const;
    const Palette *getPalette(std::string const &) const;
    const Palette *getPalette(int index) const;
    bool guessCaptureFileParams(CaptureFileParams &, QString const &);
    void populatePaletteCombo(QComboBox *combo);
    static void populateAntennaCombo(
        Suscan::Source::Config &profile,
        QComboBox *combo);
    void deserializePalettes(void);

    void pushLocalTZ(void);
    void pushUTCTZ(void);

    void pushTZ(const char *);
    bool popTZ(void);

    static QString expandGlobalProperties(QString const &);
  };
}

#endif // SIGDIGGERHELPERS_H
