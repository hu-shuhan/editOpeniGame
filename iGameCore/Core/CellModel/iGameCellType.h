#ifndef iGameCellType_h
#define iGameCellType_h

#include "iGameMacro.h"

IGAME_NAMESPACE_BEGIN

typedef enum
{
	IG_EMPTY_CELL = 0,
	IG_VERTEX = 1,
	IG_LINE = 2,
	IG_FACE,
	IG_TRIANGLE,
	IG_QUAD,
	IG_POLYGON,
	IG_VOLUME,
	IG_POLYHEDRON,
	IG_TETRA,         // 四面体
	IG_HEXAHEDRON,    // 六面体
	IG_PYRAMID,       // 金字塔
	IG_PRISM,         // 三棱柱

	// Quadratic, isoparametric cells
	IG_QUADRATIC_EDGE = 21,
	IG_QUADRATIC_TRIANGLE = 22,
	IG_QUADRATIC_QUAD = 23,
	IG_QUADRATIC_POLYGON = 36,
	IG_QUADRATIC_TETRA = 24,
	IG_QUADRATIC_HEXAHEDRON = 25,
	IG_QUADRATIC_PRISM = 26,
	IG_QUADRATIC_PYRAMID = 27,
	IG_BIQUADRATIC_QUAD = 28,
	IG_TRIQUADRATIC_HEXAHEDRON = 29,
	IG_TRIQUADRATIC_PYRAMID = 37,
	IG_QUADRATIC_LINEAR_QUAD = 30,
	IG_QUADRATIC_LINEAR_WEDGE = 31,
	IG_BIQUADRATIC_QUADRATIC_WEDGE = 32,
	IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON = 33,
	IG_BIQUADRATIC_TRIANGLE = 34,

	IG_POLY_LINE,
    IG_LAGRANGE_CURVE = 68,
    IG_LAGRANGE_TRIANGLE = 69,
    IG_LAGRANGE_QUADRILATERAL = 70,
    IG_LAGRANGE_TETRAHEDRON = 71,
    IG_LAGRANGE_HEXAHEDRON = 72,
    IG_LAGRANGE_PRISM = 73,
    IG_LAGRANGE_PYRAMID = 74,
} IGCellType;

inline constexpr bool GetCellTypeIsVariable(int type)
{
	switch (type)
	{
	case IG_EMPTY_CELL: return true;
	case IG_VERTEX: return false;
	case IG_LINE: return false;
	case IG_FACE: return true;
	case IG_TRIANGLE: return false;
	case IG_QUAD: return false;
	case IG_POLYGON: return true;
	case IG_VOLUME: return true;
	case IG_TETRA: return false;
	case IG_HEXAHEDRON: return false;
	case IG_PYRAMID: return false;
	case IG_PRISM: return false;
	case IG_POLYHEDRON: return true;
	default: return true;
	}
}

inline constexpr const char* GetCellTypeAsString(int type)
{
	switch (type)
	{
	case IG_EMPTY_CELL: return "Empty";
	case IG_VERTEX: return "Vertex";
	case IG_LINE: return "Edge";
	case IG_FACE: return "Face";
	case IG_TRIANGLE: return "Triangle";
	case IG_QUAD: return "Quad";
	case IG_POLYGON: return "Polygon";
	case IG_VOLUME: return "Volume";
	case IG_TETRA: return "Tetra";
	case IG_HEXAHEDRON: return "Hexahedron";
	case IG_PYRAMID: return "Pyramid";
	case IG_PRISM: return "Prism";
	case IG_POLYHEDRON: return "Polyhedron";
	default: return "";
	}
}

inline constexpr bool GetCellTypeIsFixed(int type) {
	switch (type)
	{
	case IG_EMPTY_CELL: return false;
	case IG_VERTEX: return true;
	case IG_LINE: return true;
	case IG_FACE: return false;
	case IG_TRIANGLE: return true;
	case IG_QUAD: return true;
	case IG_POLYGON: return false;
	case IG_VOLUME: return false;
	case IG_TETRA: return true;
	case IG_HEXAHEDRON: return true;
	case IG_PYRAMID: return true;
	case IG_PRISM: return true;
	case IG_POLYHEDRON: return false;
	default: return false;
	}
}

inline constexpr int GetCellSize(int type) {
	switch (type)
	{
	case IG_EMPTY_CELL: return -1;
	case IG_VERTEX: return 1;
	case IG_LINE: return 2;
	case IG_FACE: return -1;
	case IG_TRIANGLE: return 3;
	case IG_QUAD: return 4;
	case IG_POLYGON: return -1;
	case IG_VOLUME: return -1;
	case IG_TETRA: return 4;
	case IG_HEXAHEDRON: return 6;
	case IG_PYRAMID: return 5;
	case IG_PRISM: return 6;
	case IG_POLYHEDRON: return -1;
	default: return -1;
	}
}

IGAME_NAMESPACE_END
#endif