//
// Created by m_ky on 2024/7/21.
//

/**
 * @class   iGameVTMReader
 * @brief   iGameVTMReader's brief
 */

#include "iGameVTMReader.h"
#include "VTK/iGameVTKReader.h"
#include "iGameVTSReader.h"
#include "iGameVTUReader.h"
#include "iGameDrawObject.h"
#include "iGameFlatArray.h"
#include "iGameUnstructuredMesh.h"
#include "CGNS/iGameCGNSReader.h"
#include "Log/iGameLogger.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <string>
#include <system_error>
#include <tinyxml2.h>
#include <utility>
#include <vector>


IGAME_NAMESPACE_BEGIN
namespace {
struct CommonAttributeSchema {
    std::string name;
    IGenum type{IG_NONE};
    IGenum attachmentType{IG_NONE};
    IGenum arrayType{IG_ARRAY_OBJECT};
    int dimension{0};
    std::vector<double> minimum;
    std::vector<double> maximum;
    std::vector<bool> observed;
    IGsize skippedNonFiniteElements{0};
};

bool PublishCommonFlatAttributes(const DrawObject::Pointer& parent) {
    if (!parent || !parent->HasSubDataObject()) { return true; }

    auto* parentAttributes = parent->GetAttributeSet();
    if (!parentAttributes || parentAttributes->GetNumberOfAttributes() != 0) {
        IGAME_CORE_WARN("[VTM] Common attribute proxy publication skipped: parent already has attributes");
        return false;
    }

    std::vector<DataObject::Pointer> leaves;
    leaves.reserve(static_cast<std::size_t>(parent->GetNumberOfSubDataObjects()));
    for (auto it = parent->SubDataObjectIteratorBegin(); it != parent->SubDataObjectIteratorEnd(); ++it) {
        if (!it->second || !it->second->IsDrawable()) {
            IGAME_CORE_WARN("[VTM] Common attribute proxy publication skipped: flat piece {} is not drawable",
                            leaves.size());
            return false;
        }
        leaves.push_back(it->second);
    }
    if (leaves.empty()) { return true; }

    auto* firstAttributes = leaves.front()->GetAttributeSet();
    if (!firstAttributes) {
        IGAME_CORE_WARN("[VTM] Common attribute proxy publication skipped: first piece has no AttributeSet");
        return false;
    }

    const std::size_t attributeCount = firstAttributes->GetNumberOfAttributes();
    if (attributeCount == 0) {
        IGAME_CORE_INFO("[VTM] Flat pieces have no attributes; no root attribute proxies were published");
        return true;
    }

    // Validate the complete per-index schema before publishing anything.  An
    // index-based proxy is safe only when every leaf uses exactly the same
    // order, name, semantic type, attachment, dimension, and numeric storage
    // type.  This keeps ViewCloudPicture(index, ...) deterministic.
    std::vector<CommonAttributeSchema> commonAttributes;
    commonAttributes.reserve(attributeCount);
    for (std::size_t attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex) {
        auto& reference = firstAttributes->GetAttribute(static_cast<IGsize>(attributeIndex));
        if (reference.IsNone() || !reference.pointer || reference.pointer->GetDimension() <= 0) {
            IGAME_CORE_WARN("[VTM] Common attribute proxy publication skipped: invalid attribute {} on piece 0",
                            attributeIndex);
            return false;
        }

        CommonAttributeSchema schema;
        schema.name = reference.pointer->GetName();
        schema.type = reference.type;
        schema.attachmentType = reference.attachmentType;
        schema.arrayType = reference.pointer->GetArrayType();
        schema.dimension = reference.pointer->GetDimension();
        schema.minimum.assign(static_cast<std::size_t>(schema.dimension + 1),
                              std::numeric_limits<double>::infinity());
        schema.maximum.assign(static_cast<std::size_t>(schema.dimension + 1),
                              -std::numeric_limits<double>::infinity());
        schema.observed.assign(static_cast<std::size_t>(schema.dimension + 1), false);
        commonAttributes.push_back(std::move(schema));
    }

    for (std::size_t pieceIndex = 0; pieceIndex < leaves.size(); ++pieceIndex) {
        auto* attributes = leaves[pieceIndex]->GetAttributeSet();
        if (!attributes || attributes->GetNumberOfAttributes() != attributeCount) {
            IGAME_CORE_WARN(
                    "[VTM] Common attribute proxy publication skipped: piece {} has {} attributes, expected {}",
                    pieceIndex,
                    attributes ? attributes->GetNumberOfAttributes() : 0,
                    attributeCount);
            return false;
        }

        for (std::size_t attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex) {
            auto& attribute = attributes->GetAttribute(static_cast<IGsize>(attributeIndex));
            const auto& schema = commonAttributes[attributeIndex];
            if (attribute.IsNone() || !attribute.pointer || attribute.pointer->GetName() != schema.name ||
                attribute.type != schema.type || attribute.attachmentType != schema.attachmentType ||
                attribute.pointer->GetDimension() != schema.dimension ||
                attribute.pointer->GetArrayType() != schema.arrayType) {
                IGAME_CORE_WARN(
                        "[VTM] Common attribute proxy publication skipped: schema mismatch at piece {}, attribute {}",
                        pieceIndex,
                        attributeIndex);
                return false;
            }

            const IGsize valueCount = attribute.pointer->GetNumberOfValues();
            if (valueCount % schema.dimension != 0) {
                IGAME_CORE_WARN(
                        "[VTM] Common attribute proxy publication skipped: piece {}, attribute '{}' has {} values "
                        "which is not divisible by dimension {}",
                        pieceIndex,
                        schema.name,
                        valueCount,
                        schema.dimension);
                return false;
            }
        }
    }

    // Compute ranges directly from the leaf arrays.  Do not use DBL_MIN as a
    // maximum initializer: DBL_MIN is a small positive number and corrupts the
    // range of an all-negative component.  Element 0 is magnitude; elements
    // 1..N are the signed component ranges.
    for (std::size_t attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex) {
        auto& aggregate = commonAttributes[attributeIndex];
        for (const auto& leaf: leaves) {
            auto& attribute = leaf->GetAttributeSet()->GetAttribute(static_cast<IGsize>(attributeIndex));
            auto& values = attribute.pointer;
            const IGsize valueCount = values->GetNumberOfValues();
            for (IGsize valueIndex = 0; valueIndex < valueCount; valueIndex += aggregate.dimension) {
                double magnitude = 0.0;
                bool magnitudeIsFinite = true;
                for (int component = 0; component < aggregate.dimension; ++component) {
                    const double value = values->GetValue(valueIndex + component);
                    if (!std::isfinite(value)) {
                        magnitudeIsFinite = false;
                        continue;
                    }

                    const std::size_t rangeIndex = static_cast<std::size_t>(component + 1);
                    aggregate.minimum[rangeIndex] = std::min(aggregate.minimum[rangeIndex], value);
                    aggregate.maximum[rangeIndex] = std::max(aggregate.maximum[rangeIndex], value);
                    aggregate.observed[rangeIndex] = true;
                    magnitude = std::hypot(magnitude, value);
                }

                if (magnitudeIsFinite) {
                    aggregate.minimum[0] = std::min(aggregate.minimum[0], magnitude);
                    aggregate.maximum[0] = std::max(aggregate.maximum[0], magnitude);
                    aggregate.observed[0] = true;
                } else {
                    ++aggregate.skippedNonFiniteElements;
                }
            }
        }

        if (std::find(aggregate.observed.begin(), aggregate.observed.end(), false) != aggregate.observed.end()) {
            IGAME_CORE_WARN(
                    "[VTM] Common attribute proxy publication skipped: attribute '{}' has no finite data for at "
                    "least one range component",
                    aggregate.name);
            return false;
        }
    }

    // Publication is deliberately deferred until validation and aggregation
    // have both succeeded, so heterogeneous data never leaves a partial root
    // schema behind.  The proxy arrays contain no values; only metadata and a
    // small global range array are owned by the parent.
    for (auto& aggregate: commonAttributes) {
        DoubleArray::Pointer proxy = DoubleArray::New();
        proxy->SetName(aggregate.name);
        proxy->SetDimension(aggregate.dimension);

        DoubleArray::Pointer globalRange = DoubleArray::New();
        globalRange->SetDimension(2);
        globalRange->Resize(aggregate.dimension + 1);
        for (int rangeIndex = 0; rangeIndex < aggregate.dimension + 1; ++rangeIndex) {
            const double range[2]{aggregate.minimum[static_cast<std::size_t>(rangeIndex)],
                                  aggregate.maximum[static_cast<std::size_t>(rangeIndex)]};
            globalRange->SetElement(rangeIndex, range);
        }
        globalRange->Modified();

        parentAttributes->AddAttribute(aggregate.type, aggregate.attachmentType, proxy, globalRange);
        if (aggregate.skippedNonFiniteElements > 0) {
            IGAME_CORE_WARN("[VTM] Attribute '{}' range ignored {} tuples containing non-finite values",
                            aggregate.name,
                            aggregate.skippedNonFiniteElements);
        }
    }

    // Share each parent's global range object with the corresponding source
    // leaf and the leaf's already-created renderable mesh.  No field array is
    // copied by this operation.
    if (!parent->UpdateSubDataObjectDataRange()) {
        IGAME_CORE_WARN("[VTM] Root attribute proxies were published, but global ranges could not be propagated");
        return false;
    }

    IGAME_CORE_INFO("[VTM] Published {} common root attribute proxies with global ranges (no values copied)",
                    attributeCount);
    return true;
}
} // namespace

