#ifndef MeshCodecParamSet_h
#define MeshCodecParamSet_h

#include "iGameMacro.h"
#include "iGameType.h"
#include <vector>

IGAME_NAMESPACE_BEGIN
enum class LossyMode {
	MantissaTruncation,  // 尾数截断
	Quantization  // 量化
};

enum class ErrorMode{
	None,
	Default,
	KeyArea
};

struct FloatParameters {
	LossyMode lossyMode; // 量化模式
	ErrorMode errorMode;

    int globalQuantizeLevel;
    int criticalQuantizeLevel;
    int normalQuantizeLevel;

	// element 由一组value组成 当然也有可能是单个
	IGsize valueSize; // 单个分量尺寸 单位byte
	IGsize elementCount; // 元素数量 elementCount * dimension = valueCount
	int dimension; // 维度
};

struct GeomParameters : FloatParameters {};

struct AttrParameters : FloatParameters {
	char name[256]; // 名称
	
	IGenum type; // 类型 IG_SCALAR, IG_VECTOR, IG_NORMAL, IG_TCOORD, IG_TENSOR
	IGenum attachmentType; // 附着类型 IG_POINT, IG_CELL

	IGsize binaryCount; // 在二进制流中的长度
};

struct TopoParameters {
	bool isSecondaryIndex; // 是否启用二阶索引
	int fixedCellSize; // 等于0时代表启用offset buffer 反之则代表固定offset
	
	// 由于buffer offset type使用了不同的编码技术 所以需要分开记录它们的二进制结束位置
	IGsize topCellBufferBinaryCount; // cell ids 区域占用字节数
	IGsize topCellSizeBinaryCount; // cell sizes 区域占用字节

	IGsize topCellBufferSize; // 存储cell的顶点id的array尺寸 元素类型是 unsigned int 不包括padding
	int topCellBufferPadding;

	// --------------------------------------------------------------------------------------

	IGsize bottomCellBufferBinaryCount;
	IGsize bottomCellSizeBinaryCount;

	IGsize bottomCellBufferSize; // 存储cell的顶点id的array尺寸 元素类型是 unsigned int 不包括padding
	int bottomCellBufferPadding;

	IGsize cellTypeBinaryCount;
};

struct StructuredMeshParameters {
	int axisSize[3];
};

// 仅用于二进制写入
struct ParametersWoAttr {
	int meshType; // 网格类型 
	// IG_SURFACE_MESH,
	// IG_VOLUME_MESH,
	// IG_UNSTRUCTURED_MESH,
	// IG_STRUCTURED_MESH,

	// 结构化网格比较特殊 先把它的参数放在这里
	StructuredMeshParameters structuredMeshParams;

	GeomParameters geomParams;
	TopoParameters topoParams;

	int attrCount; // 属性数量
};

struct CodecParameters : ParametersWoAttr {
	std::vector<AttrParameters> attrParams;
};

struct FloatErrorControlParameters : FloatParameters {
	// 约定 0号data是顶点坐标
	std::string dataName;
	std::vector<bool> isKeyElement; // 标记每个数据点是否是重要的
};

// --------------------------------------------------------------------------------------
// internal codec parameters

struct UIControlParams {
	// 顶点/属性误差设置
	std::vector<FloatErrorControlParameters> errorBoundSetting;

	// bool visualError;
	bool showReport;

	int compressLevel = 3;
};

// --------------------------------------------------------------------------------------
// UI下拉菜单索引与数据索引的映射工具
// 下拉菜单布局:
//   索引 0: "全体数据" (特殊项，用于批量设置)
//   索引 1: "顶点坐标" (几何数据)
//   索引 2+: 实际属性数据
namespace UIControlParamsIndex {
	constexpr int kAllDataIndex = 0;      // 全体数据
	constexpr int kGeomIndex = 1;         // 顶点坐标
	constexpr int kAttrStartIndex = 2;    // 属性数据起始索引
	constexpr int kOffset = 2;            // 偏移量

	// 判断是否为全体数据索引
	inline bool IsAllData(int uiIndex) { return uiIndex == kAllDataIndex; }

	// 判断是否为顶点坐标索引
	inline bool IsGeom(int uiIndex) { return uiIndex == kGeomIndex; }

	// 判断是否为属性数据索引
	inline bool IsAttr(int uiIndex) { return uiIndex >= kAttrStartIndex; }

	// UI索引 -> 属性索引 (仅当 IsAttr(uiIndex) 为 true 时有效)
	inline int ToAttrIndex(int uiIndex) { return uiIndex - kOffset; }

	// 属性索引 -> UI索引
	inline int FromAttrIndex(int attrIndex) { return attrIndex + kOffset; }

	// 计算总的UI项数 (全体数据 + 顶点坐标 + 属性数量)
	inline int GetTotalCount(int attrCount) { return attrCount + kOffset; }
}

IGAME_NAMESPACE_END
#endif