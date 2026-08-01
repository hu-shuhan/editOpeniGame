#include "VtkDataCodecAdapters.h"

#include <DataCodec/API/Adapter/ICellTypeMapping.h>
#include <DataCodec/Common/DataCodecTypes.h>

#include <vtkAbstractArray.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDataArray.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace vtk_datacodec_example {
namespace {

using ::datacodec::AttrAttachment;
using ::datacodec::AttrRole;
using ::datacodec::CellTypeCodecEntry;
using ::datacodec::CellTypeFamilyCode;
using ::datacodec::CellTypeLocalCode;
using ::datacodec::CellTypeMappingMode;
using ::datacodec::CellTypeNativeMappingEntry;
using ::datacodec::CellTypeRaw;
using ::datacodec::CodecCellTypeId;
using ::datacodec::DataType;

constexpr std::array<CellTypeNativeMappingEntry, 10> kVtkCellTypeMappings{{
    {CodecCellTypeId::Vertex, static_cast<CellTypeRaw>(VTK_VERTEX)},
    {CodecCellTypeId::Line, static_cast<CellTypeRaw>(VTK_LINE)},
    {CodecCellTypeId::PolyLine, static_cast<CellTypeRaw>(VTK_POLY_LINE)},
    {CodecCellTypeId::Triangle, static_cast<CellTypeRaw>(VTK_TRIANGLE)},
    {CodecCellTypeId::Quad, static_cast<CellTypeRaw>(VTK_QUAD)},
    {CodecCellTypeId::Polygon, static_cast<CellTypeRaw>(VTK_POLYGON)},
    {CodecCellTypeId::Tetra, static_cast<CellTypeRaw>(VTK_TETRA)},
    {CodecCellTypeId::Hexahedron, static_cast<CellTypeRaw>(VTK_HEXAHEDRON)},
    {CodecCellTypeId::Prism, static_cast<CellTypeRaw>(VTK_WEDGE)},
    {CodecCellTypeId::Pyramid, static_cast<CellTypeRaw>(VTK_PYRAMID)},
}};

constexpr bool HasUniqueVtkCellTypeMappings() {
    for (std::size_t index = 0; index < kVtkCellTypeMappings.size(); ++index) {
        for (std::size_t other = index + 1; other < kVtkCellTypeMappings.size(); ++other) {
            if (kVtkCellTypeMappings[index].codecType == kVtkCellTypeMappings[other].codecType ||
                kVtkCellTypeMappings[index].rawType == kVtkCellTypeMappings[other].rawType) {
                return false;
            }
        }
    }
    return true;
}

static_assert(HasUniqueVtkCellTypeMappings());

bool Fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

class VtkCellTypeMapping final : public ::datacodec::ICellTypeMapping {
public:
    CellTypeMappingMode GetCellTypeMappingMode() const override {
        return CellTypeMappingMode::FamilyLocal;
    }

    bool ResolveCellType(
        const CellTypeRaw rawType,
        CellTypeCodecEntry& entry) const override {
        return ::datacodec::ResolveMappedCellType(kVtkCellTypeMappings, rawType, entry);
    }

    bool ResolveCellSizeFromPolynomialOrder(
        CellTypeRaw,
        std::uint16_t,
        int& size) const override {
        size = -1;
        return false;
    }

    bool EncodeCellTypeFamilyLocal(
        const CellTypeRaw rawType,
        CellTypeFamilyCode& familyCode,
        CellTypeLocalCode& familyLocalCode) const override {
        return ::datacodec::EncodeMappedCellTypeFamilyLocal(
            kVtkCellTypeMappings,
            rawType,
            familyCode,
            familyLocalCode);
    }

    bool DecodeCellTypeFamilyLocal(
        const CellTypeFamilyCode familyCode,
        const CellTypeLocalCode familyLocalCode,
        CellTypeRaw& rawType) const override {
        return ::datacodec::DecodeMappedCellTypeFamilyLocal(
            kVtkCellTypeMappings,
            familyCode,
            familyLocalCode,
            rawType);
    }
};

bool TryMapVtkDataType(const int vtkType, DataType& output) {
    switch (vtkType) {
        case VTK_FLOAT:
            output = DataType::Float32;
            return true;
        case VTK_DOUBLE:
            output = DataType::Float64;
            return true;
        default:
            output = DataType::Float32;
            return false;
    }
}

AttrRole ResolveAttributeRole(vtkDataSetAttributes* attributes, const int arrayIndex) {
    if (attributes == nullptr) {
        return AttrRole::Unknown;
    }
    switch (attributes->IsArrayAnAttribute(arrayIndex)) {
        case vtkDataSetAttributes::SCALARS:
            return AttrRole::Scalar;
        case vtkDataSetAttributes::VECTORS:
            return AttrRole::Vector;
        case vtkDataSetAttributes::NORMALS:
            return AttrRole::Normal;
        case vtkDataSetAttributes::TCOORDS:
            return AttrRole::TexCoord;
        case vtkDataSetAttributes::TENSORS:
            return AttrRole::Tensor;
        default:
            return AttrRole::Unknown;
    }
}

class VtkEncodeAttributeView final : public ::datacodec::IEncodeAttrView {
public:
    VtkEncodeAttributeView(
        vtkDataArray* array,
        const AttrAttachment attachment,
        const AttrRole role)
        : m_array(array), m_attachment(attachment), m_role(role) {
        m_supported = m_array != nullptr && TryMapVtkDataType(m_array->GetDataType(), m_dataType);
    }

