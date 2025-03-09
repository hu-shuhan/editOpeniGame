/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: odiK_BaseInstance.h
//
// Description:
//

#ifndef odiK_BaseInstance_h
#define odiK_BaseInstance_h

// Begin Local includes
#include <mem_AllocationOperators.h>

#include <cow_String.h>
#include <cow_ListFloat.h>
#include <cow_ArrayCOW.h>

#include <odiK_ModelShortcut.h>
#include <odiK_AssemblyShortcut.h>
#include <odiK_BaseInstanceShortcut.h>

#include <cls_Uid.h>
#include <cls_xp1DArrayListFloat.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

class odiK_PartInstance;
class odiK_AssemblyInstance;

// Class definition for odiK_BaseInstance

class odiK_BaseInstance : public mem_AllocationOperators
{
  public:

    virtual ~odiK_BaseInstance();
    virtual odiK_BaseInstance* Copy() const;

    const cow_String& InstanceName() const { return instanceName; }
    const cow_String& SourceName() const { return sourceName; }
    const cow_ListFloat& Transform() const { return transform; }

    const odiK_ModelShortcut& ModelShortcut() const { return modelSC; }

    const odiK_AssemblyShortcut& AssemblyShortcut() const { return assemblySC; }

    const odiK_BaseInstanceShortcut& InstanceShortcut() const { 
	return instanceSC; 
    }

    virtual bool IsPartInstance() const;

    // These will throw exceptions if the instance is not the
    // correct type.

    const odiK_PartInstance& ConstPartInstance() const;
    const odiK_AssemblyInstance& ConstAssemblyInstance() const;

    static typ_typeTag TypeId();
    virtual typ_typeTag DynTypeId() const;
    virtual cow_String TypeName() const;

  public: // Database interface

    odiK_BaseInstance(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void Dtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

  protected:

    odiK_BaseInstance(const odiK_ModelShortcut& modelSC,
		      const odiK_AssemblyShortcut& assemblyParent,
		      const cow_String& instanceName,
		      const cow_String& sourceName,
		      const cow_ListFloat& transform);
    odiK_BaseInstance(const odiK_BaseInstance&);

    // Model that holds rootAssembly, assemblyContainer & meshContainer.
    // Also provides database file index needed for API calls.
    odiK_ModelShortcut modelSC;

    // Assembly that holds this instance.
    odiK_AssemblyShortcut assemblySC;

    // Shortcut to this instance
    odiK_BaseInstanceShortcut instanceSC;

    // Name of this instance
    cow_String instanceName;

    // Name of source (Mesh or Assembly)
    cow_String sourceName;

    // Transform to apply to source
    cls_xp1DArrayListFloat transform;

  private:

    cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(odiK_BaseInstance, cow_Virtual);

#endif // #ifndef odiK_BaseInstance_h
