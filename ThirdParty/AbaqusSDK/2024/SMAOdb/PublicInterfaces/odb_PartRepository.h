//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_PartRepository_h
#define odb_PartRepository_h

// Begin local includes
#include <odb_Part.h>
#include <odb_Repository.h>
#include <cow_COW.h>

class ddb_Model;
COW_COW_DECL(ddb_Model);


ODB_NEWCONTAINER_DECL(odb_String, odb_Part, odb_PartRepository);

class odb_PartRepository: public odb_PartRepositoryIMPL
{

public:
    odb_PartRepository();
    odb_PartRepository(const ddb_Model& ddb);
    
    odb_Part& Part(const odb_String& name,
                   odb_Enum::odb_DimensionEnum space,
                   odb_Enum::odb_PartTypeEnum type);


private:
    ddb_ModelCOW m_modelCOW;
};

COW_COW_DECL(odb_PartRepository);

typedef odb_PartRepository odb_DiscretePartRepository; // for backward compatibility

#endif // #ifndef odb_PartRepository_h

