#include "iGameTriangleStripFilter.h"

#include <algorithm>
#include <limits>
#include <utility>

IGAME_NAMESPACE_BEGIN

namespace {

template<typename ArrayT>
ArrayObject::Pointer CopyArrayTuples(ArrayObject::Pointer input,
                                     const std::vector<igIndex>& tupleIds) {
    auto source = DynamicCast<ArrayT>(input);
    if (!source) return nullptr;

    auto output = ArrayT::New();
    output->SetName(source->GetName());
    output->SetDimension(source->GetDimension());
    output->Reserve(static_cast<IGsize>(tupleIds.size()));

    const int dimension = source->GetDimension();
    const auto* values = source->RawPointer();
    for (const igIndex tupleId: tupleIds) {
        if (tupleId < 0 || static_cast<IGsize>(tupleId) >= source->GetNumberOfElements()) {
            return nullptr;
        }
        const IGsize offset = static_cast<IGsize>(tupleId) * static_cast<IGsize>(dimension);
        for (int component = 0; component < dimension; ++component) {
            output->AddValue(values[offset + static_cast<IGsize>(component)]);
        }
    }
    return output;
}

ArrayObject::Pointer CopyArrayTuplesByType(ArrayObject::Pointer input,
                                           const std::vector<igIndex>& tupleIds) {
    if (!input) return nullptr;
    switch (input->GetArrayType()) {
        case IG_FloatArray:            return CopyArrayTuples<FloatArray>(input, tupleIds);
        case IG_DoubleArray:           return CopyArrayTuples<DoubleArray>(input, tupleIds);
        case IG_IntArray:              return CopyArrayTuples<IntArray>(input, tupleIds);
        case IG_UnsignedIntArray:      return CopyArrayTuples<UnsignedIntArray>(input, tupleIds);
        case IG_CharArray:             return CopyArrayTuples<CharArray>(input, tupleIds);
        case IG_UnsignedCharArray:     return CopyArrayTuples<UnsignedCharArray>(input, tupleIds);
        case IG_ShortArray:            return CopyArrayTuples<ShortArray>(input, tupleIds);
        case IG_UnsignedShortArray:    return CopyArrayTuples<UnsignedShortArray>(input, tupleIds);
        case IG_LongLongArray:         return CopyArrayTuples<LongLongArray>(input, tupleIds);
        case IG_UnsignedLongLongArray: return CopyArrayTuples<UnsignedLongLongArray>(input, tupleIds);
        default:                       return nullptr;
    }
}

} // namespace

TriangleStripFilter::TriangleStripFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void TriangleStripFilter::SetMaximumLength(int length) { m_MaximumLength = std::max(1, length); }
IGsize TriangleStripFilter::GetNumberOfStrips() const noexcept { return m_Strips ? m_Strips->GetNumberOfCells() : 0; }

IGsize TriangleStripFilter::GetNumberOfOutputCells() const noexcept {
    const IGsize stripCount = m_Strips ? m_Strips->GetNumberOfCells() : 0;
    const IGsize polygonCount = m_PassThroughPolys
            ? m_PassThroughPolys->GetNumberOfCells()
            : 0;
    const IGsize lineCount = m_PolyLines ? m_PolyLines->GetNumberOfCells() : 0;
    return stripCount + polygonCount + lineCount;
}

