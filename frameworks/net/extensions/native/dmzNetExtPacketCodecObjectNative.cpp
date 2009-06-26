#include "dmzNetExtPacketCodecObjectNative.h"
#include <dmzObjectConsts.h>
#include <dmzObjectModule.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimeObjectType.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeUUID.h>
#include <dmzSystemMarshal.h>
#include <dmzSystemUnmarshal.h>
#include <dmzTypesArrays.h>
#include <dmzTypesHandleContainer.h>
#include <dmzTypesHashTableHandleTemplate.h>
#include <dmzTypesHashTableUInt32Template.h>
#include <dmzTypesMask.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesUUID.h>
#include <dmzTypesVector.h>

/*!

\class dmz::NetExtPacketCodecObjectNative
\ingroup Net
\brief Encodes and decodes objects for the network.
\details
\code
<local-scope>
   <adapter
      type="Adapter Type"
      attribute="Attribute Name"
      lnv="True/False"
      lnv-name="Last Network Value Attribute Name"
   />
   ...
</local-scope>
\endcode
Possible types are:
link, superlink, counter, state, flag, position, orientation, velocity, acceleration,
scale, vector, scalar, time-stamp, write-stamp, and text. \n \n
The lnv attribute is boolean. If it is set to true, the last network value will be
stored. The lnv attribute name is automatically generated. It defaults to false.
If the lnv attribute is not set, the lnv attribute name may be explicitly set with
the lnv-name attribute. \n \n
A counter type adapter may also have the following boolean attributes:
counter, minimum, maximum, and rollover. These attribute determine whether each value
is encoded/decode in the packet. They default to true.
*/

//! \cond
dmz::NetExtPacketCodecObjectNative::NetExtPacketCodecObjectNative (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      NetExtPacketCodecObject (Info),
      _SysID (get_runtime_uuid (Info)),
      _log (Info),
      _time (Info),
      _defaultHandle (0),
      _lnvHandle (0),
      _objMod (0),
      _attrMod (0),
      _adapterList (0) {

   _init (local);
}


dmz::NetExtPacketCodecObjectNative::~NetExtPacketCodecObjectNative () {

   if (_adapterList) { delete _adapterList; _adapterList = 0; }
}


// Plugin Interface
void
dmz::NetExtPacketCodecObjectNative::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::NetExtPacketCodecObjectNative::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_objMod) { _objMod = ObjectModule::cast (PluginPtr); }
      if (!_attrMod) { _attrMod = NetModuleAttributeMap::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_objMod && (_objMod == ObjectModule::cast (PluginPtr))) { _objMod = 0; }

      if (_attrMod && (_attrMod == NetModuleAttributeMap::cast (PluginPtr))) {

         _attrMod = 0;
      }
   }

   ObjectAttributeAdapter *current (_adapterList);

   while (current) {

      current->discover_plugin (Mode, PluginPtr);
      current = current->next;
   }
}


// NetExtPacketCodecObject Interface
dmz::Boolean
dmz::NetExtPacketCodecObjectNative::decode (Unmarshal &data, Boolean &isLoopback) {

   Boolean result (False);

   UUID uuid;
   data.get_next_uuid (uuid);

   isLoopback = (uuid == _SysID);

   if (!isLoopback && _objMod) {

      UUID objectID;
      data.get_next_uuid (objectID);
      const Int32 TypeSize (Int32 (data.get_next_int8 ()));

      Handle objectHandle (_objMod->lookup_handle_from_uuid (objectID));

      if (!TypeSize) {

         if (objectHandle) { _objMod->destroy_object (objectHandle); }
      }
      else {

         ArrayUInt32 typeArray (TypeSize);

         for (Int32 ix = 0; ix < TypeSize; ix++) {

            typeArray.set (ix, UInt32 (data.get_next_uint8 ()));
         }

         Boolean activateObject (False);

         if (!objectHandle) {

            ObjectType type;
            _attrMod->to_internal_object_type (typeArray, type);

            objectHandle = _objMod->create_object (type, ObjectRemote);

            if (objectHandle) {

               _objMod->store_uuid (objectHandle, objectID);
               activateObject = True;
            }
         }

         if (objectHandle) {

            result = True;

            ObjectAttributeAdapter *current (_adapterList);

            while (current) {

               current->decode (objectHandle, data, *_objMod);
               current = current->next;
            }

            if (activateObject) { _objMod->activate_object (objectHandle); }

            _objMod->store_time_stamp (objectHandle, _lnvHandle, _time.get_frame_time ());
         }
      }
   }

   return result;
}


