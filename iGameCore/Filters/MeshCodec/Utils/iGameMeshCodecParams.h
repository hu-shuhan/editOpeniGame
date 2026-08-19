#ifndef MeshCodecParams_h
#define MeshCodecParams_h

#include "iGameMacro.h"
#include "iGameType.h"
#include <limits>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

// =====================================================================================
// 枚举定义
// =====================================================================================
enum class LossyMode {
	MantissaTruncation,  // 尾数截断
	Quantization         // 量化
};

enum class QuantizeMode {
	None,      // 无损
	Default,   // 全局量化
	KeyArea    // 分区量化
};

// =====================================================================================
// 存储参数（写入编码文件，用于解码）
// =====================================================================================

// 浮点数据存储参数
struct FloatStorageParams {
	LossyMode lossyMode = LossyMode::MantissaTruncation;
	QuantizeMode errorMode = QuantizeMode::None;
	IGsize valueSize = sizeof(float);  // 单个分量尺寸（字节）
	IGsize elementCount = 0;           // 元素数量
	int dimension = 0;                 // 维度

	template<typename Ar>
	void Archive(Ar& ar) {
		ar.Process(lossyMode);
		ar.Process(errorMode);
		ar.Process(valueSize);
		ar.Process(elementCount);
		ar.Process(dimension);
	}
};

// 几何数据存储参数
struct GeomStorageParams : FloatStorageParams {};

// 属性数据存储参数
struct AttrStorageParams : FloatStorageParams {
	std::string name = {};             // 名称
	IGenum type = 0;                   // 类型 IG_SCALAR, IG_VECTOR, IG_NORMAL, IG_TCOORD, IG_TENSOR
	IGenum attachmentType = 0;         // 附着类型 IG_POINT, IG_CELL
	IGsize binaryCount = 0;            // 在二进制流中的长度

	template<typename Ar>
	void Archive(Ar& ar) {
		FloatStorageParams::Archive(ar);
		ar.Process(name);
		ar.Process(type);
		ar.Process(attachmentType);
		ar.Process(binaryCount);
	}
};

// 拓扑参数
struct TopoStorageParameters {
	bool isSecondaryIndex = false;     // 是否启用二阶索引
	int fixedCellSize = 0;             // 等于0时代表启用offset buffer，反之则代表固定offset
	
	// 由于buffer/offset/type使用了不同的编码技术，所以需要分开记录它们的二进制结束位置
	IGsize topCellBufferBinaryCount = 0;    // cell ids 区域占用字节数
	IGsize topCellSizeBinaryCount = 0;      // cell sizes 区域占用字节
	IGsize topCellBufferSize = 0;           // 存储cell的顶点id的array尺寸，元素类型是 unsigned int，不包括padding
	int topCellBufferPadding = 0;

	IGsize bottomCellBufferBinaryCount = 0;
	IGsize bottomCellSizeBinaryCount = 0;
	IGsize bottomCellBufferSize = 0;
	int bottomCellBufferPadding = 0;

	IGsize cellTypeBinaryCount = 0;

	template<typename Ar>
	void Archive(Ar& ar) {
		ar.Process(isSecondaryIndex);
		ar.Process(fixedCellSize);
		ar.Process(topCellBufferBinaryCount);
		ar.Process(topCellSizeBinaryCount);
		ar.Process(topCellBufferSize);
		ar.Process(topCellBufferPadding);
		ar.Process(bottomCellBufferBinaryCount);
		ar.Process(bottomCellSizeBinaryCount);
		ar.Process(bottomCellBufferSize);
		ar.Process(bottomCellBufferPadding);
		ar.Process(cellTypeBinaryCount);
	}
};

// 结构化网格参数
struct StructuredMeshStorageParameters {
	int axisSize[3] = {0, 0, 0};

	template<typename Ar>
	void Archive(Ar& ar) {
		ar.Process(axisSize);
	}
};

struct CodecStorageHeader {
	uint32_t version = 2;              // 参数块版本
	bool attrUseCrossDependency = false;
	uint8_t reserved[3] = {0, 0, 0};

	static constexpr uint8_t kRequires64BitSize = 1u << 0;

	void SetRequires64BitSize(bool value) {
		if (value) {
			reserved[0] = static_cast<uint8_t>(reserved[0] | kRequires64BitSize);
		} else {
			reserved[0] = static_cast<uint8_t>(reserved[0] & ~kRequires64BitSize);
		}
	}

	[[nodiscard]] bool Requires64BitSize() const {
		return (reserved[0] & kRequires64BitSize) != 0;
	}

	template<typename Ar>
	void Archive(Ar& ar) {
		ar.Process(version);
		ar.Process(attrUseCrossDependency);
		ar.Process(reserved);
	}
};

// 完整的存储参数
struct CodecStorageParams {
	int meshType = 0;                  // 网格类型
	StructuredMeshStorageParameters structuredMeshParams;
	GeomStorageParams geomParams;
	TopoStorageParameters topoParams;
	std::vector<AttrStorageParams> attrParams;

