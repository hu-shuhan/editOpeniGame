#ifndef MeshCodecFeature_h
#define MeshCodecFeature_h

#include "iGameMacro.h"
#include "iGameDataObject.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameMeshEncoderAdapter.h"
#include <vector>

IGAME_NAMESPACE_BEGIN
class MeshCodecFeature {
public:
	MeshCodecFeature(DataObject::Pointer obj, int dataIndex) :
		m_obj(obj)
	{
		m_adapter = new MeshEncoderAdapter(m_obj);

		// 在dialog中 Index0代表顶点坐标
		// 初始化浮点数数据
		const auto& pointSet = DynamicCast<PointSet>(m_obj);
		if (dataIndex == 0)
		{
			m_dataType = DataType::Geom;
			m_Data = pointSet->GetPoints()->RawPointer();
			m_ElementNum = pointSet->GetNumberOfPoints();
			m_ElementDim = 3;
		}
		else
		{
			const auto& attr = m_obj->GetAttributeSet()->GetAttribute(dataIndex - 1);
			m_Data = DynamicCast<FlatArray<float>>(attr.pointer)->RawPointer();
			m_ElementNum = attr.pointer->GetNumberOfElements();
			m_ElementDim = attr.pointer->GetDimension();

			m_dataType = attr.attachmentType == IG_CELL ? DataType::AttachCell : DataType::AttachPoint;
		}

		// 初始化顶点数据
		m_Points = pointSet->GetPoints()->RawPointer();
		m_pointNum = pointSet->GetNumberOfPoints();
		
		// 拓扑信息
		m_IsFixedCell = m_adapter->IsFixedCellSize();
		m_FixedCellSize = m_adapter->GetFixedCellSize();
		m_CellIds = m_adapter->GetCellIdBuffer()->RawPointer();
		m_Offset = m_adapter->GetCellIdOffset()->RawPointer();

		// 所有类型的网格都构建数据结构
		// 对于cgns多面体而言 需要建立的也是体 - 点格式 所以无需二级索引拆分
		if (m_adapter->IsFixedCellSize())
		{
			MeshCodecAdjacency* mca = new MeshCodecAdjacency(
				reinterpret_cast<unsigned int*>(m_adapter->GetCellIdBuffer()->RawPointer()),
				m_adapter->GetCellIdBufferSize(),
				m_adapter->GetNumberOfPoints(),
				m_adapter->GetNumberOfCells()
			);
			this->m_Adj = mca->GetAdjacencyData();
		}
		else
		{
			MeshCodecAdjacency* mca = new MeshCodecAdjacency(
				reinterpret_cast<unsigned int*>(m_adapter->GetCellIdBuffer()->RawPointer()),
				m_adapter->GetCellIdOffset()->RawPointer(),
				m_adapter->GetCellIdBufferSize(),
				m_adapter->GetCellIdOffsetSize(),
				m_adapter->GetNumberOfPoints(),
				m_adapter->GetNumberOfCells()
			);
			this->m_Adj = mca->GetAdjacencyData();
		}
	};

	// vortex
	/*
	std::vector<std::vector<float>> GetDataPointVortex() {
		int attrDim = m_attr.pointer->GetDimension();
		int dataPointNum = m_attr.pointer->GetNumberOfElements();

		// 只处理2维和3维向量
		if (attrDim != 2 && attrDim != 3) {
			// 返回空结果
			return std::vector<std::vector<float>>();
		}

		// 计算每个分量的梯度（雅可比矩阵）
		std::vector<std::vector<std::array<float, 3>>> gradients = GetDataPointGradient();

		// 存储结果，旋度总是3维向量
		std::vector<std::vector<float>> vorticities(dataPointNum, { 0.0f, 0.0f, 0.0f });

		// 计算旋度
		for (igIndex idx = 0; idx < dataPointNum; ++idx) {
			if (attrDim == 3) {
				// 3D向量场旋度计算
				const auto& grad_x = gradients[idx][0]; // 第一个分量的梯度
				const auto& grad_y = gradients[idx][1]; // 第二个分量的梯度
				const auto& grad_z = gradients[idx][2]; // 第三个分量的梯度

				// 计算旋度公式: ∇ × v
				float omega_x = grad_z[1] - grad_y[2]; // ∂vy/∂z - ∂vz/∂y
				float omega_y = grad_x[2] - grad_z[0]; // ∂vz/∂x - ∂vx/∂z
				float omega_z = grad_y[0] - grad_x[1]; // ∂vx/∂y - ∂vy/∂x

				vorticities[idx] = { omega_x, omega_y, omega_z };
			}
			else if (attrDim == 2) {
				// 2D向量场旋度计算（只有z分量）
				const auto& grad_x = gradients[idx][0]; // 第一个分量的梯度
				const auto& grad_y = gradients[idx][1]; // 第二个分量的梯度

				// 2D向量场的旋度是标量，但我们存储在z分量
				float omega_z = grad_y[0] - grad_x[1]; // ∂vx/∂y - ∂vy/∂x

				vorticities[idx] = { 0.0f, 0.0f, omega_z };
			}

			// 归一化（如果需要）
			float magnitude = std::sqrt(
				vorticities[idx][0] * vorticities[idx][0] +
				vorticities[idx][1] * vorticities[idx][1] +
				vorticities[idx][2] * vorticities[idx][2]
			);

			if (magnitude > 1e-6f) {
				vorticities[idx][0] /= magnitude;
				vorticities[idx][1] /= magnitude;
				vorticities[idx][2] /= magnitude;
			}
			else {
				vorticities[idx] = { 0.0f, 0.0f, 0.0f };
			}
		}

		return vorticities;
	}
	
	*/

	/*
	std::vector<std::vector<std::array<float, 3>>> GetDataPointGradient() {
		std::vector<std::vector<std::array<float, 3>>> gradients(m_ElementNum,
			std::vector<std::array<float, 3>>(m_ElementDim, { 0.0f, 0.0f, 0.0f }));
		std::vector<float> sumWeights(m_ElementNum, 0.0f);
		
		ThreadPool::parallelFor(0, m_ElementNum, [&](int start, int end) -> void {
			for (igIndex idx = start; idx < end; ++idx) {
				std::vector<Point> neighPointPos;
				std::vector<igIndex> neighborVerts;
				GetNeighbourDataPointPos(idx, neighPointPos, neighborVerts);
				Point v1 = GetDataPointPos(idx);

				for (int neighIndex = 0; neighIndex < neighPointPos.size(); neighIndex++) {
					Point v2 = neighPointPos[neighIndex];

					float x = v1[0] - v2[0];
					float y = v1[1] - v2[1];
					float z = v1[2] - v2[2];

					// 计算空间距离的倒数作为权重
					float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
					sumWeights[idx] += weight;

					// 计算每个向量分量的梯度
					for (int dimIndex = 0; dimIndex < m_ElementDim; dimIndex++) {
						float value = GetDelta(idx, dimIndex, neighIndex, neighborVerts);

						// 雅可比
						gradients[idx][dimIndex][0] += x * weight * value;
						gradients[idx][dimIndex][1] += y * weight * value;
						gradients[idx][dimIndex][2] += z * weight * value;
					}
				}

				// 归一化
				if (sumWeights[idx] > 0) {
					for (int d = 0; d < m_ElementDim; d++) {
						gradients[idx][d][0] /= sumWeights[idx];
						gradients[idx][d][1] /= sumWeights[idx];
						gradients[idx][d][2] /= sumWeights[idx];
					}
				}
			}
			});

		//for (igIndex idx = 0; idx < dataNum; ++idx) {
		//	std::vector<Point> neighPointPos;
		//	igIndex neighborVerts[256]{};
		//	GetNeighbourDataPointPos(idx, neighPointPos, neighborVerts);
		//	Point v1 = GetDataPointPos(idx);

		//	for (int neighIndex = 0; neighIndex < neighPointPos.size(); neighIndex++) {
		//		Point v2 = neighPointPos[neighIndex];

		//		float x = v1[0] - v2[0];
		//		float y = v1[1] - v2[1];
		//		float z = v1[2] - v2[2];

		//		// 计算空间距离的倒数作为权重
		//		float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
		//		sumWeights[idx] += weight;

		//		// 计算每个向量分量的梯度
		//		for (int dimIndex = 0; dimIndex < dataDim; dimIndex++) {
		//			float value = Getdelta(idx, dimIndex, neighIndex, dataDim, neighborVerts);

		//			// 雅可比
		//			gradients[idx][dimIndex][0] += x * weight * value;
		//			gradients[idx][dimIndex][1] += y * weight * value;
		//			gradients[idx][dimIndex][2] += z * weight * value;
		//		}
		//	}

		//	// 归一化
		//	if (sumWeights[idx] > 0) {
		//		for (int d = 0; d < dataDim; d++) {
		//			gradients[idx][d][0] /= sumWeights[idx];
		//			gradients[idx][d][1] /= sumWeights[idx];
		//			gradients[idx][d][2] /= sumWeights[idx];
		//		}
		//	}
		//}

		return gradients;
	}
	*/
	
