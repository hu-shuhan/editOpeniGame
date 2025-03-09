//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_JobData_h
#define odb_JobData_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <odb_Enum.h>
// End local includes

class odb_JobData : public mem_AllocationOperators
{
public:
    odb_String name() const;
    odb_Enum::odb_AnalysisCodeEnum analysisCode() const;
    odb_String version() const;
    odb_String creationTime() const;
    odb_String modificationTime() const;
    odb_String machineName() const;
    odb_Enum::odb_PrecisionEnum precision() const;
    odb_SequenceProductAddOn productAddOns() const;
   
private:
    odb_String   m_name;
    odb_Enum::odb_AnalysisCodeEnum   m_analysisCode;
    odb_String   m_version;
    odb_String   m_creationTime;
    odb_String   m_modificationTime;
    odb_String   m_machineName;
    odb_Enum::odb_PrecisionEnum   m_precision;
    odb_SequenceProductAddOn m_productAddOns;

public:   // undocumented and unsupported
    odb_JobData();
    odb_JobData(const odb_String& name,
                const odb_Enum::odb_AnalysisCodeEnum code,
                const odb_String& version,
                const odb_String& creationTime,
                const odb_String& modificationTime,
                const odb_String& machineName,
                const odb_Enum::odb_PrecisionEnum precision,
		const odb_SequenceProductAddOn& productAddOns);
};

#endif // odb_JobData_h
