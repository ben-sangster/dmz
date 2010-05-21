#include "dmzNetModuleLocalDRBasic.h"
#include <dmzObjectConsts.h>
#include <dmzObjectModule.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimeObjectType.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeTime.h>
#include <dmzTypesMask.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesVector.h>

#include <math.h>

#include <qdb.h>
static dmz::qdb out;

//#include <typeinfo>

/*!
\class dmz::NetModuleLocalDRBasic
\ingroup Net
\brief Basic module for when the next packet for an object should be transmitted.
\details
\code
<dmz>
<runtime>
   <object-type name="Type Name">
      <net>
         <rule
            type="Rule Type"
            attribute="Attribute Name"
            lnv-name="Last Network Value Attribute Name"
            value="Delta Value"
         />
         ...
      </net>
   </object-type>
</runtime>
</dmz>
\endcode
Possible rule types and their default values:\n
position = 0.25m \n
velocity = 0.25m/s \n
zero-velocity = N/A \n
acceleration = 0.25m/s^2 \n
vector = 0.25 \n
orientation = 0.25 radians \n
scalar = 0.25 \n
counter = N/A \n
state = Empty Mask \n
heartbeat = 5.0sec \n
rate-limit = 1/15 of a second \n

*/

//! \cond
namespace {

   enum TestTypeEnum { TestPosition, TestVelocity, TestAcceleration, TestVector };

   class debugWrapperTest : public dmz::NetModuleLocalDRBasic::ObjectUpdate {

      public:
         debugWrapperTest (
            const dmz::String &Name,
            dmz::NetModuleLocalDRBasic::ObjectUpdate &test,
            dmz::Stream &stream);

         virtual ~debugWrapperTest ();

         virtual dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);

      protected:
         const dmz::String _Name;
         dmz::NetModuleLocalDRBasic::ObjectUpdate &_test;
         dmz::Stream &_stream;

   };
 
   class valueTest : public dmz::NetModuleLocalDRBasic::ObjectUpdate {

      public:
         valueTest (
            const dmz::Handle AttributeHandle,
            const dmz::Handle LNVHandle,
            const dmz::Float64 Diff);

      protected:
         const dmz::Handle _AttributeHandle;
         const dmz::Handle _LNVHandle;
         const dmz::Float64 _Diff;
   };

   class zeroVelocityTest : public valueTest {

      public:
         zeroVelocityTest (
            const dmz::Handle AttributeHandle,
            const dmz::Handle LNVHandle);

         dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);
   };

   class counterTest : public valueTest {

      public:
         counterTest (
            const dmz::Handle AttributeHandle,
            const dmz::Handle LNVHandle);

         dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);
   };

   class stateTest : public valueTest {

      public:
         stateTest (
            const dmz::Handle AttributeHandle,
            const dmz::Handle LNVHandle,
            const dmz::Mask &TheState);

         dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);

      protected:
         const dmz::Mask _StateMask;
   };

   class posSkewTest : public valueTest {

      public:
         posSkewTest (
            const dmz::Handle AttributeHandle,
            const dmz::Handle LNVHandle,
            const dmz::Time &TheTime,
            const dmz::Float64 Diff);

         dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);

      protected:
         const dmz::Time &_Time;
   };

   class vectorTest : public valueTest {

      public:
         vectorTest (
            const TestTypeEnum Type,
            const dmz::Handle AttributeHandle,
            const dmz::Handle LNVHandle,
            const dmz::Float64 Diff);

         dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);

      protected:
         const TestTypeEnum _Type;
   };

   class oriTest : public valueTest {

      public:
         oriTest (
            const dmz::Handle AttributeHandle,
            const dmz::Handle LNVHandle,
            const dmz::Float64 Diff);

         dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);
   };

   class scalarTest : public valueTest {

      public:
         scalarTest (
            const dmz::Handle AttributeHandle,
            const dmz::Handle LNVHandle,
            const dmz::Float64 Diff);

         dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);
   };

   class timeTest : public dmz::NetModuleLocalDRBasic::ObjectUpdate {

      public:
         timeTest (
            const dmz::Handle LNVHandle,
            const dmz::Time &TheTime,
            const dmz::Float64 Diff);

      protected:
         const dmz::Handle _LNVHandle;
         const dmz::Time &_Time;
         const dmz::Float64 _Diff;
         dmz::Boolean _init;
   };

   class heartbeatTest : public timeTest {

      public:
         heartbeatTest (
            const dmz::Handle LNVHandle,
            const dmz::Time &TheTime,
            const dmz::Float64 Diff);

         dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);
   };

   class limitRateTest : public timeTest {

      public:
         limitRateTest (
            const dmz::Handle LNVHandle,
            const dmz::Time &TheTime,
            const dmz::Float64 Diff);

         dmz::Boolean update_object (
            const dmz::Handle ObjectHandle,
            dmz::ObjectModule &module,
            dmz::Boolean &limitRate);
   };
};


