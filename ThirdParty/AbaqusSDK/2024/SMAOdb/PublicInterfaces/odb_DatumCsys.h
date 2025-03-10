//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_DatumCsys_h
#define odb_DatumCsys_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_String.h>
#include <odb_Enum.h>
#include <cow_COW.h>
// End local includes

// Forward Declarations
class odb_FieldOutput;
class dtu_DatumCsys;
class ddb_DatumCsys;
COW_COW_DECL(ddb_DatumCsys); 

class odb_DatumCsys  : public mem_AllocationOperators
{
 public:
  // documented and supported
  odb_DatumCsys();
  odb_DatumCsys(const odb_String& name, 
		const ddb_DatumCsys& csys);
  
  const odb_String& name() const { return m_name; };
  odb_Enum::odb_DatumCsysTypeEnum type() const;
  const float* xAxis() const;
  const float* yAxis() const;
  const float* zAxis() const;
  const float* origin() const;

  // undocumented and unsupported
  dtu_DatumCsys GetDatumCsys( ) const;
  dtu_DatumCsys GetDatumCsys( odb_FieldOutput& displacements ) const;
  dtu_DatumCsys GetDatumCsys( odb_FieldOutput& displacements,
                              odb_FieldOutput& rotations ) const;

  ddb_DatumCsys datumCsys() const;
  bool hasValue() const;
  void summaryValue1( odb_String & ) const;
  void summaryValue2( odb_String & ) const;

 private:
  odb_String m_name;
  ddb_DatumCsysCOW  m_datumCsys;
  float x[3];
  float y[3];
  float z[3];
  float o[3];
};

#endif // odb_DatumCsys_h