	/*
	std::vector<std::vector<std::array<float, 3>>> GetDataPointGradient() {
		std::vector<std::vector<std::array<float, 3>>> gradients(m_ElementNum,
			std::vector<std::array<float, 3>>(m_ElementDim, { 0.0f, 0.0f, 0.0f }));
		std::vector<float> sumWeights(m_ElementNum, 0.0f);

		ThreadPool::parallelFor(0, m_ElementNum, [&](int start, int end) -> void {
			// 预分配常用的临时空间
			std::vector<Point> neighPointPos;
			std::vector<igIndex> neighborVerts;
			neighPointPos.reserve(128);
			neighborVerts.reserve(128);

			// 每个线程局部的临时数组
			float localDx[128], localDy[128], localDz[128], localWeights[128];

			for (igIndex idx = start; idx < end; ++idx) {
				// 清空而不是重新分配
				neighPointPos.clear();
				neighborVerts.clear();

				GetNeighbourDataPointPos(idx, neighPointPos, neighborVerts);
				Point v1 = GetDataPointPos(idx);

				// 获取邻居点数量
				const int neighCount = neighPointPos.size();

				// 预计算所有向量和权重
				for (int n = 0; n < neighCount; ++n) {
					const Point& v2 = neighPointPos[n];
					localDx[n] = v1[0] - v2[0];
					localDy[n] = v1[1] - v2[1];
					localDz[n] = v1[2] - v2[2];

					const float distSq = localDx[n] * localDx[n] + localDy[n] * localDy[n] + localDz[n] * localDz[n];
					localWeights[n] = 1.0f / std::sqrt(distSq + 1e-10f);
					sumWeights[idx] += localWeights[n];
				}

				// 计算每个分量的梯度
				for (int dimIndex = 0; dimIndex < m_ElementDim; dimIndex++) {
					float& gradX = gradients[idx][dimIndex][0];
					float& gradY = gradients[idx][dimIndex][1];
					float& gradZ = gradients[idx][dimIndex][2];

					for (int n = 0; n < neighCount; n++) {
						const float delta = GetDelta(idx, dimIndex, n, neighborVerts);
						const float weighted = localWeights[n] * delta;

						gradX += localDx[n] * weighted;
						gradY += localDy[n] * weighted;
						gradZ += localDz[n] * weighted;
					}
				}
			}
			});

		// 归一化 - 在并行区域外执行，避免竞争条件
		for (igIndex idx = 0; idx < m_ElementNum; ++idx) {
			if (sumWeights[idx] > 1e-6f) {
				const float invWeight = 1.0f / sumWeights[idx];
				for (int d = 0; d < m_ElementDim; d++) {
					gradients[idx][d][0] *= invWeight;
					gradients[idx][d][1] *= invWeight;
					gradients[idx][d][2] *= invWeight;
				}
			}
		}

		return gradients;
	}
	*/

