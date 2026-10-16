#include "iGameShrinkFilter.h"

#include "iGameCell.h"
#include "iGamePoints.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"

#include <algorithm>

IGAME_NAMESPACE_BEGIN

ShrinkFilter::ShrinkFilter() {
	SetNumberOfInputs(1);
	SetNumberOfOutputs(1);
}

void ShrinkFilter::SetShrinkFactor(double factor) {
	m_ShrinkFactor = std::clamp(factor, 0.0, 1.0);
}

double ShrinkFilter::GetShrinkFactor() const { return m_ShrinkFactor; }

bool ShrinkFilter::CopyPointAttributes(PointSet* pointSet, const std::vector<IGsize>& srcOfNew) {
	auto attrs = pointSet->GetAttributeSet();
	if (attrs == nullptr || srcOfNew.empty()) return true;

	IGsize nAttrs = attrs->GetNumberOfAttributes();
	for (IGsize i = 0; i < nAttrs; i++) {
		auto& a = attrs->GetAttribute(i);
		if (a.isDeleted || a.attachmentType != IG_POINT || a.pointer.IsNull()) continue;
		auto newArr = CloneArray(a.pointer, srcOfNew);
		if (newArr.IsNull()) continue;

		a.pointer = newArr;
		a.dataRange = nullptr;
		a.UpdateAllDataRange();
	}
	return true;
}

ArrayObject::Pointer ShrinkFilter::CloneArray(ArrayObject::Pointer src,
                                              const std::vector<IGsize>& srcOfNew) {
	ArrayObject::Pointer dst;
	switch (src->GetArrayType()) {
		case IG_CharArray: dst = CharArray::New(); break;
		case IG_UnsignedCharArray: dst = UnsignedCharArray::New(); break;
		case IG_ShortArray: dst = ShortArray::New(); break;
		case IG_UnsignedShortArray: dst = UnsignedShortArray::New(); break;
		case IG_IntArray: dst = IntArray::New(); break;
		case IG_UnsignedIntArray: dst = UnsignedIntArray::New(); break;
		case IG_LongLongArray: dst = LongLongArray::New(); break;
		case IG_UnsignedLongLongArray: dst = UnsignedLongLongArray::New(); break;
		case IG_FloatArray: dst = FloatArray::New(); break;
		case IG_DoubleArray: dst = DoubleArray::New(); break;
		default: return nullptr;
	}

	IGsize dim = src->GetDimension();
	if (dim < 1) dim = 1;
	dst->SetName(src->GetName());
	dst->SetDimension(static_cast<int>(dim));
	IGsize newCount = srcOfNew.size();

	dst->Resize(newCount);
    auto copyValues = [&]<class T>() {
        auto* source = static_cast<FlatArray<T>*>(src.get());
        auto* target = static_cast<FlatArray<T>*>(dst.get());
        for (IGsize i = 0; i < newCount; ++i) {
            const T* tuple = source->RawPointer(srcOfNew[i]);
            std::copy(tuple, tuple + dim, target->RawPointer(i));
        }
    };
    switch (src->GetArrayType()) {
        case IG_CharArray: copyValues.template operator()<char>(); break;
        case IG_UnsignedCharArray: copyValues.template operator()<unsigned char>(); break;
        case IG_ShortArray: copyValues.template operator()<short>(); break;
        case IG_UnsignedShortArray: copyValues.template operator()<unsigned short>(); break;
        case IG_IntArray: copyValues.template operator()<int>(); break;
        case IG_UnsignedIntArray: copyValues.template operator()<unsigned int>(); break;
        case IG_LongLongArray: copyValues.template operator()<long long>(); break;
        case IG_UnsignedLongLongArray: copyValues.template operator()<unsigned long long>(); break;
        case IG_FloatArray: copyValues.template operator()<float>(); break;
        case IG_DoubleArray: copyValues.template operator()<double>(); break;
        default: return nullptr;
    }
	return dst;
}

