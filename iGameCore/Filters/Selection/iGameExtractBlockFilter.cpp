#include "iGameExtractBlockFilter.h"
#include "iGameCompositeDataObject.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"
#include <cmath>
#include <limits>
#include <numeric>
#include <unordered_set>

IGAME_NAMESPACE_BEGIN

namespace ExtractBlockDetail {
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


std::vector<DataObject::Pointer> ExtractBlockFilter::GetBlocks(DataObject::Pointer input) {
    std::vector<DataObject::Pointer> blocks;
    if (!input) return blocks;
    if (auto composite = DynamicCast<CompositeDataObject>(input)) {
        const auto count = composite->GetNumberOfChildren();
        if (count) {
            for (unsigned int i = 0; i < count; ++i) blocks.push_back(composite->GetChild(i));
            return blocks;
        }
    }
    if (input->HasSubDataObject()) {
        for (auto it = input->SubDataObjectIteratorBegin(); it != input->SubDataObjectIteratorEnd(); ++it)
            blocks.push_back(it->second);
    }
    return blocks;
}

namespace {
std::vector<igIndex> AllIds(IGsize count) {
    std::vector<igIndex> ids(count);
    std::iota(ids.begin(), ids.end(), igIndex{0});
    return ids;
}

DataObject::Pointer CopyBlock(DataObject::Pointer input, std::string& error,
                              std::unordered_set<DataObject*>& active) {
    if (!input || !active.insert(input.get()).second) {
        error = "Null block or cyclic block hierarchy.";
        return nullptr;
    }
    DataObject::Pointer output;
    IGsize cellCount = 0;
    if (auto source = DynamicCast<StructuredMesh>(input)) {
        igIndex dimensions[3];
        std::copy_n(source->GetDimensionSize(), 3, dimensions);
        if (dimensions[0] < 2 || dimensions[1] < 2 || dimensions[2] < 1) {
            error = "Unsupported structured mesh dimensions.";
            return nullptr;
        }
        IGsize pointCount = 1;
        for (auto dimension : dimensions) {
            if (static_cast<IGsize>(dimension) > std::numeric_limits<IGsize>::max() / pointCount) {
                error = "Structured mesh dimensions overflow.";
                return nullptr;
            }
            pointCount *= dimension;
        }
        if (source->GetNumberOfPoints() != pointCount) {
            error = "Structured dimensions do not match point count.";
            return nullptr;
        }
        auto result = StructuredMesh::New();
        result->SetDimensionSize(dimensions);
        cellCount = static_cast<IGsize>(dimensions[0] - 1) * (dimensions[1] - 1) *
                    (dimensions[2] > 1 ? dimensions[2] - 1 : 1);
        output = result;
    } else if (auto source = DynamicCast<UnstructuredMesh>(input)) {
        auto result = UnstructuredMesh::New();
        auto cells = CellArray::New();
        if (!source->GetCells() || !source->GetCellTypes() ||
            source->GetCellTypes()->GetNumberOfElements() != source->GetNumberOfCells()) {
            error = "Cell connectivity and types do not match.";
            return nullptr;
        }
        cells->DeepCopy(source->GetCells());
        auto types = UnsignedIntArray::New();
        types->DeepCopy(source->GetCellTypes());
        result->SetCells(cells, types);
        cellCount = source->GetNumberOfCells();
        output = result;
    } else if (auto source = DynamicCast<VolumeMesh>(input)) {
        if (source->GetIsPolyhedronType()) {
            error = "Polyhedral VolumeMesh must be converted to UnstructuredMesh first.";
            return nullptr;
        }
        auto result = VolumeMesh::New();
        auto cells = CellArray::New();
        if (!source->GetVolumes()) { error = "Volume mesh has no connectivity."; return nullptr; }
        cells->DeepCopy(source->GetVolumes());
        result->SetVolumes(cells);
        cellCount = source->GetNumberOfVolumes();
        output = result;
    } else if (auto source = DynamicCast<SurfaceMesh>(input)) {
        auto result = SurfaceMesh::New();
        auto cells = CellArray::New();
        if (!source->GetFaces()) { error = "Surface mesh has no connectivity."; return nullptr; }
        cells->DeepCopy(source->GetFaces());
        result->SetFaces(cells);
        cellCount = source->GetNumberOfFaces();
        output = result;
    } else if (DynamicCast<PointSet>(input)) {
        output = PointSet::New();
    } else if (!ExtractBlockFilter::GetBlocks(input).empty() ||
               input->GetDataObjectType() == IG_DRAW_OBJECT ||
               input->GetDataObjectType() == IG_DATA_OBJECT ||
               input->GetDataObjectType() == IG_COMPOSITE_DATA_OBJECT) {
        output = DrawObject::New();
    } else {
        error = "Unsupported block data type.";
        return nullptr;
    }

    if (auto source = DynamicCast<PointSet>(input)) {
        if (!source->GetPoints()) { error = "Block has no point array."; return nullptr; }
        auto points = Points::New();
        points->DeepCopy(source->GetPoints());
        auto result = DynamicCast<PointSet>(output);
        result->SetPoints(points);
        if (auto structured = DynamicCast<StructuredMesh>(output))
            structured->GenStructuredCellConnectivities();
        if (!ExtractBlockDetail::CopyAttributes(input, output, AllIds(source->GetNumberOfPoints()),
                                              AllIds(cellCount), cellCount != 0, error)) return nullptr;
    }
    output->SetName(input->GetName());
    for (auto child : ExtractBlockFilter::GetBlocks(input)) {
        // Keep empty slots out of drawable output, but retain them during path lookup.
        if (!child) continue;
        auto copied = CopyBlock(child, error, active);
        if (!copied) return nullptr;
        output->AddSubDataObject(copied);
    }
    active.erase(input.get());
    return output;
}

bool FindByName(DataObject::Pointer input, const std::string& name,
                std::vector<DataObject::Pointer>& matches,
                std::unordered_set<DataObject*>& active) {
    if (!active.insert(input.get()).second) return false;
    for (auto child : ExtractBlockFilter::GetBlocks(input)) {
        if (!child) continue;
        if (child->GetName() == name) matches.push_back(child);
        if (!FindByName(child, name, matches, active)) return false;
    }
    active.erase(input.get());
    return true;
}
}

ExtractBlockFilter::ExtractBlockFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ExtractBlockFilter::Execute() {
    SetOutput(0, nullptr);
    m_LastError.clear();
    auto input = GetInput(0);
    auto fail = [this](const std::string& error) { m_LastError = error; return false; };
    if (!input || GetBlocks(input).empty()) return fail("Input must contain data blocks.");
    DataObject::Pointer selected;
    if (m_ByName) {
        if (m_Name.empty()) return fail("Block name must not be empty.");
        std::vector<DataObject::Pointer> matches;
        std::unordered_set<DataObject*> active;
        if (!FindByName(input, m_Name, matches, active)) return fail("Cyclic block hierarchy.");
        if (matches.size() != 1)
            return fail(matches.empty() ? "Block name not found." : "Block name is ambiguous; use an index path.");
        selected = matches.front();
    } else {
        if (m_Path.empty()) return fail("Block path must not be empty.");
        selected = input;
        for (int index : m_Path) {
            auto blocks = GetBlocks(selected);
            if (index < 0 || static_cast<size_t>(index) >= blocks.size()) return fail("Block index is out of range.");
            selected = blocks[index];
            if (!selected) return fail("Selected block is empty.");
        }
    }
    std::unordered_set<DataObject*> active;
    auto output = CopyBlock(selected, m_LastError, active);
    if (!output) return false;
    output->SetName(selected->GetName() + "_extracted_block");
    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END