debugWrapperTest::debugWrapperTest (
      const dmz::String &Name,
      dmz::NetModuleLocalDRBasic::ObjectUpdate &test,
      dmz::Stream &stream) :
      _Name (Name),
      _test (test),
      _stream (stream) {;}


debugWrapperTest::~debugWrapperTest () { delete &_test; }


dmz::Boolean
debugWrapperTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Boolean result = _test.update_object (ObjectHandle, module, limitRate);

   if (result) { _stream << _Name << " update: " << ObjectHandle << dmz::endl; }
//   if (limitRate) { _stream << _Name << " limit rate: " << ObjectHandle << dmz::endl; }

   return result;
}


valueTest::valueTest (
      const dmz::Handle AttributeHandle,
      const dmz::Handle LNVHandle,
      const dmz::Float64 Diff) :
      _AttributeHandle (AttributeHandle),
      _LNVHandle (LNVHandle),
      _Diff (Diff) {;}



zeroVelocityTest::zeroVelocityTest (
      const dmz::Handle AttributeHandle,
      const dmz::Handle LNVHandle) :
      valueTest (AttributeHandle, LNVHandle, 0.0) {;}


dmz::Boolean
zeroVelocityTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Boolean result (dmz::False);

   dmz::Vector previous, current;

   const dmz::Boolean FoundPrevious (
      module.lookup_velocity (ObjectHandle, _LNVHandle, previous));

   const dmz::Boolean FoundCurrent (
      module.lookup_velocity (ObjectHandle, _AttributeHandle, current));

   if (FoundPrevious && FoundCurrent) {

      const dmz::Float64 PMag (previous.magnitude ());
      const dmz::Float64 CMag (current.magnitude ());

      if (dmz::is_zero64 (PMag) && !dmz::is_zero64 (CMag)) { result = dmz::True; }
      else if (!dmz::is_zero64 (PMag) && dmz::is_zero64 (CMag)) { result = dmz::True; }
   }
   else if (FoundCurrent && !FoundPrevious) { result = dmz::True; }

   return result;
}


counterTest::counterTest (
      const dmz::Handle AttributeHandle,
      const dmz::Handle LNVHandle) :
      valueTest (AttributeHandle, LNVHandle, 0.0) {;}


dmz::Boolean
counterTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Boolean result (dmz::False);

   dmz::Int64 previous (0), current (0);

   if (module.lookup_counter (ObjectHandle, _LNVHandle, previous) &&
         module.lookup_counter (ObjectHandle, _AttributeHandle, current)) {

      if (previous != current) { result = dmz::True; }
   }

   return result;
}


stateTest::stateTest (
      const dmz::Handle AttributeHandle,
      const dmz::Handle LNVHandle,
      const dmz::Mask &TheState) :
      valueTest (AttributeHandle, LNVHandle, 0.0), _StateMask (TheState) {;}


dmz::Boolean
stateTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Boolean result (dmz::False);

   dmz::Mask previous, current;

   if (module.lookup_state (ObjectHandle, _LNVHandle, previous) &&
         module.lookup_state (ObjectHandle, _AttributeHandle, current)) {

      if (_StateMask) {

         previous &= _StateMask;
         current &= _StateMask;
      }

      if (previous != current) { result = dmz::True; }
   }

   return result;
}


posSkewTest::posSkewTest (
      const dmz::Handle AttributeHandle,
      const dmz::Handle LNVHandle,
      const dmz::Time &TheTime,
      const dmz::Float64 Diff) :
      valueTest (AttributeHandle, LNVHandle, Diff), _Time (TheTime) {;}


