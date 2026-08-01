#ifndef IGAME_EXAMPLES_VTK_DATACODEC_ADAPTERS_H
#define IGAME_EXAMPLES_VTK_DATACODEC_ADAPTERS_H

#include <DataCodec/API/Adapter/IDecodeAdapter.h>
#include <DataCodec/API/Adapter/IEncodeAdapter.h>

#include <vtkSmartPointer.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class vtkUnstructuredGrid;

namespace vtk_datacodec_example {

class VtkDataCodecEncodeAdapter final : public ::datacodec::IEncodeAdapter {
public:
    using IndexType = ::datacodec::IndexType;
    using MeshType = ::datacodec::MeshType;
    using ScalarType = ::datacodec::ScalarType;
    using TopologyInputDescriptor = ::datacodec::TopologyInputDescriptor;
    using CellTypeRaw = ::datacodec::CellTypeRaw;
    using CellTypeCodecEntry = ::datacodec::CellTypeCodecEntry;
    using CellTypeMappingMode = ::datacodec::CellTypeMappingMode;
    using CellTypeFamilyCode = ::datacodec::CellTypeFamilyCode;
    using CellTypeLocalCode = ::datacodec::CellTypeLocalCode;

    static std::unique_ptr<VtkDataCodecEncodeAdapter> Create(
        vtkUnstructuredGrid* input,
        std::string* error = nullptr);

    ~VtkDataCodecEncodeAdapter() override;

    VtkDataCodecEncodeAdapter(const VtkDataCodecEncodeAdapter&) = delete;
    VtkDataCodecEncodeAdapter& operator=(const VtkDataCodecEncodeAdapter&) = delete;

    MeshType GetMeshType() const override;
    std::string GetName() const override;

    std::size_t GetNumberOfPoints() const override;
    void GetPoint(std::size_t index, double output[3]) const override;
    ScalarType GetPointScalarType() const override;
    const float* TryGetPointsF32() const override;
    const double* TryGetPointsF64() const override;

    bool DescribeTopology(
        TopologyInputDescriptor& output,
        std::string* error = nullptr) const override;
    std::size_t GetNumberOfCells() const override;
    std::size_t GetCellIdBufferSize() const override;
    const IndexType* GetCellIdBufferPtr() const override;
    const IndexType* GetCellIdOffsetPtr() const override;
    bool IsFixedCellSize() const override;
    int GetFixedCellSize() const override;
    const IndexType* GetCellTypesPtr() const override;

    std::size_t GetCellFaceBufferSize() const override;

    std::size_t GetNumberOfPointAttrs() const override;
    const ::datacodec::IEncodeAttrView& GetPointAttr(std::size_t index) const override;
    std::size_t GetNumberOfCellAttrs() const override;
    const ::datacodec::IEncodeAttrView& GetCellAttr(std::size_t index) const override;

    bool ResolveCellType(CellTypeRaw rawType, CellTypeCodecEntry& entry) const override;
    CellTypeMappingMode GetCellTypeMappingMode() const override;
    bool ResolveCellSizeFromPolynomialOrder(
        CellTypeRaw rawType,
        std::uint16_t order,
        int& size) const override;
    bool EncodeCellTypeFamilyLocal(
        CellTypeRaw rawType,
        CellTypeFamilyCode& familyCode,
        CellTypeLocalCode& familyLocalCode) const override;
    bool DecodeCellTypeFamilyLocal(
        CellTypeFamilyCode familyCode,
        CellTypeLocalCode familyLocalCode,
        CellTypeRaw& rawType) const override;

    void ResetInput() override;
    void Abort() override;

private:
    class Impl;

    VtkDataCodecEncodeAdapter();
    bool Initialize(vtkUnstructuredGrid* input, std::string* error);