	std::vector<std::vector<std::array<float, 3>>> GetDataPointGradient() {
		std::vector<std::vector<std::array<float, 3>>> gradients(m_ElementNum,
			std::vector<std::array<float, 3>>(m_ElementDim, { 0.0f, 0.0f, 0.0f }));
		std::vector<float> sumWeights(m_ElementNum, 0.0f);

		ThreadPool::parallelFor(0, m_ElementNum, [&](int start, int end) -> void {
			// 预分配常用的临时空间
			std::vector<Point> neighPointPos;
			std::vector<igIndex> neighborVerts;
			neighPointPos.reserve(128);
			neighborVerts.reserve(128);

			for (igIndex idx = start; idx < end; ++idx) {
				// 清空而不是重新分配
				neighPointPos.clear();
				neighborVerts.clear();

				GetNeighbourDataPointPos(idx, neighPointPos, neighborVerts);
				Point v1 = GetDataPointPos(idx);

				// 获取邻居点数量
				const int neighCount = neighPointPos.size();

				// 使用SIMD友好的数据排列
				float* dx = (float*)_mm_malloc(neighCount * sizeof(float), 16);
				float* dy = (float*)_mm_malloc(neighCount * sizeof(float), 16);
				float* dz = (float*)_mm_malloc(neighCount * sizeof(float), 16);
				float* weights = (float*)_mm_malloc(neighCount * sizeof(float), 16);

				// 预计算所有向量和权重
	#pragma omp simd
				for (int n = 0; n < neighCount; ++n) {
					const Point& v2 = neighPointPos[n];
					dx[n] = v1[0] - v2[0];
					dy[n] = v1[1] - v2[1];
					dz[n] = v1[2] - v2[2];

					const float distSq = dx[n] * dx[n] + dy[n] * dy[n] + dz[n] * dz[n];
					weights[n] = 1.0f / std::sqrt(distSq + 1e-10f);
					sumWeights[idx] += weights[n];
				}

				// 计算每个分量的梯度
				for (int dimIndex = 0; dimIndex < m_ElementDim; dimIndex++) {
					float& gradX = gradients[idx][dimIndex][0];
					float& gradY = gradients[idx][dimIndex][1];
					float& gradZ = gradients[idx][dimIndex][2];

	#pragma omp simd
					for (int n = 0; n < neighCount; n++) {
						const float delta = GetDelta(idx, dimIndex, n, neighborVerts);
						const float weighted = weights[n] * delta;

						gradX += dx[n] * weighted;
						gradY += dy[n] * weighted;
						gradZ += dz[n] * weighted;
					}
				}

				// 释放内存
				_mm_free(dx);
				_mm_free(dy);
				_mm_free(dz);
				_mm_free(weights);
			}
			});

		// 归一化
	#pragma omp parallel for
		for (igIndex idx = 0; idx < m_ElementNum; ++idx) {
			if (sumWeights[idx] > 1e-6f) {
				const float invWeight = 1.0f / sumWeights[idx];
				for (int d = 0; d < m_ElementDim; d++) {
					gradients[idx][d][0] *= invWeight;
					gradients[idx][d][1] *= invWeight;
					gradients[idx][d][2] *= invWeight;
				}
			}
		}

		return gradients;
	}

	/*
	std::vector<std::vector<float>> GetDataPointLaplacian() {
		// 存储各维度的拉普拉斯值
		std::vector<std::vector<float>> laplacians(m_ElementNum,
			std::vector<float>(m_ElementDim, 0.0f));

		// 对每个数据点计算拉普拉斯算子
		for (igIndex idx = 0; idx < m_ElementNum; ++idx) {
			std::vector<Point> neighPointPos;
			std::vector<igIndex> neighborVerts;
			GetNeighbourDataPointPos(idx, neighPointPos, neighborVerts);

			Point v1 = GetDataPointPos(idx);
			std::vector<float> tempValues(m_ElementDim, 0.0f);
			float weightSum = 0.0f;

			// 遍历所有邻居点
			for (int neighIndex = 0; neighIndex < neighPointPos.size(); neighIndex++) {
				Point v2 = neighPointPos[neighIndex];
				float x = v1[0] - v2[0];
				float y = v1[1] - v2[1];
				float z = v1[2] - v2[2];

				// 计算空间距离的倒数作为权重
				float weight = 1.0f / std::sqrt(x * x + y * y + z * z);
				weightSum += weight;

				// 计算每个分量的拉普拉斯值
				for (int dimIndex = 0; dimIndex < m_ElementDim; dimIndex++) {
					float value = GetDelta(idx, dimIndex, neighIndex, neighborVerts);
					tempValues[dimIndex] += weight * value;
				}
			}

			// 归一化并存储结果
			if (weightSum > 0) {
				for (int d = 0; d < m_ElementDim; d++) {
					laplacians[idx][d] = tempValues[d] / weightSum;
				}
			}
		}

		return laplacians;
	}
	*/
	