dmz::Boolean
posSkewTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Boolean result (dmz::False);

   dmz::Vector pos, lnvPos, lnvVel;
   dmz::Float64 lnvStamp (0.0);

   if (module.lookup_position (ObjectHandle, _AttributeHandle, pos) &&
         module.lookup_position (ObjectHandle, _LNVHandle, lnvPos) &&
         module.lookup_velocity (ObjectHandle, _LNVHandle, lnvVel) &&
         module.lookup_time_stamp (ObjectHandle, _LNVHandle, lnvStamp)) {

      const dmz::Float64 FrameTime (_Time.get_frame_time ());

      lnvPos += lnvVel * (FrameTime - lnvStamp);

      const dmz::Float64 CalcDiff ((pos - lnvPos).magnitude ());

      // out << CalcDiff << dmz::endl;
      if (CalcDiff > _Diff) { result = dmz::True; }
   }

   return result;
}


vectorTest::vectorTest (
      const TestTypeEnum Type,
      const dmz::Handle AttributeHandle,
      const dmz::Handle LNVHandle,
      const dmz::Float64 Diff) :
      valueTest (AttributeHandle, LNVHandle, Diff),
      _Type (Type) {;}


dmz::Boolean
vectorTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Boolean result (dmz::False);

   dmz::Vector current, previous;
   dmz::Boolean found (dmz::False);

   if (_Type == TestPosition) {

      found = module.lookup_position (ObjectHandle, _AttributeHandle, current) &&
         module.lookup_position (ObjectHandle, _LNVHandle, previous);
   }
   else if (_Type == TestVelocity) {

      found = module.lookup_velocity (ObjectHandle, _AttributeHandle, current) &&
         module.lookup_velocity (ObjectHandle, _LNVHandle, previous);
   }
   else if (_Type == TestAcceleration) {

      found = module.lookup_acceleration (ObjectHandle, _AttributeHandle, current) &&
         module.lookup_acceleration (ObjectHandle, _LNVHandle, previous);
   }
   else if (_Type == TestVector) {

      found = module.lookup_vector (ObjectHandle, _AttributeHandle, current) &&
         module.lookup_vector (ObjectHandle, _LNVHandle, previous);
   }

   if (found) {

      if (_Diff < ((current - previous).magnitude ())) { result = dmz::True; }
   }

   return result;
}


oriTest::oriTest (
      const dmz::Handle AttributeHandle,
      const dmz::Handle LNVHandle,
      const dmz::Float64 Diff) : valueTest (AttributeHandle, LNVHandle, Diff) {;}


dmz::Boolean
oriTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Boolean result (dmz::False);

   dmz::Matrix ori, lnvOri;

   if (module.lookup_orientation (ObjectHandle, _LNVHandle, lnvOri) &&
         module.lookup_orientation (ObjectHandle, _AttributeHandle, ori)) {

      dmz::Vector vec1 (0.0, 0.0, -1.0);
      dmz::Vector vec2 (0.0, 0.0, -1.0);

      ori.transform_vector (vec1);
      lnvOri.transform_vector (vec2);

      if (vec1.get_angle (vec2) > _Diff) { result = dmz::True; }
      else {

         dmz::Vector vec3 (0.0, 1.0, 0.0);
         dmz::Vector vec4 (0.0, 1.0, 0.0);

         ori.transform_vector (vec3);
         lnvOri.transform_vector (vec4);

         if (vec3.get_angle (vec4) > _Diff) { result = dmz::True; }
      }
   }

   return result;
}


scalarTest::scalarTest (
      const dmz::Handle AttributeHandle,
      const dmz::Handle LNVHandle,
      const dmz::Float64 Diff) : valueTest (AttributeHandle, LNVHandle, Diff) {;}


dmz::Boolean
scalarTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Boolean result (dmz::False);

   dmz::Float64 value (0.0), lnv (0.0);

   if (module.lookup_scalar (ObjectHandle, _LNVHandle, lnv) &&
         module.lookup_scalar (ObjectHandle, _AttributeHandle, value)) {

      if (fabs (value - lnv)  > _Diff) { result = dmz::True; }
      else {

        const dmz::Boolean IsZero1 (dmz::is_zero64 (lnv));
        const dmz::Boolean IsZero2 (dmz::is_zero64 (value));

        if (IsZero1 && !IsZero2) { result = dmz::True; }
        else if (!IsZero1 && IsZero2) { result = dmz::True; }
      }
   }


   return result;
}


timeTest::timeTest (
      const dmz::Handle LNVHandle,
      const dmz::Time &TheTime,
      const dmz::Float64 Diff) :
      _LNVHandle (LNVHandle),
      _Time (TheTime),
      _Diff (Diff),
      _init (dmz::False) {;}


