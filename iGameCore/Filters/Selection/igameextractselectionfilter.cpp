#include "igameextractselectionfilter.h"
#include "iGameExtractionUtilities.h"
#include "iGameSelection.h"
#include "iGameUnstructuredMesh.h"
#include <unordered_map>
#include <vector>
#include <cmath>
#include <limits>

IGAME_NAMESPACE_BEGIN

namespace ExtractionDetail {
template<typename Array>
ArrayObject::Pointer CopyTuples(ArrayObject::Pointer source,
                               const std::vector<igIndex>& ids) {
    auto typed = DynamicCast<Array>(source);
    if (!typed) return nullptr;
    auto result = Array::New();
    const int dim = typed->GetDimension();
    result->SetName(typed->GetName());
    result->SetDimension(dim);
    result->Reserve(ids.size());
    for (auto id : ids) {
        for (int component = 0; component < dim; ++component) {
            result->AddValue(typed->RawPointer()[static_cast<IGsize>(id) * dim + component]);
        }
    }
    return result;
}

ArrayObject::Pointer CopyArray(ArrayObject::Pointer source,
                              const std::vector<igIndex>& ids) {
    // Copy typed values directly: large integer IDs must not pass through double.
#define COPY_ARRAY(Type) if (auto result = CopyTuples<Type>(source, ids)) return result
    COPY_ARRAY(FloatArray);
    COPY_ARRAY(DoubleArray);
    COPY_ARRAY(IntArray);
    COPY_ARRAY(UnsignedIntArray);
    COPY_ARRAY(CharArray);
    COPY_ARRAY(UnsignedCharArray);
    COPY_ARRAY(ShortArray);
    COPY_ARRAY(UnsignedShortArray);
    COPY_ARRAY(LongLongArray);
    COPY_ARRAY(UnsignedLongLongArray);
    COPY_ARRAY(FlatArray<igIndex>);
#undef COPY_ARRAY
    return nullptr;
}

bool CopyAttributes(DataObject::Pointer input, DataObject::Pointer output,
                    const std::vector<igIndex>& pointIds,
                    const std::vector<igIndex>& cellIds, bool copyCells,
                    std::string& error) {
    auto attrs = input->GetAttributeSet();
    for (IGsize i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        const auto& attr = attrs->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer) continue;
        if (attr.attachmentType != IG_POINT &&
            !(copyCells && attr.attachmentType == IG_CELL)) continue;
        const auto& ids = attr.attachmentType == IG_POINT ? pointIds : cellIds;
        const int dim = attr.pointer->GetDimension();
        if (dim <= 0 || attr.pointer->GetNumberOfValues() % dim != 0) {
            error = "Invalid attribute dimensions: " + attr.pointer->GetName();
            return false;
        }
        for (auto id : ids) {
            if (static_cast<IGsize>(id) >= attr.pointer->GetNumberOfElements()) {
                error = "Attribute has too few tuples: " + attr.pointer->GetName();
                return false;
            }
        }
        auto copied = CopyArray(attr.pointer, ids);
        if (!copied) {
            error = "Unsupported attribute array: " + attr.pointer->GetName();
            return false;
        }
        auto ranges = DoubleArray::New();
        ranges->SetDimension(2);
        ranges->Resize(dim + 1);
        std::vector<double> minimum(dim + 1, std::numeric_limits<double>::infinity());
        std::vector<double> maximum(dim + 1, -std::numeric_limits<double>::infinity());
        for (IGsize tuple = 0; tuple < copied->GetNumberOfElements(); ++tuple) {
            double magnitude = 0;
            for (int component = 0; component < dim; ++component) {
                const double value = copied->GetValue(tuple * dim + component);
                magnitude = std::hypot(magnitude, value);
                if (std::isfinite(value)) {
                    minimum[component + 1] = std::min(minimum[component + 1], value);
                    maximum[component + 1] = std::max(maximum[component + 1], value);
                }
            }
            if (std::isfinite(magnitude)) {
                minimum[0] = std::min(minimum[0], magnitude);
                maximum[0] = std::max(maximum[0], magnitude);
            }
        }
        for (int component = 0; component <= dim; ++component) {
            double range[]{std::isfinite(minimum[component]) ? minimum[component] : 0,
                           std::isfinite(maximum[component]) ? maximum[component] : 0};
            ranges->SetElement(component, range);
        }
        output->GetAttributeSet()->AddAttribute(attr.type, attr.attachmentType, copied, ranges);
    }
    return true;
}
}

