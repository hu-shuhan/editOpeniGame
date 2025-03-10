//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_LoadRepository_h
#define odb_LoadRepository_h

// Begin local includes

#include <odb_Amplitude.h>
#include <odb_DatumCsys.h>
#include <odb_Load.h>
#include <odb_Repository.h>
#include <odb_PropagatingComponent.h>
#include <gsl_ModelSegment.h>

// Forward declarations
class ddb_Model;
class odb_Set;
COW_COW_DECL(ddb_Model);



ODB_NEWCONTAINER_DECL(odb_String, odb_Load, odb_LoadRepository);

class odb_LoadRepository: public odb_LoadRepositoryIMPL
{

public:   // Undocumented and unsupported

    odb_LoadRepository(const ddb_Model& ddb);

    odb_Load& Pressure(const odb_String& name,
	    const odb_String& stepName,
            const odb_Set& surface,
            odb_PropagatingComponent p,
	    double hZero,
	    double hReference,
	    odb_String distribution = "UNIFORM",
            const odb_Amplitude& amp=odb_Amplitude());

    odb_Load& LineLoad( const odb_String& name,
			   			const odb_String& stepName,
			   			const odb_Set& region,
			   			odb_PropagatingComponent c1,
			   			odb_PropagatingComponent c2,
			   			odb_PropagatingComponent c3,
			   			const odb_Amplitude& amp=odb_Amplitude(),  //amplitude
			   			odb_String distribution = "UNIFORM",      //distributionType
			   			odb_String system="GLOBAL");

    odb_Load& BodyForce( const odb_String& name,
			     		 const odb_String& stepName,
			     		 const odb_Set& region,
			     		 //odb_String distribution = "UNIFORM",      //distributionType
			     		 odb_String distribution,      //distributionType
				   		 odb_PropagatingComponent c1,
				   		 odb_PropagatingComponent c2,
				   		 odb_PropagatingComponent c3,
				   		 const odb_Amplitude& amp=odb_Amplitude());  

    odb_Load& SurfaceTraction(const odb_String& name,          //name
    			      			  const odb_String& stepName,      //createStepName
    			      			  const odb_Set& surface,          //?Region?
    			      			  odb_PropagatingComponent p,      //?magnitude?
    			      			  odb_String distribution = "UNIFORM",      //distributionType
    			      			  odb_String traction = "GENERAL", //tractionType
    			      			  const odb_Amplitude& amp=odb_Amplitude(),  //amplitude
    			      			  gsl_ModelSegment directionVector=gsl_ModelSegment(gsl_Dot(0.0,0.0,0.0),gsl_Dot(0.0,0.0,0.0))
    			      				);
                                  
    odb_Load& ConcentratedForce(const odb_String& name,
            const odb_String& stepName,
            const odb_Set& pointSet,
            odb_PropagatingComponent f1,
            odb_PropagatingComponent f2,
            odb_PropagatingComponent f3,
            const odb_Amplitude& amp=odb_Amplitude(),
	    bool follower=false,
	    const odb_DatumCsys& csys=odb_DatumCsys());

    odb_Load& Moment(const odb_String& name,
                                const odb_String& stepName,
                                const odb_Set& pointSet,
                                odb_PropagatingComponent m1,
                                odb_PropagatingComponent m2,
                                odb_PropagatingComponent m3,
                                const odb_Amplitude& amp=odb_Amplitude(),
                                bool follower=false,
                                const odb_DatumCsys& csys=odb_DatumCsys());

    

    odb_Load& ConcentratedHeatFlux(const odb_String& name,
            const odb_String& stepName,
            const odb_Set& region,
	    double magnitude,
            const odb_Amplitude& amp = odb_Amplitude(),
	    int dof = 11);

    odb_Load& ConnectorForce(const odb_String& name,
            const odb_String& stepName,
	    const odb_Set& region,
            odb_PropagatingComponent f1=odb_PropagatingComponent(),
            odb_PropagatingComponent f2=odb_PropagatingComponent(),
            odb_PropagatingComponent f3=odb_PropagatingComponent() );

    odb_Load& BodyHeatFlux(const odb_String& name,
            const odb_String& stepName,
            const odb_Set& region,
	    double magnitude,
	    odb_String distribution = "UNIFORM",
            const odb_Amplitude& amp = odb_Amplitude() );

    odb_Load& SurfaceHeatFlux(const odb_String& name,
            const odb_String& stepName,
            const odb_Set& region,
	    double magnitude,
	    odb_String distribution = "UNIFORM",
            const odb_Amplitude& amp = odb_Amplitude() );

    odb_Load& Gravity(const odb_String& name,
            const odb_String& stepName,
            const odb_Set& region,
            odb_PropagatingComponent f1=odb_PropagatingComponent(),
            odb_PropagatingComponent f2=odb_PropagatingComponent(),
            odb_PropagatingComponent f3=odb_PropagatingComponent(),
            const odb_Amplitude& amp = odb_Amplitude() );

    odb_Load& RotationalBodyForce(const odb_String& name,
            const odb_String& stepName,
            const odb_Set& region,
	    double magnitude,
	    bool centrifugal,
	    bool rotaryAcceleration,
	    const odb_SequenceFloat& point1,
	    const odb_SequenceFloat& point2,
            const odb_Amplitude& amp = odb_Amplitude() );

private:
    ddb_ModelCOW m_modelCOW;
    bool isRead;

public:   // Undocumented and unsupported
    odb_LoadRepository();
    void Read();

};

COW_COW_DECL(odb_LoadRepository);

#endif
