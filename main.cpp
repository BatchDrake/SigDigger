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

#include <QApplication>
#include <iostream>

#include "Loader.h"
using namespace SigDigger;

int
main(int argc, char *argv[])
{
  int ret;

  QApplication app(argc, argv);

  Application main_app;
  Loader loader(&main_app);

  loader.load();

  ret = app.exec();

  std::cout << "Saving config..." << std::endl;

  loader.saveConfig();

  return ret;
}
