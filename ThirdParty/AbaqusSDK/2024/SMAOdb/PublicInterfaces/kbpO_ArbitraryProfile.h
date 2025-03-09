//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_ArbitraryProfile_h
#define kbpO_ArbitraryProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_ArbitraryProfile;

class kbpO_ArbitraryProfile : public kbpO_Profile
{
public:
    kbpO_ArbitraryProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_ArbitraryProfile();

    kbpO_ArbitraryProfile(const odb_SequenceSequenceDouble& table);

    kbpO_ArbitraryProfile( const kbpO_ArbitraryProfile& copy );

    virtual ~kbpO_ArbitraryProfile();
    odb_SequenceSequenceDouble table() const;
    kbpO_ArbitraryProfile& operator=( const kbpO_ArbitraryProfile& rhs );

    const kbpC_ArbitraryProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