bool TriangleStripFilter::ReadOutputStrips(
        DataObject::Pointer output, CellArray::Pointer& strips,
        CellArray::Pointer& stripSourceFaceIds) {
    strips = nullptr;
    stripSourceFaceIds = nullptr;
    if (!output || !output->GetMetadata()) return false;

    auto* metadata = output->GetMetadata();
    const auto& entries = metadata->entries();
    auto findIntArray = [&entries](const char* name) -> IntArray::Pointer {
        const auto it = entries.find(name);
        if (it == entries.end()) return nullptr;
        const auto* value = std::get_if<IntArray::Pointer>(&it->second);
        return value ? *value : nullptr;
    };

    auto stripOffsets = findIntArray(StripOffsetsMetadataName);
    auto stripPointIds = findIntArray(StripPointIdsMetadataName);
    auto sourceOffsets = findIntArray(StripSourceFaceOffsetsMetadataName);
    auto sourceFaceIds = findIntArray(StripSourceFaceIdsMetadataName);
    if (!stripOffsets || !stripPointIds || !sourceOffsets || !sourceFaceIds ||
        stripOffsets->GetNumberOfValues() == 0 ||
        stripOffsets->GetNumberOfValues() != sourceOffsets->GetNumberOfValues()) {
        return false;
    }

    const auto* pointOffsetValues = stripOffsets->RawPointer();
    const auto* sourceOffsetValues = sourceOffsets->RawPointer();
    const auto* pointIdValues = stripPointIds->RawPointer();
    const auto* sourceFaceIdValues = sourceFaceIds->RawPointer();
    const IGsize offsetCount = stripOffsets->GetNumberOfValues();
    if (stripPointIds->GetNumberOfValues() >
                static_cast<IGsize>(std::numeric_limits<int>::max()) ||
        sourceFaceIds->GetNumberOfValues() >
                static_cast<IGsize>(std::numeric_limits<int>::max()) ||
        pointOffsetValues[0] != 0 || sourceOffsetValues[0] != 0 ||
        pointOffsetValues[offsetCount - 1] !=
                static_cast<int>(stripPointIds->GetNumberOfValues()) ||
        sourceOffsetValues[offsetCount - 1] !=
                static_cast<int>(sourceFaceIds->GetNumberOfValues())) {
        return false;
    }

    auto restoredStrips = CellArray::New();
    auto restoredSourceFaceIds = CellArray::New();
    for (IGsize stripId = 0; stripId + 1 < offsetCount; ++stripId) {
        const int pointBegin = pointOffsetValues[stripId];
        const int pointEnd = pointOffsetValues[stripId + 1];
        const int sourceBegin = sourceOffsetValues[stripId];
        const int sourceEnd = sourceOffsetValues[stripId + 1];
        if (pointBegin < 0 || pointEnd < pointBegin || sourceBegin < 0 ||
            sourceEnd < sourceBegin || pointEnd - pointBegin < 3 ||
            sourceEnd - sourceBegin != pointEnd - pointBegin - 2) {
            return false;
        }
        restoredStrips->AddCellIds(pointIdValues + pointBegin,
                                   pointEnd - pointBegin);
        restoredSourceFaceIds->AddCellIds(sourceFaceIdValues + sourceBegin,
                                          sourceEnd - sourceBegin);
    }

    strips = restoredStrips;
    stripSourceFaceIds = restoredSourceFaceIds;
    return true;
}

bool TriangleStripFilter::Execute() {
    ResetWorkingState();
    if (!PrepareInput()) return false;
    if (!BuildTriangleAdjacency()) return false;
    if (!BuildTriangleStrips()) return false;
    if (!BuildPolyLines()) return false;

    if (m_JoinContiguousSegments) { JoinContiguousPolyLines(); }

    return BuildOutputDataObject();
}

void TriangleStripFilter::ResetWorkingState() {
    m_SourceInput = nullptr;
    m_InputMesh = nullptr;
    m_UnstructuredInput = nullptr;

    m_Strips = CellArray::New();
    m_PassThroughPolys = CellArray::New();
    m_PolyLines = CellArray::New();

    m_FaceMarks.clear();
    m_StripSourceFaceIds.clear();
    m_PassThroughPolySourceFaceIds.clear();
    m_LongestStripLength = 0;
}