    std::string GetName() const override {
        const auto* name = m_array != nullptr ? m_array->GetName() : nullptr;
        return name != nullptr ? name : std::string{};
    }

    DataType GetDataType() const override { return m_dataType; }
    bool IsDataTypeSupported() const override { return m_supported; }
    AttrRole GetRole() const override { return m_role; }
    AttrAttachment GetAttachType() const override { return m_attachment; }

    int GetComponentCount() const override {
        return m_array != nullptr ? m_array->GetNumberOfComponents() : 0;
    }

    std::size_t GetElementCount() const override {
        return m_array != nullptr
            ? static_cast<std::size_t>(m_array->GetNumberOfTuples())
            : 0u;
    }

    const void* TryGetRawPtr() const override {
        if (m_array == nullptr || !m_array->HasStandardMemoryLayout() || m_array->GetNumberOfValues() == 0) {
            return nullptr;
        }
        return m_array->GetVoidPointer(0);
    }

    void GetTuple(const std::size_t index, double* output) const override {
        if (m_array == nullptr || output == nullptr || index >= GetElementCount()) {
            return;
        }
        m_array->GetTuple(static_cast<vtkIdType>(index), output);
    }

private:
    vtkSmartPointer<vtkDataArray> m_array;
    AttrAttachment m_attachment{AttrAttachment::Point};
    AttrRole m_role{AttrRole::Unknown};
    DataType m_dataType{DataType::Float32};
    bool m_supported{false};
};

template<typename TValue>
bool CopyRange(
    std::vector<TValue>& target,
    const std::size_t offset,
    const TValue* data,
    const std::size_t count,
    const char* label,
    std::string* error) {
    if (count == 0u) {
        return true;
    }
    if (data == nullptr) {
        return Fail(error, std::string(label) + " range has null input");
    }
    if (offset > target.size() || count > target.size() - offset) {
        return Fail(error, std::string(label) + " range is outside the target buffer");
    }
    std::copy(data, data + count, target.begin() + static_cast<std::ptrdiff_t>(offset));
    return true;
}

vtkSmartPointer<vtkDataArray> CreateVtkArray(const DataType type) {
    switch (type) {
        case DataType::Float32:
            return vtkSmartPointer<vtkFloatArray>::New();
        case DataType::Float64:
            return vtkSmartPointer<vtkDoubleArray>::New();
        default:
            return nullptr;
    }
}

int ToVtkAttributeRole(const AttrRole role) {
    switch (role) {
        case AttrRole::Scalar:
        case AttrRole::Color:
            return vtkDataSetAttributes::SCALARS;
        case AttrRole::Vector:
            return vtkDataSetAttributes::VECTORS;
        case AttrRole::Normal:
            return vtkDataSetAttributes::NORMALS;
        case AttrRole::TexCoord:
            return vtkDataSetAttributes::TCOORDS;
        case AttrRole::Tensor:
            return vtkDataSetAttributes::TENSORS;
        case AttrRole::Unknown:
            return -1;
    }
    return -1;
}

} // namespace

class VtkDataCodecEncodeAdapter::Impl {
public:
    vtkSmartPointer<vtkUnstructuredGrid> input;
    std::vector<IndexType> connectivity;
    std::vector<IndexType> offsets;
    std::vector<IndexType> cellTypes;
    std::vector<std::unique_ptr<VtkEncodeAttributeView>> pointAttributes;
    std::vector<std::unique_ptr<VtkEncodeAttributeView>> cellAttributes;
    VtkCellTypeMapping cellTypeMapping;
};

VtkDataCodecEncodeAdapter::VtkDataCodecEncodeAdapter()
    : m_impl(std::make_unique<Impl>()) {}

VtkDataCodecEncodeAdapter::~VtkDataCodecEncodeAdapter() = default;

std::unique_ptr<VtkDataCodecEncodeAdapter> VtkDataCodecEncodeAdapter::Create(
    vtkUnstructuredGrid* input,
    std::string* error) {
    auto adapter = std::unique_ptr<VtkDataCodecEncodeAdapter>(new VtkDataCodecEncodeAdapter());
    if (!adapter->Initialize(input, error)) {
        return nullptr;
    }
    return adapter;
}

