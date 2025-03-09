//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_FromFile_h
#define odb_FromFile_h

// Begin local includes
#include <odb_PredefinedField.h>

// Forward declarations
class ddb_FromFile;

class odb_FromFile: public odb_PredefinedField
{
public:
    void setFileNameInStep(const odb_String& stepName, 
                     const odb_String& fileName);
    void setBeginStepInStep(const odb_String& stepName, 
                            int beginStep);    
    void setBeginIncInStep(const odb_String& stepName, 
                            int beginInc);    
    void setEndStepInStep(const odb_String& stepName, 
                            int endStep);    
    void setEndIncInStep(const odb_String& stepName, 
                            int endInc);    
    void setMidSideInStep(const odb_String& stepName, 
                            bool midside);    

private:

public:   // undocumented and unsupported

    // Needed for container
    odb_FromFile();

    odb_FromFile(const ddb_FromFile& predefinedField);
    
};

#endif // odb_FromFile_h
