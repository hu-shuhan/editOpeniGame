#ifndef IMeshEncoderAdapter_h
#define IMeshEncoderAdapter_h

#include "iGameMacro.h"
#include "iGamePoints.h"
#include "iGameIdArray.h"
#include "iGameFlatArray.h"
#include <vector>

IGAME_NAMESPACE_BEGIN

class IMeshEncodeAdapter
{
public:
    virtual ~IMeshEncodeAdapter() = default;

    virtual IGenum GetMeshType() = 0;

    virtual IGsize GetCellIdOffsetSize() = 0;

    virtual IGsize GetNumberOfFaces() = 0;

    virtual IGsize GetNumberOfCells() = 0;

    virtual IGsize GetNumberOfPoints() = 0;

    virtual bool IsFixedCellSize() = 0;

    virtual bool IsSecondaryIndexPolyhedronMesh() = 0;

    virtual void SplitSecondaryIndex(
        std::vector<unsigned int>& volume2facesIndex,
        std::vector<unsigned int>& volume2facesOffset,
        std::vector<unsigned int>& face2pointsIndex,
        std::vector<unsigned int>& face2pointsOffset) = 0;

    virtual igIndex GetFaceId(igIndex* ids, int size) = 0;

    virtual Points::Pointer GetPoints() = 0;

    virtual int GetFixedCellSize() = 0;

    virtual IGsize GetCellIdBufferSize() = 0;

    virtual UnsignedIntArray::Pointer GetCellIdOffset() = 0;

    virtual IdArray::Pointer GetCellIdBuffer() = 0;

    virtual UnsignedIntArray::Pointer GetCellTypes() = 0;
};

IGAME_NAMESPACE_END

#endif
