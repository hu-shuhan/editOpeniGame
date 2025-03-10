//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SectionCategoryRepository_h
#define odb_SectionCategoryRepository_h

// Begin local includes
#include <odb_Types.h>
#include <odb_SectionCategory.h>
#include <odb_Repository.h>
#include <cow_COW.h>


ODB_NEWCONTAINER_DECL(odb_String, odb_SectionCategory, odb_SectionCategoryRepository);

class odb_SectionCategoryRepository: public odb_SectionCategoryRepositoryIMPL
{

public:
    odb_SectionCategoryRepository();
    odb_SectionCategory& SectionCategory(const odb_String& name,
                                         const odb_String& description);

};

COW_COW_DECL(odb_SectionCategoryRepository);


#endif // #ifndef odb_SectionCategoryRepository_h

