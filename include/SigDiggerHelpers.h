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
#include <Suscan/Source.h>
#include <Palette.h>
#include <QStyledItemDelegate>
#include <QItemDelegate>
#include <list>
#include <time.h>
#include <sys/time.h>

class QComboBox;
class QFile;

namespace SigDigger {
  class MultitaskController;

  enum AudioDemod {
    AM,
    FM,
    USB,
    LSB
  };

  class SigDiggerHelpers
  {
    std::vector<Palette>           m_palettes;
    Palette                       *m_gqrxPalette = nullptr;
    std::list<std::string>         m_tzs;
    std::list<const std::string *> m_tzStack;

    bool                           m_haveTZvar = false;
    std::string                    m_tzVar;
    static SigDiggerHelpers       *m_currInstance;

    // Private methods

    SigDiggerHelpers();
    Palette *getGqrxPalette();

  public:
    static unsigned int abiVersion();
    static QString version();
    static QString pkgversion();
    static void timerdup(struct timeval *);

    // Demod helpers
    static bool tokenize(QString const &command, QStringList &out);
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

    static void openSaveCoherentSamplesDialog(
        QWidget *root,
        const SUCOMPLEX *channel1,
        const SUCOMPLEX *channel2,
        size_t len,
        qreal fs,
        int start,
        int end,
        Suscan::MultitaskController *);

    static SigDiggerHelpers *instance();
    int getPaletteIndex(std::string const &) const;
    const Palette *getPalette(std::string const &) const;
    const Palette *getPalette(int index) const;
    void populatePaletteCombo(QComboBox *combo);
    static void populateAntennaCombo(
        Suscan::Source::Config &profile,
        QComboBox *combo);
    void deserializePalettes();

    void pushLocalTZ();
    void pushUTCTZ();

    void pushTZ(const char *);
    bool popTZ();

    static QString expandGlobalProperties(QString const &);
  };
}

#endif // SIGDIGGERHELPERS_H