dmz::Boolean
dmz::NetExtPacketCodecObjectNative::encode_object (
      const Handle ObjectHandle,
      const NetObjectEncodeEnum EncodeMode,
      Marshal &data) {

   Boolean result (False);

   if (_attrMod && _objMod) {

      UUID objectID;

      if (_objMod->lookup_uuid (ObjectHandle, objectID)) {

         data.set_next_uuid (_SysID);
         data.set_next_uuid (objectID);

         if (EncodeMode == NetObjectDeactivate) {

            data.set_next_int8 (0);
            result = True;
         }
         else {

            const ObjectType Type (_objMod->lookup_object_type (ObjectHandle));
            ArrayUInt32 typeArray;

            if (_attrMod->to_net_object_type (Type, typeArray)) {

               const Int32 TypeSize (typeArray.get_size ());
               data.set_next_int8 (Int8 (TypeSize));

               for (Int32 ix = 0; ix < TypeSize; ix++) {

                  data.set_next_uint8 (UInt8 (typeArray.get (ix)));
               }

               ObjectAttributeAdapter *current (_adapterList);

               while (current) {

                  current->encode (ObjectHandle, *_objMod, data);
                  current = current->next;
               }

               result = True;
            }

            _objMod->store_time_stamp (
               ObjectHandle,
               _lnvHandle,
               _time.get_last_frame_time ());
         }
      }
   }

   return result;
}


void
dmz::NetExtPacketCodecObjectNative::_init (Config &local) {

   RuntimeContext *context (get_plugin_runtime_context ());
   Definitions defs (context, &_log);

   _defaultHandle = defs.create_named_handle (ObjectAttributeDefaultName);
   _lnvHandle = defs.create_named_handle (ObjectAttributeLastNetworkValueName);

   Config adapters;
   ObjectAttributeAdapter *current (0);

   if (local.lookup_all_config ("adapter", adapters)) {

      ConfigIterator it;
      Config data;

      while (adapters.get_next_config (it, data)) {

         ObjectAttributeAdapter *next (create_object_adapter (data, context, _log));

         if (next) {

            if (current) { current->next = next; current = next; }
            else { _adapterList = current = next; }
         }
      }
   }
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzNetExtPacketCodecObjectNative (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::NetExtPacketCodecObjectNative (Info, local);
}

};


namespace {

typedef dmz::NetExtPacketCodecObjectNative::ObjectAttributeAdapter Adapter;

static dmz::Handle
local_create_attribute_handle (dmz::Config &local, dmz::RuntimeContext *context) {

   return  dmz::Definitions (context).create_named_handle (
      config_to_string ("attribute", local, dmz::ObjectAttributeDefaultName));
}


static dmz::Handle
local_create_lnv_handle (dmz::Config &local, dmz::RuntimeContext *context) {

   dmz::Handle result (0);

   dmz::Definitions defs (context);

   const dmz::String Name (
      config_to_string ("attribute", local, dmz::ObjectAttributeDefaultName));

   const dmz::Boolean DefaultLNV (config_to_boolean ("lnv", local, dmz::False));

   if (DefaultLNV) {

      dmz::String lnvName (dmz::ObjectAttributeLastNetworkValueName);

      if (Name != dmz::ObjectAttributeDefaultName) {

         lnvName = dmz::create_last_network_value_name (Name);
      }

      result = defs.create_named_handle (lnvName);
   }
   else {

      result = defs.create_named_handle (config_to_string ("lnv-name", local));
   }

   return result;
}


class SubLink : public Adapter {

