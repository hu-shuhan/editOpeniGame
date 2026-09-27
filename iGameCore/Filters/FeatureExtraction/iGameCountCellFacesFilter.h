#ifndef iGameCountCellFacesFilter_h
#define iGameCountCellFacesFilter_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class CountCellFacesFilter : public Filter {
public:
    I_OBJECT(CountCellFacesFilter);
    static Pointer New() { return new CountCellFacesFilter; }

    static constexpr const char* ResultAttributeName = "cellFaceCounts";

    bool Execute() override;
    UnsignedIntArray::Pointer GetResult() const { return m_FaceCounts; }
    std::string GetMessage() const { return m_Message; }

protected:
    CountCellFacesFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~CountCellFacesFilter() override = default;

private:
    bool ExecuteInternal();

    UnsignedIntArray::Pointer m_FaceCounts{};
    std::string m_Message{"Not executed."};
};

IGAME_NAMESPACE_END
#endif
