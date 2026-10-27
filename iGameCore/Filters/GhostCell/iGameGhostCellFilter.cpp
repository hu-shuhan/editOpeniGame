#include "iGameGhostCellFilter.h"
#include "Selection/iGameExtractBlockFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameAttributeSet.h"
#include <array>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

IGAME_NAMESPACE_BEGIN
namespace {
constexpr unsigned char Duplicate = 1, HiddenPoint = 2, HiddenCell = 32;
using Tuple = std::pair<size_t, IGsize>;
struct Partition {
    UnstructuredMesh::Pointer mesh;
    std::vector<unsigned char> pointFlags, cellFlags;
    std::vector<IGsize> cells;
    std::vector<bool> active;
    std::vector<size_t> points;
};
struct GlobalPoint { Point position; Tuple source; };
struct GlobalCell { Tuple source; IGenum type; std::vector<size_t> points; };
struct Field {
    std::string name;
    IGenum type, attachment;
    std::vector<ArrayObject::Pointer> arrays;
};
void Require(bool ok, const std::string& error) { if (!ok) throw std::runtime_error(error); }
ArrayObject::Pointer FindArray(DataObject::Pointer mesh, IGenum attachment, const std::string& name) {
    auto attrs = mesh->GetAttributeSet(); ArrayObject::Pointer result;
    for (size_t i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        const auto& a = attrs->GetAttribute(i);
        if (a.isDeleted || !a.pointer || a.attachmentType != attachment || a.pointer->GetName() != name) continue;
        Require(!result, "Duplicate attribute: " + name); result = a.pointer;
    }
    return result;
}
std::vector<unsigned char> ReadFlags(DataObject::Pointer mesh, IGenum attachment, IGsize count) {
    auto a = FindArray(mesh, attachment, "vtkGhostType");
    std::vector<unsigned char> flags(count, 0);
    if (!a) return flags;
    Require(a->GetDimension() == 1 && a->GetNumberOfElements() == count, "Invalid vtkGhostType shape.");
    for (IGsize i = 0; i < count; ++i) {
        const double v = a->GetValue(i);
        Require(std::isfinite(v) && v >= 0 && v <= 255 && std::floor(v) == v, "vtkGhostType must contain byte flags.");
        flags[i] = static_cast<unsigned char>(v);
    }
    return flags;
}
void Collect(DataObject::Pointer input, std::vector<Partition>& parts, std::set<DataObject*>& active) {
    Require(input && active.insert(input.get()).second, "Null or cyclic partition tree.");
    auto children = ExtractBlockFilter::GetBlocks(input);
    if (!children.empty()) {
        Require(!DynamicCast<PointSet>(input), "A mesh must not also contain child partitions.");
        for (auto child : children) if (child) Collect(child, parts, active);
    } else if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
        Partition p; p.mesh = mesh;
        const auto np = mesh->GetNumberOfPoints(), nc = mesh->GetNumberOfCells();
        p.pointFlags = ReadFlags(mesh, IG_POINT, np); p.cellFlags = ReadFlags(mesh, IG_CELL, nc);
        p.active.resize(np, false); p.points.resize(np);
        for (IGsize c = 0; c < nc; ++c) {
            const auto type = mesh->GetCellType(c);
            const igIndex* ids; const int n = mesh->GetCellPointIds(c, ids);
            Require((type == IG_TETRA && n == 4) || (type == IG_HEXAHEDRON && n == 8),
                    "Only linear tetrahedra and hexahedra are supported.");
            bool hidden = false; std::set<igIndex> unique;
            for (int j = 0; j < n; ++j) {
                Require(ids[j] >= 0 && static_cast<IGsize>(ids[j]) < np, "Invalid cell point ID.");
                Require(unique.insert(ids[j]).second, "Repeated point in a cell.");
                hidden |= (p.pointFlags[ids[j]] & HiddenPoint) != 0;
            }
            if (hidden || (p.cellFlags[c] & (Duplicate | HiddenCell))) continue;
            p.cells.push_back(c);
            for (int j = 0; j < n; ++j) p.active[ids[j]] = true;
        }
        for (IGsize i = 0; i < np; ++i) if (!(p.pointFlags[i] & (Duplicate | HiddenPoint))) p.active[i] = true;
        parts.push_back(std::move(p));
    } else Require(input->GetDataObjectType() == IG_DRAW_OBJECT || input->GetDataObjectType() == IG_DATA_OBJECT ||
                   input->GetDataObjectType() == IG_COMPOSITE_DATA_OBJECT, "Unsupported partition leaf; use UnstructuredMesh.");
    active.erase(input.get());
}
// Preserve integer IDs above 2^53: never round-trip IDs or field tuples through double.
std::string IntegerId(ArrayObject::Pointer array, IGsize index) {
#define ID(Type) if (auto a = DynamicCast<Type>(array)) return std::to_string(a->RawPointer()[index])
    ID(IntArray); ID(UnsignedIntArray); ID(LongLongArray); ID(UnsignedLongLongArray); ID(ShortArray); ID(UnsignedShortArray);
#undef ID
    throw std::runtime_error("Global point IDs must be scalar integer arrays.");
}
std::vector<Field> ReadFields(const std::vector<Partition>& parts) {
    std::map<std::pair<IGenum,std::string>,Field> fields;
    for (size_t p = 0; p < parts.size(); ++p) {
        auto mesh = parts[p].mesh; auto attrs = mesh->GetAttributeSet();
        for (size_t i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
            const auto& a = attrs->GetAttribute(i);
            if (a.isDeleted || !a.pointer || a.pointer->GetName() == "vtkGhostType") continue;
            Require(a.attachmentType == IG_POINT || a.attachmentType == IG_CELL, "Only point and cell fields are supported.");
            auto count = a.attachmentType == IG_POINT ? mesh->GetNumberOfPoints() : mesh->GetNumberOfCells();
            Require(a.pointer->GetDimension() > 0 && a.pointer->GetNumberOfElements() == count, "Invalid attribute: " + a.pointer->GetName());
            auto key = std::make_pair(a.attachmentType,a.pointer->GetName()); auto& f = fields[key];
            if (f.arrays.empty()) { f.name = key.second; f.type = a.type; f.attachment = a.attachmentType; f.arrays.resize(parts.size()); }
            Require(!f.arrays[p], "Duplicate attribute: " + f.name); f.arrays[p] = a.pointer;
        }
    }
    std::vector<Field> result;
    for (auto& item : fields) {
        auto& f = item.second;
        for (auto a : f.arrays)
            Require(a && f.arrays[0] && a->GetArrayType() == f.arrays[0]->GetArrayType() &&
                    a->GetDimension() == f.arrays[0]->GetDimension(), "Partition attribute schemas differ: " + f.name);
        result.push_back(std::move(f));
    }
    return result;
}
template<class Array> ArrayObject::Pointer CopyField(const Field& field, const std::vector<Tuple>& tuples) {
    if (!DynamicCast<Array>(field.arrays[0])) return nullptr;
    std::vector<SmartPointer<Array>> sources;
    for (auto a : field.arrays) sources.push_back(DynamicCast<Array>(a));
    auto result = Array::New(); result->SetName(field.name);
    int dim = field.arrays[0]->GetDimension(); result->SetDimension(dim); result->Reserve(tuples.size());
    for (const auto& t : tuples) {
        const auto* data = sources[t.first]->RawPointer() + t.second * dim;
        for (int component = 0; component < dim; ++component) result->AddValue(data[component]);
    }
    return result;
}
ArrayObject::Pointer CopyField(const Field& f, const std::vector<Tuple>& tuples) {
#define COPY(Type) if (auto result = CopyField<Type>(f, tuples)) return result
    COPY(FloatArray); COPY(DoubleArray); COPY(IntArray); COPY(UnsignedIntArray); COPY(CharArray);
    COPY(UnsignedCharArray); COPY(ShortArray); COPY(UnsignedShortArray); COPY(LongLongArray); COPY(UnsignedLongLongArray); COPY(FlatArray<igIndex>);
#undef COPY
    throw std::runtime_error("Unsupported attribute storage: " + f.name);
}
DataObject::Pointer Rebuild(DataObject::Pointer input, const std::vector<UnstructuredMesh::Pointer>& outputs, size_t& index) {
    if (DynamicCast<UnstructuredMesh>(input)) return outputs.at(index++);
    auto output = DrawObject::New(); output->SetName(input->GetName());
    for (auto child : ExtractBlockFilter::GetBlocks(input)) {
        // Keep an empty slot's position, even though drawable containers use
        // concrete empty nodes rather than null child pointers.
        if (child) output->AddSubDataObject(Rebuild(child,outputs,index));
        else output->AddSubDataObject(DrawObject::New());
    }
    return output;
}
} // namespace

