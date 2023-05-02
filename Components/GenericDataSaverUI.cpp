//
//    GenericDataSaverUI.cpp: Base class for data saver widgets
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

#include <GenericDataSaverUI.h>
#include <sigutils/util/compat-statvfs.h>
#include <cmath>

using namespace SigDigger;

GenericDataSaverUI::GenericDataSaverUI(QWidget *parent) : PersistentWidget(parent)
{

}

GenericDataSaverUI::~GenericDataSaverUI(void)
{

}

void
GenericDataSaverUI::refreshDiskUsage(void)
{
  std::string path = this->getRecordSavePath().c_str();
  struct statvfs svfs;

  if (statvfs(path.c_str(), &svfs) != -1)
    this->setDiskUsage(
          1. - static_cast<qreal>(svfs.f_bavail) /
          static_cast<qreal>(svfs.f_blocks));
  else
    this->setDiskUsage(std::nan(""));
}
