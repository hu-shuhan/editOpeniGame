#include "iGameThresholdFilter.h"

#include <iGameFlatArray.h>

#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN
namespace {

ArrayObject::Pointer CreateArrayLike(const ArrayObject::Pointer& source) {
	if (!source) return nullptr;

	ArrayObject::Pointer result;
	switch (source->GetArrayType()) {
		case IG_FloatArray: result = FloatArray::New(); break;
		case IG_DoubleArray: result = DoubleArray::New(); break;
		case IG_IntArray:
		case IG_INTARRAY: result = IntArray::New(); break;
		case IG_UnsignedIntArray: result = UnsignedIntArray::New(); break;
		case IG_CharArray: result = CharArray::New(); break;
		case IG_UnsignedCharArray: result = UnsignedCharArray::New(); break;
		case IG_ShortArray: result = ShortArray::New(); break;
		case IG_UnsignedShortArray: result = UnsignedShortArray::New(); break;
		case IG_LongLongArray: result = LongLongArray::New(); break;
		case IG_UnsignedLongLongArray: result = UnsignedLongLongArray::New(); break;
		default: return nullptr;
	}

	result->SetName(source->GetName());
	result->SetDimension(source->GetDimension());
	return result;
}

template <typename TValue>
bool CopyElementsByType(const ArrayObject::Pointer& source,
						const std::vector<igIndex>& indices,
						ArrayObject::Pointer& result) {
	auto src = DynamicCast<FlatArray<TValue>>(source);
	auto dst = DynamicCast<FlatArray<TValue>>(result);
	if (!src || !dst) return false;

	const int dimension = source->GetDimension();
	dst->Resize(indices.size());
	for (igIndex i = 0; i < static_cast<igIndex>(indices.size()); ++i) {
		const TValue* srcElement = src->RawPointer(indices[i]);
		TValue* dstElement = dst->RawPointer(i);
		for (int c = 0; c < dimension; ++c) {
			dstElement[c] = srcElement[c];
		}
	}
	return true;
}

bool CopyArrayElements(const ArrayObject::Pointer& source, const std::vector<igIndex>& indices,
					   ArrayObject::Pointer& result) {
	result = CreateArrayLike(source);
	if (!result) return false;

	// Copy elements with their exact storage type. Round-tripping through
	// double (GetElement/SetElement) would corrupt 64-bit integer IDs beyond
	// 2^53, even though the output array keeps the same type.
	switch (source->GetArrayType()) {
		case IG_FloatArray: return CopyElementsByType<float>(source, indices, result);
		case IG_DoubleArray: return CopyElementsByType<double>(source, indices, result);
		case IG_IntArray:
		case IG_INTARRAY: return CopyElementsByType<int>(source, indices, result);
		case IG_UnsignedIntArray: return CopyElementsByType<unsigned int>(source, indices, result);
		case IG_CharArray: return CopyElementsByType<char>(source, indices, result);
		case IG_UnsignedCharArray: return CopyElementsByType<unsigned char>(source, indices, result);
		case IG_ShortArray: return CopyElementsByType<short>(source, indices, result);
		case IG_UnsignedShortArray: return CopyElementsByType<unsigned short>(source, indices, result);
		case IG_LongLongArray: return CopyElementsByType<long long>(source, indices, result);
		case IG_UnsignedLongLongArray: return CopyElementsByType<unsigned long long>(source, indices, result);
		default: return false;
	}
}

} // namespace

ThresholdFilter::ThresholdFilter() {
	SetNumberOfInputs(1);
	SetNumberOfOutputs(1);
}

void ThresholdFilter::SetScalarData(ArrayObject::Pointer array, Association association, int dimension) {
	m_ScalarData = array;
	m_Association = association;
	m_Dimension = dimension;
}

bool ThresholdFilter::IsInRange(double value) const {
	if (!std::isfinite(value)) return false;

	const bool lower = m_BoundaryMode == BoundaryMode::Closed ||
					   m_BoundaryMode == BoundaryMode::LowerInclusive;
	const bool upper = m_BoundaryMode == BoundaryMode::Closed ||
					   m_BoundaryMode == BoundaryMode::UpperInclusive;
	const bool lowerOk = lower ? value >= m_LowerThreshold : value > m_LowerThreshold;
	const bool upperOk = upper ? value <= m_UpperThreshold : value < m_UpperThreshold;
	return lowerOk && upperOk;
}