bool VtkDataCodecEncodeAdapter::Initialize(
    vtkUnstructuredGrid* input,
    std::string* error) {
    ResetInput();
    if (input == nullptr) {
        return Fail(error, "VTK encode adapter requires vtkUnstructuredGrid input");
    }

    const auto pointCount = input->GetNumberOfPoints();
    const auto cellCount = input->GetNumberOfCells();
    if (pointCount < 0 || cellCount < 0) {
        return Fail(error, "VTK grid reports a negative point or cell count");
    }
    if (static_cast<std::uint64_t>(pointCount) > std::numeric_limits<IndexType>::max() ||
        static_cast<std::uint64_t>(cellCount) > std::numeric_limits<IndexType>::max()) {
        return Fail(error, "VTK grid exceeds the DataCodec 32-bit topology limit");
    }

    auto* points = input->GetPoints();
    auto* pointArray = points != nullptr ? points->GetData() : nullptr;
    if (pointCount != 0 && pointArray == nullptr) {
        return Fail(error, "VTK grid has no point coordinate array");
    }
    if (pointArray != nullptr && pointArray->GetDataType() != VTK_FLOAT &&
        pointArray->GetDataType() != VTK_DOUBLE) {
        return Fail(error, "VTK point coordinates must use float or double storage");
    }

    m_impl->input = input;
    m_impl->offsets.reserve(static_cast<std::size_t>(cellCount) + 1u);
    m_impl->cellTypes.reserve(static_cast<std::size_t>(cellCount));
    m_impl->offsets.push_back(0u);

    for (vtkIdType cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
        const auto rawCellType = static_cast<CellTypeRaw>(input->GetCellType(cellIndex));
        CellTypeCodecEntry entry;
        if (!m_impl->cellTypeMapping.ResolveCellType(rawCellType, entry)) {
            std::ostringstream message;
            message << "unsupported VTK cell type " << rawCellType
                    << " at cell " << cellIndex;
            ResetInput();
            return Fail(error, message.str());
        }

        vtkIdType cellPointCount = 0;
        const vtkIdType* cellPointIds = nullptr;
        input->GetCellPoints(cellIndex, cellPointCount, cellPointIds);
        if (cellPointCount < 0 || (cellPointCount != 0 && cellPointIds == nullptr)) {
            ResetInput();
            return Fail(error, "VTK grid returned invalid cell connectivity");
        }
        if (static_cast<std::uint64_t>(cellPointCount) >
            std::numeric_limits<IndexType>::max() - m_impl->connectivity.size()) {
            ResetInput();
            return Fail(error, "VTK connectivity exceeds the DataCodec 32-bit topology limit");
        }

        for (vtkIdType localIndex = 0; localIndex < cellPointCount; ++localIndex) {
            const auto pointId = cellPointIds[localIndex];
            if (pointId < 0 || pointId >= pointCount ||
                static_cast<std::uint64_t>(pointId) > std::numeric_limits<IndexType>::max()) {
                ResetInput();
                return Fail(error, "VTK connectivity contains an invalid point id");
            }
            m_impl->connectivity.push_back(static_cast<IndexType>(pointId));
        }
        m_impl->offsets.push_back(static_cast<IndexType>(m_impl->connectivity.size()));
        m_impl->cellTypes.push_back(rawCellType);
    }

    const auto collectAttributes = [this, error](
        vtkDataSetAttributes* attributes,
        const AttrAttachment attachment,
        const vtkIdType expectedTupleCount,
        std::vector<std::unique_ptr<VtkEncodeAttributeView>>& output) {
        if (attributes == nullptr) {
            return true;
        }
        const auto arrayCount = attributes->GetNumberOfArrays();
        output.reserve(static_cast<std::size_t>(std::max(arrayCount, 0)));
        for (int arrayIndex = 0; arrayIndex < arrayCount; ++arrayIndex) {
            auto* abstractArray = attributes->GetAbstractArray(arrayIndex);
            auto* array = vtkDataArray::SafeDownCast(abstractArray);
            const auto* arrayName = abstractArray != nullptr ? abstractArray->GetName() : nullptr;
            const std::string resolvedName = arrayName != nullptr ? arrayName : "<unnamed>";
            if (array == nullptr) {
                return Fail(error, "VTK attribute '" + resolvedName + "' is not a numeric array");
            }
            DataType dataType;
            if (!TryMapVtkDataType(array->GetDataType(), dataType)) {
                return Fail(error, "VTK attribute '" + resolvedName + "' must use float or double storage");
            }
            if (array->GetNumberOfComponents() <= 0 ||
                array->GetNumberOfTuples() != expectedTupleCount) {
                return Fail(error, "VTK attribute '" + resolvedName + "' has an invalid tuple layout");
            }
            output.push_back(std::make_unique<VtkEncodeAttributeView>(
                array,
                attachment,
                ResolveAttributeRole(attributes, arrayIndex)));
        }
        return true;
    };

    if (!collectAttributes(
            input->GetPointData(),
            AttrAttachment::Point,
            pointCount,
            m_impl->pointAttributes) ||
        !collectAttributes(
            input->GetCellData(),
            AttrAttachment::Cell,
            cellCount,
            m_impl->cellAttributes)) {
        ResetInput();
        return false;
    }

    return true;
}

