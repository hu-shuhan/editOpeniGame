//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef mdl_PropertyTable_h
#define mdl_PropertyTable_h
///////////////////////////////////////////////////////////////////////////////
//
// File Name: mdl_PropertyTable
//
// Description: Property table made from a cow_2DArrayDouble
//

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cls_xp2DArrayDouble.h>
#include <cow_String.h>
#include <typ_typeTag.h>
#include <cow_COW.h>
#include <cls_IntCOW.h>

class cls_ClassRegistrar;

// Class definition

class mdl_PropertyTable : public mem_AllocationOperators
{
  public:
    mdl_PropertyTable();
    mdl_PropertyTable(const cow_2DArrayDouble& materialTable);
    mdl_PropertyTable(int row, int col);
    ~mdl_PropertyTable();
    virtual mdl_PropertyTable *Copy() const;

    const cow_2DArrayDouble& ConstGetArray() const { return table;}
    cow_2DArrayDouble& GetArray() { return table;}
    const double& ConstGet(int i, int j) const { return table.ConstGet(i,j);}
    double& Get(int i, int j) { return table.Get(i,j);}
    int Rows() const { return table.Rows();}
    int Cols() const { return table.Cols();}
    void Sort(int ndep);
    void AssertValid() const;

    void AllowEmptyFirstRow(bool aa=true) { allowEmptyFirstRow=aa; }
    bool IsEmptyFirstRowAllowed() { return allowEmptyFirstRow; }

    bool operator == (const mdl_PropertyTable&) const;
    bool operator != (const mdl_PropertyTable& rhs) const {return !(*this==rhs);}

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    // DDB Pers Interface
    mdl_PropertyTable(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    void DBWrite(const cls_WriteVisitor& wv) const;

  private:
    cls_xp2DArrayDouble table;
    bool allowEmptyFirstRow;
    cls_Uid m_ClsUid; // object identifier for odb
};

COW_COW2_DECL(mdl_PropertyTable, cow_Virtual);


#endif /* mdl_PropertyTable_h */
