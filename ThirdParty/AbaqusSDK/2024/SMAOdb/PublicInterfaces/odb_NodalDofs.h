//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_NodalDofs_h
#define odb_NodalDofs_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <kprC_RetainedDofs.h>
#include <odb_Exceptions.h>
#include <odb_MeshList.h>
#include <odb_Types.h>
// End local includes

// Forward declarations
class ddb_Assembly;
COW_ARRAYCOW_DECL(ddb_Assembly);
class odb_Set;

// Class definition

class odb_NodalDofs : public mem_AllocationOperators
{
public:
    odb_Set         nset() const;
    odb_SequenceInt dofs() const;

private:
    kprC_RetainedDofs   m_rnd;
    ddb_AssemblyCOW     m_asmb;

public: // Undocumented - internal stuff
    odb_NodalDofs(const kprC_RetainedDofs & rd,
                  const ddb_AssemblyCOW & asmb);
};

///////////////////////////////////////////////////////////////////////

class odb_SequenceNodalDofs : public mem_AllocationOperators
{
public:
    int size() const;
    bool isEmpty() const;
    odb_NodalDofs constGet(int i) const;
    odb_NodalDofs operator[](int i) const;

private:
    kprC_RetainedDofsList   m_list;
    ddb_AssemblyCOW         m_asmb;

public: // Undocumented - internal stuff
    odb_SequenceNodalDofs();
    odb_SequenceNodalDofs(const kprC_RetainedDofsList & rdl,
                          const ddb_AssemblyCOW & asmb);
};

///////////////////////////////////////////////////////////////////////

#endif // odb_NodalDofs_h
