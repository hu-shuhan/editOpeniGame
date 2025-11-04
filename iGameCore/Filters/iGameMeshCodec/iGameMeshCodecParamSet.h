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

	float defaultErrorBound;
	float keyAreaErrorBound;
	float nonKeyAreaErrorBound;

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

	bool visualError;
	bool showReport;
};

IGAME_NAMESPACE_END
#endif