heartbeatTest::heartbeatTest (
      const dmz::Handle LNVHandle,
      const dmz::Time &TheTime,
      const dmz::Float64 Diff) : timeTest (LNVHandle, TheTime, Diff) {;}


dmz::Boolean
heartbeatTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Boolean result (dmz::False);

      dmz::Float64 lnvStamp (0.0);

      if (module.lookup_time_stamp (ObjectHandle, _LNVHandle, lnvStamp)) {

         if (_Time.get_frame_time () >= (lnvStamp + _Diff)) { result = dmz::True; }
      }

   return result;
}


limitRateTest::limitRateTest (
      const dmz::Handle LNVHandle,
      const dmz::Time &TheTime,
      const dmz::Float64 Diff) : timeTest (LNVHandle, TheTime, Diff) {;}


dmz::Boolean
limitRateTest::update_object (
      const dmz::Handle ObjectHandle,
      dmz::ObjectModule &module,
      dmz::Boolean &limitRate) {

   dmz::Float64 lnvStamp (0.0);

   if (module.lookup_time_stamp (ObjectHandle, _LNVHandle, lnvStamp)) {

      if (_Time.get_frame_time () < (lnvStamp + _Diff)) { limitRate = dmz::True; }
   }

   return dmz::False;
}


dmz::NetModuleLocalDRBasic::NetModuleLocalDRBasic (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      NetModuleLocalDR (Info),
      _log (Info),
      _time (Info),
      _objMod (0),
      _debug (False),
      _defaultHandle (0) {

   _init (local);
}


dmz::NetModuleLocalDRBasic::~NetModuleLocalDRBasic () {

   if (_defaultTest) { delete _defaultTest; _defaultTest = 0; }

   _baseTable.empty ();
   _typeTable.clear ();
}


// Plugin Interface
void
dmz::NetModuleLocalDRBasic::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_objMod) { _objMod = ObjectModule::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_objMod && (_objMod == ObjectModule::cast (PluginPtr))) { _objMod = 0; }
   }
}


dmz::Boolean
dmz::NetModuleLocalDRBasic::update_object (const Handle ObjectHandle) {

   Boolean result (False);

   if (_objMod) {

      const ObjectType Type (_objMod->lookup_object_type (ObjectHandle));

      if (Type) {

         ObjectUpdate *test (_typeTable.lookup (Type.get_handle ()));

         if (!test) { test = _create_test_from_type (Type); }

         Boolean limitRate (False);

         while (test && !result && !limitRate) {

            result = test->update_object (ObjectHandle, *_objMod, limitRate);
            test = test->next;
         }
      }
   }

   return result;
}


void
dmz::NetModuleLocalDRBasic::_init (Config &local) {

   Definitions defs (get_plugin_runtime_context (), &_log);

   _debug = config_to_boolean ("debug-test.value", local, _debug);

   _defaultHandle = defs.create_named_handle (ObjectAttributeDefaultName);

   Config defaultList;

   _log.info << "Creating default network transmission rules." << endl;

   if (local.lookup_all_config_merged ("default", defaultList)) {

      _defaultTest = _create_update_list (defaultList);
   }
   else {

      _log.info << "Using default rules set." << endl;

      const Handle LNVHandle (defs.create_named_handle (
            ObjectAttributeLastNetworkValueName));

      const Handle AttributeHandle (defs.create_named_handle (
            ObjectAttributeDefaultName));

      ObjectUpdate *current = _defaultTest = new heartbeatTest (
         LNVHandle,
         _time,
         5.0);

      if (current) {

         const Mask EmptyState;

         current->next = new stateTest (
            AttributeHandle,
            LNVHandle,
            EmptyState);

         if (current->next) { current = current->next; }

         current->next = new zeroVelocityTest (
            AttributeHandle,
            LNVHandle);

         if (current->next) { current = current->next; }

         current->next = new limitRateTest (
            LNVHandle,
            _time,
            0.066666666667); // 1/15 of a second max update rate

         if (current->next) { current = current->next; }

         current->next = new oriTest (
            AttributeHandle,
            LNVHandle,
            Pi64 / 60.0); // 0.05235987756 == 3 degrees

         if (current->next) { current = current->next; }

         current->next = new scalarTest (
            defs.create_named_handle (ObjectAttributeScalarThrottleName),
            defs.create_named_handle (
               create_last_network_value_name (ObjectAttributeScalarThrottleName)),
            0.01);
      }
   }
}


