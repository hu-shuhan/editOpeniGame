//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Amplitude_h
#define odb_Amplitude_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kamC_AmplitudeShortcut.h>
#include <odb_Types.h>
#include <odb_String.h>
// Forward declarations
class kamC_Amplitude;

typedef cow_CountedHld<kamC_Amplitude> kamC_AmplitudeHld;
class odb_Amplitude : public mem_AllocationOperators
{
public:
    odb_Amplitude( const kamC_AmplitudeShortcut& shortcut );
    odb_Amplitude();

    virtual ~odb_Amplitude();
    kamC_AmplitudeShortcut Shortcut() const { return shortcut; }

    bool hasValue() const { return shortcut.HasValue(); }

    static unsigned int typeIdentifier();

    unsigned int subTypeIdentifier() const;

    odb_String name() const { if (hasValue()) return shortcut.Name();
      else return odb_String(); }

    virtual odb_Amplitude* Copy() const;

protected:
    kamC_AmplitudeShortcut shortcut;
    kamC_AmplitudeHld pointer;
};

COW_ARRAYCOW2_DECL(odb_Amplitude, cow_Virtual);

#endif