VtkDataCodecEncodeAdapter::MeshType VtkDataCodecEncodeAdapter::GetMeshType() const {
    return MeshType::UnstructuredMesh;
}

std::string VtkDataCodecEncodeAdapter::GetName() const {
    return "vtkUnstructuredGrid";
}

std::size_t VtkDataCodecEncodeAdapter::GetNumberOfPoints() const {
    return m_impl->input != nullptr
        ? static_cast<std::size_t>(m_impl->input->GetNumberOfPoints())
        : 0u;
}

void VtkDataCodecEncodeAdapter::GetPoint(
    const std::size_t index,
    double output[3]) const {
    if (output == nullptr) {
        return;
    }
    output[0] = 0.0;
    output[1] = 0.0;
    output[2] = 0.0;
    if (m_impl->input == nullptr || index >= GetNumberOfPoints()) {
        return;
    }
    m_impl->input->GetPoint(static_cast<vtkIdType>(index), output);
}

VtkDataCodecEncodeAdapter::ScalarType
VtkDataCodecEncodeAdapter::GetPointScalarType() const {
    if (m_impl->input != nullptr && m_impl->input->GetPoints() != nullptr &&
        m_impl->input->GetPoints()->GetDataType() == VTK_FLOAT) {
        return ScalarType::Float32;
    }
    return ScalarType::Float64;
}

const float* VtkDataCodecEncodeAdapter::TryGetPointsF32() const {
    if (m_impl->input == nullptr || m_impl->input->GetPoints() == nullptr) {
        return nullptr;
    }
    auto* array = m_impl->input->GetPoints()->GetData();
    if (array == nullptr || array->GetDataType() != VTK_FLOAT ||
        !array->HasStandardMemoryLayout() || array->GetNumberOfValues() == 0) {
        return nullptr;
    }
    return static_cast<const float*>(array->GetVoidPointer(0));
}

const double* VtkDataCodecEncodeAdapter::TryGetPointsF64() const {
    if (m_impl->input == nullptr || m_impl->input->GetPoints() == nullptr) {
        return nullptr;
    }
    auto* array = m_impl->input->GetPoints()->GetData();
    if (array == nullptr || array->GetDataType() != VTK_DOUBLE ||
        !array->HasStandardMemoryLayout() || array->GetNumberOfValues() == 0) {
        return nullptr;
    }
    return static_cast<const double*>(array->GetVoidPointer(0));
}

bool VtkDataCodecEncodeAdapter::DescribeTopology(
    TopologyInputDescriptor& output,
    std::string* error) const {
    output = {};
    if (m_impl->input == nullptr) {
        return Fail(error, "VTK encode adapter has no input grid");
    }
    output.pointCount = GetNumberOfPoints();
    output.cellCount = GetNumberOfCells();
    output.connectivityCount = GetCellIdBufferSize();
    output.connectivity = ::datacodec::TopologyValueSource::CompactArray;
    output.offsets = ::datacodec::TopologyValueSource::CompactArray;
    output.cellSize = ::datacodec::TopologyCellSizeSource::Offsets;
    output.cellTypes = ::datacodec::TopologyValueSource::CompactArray;
    return true;
}

std::size_t VtkDataCodecEncodeAdapter::GetNumberOfCells() const {
    return m_impl->cellTypes.size();
}

std::size_t VtkDataCodecEncodeAdapter::GetCellIdBufferSize() const {
    return m_impl->connectivity.size();
}

const VtkDataCodecEncodeAdapter::IndexType*
VtkDataCodecEncodeAdapter::GetCellIdBufferPtr() const {
    return m_impl->connectivity.empty() ? nullptr : m_impl->connectivity.data();
}

const VtkDataCodecEncodeAdapter::IndexType*
VtkDataCodecEncodeAdapter::GetCellIdOffsetPtr() const {
    return m_impl->offsets.empty() ? nullptr : m_impl->offsets.data();
}

bool VtkDataCodecEncodeAdapter::IsFixedCellSize() const { return false; }
int VtkDataCodecEncodeAdapter::GetFixedCellSize() const { return -1; }

const VtkDataCodecEncodeAdapter::IndexType*
VtkDataCodecEncodeAdapter::GetCellTypesPtr() const {
    return m_impl->cellTypes.empty() ? nullptr : m_impl->cellTypes.data();
}

std::size_t VtkDataCodecEncodeAdapter::GetCellFaceBufferSize() const { return 0u; }

std::size_t VtkDataCodecEncodeAdapter::GetNumberOfPointAttrs() const {
    return m_impl->pointAttributes.size();
}

