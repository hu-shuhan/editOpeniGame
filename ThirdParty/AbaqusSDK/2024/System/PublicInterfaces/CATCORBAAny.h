/* -*-c++-*- */
#ifndef _CATCORBAAny_h
#define _CATCORBAAny_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


// COPYRIGHT DASSAULT SYSTEMES 2000

class any;
class CATTypeCode;
class CATBaseDispatch;

#include "JS0CTYP.h"

#include "CATCORBATypes.h"

//#include "CATCORBASequence.h"

//#include "CATTypUtilit.h"

#include "CATBaseUnknown.h"
#include "CATBaseDispatch.h"

#include "CATIIRBase.h"

#include "CATTypeCode.h"

ExportedByJS0CTYP CATTypeCode * BuildTypeEnum(const char *);
ExportedByJS0CTYP CATTypeCode * BuildTypeArray(CATTypeCode * tpcode, long longueur);

class ExportedByJS0CTYP any : public CATBaseUnknown
{
  CATDeclareClass;
public:
  any();
  
  // TCKind should be CATTypeCode_ptr
  any(CATTypeCode * tc, void* value, boolean release = FALSE);
  
  ~any();
  any(const any&);
  any& operator = (const any&);

  any(short);
  any(unsigned short);
  any(long);
  any(unsigned long);
  any(double);
  any(float);
  any(const char*);
  any(CATBaseDispatch*);

  void operator <<=(short);
  void operator <<=(unsigned short);
  void operator <<=(long);
  void operator <<=(unsigned long);
  void operator <<=(float);
  void operator <<=(double);
  void operator <<=(const any&);
  void operator <<=(const char *);
  void operator <<=(CATBaseDispatch *);
//  void operator <<=(const SEQUENCE_any&);
  
  boolean operator >>=(short &) const;

  boolean operator >>=(unsigned short &) const;
  boolean operator >>=(long &) const;
  boolean operator >>=(unsigned long &) const;
  boolean operator >>=(float &) const;
  boolean operator >>=(double &) const;
  boolean operator >>=(any&) const;
  boolean operator >>=(char *&) const;
  boolean operator >>=(CATBaseDispatch *&);
//  boolean operator >>=( SEQUENCE_any& ) const;


  operator CATBaseDispatch* () const;
  
  // pour discriminer boolean, octet et char
  //--------------------------------------------
  struct from_boolean 
    {from_boolean(boolean b): val(b) {};
     boolean val;};
  struct from_octet
    {from_octet(octet b): val(b) {};
     octet val;};
  struct from_char 
    {from_char(char b): val(b) {};
     char val;};
  void operator <<=(from_boolean);
  void operator <<=(from_octet);
  void operator <<=(from_char);

  // pour discriminer boolean, octet et char
  //--------------------------------------------
  struct to_boolean 
    {to_boolean(boolean& b): ref(b) {};
     boolean& ref;};
  struct to_octet
    {to_octet(octet& b): ref(b) {};
     octet& ref;};
  struct to_char 
    {to_char(char& b): ref(b) {};
     char& ref;};
  boolean operator >>=(to_boolean&) const;
  boolean operator >>=(to_octet&) const;
  boolean operator >>=(to_char&) const;

  // operateurs de comparaison
 boolean operator ==(any &) const;
 boolean operator !=(any &) const;


  void         replace (CATTypeCode * tc, void* value, boolean release = FALSE);
  CATTypeCode *   type    ()  const;
  const void * value   ()  const;

  private:
  // to avoid mismatch use of char
  // instead of one of boolean, octet and char
  //------------------------------------------
//  void operator <<(unsigned char) {}
//  void operator >>(unsigned char&) const{}

  // attributes (TCKind should be CATTypeCode_ptr)
  //------------------------------------------
  CATTypeCode *     _typ;
  void *         _val;
  unsigned char  _release_mod;

  void ReleaseData();
};


#endif


