#include "iGameMeshAttributeTransfer.h"
#include <igl/point_mesh_squared_distance.h>
#include <igl/barycentric_coordinates.h>
#include <algorithm>
#include <exception>
#include <limits>

IGAME_NAMESPACE_BEGIN
bool TransferMeshAttributes(SurfaceMesh* source, SurfaceMesh* destination, std::string& error) {
    error.clear();
    auto fail = [&](const std::string& message) { error = message; return false; };
    if (!source || !destination) return fail("Missing attribute transfer mesh.");
    auto attributes = source->GetAttributeSet();
    auto result = AttributeSet::New();
    if (!attributes || attributes->GetNumberOfAttributes() == 0) {
        destination->SetAttributeSet(result);
        return true;
    }
    const auto nv = source->GetNumberOfPoints();
    const auto nf = source->GetNumberOfFaces();
    if (!nv || !nf || nv > std::numeric_limits<int>::max())
        return fail("Invalid source mesh for attribute transfer.");
    try {
        Eigen::MatrixXd vertices(nv, 3);
        for (IGsize i = 0; i < nv; ++i) {
            auto p = source->GetPoint(i);
            for (int d = 0; d < 3; ++d) vertices(i, d) = p[d];
        }
        if (!vertices.allFinite()) return fail("Non-finite source coordinates.");
        std::vector<Eigen::Vector3i> triangles;
        std::vector<igIndex> sourceFaces;
        for (IGsize f = 0; f < nf; ++f) {
            if (source->GetFaces()->IsDeleted(f)) continue;
            const igIndex* ids = nullptr;
            const int n = source->GetFaces()->GetCellIds(f, ids);
            for (int k = 1; k + 1 < n; ++k) {
                if (ids[0] < 0 || ids[k] < 0 || ids[k+1] < 0 ||
                    ids[0] >= nv || ids[k] >= nv || ids[k+1] >= nv)
                    return fail("Invalid source triangle indices.");
                Eigen::Vector3d ab = vertices.row(ids[k]) - vertices.row(ids[0]);
                Eigen::Vector3d ac = vertices.row(ids[k+1]) - vertices.row(ids[0]);
                if (ab.cross(ac).squaredNorm() == 0.0) continue;
                triangles.emplace_back(ids[0], ids[k], ids[k+1]);
                sourceFaces.push_back(f);
            }
        }
        if (triangles.empty()) return fail("No non-degenerate source triangles.");
        Eigen::MatrixXi faces(triangles.size(), 3);
        for (size_t i = 0; i < triangles.size(); ++i) faces.row(i) = triangles[i];
        const auto np = destination->GetNumberOfPoints();
        const auto nc = destination->GetNumberOfFaces();
        Eigen::MatrixXd queries(np + nc, 3);
        for (IGsize i = 0; i < np; ++i) {
            auto p = destination->GetPoint(i);
            for (int d = 0; d < 3; ++d) queries(i, d) = p[d];
        }
        for (IGsize f = 0; f < nc; ++f) {
            const igIndex* ids = nullptr;
            int n = destination->GetFaces()->GetCellIds(f, ids);
            if (n < 3) return fail("Invalid destination face.");
            queries.row(np + f).setZero();
            for (int k = 0; k < n; ++k) {
                if (ids[k] < 0 || ids[k] >= np) return fail("Invalid destination vertex index.");
                queries.row(np + f) += queries.row(ids[k]) / double(n);
            }
        }
        if (!queries.allFinite()) return fail("Non-finite destination coordinates.");
        Eigen::VectorXd distances;
        Eigen::VectorXi nearest;
        Eigen::MatrixXd closest;
        // libigl builds an AABB tree, searching the entire source surface.
        igl::point_mesh_squared_distance(queries, vertices, faces, distances, nearest, closest);
        Eigen::MatrixXd a(np, 3), b(np, 3), c(np, 3), weights;
        for (IGsize i = 0; i < np; ++i) {
            a.row(i) = vertices.row(faces(nearest(i), 0));
            b.row(i) = vertices.row(faces(nearest(i), 1));
            c.row(i) = vertices.row(faces(nearest(i), 2));
        }
        Eigen::MatrixXd projected = closest.topRows(np);
        igl::barycentric_coordinates(projected, a, b, c, weights);
        if (!weights.allFinite()) return fail("Invalid barycentric coordinates.");
        for (IGsize i = 0; i < np; ++i) {
            weights.row(i) = weights.row(i).cwiseMax(0.0).cwiseMin(1.0);
            weights.row(i) /= weights.row(i).sum();
        }
        auto buffer = ElementArray<AttributeSet::Attribute>::New();
        for (IGsize index = 0; index < attributes->GetNumberOfAttributes(); ++index) {
            const auto& attr = attributes->GetAttribute(index);
            AttributeSet::Attribute output = attr;
            if (!attr.isDeleted && attr.pointer &&
                (attr.attachmentType == IG_POINT || attr.attachmentType == IG_CELL)) {
                const bool pointData = attr.attachmentType == IG_POINT;
                const int dimension = attr.pointer->GetDimension();
                if (dimension <= 0 || attr.pointer->GetNumberOfElements() != (pointData ? nv : nf))
                    return fail("Attribute tuple count does not match source mesh: " + attr.pointer->GetName());
                // Floating point output preserves fractional values even for integer input.
                auto values = DoubleArray::New();
                values->SetName(attr.pointer->GetName());
                values->SetDimension(dimension);
                values->Resize(pointData ? np : nc);
                for (IGsize i = 0; i < (pointData ? np : nc); ++i) {
                    for (int d = 0; d < dimension; ++d) {
                        double value = 0.0;
                        if (pointData) {
                            for (int k = 0; k < 3; ++k)
                                value += weights(i, k) * attr.pointer->GetValue(IGsize(faces(nearest(i), k)) * dimension + d);
                        } else {
                            value = attr.pointer->GetValue(IGsize(sourceFaces[nearest(np + i)]) * dimension + d);
                        }
                        values->SetValue(i * dimension + d, value);
                    }
                }
                output.pointer = values;
                output.dataRange = nullptr;
                output.UpdateAllDataRange();
            }
            buffer->AddElement(output);
        }
        result->SetAllAttributes(buffer);
        destination->SetAttributeSet(result);
        return true;
    } catch (const std::exception& e) {
        return fail(std::string("Attribute transfer failed: ") + e.what());
    }
}
IGAME_NAMESPACE_END