bool TriangleStripFilter::PrepareInput() {
    m_SourceInput = GetInput(0);
    if (!m_SourceInput) return false;
    switch (m_SourceInput->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            m_InputMesh = DynamicCast<SurfaceMesh>(m_SourceInput);
            break;
        case IG_UNSTRUCTURED_MESH: {
            m_UnstructuredInput = DynamicCast<UnstructuredMesh>(m_SourceInput);
            if (!m_UnstructuredInput || !m_UnstructuredInput->GetCells()) {
                igError("TriangleStripFilter received an invalid UnstructuredMesh. ");
                return false;
            }

            bool hasExplicitLines = false;
            for (igIndex cellId = 0;
                 cellId < m_UnstructuredInput->GetNumberOfCells(); ++cellId) {
                const IGenum cellType = m_UnstructuredInput->GetCellType(cellId);
                if (cellType == IG_LINE || cellType == IG_POLY_LINE) {
                    hasExplicitLines = true;
                } else if (Cell::GetCellDimension(cellType) != 2) {
                    igError("TriangleStripFilter only accepts surface cells and explicit lines. ");
                    return false;
                }
            }

            // Preserve the original surface-only path and its AttributeSet
            // behavior. Additional extraction/remapping is only required for
            // mixed surface/line inputs, which SurfaceMesh cannot represent.
            if (!hasExplicitLines) {
                m_InputMesh = m_UnstructuredInput->TransferToSurfaceMesh();
                if (!m_InputMesh) {
                    igError("TriangleStripFilter requires surface cells. ");
                    return false;
                }
                break;
            }

            auto faces = CellArray::New();
            std::vector<igIndex> sourceFaceIds;
            for (igIndex cellId = 0;
                 cellId < m_UnstructuredInput->GetNumberOfCells(); ++cellId) {
                const IGenum cellType = m_UnstructuredInput->GetCellType(cellId);
                const int dimension = Cell::GetCellDimension(cellType);
                if (dimension == 2) {
                    const igIndex* pointIds = nullptr;
                    const int pointCount = m_UnstructuredInput->GetCells()->GetCellIds(
                            cellId, pointIds);
                    if (!pointIds || pointCount < 3) {
                        igError("TriangleStripFilter found an invalid surface cell. ");
                        return false;
                    }
                    faces->AddCellIds(pointIds, pointCount);
                    sourceFaceIds.push_back(cellId);
                } else if (cellType != IG_LINE && cellType != IG_POLY_LINE) {
                    igError("TriangleStripFilter only accepts surface cells and explicit lines. ");
                    return false;
                }
            }
            if (faces->GetNumberOfCells() == 0) {
                igError("TriangleStripFilter requires at least one surface cell. ");
                return false;
            }

            m_InputMesh = SurfaceMesh::New();
            m_InputMesh->SetName(m_UnstructuredInput->GetName());
            m_InputMesh->SetPoints(m_UnstructuredInput->GetPoints());
            m_InputMesh->SetFaces(faces);

            // A mixed unstructured input may interleave lines and faces. Build
            // face-only CellData so local face IDs remain valid for strip
            // generation and output attribute remapping.
            auto surfaceAttributes = AttributeSet::New();
            if (auto* inputAttributes = m_UnstructuredInput->GetAttributeSet()) {
                for (IGsize attributeId = 0;
                     attributeId < inputAttributes->GetNumberOfAttributes();
                     ++attributeId) {
                    auto& attribute = inputAttributes->GetAttribute(attributeId);
                    if (attribute.isDeleted || !attribute.pointer) continue;
                    if (attribute.attachmentType == IG_POINT) {
                        surfaceAttributes->AddAttribute(
                                attribute.type, IG_POINT, attribute.pointer,
                                attribute.GetDataRange());
                    } else if (attribute.attachmentType == IG_CELL) {
                        auto faceArray = CopyArrayTuplesByType(
                                attribute.pointer, sourceFaceIds);
                        if (!faceArray) {
                            igError("TriangleStripFilter could not extract face CellData. ");
                            return false;
                        }
                        surfaceAttributes->AddAttribute(
                                attribute.type, IG_CELL, faceArray,
                                attribute.GetDataRange());
                    }
                }
            }
            m_InputMesh->SetAttributeSet(surfaceAttributes);
            break;
        }
        default:
            igError("TriangleStripFilter only supports SurfaceMesh or surface/line UnstructuredMesh. ");
            return false;
    }
    return m_InputMesh != nullptr;
}

