//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SurfaceToSurfaceContactExp_h
#define odb_SurfaceToSurfaceContactExp_h

// Begin local includes
#include <odb_Interaction.h>
#include <odb_Types.h>
#include <cow_COW.h>

// Forward declarations
class ddb_SurfaceToSurfaceContactExp;
class odb_DatumCsys;
class odb_ContactProperty;
class odb_Union;
class odb_Set;

class odb_SurfaceToSurfaceContactExp: public odb_Interaction
{
public:
    
    // member access
    odb_String sliding() const;
    bool masterNoThick() const;
    bool slaveNoThick() const;
    odb_String mechanicalConstraint() const;
    odb_String weightingFactorType() const;
    double weightingFactor() const;
    odb_Union initialClearance() const;
    odb_String createStepName() const;
    odb_String interactionProperty() const;
    odb_Set master() const;
    odb_Set slave() const;
    bool useReverseDatumAxis() const;
    odb_String contactControls() const;
    static unsigned int typeIdentifier();
    odb_DatumCsys datumAxis() const;

    // Options
  void setInteractionPropertyInStep( const odb_String& stepName,
				     const odb_ContactProperty& p );

private:

public:   // undocumented and unsupported
  
  odb_SurfaceToSurfaceContactExp( const odb_SurfaceToSurfaceContactExp& copy );
  odb_SurfaceToSurfaceContactExp& operator=( const odb_SurfaceToSurfaceContactExp& rhs );


    // Needed for container
    odb_SurfaceToSurfaceContactExp();

    odb_SurfaceToSurfaceContactExp(const 
           ddb_SurfaceToSurfaceContactExp& interaction);
    



};

#endif // odb_SurfaceToSurfaceContactExp_h
