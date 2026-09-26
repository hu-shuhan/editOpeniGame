#ifndef iGameOutlineCornerFilter_h
#define iGameOutlineCornerFilter_h

#include "iGameBoundingBox.h"
#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class OutlineCornerFilter : public Filter {
public:
    I_OBJECT(OutlineCornerFilter);
    static Pointer New() { return new OutlineCornerFilter; }

    bool Execute() override;

    void SetCornerFactor(float factor);
    float GetCornerFactor() const { return m_CornerFactor; }

    UnstructuredMesh::Pointer GetResult() const { return m_Result; }
    std::string GetMessage() const { return m_Message; }

protected:
    OutlineCornerFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~OutlineCornerFilter() override = default;

private:
    bool ExecuteInternal();
    UnstructuredMesh::Pointer BuildResult(const BoundingBox& bounds) const;

    float m_CornerFactor{0.2f};
    UnstructuredMesh::Pointer m_Result{};
    std::string m_Message{"Not executed."};
};

IGAME_NAMESPACE_END

#endif
