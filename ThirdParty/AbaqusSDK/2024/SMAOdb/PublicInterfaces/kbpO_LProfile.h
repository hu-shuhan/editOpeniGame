//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_LProfile_h
#define kbpO_LProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_LbeamProfile;

class kbpO_LProfile : public kbpO_Profile
{
public:
    kbpO_LProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_LProfile();

    kbpO_LProfile(double a,
                  double b,
                  double t1,
                  double t2);

    kbpO_LProfile( const kbpO_LProfile& copy );

    virtual ~kbpO_LProfile();
    double a() const;
    double b() const;
    double t1() const;
    double t2() const;
    kbpO_LProfile& operator=( const kbpO_LProfile& rhs );

    const kbpC_LbeamProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