   public:
      SubLink (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
SubLink::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   const dmz::Int32 Size (data.get_next_int8 ());

   dmz::HandleContainer handles;
   objMod.lookup_sub_links (ObjectHandle, _AttributeHandle, handles);

   for (dmz::Int32 ix = 0; ix < Size; ix++) {

      dmz::UUID objectID;
      data.get_next_uuid (objectID);
      const dmz::Handle SubHandle (objMod.lookup_handle_from_uuid (objectID));

      if (SubHandle) {

         if (!handles.contains (SubHandle)) {

            objMod.link_objects (_AttributeHandle, ObjectHandle, SubHandle);
         }
         else { handles.remove_handle (SubHandle); }
      }
   }

   dmz::Handle removedObj (handles.get_first ());

   while (removedObj) {

      const dmz::Handle LinkHandle (
         objMod.lookup_link_handle (_AttributeHandle, ObjectHandle, removedObj));

      if (LinkHandle) { objMod.unlink_objects (LinkHandle); }

      removedObj = handles.get_next ();
   }
}


void
SubLink::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::HandleContainer handles;
   objMod.lookup_sub_links (ObjectHandle, _AttributeHandle, handles);

   dmz::Int32 Size (handles.get_count ());

   data.set_next_int8 (dmz::Int8 (Size));

   dmz::Handle subHandle (handles.get_first ());

   dmz::Int32 count (0);

   while (subHandle && (count < Size)) {

      dmz::UUID objectID;
      objMod.lookup_uuid (subHandle, objectID);
      data.set_next_uuid (objectID);
      subHandle = handles.get_next ();
      count++;
   }
}


class SuperLink : public Adapter {

   public:
      SuperLink (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
SuperLink::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   const dmz::Int32 Size (data.get_next_int8 ());

   dmz::HandleContainer handles;
   objMod.lookup_super_links (ObjectHandle, _AttributeHandle, handles);

   for (dmz::Int32 ix = 0; ix < Size; ix++) {

      dmz::UUID objectID;
      data.get_next_uuid (objectID);
      const dmz::Handle SuperHandle (objMod.lookup_handle_from_uuid (objectID));

      if (SuperHandle) {

         if (!handles.contains (SuperHandle)) {

            objMod.link_objects (_AttributeHandle, SuperHandle, ObjectHandle);
         }
         else { handles.remove_handle (SuperHandle); }
      }
   }

   dmz::Handle removedObj (handles.get_first ());

   while (removedObj) {

      const dmz::Handle LinkHandle (
         objMod.lookup_link_handle (_AttributeHandle, removedObj, ObjectHandle));

      if (LinkHandle) { objMod.unlink_objects (LinkHandle); }

      removedObj = handles.get_next ();
   }
}


void
SuperLink::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::HandleContainer handles;
   objMod.lookup_sub_links (ObjectHandle, _AttributeHandle, handles);

   dmz::Int32 Size (handles.get_count ());

   data.set_next_int8 (dmz::Int8 (Size));

   dmz::Handle subHandle (handles.get_first ());

   dmz::Int32 count (0);

   while (subHandle && (count < Size)) {

      dmz::UUID objectID;
      objMod.lookup_uuid (subHandle, objectID);
      data.set_next_uuid (objectID);
      subHandle = handles.get_next ();
      count++;
   }
}


class Counter : public Adapter {

   public:
      Counter (dmz::Config &local, dmz::RuntimeContext *context);

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);

   protected:
      dmz::Boolean _includeCounter;
      dmz::Boolean _includeMin;
      dmz::Boolean _includeMax;
      dmz::Boolean _includeRollover;
};


Counter::Counter (dmz::Config &local, dmz::RuntimeContext *context) :
      Adapter (local, context),
      _includeCounter (dmz::config_to_boolean ("counter", local, dmz::True)),
      _includeMin (dmz::config_to_boolean ("minimum", local, dmz::False)),
      _includeMax (dmz::config_to_boolean ("maximum", local, dmz::False)),
      _includeRollover (dmz::config_to_boolean ("rollover", local, dmz::False)) {;}

void
Counter::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   if (_includeMin) {

      const dmz::Int64 Value (data.get_next_int64 ());
      objMod.store_counter_minimum (ObjectHandle, _AttributeHandle, Value);
      if (_LNVHandle) { objMod.store_counter_minimum (ObjectHandle, _LNVHandle, Value); }
   }

   if (_includeMax) {

      const dmz::Int64 Value (data.get_next_int64 ());
      objMod.store_counter_maximum (ObjectHandle, _AttributeHandle, Value);
      if (_LNVHandle) { objMod.store_counter_maximum (ObjectHandle, _LNVHandle, Value); }
   }

   if (_includeCounter) {

      const dmz::Int64 Value (data.get_next_int64 ());
      objMod.store_counter (ObjectHandle, _AttributeHandle, Value);
      if (_LNVHandle) { objMod.store_counter (ObjectHandle, _LNVHandle, Value); }
   }
}


