//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_ElemCtrlParams_h
#define odb_ElemCtrlParams_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>
#include <cow_ArrayCOW.h>
#include <odb_String.h>
#include <ddb_ElemCtrlParams.h>

// Forward declarations
class ddb_ElemCtrlParams;
COW_COW_DECL(ddb_ElemCtrlParams);

class odb_ElemCtrlParams : public mem_AllocationOperators
{
public:    // undocumented and unsupported
    odb_ElemCtrlParams();
   
    
    odb_ElemCtrlParams(odb_String elemLibrary, //Standard, Explicit
		       double hourglassStiffness = 0.0,
		       double bendingHourglass = 0.0,
		       double drillingHourglass = 0.0,
		       odb_String kinematicSplit = "Average strain", //Average strain, Orthogonal, Centroid
		       odb_String distortionControl = "Default Distortion Control", //Distortion Control ON 
		                                                                    //Distortion Control OFF 
		                                                                    //Default Distortion Control
		       double lengthRatio = 0.1,
		       bool secondOrderAccuracy = false,
		       odb_String hourglassControl = "Enhanced", //Default Hourglass Control, Relax stiffness, Stiffness, Viscous, Enhanced, Combined
		       double weightFactor = 0.5,
		       double displacementHourglass = 1.0,
		       double rotationalHourglass = 1.0,
		       double outOfPlaneDisplacementHourglass = 1.0,
		       double linBlkViscosity = 1.0,
		       double quadBlkViscosity = 1.0);
    

private:
    ddb_ElemCtrlParamsCOW m_ElemCtrlParams;

private:
    bool m_hasValue;

public:
    bool HasValue() const;
    const ddb_ElemCtrlParams& ElemCtrlParams() const;

};

#endif //odb_ElemCtrlParams_h
