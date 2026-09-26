#ifndef iGameMergeVectorComponentsFilter_h
#define iGameMergeVectorComponentsFilter_h

#include "iGameFilter.h"
#include "iGameFlatArray.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class MergeVectorComponentsFilter : public Filter {
public:
    I_OBJECT(MergeVectorComponentsFilter);
    static Pointer New() { return new MergeVectorComponentsFilter; }

    bool Execute() override;

    // Names of scalar arrays to merge; order is component order, count N = output dimension
    void SetComponentArrayNames(const std::vector<std::string>& names) { m_ComponentNames = names; }
    const std::vector<std::string>& GetComponentArrayNames() const { return m_ComponentNames; }

    // Output vector name; if empty defaults to "vector"
    void SetOutputVectorName(const std::string& name) { m_OutputName = name; }
    const std::string& GetOutputVectorName() const { return m_OutputName; }

    // Attachment: IG_POINT (point data) or IG_CELL (cell data); default IG_POINT
    void SetAttachmentType(IGenum type) { m_AttachmentType = type; }
    IGenum GetAttachmentType() const { return m_AttachmentType; }

    std::string GetMessage() const { return m_Message; }

protected:
    MergeVectorComponentsFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~MergeVectorComponentsFilter() override = default;

    // Create an empty array of the same concrete type (for type preservation);
    // unknown/mixed types fall back to DoubleArray in the caller
    static ArrayObject::Pointer CreateArrayOfType(IGenum type);

    std::vector<std::string> m_ComponentNames;
    std::string m_OutputName;
    IGenum m_AttachmentType{IG_POINT};
    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif
