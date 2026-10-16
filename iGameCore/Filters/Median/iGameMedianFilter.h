/**
 * @class MedianFilter
 * @brief 结构化网格点标量的中值滤波（对标 ParaView 的 Median 滤波器）。
 *
 * 对 StructuredMesh 的 IG_POINT 单分量标量做核窗口内邻域中值替换：
 *  - 只处理单分量标量：多分量请先提取/选择分量；
 *  - 输出数组与输入同名、同类型（signed char 仍是 signed char），不改几何、不做归一化；
 *  - 核大小各维取奇数，默认 3x3x3，2D 数据第三维固定为 1；
 *  - 邻域只取输入 extent 内的点，越界直接跳过（不补零、不复制边界）；
 *  - 候选按原类型升序排序后取 count/2（0-based，上中位数）。
 */

#ifndef iGameMedianFilter_h
#define iGameMedianFilter_h

#include "iGameFilter.h"
#include "iGameStructuredMesh.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class MedianFilter : public Filter {
public:
    I_OBJECT(MedianFilter);
    static Pointer New() { return new MedianFilter; }

    bool Execute() override;

    void SetAttributeByIndex(int index) { m_AttributeIndex = index; }
    void SetAttributeByName(const std::string& name) { m_AttributeName = name; }
    void SetKernelSize(int kx, int ky, int kz) {
        m_KernelSize[0] = kx;
        m_KernelSize[1] = ky;
        m_KernelSize[2] = kz;
    }

    // 执行结果提示：每次 Execute() 开头清空，失败时写入具体原因，成功时为空字符串。
    std::string GetMessage() const { return m_Message; }

protected:
    MedianFilter();
    ~MedianFilter() override = default;

private:
    int m_AttributeIndex{-1};
    std::string m_AttributeName;
    int m_KernelSize[3]{3, 3, 3};
    std::string m_Message{}; // 失败原因；成功时为空
};

IGAME_NAMESPACE_END
#endif
