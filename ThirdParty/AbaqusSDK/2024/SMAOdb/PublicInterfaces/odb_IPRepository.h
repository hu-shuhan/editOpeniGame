//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_IPRepository_h
#define odb_IPRepository_h

// Begin local includes

#include <odb_InteractionProperty.h>
#include <odb_FilmConditionProp.h>
#include <odb_Repository.h>
#include <odb_Util.h>

// Forward declarations
class ddb_Model;
COW_COW_DECL(ddb_Model);

class odb_ContactProperty;


ODB_NEWCONTAINER_DECL(odb_String, odb_InteractionProperty, odb_IPRepository);

class odb_IPRepository: public odb_IPRepositoryIMPL
{

public:   // Undocumented and unsupported

    odb_IPRepository(const ddb_Model& ddb);

    odb_ContactProperty& ContactProperty(const odb_String& name);

    odb_FilmConditionProp& FilmConditionProp(const odb_String& name,
	    bool temperatureDependency = false,
	    int dependencies = 0,
	    const odb_SequenceSequenceFloat& table=odb_SequenceSequenceFloat());

private:

    ddb_ModelCOW m_modelCOW;
    bool isRead;

public:   // Undocumented and unsupported

    odb_IPRepository();
    void Read();

};

COW_COW_DECL(odb_IPRepository);

#endif