bool ThresholdFilter::Execute() {
	// m_Dimension 为 -1 表示取分量模长(与 ArrayObject::GetElementValue 的约定一致),
	// 因此只拒绝小于 -1 的非法分量下标。
	if (GetNumberOfInputs() == 0 || !GetInput(0) || !m_ScalarData || m_Dimension < -1) return false;

	auto input = GetInput(0);
	if (input->GetDataObjectType() == IG_NONE) return true;
	auto mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
	if (!mesh) return false;
	return BuildOutput(mesh);
}

bool ThresholdFilter::BuildOutput(UnstructuredMesh::Pointer input) {
	const igIndex pointCount = input->GetNumberOfPoints();
	const igIndex cellCount = input->GetNumberOfCells();
	const igIndex valueCount = m_ScalarData->GetNumberOfElements();
	const igIndex expectedCount = m_Association == Association::Point ? pointCount : cellCount;
	if (m_Dimension >= m_ScalarData->GetDimension() || valueCount < expectedCount ||
		m_LowerThreshold > m_UpperThreshold) return false;

	std::vector<bool> selected(cellCount, false);
	std::vector<igIndex> selectedCells;
	selectedCells.reserve(cellCount);
	igIndex ids[IGAME_CELL_MAX_SIZE]{};

	for (igIndex cellId = 0; cellId < cellCount; ++cellId) {
		bool keep = m_Association == Association::Cell;
		int pointNumber = 0;
		if (m_Association == Association::Cell) {
			keep = IsInRange(m_ScalarData->GetElementValue(cellId, m_Dimension));
		} else {
			pointNumber = input->GetCells()->GetCellIds(cellId, ids);
			keep = m_PointEvaluation == PointEvaluation::AllScalars;
			for (int i = 0; i < pointNumber; ++i) {
				const bool inRange = IsInRange(m_ScalarData->GetElementValue(ids[i], m_Dimension));
				if (m_PointEvaluation == PointEvaluation::AllScalars) keep = keep && inRange;
				else keep = keep || inRange;
			}
		}
		selected[cellId] = keep;
		if (keep) selectedCells.push_back(cellId);
	}

	std::vector<igIndex> pointMap(pointCount, -1);
	std::vector<igIndex> selectedPoints;
	for (igIndex cellId : selectedCells) {
		const int pointNumber = input->GetCells()->GetCellIds(cellId, ids);
		for (int i = 0; i < pointNumber; ++i) {
			if (pointMap[ids[i]] < 0) {
				pointMap[ids[i]] = selectedPoints.size();
				selectedPoints.push_back(ids[i]);
			}
		}
	}

	auto output = UnstructuredMesh::New();
	auto outputPoints = Points::New();
	outputPoints->Reserve(selectedPoints.size());
	for (igIndex pointId : selectedPoints) outputPoints->AddPoint(input->GetPoints()->GetPoint(pointId));
	output->SetPoints(outputPoints);

	auto outputCells = CellArray::New();
	auto outputTypes = UnsignedIntArray::New();
	for (igIndex cellId : selectedCells) {
		const int pointNumber = input->GetCells()->GetCellIds(cellId, ids);
		igIndex outputIds[IGAME_CELL_MAX_SIZE]{};
		for (int i = 0; i < pointNumber; ++i) outputIds[i] = pointMap[ids[i]];
		outputCells->AddCellIds(outputIds, pointNumber);
		outputTypes->AddValue(input->GetCellType(cellId));
	}
	output->SetCells(outputCells, outputTypes);

	auto inputAttributes = input->GetAttributeSet();
	auto outputAttributes = AttributeSet::New();
	if (inputAttributes) {
		for (igIndex i = 0; i < static_cast<igIndex>(inputAttributes->GetNumberOfAttributes()); ++i) {
			auto& attribute = inputAttributes->GetAttribute(i);
			std::vector<igIndex> indices = attribute.attachmentType == IG_POINT ? selectedPoints : selectedCells;
			ArrayObject::Pointer outputArray;
			if (!CopyArrayElements(attribute.pointer, indices, outputArray)) return false;
			outputAttributes->AddAttribute(attribute.type, attribute.attachmentType, outputArray,
											attribute.GetDataRange());
		}
	}
	output->SetAttributeSet(outputAttributes);
	SetOutput(output);
	UpdateProgress(1.0);
	return true;
}

IGAME_NAMESPACE_END
