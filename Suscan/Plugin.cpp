//
//    Plugin.cpp: Plugin support
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

#define SU_LOG_DOMAIN "sigdigger-plugin"

#include <Suscan/Plugin.h>
#include <cstdio>
#include <Suscan/Logger.h>
#include <SuWidgetsHelpers.h>
#include <Version.h>
#include <FeatureFactory.h>
#include <Default/Registration.h>
#include <QCoreApplication>

using namespace Suscan;

Plugin *Plugin::m_default = nullptr;

Plugin *
Plugin::getDefaultPlugin()
{
  if (m_default == nullptr)
    m_default = new Plugin();

  return m_default;
}

Plugin *
Plugin::make(suscan_plugin_t *plugin)
{
  try {
    return new Plugin(plugin);
  } catch (std::runtime_error const &e) {
    SU_ERROR("Cannot register services for this plugin: %s\n", e.what());
  }

  return nullptr;
}

// Implementation
Plugin::Plugin()
{
  m_name         = "default";
  m_path         = QCoreApplication::applicationName().toStdString();
  m_description  = "Default plugin";
  m_version      = SIGDIGGER_VERSION_STRING;
  m_suscanPlugin = nullptr;

  SigDigger::DefaultPluginEntry(this);

  postLoad();
}

Plugin::~Plugin()
{
  // Do nothing.
}

Plugin::Plugin(suscan_plugin_t *plugin)
{
  uint32_t version = suscan_plugin_get_version(plugin);

  m_suscanPlugin = plugin;
  m_name         = suscan_plugin_get_name(plugin);
  m_description  = suscan_plugin_get_description(plugin);
  m_path         = suscan_plugin_get_path(plugin);
  m_version      = QString::asprintf(
        "%d.%d.%d",
        0xff & (version >> 16),
        0xff & (version >> 8),
        0xff & (version >> 0)).toStdString();

  if (suscan_plugin_get_api_version(plugin) > SIGDIGGER_API_VERSION)
    throw std::runtime_error(m_path + ": plugin incompatible with this SigDigger version");
}

void
Plugin::postLoad()
{
  // Load okay, perform registration
  for (auto p : this->m_factorySet)
    if (!p->registerGlobally())
      SU_ERROR(
            "%s: failed to register factory %s globally\n",
            m_path.c_str(),
            p->name());
}

bool
Plugin::registerFactory(FeatureFactory *factory)
{
  if (m_factorySet.contains(factory)) {
    SU_ERROR(
          "%s: attempting to register a %s factory twice\n",
          m_path.c_str(),
          factory->name());
    return false;
  }

  m_factorySet.insert(factory);

  return true;
}

bool
Plugin::unregisterFactory(FeatureFactory *factory)
{
  // This is not an error: removal may have been triggered by erase
  if (this->m_factorySet.contains(factory))
    this->m_factorySet.remove(factory);

  return true;
}

extern "C" {
  static void *
  sigdigger_plugin_ctor(suscan_plugin_t *plugin)
  {
    return Plugin::make(plugin);
  }

  static SUBOOL
  sigdigger_plugin_post_load(void *userdata)
  {
    Plugin *plugin = reinterpret_cast<Plugin *>(userdata);
    plugin->postLoad();

    return SU_TRUE;
  }
}

bool
Plugin::registerSigDiggerPluginService()
{
  static suscan_plugin_service_desc desc;

  desc.name      = "SigDigger";
  desc.ctor      = sigdigger_plugin_ctor;
  desc.post_load = sigdigger_plugin_post_load;

  return suscan_plugin_register_service(&desc);
}
