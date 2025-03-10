//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cow_BtreeCompare_h
#define cow_BtreeCompare_h

#include <string.h>
// Begin local includes
#include <omi_types.h>
#include <cow_String.h>
#include <cow_ListString.h>
#include <atr_String.h>
#include <atr_NatOrder.h>


inline int
BtreeCompare(const cow_String &key1, const cow_String &key2)
{
    if (atr_getNaturalOrder()) 
        return strnatcmp(key2.CStr(),key1.CStr());
    else
        return strcmp(key2.CStr(),key1.CStr());
}


inline int
BtreeCompare(const atr_String &key1, const atr_String &key2)
{
    return strcmp(key2.CStr(), key1.CStr());
}


inline int
BtreeCompare(const cow_ListString &key1, const cow_ListString &key2)
{
    int s1 = key1.Length(), s2 = key2.Length(), i, cmp;
    for(i=0; i<s1 && i<s2; i++) {
        cmp = BtreeCompare(key1.ConstGet(i), key2.ConstGet(i));
        if(cmp)
            return cmp;
    }
    return s1 - s2;
}

//
// These comparisons for primitive types should be unnecessary because we
// can instead use the default implementation that uses operators.
// But it doesn't hurt to keep them around just in case...
//

inline int
BtreeCompare(const int& key1, const int& key2)
{
     return key2 - key1;
}

inline int
BtreeCompare(const int64_t& key1, const int64_t& key2)
{
     if(key1 < key2)
        return 1;
     if(key1 > key2)
        return -1;
     return 0;
}

inline int BtreeCompare(const uint& key1, const uint& key2)
{
    return key2 - key1;
}

inline int BtreeCompare(const float& key1, const float& key2)
{
     if(key1 < key2)
        return 1;
     if(key1 > key2)
        return -1;
     return 0;
}

#endif