bool TriangleStripFilter::BuildTriangleAdjacency() {
    if (!m_InputMesh) return false;
    m_InputMesh->BuildEdges();
    m_InputMesh->BuildFaceEdgeLinks();
    m_FaceMarks.assign(static_cast<size_t>(m_InputMesh->GetNumberOfFaces()), FaceMark::Free);
    return true;
}

bool TriangleStripFilter::BuildTriangleStrips() {
    if (!m_InputMesh) return false;
    const IGsize numFaces = m_InputMesh->GetNumberOfFaces();
    for (igIndex faceId = 0; faceId < numFaces; ++faceId) {
        if (m_FaceMarks[static_cast<size_t>(faceId)] != FaceMark::Free) continue;
        if (!IsTriangleFace(faceId)) {
            PassThroughPolygon(faceId);
            m_FaceMarks[static_cast<size_t>(faceId)] = FaceMark::Committed;
            continue;
        }
        StripCandidate candidate = FindBestStrip(static_cast<igIndex>(faceId));
        if (candidate.Empty()) return true;
        CommitStrip(std::move(candidate));
    }
    return true;
}

TriangleStripFilter::StripCandidate TriangleStripFilter::FindBestStrip(igIndex seedFaceId) {
    StripCandidate singleTriangle;
    for (int localEdge = 0; localEdge < 3; ++localEdge) {
        OrientedEdge start{seedFaceId, localEdge};
        StripCandidate candidate = TraceStrip(start);
        if (candidate.GetTriangleCount() > 1) {
            // vtkStripper accepts the first edge, in face point order, that
            // reaches an unvisited triangle.  Choosing the longest of all
            // three trials changes later seed availability and therefore the
            // final output-cell count on larger meshes.
            return candidate;
        }
        if (localEdge == 0) { singleTriangle = std::move(candidate); }
    }
    return singleTriangle;
}

TriangleStripFilter::StripCandidate TriangleStripFilter::TraceStrip(const OrientedEdge& startEdge) {
    StripCandidate candidate;
    candidate.StartEdge = startEdge;
    if (!startEdge.IsValid()) return candidate;
    if (!IsTriangleFace(startEdge.FaceId)) return candidate;

    std::array<igIndex, 3> seedPoints{};
    if (!GetTrianglePointIds(startEdge.FaceId, seedPoints)) { return candidate; }

    std::vector<igIndex> trail;
    if (!AddFaceToTrial(startEdge.FaceId, trail)) { return candidate; }

    const int i = startEdge.LocalEdge;

    // 循环旋转，不改变种子三角形绕序。
    candidate.PointIds.push_back(seedPoints[(i + 2) % 3]);
    candidate.PointIds.push_back(seedPoints[i]);
    candidate.PointIds.push_back(seedPoints[(i + 1) % 3]);
    candidate.FaceIds.push_back(startEdge.FaceId);

    igIndex currentFaceId = startEdge.FaceId;

    while (candidate.GetTriangleCount() < static_cast<IGsize>(m_MaximumLength)) {
        const size_t size = candidate.PointIds.size();
        const igIndex edgePoint0 = candidate.PointIds[size - 2];
        const igIndex edgePoint1 = candidate.PointIds[size - 1];

        // 先按 strip 尾部方向查找；若面内方向相反则反向查找。
        OrientedEdge currentEdge = FindOrientedEdge(currentFaceId, edgePoint0, edgePoint1);
        if (!currentEdge.IsValid()) { currentEdge = FindOrientedEdge(currentFaceId, edgePoint1, edgePoint0); }
        if (!currentEdge.IsValid()) break;

        const igIndex edgeId = GetMeshEdgeId(currentEdge);
        if (edgeId < 0) break;

        const igIndex neighborFaceId = FindAvailableNeighbor(edgeId, currentFaceId);
        if (neighborFaceId < 0) break;

        const igIndex thirdPoint = FindThirdPoint(neighborFaceId, edgePoint0, edgePoint1);
        if (thirdPoint < 0) break;
        if (thirdPoint == edgePoint0 || thirdPoint == edgePoint1) { break; }
        if (!AddFaceToTrial(neighborFaceId, trail)) { break; }

        candidate.PointIds.push_back(thirdPoint);
        candidate.FaceIds.push_back(neighborFaceId);
        currentFaceId = neighborFaceId;
    }
    FreeTrial(trail);
    return candidate;
}

