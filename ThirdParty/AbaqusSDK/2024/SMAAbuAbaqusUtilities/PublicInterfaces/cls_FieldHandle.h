//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_FieldHandle_h
#define cls_FieldHandle_h
//
// Description:
//       
//
// Modification history:
//      Rob deFriesse 11/2000 original
//
////////////////////////////////////////////////////////////////////////// */

// begin local includes
#include <mem_AllocationOperators.h>

#include <cow_String.h>

// end local includes

class cls_FieldHandle  : public mem_AllocationOperators
{
public:
  cls_FieldHandle() : m_FieldID(-1), m_FieldName() {};
  cls_FieldHandle(int fieldID) : m_FieldID(fieldID), m_FieldName() {};
  cls_FieldHandle(const cow_String& fieldName, int fieldID = -1) : m_FieldID(fieldID), m_FieldName(fieldName) {};
  cls_FieldHandle(const char* fieldName, int fieldID = -1) : m_FieldID(fieldID), m_FieldName(fieldName) {};

  int ID() const { return m_FieldID; };
  const cow_String& Name() const { return m_FieldName; };

private:
  int m_FieldID;
  cow_String m_FieldName;
};

#endif
