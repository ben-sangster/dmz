#include <dmzTypesConsts.h>
#include <dmzTypesHashTableHandle.h>
#include <dmzTypesBase.h>

static inline dmz::Int32
local_hash (const dmz::UInt32 Value) {

   dmz::Int32 result (Value);
   if (result < 0) { result = -result; }
   return result;
}


static inline dmz::Int32
local_hash (const dmz::UInt64 Value) {

   dmz::UInt32 *ptr = (dmz::UInt32 *)&Value;
   dmz::Int32 result = (dmz::Int32)ptr[0] ^ ptr[1];
   if (result < 0) { result = -result; }
   return result;
}


#ifdef DMZ_TYPES_STRING_DOT_H
static inline dmz::Int32
local_hash (const dmz::String &Value) {

   dmz::Int32 result (0);
   dmz::Int32 len (0);
   const char *buf = Value.get_buffer (len);

   result = (5 * len);

   for (dmz::Int32 ix = 0; ix < len; ix++) {

      result = ((result << 1) + dmz::Int32 (buf[ix]));
   }

   if (result < 0) { result = -result; }

   return result;
}
#endif


#ifdef DMZ_TYPES_UUID_DOT_H
static inline dmz::Int32
local_hash (const dmz::UUID &Value) {

   dmz::Int32 result (0);

   dmz::UInt8 array[16];

   Value.to_array (array);

   for (dmz::Int32 ix = 0; ix < 16; ix++) {

      result = ((result << 2) + dmz::Int32 (array[ix]));
   }

   if (result < 0) { result = -result; }

   return result;
}
#endif


namespace {

   struct DataStruct {

      DataStruct *prev;
      DataStruct *next;
      dmz::Handle key;
      void *data;
      dmz::Boolean dirty;

      DataStruct () :
         prev (0),
         next (0),
         key (0),
         data (0),
         dirty (dmz::False) {;}

      void clear () {

         prev = 0;
         next = 0;
         key = 0;
         data = 0;
         dirty = dmz::False;
      }
   };
};


/*!

\class dmz::HashTableHandleIterator
\ingroup Types
\brief Use to iterate over hash table elements.
\sa dmz::HashTableHandleTemplate::get_first \n
dmz::HashTableHandleTemplate::get_next \n
dmz::HashTableHandleTemplate::get_prev \n
dmz::HashTableHandleTemplate::get_last \n

*/

struct dmz::HashTableHandleIterator::idata {

   Int32 index;
   Handle key;
   UInt32 growCount;

   idata () :
      index (-1),
      key (0),
      growCount (0) {;}

   idata (const idata &Id);
   void clear () {

      index = -1;
      key = 0;
      growCount = 0;
   }

   void reset ();
};

//! Constructor.
dmz::HashTableHandleIterator::HashTableHandleIterator () : data (*(new idata)) {;}


//! Destructor.
dmz::HashTableHandleIterator::~HashTableHandleIterator () { delete &data; }


//! Gets hash key of current item returned from iteration.
dmz::Handle
dmz::HashTableHandleIterator::get_hash_key () { return data.key; }


//! Resets iterator.
void
dmz::HashTableHandleIterator::reset () { data.index = -1; }


/*!

\class dmz::HashTableHandle
\ingroup Types
\brief Internal class used by dmz::HashTableHandleTemplate.
\note This Class should not be used directly. Use dmz::HashTableHandleTemplate instead.

*/

struct dmz::HashTableHandle::State {

   UInt32 growCount;
   Int32 size;
   Int32 count;
   Boolean autoGrow;
   DataStruct *table;
   DataStruct *head;
   DataStruct *tail;

   State () :
      growCount (0),
      size (0),
      count (0),
      autoGrow (True),
      table (0),
      head (0),
      tail (0) {;}

   ~State () { if (table) { delete []table; table = 0; } }

   Boolean find_index (const Handle &Key, Int32 &index) const {

      Boolean result (False);

      if (table) {

         index = local_hash (Key) % size;
         Boolean done (False);
         const Int32 StartIndex (index);

         while (!done) {

            if ((table[index].key == Key) && table[index].data) { result = done = True; }
            else if (!table[index].dirty) { done = True; }
            else { index++; if (index >= size) { index = 0; } }
            if (StartIndex == index) { done = True; }
         }

         if (!result) { index = -1; }
      }

      return result;
   }

