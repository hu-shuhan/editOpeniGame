#ifndef MeshCodecParamSet_h
#define MeshCodecParamSet_h

#include "iGameMacro.h"
#include "iGameType.h"
#include <vector>

IGAME_NAMESPACE_BEGIN
typedef enum {
	None = 0,
	FP16 = 1, 
	Float = 2 // 暂时禁用
} QuantMode;

struct MeshOptFloatParameters {
	double scale; // 小数点后精度控制 如果-1代表不启用
	QuantMode quantMode; // 量化模式
	int quantParam; // 量化参数 仅在Float形式有效

	// element 由一组value组成 当然也有可能是单个
	IGsize valueSize; // 单个分量尺寸 单位byte
	IGsize elementCount; // 元素数量 elementCount * dimension = valueCount
	int dimension; // 维度
};

struct MeshOptGeomParameters : MeshOptFloatParameters {};

struct MeshOptAttrParameters : MeshOptFloatParameters {
	char name[256]; // 名称
	
	IGenum type; // 类型 IG_SCALAR, IG_VECTOR, IG_NORMAL, IG_TCOORD, IG_TENSOR
	IGenum attachmentType; // 附着类型 IG_POINT, IG_CELL

	IGsize binaryCount; // 在二进制流中的长度
};

struct MeshOptTopoParameters {
	int fixedCellSize; // 等于0时代表启用offset buffer 反之则代表固定offset
	IGsize cellBufferSize; //存储cell的顶点id的array的尺寸 不包括padding
	IGsize cellCount;
	int cellBufferPadding;

	// 由于buffer offset type使用了不同的编码技术 所以需要分开记录它们的二进制结束位置
	IGsize cellBufferBinaryCount;
	IGsize cellSizeBinaryCount;
	IGsize cellTypeBinaryCount;
};

struct MeshOptStructuredMeshParameters {
	int axisSize[3];
};

// 仅用于二进制写入
struct MeshOptParametersWithoutAttr {
	int meshType; // 网格类型 
	// IG_SURFACE_MESH,
	// IG_VOLUME_MESH,
	// IG_UNSTRUCTURED_MESH,
	// IG_STRUCTURED_MESH,

	// 结构化网格比较特殊 先把它的参数放在这里
	MeshOptStructuredMeshParameters structuredMeshParams;

	MeshOptGeomParameters geomParams;
	MeshOptTopoParameters topoParams;

	int attrCount; // 属性数量
};

struct MeshOptParameters : MeshOptParametersWithoutAttr {
	std::vector<MeshOptAttrParameters> attrParams;
};

struct ParamInformation {
    iGame::QuantMode PointQuantMode = iGame::QuantMode::Float;
    int PointQuantizedBits = 16;
    iGame::QuantMode AttrbQuantMode = iGame::QuantMode::None;
    int AttrbQuantizedBits = 16;
};

IGAME_NAMESPACE_END
#endif