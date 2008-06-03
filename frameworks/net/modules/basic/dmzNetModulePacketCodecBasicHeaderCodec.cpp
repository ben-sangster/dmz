#include "dmzNetModulePacketCodecBasic.h"
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigRead.h>
#include <dmzSystemMarshal.h>
#include <dmzSystemUnmarshal.h>

using namespace dmz;

namespace {

   template <class T> class constElement :
         public NetModulePacketCodecBasic::HeaderElement {

      public:
         constElement (const T &TheValue);
         ~constElement () {;}

         virtual Boolean read_element (Unmarshal &data, Handle &packetHandle);
         virtual Boolean write_element (const Handle PacketHandle, Marshal &data);

      protected:
         const T _Value;
   };

   template <class T> class handleElement :
         public NetModulePacketCodecBasic::HeaderElement {

      public:
         handleElement () {;}
         ~handleElement () {;}

         virtual Boolean read_element (Unmarshal &data, Handle &packetHandle);
         virtual Boolean write_element (const Handle PacketHandle, Marshal &data);

      protected:
   };

   template <class T> class sizeElement :
         public NetModulePacketCodecBasic::HeaderElement {

      public:
         sizeElement () {;}
         ~sizeElement () {;}

         virtual Boolean read_element (Unmarshal &data, Handle &packetHandle);
         virtual Boolean write_element (const Handle PacketHandle, Marshal &data);

      protected:
   };
};


template <class T> inline
constElement<T>::constElement (const T &TheValue) : _Value (TheValue) {;}


template <class T> Boolean
constElement<T>::read_element (Unmarshal &data, UInt32 &packetHandle) {

   T value (_Value);

   UnmarshalWrap wrap (data);

   wrap.get_next (value);

   return _Value == value;
}


template <class T> Boolean
constElement<T>::write_element (const UInt32 PacketHandle, Marshal &data) {

   MarshalWrap wrap (data);
   wrap.set_next (_Value);

   return True;
}


template <class T> Boolean
handleElement<T>::read_element (Unmarshal &data, UInt32 &packetHandle) {

   T value (0);

   UnmarshalWrap wrap (data);

   wrap.get_next (value);

   packetHandle = (UInt32)value;

   return True;
}


template <class T> Boolean
handleElement<T>::write_element (const UInt32 PacketHandle, Marshal &data) {

   T value (0);

   value = T (PacketHandle);

   MarshalWrap wrap (data);

   wrap.set_next (value);

   return True;
}


template <class T> Boolean
sizeElement<T>::read_element (Unmarshal &data, UInt32 &packetHandle) {

   T value (0);

   UnmarshalWrap wrap (data);

   wrap.get_next (value);
   // We don't really care about the size so just ignore for now.

   return True;
}


template <class T> Boolean
sizeElement<T>::write_element (const UInt32 PacketHandle, Marshal &data) {

   T value (0);

   value = T (data.get_length ());

   MarshalWrap wrap (data);

   wrap.set_next (value);

   return True;
}


void
dmz::NetModulePacketCodecBasic::_build_header_codec (Config &local) {

   Config header;

   if (local.lookup_all_config_merged ("header", header)) {

      Boolean error (False);

      Config element;

      ConfigIterator it;

      Boolean found (header.get_first_config (it, element));

      HeaderElement *current (0);

      while (found) {

         HeaderElement *next (0);

         const String TypeName (element.get_name ().to_lower ());

         const BaseTypeEnum BaseType (
            string_to_base_type_enum (config_to_string ("type", element)));

         if (TypeName == "const") {

            if (BaseType == BaseTypeInt8) {

               next = new constElement<Int8> (
                  Int8 (config_to_int32 ("value", element)));
            }
            else if (BaseType == BaseTypeInt16) {

               next = new constElement<Int16> (
                  Int16 (config_to_int32 ("value", element)));
            }
            else if (BaseType == BaseTypeInt32) {

               next = new constElement<Int32> (config_to_int32 ("value", element));
            }
            else if (BaseType == BaseTypeInt64) {

               next = new constElement<Int64> (config_to_int64 ("value", element));
            }
            else if (BaseType == BaseTypeUInt8) {

               next = new constElement<UInt8> (
                  UInt8 (config_to_uint32 ("value", element)));
            }
            else if (BaseType == BaseTypeUInt16) {

               next = new constElement<UInt16> (
                  UInt16 (config_to_uint32 ("value", element)));
            }
            else if (BaseType == BaseTypeUInt32) {

               next = new constElement<UInt32> (config_to_uint32 ("value", element));
            }
            else if (BaseType == BaseTypeUInt64) {

               next = new constElement<UInt64> (config_to_uint64 ("value", element));
            }
            else {

               _log.error << "Header codec element: " << element.get_name ()
                  << " is an unsupported type: " << base_type_enum_to_string (BaseType)
                  << endl;
            }
         }
         else if (TypeName == "handle") {

            if (BaseType == BaseTypeInt8) { next = new handleElement<Int8>; }
            else if (BaseType == BaseTypeInt16) { next = new handleElement<Int16>; }
            else if (BaseType == BaseTypeInt32) { next = new handleElement<Int32>; }
            else if (BaseType == BaseTypeInt64) { next = new handleElement<Int64>; }
            else if (BaseType == BaseTypeUInt8) { next = new handleElement<UInt8>; }
            else if (BaseType == BaseTypeUInt16) { next = new handleElement<UInt16>; }
            else if (BaseType == BaseTypeUInt32) { next = new handleElement<UInt32>; }
            else if (BaseType == BaseTypeUInt64) { next = new handleElement<UInt64>; }
            else {

               _log.error << "Header codec element: " << element.get_name ()
                  << " is an unsupported type: " << base_type_enum_to_string (BaseType)
                  << endl;
            }
         }
         else if (TypeName == "size") {

            if (BaseType == BaseTypeInt8) { next = new sizeElement<Int8>; }
            else if (BaseType == BaseTypeInt16) { next = new sizeElement<Int16>; }
            else if (BaseType == BaseTypeInt32) { next = new sizeElement<Int32>; }
            else if (BaseType == BaseTypeInt64) { next = new sizeElement<Int64>; }
            else if (BaseType == BaseTypeUInt8) { next = new sizeElement<UInt8>; }
            else if (BaseType == BaseTypeUInt16) { next = new sizeElement<UInt16>; }
            else if (BaseType == BaseTypeUInt32) { next = new sizeElement<UInt32>; }
            else if (BaseType == BaseTypeUInt64) { next = new sizeElement<UInt64>; }
            else {

               _log.error << "Header codec element: " << element.get_name ()
                  << " is an unsupported type: " << base_type_enum_to_string (BaseType)
                  << endl;
            }
         }
         else {

            _log.error << "Header codec element: " << element.get_name ()
               << " is an unknown type" << endl;
         }

         if (next) {

            if (!current) { _headerCodec = current = next; }
            else { current->next = next; current = next; }

            found = header.get_next_config (it, element);
         }
         else { found = False; error = True; }
      }

      if (error) { delete _headerCodec; _headerCodec = 0; }
   }
   else { _log.error << "No header codec defined" << endl; }
}