bool iGameVTMReader::Parsing() {
    namespace fs = std::filesystem;

    parseData = nullptr;
    m_Output = nullptr;

    const fs::path manifestPath = fs::path(m_FilePath);
    const fs::path manifestDirectory = manifestPath.parent_path();

    tinyxml2::XMLElement* vtkMultiBlockElem = FindTargetItem(root, "vtkMultiBlockDataSet");
    if (!vtkMultiBlockElem) {
        IGAME_CORE_ERROR("[VTM] Missing vtkMultiBlockDataSet element in {}", m_FilePath);
        return false;
    }

    // Support both <Block> and <DataSet> formats
    tinyxml2::XMLElement* BlockElem = vtkMultiBlockElem->FirstChildElement("Block");
    tinyxml2::XMLElement* DataSetElem = vtkMultiBlockElem->FirstChildElement("DataSet");
    const bool isFlatDataSet = BlockElem == nullptr && DataSetElem != nullptr;

    std::size_t totalFileCount = 0;
    if (BlockElem) {
        for (auto* block = BlockElem; block; block = block->NextSiblingElement("Block")) {
            for (auto* elem = block->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
                if (elem->Attribute("file")) { ++totalFileCount; }
            }
        }
    } else {
        for (auto* dataSet = DataSetElem; dataSet; dataSet = dataSet->NextSiblingElement("DataSet")) {
            if (dataSet->Attribute("file")) { ++totalFileCount; }
        }
    }

    if (totalFileCount == 0) {
        IGAME_CORE_ERROR("[VTM] No referenced files found in {}", m_FilePath);
        return false;
    }

    std::size_t currentFileCount = 0;
    std::size_t successfulFileCount = 0;
    IGsize unstructuredPointRecords = 0;
    IGsize unstructuredCellRecords = 0;
    std::size_t unstructuredPieceCount = 0;
    IGAME_CORE_INFO("[VTM] Found {} referenced files in {}", totalFileCount, m_FilePath);
    const bool disableEagerPieceLod = isFlatDataSet && totalFileCount > 1;
    if (disableEagerPieceLod) {
        IGAME_CORE_INFO(
                "[VTM] Eager per-piece interaction LOD is disabled for this flat multi-piece data set; "
                "interaction falls back to the complete piece geometry");
    }

    std::vector<DataObject::Pointer> overall_multiBlock;

    auto readReferencedFile = [&](const char* fileAttribute) -> DataObject::Pointer {
        ++currentFileCount;

        const std::string fileName(fileAttribute);
        const fs::path referencedPath = fs::path(fileName);
        const fs::path resolvedPath =
                (referencedPath.is_absolute() ? referencedPath : manifestDirectory / referencedPath).lexically_normal();
        const std::string resolvedFileName = resolvedPath.string();

        IGAME_CORE_INFO("[VTM] Reading file {}/{}: {} -> {}",
                        currentFileCount,
                        totalFileCount,
                        fileName,
                        resolvedFileName);

        std::error_code pathError;
        if (!fs::exists(resolvedPath, pathError) || pathError) {
            IGAME_CORE_ERROR("[VTM] Referenced file does not exist: '{}' (resolved to '{}')",
                             fileName,
                             resolvedFileName);
            return nullptr;
        }

        std::string fileSuffix = resolvedPath.extension().string();
        if (!fileSuffix.empty() && fileSuffix.front() == '.') { fileSuffix.erase(fileSuffix.begin()); }
        std::transform(fileSuffix.begin(), fileSuffix.end(), fileSuffix.begin(), [](unsigned char value) {
            return static_cast<char>(std::tolower(value));
        });

        DataObject::Pointer object = nullptr;
        bool readSucceeded = false;
        if (fileSuffix == "vts") {
            iGameVTSReader::Pointer reader = iGameVTSReader::New();
            reader->SetFilePath(resolvedFileName);
            readSucceeded = reader->Execute();
            if (readSucceeded) { object = reader->GetOutput(); }
        } else if (fileSuffix == "vtu") {
            iGameVTUReader::Pointer reader = iGameVTUReader::New();
            reader->SetFilePath(resolvedFileName);
            readSucceeded = reader->Execute();
            if (readSucceeded) { object = reader->GetOutput(); }
        } else if (fileSuffix == "vtk") {
            VTKReader::Pointer reader = VTKReader::New();
            reader->SetFilePath(resolvedFileName);
            readSucceeded = reader->Execute();
            if (readSucceeded) { object = reader->GetOutput(); }
        }
#if defined(CGNS_ENABLE)
        else if (fileSuffix == "cgns") {
            iGameCGNSReader::Pointer reader = iGameCGNSReader::New();
            object = reader->ReadFile(resolvedFileName);
            readSucceeded = object != nullptr;
        }
#endif
        else {
            IGAME_CORE_ERROR("[VTM] Unsupported referenced file type '.{}': {}", fileSuffix, resolvedFileName);
            return nullptr;
        }

        if (!readSucceeded || !object) {
            IGAME_CORE_ERROR("[VTM] Failed to read referenced file: '{}' (resolved to '{}')",
                             fileName,
                             resolvedFileName);
            return nullptr;
        }

        object->SetName(referencedPath.stem().string());
        if (disableEagerPieceLod) {
            if (auto drawObject = DynamicCast<DrawObject>(object)) {
                // AddSubDataObject converts drawable leaves immediately. Set
                // this before insertion so loading hundreds of large pieces
                // does not synchronously simplify every piece on the reader
                // thread. Static rendering already uses complete geometry.
                drawObject->SetAutoBuildInteractionLod(false);
            }
        }
        if (auto unstructuredMesh = DynamicCast<UnstructuredMesh>(object)) {
            unstructuredPointRecords += unstructuredMesh->GetNumberOfPoints();
            unstructuredCellRecords += unstructuredMesh->GetNumberOfCells();
            ++unstructuredPieceCount;
        }
        ++successfulFileCount;
        return object;
    };

    // Handle <Block> format (nested structure)
    if (BlockElem) {
        while (BlockElem) {
            tinyxml2::XMLElement* elem = BlockElem->FirstChildElement();

            // 因为Scene中都用到了DrawObject的属性，后续多块要特殊处理
            // DataObject::Pointer curMultiBlock = DataObject::New();
            DataObject::Pointer curMultiBlock = DrawObject::New();

            while (elem) {
                // TODO: Not finish Reading the Nested block.
                //            std::string elemType(elem->Value());

                const char* existAttribute = elem->Attribute("file");
                if (existAttribute) {
                    DataObject::Pointer newObj = readReferencedFile(existAttribute);
                    if (!newObj) {
                        IGAME_CORE_ERROR("[VTM] Aborting after {}/{} referenced files were read successfully",
                                         successfulFileCount,
                                         totalFileCount);
                        return false;
                    }
                    curMultiBlock->AddSubDataObject(newObj);
                }
                elem = elem->NextSiblingElement();
            }

            overall_multiBlock.push_back(curMultiBlock);
            BlockElem = BlockElem->NextSiblingElement("Block");
        }
    }
    // Handle <DataSet> format (flat structure)
    else if (DataSetElem) {
        DataObject::Pointer curMultiBlock = DrawObject::New();

        while (DataSetElem) {
            const char* existAttribute = DataSetElem->Attribute("file");
            if (existAttribute) {
                DataObject::Pointer newObj = readReferencedFile(existAttribute);
                if (!newObj) {
                    IGAME_CORE_ERROR("[VTM] Aborting after {}/{} referenced files were read successfully",
                                     successfulFileCount,
                                     totalFileCount);
                    return false;
                }
                curMultiBlock->AddSubDataObject(newObj);
            }
            DataSetElem = DataSetElem->NextSiblingElement("DataSet");
        }

        overall_multiBlock.push_back(curMultiBlock);
    }

    IGAME_CORE_INFO("[VTM] Finished reading {}/{} referenced files successfully from {}",
                    successfulFileCount,
                    totalFileCount,
                    m_FilePath);
    if (unstructuredPieceCount > 0) {
        IGAME_CORE_INFO("[VTM] Unstructured summary: pieces={}, point records={}, cells={}",
                        unstructuredPieceCount,
                        unstructuredPointRecords,
                        unstructuredCellRecords);
    }

    if (successfulFileCount != totalFileCount || overall_multiBlock.empty()) {
        return false;
    } else if (overall_multiBlock.size() == 1) {
        parseData = overall_multiBlock.front();
        if (isFlatDataSet) {
            auto flatRoot = DynamicCast<DrawObject>(parseData);
            if (!PublishCommonFlatAttributes(flatRoot)) {
                IGAME_CORE_WARN("[VTM] Continuing without a common root attribute schema for {}", m_FilePath);
            }
        }
        return true;
    } else {
        DataObject::Pointer rootDataObject = DataObject::New();
        for (const auto& mp: overall_multiBlock) rootDataObject->AddSubDataObject(mp);

        parseData = rootDataObject;
        return true;
    }
}

bool iGameVTMReader::CreateDataObject() {
    if (!parseData) { return false; }
    m_Output = parseData;

    return m_Output != nullptr;
}


IGAME_NAMESPACE_END