void
Counter::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Int64 value (0);

   if (objMod.lookup_counter_minimum (ObjectHandle, _AttributeHandle, value)) {

      if (_LNVHandle) { objMod.store_counter_minimum (ObjectHandle, _LNVHandle, value); }
   }

   if (_includeMin) { data.set_next_int64 (value); }

   value = 0;

   if (objMod.lookup_counter_maximum (ObjectHandle, _AttributeHandle, value)) {

      if (_LNVHandle) { objMod.store_counter_maximum (ObjectHandle, _LNVHandle, value); }
   }

   if (_includeMax) { data.set_next_int64 (value); }

   value = 0;

   if (objMod.lookup_counter (ObjectHandle, _AttributeHandle, value)) {

      if (_LNVHandle) { objMod.store_counter (ObjectHandle, _LNVHandle, value); }
   }

   if (_includeCounter) { data.set_next_int64 (value); }
}


class State : public Adapter {

   public:
      State (dmz::Config &local, dmz::RuntimeContext *context);

      virtual void discover_plugin (
         const dmz::PluginDiscoverEnum Mode,
         const dmz::Plugin *PluginPtr);

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);

   protected:
      const dmz::Handle _DefaultHandle;
      dmz::NetModuleAttributeMap *_attrMod;

};


State::State (dmz::Config &local, dmz::RuntimeContext *context) :
      Adapter (local, context),
      _DefaultHandle (dmz::Definitions (context).create_named_handle (
         dmz::ObjectAttributeDefaultName)),
      _attrMod (0) {;}


void
State::discover_plugin (
      const dmz::PluginDiscoverEnum Mode,
      const dmz::Plugin *PluginPtr) {

   if (Mode == dmz::PluginDiscoverAdd) {

      if (!_attrMod) { _attrMod = dmz::NetModuleAttributeMap::cast (PluginPtr); }
   }
   else if (Mode == dmz::PluginDiscoverRemove) {

      if (_attrMod && (_attrMod == dmz::NetModuleAttributeMap::cast (PluginPtr))) {

         _attrMod = 0;
      }
   }
}


void
State::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   const dmz::Int32 Size (data.get_next_int8 ());

   dmz::ArrayUInt32 stateArray (Size);

   for (dmz::Int32 ix = 0; ix < Size; ix++) {

      stateArray.set (ix, data.get_next_uint32 ());
   }

   if (_attrMod) {

      const dmz::ObjectType Type (objMod.lookup_object_type (ObjectHandle));
      dmz::Mask value;
      _attrMod->to_internal_object_mask (Type, stateArray, value);

      objMod.store_state (ObjectHandle, _AttributeHandle, value);

      if (_LNVHandle) { objMod.store_state (ObjectHandle, _LNVHandle, value); }
   }
}


void
State::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::ArrayUInt32 stateArray;

   if (_attrMod) {

      const dmz::ObjectType Type (objMod.lookup_object_type (ObjectHandle));
      dmz::Mask value;
      objMod.lookup_state (ObjectHandle, _AttributeHandle, value);

      _attrMod->to_net_object_mask (Type, value, stateArray);

      if (_LNVHandle) { objMod.store_state (ObjectHandle, _LNVHandle, value); }
   }

   const dmz::Int32 Size (stateArray.get_size ());

   data.set_next_int8 (dmz::Int8 (Size));

   for (dmz::Int32 ix = 0; ix < Size; ix++) {

      data.set_next_uint32 (stateArray.get (ix));
   }
}


class Flag : public Adapter {

   public:
      Flag (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
Flag::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   const dmz::Boolean Value (0 != data.get_next_int8 ());
   objMod.store_flag (ObjectHandle, _AttributeHandle, Value);

   if (_LNVHandle) { objMod.store_flag (ObjectHandle, _LNVHandle, Value); }
}


void
Flag::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   const dmz::Boolean Value (objMod.lookup_flag (ObjectHandle, _AttributeHandle));
   data.set_next_int8 (Value ? 1 : 0);

   if (_LNVHandle) { objMod.store_flag (ObjectHandle, _LNVHandle, Value); }
}


class TimeStamp : public Adapter {

