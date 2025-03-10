//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_BCRepository_h
#define odb_BCRepository_h

// Begin local includes

#include <odb_Amplitude.h>
#include <odb_BC.h>
#include <odb_Repository.h>
#include <odb_PropagatingComponent.h>
#include <odb_DatumCsys.h>
#include <odb_SequenceFrame.h>
#include <odb_Union.h>

// Forward declarations
class ddb_Model;
class odb_Set;
class ddb_SubmodelBC;
COW_COW_DECL(ddb_Model);



ODB_NEWCONTAINER_DECL(odb_String, odb_BC, odb_BCRepository);

class odb_BCRepository: public odb_BCRepositoryIMPL
{

public:  // Undocumented and unsupported

    odb_BCRepository(const ddb_Model& ddb);

    odb_BC DisplacementBC(const odb_String& name,
	    const odb_String& stepName,
            const odb_Set& set,
            odb_PropagatingComponent u1 = odb_PropagatingComponent(),
            odb_PropagatingComponent u2 = odb_PropagatingComponent(),
            odb_PropagatingComponent u3 = odb_PropagatingComponent(),
            odb_PropagatingComponent ur1 = odb_PropagatingComponent(),
            odb_PropagatingComponent ur2 = odb_PropagatingComponent(),
            odb_PropagatingComponent ur3 = odb_PropagatingComponent(),
	    bool fixed = false,
            const odb_Amplitude& amp = odb_Amplitude(),
	    odb_String distribution = "UNIFORM",
            const odb_DatumCsys& csys = odb_DatumCsys() );

    odb_BC VelocityBC(const odb_String& name,
	    const odb_String& stepName,
            const odb_Set& set,
            odb_PropagatingComponent v1 = odb_PropagatingComponent(),
            odb_PropagatingComponent v2 = odb_PropagatingComponent(),
            odb_PropagatingComponent v3 = odb_PropagatingComponent(),
            odb_PropagatingComponent vr1 = odb_PropagatingComponent(),
            odb_PropagatingComponent vr2 = odb_PropagatingComponent(),
            odb_PropagatingComponent vr3 = odb_PropagatingComponent(),
	    const odb_Amplitude& amp = odb_Amplitude(),
	    odb_String distribution = "UNIFORM",
            const odb_DatumCsys& csys = odb_DatumCsys() );

    odb_BC AccelerationBC(const odb_String& name,
	    const odb_String& stepName,
            const odb_Set& set,
            odb_PropagatingComponent a1 = odb_PropagatingComponent(),
            odb_PropagatingComponent a2 = odb_PropagatingComponent(),
            odb_PropagatingComponent a3 = odb_PropagatingComponent(),
            odb_PropagatingComponent ar1 = odb_PropagatingComponent(),
            odb_PropagatingComponent ar2 = odb_PropagatingComponent(),
            odb_PropagatingComponent ar3 = odb_PropagatingComponent(),
	    const odb_Amplitude& amp = odb_Amplitude(),
	    odb_String distribution = "UNIFORM",
            const odb_DatumCsys& csys = odb_DatumCsys() );
 
    odb_BC& TemperatureBC(const odb_String& name,
	    const odb_String& stepName,
            const odb_Set& region,
	    double magnitude=0.0,
			  //int dof=11,
	    const odb_SequenceInt& dofs = odb_SequenceInt(),
            const odb_Amplitude& amp = odb_Amplitude(),
	    odb_String distribution="UNIFORM",
	    bool fixed=false );

    odb_BC& ConnDisplacementBC(const odb_String& name,
	    const odb_String& stepName,
	    const odb_Set& region,
            odb_PropagatingComponent u1=odb_PropagatingComponent(),
            odb_PropagatingComponent u2=odb_PropagatingComponent(),
            odb_PropagatingComponent u3=odb_PropagatingComponent(),
            odb_PropagatingComponent ur1=odb_PropagatingComponent(),
            odb_PropagatingComponent ur2=odb_PropagatingComponent(),
            odb_PropagatingComponent ur3=odb_PropagatingComponent(),
	    bool fixed=false,
	    odb_String distribution="UNIFORM" );
    
    odb_BC& SubmodelBC(const odb_String& name,
		       const odb_String& createStepName,
		       const odb_Set& region,
		       const odb_SequenceInt& dof,
		       const odb_String& globalStep,
		       bool timeScale = false,
		       double shellThickness = 0.0,
		       const odb_String& globalDrivingRegion = odb_String(),
		       double absoluteExteriorTolerance = 0.0,
		       double exteriorTolerance = 0.05,
		       const odb_DatumCsys& csys=odb_DatumCsys(),
		       int globalIncrement = 0,
		       odb_Union centerZoneSize = "NONE");

    odb_BC &EncastreBC( const odb_String& name,
                        const odb_String& createStepName,
                        const odb_Set& set,
                        const odb_String& buckleCaseEnmValue = "NOT_APPLICABLE" );

    odb_BC &PinnedBC( const odb_String& name,
                      const odb_String& createStepName,
                      const odb_Set& set,
                      const odb_String& buckleCaseEnmValue = "NOT_APPLICABLE");

    odb_BC &XsymmBC( const odb_String& name,
                     const odb_String& createStepName,
                     const odb_Set& set,
                     const odb_String& buckleCaseEnmValue = "NOT_APPLICABLE");

    odb_BC &YsymmBC( const odb_String& name,
                     const odb_String& createStepName,
                     const odb_Set& set,
                     const odb_String& buckleCaseEnmValue = "NOT_APPLICABLE");

    odb_BC &ZsymmBC( const odb_String& name,
                     const odb_String& createStepName,
                     const odb_Set& set,
                     const odb_String& buckleCaseEnmValue = "NOT_APPLICABLE");

    odb_BC &XasymmBC( const odb_String& name,
                      const odb_String& createStepName,
                      const odb_Set& set,
                      const odb_String& buckleCaseEnmValue = "NOT_APPLICABLE");

    odb_BC &YasymmBC( const odb_String& name,
                      const odb_String& createStepName,
                      const odb_Set& set,
                      const odb_String& buckleCaseEnmValue = "NOT_APPLICABLE");

    odb_BC &ZasymmBC( const odb_String& name,
                      const odb_String& createStepName,
                      const odb_Set& set,
                      const odb_String& buckleCaseEnmValue = "NOT_APPLICABLE");

private:
    ddb_ModelCOW m_modelCOW;
    bool isRead;

public:  // Undocumented and unsupported
    odb_BCRepository();
    void Read();

};

COW_COW_DECL(odb_BCRepository);

#endif