   void clear () {

      if (table) {

         for (Int32 ix = 0; ix < size; ix++) {

            table[ix].clear ();
         }
      }

      count = 0;

      head = tail = 0;
   }

   Int32 find_ptr_index (DataStruct *data) const {

      return Int32 (data - table);
   }
};


//! Constructor.
dmz::HashTableHandle::HashTableHandle () : _state (*(new State ())) {;}


//! Constructor.
dmz::HashTableHandle::HashTableHandle (
      const Int32 Size,
      const UInt32 Mask) :
      _state (*(new State ())) {

   if (!(Mask & AutoGrow)) {

      _state.autoGrow = False;
      set_table_size (Size ? Size : 1);
   }
   else { grow (Size); }
}


//! Destructor.
dmz::HashTableHandle::~HashTableHandle () {

   _state.clear ();
   delete &_state;
}


//! Clears hash table.
void
dmz::HashTableHandle::clear () {

   if (_state.table) { _state.clear (); }
}


//! Gets size of hash table.
dmz::Int32
dmz::HashTableHandle::get_size () const { return _state.size; }


//! Gets count of elements stored in hash table.
dmz::Int32
dmz::HashTableHandle::get_count () const { return _state.count; }


//! Moves an element in the list.
dmz::Boolean
dmz::HashTableHandle::move (
      const Handle &Key,
      const Handle *TargetKey,
      const Boolean SingleStep,
      const Boolean Before) {

   Boolean result (False);

   Int32 index (-1);

   if (_state.find_index (Key, index)) {

      DataStruct &current = _state.table[index];
      DataStruct *target (0);

      if (TargetKey) {

         Int32 targetIndex (-1);

         if (_state.find_index (*TargetKey, targetIndex)) {

            target = &(_state.table[targetIndex]);
         }
      }
      else if (Before) { target = SingleStep ? current.prev : _state.head; }
      else { target = SingleStep ? current.next : _state.tail; }

      if (target && (target != &current)) {

         if (current.next) { current.next->prev = current.prev; }
         else { _state.tail = current.prev; }

         if (current.prev) { current.prev->next = current.next; }
         else { _state.head = current.next; }
 
         if (Before) {

            current.next = target;

            if (target->prev) {

               target->prev->next = &current;
               current.prev = target->prev;
            }
            else { _state.head = &current; current.prev = 0; }

            target->prev = &current;
         }
         else {

            current.prev = target;

            if (target->next) {

               target->next->prev = &current;
               current.next = target->next;
            }
            else { _state.tail = &current; current.next = 0; }

            target->next = &current;
         }

         result = True;
      }
   }

   return result;
}


//! Gets first/next/previous/last element in hash table.
void *
dmz::HashTableHandle::get_next (HashTableHandleIterator &it, const Boolean Prev) const {

   void *data (0);

   if (_state.table) {

      if (it.data.growCount != _state.growCount) {

         if ((it.data.index >= 0) && !_state.find_index (it.data.key, it.data.index)) {

            // if the table has grown and the key can not be resolved
            // the table must have grown and the element we were pointing at was
            //  removed! So we just start at the beginning again.
            it.data.index = -1;
         }

         it.data.growCount = _state.growCount;
      }

      // index is out of range, some one is probably using an iterator from a
      // different table, just reset to prevent from going out of bounds.
      if (it.data.index >= _state.size) { it.data.index = -1; }

      DataStruct *cur = (it.data.index >= 0) ?
         &(_state.table[it.data.index]) :
         (Prev ? _state.tail : _state.head);

      if (cur && (it.data.index >= 0)) {

         cur = Prev ? cur->prev : cur->next;

         while (cur && !cur->data) { cur = Prev ? cur->prev : cur->next; }
      }

      if (cur) {

         data = cur->data;
         it.data.index = _state.find_ptr_index (cur);
         it.data.key = cur->key;
      }
   }

   return data;
}


//! Looks up element in hash table.
void *
dmz::HashTableHandle::lookup (const Handle &Key) const {

   void *data (0);
   Int32 index (-1);

   if (_state.table && _state.find_index (Key, index)) {

      data = _state.table[index].data;
   }

   return data;
}


