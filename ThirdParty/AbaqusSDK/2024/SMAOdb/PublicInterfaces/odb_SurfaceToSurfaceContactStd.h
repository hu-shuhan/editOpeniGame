//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SurfaceToSurfaceContactStd_h
#define odb_SurfaceToSurfaceContactStd_h

// Begin local includes
#include <odb_Interaction.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_SurfaceToSurfaceContactStd;
class odb_Union;
class odb_Set;
class odb_DatumCsys;
class odb_ContactProperty;

class odb_SurfaceToSurfaceContactStd: public odb_Interaction
{
public:

    // member access
    odb_String sliding() const;
    odb_String interferenceType() const;
    double overclosure() const;
    odb_String interferenceDirectionType() const;
    double smooth() const;
    double hcrit() const;
    bool limitSlideDistance() const;
    double slideDistance() const;
    double extensionZone() const;
    odb_String adjustMethod() const;
    double adjustTolerance() const;
    odb_String enforcement() const;
    bool thickness() const;
    bool tied() const;
    odb_String contactTracking() const;
    odb_String supplementaryContact() const;
    odb_Union initialClearance() const;
    odb_Union majorBoltDiameter() const;
    odb_Union meanBoltDiameter() const;
    bool useReverseDatumAxis() const;
    odb_String createStepName() const;
    odb_String interactionProperty() const;
    odb_String amplitude() const;
    odb_String contactControls() const;
    odb_Set master() const;
    odb_Set slave() const;
    odb_Set adjustSet() const;
    odb_SequenceDouble direction() const;
    odb_SequenceDouble halfThreadAngle() const;
    odb_SequenceDouble pitch() const;
    odb_DatumCsys datumAxis() const;
    odb_Set clearanceRegion() const;
    static unsigned int typeIdentifier();
    odb_String geometricCorrection() const;

    // Options

    void setInteractionPropertyInStep( const odb_String& stepName,
                                       const odb_ContactProperty& p );


private:

public:   // undocumented and unsupported
    odb_SurfaceToSurfaceContactStd( const odb_SurfaceToSurfaceContactStd& copy );
    odb_SurfaceToSurfaceContactStd& operator=( const odb_SurfaceToSurfaceContactStd& rhs );

    // Needed for container
    odb_SurfaceToSurfaceContactStd();

    odb_SurfaceToSurfaceContactStd(const ddb_SurfaceToSurfaceContactStd& interaction);
    
};

#endif // odb_SurfaceToSurfaceContactStd_h
