#ifndef __TIE_CATIASettingController
#define __TIE_CATIASettingController

#include <string.h>
#include "CATBaseUnknown.h"
#include "CATMetaClass.h"
#include "CATMacForTie.h"
#include "CATIASettingController.h"
#include "JS0DSPA.h"
#include "DSYExport.h"


#define Exported DSYExport
#define Imported DSYImport


/* To link an implementation with the interface CATIASettingController */
#define declare_TIE_CATIASettingController(classe,TIE_Version) \
 \
      CATForwardDeclareTemplateFunctionSpecialization_##TIE_Version(classe) \
 \
class TIECATIASettingController##classe : public CATIASettingController \
{ \
   private: \
      CATDeclareCommonTIEMembers2 \
   public: \
      CATDeclareTIEMethods(CATIASettingController, classe) \
      CATDeclareIUnknownMethodsForCATBaseUnknownTIE \
      CATDeclareIDispatchMethodsForCATBaseUnknownTIE \
      CATDeclareCATBaseUnknownMethodsForTIE \
      virtual HRESULT __stdcall Commit(); \
      virtual HRESULT __stdcall Rollback(); \
      virtual HRESULT __stdcall ResetToAdminValues(); \
      virtual HRESULT __stdcall ResetToAdminValuesByName(const CATSafeArrayVariant & iAttList); \
      virtual HRESULT __stdcall SaveRepository(); \
      virtual HRESULT  __stdcall get_Application(CATIAApplication *& oApplication); \
      virtual HRESULT  __stdcall get_Parent(CATBaseDispatch *& oParent); \
      virtual HRESULT  __stdcall get_Name(CATBSTR & oNameBSTR); \
      virtual HRESULT  __stdcall put_Name(const CATBSTR & iNameBSTR); \
      virtual HRESULT  __stdcall GetItem(const CATBSTR & IDName, CATBaseDispatch *& RealObj); \
};



#define ENVTIEdeclare_CATIASettingController(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
virtual HRESULT __stdcall Commit(); \
virtual HRESULT __stdcall Rollback(); \
virtual HRESULT __stdcall ResetToAdminValues(); \
virtual HRESULT __stdcall ResetToAdminValuesByName(const CATSafeArrayVariant & iAttList); \
virtual HRESULT __stdcall SaveRepository(); \
virtual HRESULT  __stdcall get_Application(CATIAApplication *& oApplication); \
virtual HRESULT  __stdcall get_Parent(CATBaseDispatch *& oParent); \
virtual HRESULT  __stdcall get_Name(CATBSTR & oNameBSTR); \
virtual HRESULT  __stdcall put_Name(const CATBSTR & iNameBSTR); \
virtual HRESULT  __stdcall GetItem(const CATBSTR & IDName, CATBaseDispatch *& RealObj); \


#define ENVTIEdefine_CATIASettingController(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
HRESULT __stdcall  ENVTIEName::Commit() \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)Commit()); \
} \
HRESULT __stdcall  ENVTIEName::Rollback() \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)Rollback()); \
} \
HRESULT __stdcall  ENVTIEName::ResetToAdminValues() \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)ResetToAdminValues()); \
} \
HRESULT __stdcall  ENVTIEName::ResetToAdminValuesByName(const CATSafeArrayVariant & iAttList) \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)ResetToAdminValuesByName(iAttList)); \
} \
HRESULT __stdcall  ENVTIEName::SaveRepository() \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)SaveRepository()); \
} \
HRESULT  __stdcall  ENVTIEName::get_Application(CATIAApplication *& oApplication) \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)get_Application(oApplication)); \
} \
HRESULT  __stdcall  ENVTIEName::get_Parent(CATBaseDispatch *& oParent) \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)get_Parent(oParent)); \
} \
HRESULT  __stdcall  ENVTIEName::get_Name(CATBSTR & oNameBSTR) \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)get_Name(oNameBSTR)); \
} \
HRESULT  __stdcall  ENVTIEName::put_Name(const CATBSTR & iNameBSTR) \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)put_Name(iNameBSTR)); \
} \
HRESULT  __stdcall  ENVTIEName::GetItem(const CATBSTR & IDName, CATBaseDispatch *& RealObj) \
{ \
return (ENVTIECALL(CATIASettingController,ENVTIETypeLetter,ENVTIELetter)GetItem(IDName,RealObj)); \
} \


