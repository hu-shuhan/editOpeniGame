//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_IProfile_h
#define kbpO_IProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_IbeamProfile;

class kbpO_IProfile : public kbpO_Profile
{
public:
    kbpO_IProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_IProfile();

    kbpO_IProfile(double l,
                  double h,
                  double b1,
                  double b2,
                  double t1,
                  double t2,
                  double t3);

    kbpO_IProfile( const kbpO_IProfile& copy );

    virtual ~kbpO_IProfile();
    double l() const;
    double h() const;
    double b1() const;
    double b2() const;
    double t1() const;
    double t2() const;
    double t3() const;
    kbpO_IProfile& operator=( const kbpO_IProfile& rhs );

    const kbpC_IbeamProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
