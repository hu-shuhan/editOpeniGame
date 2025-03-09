//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_GeneralFieldBase_h
#define odb_GeneralFieldBase_h

// Begin local includes
#include <odb_FromFile.h>
#include <odb_Types.h>

// Forward declarations
class ddb_GeneralFieldBase;


class odb_GeneralFieldBase: public odb_FromFile
{
public:

    void setMagnitudesInStep(const odb_String& stepName, 
                            const odb_SequenceDouble& magnitudes);
    void resetToInitial(const odb_String& stepName);
    void resetToPropagated(const odb_String& stepName);

private:

public:   // undocumented and unsupported

    // Needed for container
    odb_GeneralFieldBase();

    odb_GeneralFieldBase(const ddb_GeneralFieldBase& load);
    
};

#endif // odb_GeneralFieldBase_h
