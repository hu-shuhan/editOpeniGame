//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_TrapezoidalProfile_h
#define kbpO_TrapezoidalProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_TrapezoidalProfile;

class kbpO_TrapezoidalProfile : public kbpO_Profile
{
public:
    kbpO_TrapezoidalProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_TrapezoidalProfile();

    kbpO_TrapezoidalProfile(double a,
                            double b,
                            double c,
                            double d);

    kbpO_TrapezoidalProfile( const kbpO_TrapezoidalProfile& copy );

    virtual ~kbpO_TrapezoidalProfile();
    double a() const;
    double b() const;
    double c() const;
    double d() const;
    kbpO_TrapezoidalProfile& operator=( const kbpO_TrapezoidalProfile& rhs );

    const kbpC_TrapezoidalProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
