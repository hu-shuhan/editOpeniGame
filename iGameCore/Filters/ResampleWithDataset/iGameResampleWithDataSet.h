#pragma once
#ifndef iGameResampleWithDataSet_h
#define iGameResampleWithDataSet_h

#include "iGameCellCenter.h"
#include "iGameDrawObject.h"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSceneManager.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVector.h"
#include "iGameVolumeMesh.h"

#include <string>
#include <array>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 重采样至数据集（对标 ParaView / VTK 的 Resample With Dataset）
 *
 * 两个输入，语义与 vtkProbeFilter 一致。**注意 VTK 的命名与直觉相反**：
 *   - 输入 0「被采样网格」= VTK 的 **Source**：提供数据的网格。只有它的**点数据**被插值到
 *     采样点上；它的**单元数据不进入输出**，单元只用于判断采样点落在哪个单元。
 *     输入会先归一化为一份 UnstructuredMesh 再参与定位与插值：PointSet（每点一个顶点单元）、
 *     SurfaceMesh、VolumeMesh、StructuredMesh（按 (ni,nj,nk) 生成 QUAD / HEXAHEDRON）、
 *     LagrangeUnstructuredMesh 直接转换；容器网格（DrawObject / CompositeDataObject / 带子数据
 *     对象的多帧时间序列、多 zone 文件）递归展开后合并为一份网格（多块等价先做 Merge Blocks）。
 *   - 输入 1「采样点网格」= VTK 的 **Input**：探针几何。它的**所有点**就是采样位置，
 *     并且它的几何与拓扑会被原样**深拷贝**成输出。
 *
 * 输出（output 0）：
 *   - 几何与拓扑：输入 1 的同类型深拷贝（PointSet / SurfaceMesh / UnstructuredMesh /
 *     VolumeMesh / StructuredMesh），类型与输入 1 保持一致；
 *   - 属性：
 *     ① 输入 0 的点数据插值到输出点上；
 *     ② 输入 0 的单元数据按命中单元拷到输出点上（与 vtkProbeFilter 的 InputCellArrays 一致：
 *        源单元数组进入**输出点数据**，不做单元内插值；若同一名字同时存在于输入 0 的点数据与
 *        单元数据，只取点数据）；
 *     ③ validpointmask（UnsignedCharArray，1 = 该采样点插值成功，0 = 无效；无效点的
 *        各插值属性填 0）；
 *     （不包含）输入 1 自身的属性数组：本实现不复制「采样点网格」自带的数组，
 *     与 vtkProbeFilter 的默认值 PassPointArrays = 0 / PassCellArrays = 0 一致
 *     （注意这两个开关针对的是探针输入，不是被采样网格）。
 *
 * 采样与容差：
 *   - 采样位置是输入 1 的所有点，不再沿线均匀生成；
 *   - 容差默认自动（被采样网格包围盒对角线 × 1e-6），可用 SetTolerance() 手动指定；
 *   - 采样点落在所有单元之外时可选的「吸附半径」：SetSnappingRadius(r) 打开后，若在 r
 *     （相对包围盒对角线）范围内存在单元，则把该点吸附到最近单元的边界最近点处取值，
 *     该采样点记为有效。对应 vtkProbeFilter 的 SnappingRadius；本实现不移动采样点坐标。
 *
 * 单元类型与插值方式沿用 ResampleToLine 的内核：线性面单元用线性插值（重心坐标 / 双线性 /
 * 扇形三角化），八节点六面体用三线性形函数，其他体单元用均值坐标（Mean Value Coordinates），二次/高次单元用二次形函数 +
 * 参数坐标 Newton 反解；未覆盖的高次单元退化为角点线性处理，并在 GetMessage() 中提示数量。
 */
class ResampleWithDataSet : public Filter {
public:
    I_OBJECT(ResampleWithDataSet);
    static Pointer New() { return new ResampleWithDataSet; }

