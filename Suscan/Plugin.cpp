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

#include "Plugin.h"
#include <cstdio>
#include <dlfcn.h>
#include <Suscan/Logger.h>
#include <SuWidgetsHelpers.h>
#include <Version.h>

using namespace Suscan;

Plugin::Plugin()
{

}

Plugin::~Plugin()
{
  if (this->m_loaded) {
    fprintf(
          stderr,
          "Critical: cannot destroy plugin `%s': plugin is loaded\n",
          this->m_name.c_str());
    abort();
  }

  if (this->m_handle != nullptr)
    dlclose(this->m_handle);
}

void *
Plugin::resolveSym(std::string const &sym)
{
  void *addr;
  std::string full_name =
      STRINGIFY(SUSCAN_SYM_PFX) + sym;

  addr = dlsym(this->m_handle, full_name.c_str());

  if (addr == nullptr) {
    SU_ERROR(
          "%s: cannot resolve plugin symbol `%s'\n",
          this->m_path.c_str(),
          sym.c_str());
    return nullptr;
  }

  return addr;
}

Plugin::Plugin(
    std::string const &name,
    std::string const &path,
    std::string const &desc,
    void *handle)
{
  this->m_handle      = handle;
  this->m_name        = name;
  this->m_path        = path;
  this->m_description = desc;
}

bool
Plugin::load(void)
{
  PluginEntryFunc pluginEntry =
      reinterpret_cast<PluginEntryFunc>(this->resolveSym("plugin_load"));
  const uint32_t *pPluginVer =
      SCAST(const uint32_t *, this->resolveSym("plugin_ver"));
  const uint32_t *pAPIVer    =
      SCAST(const uint32_t *, this->resolveSym("api_ver"));

  if (pPluginVer == nullptr || pAPIVer == nullptr || pluginEntry == nullptr)
    return false;

  this->m_version = QString::asprintf(
        "%d.%d.%d",
        0xff & (*pPluginVer >> 16),
        0xff & (*pPluginVer >> 8),
        0xff & (*pPluginVer >> 0)).toStdString();

  if (*pAPIVer > SIGDIGGER_API_VERSION) {
    SU_ERROR(
          "%s: this plugin is not compatible with the current SigDigger version\n",
          this->m_path.c_str());
    return false;
  }

  this->m_loaded = (pluginEntry) (this);

  return true;
}