	template<typename Ar>
	void Archive(Ar& ar) {
		ar.Process(meshType);
		ar.Process(structuredMeshParams);
		ar.Process(geomParams);
		ar.Process(topoParams);
		ar.Process(attrParams);
	}
};

struct CodecStorageParamSizeLimits {
	static constexpr uint64_t k32BitMax = std::numeric_limits<uint32_t>::max();

	static bool AddWillOverflow(IGsize a, IGsize b) {
		return b > std::numeric_limits<IGsize>::max() - a;
	}

	static bool MulWillOverflow(IGsize a, IGsize b) {
		return b != 0 && a > std::numeric_limits<IGsize>::max() / b;
	}

	static bool Exceeds32BitValue(uint64_t value) {
		return value > k32BitMax;
	}

	static bool SignedExceeds32Bit(int value) {
		return value < 0 || static_cast<uint64_t>(value) > k32BitMax;
	}

	static bool AddExceeds32Bit(IGsize a, IGsize b) {
		return AddWillOverflow(a, b) || Exceeds32BitValue(static_cast<uint64_t>(a + b));
	}

	static bool MulExceeds32Bit(IGsize a, IGsize b) {
		return MulWillOverflow(a, b) || Exceeds32BitValue(static_cast<uint64_t>(a * b));
	}

	static bool FloatParamsExceed32Bit(const FloatStorageParams& params) {
		return params.dimension < 0 ||
		       Exceeds32BitValue(static_cast<uint64_t>(params.valueSize)) ||
		       Exceeds32BitValue(static_cast<uint64_t>(params.elementCount)) ||
		       SignedExceeds32Bit(params.dimension) ||
		       MulExceeds32Bit(params.elementCount, static_cast<IGsize>(params.dimension));
	}

	static bool TopoParamsExceed32Bit(const TopoStorageParameters& params) {
		if (params.topCellBufferPadding < 0 || params.bottomCellBufferPadding < 0) { return true; }
		return Exceeds32BitValue(static_cast<uint64_t>(params.topCellBufferBinaryCount)) ||
		       Exceeds32BitValue(static_cast<uint64_t>(params.topCellSizeBinaryCount)) ||
		       Exceeds32BitValue(static_cast<uint64_t>(params.topCellBufferSize)) ||
		       SignedExceeds32Bit(params.topCellBufferPadding) ||
		       AddExceeds32Bit(params.topCellBufferSize, static_cast<IGsize>(params.topCellBufferPadding)) ||
		       Exceeds32BitValue(static_cast<uint64_t>(params.bottomCellBufferBinaryCount)) ||
		       Exceeds32BitValue(static_cast<uint64_t>(params.bottomCellSizeBinaryCount)) ||
		       Exceeds32BitValue(static_cast<uint64_t>(params.bottomCellBufferSize)) ||
		       SignedExceeds32Bit(params.bottomCellBufferPadding) ||
		       AddExceeds32Bit(params.bottomCellBufferSize, static_cast<IGsize>(params.bottomCellBufferPadding)) ||
		       Exceeds32BitValue(static_cast<uint64_t>(params.cellTypeBinaryCount));
	}

	static bool ParamsExceed32Bit(const CodecStorageParams& params) {
		if (Exceeds32BitValue(static_cast<uint64_t>(params.attrParams.size()))) { return true; }
		if (FloatParamsExceed32Bit(params.geomParams)) { return true; }
		if (TopoParamsExceed32Bit(params.topoParams)) { return true; }
		for (const auto& attr : params.attrParams) {
			if (FloatParamsExceed32Bit(attr) ||
			    Exceeds32BitValue(static_cast<uint64_t>(attr.binaryCount)) ||
			    Exceeds32BitValue(static_cast<uint64_t>(attr.name.size()))) {
				return true;
			}
		}
		return false;
	}
};

// =====================================================================================
// 控制参数（控制编码行为，不写入文件）
// =====================================================================================

// 浮点数据控制参数
struct FloatControlParams {
	QuantizeMode errorMode = QuantizeMode::None;  // 量化模式
	int globalQuantizeLevel = 0;       // 全局量化等级
	int criticalQuantizeLevel = 0;     // 关键区域量化等级
	int normalQuantizeLevel = 0;       // 非关键区域量化等级
	std::vector<bool> isKeyElement;    // 关键元素标记
};

// 编码器控制参数（从 UI 传入）
struct CodecControlParams {
	FloatControlParams geomControl;                 // 几何数据控制参数
	std::vector<FloatControlParams> attrControl;    // 属性数据控制参数列表
	bool showReport = false;
	int compressLevel = 12;
	bool exportNumpy = false;                       // 是否导出原始/重建场数据
	std::vector<int> numpyAttributeIndices;         // 要导出的属性索引
	std::string numpyOutputBasePath;                // 不含扩展名的输出基准路径
};

IGAME_NAMESPACE_END
#endif