GhostCellFilter::GhostCellFilter() { SetNumberOfInputs(1); SetNumberOfOutputs(1); }
bool GhostCellFilter::Execute() {
    SetOutput(0,nullptr); m_Error.clear(); UpdateProgress(0);
    try {
        Require(m_Layers >= 0, "Ghost layer count must be nonnegative.");
        std::vector<Partition> parts; std::set<DataObject*> active; Collect(GetInput(0),parts,active);
        Require(!parts.empty(), "Input contains no unstructured partitions.");
        const auto fields = ReadFields(parts);
        std::vector<ArrayObject::Pointer> globalIds; size_t idArrays = 0;
        for (const auto& p : parts) {
            ArrayObject::Pointer ids;
            if (!m_GlobalIds.empty()) ids = FindArray(p.mesh,IG_POINT,m_GlobalIds);
            if (ids) {
                Require(ids->GetDimension() == 1 && ids->GetNumberOfElements() == p.mesh->GetNumberOfPoints(), "Invalid global point IDs.");
                ++idArrays;
            }
            globalIds.push_back(ids);
        }
        Require(idArrays == 0 || idArrays == parts.size(), "Global point IDs must be present on all partitions or none.");
        std::vector<GlobalPoint> points;
        std::map<std::array<float,3>,size_t> coordinates; std::map<std::string,size_t> identifiers;
        for (size_t part = 0; part < parts.size(); ++part) {
            auto& p = parts[part]; std::set<size_t> local;
            for (IGsize i = 0; i < p.mesh->GetNumberOfPoints(); ++i) {
                if (!p.active[i]) continue;
                Point position = p.mesh->GetPoint(i);
                Require(std::isfinite(position[0]) && std::isfinite(position[1]) && std::isfinite(position[2]), "Non-finite point coordinates.");
                std::array<float,3> key{position[0],position[1],position[2]}; size_t g;
                if (idArrays) g = identifiers.emplace(IntegerId(globalIds[part],i),points.size()).first->second;
                else g = coordinates.emplace(key,points.size()).first->second;
                if (g == points.size()) points.push_back({position,{part,i}});
                else for (int a = 0; a < 3; ++a) Require(points[g].position[a] == position[a], "One global point ID has inconsistent coordinates.");
                Require(local.insert(g).second, "Duplicate point keys inside a partition; use unique GlobalPointIds for coincident points.");
                p.points[i] = g;
            }
        }
        std::vector<GlobalCell> cells;
        std::vector<std::vector<size_t>> adjacency(points.size()), native(parts.size());
        std::set<std::pair<IGenum,std::vector<size_t>>> owned;
        for (size_t part = 0; part < parts.size(); ++part) {
            const auto& p = parts[part];
            for (auto c : p.cells) {
                GlobalCell cell; cell.source = {part,c}; cell.type = p.mesh->GetCellType(c);
                const igIndex* ids; int n = p.mesh->GetCellPointIds(c,ids);
                for (int j = 0; j < n; ++j) cell.points.push_back(p.points[ids[j]]);
                auto sorted = cell.points; std::sort(sorted.begin(),sorted.end());
                Require(owned.insert({cell.type,sorted}).second, "Overlapping owned cells; existing duplicate cells need vtkGhostType DUPLICATECELL.");
                size_t id = cells.size();
                for (auto point : cell.points) adjacency[point].push_back(id);
                native[part].push_back(id); cells.push_back(std::move(cell));
            }
        }
        UpdateProgress(0.25); std::vector<UnstructuredMesh::Pointer> outputs;
        for (size_t part = 0; part < parts.size(); ++part) {
            const auto& p = parts[part]; std::vector<size_t> selected = native[part], frontier;
            std::unordered_set<size_t> seenCells(selected.begin(),selected.end()), seenPoints;
            for (auto c : selected) for (auto point : cells[c].points) if (seenPoints.insert(point).second) frontier.push_back(point);
            // Like VTK, exchange with original neighboring partitions only.
            // More layers extend into those neighbors, not through a chain of
            // previously unrelated partitions whose ghosts happen to touch.
            std::set<size_t> neighbors{part};
            for (auto point : frontier) for (auto c : adjacency[point]) neighbors.insert(cells[c].source.first);
            for (int layer = 0; layer < m_Layers && !frontier.empty(); ++layer) {
                std::vector<size_t> added;
                for (auto point : frontier) for (auto c : adjacency[point])
                    if (neighbors.count(cells[c].source.first) && seenCells.insert(c).second) added.push_back(c);
                std::sort(added.begin(),added.end()); selected.insert(selected.end(),added.begin(),added.end()); frontier.clear();
                for (auto c : added) for (auto point : cells[c].points) if (seenPoints.insert(point).second) frontier.push_back(point);
            }
            auto output = UnstructuredMesh::New(); output->SetName(p.mesh->GetName());
            auto pf = UnsignedCharArray::New(); pf->SetName("vtkGhostType");
            auto cf = UnsignedCharArray::New(); cf->SetName("vtkGhostType");
            std::map<size_t,igIndex> local; std::vector<Tuple> pointTuples,cellTuples;
            auto addPoint = [&](size_t g) {
                auto insertion = local.emplace(g,static_cast<igIndex>(pointTuples.size()));
                if (insertion.second) {
                    auto source = points[g].source;
                    output->GetPoints()->AddPoint(points[g].position); pointTuples.push_back(source);
                    auto flags = parts[source.first].pointFlags[source.second] & ~(Duplicate | HiddenPoint);
                    pf->AddValue(static_cast<unsigned char>(flags | (source.first != part ? Duplicate : 0)));
                }
                return insertion.first->second;
            };
            for (IGsize i = 0; i < p.mesh->GetNumberOfPoints(); ++i) if (p.active[i]) addPoint(p.points[i]);
            for (auto c : selected) {
                std::vector<igIndex> ids; for (auto point : cells[c].points) ids.push_back(addPoint(point));
                output->AddCell(ids.data(),static_cast<int>(ids.size()),cells[c].type);
                auto source = cells[c].source; cellTuples.push_back(source);
                cf->AddValue(static_cast<unsigned char>(parts[source.first].cellFlags[source.second] | (source.first != part ? Duplicate : 0)));
            }
            for (const auto& f : fields) output->GetAttributeSet()->AddAttribute(f.type,f.attachment,CopyField(f,f.attachment == IG_POINT ? pointTuples : cellTuples));
            output->GetAttributeSet()->AddAttribute(IG_SCALAR,IG_POINT,pf);
            if (m_Layers > 0) output->GetAttributeSet()->AddAttribute(IG_SCALAR,IG_CELL,cf);
            outputs.push_back(output); UpdateProgress(0.25 + 0.75*(part+1.0)/parts.size());
        }
        size_t index = 0; auto result = Rebuild(GetInput(0),outputs,index); SetOutput(0,result); return true;
    } catch (const std::exception& e) { m_Error = e.what(); SetOutput(0,nullptr); return false; }
}
IGAME_NAMESPACE_END
