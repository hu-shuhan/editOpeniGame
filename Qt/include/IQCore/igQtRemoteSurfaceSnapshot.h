#pragma once

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameDrawObject.h"
#include "iGameModel.h"
#include "iGameScene.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include <QString>
#include <QtGlobal>
#include <functional>
#include <limits>
#include <unordered_set>

// Lightweight metadata guard for caching the validated, static triangle
// surface data. It does not certify renderer mode or a completed GPU frame.
// It never reads coordinate/scalar/connectivity values,
// constructs renderable meshes, computes ranges, modifies state, or calls GL.
// Counts are point records and source face/cell records, not deduplicated points.
// Uniform cell types and variable-offset triangle arity remain the responsibility
// of the package's offline topology validation; this is not a replacement for it.
struct igQtRemoteSurfaceSnapshot {
    bool valid{false};
    quint64 leafCount{0};
    quint64 pointCount{0};
    quint64 faceCount{0};
    quint64 variableArityLeafCount{0};
    QString detail;

    static igQtRemoteSurfaceSnapshot Inspect(iGame::Scene* scene, iGame::Model* model) {
        igQtRemoteSurfaceSnapshot result;
        QString failure;
        auto fail = [&failure](const QString& message) { failure = message; return false; };
        if (!scene || !model || model->GetScene().get() != scene || !model->GetVisibility()) {
            result.detail = QStringLiteral("Surface guard: no visible model in the expected scene");
            return result;
        }
        std::unordered_set<const iGame::DataObject*> visited;
        std::function<bool(iGame::DataObject*, const QString&, unsigned)> visit;
        visit = [&](iGame::DataObject* object, const QString& path, unsigned depth) {
            if (!object || depth > 256 || !visited.insert(object).second) {
                return fail(path + QStringLiteral(": null, repeated or cyclic object hierarchy"));
            }
            auto* draw = dynamic_cast<iGame::DrawObject*>(object);
            if (!draw || !draw->GetVisibility()) {
                return fail(path + QStringLiteral(": object is not visible/drawable"));
            }
            if (object->PeekTimeFrames() && object->PeekTimeFrames()->GetTimeNum() != 0) {
                return fail(path + QStringLiteral(": time-dependent data is not supported by this benchmark"));
            }
            if (draw->GetTransparency() != 1.0f || draw->GetOpacityMappingEnabled() ||
                draw->GetAccelerationOption()) {
                return fail(path + QStringLiteral(": transparency, opacity mapping or accelerated rendering is enabled"));
            }
            auto clipper = draw->GetClipper();
            if (clipper && !clipper->IsAllDisable()) {
                return fail(path + QStringLiteral(": clipping is enabled"));
            }
            if (object->HasSubDataObject()) {
                for (auto it = object->SubDataObjectIteratorBegin();
                     it != object->SubDataObjectIteratorEnd(); ++it) {
                    if (!visit(it->second.get(), path + QStringLiteral("/%1")
                                   .arg(static_cast<qulonglong>(it->first)), depth + 1)) { return false; }
                }
                return true;
            }

            if (draw->GetViewStyle() != IG_SURFACE) {
                return fail(path + QStringLiteral(": leaf must use Surface only (not Outline/Points/Wireframe)"));
            }
            const auto type = object->GetDataObjectType();
            if (type != IG_SURFACE_MESH && type != IG_UNSTRUCTURED_MESH) {
                return fail(path + QStringLiteral(": leaf is not a supported validated surface dataset"));
            }
            auto points = object->GetPoints();
            auto cells = object->GetCellArray();
            if (!points || !cells) { return fail(path + QStringLiteral(": missing points or face/cell metadata")); }
            const quint64 pointCount = static_cast<quint64>(points->GetNumberOfPoints());
            const quint64 cellCount = static_cast<quint64>(cells->GetNumberOfCells());
            auto ids = cells->GetCellIdArray();
            if (!pointCount || !cellCount || !ids ||
                cellCount > std::numeric_limits<IGuint>::max() / quint64{3}) {
                // GetNumberOfCellIds currently obtains an IGuint end offset.
                // Reject an unrepresentable logical length before calling it.
                return fail(path + QStringLiteral(": empty data or logical connectivity length exceeds CellArray offset metadata"));
            }
            const quint64 expectedIds = cellCount * 3;
            const quint64 backingIds = static_cast<quint64>(ids->GetNumberOfIds());
            if (!cells->IsUseOffSet()) {
                // In fixed-size mode GetCellSize reads fixed metadata only.
                if (cells->GetCellSize(0) != 3) {
                    return fail(path + QStringLiteral(": fixed cell arity is not three"));
                }
            } else {
                auto offsets = cells->GetOffset();
                if (!offsets || offsets->GetDimension() != 1 ||
                    static_cast<quint64>(offsets->GetNumberOfValues()) < cellCount + 1) {
                    return fail(path + QStringLiteral(": missing or truncated offset metadata"));
                }
                // Only constant-size metadata reads: first/last boundaries.
                // Interior monotonicity and arity still rely on offline checks.
                if (offsets->GetValue(0) != 0 || offsets->GetValue(1) != 3 ||
                    static_cast<quint64>(offsets->GetValue(cellCount - 1)) != expectedIds - 3 ||
                    static_cast<quint64>(offsets->GetValue(cellCount)) != expectedIds) {
                    return fail(path + QStringLiteral(": invalid first/last triangle offset boundaries"));
                }
                ++result.variableArityLeafCount;
            }
            // AddCellIds geometrically resizes its backing array, so its size
            // can exceed the active connectivity length even for valid meshes.
            const quint64 logicalIds = static_cast<quint64>(cells->GetNumberOfCellIds());
            if (logicalIds != expectedIds || logicalIds > backingIds) {
                return fail(path + QStringLiteral(": logical connectivity must equal 3 x face/cell count and fit its backing array"));
            }
            if (auto* mesh = dynamic_cast<iGame::UnstructuredMesh*>(object)) {
                auto* types = mesh->GetCellTypes();
                if (!types || static_cast<quint64>(types->GetNumberOfValues()) != cellCount) {
                    return fail(path + QStringLiteral(": cell-type metadata length mismatch"));
                }
            }

            // Cache eligibility follows the physical dataset, not the current
            // scalar selection. Clicking the model row selects solid coloring;
            // preserve that state on reattachment instead of evicting the cache.
            // Prepared-cache lookup separately validates the display signature.
            auto* attributes = object->GetAttributeSet();
            const int attributeIndex = attributes ? attributes->GetAttributeIndex("PressureCoefficient") : -1;
            if (attributeIndex < 0) {
                return fail(path + QStringLiteral(": missing point PressureCoefficient"));
            }
            const auto& attribute = attributes->GetAttribute(attributeIndex);
            const auto scalar = attribute.pointer;
            if (attribute.isDeleted || attribute.type != IG_SCALAR || attribute.attachmentType != IG_POINT ||
                !scalar || scalar->GetName() != "PressureCoefficient" || scalar->GetDimension() != 1 ||
                scalar->GetArrayType() != IG_FloatArray ||
                static_cast<quint64>(scalar->GetNumberOfValues()) != pointCount) {
                return fail(path + QStringLiteral(": PressureCoefficient must be one Float32 scalar per point"));
            }
            if (pointCount > std::numeric_limits<quint64>::max() - result.pointCount ||
                cellCount > std::numeric_limits<quint64>::max() - result.faceCount) {
                return fail(path + QStringLiteral(": aggregate count overflow"));
            }
            ++result.leafCount;
            result.pointCount += pointCount;
            result.faceCount += cellCount;
            return true;
        };

        result.valid = visit(model->GetDataObject().get(), QStringLiteral("root"), 0);
        if (result.valid && result.leafCount == 0) {
            result.valid = false;
            failure = QStringLiteral("no surface leaves");
        }
        result.detail = QStringLiteral("Surface guard %1: leaves=%2 point_records=%3 face_cells=%4; %5")
                                .arg(result.valid ? QStringLiteral("PASS") : QStringLiteral("FAIL"))
                                .arg(result.leafCount).arg(result.pointCount).arg(result.faceCount)
                                .arg(result.valid
                                             ? QStringLiteral("visible, opaque Surface metadata with point Float32 PressureCoefficient; active scalar coloring is not required; renderer mode/GPU output not checked; topology types/variable arity rely on prior offline validation")
                                             : failure);
        return result;
    }
};