bool TriangleStripFilter::AddFaceToTrial(igIndex faceId, std::vector<igIndex>& trail) {
    if (faceId < 0 || static_cast<size_t>(faceId) >= m_FaceMarks.size()) { return false; }
    if (m_FaceMarks[faceId] != FaceMark::Free) { return false; }

    m_FaceMarks[faceId] = FaceMark::Trial;
    trail.push_back(faceId);
    return true;
}

void TriangleStripFilter::FreeTrial(std::vector<igIndex>& trail) {
    for (igIndex faceId: trail) {
        if (m_FaceMarks[faceId] == FaceMark::Trial) { m_FaceMarks[faceId] = FaceMark::Free; }
    }
    trail.clear();
}

void TriangleStripFilter::CommitStrip(StripCandidate&& candidate) {
    if (candidate.Empty()) return;
    if (candidate.PointIds.size() < 3) return;
    m_Strips->AddCellIds(candidate.PointIds.data(), static_cast<int>(candidate.PointIds.size()));
    std::vector<igIndex> sourceFaceIds;
    sourceFaceIds.reserve(candidate.FaceIds.size());
    for (igIndex faceId: candidate.FaceIds) {
        m_FaceMarks[faceId] = FaceMark::Committed;
        sourceFaceIds.push_back(faceId);
    }
    m_LongestStripLength = std::max(m_LongestStripLength, candidate.GetTriangleCount());
    m_StripSourceFaceIds.push_back(std::move(sourceFaceIds));
}

bool TriangleStripFilter::IsTriangleFace(igIndex faceId) const {
    if (!m_InputMesh || faceId < 0) return false;
    return m_InputMesh->GetFaces()->GetCellSize(faceId) == 3;
}

bool TriangleStripFilter::GetTrianglePointIds(igIndex faceId, std::array<igIndex, 3>& pointIds) const {
    if (!IsTriangleFace(faceId)) return false;

    const igIndex* ids = nullptr;
    const int count = m_InputMesh->GetFaces()->GetCellIds(faceId, ids);
    if (count != 3 || !ids) return false;
    pointIds[0] = ids[0];
    pointIds[1] = ids[1];
    pointIds[2] = ids[2];
    return true;
}

bool TriangleStripFilter::GetOrientedEdgePointIds(const OrientedEdge& edge, igIndex& origin,
                                                  igIndex& destination) const {
    std::array<igIndex, 3> ids{};
    if (!edge.IsValid() || !GetTrianglePointIds(edge.FaceId, ids))  return false;
    origin = ids[edge.LocalEdge];
    destination = ids[(edge.LocalEdge + 1) % 3];
    return true;
}

igIndex TriangleStripFilter::GetMeshEdgeId(const OrientedEdge& edge) const {
    if (!edge.IsValid() || !m_InputMesh) return -1;
    igIndex edgeIds[3]{};
    const int count = m_InputMesh->GetFaceEdgeIds(edge.FaceId, edgeIds);
    if (count != 3) return -1;
    return edgeIds[edge.LocalEdge];
}

igIndex TriangleStripFilter::FindAvailableNeighbor(igIndex edgeId, igIndex currentFaceId) const {
    if (!m_InputMesh || edgeId < 0) return -1;

    const igIndex* faceIds = nullptr;
    int faceCount = 0;
    m_InputMesh->GetEdgeToNeighborFaces(edgeId, faceIds, faceCount);
    for (int i = 0; i < faceCount; ++i) {
        const igIndex candidate = faceIds[i];
        if (candidate == currentFaceId) continue;
        // vtkStripper consults the first edge neighbor returned by the mesh.
        // It does not select another neighbor when that first one is already
        // visited or is not a triangle (relevant on non-manifold edges).
        if (!IsTriangleFace(candidate) ||
            m_FaceMarks[candidate] != FaceMark::Free) {
            return -1;
        }
        return candidate;
    }
    return -1;
}