//! Stores element in hash table.
dmz::Boolean
dmz::HashTableHandle::store (const Handle &Key, void *data) {

   Boolean result (False);

   if (data) {

      if ((_state.count + 1) > _state.size) { grow (); }

      if (_state.size >= (_state.count + 1)) {

         Int32 index = local_hash (Key) % _state.size;
         const Int32 StartIndex (index);
         Boolean done (False);

         Int32 foundIndex (-1);

         while (!done) {

            if (!_state.table[index].data && (foundIndex < 0)) { foundIndex = index; }

            if (!_state.table[index].dirty) { done = True; }
            else {

               if ((_state.table[index].key == Key) && _state.table[index].data) {

                  done = True;
                  foundIndex = -1;
               }
               else {

                  index++;
                  if (index >= _state.size) { index = 0; }
                  if (index == StartIndex) { done = True; }
               }
            }
         }

         if (foundIndex >= 0) {

            result = True;
            _state.count++;

            _state.table[foundIndex].prev = 0;
            _state.table[foundIndex].next = 0;
            _state.table[foundIndex].key = Key;
            _state.table[foundIndex].data = data;
            _state.table[foundIndex].dirty = True;

            if (_state.tail) {

               _state.table[foundIndex].prev = _state.tail;
               _state.tail->next = &(_state.table[foundIndex]);
               _state.tail = &(_state.table[foundIndex]);
            }
            else {

               _state.head = _state.tail = &(_state.table[foundIndex]);
            }
         }
      }
   }

   return result;
}


//! Removes element from hash table.
void *
dmz::HashTableHandle::remove (const Handle &Key) {

   void *data (0);
   Int32 index (-1);

   if (_state.table && _state.find_index (Key, index)) {

      DataStruct &el = _state.table[index];
      data = el.data;
      el.data = 0;

      if (el.prev) { el.prev->next = el.next; }
      else { _state.head = el.next; }
      if (el.next) { el.next->prev = el.prev; }
      else { _state.tail = el.prev; }

      _state.count--;
   }

   return data;
}


//! Grows the hash table.
void
dmz::HashTableHandle::grow (const Int32 Size) {

   Int32 newSize (0);

   if (_state.autoGrow) {

      if (!Size) {

         if (!_state.size) { newSize = 1; }
         else { newSize = _state.size; newSize = newSize << 1; }
      }
      else {

         if (Size > _state.size) {

            newSize = _state.size;
            if (!newSize) { newSize = 1; }
            while ((newSize < Size) && (newSize <= 0x80000000)) {

               newSize = newSize << 1;
            }
         }
      }
   }

   if (newSize) {

      _state.growCount++;

      const Int32 OldSize (_state.size);
      const Int32 OldCount (_state.count);
      DataStruct *table (_state.table);

      _state.size = 0;
      _state.count = 0;
      _state.table = new DataStruct[newSize];

      if (table && _state.table) {

         _state.size = newSize;

         DataStruct *cur (_state.head);
         _state.tail = _state.head = 0;

         while (cur) {

            store (cur->key, cur->data);
            cur = cur->next;
         }

         delete []table; table = 0;
      }
      else if (_state.table) { _state.size = newSize; }
      else if (table) {

         _state.table = table;
         _state.size = OldSize;
         _state.count = OldCount;
      }
   }
}


//! Sets the hash table size.
void
dmz::HashTableHandle::set_table_size (const Int32 Size) {

   if (_state.table) { delete []_state.table; _state.table = 0; }
   _state.size = 0;
   _state.count = 0;
   _state.head = _state.tail = 0;

   if (Size) {

      _state.table = new DataStruct[Size];

      if (_state.table) { _state.size = Size; }
   }
}


//! Sets hash table attributes.
void
dmz::HashTableHandle::set_attributes (const UInt32 Attributes) {

   if (Attributes & AutoGrow) { _state.autoGrow = True; }
   else { _state.autoGrow = False; }
}


//! Gets hash table attributes.
dmz::UInt32
dmz::HashTableHandle::get_attributes () const {

   UInt32 result (0);

   if (_state.autoGrow) { result |= AutoGrow; }

   return result;
}

