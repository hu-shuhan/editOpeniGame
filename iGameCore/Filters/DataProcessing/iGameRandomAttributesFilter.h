#ifndef iGameRandomAttributesFilter_h
#define iGameRandomAttributesFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameUnstructuredMesh.h"

#include <random>
#include <ctime>
#include <string>
#include <type_traits>
#include <limits>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 生成随机标量并挂接到输出数据对象的 POINT / CELL 上。
 *
 * 改进点（相对复测版）：
 *   1. 核心创建独立输出（DeepCopy），确保原模型属性不变。
 *   2. 完整保留点、面、体单元、单元类型和结构尺寸。
 *   3. 增加逐项输入校验，错误信息说明具体原因。
 *   4. 修正二维结构网格的 Cell 计数（2D 取面数，3D 取体单元数）。
 *   5. 增加数组名称设置 + 同名冲突处理策略（替换/自动改名/追加）。
 */
class RandomAttributesFilter : public Filter {
public:
    I_OBJECT(RandomAttributesFilter);
    static Pointer New() { return new RandomAttributesFilter; }
    ~RandomAttributesFilter();

    // 同名数组冲突处理策略
    enum class NameConflictMode {
        Replace,        // 替换已有同名数组
        AutoRename,     // 自动改名（如 name_1, name_2）
        Append          // 直接追加
    };

    void SetRange(double lo, double hi) noexcept { _min = lo; _max = hi; }
    void SetDataType(IGenum type) noexcept { _dataType = type; }
    void SetAttachmentType(IGenum type) noexcept { _attach = type; }
    void SetSeed(unsigned int seed) noexcept { _seed = seed; }
    void SetAttributeName(const std::string& name) { _name = name; }
    void SetNameConflictMode(NameConflictMode mode) noexcept { _conflictMode = mode; }
    std::string GetMessage() const { return _msg; }

    bool Execute() override;

protected:
    RandomAttributesFilter();

    // 逐项输入校验
    bool ValidateInputs(std::string& why) const;

    // 创建独立输出副本（DeepCopy 输入的几何结构 + 属性）
    DataObject::Pointer CreateIndependentOutput(DataObject::Pointer input);

    template<typename TArray, typename TValue>
    ArrayObject::Pointer CreateTypedArray(IGsize num, double lo, double hi, unsigned int seed);

    ArrayObject::Pointer CreateDataArray(IGsize num, double lo, double hi, unsigned int seed);

    // 改进点4：二维结构网格取实际单元数（面数），3D 取体单元数
    bool ComputeCount(DataObject::Pointer input, IGsize& out, std::string& why) const;

    // 处理同名数组：根据 _conflictMode 替换/改名/追加
    bool ApplyArrayToOutput(DataObject::Pointer output, ArrayObject::Pointer data,
                            const std::string& name, std::string& why);

    double            _min{ 0.0 };
    double            _max{ 1.0 };
    IGenum            _dataType{ IG_FLOAT };
    IGenum            _attach{ IG_POINT };
    unsigned int      _seed{ static_cast<unsigned int>(std::time(nullptr)) };
    std::string       _name;
    NameConflictMode  _conflictMode{ NameConflictMode::Append };
    std::string       _msg{ "随机属性生成失败。" };
};

template<typename TArray, typename TValue>
ArrayObject::Pointer RandomAttributesFilter::CreateTypedArray(
    IGsize num, double lo, double hi, unsigned int seed)
{
    auto arr = TArray::New();
    arr->SetName(_attach == IG_POINT ? "RandomPointScalars" : "RandomCellScalars");
    arr->SetDimension(1);
    if (num == 0) return arr;
    arr->Reserve(num);

    using lim = std::numeric_limits<TValue>;
    if constexpr (std::is_floating_point_v<TValue>) {
        double lo2 = lo, hi2 = hi;
        if (lo2 < static_cast<double>(lim::lowest()))  lo2 = static_cast<double>(lim::lowest());
        if (hi2 > static_cast<double>(lim::max()))     hi2 = static_cast<double>(lim::max());
        if (hi2 < lo2) hi2 = lo2;
        std::mt19937 gen(seed);
        std::uniform_real_distribution<double> dist(lo2, hi2);
        for (IGsize i = 0; i < num; ++i)
            arr->AddValue(static_cast<TValue>(dist(gen)));
    } else {
        long long loLL, hiLL;
        if constexpr (std::is_signed_v<TValue>) {
            constexpr auto MIN = static_cast<long long>(lim::min());
            constexpr auto MAX = static_cast<long long>(lim::max());
            loLL = static_cast<long long>(lo);
            hiLL = static_cast<long long>(hi);
            if (loLL < MIN) loLL = MIN;
            if (hiLL > MAX) hiLL = MAX;
        } else {
            using ULL = unsigned long long;
            constexpr auto MAX = static_cast<ULL>(lim::max());
            ULL loU = static_cast<ULL>(lo), hiU = static_cast<ULL>(hi);
            if (loU > MAX) loU = MAX;
            if (hiU > MAX) hiU = MAX;
            if (hiU < loU) hiU = loU;
            loLL = static_cast<long long>(loU);
            hiLL = static_cast<long long>(hiU);
        }
        if (hiLL < loLL) hiLL = loLL;
        std::mt19937 gen(seed);
        std::uniform_int_distribution<long long> dist(loLL, hiLL);
        for (IGsize i = 0; i < num; ++i)
            arr->AddValue(static_cast<TValue>(dist(gen)));
    }
    return arr;
}

IGAME_NAMESPACE_END
#endif