    bool Execute() override;

    /* ---------------- 输入 ---------------- */

    /** 被采样网格（数据来源，对应 VTK 的 Source）→ 输入 0 */
    void SetSourceData(DataObject::Pointer data) { SetInput(0, data); }
    /** 采样点网格（探针几何，输出是它的同类型深拷贝；对应 VTK 的 Input）→ 输入 1 */
    void SetProbeData(DataObject::Pointer data) { SetInput(1, data); }
    DataObject::Pointer GetSourceData() { return GetInput(0); }
    DataObject::Pointer GetProbeData() { return GetInput(1); }

    /* ---------------- 参数设置 ---------------- */

    /** 是否使用自动容差（默认开启） */
    void SetAutoTolerance(bool flag) { m_AutoTolerance = flag; }
    bool IsAutoTolerance() const { return m_AutoTolerance; }

    /**
     * 手动指定容差（相对被采样网格包围盒对角线长度）。
     * t <= 0 时恢复自动容差（包围盒对角线 × 1e-6）。
     */
    void SetTolerance(double t) {
        if (t > 0.0) {
            m_AutoTolerance = false;
            m_Tolerance = t;
        } else {
            m_AutoTolerance = true;
            m_Tolerance = -1.0;
        }
    }
    /** 手动容差；自动容差时返回 -1 */
    double GetTolerance() const { return m_AutoTolerance ? -1.0 : m_Tolerance; }
    /** 上一次 Execute() 实际使用的容差（绝对长度） */
    double GetEffectiveTolerance() const { return m_EffectiveTolerance; }

    /**
     * 吸附半径（相对被采样网格包围盒对角线长度）。r <= 0 表示关闭吸附（默认）。
     * 开启后，未被任何单元包含、但在半径内存在单元的采样点会吸附到最近单元边界最近点取值。
     */
    /** 有效采样点 id 列表（等价 VTK vtkProbeFilter::GetValidPoints()） */
    std::vector<igIndex> GetValidPoints() const;
    /** validpointmask 数组名；传空串恢复默认 "validpointmask" */
    void SetValidPointMaskArrayName(const std::string& maskName) {
        m_ValidPointMaskArrayName = maskName.empty() ? std::string("validpointmask") : maskName;
    }
    const std::string& GetValidPointMaskArrayName() const { return m_ValidPointMaskArrayName; }
    void SetSnappingRadius(double r = 0) { m_SnappingRadius = (r > 0.0) ? r : -1.0; }
    double GetSnappingRadius() const { return m_SnappingRadius; }
    /** 上一次 Execute() 实际使用的吸附半径（绝对长度）；< 0 表示未启用 */
    double GetEffectiveSnappingRadius() const { return m_EffectiveSnappingRadius; }

    std::string GetMessage() const { return m_Message; }

    /* ---------------- 结果查询 ---------------- */

    /** output 0：输入 1 的同类型深拷贝 + 采样得到的属性 */
    DataObject::Pointer GetResampledData() const { return m_Output; }
    /** 每个采样点命中的单元 id，-1 表示无效（不在任何单元内且未吸附） */
    const std::vector<igIndex>& GetSampleCellIds() const { return m_SampleCellIds; }
    /** 每个采样点是否有效（1 = 可插值，0 = 无效），与 validpointmask 属性一致 */
    const std::vector<unsigned char>& GetSampleValidMask() const { return m_SampleValidMask; }
    /** 本次执行中依靠吸附半径才命中的采样点数量 */
    IGsize GetSnappedPointCount() const { return m_SnappedPointCount; }
    /** 本次执行中退化为线性角点处理的二次/高次单元数量 */
    IGsize GetUnsupportedQuadraticCellCount() const { return m_UnsupportedQuadraticCellCount; }

private:
    /** 单个采样点的定位与插值权重 */
    struct SampleLocation {
        igIndex cellId{-1};            // 命中的单元 id，-1 = 无效
        Point point{0.f, 0.f, 0.f};    // 采样点
        std::vector<igIndex> pointIds; // 命中单元的点 id（与权重一一对应）
        std::vector<double> weights;   // 插值权重
    };

