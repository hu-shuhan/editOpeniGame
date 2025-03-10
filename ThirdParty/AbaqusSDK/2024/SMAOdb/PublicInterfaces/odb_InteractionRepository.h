//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_InteractionRepository_h
#define odb_InteractionRepository_h

// Begin local includes

#include <odb_Interaction.h>
#include <odb_Amplitude.h>
#include <odb_Repository.h>
#include <odb_PropagatingComponent.h>
#include <odb_RegionPairSequence.h>
#include <odb_ConPropAssignSequence.h>
#include <odb_StdContactControls.h>
#include <odb_ExpContactControls.h>
#include <odb_Set.h>
#include <odb_Union.h>


// Forward declarations
class ddb_Model;
COW_COW_DECL(ddb_Model);
class odb_ContactProperty;
class odb_FilmConditionProp;


ODB_NEWCONTAINER_DECL(odb_String, odb_Interaction, odb_InteractionRepository);

class odb_InteractionRepository: public odb_InteractionRepositoryIMPL
{

public:   // Undocumented and unsupported

    odb_InteractionRepository(const ddb_Model& ddb);

    odb_Interaction& 
    SurfaceToSurfaceContactStd(const odb_String& name,
			       const odb_String& stepName,
			       const odb_Set& master,
			       const odb_Set& slave,
			       odb_String sliding,
			       const odb_ContactProperty& cp,			       
			       odb_String interferenceType ="NONE", //NONE, SHRINK_FIT, UNIFORM
			       double overClosure=0.0,
			       const odb_Amplitude& amp = odb_Amplitude(),
			       odb_String adjustMethod="NONE",
			       double adjustTolerance=0.0,
			       const odb_Set& adjustSet = odb_Set(), 
                               odb_String enforcement = "NODE_TO_SURFACE", 
			       bool thickness=false,
			       odb_Union initialClearance="OMIT",
			       const odb_Set& clearanceRegion = odb_Set(),
			       odb_Union hcritValue = "DEFAULT",
			       const odb_StdContactControls& sc = odb_StdContactControls(),
			       odb_String interferenceDirType = "COMPUTED", //COMPUTED, DIRECTION_COSINE
			       const odb_SequenceFloat& interferenceDir = odb_SequenceFloat(),
			       double extensionZone = 0.1,
			       bool tied = false,
	                       odb_String geometricCorrection=odb_String());



    odb_Interaction& ContactExp(const odb_String& name,
				const odb_String& stepName, 
				bool useAll,
				const odb_RegionPairSequence& includedPairs = odb_RegionPairSequence(),
				const odb_RegionPairSequence& excludedPairs = odb_RegionPairSequence(),
				const odb_ConPropAssignSequence& conPropAssignSeq = odb_ConPropAssignSequence()
	);
    odb_Interaction&
    SurfaceToSurfaceContactExp(const odb_String& name,
			       const odb_String& stepName,
			       const odb_Set& master,
			       const odb_Set& slave,
			       const odb_ContactProperty& cp,
			       odb_String sliding="FINITE",
			       odb_String mechanicalConstraint="KINEMATIC",
			       odb_String weightingFactorType="DEFAULT",
			       double weightingFactor=0.0,
			       odb_Union initialClearance="OMIT",
			       const odb_Set& clearanceRegion = odb_Set(),
			       const odb_ExpContactControls& ec = odb_ExpContactControls()); 
  
    //datumAxis,
    //useReverseDatumAxis,
    //odb_Union halfThreadAngle,
    //odb_Union pitch,
    //odb_Union majorBoltDiameter,
    //odb_Union meanBoltDiameter
    //contactControls
				
    odb_Interaction& FilmCondition(const odb_String& name,
	    const odb_String& stepName,
            const odb_Set& surface,
            odb_String definition,
	    const odb_FilmConditionProp& intProp,
	    double sinkTemp=0.0,
            const odb_Amplitude& sinkAmp = odb_Amplitude(),
	    double filmCoef=0.0,
            const odb_Amplitude& filmCoefAmp = odb_Amplitude() );

odb_Interaction& SurfaceRadiation(
	    const odb_String& name,
	    const odb_String& stepName,
            const odb_Set& surface,
            double ambientTemp=0.0,
	    const  odb_Amplitude& ambTempAmp= odb_Amplitude() ,
	    double emissivity=0.0,
            const cow_String& radType = "AMBIENT" );

    odb_Interaction& ConcRadiation(const odb_String& name,
	    const odb_String& stepName,
            const odb_Set& nodeSet,
            double associatedArea = 1.0,
	    double ambTemp=0.0,
            double emissivity=1.0,
            const odb_Amplitude& ambTempAmp= odb_Amplitude() );

private:

    ddb_ModelCOW m_modelCOW;
    bool isRead;

public:   // Undocumented and unsupported

    odb_InteractionRepository();
    void Read();

};

COW_COW_DECL(odb_InteractionRepository);

#endif