	std::vector<std::vector<float>> GetDataPointLaplacian() {
		// 存储各维度的拉普拉斯值
		std::vector<std::vector<float>> laplacians(m_ElementNum,
			std::vector<float>(m_ElementDim, 0.0f));

		ThreadPool::parallelFor(0, m_ElementNum, [&](int start, int end) -> void {
			// 预分配临时空间
			std::vector<Point> neighPointPos;
			std::vector<igIndex> neighborVerts;
			neighPointPos.reserve(128);
			neighborVerts.reserve(128);

			for (igIndex idx = start; idx < end; ++idx) {
				// 清空而不是重新分配
				neighPointPos.clear();
				neighborVerts.clear();

				GetNeighbourDataPointPos(idx, neighPointPos, neighborVerts);
				Point v1 = GetDataPointPos(idx);

				// 获取邻居点数量
				const int neighCount = neighPointPos.size();

				// 使用SIMD友好的数据排列
				float* dx = (float*)_mm_malloc(neighCount * sizeof(float), 16);
				float* dy = (float*)_mm_malloc(neighCount * sizeof(float), 16);
				float* dz = (float*)_mm_malloc(neighCount * sizeof(float), 16);
				float* weights = (float*)_mm_malloc(neighCount * sizeof(float), 16);
				float* tempValues = (float*)_mm_malloc(m_ElementDim * sizeof(float), 16);

				// 初始化临时值
				memset(tempValues, 0, m_ElementDim * sizeof(float));
				float weightSum = 0.0f;

				// 预计算所有向量和权重
#pragma omp simd
				for (int n = 0; n < neighCount; ++n) {
					const Point& v2 = neighPointPos[n];
					dx[n] = v1[0] - v2[0];
					dy[n] = v1[1] - v2[1];
					dz[n] = v1[2] - v2[2];

					const float distSq = dx[n] * dx[n] + dy[n] * dy[n] + dz[n] * dz[n];
					weights[n] = 1.0f / std::sqrt(distSq + 1e-10f);
					weightSum += weights[n];
				}

				// 计算每个分量的拉普拉斯值
				for (int dimIndex = 0; dimIndex < m_ElementDim; dimIndex++) {
					float accumValue = 0.0f;

#pragma omp simd reduction(+:accumValue)
					for (int n = 0; n < neighCount; n++) {
						const float delta = GetDelta(idx, dimIndex, n, neighborVerts);
						accumValue += weights[n] * delta;
					}

					tempValues[dimIndex] = accumValue;
				}

				// 归一化并存储结果
				if (weightSum > 1e-6f) {
					const float invWeight = 1.0f / weightSum;
					for (int d = 0; d < m_ElementDim; d++) {
						laplacians[idx][d] = tempValues[d] * invWeight;
					}
				}

				// 释放内存
				_mm_free(dx);
				_mm_free(dy);
				_mm_free(dz);
				_mm_free(weights);
				_mm_free(tempValues);
			}
			});

		return laplacians;
	}

private:
	float GetDelta(int idx, int dimIndex, int neighIndex, std::vector<igIndex>& neighborVerts)
	{
		const float* idxData = m_Data + idx * m_ElementDim;
		const float* neighData = m_Data + neighborVerts[neighIndex] * m_ElementDim;

		return idxData[dimIndex] - neighData[dimIndex];

		//return m_Data[idx * m_ElementDim + dimIndex] -
		//	m_Data[neighborVerts[neighIndex] * m_ElementDim + dimIndex];
	}

	Point MiddlePos(igIndex cellId)
	{
		Point p = { 0.0f, 0.0f, 0.0f };
		int start, end;
		GetIdStartEnd(cellId, start, end);

		int numPoints = end - start;
		if (numPoints == 0) return p;

		float invNumPoints = 1.0f / numPoints;
		for (int i = start; i < end; i++)
		{
			const Point point = GetPoint(m_CellIds[i]);
			p[0] += point[0];
			p[1] += point[1];
			p[2] += point[2];
		}

		// 一次性缩放，避免多次除法
		p[0] *= invNumPoints;
		p[1] *= invNumPoints;
		p[2] *= invNumPoints;

		return p;

		//Point p = { 0.0f, 0.0f, 0.0f };
		//int start, end;
		//GetIdStartEnd(cellId, start, end);

		//for (int i = start; i < end; i++)
		//{
		//	p += GetPoint(m_CellIds[i]);
		//}
		//return p;
	}

	Point GetPoint(igIndex pId)
	{
		const float* p = m_Points + pId * 3;
		return { p[0], p[1], p[2] };

		/*Point p = { 0.0f, 0.0f, 0.0f };
		p[0] = m_Points[pId * 3 + 0];
		p[1] = m_Points[pId * 3 + 1];
		p[2] = m_Points[pId * 3 + 2];
		return p;*/
	}

