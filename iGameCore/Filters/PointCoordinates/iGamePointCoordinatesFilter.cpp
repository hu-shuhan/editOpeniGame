#include "iGamePointCoordinatesFilter.h"

#include "iGameAttributeSet.h"
#include "iGamePoints.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

namespace {

DataObject::Pointer DeepCopyMesh(DataObject::Pointer input) {
    if (!input) return nullptr;

    switch (input->GetDataObjectType()) {
        case IG_UNSTRUCTURED_MESH: {
            auto inMesh = DynamicCast<UnstructuredMesh>(input);
            auto outMesh = UnstructuredMesh::New();

            // Deep copy points
            auto inPoints = inMesh->GetPoints();
            if (inPoints) {
                auto outPoints = Points::New();
                outPoints->DeepCopy(inPoints);
                outMesh->SetPoints(outPoints);
            }

            // Deep copy cells and types
            auto inCells = inMesh->GetCells();
            auto inTypes = inMesh->GetCellTypes();
            if (inCells && inTypes) {
                auto outCells = CellArray::New();
                outCells->DeepCopy(inCells);
                auto outTypes = UnsignedIntArray::New();
                outTypes->DeepCopy(inTypes);
                outMesh->SetCells(outCells, outTypes);
            }

            // Deep copy attributes
            auto inAttrs = inMesh->GetAttributeSet();
            if (inAttrs && inAttrs->GetNumberOfAttributes() > 0) {
                auto outAttrs = AttributeSet::New();
                outAttrs->DeepCopy(inAttrs);
                outMesh->SetAttributeSet(outAttrs);
            }

            return outMesh;
        }
        default: {
            // Generic fallback: copy points and attributes only
            auto outMesh = UnstructuredMesh::New();

            auto inPoints = input->GetPoints();
            if (inPoints) {
                auto outPoints = Points::New();
                outPoints->DeepCopy(inPoints);
                outMesh->SetPoints(outPoints);
            }

            auto inAttrs = input->GetAttributeSet();
            if (inAttrs && inAttrs->GetNumberOfAttributes() > 0) {
                auto outAttrs = AttributeSet::New();
                outAttrs->DeepCopy(inAttrs);
                outMesh->SetAttributeSet(outAttrs);
            }

            return outMesh;
        }
    }
}

} // namespace

PointCoordinatesFilter::PointCoordinatesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void PointCoordinatesFilter::SetArrayName(const std::string& name) {
    if (!name.empty()) { m_ArrayName = name; }
}

bool PointCoordinatesFilter::Execute() {
    m_CoordinatesArray = nullptr;
    SetOutput(nullptr);

    auto input = GetInput(0);
    if (!input) {
        igDebug("PointCoordinatesFilter requires a non-null input.");
        return false;
    }

    auto points = input->GetPoints();
    if (!points) {
        igDebug("PointCoordinatesFilter requires input data with points.");
        return false;
    }

    auto coordinates = points->ConvertToArray();
    if (!coordinates || coordinates->GetDimension() != 3) {
        igDebug("PointCoordinatesFilter requires three-component point coordinates.");
        return false;
    }

    // Deep copy the input to create an independent output node
    auto output = DeepCopyMesh(input);
    if (!output) {
        igDebug("PointCoordinatesFilter failed to deep copy the input.");
        return false;
    }
    output->SetName(input->GetName() + "_Coordinates");

    auto outPoints = output->GetPoints();
    auto outCoordinates = outPoints->ConvertToArray();
    if (!outCoordinates || outCoordinates->GetDimension() != 3) {
        igDebug("PointCoordinatesFilter: output point coordinates are invalid.");
        return false;
    }

    auto outAttributes = output->GetAttributeSet();
    if (!outAttributes) {
        igDebug("PointCoordinatesFilter could not access the output attribute set.");
        return false;
    }

    // Check for name collision on the output copy
    const int existingIndex = outAttributes->GetAttributeIndex(m_ArrayName);
    if (existingIndex >= 0) {
        igDebug("PointCoordinatesFilter cannot create array '{}': the name is already in use on the output.", m_ArrayName);
        return false;
    }

    outCoordinates->SetName(m_ArrayName);
    outCoordinates->Modified();
    const auto coordinatesIndex = outAttributes->AddAttribute(IG_VECTOR, IG_POINT, outCoordinates);
    if (coordinatesIndex < 0) {
        igDebug("PointCoordinatesFilter failed to add array '{}'.", m_ArrayName);
        return false;
    }

    outAttributes->GetAttribute(coordinatesIndex).UpdateAllDataRange();
    output->Modified();
    m_CoordinatesArray = outCoordinates;
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
