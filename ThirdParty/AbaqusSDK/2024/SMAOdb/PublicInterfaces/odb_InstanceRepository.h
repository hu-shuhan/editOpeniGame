//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_InstanceRepository_h
#define odb_InstanceRepository_h

// Begin local includes
#include <odb_Instance.h>
#include <odb_Repository.h>
#include <odb_MeshList.h>
#include <odb_Types.h>
#include <cow_COW.h>

class ddb_Model;
COW_COW_DECL(ddb_Model);

class odb_Instance;
class odb_Part;

ODB_NEWCONTAINER_DECL(odb_String, odb_Instance, odb_InstanceRepository);

class odb_InstanceRepository: public odb_InstanceRepositoryIMPL
{

public:
    odb_InstanceRepository();
    odb_InstanceRepository(const ddb_Model& ddb);
    bool isMember( const odb_String& name ) const { return odb_InstanceRepositoryIMPL::isMember(name); }
    int size() const { return odb_InstanceRepositoryIMPL::size(); }
    odb_Instance& Instance(const odb_String& name,
                           const odb_Part& part,
                           const odb_SequenceSequenceFloat& transForm =
                                    odb_SequenceSequenceFloat());    

private:
    ddb_ModelCOW m_modelCOW;
    odb_MeshList           m_meshList;

public:
    const odb_MeshList& meshList() const;
    odb_MeshList& getMeshList(); 

};

COW_COW_DECL(odb_InstanceRepository);


#endif // #ifndef odb_InstanceRepository_h