    std::unique_ptr<Impl> m_impl;
};

class VtkDataCodecDecodeAdapter final : public ::datacodec::IDecodeAdapter {
public:
    using IndexType = ::datacodec::IndexType;
    using MeshType = ::datacodec::MeshType;
    using PolyhedronTopologyView = ::datacodec::PolyhedronTopologyView;
    using AttrStorageParams = ::datacodec::AttrStorageParams;
    using CellTypeRaw = ::datacodec::CellTypeRaw;
    using CellTypeCodecEntry = ::datacodec::CellTypeCodecEntry;
    using CellTypeMappingMode = ::datacodec::CellTypeMappingMode;
    using CellTypeFamilyCode = ::datacodec::CellTypeFamilyCode;
    using CellTypeLocalCode = ::datacodec::CellTypeLocalCode;

    VtkDataCodecDecodeAdapter();
    ~VtkDataCodecDecodeAdapter() override;

    VtkDataCodecDecodeAdapter(const VtkDataCodecDecodeAdapter&) = delete;
    VtkDataCodecDecodeAdapter& operator=(const VtkDataCodecDecodeAdapter&) = delete;

    bool SetMeshType(MeshType type, std::string* error = nullptr) override;
    bool BeginPoints(
        std::size_t count,
        std::size_t dimension,
        std::string* error = nullptr) override;
    bool WritePointsRange(
        std::size_t offset,
        std::size_t count,
        const float* data,
        std::string* error = nullptr) override;
    bool EndPoints(std::string* error = nullptr) override;

    bool BeginTopology(
        std::size_t cellCount,
        std::size_t connectivityCount,
        bool hasOffsets,
        std::string* error = nullptr) override;
    bool WriteConnectivityRange(
        std::size_t offset,
        const IndexType* data,
        std::size_t count,
        std::string* error = nullptr) override;
    bool WriteOffsetsRange(
        std::size_t offset,
        const IndexType* data,
        std::size_t count,
        std::string* error = nullptr) override;
    bool WriteCellTypesRange(
        std::size_t offset,
        const IndexType* data,
        std::size_t count,
        std::string* error = nullptr) override;
    bool WriteCellPolynomialOrdersRange(
        std::size_t offset,
        const std::uint16_t* data,
        std::size_t count,
        std::string* error = nullptr) override;
    bool EndTopology(std::string* error = nullptr) override;
    bool SetStructuredAxisSize(const int size[3], std::string* error = nullptr) override;

    bool SupportsPolyhedronTopology() const override;
    bool BeginPolyhedronTopology(
        std::size_t cellCount,
        std::string* error = nullptr) override;
    bool WritePolyhedronCellBatch(
        std::size_t firstCell,
        const PolyhedronTopologyView& batch,
        std::string* error = nullptr) override;
    bool EndPolyhedronTopology(std::string* error = nullptr) override;

    bool BeginAttribute(
        std::size_t attrIndex,
        const AttrStorageParams& meta,
        std::string* error = nullptr) override;
    bool WriteAttributeRange(
        std::size_t attrIndex,
        std::size_t offset,
        std::size_t count,
        const void* data,
        std::size_t byteSize,
        std::string* error = nullptr) override;
    bool SupportsConcurrentAttributeRangeWrites() const noexcept override;
    bool SupportsAttributeDecodeStore() const noexcept override;
    bool EndAttribute(std::size_t attrIndex, std::string* error = nullptr) override;

    bool ResolveCellType(CellTypeRaw rawType, CellTypeCodecEntry& entry) const override;
    CellTypeMappingMode GetCellTypeMappingMode() const override;
    bool ResolveCellSizeFromPolynomialOrder(
        CellTypeRaw rawType,
        std::uint16_t order,
        int& size) const override;
    bool EncodeCellTypeFamilyLocal(
        CellTypeRaw rawType,
        CellTypeFamilyCode& familyCode,
        CellTypeLocalCode& familyLocalCode) const override;
    bool DecodeCellTypeFamilyLocal(
        CellTypeFamilyCode familyCode,
        CellTypeLocalCode familyLocalCode,
        CellTypeRaw& rawType) const override;

    void Abort() override;
    void ResetOutput() override;
    bool Commit(std::string* error = nullptr) override;

    vtkSmartPointer<vtkUnstructuredGrid> TakeOutput();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vtk_datacodec_example

#endif
