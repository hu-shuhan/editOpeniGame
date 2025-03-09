//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_RectangularProfile_h
#define kbpO_RectangularProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_RectangularProfile;

class kbpO_RectangularProfile : public kbpO_Profile
{
public:
    kbpO_RectangularProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_RectangularProfile();

    kbpO_RectangularProfile(double a,
                            double b);

    kbpO_RectangularProfile( const kbpO_RectangularProfile& copy );

    virtual ~kbpO_RectangularProfile();
    double a() const;
    double b() const;
    kbpO_RectangularProfile& operator=( const kbpO_RectangularProfile& rhs );

    const kbpC_RectangularProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