	Point GetDataPointPos(igIndex id)
	{
		if (m_dataType == DataType::AttachCell)
		{
			return MiddlePos(id);
		}
		else
		{
			return GetPoint(id);
		}
	}

	void GetNeighbourDataPointPos(igIndex id, std::vector<Point>& points, std::vector<igIndex>& neighborIndices)
	{
		points.reserve(128);
		neighborIndices.reserve(128);

		points.clear();
		neighborIndices.clear();

		switch (m_dataType)
		{
		case DataType::AttachCell:
		{
			auto neighNum = GetCellNeighbour(id, neighborIndices);
			points.resize(neighNum);
			for (int i = 0; i < neighNum; i++)
			{
				points[i] = MiddlePos(neighborIndices[i]);
			}
			break;
		}
		case DataType::Geom:
		case DataType::AttachPoint:
		{
			auto neighNum = GetPointRing(id, neighborIndices);
			points.resize(neighNum);
			for (int i = 0; i < neighNum; i++)
			{
				points[i] = GetPoint(neighborIndices[i]);
			}
			break;
		}
		}

		/*switch (m_dataType)
		{
		case DataType::AttachCell:
		{
			auto neighNum = GetCellNeighbour(id, neighborIndices);

			for (int i = 0; i < neighNum; i++)
			{
				points.push_back(MiddlePos(neighborIndices[i]));
			}
			break;
		}
		case DataType::Geom:
		case DataType::AttachPoint:
		{
			auto neighNum = GetPointRing(id, neighborIndices);

			for (int i = 0; i < neighNum; i++)
			{
				points.push_back(GetPoint(neighborIndices[i]));
			}
			break;
		}
		}*/
	}

	void GetIdStartEnd(igIndex cellId, int& start, int& end)
	{
		start = m_IsFixedCell ? cellId * m_FixedCellSize : m_Offset[cellId];
		end = m_IsFixedCell ? (cellId + 1) * m_FixedCellSize : m_Offset[cellId + 1];
	}

	int GetCellPointNum(igIndex cellId)
	{
		int start, end;
		GetIdStartEnd(cellId, start, end);

		return end - start;
	}

	int GetCellPointId(igIndex cellId, int index)
	{
		int start, end;
		GetIdStartEnd(cellId, start, end);

		return m_CellIds[start + index];
	}

	int GetCellNeighbour(igIndex cellId, std::vector<igIndex>& neighIndices)
	{
		std::unordered_set<igIndex> uniqueIndices;
		int pointNum = GetCellPointNum(cellId);

		for (int i = 0; i < pointNum; i++)
		{
			auto pid = GetCellPointId(cellId, i);
			for (int j = 0; j < m_Adj.counts[pid]; j++)
			{
				uniqueIndices.insert(m_Adj.data[m_Adj.offsets[pid] + j]);
			}
		}

		uniqueIndices.erase(cellId);
		for (igIndex pointId : uniqueIndices) {
			neighIndices.push_back(pointId);
		}

		return neighIndices.size();
	}

	int GetPointRing(igIndex pointId, std::vector<igIndex>& neighIndices)
	{
		std::unordered_set<igIndex> uniqueIndices;

		for (int i = 0; i < m_Adj.counts[pointId]; i++)
		{
			int cellId = m_Adj.data[m_Adj.offsets[pointId] + i];
			int pointNum = GetCellPointNum(cellId);

			for (int j = 0; j < pointNum; j++) {
				uniqueIndices.insert(GetCellPointId(cellId, j));
			}
		}

		uniqueIndices.erase(pointId);
		for (igIndex pointId : uniqueIndices) {
			neighIndices.push_back(pointId);
		}

		return neighIndices.size();
	}
	
	enum class DataType {
		AttachCell,
		AttachPoint,
		Geom
	};

	// 计算模式
	DataType m_dataType;

	// 浮点数数据
	float* m_Data;
	size_t m_ElementNum;
	size_t m_ElementDim;

	// 坐标
	float* m_Points;
	size_t m_pointNum;

	// 拓扑信息
	bool m_IsFixedCell;
	size_t m_FixedCellSize;
	igIndex* m_CellIds;
	unsigned int* m_Offset;

	bool m_pointMode;
	DataObject::Pointer m_obj;

	MeshEncoderAdapter* m_adapter;
	MeshCodecAdjacency::CellAdjacency m_Adj;
};

IGAME_NAMESPACE_END
#endif