const ::datacodec::IEncodeAttrView&
VtkDataCodecEncodeAdapter::GetPointAttr(const std::size_t index) const {
    return *m_impl->pointAttributes.at(index);
}

std::size_t VtkDataCodecEncodeAdapter::GetNumberOfCellAttrs() const {
    return m_impl->cellAttributes.size();
}

const ::datacodec::IEncodeAttrView&
VtkDataCodecEncodeAdapter::GetCellAttr(const std::size_t index) const {
    return *m_impl->cellAttributes.at(index);
}

bool VtkDataCodecEncodeAdapter::ResolveCellType(
    const CellTypeRaw rawType,
    CellTypeCodecEntry& entry) const {
    return m_impl->cellTypeMapping.ResolveCellType(rawType, entry);
}

VtkDataCodecEncodeAdapter::CellTypeMappingMode
VtkDataCodecEncodeAdapter::GetCellTypeMappingMode() const {
    return m_impl->cellTypeMapping.GetCellTypeMappingMode();
}

bool VtkDataCodecEncodeAdapter::ResolveCellSizeFromPolynomialOrder(
    const CellTypeRaw rawType,
    const std::uint16_t order,
    int& size) const {
    return m_impl->cellTypeMapping.ResolveCellSizeFromPolynomialOrder(rawType, order, size);
}

bool VtkDataCodecEncodeAdapter::EncodeCellTypeFamilyLocal(
    const CellTypeRaw rawType,
    CellTypeFamilyCode& familyCode,
    CellTypeLocalCode& familyLocalCode) const {
    return m_impl->cellTypeMapping.EncodeCellTypeFamilyLocal(
        rawType,
        familyCode,
        familyLocalCode);
}

bool VtkDataCodecEncodeAdapter::DecodeCellTypeFamilyLocal(
    const CellTypeFamilyCode familyCode,
    const CellTypeLocalCode familyLocalCode,
    CellTypeRaw& rawType) const {
    return m_impl->cellTypeMapping.DecodeCellTypeFamilyLocal(
        familyCode,
        familyLocalCode,
        rawType);
}

void VtkDataCodecEncodeAdapter::ResetInput() {
    m_impl->input = nullptr;
    m_impl->connectivity.clear();
    m_impl->offsets.clear();
    m_impl->cellTypes.clear();
    m_impl->pointAttributes.clear();
    m_impl->cellAttributes.clear();
}

void VtkDataCodecEncodeAdapter::Abort() { ResetInput(); }

class VtkDataCodecDecodeAdapter::Impl {
public:
    struct PendingAttribute {
        AttrStorageParams meta;
        vtkSmartPointer<vtkDataArray> array;
    };

    vtkSmartPointer<vtkUnstructuredGrid> output;
    vtkSmartPointer<vtkPoints> points;
    std::size_t pointCount{0u};
    std::size_t cellCount{0u};
    bool hasOffsets{false};
    std::vector<IndexType> connectivity;
    std::vector<IndexType> offsets;
    std::vector<IndexType> cellTypes;
    std::vector<PendingAttribute> attributes;
    VtkCellTypeMapping cellTypeMapping;
};

VtkDataCodecDecodeAdapter::VtkDataCodecDecodeAdapter()
    : m_impl(std::make_unique<Impl>()) {}

VtkDataCodecDecodeAdapter::~VtkDataCodecDecodeAdapter() = default;

bool VtkDataCodecDecodeAdapter::SetMeshType(
    const MeshType type,
    std::string* error) {
    ResetOutput();
    if (type != MeshType::UnstructuredMesh) {
        return Fail(error, "VTK decode adapter only supports UnstructuredMesh");
    }
    m_impl->output = vtkSmartPointer<vtkUnstructuredGrid>::New();
    return true;
}

bool VtkDataCodecDecodeAdapter::BeginPoints(
    const std::size_t count,
    const std::size_t dimension,
    std::string* error) {
    if (m_impl->output == nullptr) {
        return Fail(error, "VTK point decode requires an initialized output grid");
    }
    if (dimension != 3u || count > static_cast<std::size_t>(std::numeric_limits<vtkIdType>::max())) {
        return Fail(error, "VTK point decode requires three components and a valid tuple count");
    }
    m_impl->points = vtkSmartPointer<vtkPoints>::New();
    m_impl->points->SetDataTypeToFloat();
    m_impl->points->SetNumberOfPoints(static_cast<vtkIdType>(count));
    m_impl->output->SetPoints(m_impl->points);
    m_impl->pointCount = count;
    return true;
}

bool VtkDataCodecDecodeAdapter::WritePointsRange(
    const std::size_t offset,
    const std::size_t count,
    const float* data,
    std::string* error) {
    if (count == 0u) {
        return true;
    }
    if (m_impl->points == nullptr || data == nullptr ||
        offset > m_impl->pointCount || count > m_impl->pointCount - offset) {
        return Fail(error, "VTK point range is outside the target array");
    }
    auto* target = static_cast<float*>(m_impl->points->GetData()->GetVoidPointer(0));
    if (target == nullptr) {
        return Fail(error, "VTK point array has no writable storage");
    }
    std::memcpy(
        target + offset * 3u,
        data,
        count * 3u * sizeof(float));
    return true;
}