ExtractSelectionFilter::ExtractSelectionFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ExtractSelectionFilter::Execute() {
    SetOutput(0, nullptr);
    m_LastError.clear();
    auto fail = [this](const std::string& error) {
        m_LastError = error;
        return false;
    };
    auto input = DynamicCast<PointSet>(GetInput(0));
    if (!input || !input->GetPoints()) return fail("Input must be a point-based mesh.");
    if (m_SelectionType != IG_POINT && m_SelectionType != IG_CELL)
        return fail("Selection type must be IG_POINT or IG_CELL.");
    const auto& selected = input->GetSelection()->GetSelectedItems(m_SelectionType);
    if (selected.empty()) return fail("No selected items for the requested extraction type.");

    std::vector<igIndex> pointIds;
    std::vector<igIndex> cellIds;
    PointSet::Pointer output;
    if (m_SelectionType == IG_POINT) {
        auto vertices = UnstructuredMesh::New();
        vertices->SetViewStyle(IG_POINTS);
        vertices->SetShellRenderingOption(false);
        output = vertices;
        pointIds.assign(selected.begin(), selected.end());
        for (auto id : pointIds) {
            if (id < 0 || static_cast<IGsize>(id) >= input->GetNumberOfPoints())
                return fail("Selected point ID is out of range.");
            output->GetPoints()->AddPoint(input->GetPoint(id));
            igIndex vertex = static_cast<igIndex>(vertices->GetNumberOfPoints() - 1);
            vertices->AddCell(&vertex, 1, IG_VERTEX);
        }
    } else {
        auto mesh = DynamicCast<UnstructuredMesh>(input);
        if (!mesh) {
            mesh = UnstructuredMesh::New();
            // VolumeMesh derives from SurfaceMesh; test the more specific type first.
            if (auto volume = DynamicCast<VolumeMesh>(input)) {
                // The existing polyhedral VolumeMesh converter uses fixed-size buffers.
                if (volume->GetIsPolyhedronType())
                    return fail("Convert polyhedral VolumeMesh to UnstructuredMesh before extraction.");
                auto volumes = volume->GetVolumes();
                if (!volumes) return fail("Volume mesh has no connectivity.");
                auto types = UnsignedIntArray::New();
                for (IGsize i = 0; i < volumes->GetNumberOfCells(); ++i) {
                    switch (volumes->GetCellSize(i)) {
                        case 4: types->AddValue(IG_TETRA); break;
                        case 5: types->AddValue(IG_PYRAMID); break;
                        case 6: types->AddValue(IG_PRISM); break;
                        case 8: types->AddValue(IG_HEXAHEDRON); break;
                        default: return fail("Unsupported volume cell topology.");
                    }
                }
                mesh->SetCells(volumes, types);
            } else if (auto surface = DynamicCast<SurfaceMesh>(input)) {
                if (!mesh->GenerateFromSurfaceMesh(surface)) return fail("Cannot read surface cells.");
            } else {
                return fail("Cell extraction requires a surface, volume or unstructured mesh.");
            }
        }
        if (!mesh->GetCells() || !mesh->GetCellTypes() ||
            mesh->GetCellTypes()->GetNumberOfElements() != mesh->GetNumberOfCells())
            return fail("Cell connectivity and types do not match.");
        auto result = UnstructuredMesh::New();
        output = result;
        std::unordered_map<igIndex, igIndex> remap;
        auto mapPoint = [&](igIndex& id) {
            if (id < 0 || static_cast<IGsize>(id) >= input->GetNumberOfPoints()) return false;
            auto found = remap.find(id);
            if (found == remap.end()) {
                const auto newId = static_cast<igIndex>(pointIds.size());
                remap.emplace(id, newId);
                pointIds.push_back(id);
                result->GetPoints()->AddPoint(input->GetPoint(id));
                id = newId;
            } else {
                id = found->second;
            }
            return true;
        };
        for (auto cellId : selected) {
            if (cellId < 0 || static_cast<IGsize>(cellId) >= mesh->GetNumberOfCells())
                return fail("Selected cell ID is out of range.");
            const igIndex* ids = nullptr;
            const int count = mesh->GetCellPointIds(cellId, ids);
            if (count <= 0 || !ids) return fail("Selected cell has no connectivity.");
            std::vector<igIndex> connectivity(ids, ids + count);
            const auto type = mesh->GetCellType(cellId);
            if (type == IG_POLYHEDRON) {
                // Polyhedron connectivity: face count, then (size, point IDs) per face.
                const auto faces = connectivity[0];
                if (faces <= 0 || faces >= count) return fail("Invalid polyhedron face count.");
                size_t offset = 1;
                for (igIndex face = 0; face < faces; ++face) {
                    if (offset >= connectivity.size()) return fail("Invalid polyhedron connectivity.");
                    const auto size = connectivity[offset++];
                    if (size < 3 || static_cast<size_t>(size) > connectivity.size() - offset)
                        return fail("Invalid polyhedron face.");
                    for (igIndex vertex = 0; vertex < size; ++vertex) {
                        if (!mapPoint(connectivity[offset++])) return fail("Cell point ID is out of range.");
                    }
                }
                if (offset != connectivity.size()) return fail("Invalid polyhedron connectivity length.");
            } else {
                for (auto& id : connectivity) {
                    if (!mapPoint(id)) return fail("Cell point ID is out of range.");
                }
            }
            result->AddCell(connectivity.data(), count, type);
            cellIds.push_back(cellId);
        }
    }
    if (!ExtractionDetail::CopyAttributes(input, output, pointIds, cellIds, m_SelectionType == IG_CELL, m_LastError))
        return false;
    output->SetName(input->GetName() + (m_SelectionType == IG_POINT ? "_selected_points" : "_selected_cells"));
    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END