/* Name of the TIE class */
#define class_TIE_CATIASettingController(classe)    TIECATIASettingController##classe


/* Common methods inside a TIE */
#define common_TIE_CATIASettingController(classe,TIE_Version) \
 \
 \
/* Static initialization */ \
CATDefineCommonTIEMembers2(CATIASettingController, classe) \
 \
 \
CATImplementTIEMethods(CATIASettingController, classe) \
CATImplementIUnknownMethodsForCATBaseUnknownTIE(CATIASettingController, classe, 2) \
CATImplementIDispatchMethodsForCATBaseUnknownTIE(CATIASettingController, classe) \
CATImplementCATBaseUnknownMethodsForTIE(CATIASettingController, classe) \
 \
HRESULT __stdcall  TIECATIASettingController##classe::Commit() \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->Commit()); \
} \
HRESULT __stdcall  TIECATIASettingController##classe::Rollback() \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->Rollback()); \
} \
HRESULT __stdcall  TIECATIASettingController##classe::ResetToAdminValues() \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->ResetToAdminValues()); \
} \
HRESULT __stdcall  TIECATIASettingController##classe::ResetToAdminValuesByName(const CATSafeArrayVariant & iAttList) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->ResetToAdminValuesByName(iAttList)); \
} \
HRESULT __stdcall  TIECATIASettingController##classe::SaveRepository() \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->SaveRepository()); \
} \
HRESULT  __stdcall  TIECATIASettingController##classe::get_Application(CATIAApplication *& oApplication) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->get_Application(oApplication)); \
} \
HRESULT  __stdcall  TIECATIASettingController##classe::get_Parent(CATBaseDispatch *& oParent) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->get_Parent(oParent)); \
} \
HRESULT  __stdcall  TIECATIASettingController##classe::get_Name(CATBSTR & oNameBSTR) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->get_Name(oNameBSTR)); \
} \
HRESULT  __stdcall  TIECATIASettingController##classe::put_Name(const CATBSTR & iNameBSTR) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->put_Name(iNameBSTR)); \
} \
HRESULT  __stdcall  TIECATIASettingController##classe::GetItem(const CATBSTR & IDName, CATBaseDispatch *& RealObj) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->GetItem(IDName,RealObj)); \
} \



/* Macro used to link an implementation with an interface */
#define Real_TIE_CATIASettingController(classe,TIE_Version) \
 \
 \
declare_TIE_CATIASettingController(classe,TIE_Version) \
 \
 \
common_TIE_CATIASettingController(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIECreation(CATIASettingController, classe) \
 \
 \
CATImplementTIEMeta(CATIASettingController, classe, ENUMTypeOfClass::TIE, CATIASettingController::MetaObject(), CATIASettingController::MetaObject())


/* Macro used to link an implementation with an interface */
/* This TIE is chained on the implementation object */
#define Real_TIEchain_CATIASettingController(classe,TIE_Version) \
 \
 \
declare_TIE_CATIASettingController(classe,TIE_Version) \
 \
 \
common_TIE_CATIASettingController(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIEchainCreation(CATIASettingController, classe) \
 \
 \
CATImplementTIEMeta(CATIASettingController, classe, ENUMTypeOfClass::TIEchain, CATIASettingController::MetaObject(), CATIASettingController::MetaObject())

/* Macro to switch between BOA and TIE at build time */ 
#ifdef CATSYS_BOA_IS_TIE
#define BOA_CATIASettingController(classe) TIE_CATIASettingController(classe)
#else
#define BOA_CATIASettingController(classe) CATImplementBOA(CATIASettingController, classe)
#endif


/* Macros used to link an implementation with an interface */
#define TIE_Deprecated_CATIASettingController(classe) Real_TIE_CATIASettingController(classe,TIEV1)
#define TIEchain_Deprecated_CATIASettingController(classe) Real_TIEchain_CATIASettingController(classe,TIEV1) 
#define TIE_CATIASettingController(classe) Real_TIE_CATIASettingController(classe,TIEV2)
#define TIEchain_CATIASettingController(classe) Real_TIEchain_CATIASettingController(classe,TIEV2) 

#endif
