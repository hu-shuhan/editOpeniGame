//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_ContactControlsRepository_h
#define odb_ContactControlsRepository_h

// Begin local includes

#include <odb_ContactControls.h>
#include <odb_Repository.h>
#include <odb_Util.h>
#include <odb_Union.h>


// Forward declarations
class ddb_Model;
COW_COW_DECL(ddb_Model);

class odb_StdContactControls;
class odb_ExpContactControls;

ODB_NEWCONTAINER_DECL(odb_String, odb_ContactControls, odb_ContactControlsRepository);

class odb_ContactControlsRepository: public odb_ContactControlsRepositoryIMPL
{

public:   // Undocumented and unsupported

    odb_ContactControlsRepository(const ddb_Model& ddb);

    odb_StdContactControls& 
    StdContactControls(const odb_String& name,
		       double stiffnessScaleFactor = 1.0,
		       const odb_String& penetrationTolChoice = "RELATIVE", //RELATIVE, ABSOLUTE
		       double relativePenetrationTolerance = 0.001,
		       odb_Union absolutePenetrationTolerance = "None",
		       const odb_String& slideDistanceChoice = "DEFAULT", //DEFAULT, SPECIFY
		       double slideDistance = 0.0,
		       const odb_String& frictionOnsetChoice = "IMMEDIATE", //IMMEDIATE, DELAYED
		       bool automaticTolerances = false,
		       int maxchp = 0,
		       double perrmx = 0.0,
		       double uerrmx = 0.0,
		       const odb_String& stabilizeChoice = "NONE", //NONE, AUTOMATIC, COEFFICIENT
		       double dampFactor = 1.0,
		       double dampCoef = 0.0,
		       double tangFraction = 1.0,
		       double eosFraction = 0.0,
		       const odb_String& zeroDampingChoice = "COMPUTE", //COMPUTE, SPECIFY
		       odb_Union zeroDamping = "None",
		       const odb_String& enforceWithLagrangeMultipliers = "DEFAULT" 
		       //DEFAULT, ENFORCEMENT_OFF, ENFORCEMENT_ON  
		       );
      // odb_StdContactControls stdContactControls();
    
    odb_ExpContactControls& 
    ExpContactControls(const odb_String& name,
		       const odb_String& globTrkChoice = "DEFAULT", //DEFAULT, SPECIFY
		       int globTrkInc = INT_MAX,
		       bool fastLocalTrk = true,
		       double scalePenalty = 1.0,
		       int warpCheckPeriod = 20,
		       double warpCutoff = 20.0);
    //odb_ExpContactControls expContactControls();

private:

    ddb_ModelCOW m_modelCOW;
    bool isRead;

public:   // Undocumented and unsupported

    odb_ContactControlsRepository();
    void Read();

};

COW_COW_DECL(odb_ContactControlsRepository);

#endif //odb_ContactControlsRepository_h










