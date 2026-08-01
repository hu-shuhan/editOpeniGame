#include "VtkDataCodecAdapters.h"
#include "VtkUnstructuredGridIO.h"

#include <DataCodec/API/Entry/DataCodecDecodeEntry.h>
#include <DataCodec/API/Entry/DataCodecEncodeEntry.h>
#include <DataCodec/Filter/Adapter/iGameDecodeAdapter.h>
#include <DataCodec/Storage/ByteIO/ByteRange.h>

#include <iGameUnstructuredMesh.h>

#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kDefaultSourceFile = "./Models/Tet_Plane.vtk";
constexpr const char* kDefaultEncodedFile = "./Models/vtk_roundtrip.igc";
constexpr const char* kDefaultDecodedFile = "./Models/vtk_roundtrip.vtu";

bool Fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

bool WriteBinaryFile(
    const std::filesystem::path& path,
    const std::span<const std::uint8_t> bytes,
    std::string* error) {
    if (!path.parent_path().empty()) {
        std::error_code directoryError;
        std::filesystem::create_directories(path.parent_path(), directoryError);
        if (directoryError) {
            return Fail(error, "failed to create encoded output directory: " + directoryError.message());
        }
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return Fail(error, "failed to open encoded output file");
    }
    output.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    return output.good() ? true : Fail(error, "failed to write encoded output file");
}

bool CompareAttributeContainerLayout(
    vtkDataSetAttributes* source,
    vtkDataSetAttributes* decoded,
    const char* label,
    std::string* error) {
    if (source == nullptr || decoded == nullptr ||
        source->GetNumberOfArrays() != decoded->GetNumberOfArrays()) {
        return Fail(error, std::string(label) + " attribute count changed after round trip");
    }
    for (int arrayIndex = 0; arrayIndex < source->GetNumberOfArrays(); ++arrayIndex) {
        auto* sourceArray = source->GetArray(arrayIndex);
        auto* decodedArray = decoded->GetArray(arrayIndex);
        if (sourceArray == nullptr || decodedArray == nullptr ||
            sourceArray->GetDataType() != decodedArray->GetDataType() ||
            sourceArray->GetNumberOfComponents() != decodedArray->GetNumberOfComponents() ||
            sourceArray->GetNumberOfTuples() != decodedArray->GetNumberOfTuples()) {
            return Fail(error, std::string(label) + " attribute layout changed after round trip");
        }
        const std::string sourceName = sourceArray->GetName() != nullptr ? sourceArray->GetName() : "";
        const std::string decodedName = decodedArray->GetName() != nullptr ? decodedArray->GetName() : "";
        if (sourceName != decodedName) {
            return Fail(error, std::string(label) + " attribute name changed after round trip");
        }
    }
    return true;
}

template<typename TValue>
void AppendValue(std::vector<std::uint8_t>& output, const TValue& value) {
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
    output.insert(output.end(), bytes, bytes + sizeof(TValue));
}

bool AppendAttributeTuple(
    vtkDataSetAttributes* attributes,
    const vtkIdType tupleIndex,
    std::vector<std::uint8_t>& output,
    std::string* error) {
    if (attributes == nullptr) {
        return true;
    }
    for (int arrayIndex = 0; arrayIndex < attributes->GetNumberOfArrays(); ++arrayIndex) {
        auto* array = attributes->GetArray(arrayIndex);
        if (array == nullptr || tupleIndex < 0 || tupleIndex >= array->GetNumberOfTuples()) {
            return Fail(error, "VTK semantic signature encountered an invalid attribute tuple");
        }
        const auto componentCount = array->GetNumberOfComponents();
        std::vector<double> tuple(static_cast<std::size_t>(componentCount), 0.0);
        array->GetTuple(tupleIndex, tuple.data());
        if (array->GetDataType() == VTK_FLOAT) {
            for (const auto component : tuple) {
                AppendValue(output, static_cast<float>(component));
            }
        } else if (array->GetDataType() == VTK_DOUBLE) {
            for (const auto component : tuple) {
                AppendValue(output, component);
            }
        } else {
            return Fail(error, "VTK semantic signature encountered an unsupported attribute type");
        }
    }
    return true;
}

bool BuildPointSemanticKey(
    vtkUnstructuredGrid* grid,
    const vtkIdType pointId,
    std::vector<std::uint8_t>& output,
    std::string* error) {
    if (grid == nullptr || pointId < 0 || pointId >= grid->GetNumberOfPoints()) {
        return Fail(error, "VTK semantic signature encountered an invalid point id");
    }
    double point[3]{0.0, 0.0, 0.0};
    grid->GetPoint(pointId, point);
    for (const auto coordinate : point) {
        // DataCodec当前解码接口以float提交点坐标
        AppendValue(output, static_cast<float>(coordinate));
    }
    return AppendAttributeTuple(grid->GetPointData(), pointId, output, error);
}

bool BuildPointSemanticSignatures(
    vtkUnstructuredGrid* grid,
    std::vector<std::vector<std::uint8_t>>& output,
    std::string* error) {
    output.clear();
    output.reserve(static_cast<std::size_t>(grid->GetNumberOfPoints()));
    for (vtkIdType pointId = 0; pointId < grid->GetNumberOfPoints(); ++pointId) {
        std::vector<std::uint8_t> key;
        if (!BuildPointSemanticKey(grid, pointId, key, error)) {
            return false;
        }
        output.push_back(std::move(key));
    }
    std::sort(output.begin(), output.end());
    return true;
}

bool BuildCellSemanticSignatures(
    vtkUnstructuredGrid* grid,
    std::vector<std::vector<std::uint8_t>>& output,
    std::string* error) {
    output.clear();
    output.reserve(static_cast<std::size_t>(grid->GetNumberOfCells()));
    for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId) {
        vtkIdType pointCount = 0;
        const vtkIdType* pointIds = nullptr;
        grid->GetCellPoints(cellId, pointCount, pointIds);
        if (pointCount < 0 || (pointCount != 0 && pointIds == nullptr)) {
            return Fail(error, "VTK semantic signature encountered invalid cell connectivity");
        }

        std::vector<std::vector<std::uint8_t>> pointKeys;
        pointKeys.reserve(static_cast<std::size_t>(pointCount));
        for (vtkIdType localIndex = 0; localIndex < pointCount; ++localIndex) {
            std::vector<std::uint8_t> pointKey;
            if (!BuildPointSemanticKey(grid, pointIds[localIndex], pointKey, error)) {
                return false;
            }
            pointKeys.push_back(std::move(pointKey));
        }
        std::sort(pointKeys.begin(), pointKeys.end());

        std::vector<std::uint8_t> cellKey;
        AppendValue(cellKey, static_cast<std::uint32_t>(grid->GetCellType(cellId)));
        AppendValue(cellKey, static_cast<std::uint64_t>(pointKeys.size()));
        for (const auto& pointKey : pointKeys) {
            AppendValue(cellKey, static_cast<std::uint64_t>(pointKey.size()));
            cellKey.insert(cellKey.end(), pointKey.begin(), pointKey.end());
        }
        if (!AppendAttributeTuple(grid->GetCellData(), cellId, cellKey, error)) {
            return false;
        }
        output.push_back(std::move(cellKey));
    }
    std::sort(output.begin(), output.end());
    return true;
}

bool VerifyRoundTrip(
    vtkUnstructuredGrid* source,
    vtkUnstructuredGrid* decoded,
    std::string* error) {
    if (source == nullptr || decoded == nullptr ||
        source->GetNumberOfPoints() != decoded->GetNumberOfPoints() ||
        source->GetNumberOfCells() != decoded->GetNumberOfCells()) {
        return Fail(error, "VTK grid dimensions changed after round trip");
    }

    if (!CompareAttributeContainerLayout(
               source->GetPointData(),
               decoded->GetPointData(),
               "point",
               error) ||
        !CompareAttributeContainerLayout(
               source->GetCellData(),
               decoded->GetCellData(),
               "cell",
               error)) {
        return false;
    }

    std::vector<std::vector<std::uint8_t>> sourcePointSignatures;
    std::vector<std::vector<std::uint8_t>> decodedPointSignatures;
    std::vector<std::vector<std::uint8_t>> sourceCellSignatures;
    std::vector<std::vector<std::uint8_t>> decodedCellSignatures;
    if (!BuildPointSemanticSignatures(source, sourcePointSignatures, error) ||
        !BuildPointSemanticSignatures(decoded, decodedPointSignatures, error) ||
        sourcePointSignatures != decodedPointSignatures) {
        return error != nullptr && !error->empty()
            ? false
            : Fail(error, "VTK point semantics changed after round trip");
    }
    if (!BuildCellSemanticSignatures(source, sourceCellSignatures, error) ||
        !BuildCellSemanticSignatures(decoded, decodedCellSignatures, error) ||
        sourceCellSignatures != decodedCellSignatures) {
        return error != nullptr && !error->empty()
            ? false
            : Fail(error, "VTK cell semantics changed after round trip");
    }
    return true;
}

void PrintMessages(const std::vector<::datacodec::TelemetryMessageRecord>& messages) {
    for (const auto& message : messages) {
        std::cerr << message.text << '\n';
    }
}

bool VerifyIgameDecode(
    const std::shared_ptr<const std::vector<std::uint8_t>>& encodedBytes,
    vtkUnstructuredGrid* source,
    const ::datacodec::DataCodecDecodePackageConfigurationParams& configuration,
    std::string* error) {
    auto inputReader = std::make_shared<::datacodec::MemoryByteRangeReader>(encodedBytes);
    iGame::iGameDecodeAdapter adapter;
    auto result = ::datacodec::DecodePackage({
        .inputReader = inputReader,
        .leafAdapter = &adapter,
        .attributeSelection = ::datacodec::AttributeSelectionMode::AllAvailable,
        .configuration = configuration,
        .executionResources = {},
    });
    if (!result.success) {
        std::ostringstream message;
        message << "iGame adapter failed to decode VTK DataCodec bytes";
        for (const auto& record : result.messages) {
            message << '\n' << record.text;
        }
        return Fail(error, message.str());
    }

    const auto object = adapter.TakeDataObject();
    const auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(object);
    if (mesh == nullptr || source == nullptr ||
        mesh->GetNumberOfPoints() != static_cast<std::size_t>(source->GetNumberOfPoints()) ||
        mesh->GetNumberOfCells() != static_cast<std::size_t>(source->GetNumberOfCells())) {
        return Fail(error, "iGame adapter produced an unexpected unstructured grid");
    }
    return true;
}

void ShowDecodedGrid(vtkUnstructuredGrid* grid) {
    if (grid == nullptr) {
        return;
    }

    // 映射器把 vtkUnstructuredGrid 的几何与拓扑交给渲染管线
    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputData(grid);

    // 若存在点属性标量，则用它驱动着色与颜色条
    if (grid->GetPointData() != nullptr && grid->GetPointData()->GetScalars() != nullptr) {
        mapper->SetScalarModeToUsePointData();
        mapper->SelectColorArray(grid->GetPointData()->GetScalars()->GetName());
        mapper->SetScalarRange(grid->GetPointData()->GetScalars()->GetRange());
    } else if (grid->GetCellData() != nullptr && grid->GetCellData()->GetScalars() != nullptr) {
        mapper->SetScalarModeToUseCellData();
        mapper->SelectColorArray(grid->GetCellData()->GetScalars()->GetName());
        mapper->SetScalarRange(grid->GetCellData()->GetScalars()->GetRange());
    }

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> renderer;
    renderer->AddActor(actor);
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->ResetCamera();

    vtkNew<vtkRenderWindow> window;
    window->SetWindowName("DataCodec VTK Round Trip - Decoded Grid");
    window->SetSize(1024, 768);
    window->AddRenderer(renderer);

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(window);
    interactor->Initialize();
    interactor->Start();
}

} // namespace

int main(const int argc, char** argv) {
    if (argc > 4) {
        std::cerr << "usage: testDataCodecVtkUnstructuredRoundTrip [input.vtk|input.vtu] [output.igc] [decoded.vtk|decoded.vtu]\n";
        return 2;
    }

    const std::filesystem::path sourceFile = argc >= 2 ? argv[1] : kDefaultSourceFile;
    const std::filesystem::path encodedFile = argc >= 3 ? argv[2] : kDefaultEncodedFile;
    const std::filesystem::path decodedFile = argc >= 4 ? argv[3] : kDefaultDecodedFile;
    std::string error;

    // VTK IO只负责产生和保存原生vtkUnstructuredGrid
    auto source = vtk_datacodec_example::ReadVtkUnstructuredGrid(sourceFile, &error);
    if (source == nullptr) {
        std::cerr << error << '\n';
        return 1;
    }

    // Encode adapter把VTK对象桥接为DataCodec借用视图
    auto encodeAdapter = vtk_datacodec_example::VtkDataCodecEncodeAdapter::Create(
        source,
        &error);
    if (encodeAdapter == nullptr) {
        std::cerr << error << '\n';
        return 1;
    }

    auto encodeResult = ::datacodec::Encode({
        .input = ::datacodec::EncodeInput::LeafAdapter(encodeAdapter.get()),
        .output = ::datacodec::EncodeOutput::Memory(
            ::datacodec::EncodePackageKind::LeafPackage),
        .attributeSelection = ::datacodec::AttributeSelectionMode::AllAvailable,
        .configuration = ::datacodec::MakeEncodeConfigurationParams({
            .tier = ::datacodec::DataCodecEncodeTier::Balanced,
            .enableCompressionEnhancement = false,
        }),
        .executionResources = {},
    });
    if (!encodeResult.success || !encodeResult.hasEncodedOutput) {
        std::cerr << "DataCodec VTK encode failed\n";
        PrintMessages(encodeResult.messages);
        return 1;
    }

    auto encodedBytes = std::make_shared<const std::vector<std::uint8_t>>(
        std::move(encodeResult.encodedBytes));
    if (!WriteBinaryFile(encodedFile, *encodedBytes, &error)) {
        std::cerr << error << '\n';
        return 1;
    }

    // DecodePackage直接把同一份编码字节写入VTK decode adapter
    auto inputReader = std::make_shared<::datacodec::MemoryByteRangeReader>(encodedBytes);
    vtk_datacodec_example::VtkDataCodecDecodeAdapter decodeAdapter;
    const auto decodeConfiguration = ::datacodec::MakeDecodeConfigurationParams({
        .tier = ::datacodec::DataCodecDecodeTier::Balanced,
        .validationProfile = ::datacodec::DataCodecDecodeValidationProfile::Required,
    });
    auto decodeResult = ::datacodec::DecodePackage({
        .inputReader = inputReader,
        .leafAdapter = &decodeAdapter,
        .attributeSelection = ::datacodec::AttributeSelectionMode::AllAvailable,
        .configuration = decodeConfiguration.PackageConfiguration(),
        .executionResources = {},
    });
    if (!decodeResult.success) {
        std::cerr << "DataCodec VTK decode failed\n";
        PrintMessages(decodeResult.messages);
        return 1;
    }

    auto decoded = decodeAdapter.TakeOutput();
    if (decoded == nullptr || !VerifyRoundTrip(source, decoded, &error)) {
        std::cerr << (error.empty() ? "VTK round-trip verification failed" : error) << '\n';
        return 1;
    }
    if (!VerifyIgameDecode(
            encodedBytes,
            source,
            decodeConfiguration.PackageConfiguration(),
            &error)) {
        std::cerr << error << '\n';
        return 1;
    }
    if (!vtk_datacodec_example::WriteVtkUnstructuredGrid(decodedFile, decoded, &error)) {
        std::cerr << error << '\n';
        return 1;
    }

    std::cout << "VTK source file: " << sourceFile.string() << '\n';
    std::cout << "DataCodec encoded file: " << encodedFile.string() << '\n';
    std::cout << "VTK decoded file: " << decodedFile.string() << '\n';
    std::cout << "Points: " << decoded->GetNumberOfPoints() << '\n';
    std::cout << "Cells: " << decoded->GetNumberOfCells() << '\n';
    std::cout << "Encoded bytes: " << encodedBytes->size() << '\n';
    std::cout << "VTK DataCodec round trip verified\n";
    std::cout << "VTK to iGame DataCodec decode verified\n";

    // 用 VTK 渲染窗口显示解码后的 vtkUnstructuredGrid
    std::cout << "Launching VTK render window for decoded grid...\n";
    ShowDecodedGrid(decoded);
    return 0;
}
