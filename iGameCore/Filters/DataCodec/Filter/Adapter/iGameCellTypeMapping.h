#ifndef iGameDataCodeciGameCellTypeMapping_h
#define iGameDataCodeciGameCellTypeMapping_h

#include "DataCodec/API/Adapter/ICellTypeMapping.h"

#include "iGameCellType.h"

#include <array>
#include <cstdint>
#include <limits>

IGAME_NAMESPACE_BEGIN
namespace datacodec_cell_mapping = ::datacodec;

inline constexpr std::array<datacodec_cell_mapping::CellTypeNativeMappingEntry, datacodec_cell_mapping::kCodecCellTypeCount> kIgameCellTypeMappingEntries{{
    {datacodec_cell_mapping::CodecCellTypeId::Vertex, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_VERTEX)},
    {datacodec_cell_mapping::CodecCellTypeId::Triangle, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_TRIANGLE)},
    {datacodec_cell_mapping::CodecCellTypeId::Quad, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_QUAD)},
    {datacodec_cell_mapping::CodecCellTypeId::Tetra, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_TETRA)},
    {datacodec_cell_mapping::CodecCellTypeId::Hexahedron, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_HEXAHEDRON)},
    {datacodec_cell_mapping::CodecCellTypeId::Pyramid, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_PYRAMID)},
    {datacodec_cell_mapping::CodecCellTypeId::Prism, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_PRISM)},
    {datacodec_cell_mapping::CodecCellTypeId::QuadraticEdge, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_QUADRATIC_EDGE)},
    {datacodec_cell_mapping::CodecCellTypeId::QuadraticTriangle, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_QUADRATIC_TRIANGLE)},
    {datacodec_cell_mapping::CodecCellTypeId::QuadraticQuad, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_QUADRATIC_QUAD)},
    {datacodec_cell_mapping::CodecCellTypeId::QuadraticTetra, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_QUADRATIC_TETRA)},
    {datacodec_cell_mapping::CodecCellTypeId::QuadraticHexahedron, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_QUADRATIC_HEXAHEDRON)},
    {datacodec_cell_mapping::CodecCellTypeId::QuadraticPrism, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_QUADRATIC_PRISM)},
    {datacodec_cell_mapping::CodecCellTypeId::QuadraticPyramid, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_QUADRATIC_PYRAMID)},
    {datacodec_cell_mapping::CodecCellTypeId::Line, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_LINE)},
    {datacodec_cell_mapping::CodecCellTypeId::Polygon, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_POLYGON)},
    {datacodec_cell_mapping::CodecCellTypeId::PolyLine, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_POLY_LINE)},
    {datacodec_cell_mapping::CodecCellTypeId::Polyhedron, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_POLYHEDRON)},
    {datacodec_cell_mapping::CodecCellTypeId::Face, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_FACE)},
    {datacodec_cell_mapping::CodecCellTypeId::Volume, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_VOLUME)},
    {datacodec_cell_mapping::CodecCellTypeId::LagrangeCurve, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_LAGRANGE_CURVE)},
    {datacodec_cell_mapping::CodecCellTypeId::LagrangeTriangle, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_LAGRANGE_TRIANGLE)},
    {datacodec_cell_mapping::CodecCellTypeId::LagrangeQuadrilateral, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_LAGRANGE_QUADRILATERAL)},
    {datacodec_cell_mapping::CodecCellTypeId::LagrangeTetrahedron, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_LAGRANGE_TETRAHEDRON)},
    {datacodec_cell_mapping::CodecCellTypeId::LagrangeHexahedron, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_LAGRANGE_HEXAHEDRON)},
    {datacodec_cell_mapping::CodecCellTypeId::LagrangePrism, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_LAGRANGE_PRISM)},
    {datacodec_cell_mapping::CodecCellTypeId::LagrangePyramid, static_cast<datacodec_cell_mapping::CellTypeRaw>(IG_LAGRANGE_PYRAMID)},
}};

static_assert(
    datacodec_cell_mapping::IsCompleteCellTypeMapping(kIgameCellTypeMappingEntries),
    "iGame cell type mapping must cover every DataCodec codec cell type exactly once");

struct iGameCellTypeMapping final : public datacodec_cell_mapping::ICellTypeMapping {
    datacodec_cell_mapping::CellTypeMappingMode GetCellTypeMappingMode() const override {
        return datacodec_cell_mapping::CellTypeMappingMode::FamilyLocal;
    }

    bool ResolveCellType(const datacodec_cell_mapping::CellTypeRaw rawType, datacodec_cell_mapping::CellTypeCodecEntry& entry) const override {
        return datacodec_cell_mapping::ResolveMappedCellType(kIgameCellTypeMappingEntries, rawType, entry);
    }

    static bool ResolveLagrangeCellSize(const datacodec_cell_mapping::CellTypeRaw rawType, const std::uint16_t order, int& size) {
        return datacodec_cell_mapping::ResolveMappedCellSizeFromPolynomialOrder(kIgameCellTypeMappingEntries, rawType, order, size);
    }

    bool ResolveCellSizeFromPolynomialOrder(
        const datacodec_cell_mapping::CellTypeRaw rawType,
        const std::uint16_t order,
        int& size) const override {
        return ResolveLagrangeCellSize(rawType, order, size);
    }

    bool EncodeCellTypeFamilyLocal(
        const datacodec_cell_mapping::CellTypeRaw rawType,
        datacodec_cell_mapping::CellTypeFamilyCode& familyCode,
        datacodec_cell_mapping::CellTypeLocalCode& familyLocalCode) const override {
        return datacodec_cell_mapping::EncodeMappedCellTypeFamilyLocal(
            kIgameCellTypeMappingEntries,
            rawType,
            familyCode,
            familyLocalCode);
    }

    bool DecodeCellTypeFamilyLocal(
        const datacodec_cell_mapping::CellTypeFamilyCode familyCode,
        const datacodec_cell_mapping::CellTypeLocalCode familyLocalCode,
        datacodec_cell_mapping::CellTypeRaw& rawType) const override {
        return datacodec_cell_mapping::DecodeMappedCellTypeFamilyLocal(
            kIgameCellTypeMappingEntries,
            familyCode,
            familyLocalCode,
            rawType);
    }

    bool EncodeCellPolynomialOrderLocal(
        const datacodec_cell_mapping::CellTypeRaw rawType,
        const std::uint16_t order,
        datacodec_cell_mapping::CellTypeLocalCode& localOrder) const override {
        int size = -1;
        if (!ResolveLagrangeCellSize(rawType, order, size)) {
            localOrder = 0u;
            return false;
        }
        localOrder = static_cast<datacodec_cell_mapping::IndexType>(order - 1u);
        return true;
    }

    bool DecodeCellPolynomialOrderLocal(
        const datacodec_cell_mapping::CellTypeRaw rawType,
        const datacodec_cell_mapping::CellTypeLocalCode localOrder,
        std::uint16_t& order) const override {
        if (localOrder >= std::numeric_limits<std::uint16_t>::max()) {
            order = 0u;
            return false;
        }

        const auto decodedOrder = static_cast<std::uint16_t>(localOrder + 1u);
        int size = -1;
        if (!ResolveLagrangeCellSize(rawType, decodedOrder, size)) {
            order = 0u;
            return false;
        }

        order = decodedOrder;
        return true;
    }
};

IGAME_NAMESPACE_END

#endif
