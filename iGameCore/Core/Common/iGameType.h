#ifndef iGameType_h
#define iGameType_h

#include <iGameAbi.h>
#include <stdint.h>
#define IGAME_SIZEOF_CHAR KWIML_ABI_SIZEOF_CHAR
#define IGAME_SIZEOF_SHORT KWIML_ABI_SIZEOF_SHORT
#define IGAME_SIZEOF_INT KWIML_ABI_SIZEOF_INT
#define IGAME_SIZEOF_LONG KWIML_ABI_SIZEOF_LONG
#define IGAME_SIZEOF_LONG_LONG KWIML_ABI_SIZEOF_LONG_LONG
#define IGAME_SIZEOF_FLOAT KWIML_ABI_SIZEOF_FLOAT
#define IGAME_SIZEOF_DOUBLE KWIML_ABI_SIZEOF_DOUBLE
#define IGAME_SIZEOF_VOID_P KWIML_ABI_SIZEOF_DATA_PTR

inline constexpr const char* FILE_NAME = "FILE_NAME";
inline constexpr const char* FILE_PATH = "FILE_PATH";
inline constexpr const char* FILE_SUFFIX = "FILE_SUFFIX";
inline constexpr const char* ATTRIBUTE_NAME_ARRAY = "ATTRIBUTE_NAME_ARRAY";
inline constexpr const char* TIME_FRAME_ARRAY = "TIME_FRAME_ARRAY";

enum ItemSource { File = 0, Algorithm };

// DataObjectType
inline constexpr int IG_NONE = -1;
enum {
    IG_DATA_OBJECT = 0,
    IG_COMPOSITE_DATA_OBJECT,
    IG_DRAW_OBJECT,
    IG_POINT_SET,
    IG_SURFACE_MESH,
    IG_VOLUME_MESH,
    IG_UNSTRUCTURED_MESH,
    IG_STRUCTURED_MESH,
    IG_MULTIBLOCK_MESH,
    IG_DATA_OBJECT_COUNT,
};

// ArrayType
enum {
    IG_ARRAY_OBJECT = 0,
    IG_ELEMENT_ARRAY,
};

// FlatArrayType
enum {
    IG_FLAT_ARRAY = 0,
    IG_FloatArray,
    IG_DoubleArray,
    IG_IntArray,
    IG_INTARRAY,
    IG_UnsignedIntArray,
    IG_CharArray,
    IG_UnsignedCharArray,
    IG_ShortArray,
    IG_UnsignedShortArray,
    IG_LongLongArray,
    IG_UnsignedLongLongArray,

};


// DrawMode
enum {
    IG_POINTS = 1 << 0,
    IG_WIREFRAME = 1 << 1,
    IG_SURFACE = 1 << 2,
    IG_VOLUME = 1 << 3,
    IG_DRAW_MODE_COUNT,
};

// AttributeType
enum AttributeType {
    IG_SCALAR = 0,
    IG_VECTOR,
    IG_NORMAL,
    IG_TCOORD,
    IG_TENSOR,
    IG_ATTRIBUTE_COUNT,
};

// AttributeWithElementType
enum {
    IG_POINT = 0,
    IG_CELL,
};

//FileType
enum { IGAME_NONE = 0, IGAME_ASCII = 1, IGAME_BINARY = 2 };


inline constexpr int IG_VOID = 0;
inline constexpr int IG_CHAR = 1;
inline constexpr int IG_UNSIGNED_CHAR = 2;
inline constexpr int IG_SHORT = 3;
inline constexpr int IG_UNSIGNED_SHORT = 4;
inline constexpr int IG_INT = 5;
inline constexpr int IG_UNSIGNED_INT = 6;
inline constexpr int IG_LONG_LONG = 7;
inline constexpr int IG_UNSIGNED_LONG_LONG = 8;
inline constexpr int IG_FLOAT = 9;
inline constexpr int IG_DOUBLE = 10;
inline constexpr int IG_INDEX = 11;

inline constexpr int IG_INT8 = 1;
inline constexpr int IG_UINT8 = 2;
inline constexpr int IG_INT16 = 3;
inline constexpr int IG_UINT16 = 4;
inline constexpr int IG_INT32 = 5;
inline constexpr int IG_UINT32 = 6;
inline constexpr int IG_INT64 = 7;
inline constexpr int IG_UINT64 = 8;

using igIndex = int;
using iguIndex = unsigned int;
using igIndex64 = long long;
using iguIndex64 = unsigned long long;
using igIndex16 = short;
using iguIndex16 = unsigned short;

using IGenum = long;
using IGsize = unsigned long long;
using IGuint = unsigned int;
using IGint = int;

using DataObjectType = IGenum;
using DataObjectId = int;
//using IGrange = std::pair<float, float>;
enum Color {
    None = 0,
    Red,
    Green,
    Blue,
    LightBlue,
    White,
    Black,
    Gray,
    DarkGray,
    LightGray,
    Yellow,
    Magenta,
    Cyan,
    Orange,
    Purple,
    Pink,
    Brown,
    Gold
};
#endif