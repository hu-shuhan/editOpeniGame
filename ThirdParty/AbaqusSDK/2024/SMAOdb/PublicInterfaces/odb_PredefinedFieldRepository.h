//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_PredefinedFieldRepository_h
#define odb_PredefinedFieldRepository_h

// Begin local includes

#include <odb_Amplitude.h>
#include <odb_PredefinedField.h>
#include <odb_Repository.h>
#include <odb_PropagatingComponent.h>

// Forward declarations
class ddb_Model;
class odb_Set;

COW_COW_DECL(ddb_Model);


ODB_NEWCONTAINER_DECL(odb_String, odb_PredefinedField,
	odb_PredefinedFieldRepository);

class odb_PredefinedFieldRepository: public odb_PredefinedFieldRepositoryIMPL
{

public:   // Undocumented and unsupported

    odb_PredefinedFieldRepository(const ddb_Model& ddb);

    odb_PredefinedField& Temperature(const odb_String& name,
	    const odb_String& stepName,
            const odb_Set& region,
            const odb_String& distribution="UNIFORM",
            const odb_String& crossSectDistrib="CONSTANT_OVER_SECTION",
            const odb_Amplitude& amp = odb_Amplitude(),
            const odb_String& fileName="",
            int beginStep=-2,
            int beginIncrement=-2,
            int endStep=-1,
            int endIncrement=-1,
	    bool midside=true,
	    const odb_SequenceDouble& magnitudes= odb_SequenceDouble() );

    odb_PredefinedField& Velocity(const odb_String& name,
	    const odb_Set& region,
	    double velocity1=0,
	    double velocity2=0,
	    double velocity3=0,
	    double omega=0,
	    const odb_SequenceFloat& axisBegin= odb_SequenceFloat(),
	    const odb_SequenceFloat& axisEnd= odb_SequenceFloat() );
				  

private:

    ddb_ModelCOW m_modelCOW;
    bool isRead;

public:   // Undocumented and unsupported

    odb_PredefinedFieldRepository();
    void Read();

};

COW_COW_DECL(odb_PredefinedFieldRepository);

#endif
