#include <dmzRuntimeConfig.h>
#include <dmzTypesHashTableStringTemplate.h>
#include <dmzTypesStringUtil.h>
#include <dmzXMLInterpreterConfig.h>

namespace {

   struct dataStruct {

      dmz::Config data;
      dataStruct *next;

      dataStruct (const dmz::Config &Config) : data (Config), next (0) {;}
      ~dataStruct () { if (next) { delete next; next = 0; } }
   };

   static void local_set_attributes (
         dmz::Config &data,
         const dmz::HashTableStringTemplate<dmz::String> &Attr) {

      dmz::HashTableStringIterator it;

      dmz::String *ptr = Attr.get_first (it);

       while (ptr) {

          data.store_attribute (it.get_hash_key (), *ptr);
          ptr = Attr.get_next (it);
      }
   }
};

/*!

\class dmz::XMLInterpreterConfig
\ingroup Foundation
\brief Class converts XML to a config context tree.
\details The class is used by the XMLParser to process the parsed XML and
convert it into a config context tree.
\sa dmz::XMLParser \n dmz::Config \n  dmz::ConfigContext

*/


struct dmz::XMLInterpreterConfig::State {

   dataStruct *stack;
   String value;
   Boolean inCDATA;
   String error;
   String name;

   State (const Config &RootConfig) :
         stack (0),
         inCDATA (False) {

      stack = new dataStruct (RootConfig);
   }

   ~State () {

      if (stack) { delete stack; stack = 0; }
   }

   void flush_value () {

      if (stack) {

         if (inCDATA || !is_ascii_white_space (value)) {

            stack->data.append_value (value, inCDATA);
         }

      }

      value.flush ();
   }
};


/*!

\brief Constructor.
\param[in] RootConfig Config object containing the root of the config context tree.

*/
dmz::XMLInterpreterConfig::XMLInterpreterConfig (const Config &RootConfig) :
      _state (*(new State (RootConfig))) {;}


//! Destructor.
dmz::XMLInterpreterConfig::~XMLInterpreterConfig () { delete &_state; }


dmz::Boolean
dmz::XMLInterpreterConfig::interpret_start_element (
      const String &Name,
      const HashTableStringTemplate<String> &AttributeTable) {

   Boolean result (True);

   if (_state.stack) {

      _state.flush_value ();

      Config data (Name);

      _state.stack->data.add_config (data);

      dataStruct *next = new dataStruct (data);

      if (next) {

         next->next = _state.stack;
         _state.stack = next;
      }

      local_set_attributes (data, AttributeTable);
   }

   return result;
}


dmz::Boolean
dmz::XMLInterpreterConfig::interpret_end_element (const String &Name) {

   Boolean result (False);

   dataStruct *tmp = _state.stack;

   if (tmp) {

      if (tmp->data.get_name () == Name) {

         _state.flush_value ();
         _state.stack = _state.stack->next;
         tmp->next = 0;
         delete tmp; tmp = 0;

         result = True;
      }
      else {

         _state.error = "Internal error. Closing data tag name miss-match.";
      }
   }
   else {

      _state.error = "Internal error. Closing data tag miss-match.";
   }

   return result;
}


dmz::Boolean
dmz::XMLInterpreterConfig::interpret_character_data (const String &Data) {

   Boolean result (True);

   _state.value << Data;

   return result;
}


dmz::Boolean
dmz::XMLInterpreterConfig::interpret_start_cdata_section () {

   _state.flush_value ();
   _state.inCDATA = True;
   return True;
}


dmz::Boolean
dmz::XMLInterpreterConfig::interpret_end_cdata_section () {

   _state.flush_value ();
   _state.inCDATA = False;
   return True;
}


dmz::String
dmz::XMLInterpreterConfig::get_error () { return _state.error; }