igIndex TriangleStripFilter::FindThirdPoint(igIndex faceId, igIndex edgePoint0, igIndex edgePoint1) const {
    std::array<igIndex, 3> ids{};
    if (!GetTrianglePointIds(faceId, ids)) return -1;

    bool has0 = false;
    bool has1 = false;
    igIndex third = -1;
    for (igIndex id: ids) {
        if (id == edgePoint0)
            has0 = true;
        else if (id == edgePoint1)
            has1 = true;
        else
            third = id;
    }
    return has0 && has1 ? third : -1;
}

TriangleStripFilter::OrientedEdge TriangleStripFilter::FindOrientedEdge(igIndex faceId, igIndex origin,
                                                                        igIndex destination) const {
    std::array<igIndex, 3> ids{};
    if (!GetTrianglePointIds(faceId, ids)) { return {}; }
    for (int i = 0; i < 3; ++i)
        if (ids[i] == origin && ids[(i + 1) % 3] == destination)
            return {faceId, i};
    return {};
}

void TriangleStripFilter::PassThroughPolygon(igIndex faceId) {
    if (!m_InputMesh || !m_PassThroughPolys) return;
    const igIndex* pointIds = nullptr;
    const int count = m_InputMesh->GetFaces()->GetCellIds(faceId, pointIds);
    if (count >= 3 && pointIds) {
        m_PassThroughPolys->AddCellIds(pointIds, count);
        m_PassThroughPolySourceFaceIds.push_back(faceId);
    }
}

void TriangleStripFilter::JoinContiguousPolyLines() {
    if (!m_PolyLines) return;

    const IGsize numLines = m_PolyLines->GetNumberOfCells();
    if (numLines < 2) return;

    // Copy the source cells first. Replacing m_PolyLines after the join must
    // not invalidate point-id pointers while they are still being inspected.
    std::vector<std::vector<igIndex>> sourceLines;
    sourceLines.reserve(static_cast<std::size_t>(numLines));

    for (igIndex lineId = 0; lineId < numLines; ++lineId) {
        const igIndex* pointIds = nullptr;
        const int pointCount = m_PolyLines->GetCellIds(lineId, pointIds);

        if (pointCount > 0 && pointIds) {
            sourceLines.emplace_back(pointIds, pointIds + pointCount);
        } else {
            sourceLines.emplace_back();
        }
    }

    std::vector<bool> used(sourceLines.size(), false);
    CellArray::Pointer joinedLines = CellArray::New();

    for (std::size_t seedId = 0; seedId < sourceLines.size(); ++seedId) {
        if (used[seedId]) continue;

        const auto& seed = sourceLines[seedId];
        used[seedId] = true;

        // A valid polyline has at least two points. Preserve a non-empty
        // malformed cell instead of silently dropping user data.
        if (seed.size() < 2) {
            if (!seed.empty()) {
                joinedLines->AddCellIds(seed.data(),
                                        static_cast<int>(seed.size()));
            }
            continue;
        }

        std::vector<igIndex> chain = seed;

        // Grow both ends of the current chain. This covers all four endpoint
        // combinations and reverses the candidate when its orientation is
        // opposite to the direction required by the chain.
        while (true) {
            bool extended = false;
            const igIndex chainHead = chain.front();
            const igIndex chainTail = chain.back();

            for (std::size_t candidateId = 0;
                 candidateId < sourceLines.size(); ++candidateId) {
                if (used[candidateId]) continue;

                const auto& candidate = sourceLines[candidateId];
                if (candidate.size() < 2) continue;

                const igIndex candidateHead = candidate.front();
                const igIndex candidateTail = candidate.back();

                if (chainTail == candidateHead) {
                    // chain ----> candidate ---->
                    chain.insert(chain.end(), candidate.begin() + 1,
                                 candidate.end());
                } else if (chainTail == candidateTail) {
                    // chain ----> <---- candidate
                    for (std::size_t i = candidate.size() - 1; i > 0; --i) {
                        chain.push_back(candidate[i - 1]);
                    }
                } else if (chainHead == candidateTail) {
                    // candidate ----> chain ---->
                    std::vector<igIndex> merged;
                    merged.reserve(candidate.size() + chain.size() - 1);
                    merged.insert(merged.end(), candidate.begin(),
                                  candidate.end() - 1);
                    merged.insert(merged.end(), chain.begin(), chain.end());
                    chain.swap(merged);
                } else if (chainHead == candidateHead) {
                    // candidate <---- chain ---->
                    std::vector<igIndex> merged;
                    merged.reserve(candidate.size() + chain.size() - 1);
                    for (std::size_t i = candidate.size(); i > 1; --i) {
                        merged.push_back(candidate[i - 1]);
                    }
                    merged.insert(merged.end(), chain.begin(), chain.end());
                    chain.swap(merged);
                } else {
                    continue;
                }

                used[candidateId] = true;
                extended = true;
                break;
            }

            if (!extended) break;
        }

        joinedLines->AddCellIds(chain.data(), static_cast<int>(chain.size()));
    }

    joinedLines->Squeeze();
    m_PolyLines = joinedLines;
}