   public:
      TimeStamp (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
TimeStamp::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   const dmz::Float64 Value (data.get_next_float64 ());
   objMod.store_time_stamp (ObjectHandle, _AttributeHandle, Value);

   if (_LNVHandle) { objMod.store_time_stamp (ObjectHandle, _LNVHandle, Value); }
}


void
TimeStamp::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Float64 value (0.0);
   objMod.lookup_time_stamp (ObjectHandle, _AttributeHandle, value);
   data.set_next_float64 (value);

   if (_LNVHandle) { objMod.store_time_stamp (ObjectHandle, _LNVHandle, value); }
}


class WriteStamp : public Adapter {

   public:
      WriteStamp (dmz::Config &local, dmz::RuntimeContext *context);

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);

   protected:
      const dmz::Boolean _Realtime;
      dmz::Time _time;
};


WriteStamp::WriteStamp (dmz::Config &local, dmz::RuntimeContext *context) :
      Adapter (local, context),
      _Realtime (dmz::config_to_boolean ("realtime", local, dmz::True)),
      _time (context) {;}


void
WriteStamp::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   const dmz::Float64 Value (
      _Realtime ? data.get_next_float64 () : _time.get_frame_time ());

   objMod.store_time_stamp (ObjectHandle, _AttributeHandle, Value);

   if (_LNVHandle) { objMod.store_time_stamp (ObjectHandle, _LNVHandle, Value); }
}


void
WriteStamp::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Float64 value (dmz::get_time ());
   data.set_next_float64 (value);

   objMod.store_time_stamp (ObjectHandle, _AttributeHandle, value);

   if (_LNVHandle) { objMod.store_time_stamp (ObjectHandle, _LNVHandle, value); }
}


class Position : public Adapter {

   public:
      Position (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
Position::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   dmz::Vector value;
   data.get_next_vector (value);
   objMod.store_position (ObjectHandle, _AttributeHandle, value);

   if (_LNVHandle) { objMod.store_position (ObjectHandle, _LNVHandle, value); }
}


void
Position::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Vector value;
   objMod.lookup_position (ObjectHandle, _AttributeHandle, value);
   data.set_next_vector (value);

   if (_LNVHandle) { objMod.store_position (ObjectHandle, _LNVHandle, value); }
}


class Orientation : public Adapter {

   public:
      Orientation (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
Orientation::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   dmz::Matrix value;
   data.get_next_matrix (value);
   objMod.store_orientation (ObjectHandle, _AttributeHandle, value);

   if (_LNVHandle) { objMod.store_orientation (ObjectHandle, _LNVHandle, value); }
}


void
Orientation::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Matrix value;
   objMod.lookup_orientation (ObjectHandle, _AttributeHandle, value);
   data.set_next_matrix (value);

   if (_LNVHandle) { objMod.store_orientation (ObjectHandle, _LNVHandle, value); }
}


class Velocity : public Adapter {

   public:
      Velocity (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
Velocity::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   dmz::Vector value;
   data.get_next_vector (value);
   objMod.store_velocity (ObjectHandle, _AttributeHandle, value);

   if (_LNVHandle) { objMod.store_velocity (ObjectHandle, _LNVHandle, value); }
}


void
Velocity::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Vector value;
   objMod.lookup_velocity (ObjectHandle, _AttributeHandle, value);
   data.set_next_vector (value);

   if (_LNVHandle) { objMod.store_velocity (ObjectHandle, _LNVHandle, value); }
}


class Acceleration : public Adapter {

   public:
      Acceleration (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
Acceleration::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   dmz::Vector value;
   data.get_next_vector (value);
   objMod.store_acceleration (ObjectHandle, _AttributeHandle, value);

   if (_LNVHandle) { objMod.store_acceleration (ObjectHandle, _LNVHandle, value); }
}


void
Acceleration::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Vector value;
   objMod.lookup_acceleration (ObjectHandle, _AttributeHandle, value);
   data.set_next_vector (value);

   if (_LNVHandle) { objMod.store_acceleration (ObjectHandle, _LNVHandle, value); }
}


class Scale : public Adapter {

   public:
      Scale (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
Scale::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   dmz::Vector value;
   data.get_next_vector (value);
   objMod.store_scale (ObjectHandle, _AttributeHandle, value);

   if (_LNVHandle) { objMod.store_scale (ObjectHandle, _LNVHandle, value); }
}


void
Scale::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Vector value;
   objMod.lookup_scale (ObjectHandle, _AttributeHandle, value);
   data.set_next_vector (value);