dmz::NetModuleLocalDRBasic::ObjectUpdate *
dmz::NetModuleLocalDRBasic::_create_update_list (Config &listData) {

   Definitions defs (get_plugin_runtime_context (), &_log);

   ObjectUpdate *head (0), *current (0);

   ConfigIterator it;
   Config cd;

   while (listData.get_next_config (it, cd)) {

      ObjectUpdate *next (0);

      const String Type (config_to_string ("type", cd).to_lower ());

      const String AttributeName (
         config_to_string ("attribute", cd, ObjectAttributeDefaultName));

      const Handle AttributeHandle (defs.create_named_handle (AttributeName));

      const Handle LNVHandle (defs.create_named_handle (
         config_to_string (
            "lnv-name",
            cd,
            create_last_network_value_name (AttributeName))));

      if (Type == "position") {

         next = new vectorTest (
            TestPosition,
            AttributeHandle,
            LNVHandle,
            config_to_float64 ("value", cd, 0.25));
      }
      else if (Type == "zero-velocity") {

         next = new zeroVelocityTest (
            AttributeHandle,
            LNVHandle);
      }
      else if (Type == "velocity") {

         next = new vectorTest (
            TestVelocity,
            AttributeHandle,
            LNVHandle,
            config_to_float64 ("value", cd, 0.25));
      }
      else if (Type == "acceleration") {

         next = new vectorTest (
            TestAcceleration,
            AttributeHandle,
            LNVHandle,
            config_to_float64 ("value", cd, 0.25));
      }
      else if (Type == "vector") {

         next = new vectorTest (
            TestVector,
            AttributeHandle,
            LNVHandle,
            config_to_float64 ("value", cd, 0.25));
      }
      else if (Type == "orientation") {

         next = new oriTest (
            AttributeHandle,
            LNVHandle,
            config_to_float64 ("value", cd, 0.25));
      }
      else if (Type == "scalar") {

         next = new scalarTest (
            AttributeHandle,
            LNVHandle,
            config_to_float64 ("value", cd, 0.25));
      }
      else if (Type == "counter") {

         next = new counterTest (AttributeHandle, LNVHandle);
      }
      else if (Type == "state") {

         Definitions defs (get_plugin_runtime_context (), &_log);

         Mask state;

         defs.lookup_state (config_to_string ("value", cd), state);

         next = new stateTest (AttributeHandle, LNVHandle, state);
      }
      else if (Type == "skew") {

         next = new posSkewTest (
            AttributeHandle,
            LNVHandle,
            _time,
            config_to_float64 ("value", cd, 0.25));
      }
      else if (Type == "heartbeat") {

         next = new heartbeatTest (
            LNVHandle,
            _time,
            config_to_float64 ("value", cd, 5.0));
      }
      else if (Type == "rate-limit") {

         next = new limitRateTest (
            LNVHandle,
            _time,
            // 1/15 of a second max update rate is the default.
            config_to_float64 ("value", cd, 0.066666666667));
      }

      if (next) {

         if (_debug) { next = new debugWrapperTest (Type, *next, _log.error); }

         _log.info << "Adding rule: " << Type << endl;
         if (current) { current->next = next; current = next; }
         else { head = current = next; }
      }
   }

   return head;
}


dmz::NetModuleLocalDRBasic::ObjectUpdate *
dmz::NetModuleLocalDRBasic::_create_test_from_type (const ObjectType &Type) {

   ObjectUpdate *result (0);

   ObjectType current (Type);

   while (current && !result) {

      result = _typeTable.lookup (current.get_handle ());

      if (!result) {

         result = _baseTable.lookup (current.get_handle ());

         if (result) { _typeTable.store (Type.get_handle (), result); }
      }

      if (!result) {

         Config listData;

         if (current.get_config ().lookup_all_config (
               "net.rule",
               listData)) {

            _log.info << "Creating network transmission rules for: "
               << current.get_name () << endl;

            result = _create_update_list (listData);

            if (result) {

               if (_baseTable.store (current.get_handle (), result)) {

                  _typeTable.store (Type.get_handle (), result);
               }
            }
         }
         else { current.become_parent (); }
      }
   }

   if (!result) {

      result = _defaultTest;

      if (result) { _typeTable.store (Type.get_handle (), result); }
   }

   return result;
}
//! \endcond


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzNetModuleLocalDRBasic (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::NetModuleLocalDRBasic (Info, local);
}

};