bool VtkDataCodecDecodeAdapter::EndPoints(std::string*) {
    m_impl->points = nullptr;
    return true;
}

bool VtkDataCodecDecodeAdapter::BeginTopology(
    const std::size_t cellCount,
    const std::size_t connectivityCount,
    const bool hasOffsets,
    std::string* error) {
    if (m_impl->output == nullptr ||
        cellCount > static_cast<std::size_t>(std::numeric_limits<vtkIdType>::max()) ||
        connectivityCount > static_cast<std::size_t>(std::numeric_limits<vtkIdType>::max())) {
        return Fail(error, "VTK topology dimensions are invalid");
    }
    m_impl->cellCount = cellCount;
    m_impl->hasOffsets = hasOffsets;
    m_impl->connectivity.assign(connectivityCount, 0u);
    m_impl->offsets.assign(hasOffsets ? cellCount + 1u : 0u, 0u);
    m_impl->cellTypes.assign(cellCount, 0u);
    return true;
}

bool VtkDataCodecDecodeAdapter::WriteConnectivityRange(
    const std::size_t offset,
    const IndexType* data,
    const std::size_t count,
    std::string* error) {
    return CopyRange(m_impl->connectivity, offset, data, count, "connectivity", error);
}

bool VtkDataCodecDecodeAdapter::WriteOffsetsRange(
    const std::size_t offset,
    const IndexType* data,
    const std::size_t count,
    std::string* error) {
    return CopyRange(m_impl->offsets, offset, data, count, "offset", error);
}

bool VtkDataCodecDecodeAdapter::WriteCellTypesRange(
    const std::size_t offset,
    const IndexType* data,
    const std::size_t count,
    std::string* error) {
    return CopyRange(m_impl->cellTypes, offset, data, count, "cell type", error);
}

bool VtkDataCodecDecodeAdapter::WriteCellPolynomialOrdersRange(
    std::size_t,
    const std::uint16_t*,
    const std::size_t count,
    std::string* error) {
    return count == 0u
        ? true
        : Fail(error, "VTK decode adapter does not support polynomial-order cells");
}

bool VtkDataCodecDecodeAdapter::EndTopology(std::string* error) {
    if (m_impl->output == nullptr || m_impl->cellTypes.size() != m_impl->cellCount) {
        return Fail(error, "VTK topology commit has incomplete state");
    }

    if (!m_impl->hasOffsets) {
        m_impl->offsets.assign(m_impl->cellCount + 1u, 0u);
        if (m_impl->cellCount != 0u) {
            if (m_impl->connectivity.empty() ||
                m_impl->connectivity.size() % m_impl->cellCount != 0u) {
                return Fail(error, "VTK fixed-size topology has an invalid connectivity length");
            }
            const auto fixedSize = m_impl->connectivity.size() / m_impl->cellCount;
            for (std::size_t index = 0; index <= m_impl->cellCount; ++index) {
                m_impl->offsets[index] = static_cast<IndexType>(index * fixedSize);
            }
        }
    }

    if (m_impl->offsets.size() != m_impl->cellCount + 1u ||
        m_impl->offsets.front() != 0u ||
        m_impl->offsets.back() != m_impl->connectivity.size()) {
        return Fail(error, "VTK topology offsets do not match connectivity");
    }

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    auto vtkOffsets = vtkSmartPointer<vtkIdTypeArray>::New();
    auto vtkConnectivity = vtkSmartPointer<vtkIdTypeArray>::New();
    auto types = vtkSmartPointer<vtkUnsignedCharArray>::New();
    vtkOffsets->SetNumberOfValues(static_cast<vtkIdType>(m_impl->offsets.size()));
    vtkConnectivity->SetNumberOfValues(static_cast<vtkIdType>(m_impl->connectivity.size()));
    types->SetNumberOfValues(static_cast<vtkIdType>(m_impl->cellCount));
    auto* vtkOffsetValues = static_cast<vtkIdType*>(vtkOffsets->GetVoidPointer(0));
    auto* vtkConnectivityValues = static_cast<vtkIdType*>(vtkConnectivity->GetVoidPointer(0));
    auto* vtkTypeValues = static_cast<unsigned char*>(types->GetVoidPointer(0));
    if (vtkOffsetValues == nullptr ||
        (!m_impl->connectivity.empty() && vtkConnectivityValues == nullptr) ||
        (m_impl->cellCount != 0u && vtkTypeValues == nullptr)) {
        return Fail(error, "failed to allocate VTK topology arrays");
    }
    for (std::size_t offsetIndex = 0; offsetIndex < m_impl->offsets.size(); ++offsetIndex) {
        vtkOffsetValues[offsetIndex] = static_cast<vtkIdType>(m_impl->offsets[offsetIndex]);
    }
    for (std::size_t connectivityIndex = 0;
         connectivityIndex < m_impl->connectivity.size();
         ++connectivityIndex) {
        const auto pointId = m_impl->connectivity[connectivityIndex];
        if (pointId >= m_impl->pointCount) {
            return Fail(error, "decoded VTK topology references an invalid point id");
        }
        vtkConnectivityValues[connectivityIndex] = static_cast<vtkIdType>(pointId);
    }

    for (std::size_t cellIndex = 0; cellIndex < m_impl->cellCount; ++cellIndex) {
        const auto begin = static_cast<std::size_t>(m_impl->offsets[cellIndex]);
        const auto end = static_cast<std::size_t>(m_impl->offsets[cellIndex + 1u]);
        if (begin > end || end > m_impl->connectivity.size()) {
            return Fail(error, "VTK topology contains a non-monotonic offset");
        }

        CellTypeCodecEntry entry;
        const auto rawCellType = m_impl->cellTypes[cellIndex];
        if (!m_impl->cellTypeMapping.ResolveCellType(rawCellType, entry) ||
            rawCellType > std::numeric_limits<unsigned char>::max()) {
            return Fail(error, "decoded package contains an unsupported VTK cell type");
        }

        vtkTypeValues[cellIndex] = static_cast<unsigned char>(rawCellType);
    }

    cells->SetData(vtkOffsets, vtkConnectivity);
    m_impl->output->SetCells(types, cells);
    m_impl->connectivity.clear();
    m_impl->offsets.clear();
    m_impl->cellTypes.clear();
    m_impl->cellCount = 0u;
    m_impl->hasOffsets = false;
    return true;
}