bool TriangleStripFilter::BuildPolyLines() {
    if (!m_PolyLines) return false;

    // vtkStripper processes line cells already present in vtkPolyData; it does
    // not turn open polygon boundaries into new lines. SurfaceMesh has no line
    // cell container, so only an UnstructuredMesh input can contribute here.
    if (!m_UnstructuredInput) return true;

    auto inputCells = m_UnstructuredInput->GetCells();
    if (!inputCells) return false;
    for (igIndex cellId = 0;
         cellId < m_UnstructuredInput->GetNumberOfCells(); ++cellId) {
        const IGenum cellType = m_UnstructuredInput->GetCellType(cellId);
        if (cellType != IG_LINE && cellType != IG_POLY_LINE) continue;

        const igIndex* pointIds = nullptr;
        const int pointCount = inputCells->GetCellIds(cellId, pointIds);
        if (!pointIds || pointCount < 2) {
            igError("TriangleStripFilter found an invalid input line cell. ");
            return false;
        }
        m_PolyLines->AddCellIds(pointIds, pointCount);
    }
    return true;
}

bool TriangleStripFilter::BuildOutputDataObject() {
    auto output = SurfaceMesh::New();
    auto faces = CellArray::New();
    auto stripOffsets = IntArray::New();
    auto stripPointIds = IntArray::New();
    auto stripSourceFaceOffsets = IntArray::New();
    auto stripSourceFaceIds = IntArray::New();
    std::vector<igIndex> outputSourceFaceIds;
    outputSourceFaceIds.reserve(static_cast<std::size_t>(m_InputMesh->GetNumberOfFaces()));

    stripOffsets->AddValue(0);
    stripSourceFaceOffsets->AddValue(0);

    output->SetName(m_InputMesh->GetName());
    output->SetPoints(m_InputMesh->GetPoints());
    // Faces remain expanded for the existing renderer and filters. Native
    // strip topology is flattened into the output Metadata below.
    for (IGsize stripId = 0; stripId < m_Strips->GetNumberOfCells(); ++stripId) {
        const igIndex* ids = nullptr;
        const int count = m_Strips->GetCellIds(stripId, ids);
        if (!ids || count < 3 ||
            static_cast<std::size_t>(stripId) >= m_StripSourceFaceIds.size() ||
            m_StripSourceFaceIds[static_cast<std::size_t>(stripId)].size() !=
                    static_cast<std::size_t>(count - 2)) {
            igError("TriangleStripFilter has an invalid strip/source-face mapping. ");
            return false;
        }
        const auto& sourceFaceIds = m_StripSourceFaceIds[static_cast<std::size_t>(stripId)];
        for (int i = 0; i < count; ++i) { stripPointIds->AddValue(ids[i]); }
        for (const igIndex sourceFaceId: sourceFaceIds) {
            stripSourceFaceIds->AddValue(sourceFaceId);
        }
        if (stripPointIds->GetNumberOfValues() >
                    static_cast<IGsize>(std::numeric_limits<int>::max()) ||
            stripSourceFaceIds->GetNumberOfValues() >
                    static_cast<IGsize>(std::numeric_limits<int>::max())) {
            igError("TriangleStripFilter metadata exceeds 32-bit offset storage. ");
            return false;
        }
        stripOffsets->AddValue(
                static_cast<int>(stripPointIds->GetNumberOfValues()));
        stripSourceFaceOffsets->AddValue(
                static_cast<int>(stripSourceFaceIds->GetNumberOfValues()));
        for (int i = 0; i + 2 < count; ++i) {
            igIndex tri[3] = {ids[i], ids[i + 1], ids[i + 2]};
            if (i % 2 == 1) { std::swap(tri[0], tri[1]); }
            faces->AddCellIds(tri, 3);
            outputSourceFaceIds.push_back(sourceFaceIds[static_cast<std::size_t>(i)]);
        }
    }

    // 追加未处理的 polygon。
    if (m_PassThroughPolySourceFaceIds.size() !=
        static_cast<std::size_t>(m_PassThroughPolys->GetNumberOfCells())) {
        igError("TriangleStripFilter has an invalid pass-through polygon mapping. ");
        return false;
    }
    for (IGsize i = 0; i < m_PassThroughPolys->GetNumberOfCells(); ++i) {
        const igIndex* ids = nullptr;
        const int count = m_PassThroughPolys->GetCellIds(i, ids);
        faces->AddCellIds(ids, count);
        outputSourceFaceIds.push_back(m_PassThroughPolySourceFaceIds[static_cast<std::size_t>(i)]);
    }

    if (outputSourceFaceIds.size() != static_cast<std::size_t>(faces->GetNumberOfCells())) {
        igError("TriangleStripFilter output face mapping is incomplete. ");
        return false;
    }

    AttributeSet::Pointer outputAttributes;
    if (!BuildOutputAttributes(outputSourceFaceIds, outputAttributes)) return false;

    output->SetFaces(faces);
    output->SetAttributeSet(outputAttributes);
    output->GetMetadata()->AddIntArray(StripOffsetsMetadataName, stripOffsets);
    output->GetMetadata()->AddIntArray(StripPointIdsMetadataName, stripPointIds);
    output->GetMetadata()->AddIntArray(
            StripSourceFaceOffsetsMetadataName, stripSourceFaceOffsets);
    output->GetMetadata()->AddIntArray(
            StripSourceFaceIdsMetadataName, stripSourceFaceIds);
    SetOutput(output);
    return true;
}

