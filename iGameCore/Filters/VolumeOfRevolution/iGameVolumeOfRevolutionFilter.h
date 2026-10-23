#ifndef iGameVolumeOfRevolution_h
#define iGameVolumeOfRevolution_h

#include "iGameDataObject.h"
#include "iGameFilter.h"
#include <string>

IGAME_NAMESPACE_BEGIN


class VolumeOfRevolutionFilter : public Filter {
public:
    I_OBJECT(VolumeOfRevolutionFilter);
    static Pointer New() { return new VolumeOfRevolutionFilter; }

    void SetAxis(const Vector3d& dir, const Vector3d& point) {
        m_AxisDirection = dir;
        m_AxisPoint = point;
    }
    void SetResolution(int res) { m_Resolution = (res < 3) ? 3 : res; }
    void SetAngle(double angle) { m_Angle = angle; }

    int GetResolution() const { return m_Resolution; }
    double GetAngle() const { return m_Angle; }

    bool Execute() override;

    // 失败原因；成功但跳过了不支持的单元时给出非致命说明。
    // 空字符串表示上一次执行没有可报告的失败原因。
    const std::string& GetMessage() const { return m_Message; }

protected:
    VolumeOfRevolutionFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~VolumeOfRevolutionFilter() override = default;

private:
    int m_Resolution = 12; 
    double m_Angle = 2.0 * 3.141592653589793;
    Vector3d m_AxisDirection{0.0, 0.0, 1.0}; // 轴向方向，默认 (0,0,1)
    Vector3d m_AxisPoint{0.0, 0.0, 0.0};     // 轴上一点，默认 (0,0,0)

    // 记录非致命提示与致命失败原因
    std::string m_Message;
};

IGAME_NAMESPACE_END

#endif