bool VtkDataCodecDecodeAdapter::SetStructuredAxisSize(
    const int[3],
    std::string* error) {
    return Fail(error, "VTK decode adapter does not support structured topology");
}

bool VtkDataCodecDecodeAdapter::SupportsPolyhedronTopology() const { return false; }

bool VtkDataCodecDecodeAdapter::BeginPolyhedronTopology(
    std::size_t,
    std::string* error) {
    return Fail(error, "VTK decode adapter does not support polyhedron topology");
}

bool VtkDataCodecDecodeAdapter::WritePolyhedronCellBatch(
    std::size_t,
    const PolyhedronTopologyView&,
    std::string* error) {
    return Fail(error, "VTK decode adapter does not support polyhedron topology");
}

bool VtkDataCodecDecodeAdapter::EndPolyhedronTopology(std::string* error) {
    return Fail(error, "VTK decode adapter does not support polyhedron topology");
}

bool VtkDataCodecDecodeAdapter::BeginAttribute(
    const std::size_t attrIndex,
    const AttrStorageParams& meta,
    std::string* error) {
    if (m_impl->output == nullptr || meta.dimension <= 0 ||
        meta.elementCount > static_cast<::datacodec::ParamSize>(std::numeric_limits<vtkIdType>::max())) {
        return Fail(error, "VTK attribute metadata is invalid");
    }
    auto array = CreateVtkArray(meta.dataType);
    if (array == nullptr) {
        return Fail(error, "VTK decode adapter only supports float and double attributes");
    }
    array->SetName(meta.name.c_str());
    array->SetNumberOfComponents(meta.dimension);
    array->SetNumberOfTuples(static_cast<vtkIdType>(meta.elementCount));

    if (attrIndex >= m_impl->attributes.size()) {
        m_impl->attributes.resize(attrIndex + 1u);
    }
    m_impl->attributes[attrIndex].meta = meta;
    m_impl->attributes[attrIndex].array = array;
    return true;
}

bool VtkDataCodecDecodeAdapter::WriteAttributeRange(
    const std::size_t attrIndex,
    const std::size_t offset,
    const std::size_t count,
    const void* data,
    const std::size_t byteSize,
    std::string* error) {
    if (count == 0u) {
        return true;
    }
    if (attrIndex >= m_impl->attributes.size() || data == nullptr) {
        return Fail(error, "VTK attribute range uses an unknown attribute");
    }
    auto& pending = m_impl->attributes[attrIndex];
    if (pending.array == nullptr ||
        pending.meta.elementCount > std::numeric_limits<std::size_t>::max()) {
        return Fail(error, "VTK attribute range has no writable array");
    }
    const auto elementCount = static_cast<std::size_t>(pending.meta.elementCount);
    if (offset > elementCount || count > elementCount - offset) {
        return Fail(error, "VTK attribute range is outside the target array");
    }
    const auto componentCount = static_cast<std::size_t>(pending.meta.dimension);
    const auto valueSize = ::datacodec::DataTypeSize(pending.meta.dataType);
    if (componentCount == 0u || valueSize == 0u ||
        componentCount > std::numeric_limits<std::size_t>::max() / valueSize) {
        return Fail(error, "VTK attribute tuple byte size is invalid");
    }
    const auto tupleByteSize = componentCount * valueSize;
    if (count > std::numeric_limits<std::size_t>::max() / tupleByteSize ||
        byteSize != count * tupleByteSize) {
        return Fail(error, "VTK attribute range byte size does not match metadata");
    }
    auto* target = static_cast<std::uint8_t*>(pending.array->GetVoidPointer(0));
    if (target == nullptr) {
        return Fail(error, "VTK attribute array has no writable storage");
    }
    std::memcpy(target + offset * tupleByteSize, data, byteSize);
    return true;
}

