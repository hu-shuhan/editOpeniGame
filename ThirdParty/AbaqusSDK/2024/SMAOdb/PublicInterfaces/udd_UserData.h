//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef udd_UserData_h
#define udd_UserData_h

// Begin local includes
#include <cow_Map.h>
#include <udd_UserXYDataContainer.h>
#include <ddr_UserData.h>
#include <udd_UserDataShortcut.h>
#include <cls_Uid.h>
#include <typ_typeTag.h>
#include <annC_AnnotationContainer.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

// Forward declarations
class udd_UserData : public ddr_UserData
{
public:
     udd_UserData();
     ~udd_UserData();

//     bool operator==(const udd_UserData&) const;
//     bool operator!=(const udd_UserData&) const;

     udd_UserXYData&       userXYData( const cow_String& userXYDataName);
     const udd_UserXYData& constUserXYData( const cow_String& userXYDataName ) const;

     udd_UserXYDataContainer&       userXYDatas(); 
     const udd_UserXYDataContainer& constUserXYDatas() const; 

     annC_AnnotationContainer&       annotations(); 
     const annC_AnnotationContainer& constAnnotations() const; 

     void UserXYData( const cow_String& xyDataName );

     cow_String name() const { return userDataName; }

     virtual ddr_UserData* Copy() const;

     // For dynamic casting only
     virtual typ_typeTag DynTypeId() const;
     static typ_typeTag  TypeId();

public: // Database interface
    udd_UserData(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;


private:
    cow_String               userDataName;
    udd_UserXYDataContainer  userXYDataMap;
    annC_AnnotationContainer annotationContainer;
    cls_Uid                  m_ClsUid;  
};


#endif
