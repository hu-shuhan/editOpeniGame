#ifndef iGameTextureMapToCylinderFilter_h
#define iGameTextureMapToCylinderFilter_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class TextureMapToCylinderFilter
 * @brief 把输入网格的点映射到圆柱面上，生成 2D 纹理坐标（Point Data，类型 IG_TCOORD）。
 *
 * 语义要点：
 *   - 圆柱轴由 `Point1` / `Point2` 两点定义（默认 (0,0,-1) → (0,0,1)）。
 *     默认取覆盖常见单位模型（±1）的长度，使不手动设轴时 t 也落在 [0,1] 内，
 *     与「自动求轴把轴长贴合点云」的风格保持一致；
 *   - `AutomaticCylinderGeneration`（默认开）：自动求轴，对点云做**主成分分析**取主轴
 *     （轴对齐网格、圆柱/管类模型能稳定给出轴向；
 *     各向同性模型如立方体/球体主轴不唯一，此时建议关掉自动、手动给轴）。
 *     **注意**：自动求轴时轴的正负由主轴特征向量的符号决定，不保证朝向，
 *     因此 t 的正负（PreventSeam=false 时还有 s 的绕行方向）可能整体反向；
 *     需要完全确定的坐标朝向时，请关闭自动并手动指定 Point1/Point2。
 *   - **s 坐标（圆周方向）** ∈ [0,1]
 *     （v = 点相对轴的径向单位向量，即点减去它在轴所在直线上的垂足后归一化）：
 *       PreventSeam = true（默认）：s = acos(dot(v, ref)) / π
 *           —— 绕轴一周时 s 先 0→1（0°~180°）再 1→0（180°~360°），避免接缝处跳变；
 *       PreventSeam = false        ：s = 角度 / 2π（整圈 0→1），用 axis×(ref×v) 的符号定方向；
 *   - **t 坐标（轴向）** = 点沿轴投影的归一化参数：Point1 处为 0、Point2 处为 1（轴长会影响 t）；
 *   - **输出数组名**：`"Texture Coordinates"`，2 分量 float，挂在 Point Data。
 *
 * 【独立输出节点】
 *   不修改输入：点坐标、单元连接表、单元类型表、全部属性数组逐个深拷贝到新建的
 *   UnstructuredMesh 上，再把纹理坐标数组追加进输出的 Point Data。
 *   仅"点数据"被追加一个数组，其余一切（几何/拓扑/已有属性）保持不变。
 *   **表面网格输入**：面单元、以及"不属于任何面的独立线段"都会一并保留
 *   （线段在读文件时是存放在表面网格的边表里的，通用转换只搬面表会把它丢掉）；
 *   独立线段没有对应的原始单元数据，其单元属性按 0 补齐，长度与输出单元数一致。
 *
 * 【失败与空结果（绝不把原模型当结果返回）】
 *   - 无输入 / 输入为空指针 / 数据类型不支持：返回 false；
 *   - 输入 0 个点：返回 false（没有点就无从生成纹理坐标）；
 *   - 轴退化（Point1 与 Point2 重合，或自动求轴失败）：返回 false；
 *   以上情况均通过 GetMessage() 给出可读原因，供界面提示。
 */
class TextureMapToCylinderFilter : public Filter {

public:
    I_OBJECT(TextureMapToCylinderFilter);
    static Pointer New() { return new TextureMapToCylinderFilter; }
    bool Execute() override;

    /// 圆柱轴的第一个端点（默认 (0,0,-1)）
    void SetPoint1(double x, double y, double z);
    /// 圆柱轴的第二个端点（默认 (0,0,1)）
    void SetPoint2(double x, double y, double z);
    const double* GetPoint1() const { return m_Point1; }
    const double* GetPoint2() const { return m_Point2; }

    /// 是否自动求轴（默认 true）
    void SetAutomaticCylinderGeneration(bool on) { m_Automatic = on; }
    bool GetAutomaticCylinderGeneration() const { return m_Automatic; }

    /// 是否防止接缝（默认 true）
    void SetPreventSeam(bool on) { m_PreventSeam = on; }
    bool GetPreventSeam() const { return m_PreventSeam; }

    /// 输出数组名（默认 "Texture Coordinates"）
    void SetTCoordsArrayName(const std::string& name) { m_ArrayName = name; }
    const std::string& GetTCoordsArrayName() const { return m_ArrayName; }

    /// 本次实际使用的轴两端点（自动求轴时是算出来的结果，供界面回显/核对）
    void GetResolvedAxis(double point1[3], double point2[3]) const;

    /// 最近一次执行的信息（失败原因、提示等），供界面显示
    const std::string& GetMessage() const { return m_Message; }

protected:
    TextureMapToCylinderFilter();
    ~TextureMapToCylinderFilter() override = default;

    /// 执行主体逻辑（不含异常捕获，由 Execute 包裹）
    bool ExecuteInternal();

    double m_Point1[3] = {0.0, 0.0, -1.0};
    double m_Point2[3] = {0.0, 0.0, 1.0};
    bool m_Automatic = true;
    bool m_PreventSeam = true;
    std::string m_ArrayName = "Texture Coordinates";

    /// 实际使用的轴（自动求轴时被覆盖），GetResolvedAxis 对外暴露
    double m_Resolved1[3] = {0.0, 0.0, -1.0};
    double m_Resolved2[3] = {0.0, 0.0, 1.0};

    /// 最近一次执行的信息
    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif // iGameTextureMapToCylinderFilter_h
