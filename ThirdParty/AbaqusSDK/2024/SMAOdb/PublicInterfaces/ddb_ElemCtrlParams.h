//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_ElemCtrlParams_h
#define ddb_ElemCtrlParams_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_String.h>
#include <cow_COW.h>
#include <odiK_ElemCtrlParamsShortcut.h>
#include <bme_ElemCtrlParams.h>


//Forward declarations
//class omu_PrimEnumBase;


class ddb_ElemCtrlParams  : public mem_AllocationOperators
{
public:
    ddb_ElemCtrlParams( const odiK_ElemCtrlParamsShortcut& sc );
    ddb_ElemCtrlParams();

    ~ddb_ElemCtrlParams();

    void SetElemCtrlParams(cow_String elemLibrary = "Standard", //Standard, Explicit
			   double hourglassStiffness = 0.0,
			   double bendingHourglass = 0.0,
			   double drillingHourglass = 0.0,
			   cow_String kinematicSplit = "Average strain", //Average strain, Orthogonal, Centroid
			   cow_String distortionControl = "Default", //Default, Yes, No
			   double lengthRatio = 0.1,
			   bool secondOrderAccuracy = false,
			   cow_String hourglassControl = "Enhanced", //Relax stiffness, Stiffness, Viscous, Enhanced, Combined
			   double weightFactor = 0.5,
			   double displacementHourglass = 1.0,
			   double rotationalHourglass = 1.0,
			   double outOfPlaneDisplacementHourglass = 1.0,
			   double linBlkViscosity = 1.0,
			   double quadBlkViscosity = 1.0);

    double hourglassStiffness() const;
    void setHourglassStiffness( double hs );

    double bendingHourglassStiffness() const;
    void setBendingHourglassStiffness( double bhs );

    double drillingHourglassStiffness() const;
    void setDrillingHourglassStiffness( double dhs );

    bme_KinematicSplit kinematicSplit() const;
    void setKinematicSplit( bme_KinematicSplit ks );

    bme_HourglassControl hourglassControl() const;
    void setHourglassControl( bme_HourglassControl hc );

    double weightFactor() const;
    void setWeightFactor( double wf );

    bme_StressRate stressRate() const;
    void setStressRate( bme_StressRate sr );

    bme_SoAccuracy soAccuracy() const;
    void setSoAccuracy( bme_SoAccuracy sa );

    double displacementHourglassStiffness() const;
    void setDisplacementHourglassStiffness( double dhs );

    double rotationHourglassStiffness() const;
    void setRotationHourglassStiffness( double rhs );

    double outOfPlaneHourglassStiffness() const;
    void setOutOfPlaneHourglassStiffness( double ohs );

    bme_ElemLibrary elemLibrary() const;
    void setElemLibrary( bme_ElemLibrary el );

    odiK_ElemCtrlParamsShortcut Shortcut() const { return shortcut; }

    const bme_ElemCtrlParams& elemCtrlParams() const;

protected:

    const bme_ElemCtrlParams& ConstGet(const ddr_DdbContainer& ) const;
    bme_ElemCtrlParams& Get(ddr_DdbContainer& ); 
 

private:
    odiK_ElemCtrlParamsShortcut shortcut;
    bme_ElemCtrlParams m_ec;

    void SetEnum(const cow_String& enumName,
                 omu_PrimEnumBase& enm,
                 const cow_String& typeFound);
};

// Define class handler
COW_COW_DECL(ddb_ElemCtrlParams);

#endif
