#pragma once
#include "iGameDataObject.h"
#include "iGameFilter.h"
#include "iGameStreamingData.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class iGameTemporalStatistics
 * @brief 时域统计过滤器（对标 ParaView / VTK 的 vtkTemporalStatistics）。
 *
 * 语义：把当前模型的**全部数组**在时间维度上逐元素统计一遍，
 *  - 逐元素：每个点 / 单元各得一份统计值（颜色逐元素变化）；
 *  - 输入对象只读，输出是**新数据集**：几何沿用输入的 Points / CellArray，
 *    属性集全新；调用方拿 GetOutput(0) 交给
 *    igQtModelDialogWidget::addDataObjectToModelTree() 即得到一个新模型；
 *  - 输出命名：<数组名>_average / _minimum / _maximum / _stddev；
 *  - 向量数组逐分量统计，输出与源同维的向量。
 *
 * 与 ParaView 对齐的行为：
 *  - 统计量可分别开关（默认平均 / 最小 / 最大 / 标准差全开，与 VTK 9.4 一致）；
 *  - 忽略时间间隔，每帧等权；平均值 = 累加和 / 总帧数；标准差为**总体标准差**（除以总帧数）；
 *  - 单帧输入也能执行（平均值 = 该值，最小 = 最大 = 该值，标准差 = 0）；
 *  - 某个数组在某些帧缺失、或数组规模随帧变化时**不整体失败**：
 *    缺失的帧跳过（均值仍按总帧数求），规模变化则丢弃该数组，均通过 GetWarnings() 报告。
 *
 * 输入支持 StreamingData 的两种帧类型（见 StreamingType）：
 *  - MultiSubFiles（PVD 逐帧多文件 / 多子文件）：每帧若干块对象，逐块统计；
 *    多块时输出为"容器根 + 每块一个子对象"；
 *  - SingleFieldAttributes（单场属性帧，例如 ODB 场数据）：几何挂在宿主对象上。
 */
class iGameTemporalStatistics : public Filter {
public:
    I_OBJECT(iGameTemporalStatistics);
    static Pointer New() { return new iGameTemporalStatistics; }

    /* ---------- 统计量开关（默认全开，与 VTK 9.4 的 vtkTemporalStatistics 一致） ---------- */
    void SetComputeAverage(bool enabled) { m_ComputeAverage = enabled; }
    void SetComputeMinimum(bool enabled) { m_ComputeMinimum = enabled; }
    void SetComputeMaximum(bool enabled) { m_ComputeMaximum = enabled; }
    void SetComputeStandardDeviation(bool enabled) { m_ComputeStandardDeviation = enabled; }

    bool GetComputeAverage() const { return m_ComputeAverage; }
    bool GetComputeMinimum() const { return m_ComputeMinimum; }
    bool GetComputeMaximum() const { return m_ComputeMaximum; }
    bool GetComputeStandardDeviation() const { return m_ComputeStandardDeviation; }

    bool Execute() override;

    //! 失败原因（Execute 返回 false 时有效）。
    std::string GetMessage() const { return m_Message; }
    //! 非致命提示（数组在某些帧缺失 / 规模变化被丢弃等）：Execute 返回 true 时也可能非空。
    const std::vector<std::string>& GetWarnings() const { return m_Warnings; }
    //! 把 GetWarnings() 拼成多行文本，便于面板直接显示。
    std::string GetWarningsText() const;

protected:
    iGameTemporalStatistics();
    ~iGameTemporalStatistics() override = default;

private:
    //! 结果属性名后缀（固定，与 VTK 相同）。
    static constexpr const char* kAverageSuffix = "_average";
    static constexpr const char* kMinimumSuffix = "_minimum";
    static constexpr const char* kMaximumSuffix = "_maximum";
    static constexpr const char* kStandardDeviationSuffix = "_stddev";

    //! 统计量编号：与 BuildArray 的 kind 对应。
    enum Statistic {
        StatisticAverage = 0,
        StatisticMinimum = 1,
        StatisticMaximum = 2,
        StatisticStandardDeviation = 3,
    };

