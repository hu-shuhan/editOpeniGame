#ifndef iGameReverseSenseFilter_h
#define iGameReverseSenseFilter_h

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

// 反转面朝向过滤器：
//   1) ReverseCells 打开时，反转每个面的顶点环序，从而翻转面法向；
//   2) ReverseNormals 打开时，把点/单元法向数组（IG_NORMAL）逐分量取反。
// 目前仅支持 SurfaceMesh 输入，输出为一份独立的 SurfaceMesh（几何不变、拓扑环序反转）。
class ReverseSenseFilter : public Filter {
public:
    I_OBJECT(ReverseSenseFilter);
    static Pointer New() { return new ReverseSenseFilter; }

    // 是否反转每个面的顶点环序，默认开。
    void SetReverseCells(bool v) { m_ReverseCells = v; }
    bool GetReverseCells() const { return m_ReverseCells; }

    // 是否反转点/单元法向（IG_NORMAL），默认开。
    void SetReverseNormals(bool v) { m_ReverseNormals = v; }
    bool GetReverseNormals() const { return m_ReverseNormals; }

    bool Execute() override;

    std::string GetMessage() const { return m_Message; }

protected:
    ReverseSenseFilter();
    ~ReverseSenseFilter() override = default;

private:
    bool m_ReverseCells{true};
    bool m_ReverseNormals{true};
    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif
