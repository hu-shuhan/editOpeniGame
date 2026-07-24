#ifndef DATACODEC_TEST_DATA_DATACODECTESTDATASET_H
#define DATACODEC_TEST_DATA_DATACODECTESTDATASET_H

#include "DataCodec/Common/DataCodecTypes.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace datacodec::test {

struct TestNumericField {
    std::string name;
    AttrRole role{AttrRole::Unknown};
    AttrAttachment attachment{AttrAttachment::Point};
    std::size_t componentCount{1u};
    std::vector<float> values;

    [[nodiscard]] std::size_t ElementCount() const noexcept {
        return componentCount == 0u ? 0u : values.size() / componentCount;
    }
};

struct TestDataset {
    std::string name;
    MeshType meshType{MeshType::PointSet};
    std::vector<float> points;
    std::vector<IndexType> cellConnectivity;
    std::vector<IndexType> cellOffsets;
    std::vector<IndexType> cellTypes;
    std::vector<TestNumericField> pointFields;
    std::vector<TestNumericField> cellFields;

    [[nodiscard]] std::size_t PointCount() const noexcept {
        return points.size() / 3u;
    }

    [[nodiscard]] std::size_t CellCount() const noexcept {
        return cellOffsets.empty() ? 0u : cellOffsets.size() - 1u;
    }
};

[[nodiscard]] inline TestDataset MakeAdapterRoundTripDataset() {
    TestDataset dataset;
    dataset.name = "adapter_round_trip";
    dataset.points = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
    };
    dataset.pointFields.push_back(TestNumericField{
        .name = "temperature",
        .role = AttrRole::Scalar,
        .attachment = AttrAttachment::Point,
        .componentCount = 1u,
        .values = {10.0f, 20.0f, 30.0f, 40.0f},
    });
    dataset.pointFields.push_back(TestNumericField{
        .name = "velocity",
        .role = AttrRole::Vector,
        .attachment = AttrAttachment::Point,
        .componentCount = 3u,
        .values = {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
        },
    });
    return dataset;
}

[[nodiscard]] inline TestDataset MakePipelineContractUnstructuredDataset() {
    TestDataset dataset;
    dataset.name = "pipeline_contract_unstructured";
    dataset.meshType = MeshType::UnstructuredMesh;
    dataset.points = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        2.0f, 0.0f, 0.0f,
        2.0f, 1.0f, 0.0f,
        2.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
    };
    dataset.cellConnectivity = {
        0u, 1u, 2u, 3u,
        4u, 5u, 6u, 7u,
    };
    dataset.cellOffsets = {0u, 4u, 8u};
    std::vector<float> pressure(dataset.PointCount(), 0.0f);
    std::vector<float> affinePressure(dataset.PointCount(), 0.0f);
    for (std::size_t index = 0u; index < pressure.size(); ++index) {
        pressure[index] = static_cast<float>(index) * 0.25f + 1.0f;
        affinePressure[index] = pressure[index] * 2.0f + 0.5f;
    }
    dataset.pointFields.push_back(TestNumericField{
        .name = "pressure",
        .role = AttrRole::Scalar,
        .attachment = AttrAttachment::Point,
        .componentCount = 1u,
        .values = std::move(pressure),
    });
    dataset.pointFields.push_back(TestNumericField{
        .name = "pressure_affine",
        .role = AttrRole::Scalar,
        .attachment = AttrAttachment::Point,
        .componentCount = 1u,
        .values = std::move(affinePressure),
    });
    dataset.cellFields.push_back(TestNumericField{
        .name = "cell_quality",
        .role = AttrRole::Scalar,
        .attachment = AttrAttachment::Cell,
        .componentCount = 1u,
        .values = {0.25f, 0.75f},
    });
    return dataset;
}

} // datacodec::test命名空间

#endif
