//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_Profile_h
#define kbpO_Profile_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kbpC_BeamProfileShortcut.h>
#include <odb_Enum.h>
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_BeamProfile;

typedef cow_CountedHld<kbpC_BeamProfile> kbpC_BeamProfileHld;
class kbpO_Profile : public mem_AllocationOperators
{
public:
    kbpO_Profile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_Profile();

    virtual ~kbpO_Profile();
    kbpC_BeamProfileShortcut Shortcut() const { return shortcut; }

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();

    unsigned int subTypeIdentifier() const;

    odb_String name() const { if (hasValue()) return shortcut.Name();
      else return odb_String(); }

    virtual kbpO_Profile* Copy() const;

protected:
    kbpC_BeamProfileShortcut shortcut;
    kbpC_BeamProfileHld pointer;
};

COW_ARRAYCOW2_DECL(kbpO_Profile, cow_Virtual);

#endif