    //! 一个输出块的统计累加器：单块输入只有 1 个，多文件帧每个子文件 1 个。
    struct Accumulator {
        IGsize count{0};          //!< 参与统计的元素数（点数或单元数）
        IGsize frameCount{0};     //!< 已处理的帧数（均值 / 标准差都除以它，与 ParaView 一致）
        IGsize sampleCount{0};    //!< 实际取到值的帧数（缺数组时小于 frameCount）
        int channels{1};          //!< = 源数组维度：1 = 标量；N = N 分量向量
        std::vector<double> sum;  //!< 长度 = count * channels，按"元素 × 通道"排布
        std::vector<double> m2;   //!< 平方偏差累加（Welford），用于标准差
        std::vector<double> minimum;
        std::vector<double> maximum;

        void Reset(IGsize elementCount, int channelCount);
        //! 累加一帧（逐元素、逐分量）；Welford 的样本序号用实际取到值的帧数。
        bool AddFrame(ArrayObject::Pointer array);
        //! 一帧处理结束（含该数组在本帧缺失的情况）后调用。
        void MarkFrameProcessed() { ++frameCount; }
        //! kind 见 Statistic；arrayType 为源数组类型，输出数组沿用该类型（与 vtkTemporalStatistics 一致）。
        ArrayObject::Pointer BuildArray(int kind, const std::string& name, IGenum arrayType) const;
    };

    //! 第 index 帧的一块数据：geometry 提供点 / 单元，attributes 是该帧该块的属性集。
    struct FrameBlock {
        DataObject::Pointer geometry{};
        AttributeSet::Pointer attributes{};
    };

    //! 一个待统计数组解析后的信息。
    struct SourceInfo {
        std::string name{};
        IGenum attachment{IG_POINT};
        IGenum arrayType{IG_FloatArray}; //!< 源数组类型：输出统计数组沿用（与 ParaView 一致）
        int dimension{1};
        int channels{1};
        bool valid{true};       //!< 规模随帧变化时置 false，该数组整体丢弃
        int missingFrames{0};   //!< 缺失帧数（仅用于提示）
    };

    bool ResolveSources(const std::vector<FrameBlock>& templateBlocks);
    static std::vector<FrameBlock> CollectFrameBlocks(DataObject::Pointer owner,
                                                      StreamingData::Pointer frames,
                                                      unsigned int index);
    static DataObject::Pointer BuildOutputSkeleton(const std::vector<FrameBlock>& templateBlocks);
    //! 按类型新建同构对象并共享几何。
    static DataObject::Pointer MakeGeometryCopy(DataObject::Pointer geometry);
    bool Accumulate(DataObject::Pointer owner, StreamingData::Pointer frames,
                    const std::vector<FrameBlock>& templateBlocks, unsigned int frameNum,
                    std::vector<std::vector<Accumulator>>& accumulators);
    bool WriteStatistics(DataObject::Pointer output,
                         const std::vector<std::vector<Accumulator>>& accumulators);

    /* ---------- 工具 ---------- */
    static IGsize ElementCount(DataObject::Pointer geometry, IGenum attachment);
    //! attachment 为 IG_NONE 时按名字取第一个匹配，否则必须名字 + 隶属都一致。
    static ArrayObject::Pointer FindArray(AttributeSet* attributes, const std::string& name,
                                          IGenum attachment = IG_NONE);
    //! 属性范围：dim + 1 个元素，[0] = 模长范围，[1 + c] = 第 c 分量范围。
    static DoubleArray::Pointer ComputeDataRange(ArrayObject::Pointer array);
    //! 按源数组类型创建同类型输出数组（IntArray / FloatArray / DoubleArray / ...）。
    static ArrayObject::Pointer NewArrayLike(IGenum arrayType, const std::string& name, int channels, IGsize count);
    //! 记录一条非致命提示（去重 + 上限）。
    void AddWarning(const std::string& text);

    /* ---------- 参数 ---------- */
    bool m_ComputeAverage{true};
    bool m_ComputeMinimum{true};
    bool m_ComputeMaximum{true};
    bool m_ComputeStandardDeviation{true};

    std::vector<SourceInfo> m_Sources{}; //!< ResolveSources 的解析结果
    std::string m_Message{};
    std::vector<std::string> m_Warnings{};
};

IGAME_NAMESPACE_END
