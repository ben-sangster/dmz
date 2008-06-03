#ifndef DMZ_INPUT_PLUGIN_TERM_IO_DOT_H
#define DMZ_INPUT_PLUGIN_TERM_IO_DOT_H

#include <dmzRuntimeLog.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeSync.h>

namespace dmz {

   class PluginTermExit :
         public Plugin,
         public Sync {

      public:
         //! \cond
         PluginTermExit (const PluginInfo &Info, Config &local);
         ~PluginTermExit ();

         // Plugin Interface
         virtual void discover_plugin (const Plugin *PluginPtr);
         virtual void start_plugin ();
         virtual void stop_plugin ();
         virtual void shutdown_plugin ();
         virtual void remove_plugin (const Plugin *PluginPtr);

         virtual void update_sync (const Float64 TimeDelta);

      protected:
         void _init (Config &local);

         Log _log;
         //! \endcond
      private:
         PluginTermExit ();
         PluginTermExit (const PluginTermExit &);
         PluginTermExit &operator= (const PluginTermExit &);

   };
};

#endif // DMZ_INPUT_PLUGIN_TERM_IO_DOT_H
