//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef kbpO_GeneralizedProfile_h
#define kbpO_GeneralizedProfile_h

// Begin local includes
#include <kbpO_Profile.h>
#include <odb_Types.h>
// Forward declarations
class kbpC_GeneralizedProfile;

class kbpO_GeneralizedProfile : public kbpO_Profile
{
public:
    kbpO_GeneralizedProfile( const kbpC_BeamProfileShortcut& shortcut );
    kbpO_GeneralizedProfile();

    kbpO_GeneralizedProfile(double area,
                            double i11,
                            double i12,
                            double i22,
                            double j,
                            double gammaO,
                            double gammaW);

    kbpO_GeneralizedProfile( const kbpO_GeneralizedProfile& copy );

    virtual ~kbpO_GeneralizedProfile();
    double area() const;
    double i11() const;
    double i12() const;
    double i22() const;
    double j() const;
    double gammaO() const;
    double gammaW() const;
    kbpO_GeneralizedProfile& operator=( const kbpO_GeneralizedProfile& rhs );

    const kbpC_GeneralizedProfile* GetPointer() const;

    static unsigned int typeIdentifier();

    virtual kbpO_Profile* Copy() const;

};

#endif
