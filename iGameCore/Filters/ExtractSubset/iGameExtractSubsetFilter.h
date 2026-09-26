#ifndef iGameExtractSubsetFilter_h
#define iGameExtractSubsetFilter_h

#include "iGameFilter.h"
#include "iGameStructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class ExtractSubsetFilter : public Filter {
public:
    I_OBJECT(ExtractSubsetFilter);
    static Pointer New() { return new ExtractSubsetFilter; }

    // VOI 参数统一使用 int 类型，与 .cpp 中的局部变量以及 GUI 端的 getInt()
    // 调用保持一致，避免 igIndex / int 混用带来的类型不一致。
    void SetVOI(int minI, int maxI, int minJ, int maxJ, int minK, int maxK) {
        m_VOI[0] = minI;
        m_VOI[1] = maxI;
        m_VOI[2] = minJ;
        m_VOI[3] = maxJ;
        m_VOI[4] = minK;
        m_VOI[5] = maxK;
    }

    void SetVOI(int voi[6]) {
        std::copy(voi, voi + 6, m_VOI);
    }

    void GetVOI(int voi[6]) {
        std::copy(m_VOI, m_VOI + 6, voi);
    }

    bool Execute() override;

protected:
    ExtractSubsetFilter()
    {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
        m_VOI[0] = m_VOI[2] = m_VOI[4] = 0;
        m_VOI[1] = m_VOI[3] = m_VOI[5] = -1;
    }
    ~ExtractSubsetFilter() override = default;

    int m_VOI[6];
    StructuredMesh::Pointer m_InputMesh{};
    StructuredMesh::Pointer m_OutputMesh{};
};
IGAME_NAMESPACE_END
#endif
