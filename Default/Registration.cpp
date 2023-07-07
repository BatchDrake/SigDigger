//
//    Registration.cpp: Default plugin registration
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

#include "Registration.h"
#include "Audio/AudioWidgetFactory.h"
#include "Source/SourceWidgetFactory.h"
#include "Inspection/InspToolWidgetFactory.h"
#include "FFT/FFTWidgetFactory.h"
#include "DefaultTab/DefaultTabWidgetFactory.h"
#include "GenericInspector/GenericInspectorFactory.h"
#include "RMSInspector/RMSInspectorFactory.h"
#include "SourceConfig/FileSourcePageFactory.h"
#include "SourceConfig/SoapySDRSourcePageFactory.h"
#include "SourceConfig/ToneGenSourcePageFactory.h"

#include <Suscan/Library.h>

using namespace SigDigger;

bool
SigDigger::DefaultPluginEntry(Suscan::Plugin *plugin)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  // Please note: it is not strictly necessary to call registerToolWidgetFactory
  // to register these tool factories. registerGlobally will traverse all
  // the created factories and register them accordingly. Calling register here
  // ensures that these factories are registered in order prior to any bulk
  // registration triggered by Suscan::Plugin. This is the order in which
  // they will show up in the GUI

  sus->registerToolWidgetFactory(new AudioWidgetFactory(plugin));
  sus->registerToolWidgetFactory(new SourceWidgetFactory(plugin));
  sus->registerToolWidgetFactory(new InspToolWidgetFactory(plugin));
  sus->registerToolWidgetFactory(new FFTWidgetFactory(plugin));

  sus->registerTabWidgetFactory(new DefaultTabWidgetFactory(plugin));

  sus->registerInspectionWidgetFactory(new GenericInspectorFactory(plugin));
  sus->registerInspectionWidgetFactory(new RMSInspectorFactory(plugin));

  sus->registerSourceConfigWidgetFactory(new FileSourcePageFactory(plugin));
  sus->registerSourceConfigWidgetFactory(new SoapySDRSourcePageFactory(plugin));
  sus->registerSourceConfigWidgetFactory(new ToneGenSourcePageFactory(plugin));

  return true;
}
