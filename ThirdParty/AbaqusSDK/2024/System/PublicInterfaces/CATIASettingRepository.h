// COPYRIGHT DASSAULT SYSTEMES 2005
/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */
#ifndef CATIASettingRepository_h
#define CATIASettingRepository_h

#include "CATCORBABoolean.h"
#include "CAT_VARIANT_BOOL.h"
#include "JS0SETT.h"


#include "CATBSTR.h"
#include "CATIASettingController.h"
#include "CATSafeArray.h"
#include "CATVariant.h"

extern ExportedByJS0SETT IID IID_CATIASettingRepository;

class ExportedByJS0SETT CATIASettingRepository : public CATIASettingController
{
    CATDeclareInterface;

public:

    virtual HRESULT __stdcall GetAttr(const CATBSTR & iAttrName, CATVariant & oAttr)=0;

    virtual HRESULT __stdcall PutAttr(const CATBSTR & iAttrName, const CATVariant & iAttr)=0;

    virtual HRESULT __stdcall GetAttrArray(const CATBSTR & iAttrName, CATSafeArrayVariant *& oArray)=0;

    virtual HRESULT __stdcall PutAttrArray(const CATBSTR & iAttrName, const CATSafeArrayVariant & iArray)=0;

    virtual HRESULT __stdcall SetAttrLock(const CATBSTR & iAttrName, CAT_VARIANT_BOOL iLocked)=0;

    virtual HRESULT __stdcall GetAttrInfo(const CATBSTR & iAttrName, CATBSTR & AdminLevel, CATBSTR & Locked, CAT_VARIANT_BOOL & oModified)=0;


};

CATDeclareHandler(CATIASettingRepository, CATIASettingController);

#include "CATBaseDispatch.h"
#include "CATBaseUnknown.h"
#include "CATIABase.h"
#include "IDispatch.h"
#include "IUnknown.h"



#endif
