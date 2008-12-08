#include <dmzTypesHashTableStringTemplate.h>
#include <dmzTypesString.h>
#include <dmzTypesBase.h>
#include <dmzTest.h>


#include <string.h>


using namespace dmz;

class DestructorCounter {

   public:
      DestructorCounter (Int32 &counter);
      DestructorCounter (const DestructorCounter &original);
      ~DestructorCounter ();

      Int32 &get_destruct_count () const;

   protected:
      Int32 &_destructCount;
};


DestructorCounter::DestructorCounter (Int32 &counter)
   : _destructCount (counter) { }



DestructorCounter::DestructorCounter (const DestructorCounter &original)
   : _destructCount (original.get_destruct_count ()) {
}

DestructorCounter::~DestructorCounter () {
   _destructCount++;
}

Int32 &DestructorCounter::get_destruct_count () const {
   return _destructCount;

}

int main (int argc, char *argv[]) {

   // Sample keys and values used throughout the tests
   String key1 ("key1");
   String key2 ("key2");
   String key3 ("key3");
   String key4 ("key4");
   String key5 ("key5");
   String value1 ("Value1");
   String value2 ("Value2");
   String value3 ("Value3");
   String value4 ("Value4");


   Test test ("dmzTypesHashTableStringTemplateTest", argc, argv);

   // Constructor Test - Default
   HashTableStringTemplate<String> constructorTable1;

   test.validate (
      "Default hash table String constructor creating empty hash table.",
      !constructorTable1.get_size () && !constructorTable1.get_count());

   // Constructor Test - With size and no autogrow mask
   HashTableStringTemplate<String> constructorTable2(3, 0, NULL);


   test.validate (
      "Hash Table with predefined size, mask, and lock",
      (constructorTable2.get_size () == 3) &&
      !constructorTable2.get_count ());

   // Storing values
   String value1CopyForStoreAndRetrieve (value1);
   String value2CopyForStoreAndRetrieve (value2);
   String value3CopyForStoreAndRetrieve (value3);

   test.validate (
      "Storing first value",
      constructorTable1.store (key1, &value1CopyForStoreAndRetrieve) &&
      (constructorTable1.get_count () == 1));

   test.validate (
      "Storing a second value",
      constructorTable1.store (key2, &value2CopyForStoreAndRetrieve) &&
      (constructorTable1.get_count () == 2));

  test.validate (
      "Storing a null value ",
      !constructorTable1.store (key5, NULL) &&
      (constructorTable1.get_count () == 2));

   // Retrieving values
   String *storedStr1 = constructorTable1.lookup (key1);
   test.validate (
      "Retrieving first value",
      storedStr1 &&
      ((*storedStr1) == value1) &&
      (constructorTable1.get_count () == 2));


   String *storedStr2 = constructorTable1.lookup (key2);
   test.validate (
      "Retrieving second value",
      storedStr2 &&
      ((*storedStr2) == value2) &&
      (constructorTable1.get_count () == 2));

   // Removing a value
   test.validate (
      "Removing values",
      constructorTable1.remove (key1) &&
      constructorTable1.remove (key2) &&
      !constructorTable1.lookup (key1) &&
      !constructorTable1.lookup (key2) &&
      !constructorTable1.get_count ());

   // Replacing a value
   constructorTable1.remove (key1);
   constructorTable1.store (key1, &value3CopyForStoreAndRetrieve);
   String *storedStr3 = constructorTable1.lookup (key1);
   test.validate (
      "Replacing a value",
       storedStr3 &&
       ((*storedStr3) == value3));



   // Testing autogrow disable mask
   String value1CopyForAutogrow (value1);
   String value2CopyForAutogrow (value2);
   String value3CopyForAutogrow (value3);
   String value4CopyForAutogrow (value4);

   test.validate (
      "Overflow prevention on non autogrowing table.",
      constructorTable2.store (key1, &value1CopyForAutogrow) &&
      constructorTable2.store (key2, &value2CopyForAutogrow) &&
      constructorTable2.store (key3, &value3CopyForAutogrow) &&
      !constructorTable2.store (key4, &value4CopyForAutogrow));


   // Copy constructor
   HashTableStringTemplate<String> tableEmpty;
   HashTableStringTemplate<String> tableEmptyCopy (tableEmpty);
   test.validate (
      "Using copy constructor with empty input hashtable",
      !tableEmptyCopy.get_size () &&
      !tableEmptyCopy.get_count ());

   String value1CopyForCopyCons (value1);
   String value2CopyForCopyCons (value2);
   String value3CopyForCopyCons (value3);
   String value4CopyForCopyCons (value4);

   HashTableStringTemplate<String> tableFull (3, 0, NULL);
   tableFull.store (key1, &value1CopyForCopyCons);
   tableFull.store (key2, &value2CopyForCopyCons);
   tableFull.store (key3, &value3CopyForCopyCons);

   HashTableStringTemplate<String> tableFullCopy (tableFull);



   test.validate (
      "Using copy constructor",
      (tableFullCopy.get_size () == tableFull.get_size()) &&
      (tableFullCopy.get_count () == tableFull.get_count ()) &&
      (tableFullCopy.lookup (key1) == tableFull.lookup (key1)) &&
      (tableFullCopy.lookup (key2) == tableFull.lookup (key2)) &&
      (tableFullCopy.lookup (key3) == tableFull.lookup (key3)));

   HashTableStringTemplate<String> tableFullCopy2;
   tableFullCopy2 = tableFull;
   test.validate (
      "Using assignment operator",
      (tableFullCopy2.get_size () == tableFull.get_size()) &&
      (tableFullCopy2.get_count () == tableFull.get_count()) &&
      (tableFullCopy2.lookup (key1) == tableFull.lookup (key1)) &&
      (tableFullCopy2.lookup (key2) == tableFull.lookup (key2)) &&
      (tableFullCopy2.lookup (key3) == tableFull.lookup (key3)));


   HashTableStringTemplate<String> tableFullCopy3;
   tableFullCopy3.copy (tableFull);
   test.validate (
      "Using copy function",
      (tableFullCopy3.get_size () == tableFull.get_size()) &&
      (tableFullCopy3.get_count () == tableFull.get_count()) &&
      (tableFullCopy3.lookup (key1) != tableFull.lookup (key1)) &&
      (tableFullCopy3.lookup (key2) != tableFull.lookup (key2)) &&
      (tableFullCopy3.lookup (key3) != tableFull.lookup (key3)));


   // Test clearing and emptying
   Int32 destructCounter1 = 0;
   Int32 destructCounter2 = 0;
   DestructorCounter destructData1 (destructCounter1);
   DestructorCounter destructData2 (destructCounter2);
   DestructorCounter *destructData1Heap = new DestructorCounter (destructCounter1);
   DestructorCounter *destructData2Heap = new DestructorCounter (destructCounter2);



   HashTableStringTemplate<DestructorCounter> clearTable;
   clearTable.store (key1, &destructData1);
   clearTable.store (key2, &destructData2);

   clearTable.clear ();
   test.validate (
      "Clearing of hash table",
      (clearTable.get_count() == 0) &&
      !clearTable.lookup (key1) &&
      !clearTable.lookup (key2) &&
      !destructCounter1 &&
      !destructCounter2);

   clearTable.clear ();
   test.validate (
      "Clearing of cleared hash table",
      (clearTable.get_count() == 0) &&
      !clearTable.lookup (key1) &&
      !clearTable.lookup (key2) &&
      !destructCounter1 &&
      !destructCounter2);

   HashTableStringTemplate<DestructorCounter> emptyingTable;
   emptyingTable.store (key1, destructData1Heap);
   emptyingTable.store (key2, destructData2Heap);
   emptyingTable.empty ();

   test.validate (
      "Emptying of hash table",
      (emptyingTable.get_count() == 0) &&
      !emptyingTable.lookup (key1) &&
      !emptyingTable.lookup (key2) &&
      (destructCounter1 == 1) &&
      (destructCounter2 == 1));

   emptyingTable.empty();
   test.validate (
      "Empyting of empty hash table",
      (emptyingTable.get_count() == 0) &&
      !emptyingTable.lookup (key1) &&
      !emptyingTable.lookup (key2) &&
      (destructCounter1 == 1) &&
      (destructCounter2 == 1));

   destructData1Heap = NULL;
   destructData2Heap = NULL;

   // Test iterator
   HashTableStringIterator it;
   HashTableStringTemplate<String> iteratorTable;

   String value1CopyForIterator (value1);
   String value2CopyForIterator (value2);
   String value3CopyForIterator (value3);

   iteratorTable.store (key1, &value1CopyForIterator);
   iteratorTable.store (key2, &value2CopyForIterator);
   iteratorTable.store (key3, &value3CopyForIterator);

   String *iteratorString = iteratorTable.get_first (it);
   String iteratorKey = it.get_hash_key ();

   test.validate (
      "Getting first value of iterator",
      (iteratorTable.get_count() == 3) &&
      iteratorString &&
      ((*iteratorString) == value1) &&
      iteratorKey &&
      (iteratorKey == key1));


   // Go back to before first (should have null return)
   iteratorString = iteratorTable.get_prev (it);
   iteratorKey = it.get_hash_key ();

   test.validate (
      "Getting value before first",
      !iteratorString);


   // Proceed to value 2
   iteratorString = iteratorTable.get_next (it);
   iteratorKey = it.get_hash_key ();

   test.validate (
      "Getting next value of iterator (1)",
      iteratorString &&
      ((*iteratorString) == value2) &&
      iteratorKey &&
      (iteratorKey == key2));


   // Go back to value 1
   iteratorString = iteratorTable.get_prev (it);
   iteratorKey = it.get_hash_key ();

   test.validate (
      "Getting previous value of iterator (1)",
      iteratorString &&
      ((*iteratorString) == value1) &&
      iteratorKey &&
      (iteratorKey == key1));

   // Proceed to value 3
   iteratorString = iteratorTable.get_next (it);
   iteratorString = iteratorTable.get_next (it);
   iteratorKey = it.get_hash_key ();

   test.validate (
      "Getting next value of iterator (2)",
      iteratorString &&
      ((*iteratorString) == value3) &&
      iteratorKey &&
      (iteratorKey == key3));

   // Go back to value 2
   iteratorString = iteratorTable.get_prev (it);
   iteratorKey = it.get_hash_key ();

   test.validate (
      "Getting previous value of iterator (2)",
      iteratorString &&
      ((*iteratorString) == value2) &&
      iteratorKey &&
      (iteratorKey == key2));

   // Go to value after last (should have null return)
   iteratorTable.get_next (it);
   iteratorString = iteratorTable.get_next (it);
   iteratorKey = it.get_hash_key ();

   test.validate (
      "Getting value after last",
      !iteratorString);

   // Get last value
   String *iteratorStringLast = iteratorTable.get_last (it);
   String iteratorKeyLast = it.get_hash_key ();

   // Compare last value to last value from iteration

   iteratorTable.get_first (it);
   iteratorTable.get_next (it);
   String *iteratorStringLast2 = iteratorTable.get_next (it);
   String iteratorKeyLast2 = it.get_hash_key ();

   test.validate (
      "Getting last value of iterator",
      iteratorStringLast &&
      ((*iteratorStringLast) == value3) &&
      iteratorKeyLast &&
      (iteratorKeyLast == key3));

   test.validate (
      "Comparing last iterator value to last value reached using get_next calls",
      ((*iteratorStringLast) == (*iteratorStringLast2)) &&
      (iteratorKeyLast == iteratorKeyLast2));

   HashTableStringTemplate<String> orderTable;
   const String E1 ("Element 1");
   const String E2 ("Element 2");
   const String E3 ("Element 3");
   const String E4 ("Element 4");
   const String E5 ("Element 5");

   orderTable.store (E1, new String (E1));
   orderTable.store (E2, new String (E2));
   orderTable.store (E3, new String (E3));
   orderTable.store (E4, new String (E4));
   orderTable.store (E5, new String (E5));

   String *v1 = orderTable.get_first (it);
   String *v2 = orderTable.get_next (it);
   String *v3 = orderTable.get_next (it);
   String *v4 = orderTable.get_next (it);
   String *v5 = orderTable.get_next (it);
   String *v6 = orderTable.get_next (it);

   String *p5 = orderTable.get_last (it);
   String *p4 = orderTable.get_prev (it);
   String *p3 = orderTable.get_prev (it);
   String *p2 = orderTable.get_prev (it);
   String *p1 = orderTable.get_prev (it);
   String *p0 = orderTable.get_prev (it);


   test.validate (
      "Strings returned in order stored",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_up (E3) returns True",
      orderTable.move_up (E3));

   // NOTE: 2 and 3 are swapped.
   v1 = orderTable.get_first (it);
   v3 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v4 = orderTable.get_next (it);
   v5 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p5 = orderTable.get_last (it);
   p4 = orderTable.get_prev (it);
   p2 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p1 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element 3 was moved up one place",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_down (E2) returns True",
      orderTable.move_down (E2));

   // NOTE: 2 and 4 are swapped.
   v1 = orderTable.get_first (it);
   v3 = orderTable.get_next (it);
   v4 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v5 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p5 = orderTable.get_last (it);
   p2 = orderTable.get_prev (it);
   p4 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p1 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element 2 was moved down one place",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_first (E4) returns True",
      orderTable.move_first (E4));

   // NOTE: 4 is moved to the front.
   v4 = orderTable.get_first (it);
   v1 = orderTable.get_next (it);
   v3 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v5 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p5 = orderTable.get_last (it);
   p2 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p1 = orderTable.get_prev (it);
   p4 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element 4 was moved to the front",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_last (E2) returns True",
      orderTable.move_last (E2));

   // NOTE: 2 is moved to the end
   v4 = orderTable.get_first (it);
   v1 = orderTable.get_next (it);
   v3 = orderTable.get_next (it);
   v5 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p2 = orderTable.get_last (it);
   p5 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p1 = orderTable.get_prev (it);
   p4 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element 2 was moved to the end",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_last (E4) returns True",
      orderTable.move_last (E4));

   // NOTE: 4 is moved to the end.
   v1 = orderTable.get_first (it);
   v3 = orderTable.get_next (it);
   v5 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v4 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p4 = orderTable.get_last (it);
   p2 = orderTable.get_prev (it);
   p5 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p1 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element 4 was moved from first to last",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_first (E4) returns True",
      orderTable.move_first (E4));

   // NOTE: 4 is moved to the front.
   v4 = orderTable.get_first (it);
   v1 = orderTable.get_next (it);
   v3 = orderTable.get_next (it);
   v5 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p2 = orderTable.get_last (it);
   p5 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p1 = orderTable.get_prev (it);
   p4 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element 4 was moved from last to first",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_first (E4) returns False",
      !orderTable.move_first (E4));

   // NOTE: 4 is moved to the front.
   v4 = orderTable.get_first (it);
   v1 = orderTable.get_next (it);
   v3 = orderTable.get_next (it);
   v5 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p2 = orderTable.get_last (it);
   p5 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p1 = orderTable.get_prev (it);
   p4 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element are in same order",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_after (E1, E5) returns True",
      orderTable.move_after (E1, E5));

   // NOTE: 1 is moved after 5.
   v4 = orderTable.get_first (it);
   v3 = orderTable.get_next (it);
   v5 = orderTable.get_next (it);
   v1 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p2 = orderTable.get_last (it);
   p1 = orderTable.get_prev (it);
   p5 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p4 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element 1 was after Element 5",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_before (E5, E3) returns True",
      orderTable.move_before (E5, E3));

   // NOTE: 1 is moved after 5.
   v4 = orderTable.get_first (it);
   v5 = orderTable.get_next (it);
   v3 = orderTable.get_next (it);
   v1 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p2 = orderTable.get_last (it);
   p1 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p5 = orderTable.get_prev (it);
   p4 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element 5 is before Element 3",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

   test.validate (
      "move_before (E4, E2) returns True",
      orderTable.move_before (E4, E2));

   // NOTE: 1 is moved after 5.
   v5 = orderTable.get_first (it);
   v3 = orderTable.get_next (it);
   v1 = orderTable.get_next (it);
   v4 = orderTable.get_next (it);
   v2 = orderTable.get_next (it);
   v6 = orderTable.get_next (it);

   p2 = orderTable.get_last (it);
   p4 = orderTable.get_prev (it);
   p1 = orderTable.get_prev (it);
   p3 = orderTable.get_prev (it);
   p5 = orderTable.get_prev (it);
   p0 = orderTable.get_prev (it);

   test.validate (
      "Element 4 is before Element 2",
      v1 && v2 && v3 && v4 && v5 && !v6 &&
      p1 && p2 && p3 && p4 && p5 && !p0 &&
      (E1 == *v1) && (E1 == *p1) &&
      (E2 == *v2) && (E2 == *p2) &&
      (E3 == *v3) && (E3 == *p3) &&
      (E4 == *v4) && (E4 == *p4) &&
      (E5 == *v5) && (E5 == *p5));

#if 0
test.log.error
<< *v4 << " "
<< *v5 << " "
<< *v3 << " "
<< *v1 << " "
<< *v2 << " "
<< (v6 ? "Extra Data!" : "No Extra Data")
<< endl;
#endif

   return test.result ();
}
