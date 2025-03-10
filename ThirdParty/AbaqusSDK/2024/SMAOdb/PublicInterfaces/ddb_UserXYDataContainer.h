//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_UserXYDataContainer_h
#define ddb_UserXYDataContainer_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: ddb_UserXYDataContainer.h
//
// Description:  Root uddults object on ddb
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <ddb_UserXYData.h>
#include <udd_UserXYDataContainerShortcut.h>
#include <udd_UserXYDataContainer.h>
#include <cow_ListString.h>
#include <cow_2DArrayString.h>

// Forward declarations
class ddb_UserXYDataContainerIT;

// Class definition

class ddb_UserXYDataContainer  : public mem_AllocationOperators
{
public:
    ddb_UserXYDataContainer(const udd_UserXYDataContainerShortcut&);

    ddb_UserXYData operator[](const cow_String&) const;
    void insert(const cow_String&, const ddb_UserXYData&);
//    void remove(const cow_String&);

    ddb_UserXYData CreateXYDataInContainer(const cow_String& name, int size=0 );

    int size() const;
    bool isEmpty() const;
    bool isMember(const cow_String& name) const;
    cow_ListString keys() const;
    cow_2DArrayString summary() const;

    ddb_UserXYDataContainerIT createIterator() const;

    udd_UserXYDataContainerShortcut Shortcut() const { return shortcut; }

private:
    udd_UserXYDataContainerShortcut shortcut;
};

class ddb_UserXYDataContainerIT  : public mem_AllocationOperators
{
public:
     ddb_UserXYDataContainerIT(const udd_UserXYDataContainer& cont,
			       const udd_UserXYDataContainerShortcut& sc);

     inline void first();
     inline void next();
     inline bool isDone();

     inline cow_String currentKey() const;
     inline ddb_UserXYData currentValue() const;

private:
     udd_UserXYDataContainerIT iter;
     udd_UserXYDataContainerShortcut shortcut;
};

inline
void
ddb_UserXYDataContainerIT::first()
{
     iter.First();
}

inline
void
ddb_UserXYDataContainerIT::next()
{
     iter.Next();
}

inline
bool
ddb_UserXYDataContainerIT::isDone()
{
     return iter.IsDone();
}

inline
cow_String
ddb_UserXYDataContainerIT::currentKey() const
{
     return iter.CurrentKey();
}

inline
ddb_UserXYData
ddb_UserXYDataContainerIT::currentValue() const
{
     return ddb_UserXYData(udd_UserXYDataInCShortcut(shortcut, iter.CurrentKey()));
}


#endif 