bool TriangleStripFilter::BuildOutputAttributes(
        const std::vector<igIndex>& outputSourceFaceIds,
        AttributeSet::Pointer& outputAttributes) const {
    outputAttributes = AttributeSet::New();
    if (!m_InputMesh || !m_InputMesh->GetAttributeSet()) return true;

    auto* inputAttributes = m_InputMesh->GetAttributeSet();
    for (IGsize attributeId = 0;
         attributeId < inputAttributes->GetNumberOfAttributes(); ++attributeId) {
        auto& attribute = inputAttributes->GetAttribute(attributeId);
        if (attribute.isDeleted || !attribute.pointer) continue;

        if (attribute.attachmentType == IG_POINT) {
            // Point IDs and point order are unchanged, so the array can be
            // passed through while the AttributeSet container stays independent.
            outputAttributes->AddAttribute(attribute.type, IG_POINT,
                                           attribute.pointer,
                                           attribute.GetDataRange());
        } else if (attribute.attachmentType == IG_CELL) {
            auto outputArray = CopyArrayTuplesByType(attribute.pointer,
                                                     outputSourceFaceIds);
            if (!outputArray) {
                igError("TriangleStripFilter could not remap a cell attribute. ");
                return false;
            }
            outputAttributes->AddAttribute(attribute.type, IG_CELL,
                                           outputArray,
                                           attribute.GetDataRange());
        }
    }
    return true;
}
IGAME_NAMESPACE_END
