#ifndef DMZ_RUNTIME_CONTEXT_MESSAGING_DOT_H
#define DMZ_RUNTIME_CONTEXT_MESSAGING_DOT_H

#include "dmzRuntimeConfigContext.h"
#include "dmzRuntimeContextThreadKey.h"
#include <dmzRuntimeData.h>
#include <dmzRuntimeHandleAllocator.h>
#include <dmzRuntimeMessaging.h>
#include <dmzSystemMutex.h>
#include <dmzSystemRefCount.h>
#include <dmzTypesHashTableStringTemplate.h>
#include <dmzTypesHashTableHandleTemplate.h>

namespace dmz {

   class RuntimeContext;
   class RuntimeContextMessaging;
   class RuntimeMessageContext;

   class RuntimeContextMessageContainer : public RefCountDeleteOnZero {

      public:
         RuntimeContextMessageContainer ();

         Message create_message (
            const String &Name,
            const String &ParentName,
            RuntimeContext *context,
            RuntimeContextMessaging *messagingContext);

         ConfigContextLock messageHandleLock; //!< Lock.
         HashTableHandleTemplate<Message> messageHandleTable; //!< Table.

         ConfigContextLock messageNameLock; //!< Lock.
         HashTableStringTemplate<Message> messageNameTable; //!< Table.

      protected:
         ~RuntimeContextMessageContainer ();

      private:
         RuntimeContextMessageContainer (const RuntimeContextMessageContainer &);
         RuntimeContextMessageContainer &operator= (
            const RuntimeContextMessageContainer &);
   };

   class RuntimeContextMessaging : public RefCountDeleteOnZero {

      public:
         struct MessageStruct {

            const Message Type;
            const Handle ObserverHandle;
            const Data *DataPtr;
            MessageStruct *next;

            MessageStruct (
                  const Message &TheType,
                  const Handle TheHandle,
                  const Data *TheData) :
                  Type (TheType),
                  ObserverHandle (TheHandle),
                  DataPtr (TheData ? new Data (*TheData) : 0),
                  next (0) {;}

            ~MessageStruct () {

               if (DataPtr) { delete DataPtr; DataPtr = 0; }
               if (next) { delete next; next = 0; }
            }
         };

         RuntimeContextMessaging (
            RuntimeContextThreadKey &theKey,
            RuntimeContextMessageContainer &container,
            RuntimeContext *context);

         UInt32 send (
            const Boolean IncrementCount,
            const Message &Type,
            const Handle ObserverHandle,
            const Data *InData,
            Data *outData) const;

         void update_time_slice ();
         Boolean add_observer (MessageObserver &obs);
         Boolean remove_observer (MessageObserver &obs);

         Mutex listLock; //!< Lock.
         MessageStruct *head; //!< Message type list head.
         MessageStruct *tail; //!< Message type list tail.

         UInt32 messageCount; //!< Message count.
         Message globalType; //!< Global message type.
         RuntimeContextThreadKey &key; //!< Thread key.
         ConfigContextLock obsHandleLock; //!< Lock.
         HashTableHandleTemplate<MessageObserver> obsHandleTable; //!< Table.

         ConfigContextLock obsNameLock; //!< Lock.
         HashTableStringTemplate<MessageObserver> obsNameTable; //!< Table.

      protected:
         ~RuntimeContextMessaging ();

      private:
         RuntimeContextMessaging (const RuntimeContextMessaging &);
         RuntimeContextMessaging &operator= (const RuntimeContextMessaging &);

   };
};

#endif // DMZ_RUNTIME_CONTEXT_MESSAGING_DOT_H