bool ShrinkFilter::Execute() {
    auto input = GetInput(0);
    if (input.IsNull()) return false;

    // AttributeSet::DeepCopy drops integer arrays and creates an empty range
    // when the source range has not been evaluated. Clone all numeric arrays,
    // letting each output attribute calculate its own range on demand.
    auto copyAttributes = [&]() {
        auto output = AttributeSet::New();
        auto* source = input->GetAttributeSet();
        if (!source) return output;
        for (IGsize i = 0; i < source->GetNumberOfAttributes(); ++i) {
            const auto& a = source->GetAttribute(i);
            if (a.isDeleted || !a.pointer) continue;
            std::vector<IGsize> indices(a.pointer->GetNumberOfElements());
            for (IGsize j = 0; j < indices.size(); ++j) indices[j] = j;
            auto array = CloneArray(a.pointer, indices);
            if (array) output->AddAttribute(a.type, a.attachmentType, array);
        }
        return output;
    };

    // 只在"输出的副本"上做修改，输入网格保持原样
    auto shrinkCells = [&](PointSet* mesh, IGsize count, CellArray* cells, auto getCellPointIds) -> bool {
        if (mesh == nullptr || cells == nullptr || count == 0) return false;

        auto oldPoints = mesh->GetPoints();
        if (oldPoints.IsNull()) return false;

        auto newPoints = Points::New();
        std::vector<IGsize> srcOfNew;

        igIndex ids[IGAME_CELL_MAX_SIZE];
        for (IGsize c = 0; c < count; c++) {
            int n = getCellPointIds(c, ids);
            if (n <= 0 || n > IGAME_CELL_MAX_SIZE) return false;

            double cx = 0.0, cy = 0.0, cz = 0.0;
            for (int k = 0; k < n; k++) {
                const auto& p = oldPoints->GetPoint(ids[k]);
                cx += p[0];
                cy += p[1];
                cz += p[2];
            }
            cx /= n;
            cy /= n;
            cz /= n;

            igIndex newIds[IGAME_CELL_MAX_SIZE];
            for (int k = 0; k < n; k++) {
                const auto& p = oldPoints->GetPoint(ids[k]);
                float nx = static_cast<float>(cx + (p[0] - cx) * m_ShrinkFactor);
                float ny = static_cast<float>(cy + (p[1] - cy) * m_ShrinkFactor);
                float nz = static_cast<float>(cz + (p[2] - cz) * m_ShrinkFactor);
                newIds[k] = static_cast<igIndex>(newPoints->AddPoint(nx, ny, nz));
                srcOfNew.push_back(ids[k]);
            }

            cells->SetCellIds(c, newIds, n);

            if ((c & 0x3FF) == 0) { UpdateProgress(static_cast<double>(c) / static_cast<double>(count)); }
        }

        mesh->SetPoints(newPoints);
        CopyPointAttributes(mesh, srcOfNew);
        return true;
    };

    // 体网格
    if (auto inMesh = DynamicCast<VolumeMesh>(input)) {
        auto out = VolumeMesh::New();
        auto points = Points::New();
        points->DeepCopy(inMesh->GetPoints());
        auto volumes = CellArray::New();
        volumes->DeepCopy(inMesh->GetVolumes());
        auto attrs = copyAttributes();
        out->SetPoints(points);
        out->SetVolumes(volumes);
        out->SetAttributeSet(attrs);

        if (!shrinkCells(out, out->GetNumberOfVolumes(), out->GetVolumes(),
                         [out](IGsize c, igIndex* ids) { return out->GetVolumePointIds(c, ids); })) {
            return false;
        }
        out->SetName(input->GetName() + "_shrink");
        UpdateProgress(1.0);
        SetOutput(0, out);
        return true;
    }
    // 表面网格
    if (auto inMesh = DynamicCast<SurfaceMesh>(input)) {
        auto out = SurfaceMesh::New();
        auto points = Points::New();
        points->DeepCopy(inMesh->GetPoints());
        auto faces = CellArray::New();
        faces->DeepCopy(inMesh->GetFaces());
        auto attrs = copyAttributes();
        out->SetPoints(points);
        out->SetFaces(faces);
        out->SetAttributeSet(attrs);

        if (!shrinkCells(out, out->GetNumberOfFaces(), out->GetFaces(),
                         [out](IGsize c, igIndex* ids) { return out->GetFacePointIds(c, ids); })) {
            return false;
        }
        out->SetName(input->GetName() + "_shrink");
        UpdateProgress(1.0);
        SetOutput(0, out);
        return true;
    }
    // 非结构化网格
    if (auto inMesh = DynamicCast<UnstructuredMesh>(input)) {
        auto out = UnstructuredMesh::New();
        auto points = Points::New();
        points->DeepCopy(inMesh->GetPoints());
        auto cells = CellArray::New();
        cells->DeepCopy(inMesh->GetCells());
        auto types = UnsignedIntArray::New();
        types->Resize(inMesh->GetNumberOfCells());
        auto inTypes = inMesh->GetCellTypes();
        for (IGsize i = 0; i < inMesh->GetNumberOfCells(); i++) { types->SetValue(i, inTypes->GetValue(i)); }
        auto attrs = copyAttributes();
        out->SetPoints(points);
        out->SetCells(cells, types);
        out->SetAttributeSet(attrs);

        if (!shrinkCells(out, out->GetNumberOfCells(), out->GetCellArray(),
                         [out](IGsize c, igIndex* ids) { return out->GetCellPointIds(c, ids); })) {
            return false;
        }
        out->SetName(input->GetName() + "_shrink");
        UpdateProgress(1.0);
        SetOutput(0, out);
        return true;
    }
    return false;
}

IGAME_NAMESPACE_END