   if (_LNVHandle) { objMod.store_scale (ObjectHandle, _LNVHandle, value); }
}


class VectorAttr : public Adapter {

   public:
      VectorAttr (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
VectorAttr::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   dmz::Vector value;
   data.get_next_vector (value);
   objMod.store_vector (ObjectHandle, _AttributeHandle, value);

   if (_LNVHandle) { objMod.store_vector (ObjectHandle, _LNVHandle, value); }
}


void
VectorAttr::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Vector value;
   objMod.lookup_vector (ObjectHandle, _AttributeHandle, value);
   data.set_next_vector (value);

   if (_LNVHandle) { objMod.store_vector (ObjectHandle, _LNVHandle, value); }
}


class Scalar : public Adapter {

   public:
      Scalar (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
Scalar::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   const dmz::Float64 Value (data.get_next_float64 ());
   objMod.store_scalar (ObjectHandle, _AttributeHandle, Value);

   if (_LNVHandle) { objMod.store_scalar (ObjectHandle, _LNVHandle, Value); }
}


void
Scalar::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::Float64 value (0.0);
   objMod.lookup_scalar (ObjectHandle, _AttributeHandle, value);
   data.set_next_float64 (value);

   if (_LNVHandle) { objMod.store_scalar (ObjectHandle, _LNVHandle, value); }
}

struct ScalarStruct {

   const dmz::Handle AttrHandle;
   const dmz::Handle LNVAttrHandle;
   const dmz::UInt8 NetHandle;

   ScalarStruct (
         const dmz::Handle TheAttrHandle,
         const dmz::Handle TheLNVAttrHandle,
         const dmz::UInt8 TheNetHandle) :
         AttrHandle (TheAttrHandle),
         LNVAttrHandle (TheLNVAttrHandle),
         NetHandle (TheNetHandle) {;}
};


class ScalarArray : public Adapter {

   public:
      ScalarArray (dmz::Config &local, dmz::RuntimeContext *context);
      ~ScalarArray ();

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);

   protected:
      dmz::HashTableHandleTemplate<ScalarStruct> _attrTable;
      dmz::HashTableUInt32Template<ScalarStruct> _netTable;
};


ScalarArray::ScalarArray (dmz::Config &local, dmz::RuntimeContext *context) :
      Adapter (local, context) {

   dmz::Config attrList;

   if (local.lookup_all_config ("attribute", attrList)) {

      dmz::ConfigIterator it;
      dmz::Config attr;

      dmz::UInt8 count = 0;

      dmz::Definitions defs (context);

      while (attrList.get_next_config (it, attr)) {

         const dmz::String Name (config_to_string ("name", attr));

         if (Name) {

            dmz::Handle lnvHandle (0);

            const dmz::Boolean DefaultLNV (config_to_boolean ("lnv", attr, dmz::False));

            if (DefaultLNV) {

               dmz::String lnvName = dmz::create_last_network_value_name (Name);

               lnvHandle = defs.create_named_handle (lnvName);
            }
            else {

               lnvHandle =
                  defs.create_named_handle (config_to_string ("lnv-name", attr));
            }

            ScalarStruct *ss = new ScalarStruct (
               defs.create_named_handle (Name),
               lnvHandle,
               count);

            if (ss && _attrTable.store (ss->AttrHandle, ss)) {

               _netTable.store (ss->NetHandle, ss);
            }
            else if (ss) { delete ss; ss = 0; }

            count++;
         }
         else {

            dmz::Log log ("dmzNetExtPacketCodecObjectNative", context);
            log.error << "scalar-array attribute missing name" << dmz::endl;
         }
      }
   }
}


ScalarArray::~ScalarArray () {

   _netTable.clear ();
   _attrTable.empty ();
}


void
ScalarArray::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   const dmz::UInt8 Size = data.get_next_uint8 ();

   for (dmz::UInt8 ix = 0; ix < Size; ix++) {

      const dmz::UInt8 NetHandle = data.get_next_uint8 ();
      const dmz::Float64 Value = data.get_next_float64 ();

      ScalarStruct *ss = _netTable.lookup (NetHandle);

      if (ss) {

         objMod.store_scalar (ObjectHandle, ss->AttrHandle, Value);

         if (ss->LNVAttrHandle) {

            objMod.store_scalar (ObjectHandle, ss->LNVAttrHandle, Value);
         }
      }
   }
}


