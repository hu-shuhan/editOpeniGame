//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_BoxProfile_h
#define kbpO_BoxProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_BoxProfile;

class kbpO_BoxProfile : public kbpO_Profile
{
public:
    kbpO_BoxProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_BoxProfile();

    kbpO_BoxProfile(double a,
                    double b,
                    bool uniformThickness,
                    double t1,
                    double t2=0,
                    double t3=0,
                    double t4=0);

    kbpO_BoxProfile( const kbpO_BoxProfile& copy );

    virtual ~kbpO_BoxProfile();
    double a() const;
    double b() const;
    bool uniformThickness() const;
    double t1() const;
    double t2() const;
    double t3() const;
    double t4() const;
    kbpO_BoxProfile& operator=( const kbpO_BoxProfile& rhs );

    const kbpC_BoxProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
