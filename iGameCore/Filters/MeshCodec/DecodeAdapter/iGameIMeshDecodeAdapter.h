#ifndef IMeshDecoderAdapter_h
#define IMeshDecoderAdapter_h

#include "iGameMacro.h"
#include "iGameType.h"
#include "MeshCodec/Utils/iGameCodecPayload.h"
#include <vector>

IGAME_NAMESPACE_BEGIN

class IMeshDecodeAdapterBase
{
public:
    virtual ~IMeshDecodeAdapterBase() = default;

    virtual void ResetOutput() = 0;

    virtual void SetMeshType(IGenum meshType) = 0;

    virtual void SetPointBuffer(const std::vector<float>& input) = 0;

    virtual void AddAttribute(const AttributeBuffer& attr) = 0;

    virtual void AddSameTypePolyCells(std::vector<uint32_t>& ids, const std::vector<uint32_t>& cellSizes) = 0;

    virtual void AddSameTypeFixedCells(std::vector<uint32_t>& ids, int cellSize) = 0;

    virtual void AddUnstructuredFixedCells(std::vector<uint32_t>& ids, int cellSize, std::vector<uint32_t>& types) = 0;

    virtual void AddUnstructuredPolyCells(
        std::vector<uint32_t>& ids,
        const std::vector<uint32_t>& cellSizes,
        std::vector<uint32_t>& types) = 0;

    virtual void AddStructureCells(std::vector<uint32_t>& ids, int axisSize[3]) = 0;

    virtual void SetStructuredMeshDimension(int axisSize[3]) = 0;

    virtual void AddSecondaryIndexCells(
        std::vector<unsigned int> volume2facesIndex,
        std::vector<unsigned int> volume2facesSize,
        std::vector<unsigned int> face2pointsIndex,
        std::vector<unsigned int> face2pointsSize) = 0;
};

template<typename OutputType>
class IMeshDecodeAdapter : public IMeshDecodeAdapterBase
{
public:
    virtual OutputType GetOutput() = 0;
};

IGAME_NAMESPACE_END

#endif
