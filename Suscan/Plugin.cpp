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

#define SIGDIGGER_PLUGIN_MANGLED_ENTRY "_Z11plugin_loadPN6Suscan6PluginE"

#if defined(_WIN32)
#  include <stdarg.h>
#  include <inttypes.h>
#  include <windows.h>

#  define DLFCN_ERR_BUFF_MAX 256
#  define RTLD_GLOBAL 0x100
#  define RTLD_LOCAL  0x000
#  define RTLD_LAZY   0x000
#  define RTLD_NOW    0x001

struct dlfcn_error_state {
  const char *last_error = nullptr;
  char errbuf[DLFCN_ERR_BUFF_MAX];
};

static dlfcn_error_state g_dlfcn_state;

static void
dl_set_last_error(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  vsnprintf(g_dlfcn_state.errbuf, DLFCN_ERR_BUFF_MAX, fmt, ap);
  g_dlfcn_state.last_error = g_dlfcn_state.errbuf;

  va_end(ap);
}

static void *
dlopen(const char *path, int)
{
  HINSTANCE hInst;

  hInst = LoadLibraryA(path);
  if (hInst == nullptr)
    dl_set_last_error("LoadLibrary: %s", GetLastError());
  return hInst;
}

static int
dlclose(void *handle)
{
  int ret = 0;

  if (!FreeLibrary(SCAST(HINSTANCE, handle))) {
    dl_set_last_error("FreeLibrary: %s", GetLastError());
    ret = -1;
  }

  return ret;
}

static void *
dlsym(void *handle, const char *name)
{
  FARPROC proc;
  void *asPtr;

  proc  = GetProcAddress(SCAST(HINSTANCE, handle), name);
  asPtr = SCAST(void *, proc);

  if (asPtr == nullptr)
    dl_set_last_error("GetProcAddress: %s", GetLastError());

  return asPtr;
}

const char *
dlerror()
{
  const char *error = g_dlfcn_state.last_error;

  g_dlfcn_state.last_error = nullptr;

  return error;
}

#else
#  include <dlfcn.h>
#endif // defined(_WIN32)

using namespace Suscan;

Plugin *Plugin::m_default = nullptr;

// Static members
Plugin *
Plugin::make(const char *path)
{
  void *handle = nullptr;
  Plugin *plugin = nullptr;
  const char *name;
  const char *desc;

  if ((handle = dlopen(path, RTLD_LAZY)) == nullptr) {
    SU_ERROR("Cannot open %s: %s\n", path, dlerror());
    return nullptr;
  }

  // Not an error, just not a plugin
  if ((name = SCAST(
         const char *,
         dlsym(handle, SUSCAN_SYM_NAME(plugin_name)))) == nullptr) {
    SU_WARNING("%s: not a plugin (no plugin name)\n", path);
    goto done;
  }

  if ((desc = SCAST(
         const char *,
         dlsym(handle, SUSCAN_SYM_NAME(plugin_desc)))) == nullptr) {
    SU_WARNING("%s: not a plugin (no plugin desc)\n", path);
    goto done;
  }

  plugin = new Plugin(name, path, desc, handle);

done:
  if (plugin == nullptr && handle != nullptr)
    dlclose(handle);

  return plugin;
}

Plugin *
Plugin::getDefaultPlugin()
{
  if (m_default == nullptr) {
    m_default = new Plugin(
          "default",
          QCoreApplication::applicationName().toStdString(),
          "Default plugin",
          nullptr);
    m_default->m_version = SIGDIGGER_VERSION_STRING;
  }

  return m_default;
}

// Implementation
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

  if (sym == SIGDIGGER_PLUGIN_MANGLED_ENTRY)
    full_name = sym;

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
  PluginEntryFunc pluginEntry = nullptr;

  if (this->m_loaded)
    return true;

  // Default plugin
  if (this->m_handle == nullptr) {
    pluginEntry = SigDigger::DefaultPluginEntry;
  } else {
    pluginEntry =
        reinterpret_cast<PluginEntryFunc>(this->resolveSym(SIGDIGGER_PLUGIN_MANGLED_ENTRY));
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
  }

  this->m_loaded = (pluginEntry) (this);

  // Load okay, perform registration
  if (this->m_loaded) {
    for (auto p : this->m_factorySet) {
      if (!p->registerGlobally()) {
        SU_ERROR(
              "%s: failed to register factory %s globally, unloading plugin\n",
              this->m_path.c_str(),
              p->name());
        this->unload();

        break;
      }
    }
  }

  return this->m_loaded;
}

bool
Plugin::unload(void)
{
  if (this->m_loaded) {
    if (!this->canBeUnloaded()) {
      SU_ERROR(
            "%s: plugin cannot be loaded (plugin in use)\n",
            this->m_path.c_str());
      return false;
    }

    auto p = this->m_factorySet.begin();

    // Erase and unregister all factories globally
    while (p != this->m_factorySet.end()) {
      FeatureFactory *ref = *p;
      if (!ref->unregisterGlobally()) {
        SU_ERROR(
              "%s: plugin cannot be loaded (failed to unregister global references to %s)\n",
              this->m_path.c_str(),
              ref->name());
        return false;
      }

      // Erase and get next. It is important to do this before deleting
      // the object.
      p = this->m_factorySet.erase(p);

      delete ref;
    }

    this->m_loaded = false;
  }

  return true;
}

bool
Plugin::registerFactory(FeatureFactory *factory)
{
  if (this->m_factorySet.contains(factory)) {
    SU_ERROR(
          "%s: attempting to register a %s factory twice\n",
          this->m_path.c_str(),
          factory->name());
    return false;
  }

  this->m_factorySet.insert(factory);

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

bool
Plugin::canBeUnloaded(void) const
{
  for (auto p : this->m_factorySet)
    if (!p->canBeRemoved())
      return false;

  return true;
}
