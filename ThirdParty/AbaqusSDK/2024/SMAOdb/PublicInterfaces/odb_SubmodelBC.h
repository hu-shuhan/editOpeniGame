//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SubmodelBC_h
#define odb_SubmodelBC_h

// Begin local includes
#include <odb_BC.h>
#include <odb_Types.h>
#include <cow_COW.h>
#include <odb_SequenceFrame.h>

// Forward declarations
class ddb_SubmodelBC;
class odb_PropagatingComponent;

class odb_SubmodelBC: public odb_BC
{
public:
    
    void setFixedInStep( const odb_String& stepName, bool f );
    
    void setGlobalStepInStep( const odb_String& stepName, 
			      const odb_String& gStepName );
    
    void setTimeScaleInStep( const odb_String& stepName,
			     bool timeScale );

    void setGlobalIncrementInStep( const odb_String& stepName,
				   int inc );

    void setDofInStep( const odb_String& stepName,
		       const odb_SequenceInt& dof );

    void setCenterZoneSizeInStep( const odb_String& stepName, 
				  double czs );

private:

public:   // undocumented and unsupported

    // Needed for container
    odb_SubmodelBC();

    odb_SubmodelBC(const ddb_SubmodelBC& bc);
    
};

#endif // odb_SubmodelBC_h
