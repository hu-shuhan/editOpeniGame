//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_DatumCsysRepository_h
#define odb_DatumCsysRepository_h

// Begin local includes
#include <odb_DatumCsys.h>
#include <odb_Repository.h>
#include <odb_Enum.h>
#include <odb_Util.h>
#include <cow_COW.h>

class odb_String;
class odb_Node;
class odb_DatumCsys;
class ddb_Model;
COW_COW_DECL(ddb_Model);


ODB_NEWCONTAINER_DECL(odb_String, odb_DatumCsys, odb_DatumCsysRepository);

class odb_DatumCsysRepository: public odb_DatumCsysRepositoryIMPL
{

public:
    odb_DatumCsysRepository();
    odb_DatumCsysRepository(const ddb_Model& ddb);

    odb_DatumCsys& DatumCsysByThreePoints(const odb_String &,
					  const odb_Enum::odb_DatumCsysTypeEnum,
					  const odb_SequenceFloat& origin,
					  const odb_SequenceFloat& point1,
					  const odb_SequenceFloat& point2);
    // the following method exists becos.. input file & /viewer 
    // differ in interpretation of input data...
    odb_DatumCsys& odbDatumCsysByThreePoints( const odb_String &,
                                              const odb_Enum::odb_DatumCsysTypeEnum,
                                              const odb_SequenceFloat& origin,
                                              const odb_SequenceFloat& point1,
                                              const odb_SequenceFloat& point2 );
    odb_DatumCsys& DatumCsysByThreeNodes(const odb_String &,
					 const odb_Enum::odb_DatumCsysTypeEnum,
					 const odb_Node& origin,
					 const odb_Node& point1,
					 const odb_Node& point2);
    odb_DatumCsys& DatumCsysBy6dofNode(const odb_String &,
				       const odb_Enum::odb_DatumCsysTypeEnum,
				       const odb_Node& origin);
    odb_DatumCsys& DatumCsysByThreeCircNodes(const odb_String &,
					     const odb_Enum::odb_DatumCsysTypeEnum,
					     const odb_Node&,
					     const odb_Node&,
					     const odb_Node&);
    odb_DatumCsys& DatumCsys(const odb_String &, const odb_DatumCsys&);
    void DeleteDatumCsys(const odb_String &);

private:
    ddb_ModelCOW m_modelCOW;
};

COW_COW_DECL(odb_DatumCsysRepository);


#endif // #ifndef odb_DatumCsysRepository_h


