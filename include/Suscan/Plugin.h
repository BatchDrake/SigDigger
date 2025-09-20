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
#include <sigutils/util/util.h>
#include <sigutils/version.h>
#include <suscan/plugin.h>

#include <QSet>

#define IF_LOADED_FROM_SIGDIGGER(plugin, suscanPlugin)     \
  auto plugin = reinterpret_cast<Suscan::Plugin *>(        \
    suscan_plugin_get_service(suscanPlugin, "SigDigger")); \
                                                           \
  if (plugin == nullptr) {                                 \
    SU_ERROR(                                              \
      "%s: plugin is being loaded outside SigDigger\n",    \
      suscan_plugin_get_name(suscanPlugin));               \
    return false;                                          \
  } else


namespace Suscan {
  class Plugin;
  class FeatureFactory;

  typedef bool (*PluginEntryFunc) (Plugin *);

  class Plugin {
      static Plugin *m_default;
      suscan_plugin_t *m_suscanPlugin = nullptr;
      std::string m_name;
      std::string m_path;

      std::string m_version;
      std::string m_description;

      Plugin();
      Plugin(suscan_plugin_t *);


      QSet<FeatureFactory *> m_factorySet;

    public:
      static Plugin *getDefaultPlugin();
      static Plugin *make(suscan_plugin_t *);
      static inline Plugin *cast(suscan_plugin_t *plugin) {
        void *result = suscan_plugin_get_service(plugin, "SigDigger");
        return reinterpret_cast<Plugin *>(result);
      }

      static bool    registerSigDiggerPluginService();

      void postLoad();

      ~Plugin();

      // Internal
      bool registerFactory(FeatureFactory *);
      bool unregisterFactory(FeatureFactory *);
  };
}

#endif // PLUGIN_H
