//
//    FileSourcePage.h: description
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
#ifndef FILESOURCEPAGE_H
#define FILESOURCEPAGE_H

#include <QWidget>
#include <SourceConfigWidgetFactory.h>

namespace Ui {
  class FileSourcePage;
}

namespace Suscan {
  class Source;
}

namespace SigDigger {
  class SourceConfigWidgetFactory;

  class FileSourcePage : public SourceConfigWidget
  {
    Q_OBJECT

    Suscan::Source::Config *m_config = nullptr;
    void refreshUi();
    void connectAll();

    bool guessParamsFromFileName();

  public:
    explicit FileSourcePage(
        SourceConfigWidgetFactory *,
        QWidget *parent = nullptr);
    ~FileSourcePage();

    virtual void setConfigRef(Suscan::Source::Config &);
    virtual uint64_t getCapabilityMask() const;
    virtual void activateWidget() override;

  public slots:
    void onBrowseCaptureFile();
    void onLoopChanged();
    void onFormatChanged(int);

  private:
    Ui::FileSourcePage *ui;

  };
}

#endif // FILESOURCEPAGE_H
