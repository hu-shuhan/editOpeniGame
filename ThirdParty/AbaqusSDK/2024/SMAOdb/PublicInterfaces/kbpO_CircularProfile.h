//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_CircularProfile_h
#define kbpO_CircularProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_CircularProfile;

class kbpO_CircularProfile : public kbpO_Profile
{
public:
    kbpO_CircularProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_CircularProfile();

    kbpO_CircularProfile(double r);

    kbpO_CircularProfile( const kbpO_CircularProfile& copy );

    virtual ~kbpO_CircularProfile();
    double r() const;
    kbpO_CircularProfile& operator=( const kbpO_CircularProfile& rhs );

    const kbpC_CircularProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
