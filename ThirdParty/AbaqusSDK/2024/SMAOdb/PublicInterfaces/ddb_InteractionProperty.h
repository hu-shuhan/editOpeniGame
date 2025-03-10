//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_InteractionProperty_h
#define ddb_InteractionProperty_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kinC_IPShortcut.h>
#include <cow_COW.h>

// Forward declarations
class ddb_AcousticImpedanceProp;
class ddb_ActuatorSensorProp;
class ddb_ContactProperty;
class ddb_FilmConditionProp;
class ddb_IncidentWaveProperty;

class ddb_InteractionProperty  : public mem_AllocationOperators
{
public:
    ddb_InteractionProperty( const kinC_InteractionPropertyShortcut& sc );
    ddb_InteractionProperty();

    virtual ~ddb_InteractionProperty();

    typ_typeTag interactionPropertyType() const;

    ddb_AcousticImpedanceProp CvtAcousticImpedanceProp() const;
    ddb_ActuatorSensorProp CvtActuatorSensorProp() const;
    ddb_ContactProperty CvtContactProperty() const;
    ddb_FilmConditionProp CvtFilmConditionProp() const;
    ddb_IncidentWaveProperty CvtIncidentWaveProperty() const;

    kinC_InteractionPropertyShortcut Shortcut() const { return shortcut; }

    // Used by OIF
    const kinC_InteractionProperty& interactionProperty() const;

protected:
    kinC_InteractionPropertyShortcut shortcut;
};

COW_COW_DECL(ddb_InteractionProperty);

#endif