    /* ---------------- 几何搜索 ---------------- */
    std::vector<std::vector<igIndex>> BuildUniformGrid(const UnstructuredMesh::Pointer& mesh,
                                                       const BoundingBox& bbox, igIndex nx, igIndex ny, igIndex nz);
    bool LocateSample(const UnstructuredMesh::Pointer& mesh, const Point& p, const BoundingBox& bbox,
                      const std::vector<std::vector<igIndex>>& grid, igIndex nx, igIndex ny, igIndex nz,
                      double distTol, SampleLocation& out);
    /**
     * 吸附搜索：在 radius（绝对长度）范围内寻找离 p 最近的单元，并给出该单元边界上的最近点。
     * 只有采样点未被任何单元包含（LocateSample 失败）且开启了吸附半径时才会调用。
     */
    bool LocateNearestCell(const UnstructuredMesh::Pointer& mesh, const Point& p, const BoundingBox& bbox,
                           const std::vector<std::vector<igIndex>>& grid, igIndex nx, igIndex ny, igIndex nz,
                           double radius, igIndex& cellId, Point& closest);

    /* ---------------- 单元权重 ---------------- */
    bool ComputeCellWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol,
                            std::vector<double>& weights);
    bool ComputeLinearFaceWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol,
                                  std::vector<double>& weights);
    bool ComputeMeanValueWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol,
                                 std::vector<double>& weights);
    bool ComputeQuadraticWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol,
                                 std::vector<double>& weights);
    /** 高次体单元退化：用线性角点单元的面表做均值坐标插值 */
    bool ComputeDegradedVolumeWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                     double distTol, std::vector<double>& weights);
    bool IsPointInVolumeCell(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol);
    double DistanceToCellBoundary(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, Point& closest);

    /* ---------------- 属性处理 ---------------- */
    /** 输入 0 的点数据插值 → 输出点；返回已产出的数组名（供单元数据判重：同名时点数据优先） */
    std::vector<std::string> InterpolatePointData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                                                 const std::vector<SampleLocation>& locations, int sampleNum);
    /** 输入 0 的单元数据按命中单元 → 输出点；pointNames 中的名字（源点数据）优先，同名跳过 */
    void CopyCellData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                      const std::vector<SampleLocation>& locations, int sampleNum,
                      const std::vector<std::string>& pointNames);
    void AddValidPointMask(AttributeSet::Pointer outSet, int sampleNum);

    ResampleWithDataSet() {
        SetNumberOfInputs(2);
        SetNumberOfOutputs(1);
    }
    ~ResampleWithDataSet() override = default;

    // 容差：默认自动（包围盒对角线 × kAutoToleranceRatio）
    bool m_AutoTolerance{true};
    double m_Tolerance{-1.0};
    double m_EffectiveTolerance{0.0};

    // 吸附半径：默认关闭（相对包围盒对角线长度，<= 0 表示关闭）
    std::string m_ValidPointMaskArrayName{"validpointmask"};
    double m_SnappingRadius{-1.0};
    double m_EffectiveSnappingRadius{-1.0};

    std::string m_Message{"未执行"};
    IGsize m_UnsupportedQuadraticCellCount{0};
    IGsize m_SnappedPointCount{0};

    DataObject::Pointer m_Output{};
    std::vector<igIndex> m_SampleCellIds;
    std::vector<unsigned char> m_SampleValidMask;
    // Triangle bounds include both geometric and barycentric acceptance tolerances.
    // Every possible hit is indexed in the query voxel, including boundary hits.
    bool m_TriangleGrid{false};
    std::vector<std::array<double, 6>> m_TriangleBounds;
};

IGAME_NAMESPACE_END
#endif