bool VtkDataCodecDecodeAdapter::SupportsConcurrentAttributeRangeWrites() const noexcept {
    return false;
}

bool VtkDataCodecDecodeAdapter::SupportsAttributeDecodeStore() const noexcept {
    return false;
}

bool VtkDataCodecDecodeAdapter::EndAttribute(
    const std::size_t attrIndex,
    std::string* error) {
    if (m_impl->output == nullptr || attrIndex >= m_impl->attributes.size() ||
        m_impl->attributes[attrIndex].array == nullptr) {
        return Fail(error, "VTK attribute commit uses an unknown attribute");
    }
    auto& pending = m_impl->attributes[attrIndex];
    vtkDataSetAttributes* target = pending.meta.attachmentType == AttrAttachment::Point
        ? static_cast<vtkDataSetAttributes*>(m_impl->output->GetPointData())
        : static_cast<vtkDataSetAttributes*>(m_impl->output->GetCellData());
    if (target == nullptr) {
        return Fail(error, "VTK output has no attribute container");
    }
    const auto nativeIndex = target->AddArray(pending.array);
    const auto nativeRole = ToVtkAttributeRole(pending.meta.type);
    if (nativeIndex < 0 ||
        (nativeRole >= 0 && target->SetActiveAttribute(nativeIndex, nativeRole) < 0)) {
        return Fail(error, "failed to attach decoded VTK attribute");
    }
    pending.array = nullptr;
    return true;
}

bool VtkDataCodecDecodeAdapter::ResolveCellType(
    const CellTypeRaw rawType,
    CellTypeCodecEntry& entry) const {
    return m_impl->cellTypeMapping.ResolveCellType(rawType, entry);
}

VtkDataCodecDecodeAdapter::CellTypeMappingMode
VtkDataCodecDecodeAdapter::GetCellTypeMappingMode() const {
    return m_impl->cellTypeMapping.GetCellTypeMappingMode();
}

bool VtkDataCodecDecodeAdapter::ResolveCellSizeFromPolynomialOrder(
    const CellTypeRaw rawType,
    const std::uint16_t order,
    int& size) const {
    return m_impl->cellTypeMapping.ResolveCellSizeFromPolynomialOrder(rawType, order, size);
}

bool VtkDataCodecDecodeAdapter::EncodeCellTypeFamilyLocal(
    const CellTypeRaw rawType,
    CellTypeFamilyCode& familyCode,
    CellTypeLocalCode& familyLocalCode) const {
    return m_impl->cellTypeMapping.EncodeCellTypeFamilyLocal(
        rawType,
        familyCode,
        familyLocalCode);
}

bool VtkDataCodecDecodeAdapter::DecodeCellTypeFamilyLocal(
    const CellTypeFamilyCode familyCode,
    const CellTypeLocalCode familyLocalCode,
    CellTypeRaw& rawType) const {
    return m_impl->cellTypeMapping.DecodeCellTypeFamilyLocal(
        familyCode,
        familyLocalCode,
        rawType);
}

void VtkDataCodecDecodeAdapter::Abort() { ResetOutput(); }

void VtkDataCodecDecodeAdapter::ResetOutput() {
    m_impl->output = nullptr;
    m_impl->points = nullptr;
    m_impl->pointCount = 0u;
    m_impl->cellCount = 0u;
    m_impl->hasOffsets = false;
    m_impl->connectivity.clear();
    m_impl->offsets.clear();
    m_impl->cellTypes.clear();
    m_impl->attributes.clear();
}

bool VtkDataCodecDecodeAdapter::Commit(std::string* error) {
    if (m_impl->output == nullptr) {
        return Fail(error, "VTK decode adapter has no output grid");
    }
    for (const auto& attribute : m_impl->attributes) {
        if (attribute.array != nullptr) {
            return Fail(error, "VTK decode adapter has an uncommitted attribute");
        }
    }
    return true;
}

vtkSmartPointer<vtkUnstructuredGrid> VtkDataCodecDecodeAdapter::TakeOutput() {
    auto output = m_impl->output;
    m_impl->output = nullptr;
    m_impl->attributes.clear();
    return output;
}

} // namespace vtk_datacodec_example
