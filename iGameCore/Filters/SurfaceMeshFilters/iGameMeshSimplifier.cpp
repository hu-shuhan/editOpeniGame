#include "iGameMeshSimplifier.h"

IGAME_NAMESPACE_BEGIN

class MeshSimplifier::TriMeshInternalSimplifier {
public:
    TriMeshInternalSimplifier(const std::vector<int_t>& Indices, const std::vector<Point3>& VertexPositions,
                              const std::vector<Attribute>& VertexAttributes,
                              const std::vector<float>& AttributeWeights, size_t AttributeCount, size_t TargetCount,
                              float TargetError);

    std::vector<int_t> Result;
    const std::vector<int_t>& Indices;
    const std::vector<Point3>& VertexPositions;
    const std::vector<Attribute>& VertexAttributes;
    const std::vector<float>& AttributeWeights;
    size_t AttributeCount;
    size_t TargetCount;
    float TargetError;
};

MeshSimplifier::TriMeshInternalSimplifier::TriMeshInternalSimplifier(const std::vector<int_t>& Indices,
                                                                     const std::vector<Point3>& VertexPositions,
                                                                     const std::vector<Attribute>& VertexAttributes,
                                                                     const std::vector<float>& AttributeWeights,
                                                                     size_t AttributeCount,
                                                                     size_t TargetCount, float TargetError)
    : Indices(Indices), VertexPositions(VertexPositions), VertexAttributes(VertexAttributes),
      AttributeWeights(AttributeWeights), AttributeCount(AttributeCount) {

}

bool MeshSimplifier::Execute() { return false; }

MeshSimplifier::MeshSimplifier() { SetNumberOfInputs(1); }

IGAME_NAMESPACE_END