void
ScalarArray::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   const dmz::Int32 SizePlace = data.get_place ();
   data.set_next_uint8 (0);
   dmz::UInt8 count = 0;

   dmz::HashTableHandleIterator it;
   ScalarStruct *ss (0);

   while (_attrTable.get_next (it, ss)) {

      dmz::Float64 value (0.0);

      if (objMod.lookup_scalar (ObjectHandle, ss->AttrHandle, value)) {

         data.set_next_uint8 (ss->NetHandle);
         data.set_next_float64 (value);
         count++;

         if (ss->LNVAttrHandle) {

            objMod.store_scalar (ObjectHandle, ss->LNVAttrHandle, value);
         }
      }
   }

   const dmz::Int32 EndPlace = data.get_place ();

   data.set_place (SizePlace);
   data.set_next_uint8 (count);
   data.set_place (EndPlace);
}


class Text : public Adapter {

   public:
      Text (dmz::Config &local, dmz::RuntimeContext *context) :
            Adapter (local, context) {;}

      virtual void decode (
         const dmz::Handle ObjectHandle,
         dmz::Unmarshal &data,
         dmz::ObjectModule &objMod);

      virtual void encode (
         const dmz::Handle ObjectHandle,
         dmz::ObjectModule &objMod,
         dmz::Marshal &data);
};


void
Text::decode (
      const dmz::Handle ObjectHandle,
      dmz::Unmarshal &data,
      dmz::ObjectModule &objMod) {

   dmz::String value;
   data.get_next_string (value);
   objMod.store_text (ObjectHandle, _AttributeHandle, value);

   if (_LNVHandle) { objMod.store_text (ObjectHandle, _LNVHandle, value); }
}


void
Text::encode (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &objMod,
      dmz::Marshal &data) {

   dmz::String value;
   objMod.lookup_text (ObjectHandle, _AttributeHandle, value);
   data.set_next_string (value);

   if (_LNVHandle) { objMod.store_text (ObjectHandle, _LNVHandle, value); }
}


// TODO Still need a Data Adapter
};


dmz::NetExtPacketCodecObjectNative::ObjectAttributeAdapter::ObjectAttributeAdapter (
      Config &local,
      RuntimeContext *context) :
      next (0),
      _AttributeHandle (local_create_attribute_handle (local, context)),
      _LNVHandle (local_create_lnv_handle (local, context)) {;}


dmz::NetExtPacketCodecObjectNative::ObjectAttributeAdapter::~ObjectAttributeAdapter () {

   if (next) { delete next; next = 0; }
}


dmz::NetExtPacketCodecObjectNative::ObjectAttributeAdapter *
dmz::NetExtPacketCodecObjectNative::create_object_adapter (
      Config &local,
      RuntimeContext *context,
      Log &log) {

   Adapter *result (0);

   const String Type = config_to_string ("type", local).to_lower ();

   if (Type == "link") { result = new SubLink (local, context); }
   else if (Type == "superlink") { result = new SuperLink (local, context); }
   else if (Type == "counter") { result = new Counter (local, context); }
   else if (Type == "state") { result = new State (local, context); }
   else if (Type == "flag") { result = new Flag (local, context); }
   else if (Type == "position") { result = new Position (local, context); }
   else if (Type == "orientation") { result = new Orientation (local, context); }
   else if (Type == "velocity") { result = new Velocity (local, context); }
   else if (Type == "acceleration") { result = new Acceleration (local, context); }
   else if (Type == "scale") { result = new Scale (local, context); }
   else if (Type == "vector") { result = new VectorAttr (local, context); }
   else if (Type == "scalar") { result = new Scalar (local, context); }
   else if (Type == "scalar-array") { result = new ScalarArray (local, context); }
   else if (Type == "time-stamp") { result = new TimeStamp (local, context); }
   else if (Type == "write-stamp") { result = new WriteStamp (local, context); }
   else if (Type == "text") { result = new Text (local, context); }
   else {

      log.error << "Unknown object attribute adapter type: " << Type << endl;
   }

   return result;
}
//! \endcond

