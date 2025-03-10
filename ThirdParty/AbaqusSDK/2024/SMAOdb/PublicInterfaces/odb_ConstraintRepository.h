//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_ConstraintRepository_h
#define odb_ConstraintRepository_h

// Begin local includes
#include <BasicConstants.h>
#include <odb_String.h>
#include <odb_Constraint.h>
#include <odb_DatumCsys.h>
#include <odb_Repository.h>
#include <odb_Set.h>
#include <odb_Equation.h>

// Forward declarations
class odb_Set;
class ddb_Model;

COW_COW_DECL(ddb_Model);

ODB_NEWCONTAINER_DECL(odb_String, odb_Constraint, odb_ConstraintRepository);

class odb_ConstraintRepository: public odb_ConstraintRepositoryIMPL
{

public:   // Undocumented and unsupported

    odb_ConstraintRepository(const ddb_Model& ddb);

    odb_Constraint& Tie(const odb_String& name,
                        const odb_Set& master,
                        const odb_Set& slave,
			const odb_Set& tiedNset,
			cow_ListInt & pairOffsets,
			cow_ListInt & pairNodeAndPart,
                        bool adjust=true,
                        odb_String positionToleranceType="COMPUTED",
                        double positionTolerance=0.0,
                        bool tieRotations=true,
                        odb_String constraintRatioEnm="DEFAULT",
                        double constraintRatio=0.0,
                        odb_String constraintEnforcement="SOLVER_DEFAULT", 
			bool thickness=true,
			bool cyclicSymmetry=false,
			int numPairings=0,
			int lengthTable=0);

    odb_Constraint& Coupling(const odb_String& name,
	    const odb_Set& surface, const odb_Set& refPoint,
	    cow_ListInt & pairOffsets,
	    cow_ListInt & pairNodeAndPart,
            odb_String couplingType="KINEMATIC",
            double influenceRadius=DBL_MAX,
            bool u1=true, bool u2=true, bool u3=true,
            bool u4=true, bool u5=true, bool u6=true,
            odb_String weigthingMethod="UNIFORM",
	    //            const odb_Set& couplingNodes = odb_Set(),
            const odb_DatumCsys& csys = odb_DatumCsys(),
	    int numPairings=0,
	    int lengthTable=0 );

    odb_Constraint& RigidBody(const odb_String& name,
	    const odb_Set& refPointRegion,
	    const odb_Set& bodyRegion,
	    const odb_Set& tieRegion,
	    const odb_Set& pinRegion,
	    const odb_Set& surfaceRegion,
            bool refPointAtCOM=false,
            bool isothermal=false);
    
    odb_Constraint& Equation(const odb_String& name, 
			     odb_EquationTermList& eqTermList);
			     

private:
    ddb_ModelCOW m_modelCOW;
    bool isRead;

public:   // Undocumented and unsupported
    odb_ConstraintRepository();
    void Read();

};

COW_COW_DECL(odb_ConstraintRepository);


#endif
