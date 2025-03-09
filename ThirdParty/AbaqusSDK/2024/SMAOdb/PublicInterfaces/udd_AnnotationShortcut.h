//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- mode: c++ -*-
#ifndef udd_AnnotationShortcut_h
#define udd_AnnotationShortcut_h

// Begin local includes
#include <ddr_Shortcut.h>
#include <udd_UserData.h>
#include <annC_Annotation.h>
#include <annC_AnnotationContainer.h>
#include <SMABasStringMode.h>

ddr_SHORTCUT_DECL(annC_Annotation, udd_AnnotationOdbShortcut);
ddr_SHORTCUT_DECL(annC_AnnotationContainer, udd_AnnotationContainerOdbShortcut);
ddr_MEMBER_SHORTCUT_DECL(udd_UserData, annC_AnnotationContainer, udd_AnnotationContainerInUserDataShortcut);


// Member shortcut for a dictionary where objects can be referenced by ids.

class udd_AnnotationInAnnotationContainerOdbShortcut
    : public ddr_ShortcutImpl<annC_Annotation>
{
public:

    udd_AnnotationInAnnotationContainerOdbShortcut(
        const udd_AnnotationContainerOdbShortcut& p,
        unsigned id
    );

    virtual omu_ShortcutImpl* Copy() const;

    virtual const annC_Annotation& ConstGet(const ddr_DdbContainer& ddbCont) const;
    virtual annC_Annotation& Get(ddr_DdbContainer& ddbCont) const;

    annC_AnnotationContainer &GetParent(ddr_DdbContainer& ddbCont) const;
    const annC_AnnotationContainer& ConstGetParent(const ddr_DdbContainer& ddbCont) const;
    virtual cow_String Name() const;
    virtual cow_String Path() const;
    virtual cow_String DdbName(STRMODE_DECL) const;

    // Database interface
    static void* Ctor(cls_ReadVisitor& rv);
    udd_AnnotationInAnnotationContainerOdbShortcut(const cls_ReadVisitor& rv);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;
    static void InitializeMetadata(cls_ClassRegistrar& reg);

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

private:
    int m_id;
    ddr_Shortcut<annC_AnnotationContainer> m_parent;

    cls_Uid m_ClsUid;
};



#endif
