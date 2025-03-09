//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef mdl_ExtensionSubject_h
#define mdl_ExtensionSubject_h

// Begin Local Includes
#include <typ_typeTag.h>
#include <cow_ListString.h>
#include <cls_Uid.h>

class mdl_Extension;
class cow_String;
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

/// The template parameter does not do anything. except distinguish the
/// type.
template <class T, class Table> class mdl_ExtensionSubject  : public mem_AllocationOperators
{
  public:
    mdl_ExtensionSubject() {}
    
    Table& ConstGetInstanceTable() {
        return instanceTable;}

    /// Interface:
    static void RegisterExtension(const cow_String &name,
				  const mdl_Extension &);
    /// Register a prototype instance of an extension with a given name. 
    /// when the "Get" methods are called to retrieve the extenstion, if the
    /// extension does not exist in the instance table, the prototype
    /// will be copied in to the instance table.

    static void UnRegisterExtension(const cow_String &name);
    /// Remove the regisrtaion of an extension with a given name. 
    /// This method must be called for each extension be for the
    /// Extension Subject is Finalized. If this is not called it is possible
    /// that the code for the extension object will be unloaded 
    /// (during shutdown) befor the registration table is destroyed. If this
    /// happens the destructor of the regisration table will core dump.

    const mdl_Extension& ConstGetExtension(const cow_String &name) const;
    /// Return a const reference to the Extension with the given name.
    /// If an extension does not exist with the name, and exception is
    /// is thrown.

    mdl_Extension& GetExtension(const cow_String &name);
    /// Return a reference to the Extension with the given name.
    /// If an extension does not exist with the name, and exception is
    /// is thrown.

    inline void SetExtension(const cow_String &name,const mdl_Extension &ext) {
        instanceTable.Insert(name,ext); }
    /// put a new extension in the instance table.

    cow_ListString RegisteredExtensionList();
    /// Returns the list of the names of the registered extensions.

    inline cow_ListString InstanceExtensionList() {
        return instanceTable.Keys(); }
    /// Returns the list of the names extensions with actual instances.
    

    static void Initialize();
    static void Finalize();

    /// Messaging/database interface
    /// This template is intended to be used in HAS-A relationships
    /// to standardize the implementation of objects that can be extended
    /// not in IS-A relationships.  This object should never 
    /// be used as a base class, therefore the database interface is
    /// restricted to the read/write interface only, not the ISA and
    /// DynType interface or typetable functions
    static typ_typeTag      TypeId();

  protected:

    static Table *registrationTable;
    Table        instanceTable;

public:

    // Messaging/Database interface
    mdl_ExtensionSubject( const cls_ReadVisitor & );
    virtual void DBWrite( const cls_WriteVisitor & ) const;
    static void InitializeMetadata( cls_ClassRegistrar & );

private:

    static const char *DBClassName();
    cls_Uid m_ClsUid;
};

#define MDL_EXTENSIONSUBJECT_DECL(Type1, Table, Name)\
    typedef mdl_ExtensionSubject<Type1, Table> Name

#define MDL_EXTENSIONSUBJECT_IMPL(Type1, Table, Name)                   \
    template <> typ_typeTag mdl_ExtensionSubject<Type1,Table>::TypeId() { return TYP_TIX_ ## Name; } \
    template <> const char *mdl_ExtensionSubject<Type1,Table>::DBClassName() { return #Name; } \
    CSL_TYPEADAPTER_SPECIALIZE(Name);                                   \
    template class mdl_ExtensionSubject<Type1,Table>;                   \
    template class cls_AbsTypeAdapter<Name>;                            \
    template class cls_TypeAdapter<Name>

#endif
