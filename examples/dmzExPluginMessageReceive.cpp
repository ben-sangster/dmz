#include "dmzExPluginMessageReceive.h"
#include <dmzRuntimeConfigRead.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>


dmz::ExPluginMessageReceive::ExPluginMessageReceive (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      MessageObserver (Info),
      _log (Info),
      _type (),
      _binder (Info.get_context (), &_log),
      _value (),
      _time (0.0) {

   _init (local);
}


dmz::ExPluginMessageReceive::~ExPluginMessageReceive () {

}


// Plugin Interface
void
dmz::ExPluginMessageReceive::discover_plugin (const Plugin *PluginPtr) {

}


void
dmz::ExPluginMessageReceive::start_plugin () {

}


void
dmz::ExPluginMessageReceive::stop_plugin () {

}


void
dmz::ExPluginMessageReceive::shutdown_plugin () {

}


void
dmz::ExPluginMessageReceive::remove_plugin (const Plugin *PluginPtr) {

}


// Message Observer Interface
void
dmz::ExPluginMessageReceive::receive_message (
      const MessageType &Msg,
      const UInt32 MessageSendHandle,
      const UInt32 TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Msg == _type) {

      if (InData) {

         if (_binder.read_data (*InData)) {

            _log.out << "[receive_message] time: " << _time
                     << " value: " << _value << endl;
         }
      }
   }
}


void
dmz::ExPluginMessageReceive::_init (Config &local) {

   _binder.bind ("value", 0, _value);
   _binder.bind ("time", 0, _time);

   _type =
     config_to_message_type (
        "message.name",
        local,
        get_plugin_runtime_context (),
        &_log);

   subscribe_to_message (_type);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzExPluginMessageReceive (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ExPluginMessageReceive (Info, local);
}

};
