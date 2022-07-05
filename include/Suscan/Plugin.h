//
//    Plugin.h: Plugin support
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
#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>
#include <stdint.h>
#include <util/util.h>
#include <sigutils/version.h>

#include <QSet>

#define SUSCAN_SYM_PFX  SUSCAN_CPP_

#define SUSCAN_SYM(name)                       \
  JOIN(SUSCAN_SYM_PFX, name)

#define SUSCAN_SYM_NAME(name)                  \
  STRINGIFY(SUSCAN_SYM(name))

#define SUSCAN_DECLARE_SYM(type, name, val)    \
  extern "C" {                                 \
    extern type SUSCAN_SYM(name);              \
    type SUSCAN_SYM(name) = val;               \
  }

#define SUSCAN_PLUGIN(name, desc)              \
  SUSCAN_DECLARE_SYM(                          \
    const char *,                              \
    plugin_name,                               \
    name)                                      \
  SUSCAN_DECLARE_SYM(                          \
    const char *,                              \
    plugin_desc,                               \
    desc)

#define SUSCAN_PLUGIN_VERSION(x, y, z)         \
  SUSCAN_DECLARE_SYM(                          \
    uint32_t,                                  \
    plugin_ver,                                \
    SU_VER(x, y, z));

#define SUSCAN_PLUGIN_API_VERSION(x, y, z)     \
  SUSCAN_DECLARE_SYM(                          \
    uint32_t,                                  \
    api_ver,                                   \
    SU_VER(x, y, z));

namespace Suscan {
  class Plugin;
  class FeatureFactory;

  typedef bool (*PluginEntryFunc) (Plugin *);

  class Plugin {
      static Plugin *m_default;
      void *m_handle = nullptr;
      std::string m_name;
      std::string m_path;

      std::string m_version;
      std::string m_description;

      bool m_loaded = false;

      void *resolveSym(std::string const &sym);

      Plugin();
      Plugin(
          std::string const &name,
          std::string const &path,
          std::string const &desc,
          void *handle);


      QSet<FeatureFactory *> m_factorySet;

    public:
      static Plugin *make(const char *path);
      static Plugin *getDefaultPlugin();

      bool load(void);
      bool canBeUnloaded(void) const;
      bool unload(void);

      ~Plugin();

      // Internal
      bool registerFactory(FeatureFactory *);
      bool unregisterFactory(FeatureFactory *);

  };
}

#endif // PLUGIN_H
