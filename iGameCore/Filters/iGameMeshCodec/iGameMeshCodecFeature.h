#ifndef MeshCodecFeature_h
#define MeshCodecFeature_h

#include "iGameDataObject.h"
#include "iGameMacro.h"
#include "iGameMeshEncoderAdapter.h"
#include "meshoptimizer.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <vector>
#include <execution>
#include <numeric>
#include <cmath>
#include <random>
#include <algorithm>
#include <memory>

#if defined(__GNUC__) && defined(__x86_64__)
#include <mm_malloc.h>
#endif

IGAME_NAMESPACE_BEGIN
class MeshCodecFeature {
public:
    MeshCodecFeature(DataObject::Pointer obj, int dataIndex) : m_obj(obj) {
        m_adapter = std::make_unique<MeshEncoderAdapter>(m_obj);

        // 在dialog中 Index0代表顶点坐标
        // 初始化浮点数数据
        const auto& pointSet = DynamicCast<PointSet>(m_obj);
        if (dataIndex == 1) {
            m_dataType = DataType::Geom;
            m_Data = pointSet->GetPoints()->RawPointer();
            m_ElementNum = pointSet->GetNumberOfPoints();
            m_ElementDim = 3;
        } else {
            const auto& attr = m_obj->GetAttributeSet()->GetAttribute(dataIndex - 1);
            m_ElementNum = attr.pointer->GetNumberOfElements();
            m_ElementDim = attr.pointer->GetDimension();
            m_dataType = attr.attachmentType == IG_CELL ? DataType::AttachCell : DataType::AttachPoint;

            // 参考 AttrEncoder：通过 GetValue 统一读取，不依赖 RawPointer
            m_AttrConverted.resize(m_ElementNum * m_ElementDim);
            for (igIndex i = 0; i < static_cast<igIndex>(m_ElementNum); ++i) {
                for (int k = 0; k < static_cast<int>(m_ElementDim); ++k) {
                    m_AttrConverted[i * m_ElementDim + k] =
                        static_cast<float>(attr.pointer->GetValue(i * m_ElementDim + k));
                }
            }
            m_Data = m_AttrConverted.data();
        }

        // 初始化顶点数据
        m_Points = pointSet->GetPoints()->RawPointer();
        m_pointNum = pointSet->GetNumberOfPoints();

        // 拓扑信息
        m_IsFixedCell = m_adapter->IsFixedCellSize();
        m_FixedCellSize = m_adapter->GetFixedCellSize();

        // 所有类型的网格都构建数据结构
        // 需要判断是否是二级索引多面体网格（CGNS等）
        if (m_adapter->IsSecondaryIndexPolyhedronMesh()) {
            // 多面体网格叠加非结构化网格的情况：直接提取 volume->point 关系
            // 注意：这里不需要保留二级索引结构，因为我们只需要计算邻接关系，不需要编码
            std::vector<unsigned int> volume2facesIndex;
            std::vector<unsigned int> volume2facesOffset;
            std::vector<unsigned int> face2pointsIndex;
            std::vector<unsigned int> face2pointsOffset;
            m_adapter->SplitSecondaryIndex(volume2facesIndex, volume2facesOffset, face2pointsIndex, face2pointsOffset);

            // 提取 volume->point：遍历每个 volume 的所有 faces，收集所有 points（去重）
            size_t numVolumes = volume2facesOffset.size() - 1;
            
            // 先并行计算每个volume的点集
            std::vector<std::vector<unsigned int>> volumePointSets(numVolumes);
            std::vector<size_t> indices(numVolumes);
            std::iota(indices.begin(), indices.end(), 0);
            
            std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), 
                [&](size_t volumeId) {
                    std::unordered_set<unsigned int> volumePoints; // 使用set去重
                    unsigned int faceStart = volume2facesOffset[volumeId];
                    unsigned int faceEnd = volume2facesOffset[volumeId + 1];

                    // 遍历该volume的所有faces
                    for (unsigned int faceIdx = faceStart; faceIdx < faceEnd; ++faceIdx) {
                        unsigned int faceId = volume2facesIndex[faceIdx];
                        unsigned int pointStart = face2pointsOffset[faceId];
                        unsigned int pointEnd = face2pointsOffset[faceId + 1];

                        // 收集该face的所有points
                        for (unsigned int pointIdx = pointStart; pointIdx < pointEnd; ++pointIdx) {
                            volumePoints.insert(face2pointsIndex[pointIdx]);
                        }
                    }

                    // 将去重后的points存储到临时vector
                    volumePointSets[volumeId].assign(volumePoints.begin(), volumePoints.end());
                }
            );
            
            // 串行合并结果
            m_ExpandedOffset.reserve(numVolumes + 1);
            m_ExpandedOffset.push_back(0);
            
            for (size_t volumeId = 0; volumeId < numVolumes; ++volumeId) {
                m_ExpandedCellIds.insert(m_ExpandedCellIds.end(), 
                    volumePointSets[volumeId].begin(), volumePointSets[volumeId].end());
                m_ExpandedOffset.push_back(m_ExpandedCellIds.size());
            }

            // 指向提取后的 volume->point 数据
            m_CellIds = m_ExpandedCellIds.data();
            m_Offset = m_ExpandedOffset.data();
            m_IsFixedCell = false; // 提取后的数据是非固定大小的

            // 构建 volume->point 的邻接关系（实际会反向为 point->volume）
            m_Adjacency = std::make_unique<MeshCodecAdjacency>(
                    reinterpret_cast<unsigned int*>(m_ExpandedCellIds.data()), m_ExpandedOffset.data(),
                    m_ExpandedCellIds.size(), m_ExpandedOffset.size(), m_adapter->GetNumberOfPoints(),
                    m_adapter->GetNumberOfCells());
            this->m_Adj = m_Adjacency->GetAdjacencyData();
        } else if (m_adapter->IsFixedCellSize()) {
            // 固定大小的cell（如结构化网格）
            m_CellIds = m_adapter->GetCellIdBuffer()->RawPointer();
            m_Offset = m_adapter->GetCellIdOffset()->RawPointer();

            m_Adjacency = std::make_unique<MeshCodecAdjacency>(
                    reinterpret_cast<unsigned int*>(m_adapter->GetCellIdBuffer()->RawPointer()),
                    m_adapter->GetCellIdBufferSize(), m_adapter->GetNumberOfPoints(), m_adapter->GetNumberOfCells());
            this->m_Adj = m_Adjacency->GetAdjacencyData();
        } else {
            // 非固定大小的cell（如普通非结构化网格）
            m_CellIds = m_adapter->GetCellIdBuffer()->RawPointer();
            m_Offset = m_adapter->GetCellIdOffset()->RawPointer();

            m_Adjacency = std::make_unique<MeshCodecAdjacency>(
                    reinterpret_cast<unsigned int*>(m_adapter->GetCellIdBuffer()->RawPointer()),
                    m_adapter->GetCellIdOffset()->RawPointer(), m_adapter->GetCellIdBufferSize(),
                    m_adapter->GetCellIdOffsetSize(), m_adapter->GetNumberOfPoints(), m_adapter->GetNumberOfCells());
            this->m_Adj = m_Adjacency->GetAdjacencyData();
        }
    };

    // vortex

    std::vector<std::vector<float>> GetDataPointVortex() {
        int attrDim = m_ElementDim;
        int dataPointNum = m_ElementNum;

        // 只处理2维和3维向量
        if (attrDim != 2 && attrDim != 3) {
            // 返回空结果
            return std::vector<std::vector<float>>();
        }

        // 计算每个分量的梯度（雅可比矩阵）
        std::vector<std::vector<std::array<float, 3>>> gradients = GetDataPointGradient();

        // 存储结果，旋度总是3维向量
        std::vector<std::vector<float>> vorticities(dataPointNum, {0.0f, 0.0f, 0.0f});

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

                vorticities[idx] = {omega_x, omega_y, omega_z};
            } else if (attrDim == 2) {
                // 2D向量场旋度计算（只有z分量）
                const auto& grad_x = gradients[idx][0]; // 第一个分量的梯度
                const auto& grad_y = gradients[idx][1]; // 第二个分量的梯度

                // 2D向量场的旋度是标量，但我们存储在z分量
                float omega_z = grad_y[0] - grad_x[1]; // ∂vx/∂y - ∂vy/∂x

                vorticities[idx] = {0.0f, 0.0f, omega_z};
            }

            // 归一化（如果需要）
            float magnitude =
                    std::sqrt(vorticities[idx][0] * vorticities[idx][0] + vorticities[idx][1] * vorticities[idx][1] +
                              vorticities[idx][2] * vorticities[idx][2]);

            if (magnitude > 1e-6f) {
                vorticities[idx][0] /= magnitude;
                vorticities[idx][1] /= magnitude;
                vorticities[idx][2] /= magnitude;
            } else {
                vorticities[idx] = {0.0f, 0.0f, 0.0f};
            }
        }

        return vorticities;
    }


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
        return GetDataPointGradient(m_Data);
    }

    // 显式传入待计算数据指针的梯度计算版本，便于对扰动后的数据进行评估
    std::vector<std::vector<std::array<float, 3>>> GetDataPointGradient(const float* data) {
        std::vector<std::vector<std::array<float, 3>>> gradients(
                m_ElementNum, std::vector<std::array<float, 3>>(m_ElementDim, {0.0f, 0.0f, 0.0f}));
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
                float* dx = (float*) _mm_malloc(neighCount * sizeof(float), 16);
                float* dy = (float*) _mm_malloc(neighCount * sizeof(float), 16);
                float* dz = (float*) _mm_malloc(neighCount * sizeof(float), 16);
                float* weights = (float*) _mm_malloc(neighCount * sizeof(float), 16);

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
                        const float delta = GetDelta(data, idx, dimIndex, n, neighborVerts);
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
        return GetDataPointLaplacian(m_Data);
    }

    // 显式传入待计算数据指针的拉普拉斯计算版本，便于对扰动后的数据进行评估
    /*
    std::vector<std::vector<float>> GetDataPointLaplacian(const float* data) {
        // 存储各维度的拉普拉斯值
        std::vector<std::vector<float>> laplacians(m_ElementNum, std::vector<float>(m_ElementDim, 0.0f));

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
                float* dx = (float*) _mm_malloc(neighCount * sizeof(float), 16);
                float* dy = (float*) _mm_malloc(neighCount * sizeof(float), 16);
                float* dz = (float*) _mm_malloc(neighCount * sizeof(float), 16);
                float* weights = (float*) _mm_malloc(neighCount * sizeof(float), 16);
                float* tempValues = (float*) _mm_malloc(m_ElementDim * sizeof(float), 16);

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

#pragma omp simd reduction(+ : accumValue)
                    for (int n = 0; n < neighCount; n++) {
                        const float delta = GetDelta(data, idx, dimIndex, n, neighborVerts);
                        accumValue += weights[n] * delta;
                    }

                    tempValues[dimIndex] = accumValue;
                }

                // 归一化并存储结果
                if (weightSum > 1e-6f) {
                    const float invWeight = 1.0f / weightSum;
                    for (int d = 0; d < m_ElementDim; d++) { laplacians[idx][d] = tempValues[d] * invWeight; }
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
    */

    std::vector<std::vector<float>> GetDataPointLaplacian(const float* data) {
        // 按照 ND_i(u) = |u_i - 1/|N(i)| * \sum_{j\in N(i)} u_j| 的形式计算拉普拉斯
        std::vector<std::vector<float>> laplacians(m_ElementNum, std::vector<float>(m_ElementDim, 0.0f));

        ThreadPool::parallelFor(0, m_ElementNum, [&](int start, int end) -> void {
            std::vector<Point> neighPointPos;
            std::vector<igIndex> neighborVerts;
            neighPointPos.reserve(128);
            neighborVerts.reserve(128);

            for (igIndex idx = start; idx < end; ++idx) {
                neighPointPos.clear();
                neighborVerts.clear();

                GetNeighbourDataPointPos(idx, neighPointPos, neighborVerts);
                const int neighCount = static_cast<int>(neighborVerts.size());
                if (neighCount == 0) { continue; }

                for (int dimIndex = 0; dimIndex < static_cast<int>(m_ElementDim); ++dimIndex) {
                    float accumDelta = 0.0f;

                    // \frac{1}{|N(i)|} \sum_{j\in N(i)} (u_i - u_j)
                    for (int n = 0; n < neighCount; ++n) {
                        accumDelta += GetDelta(data, static_cast<int>(idx), dimIndex, n, neighborVerts);
                    }

                    const float avgDelta = accumDelta / static_cast<float>(neighCount);
                    laplacians[idx][dimIndex] = std::fabs(avgDelta);
                }
            }
        });

        return laplacians;
    }

    // 基于 Verificarlo 思想，对输入数据进行多次随机扰动，计算拉普拉斯算子的平均绝对误差，
    // 再对每个点的误差张量做 L1 范数，并按 95 分位进行归一化：
    // \hat{S}_i = min(1, S_i / (S_95 + eps))
    std::vector<float> GetDataPointTolerance(int sampleCount, float eps = 1e-8f) {
        if (sampleCount <= 0 || m_ElementNum == 0 || m_ElementDim == 0) {
            return std::vector<float>(m_ElementNum, 0.0f);
        }

        // 基线结果：在原始数据上的拉普拉斯（内部已使用线程池并行）
        const auto baseline = GetDataPointLaplacian(m_Data);

        const size_t pointCount = m_ElementNum;
        const size_t dim = m_ElementDim;

        // 用于累积每个点、每个分量的平均绝对误差
        std::vector<std::vector<float>> meanError(pointCount, std::vector<float>(dim, 0.0f));

        const size_t valueCount = pointCount * dim;
        std::vector<float> perturbedData(valueCount);

        // 方案 A：一次 Monte Carlo 试验内对全体数据使用同一个随机保留位数 N
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distBits(7, 23);

        for (int sample = 0; sample < sampleCount; ++sample) {
            const int keptBits = distBits(gen);

            // 对原始数据逐标量进行随机量化扰动（按标量并行/向量化）
#pragma omp parallel for
            for (long long i = 0; i < static_cast<long long>(valueCount); ++i) {
                const size_t idx = static_cast<size_t>(i);
                perturbedData[idx] = MCAPerturb(m_Data[idx], keptBits);
            }

            // 在扰动数据上计算拉普拉斯（内部使用线程池并行）
            const auto perturbedLaplacian = GetDataPointLaplacian(perturbedData.data());

            // 累积误差：对每个点、每个分量并行累加
#pragma omp parallel for
            for (long long pi = 0; pi < static_cast<long long>(pointCount); ++pi) {
                const size_t p = static_cast<size_t>(pi);
                for (size_t d = 0; d < dim; ++d) {
                    const float diff = std::fabs(perturbedLaplacian[p][d] - baseline[p][d]);
                    meanError[p][d] += diff;
                }
            }
        }

        // 计算平均绝对误差
        const float invSample = 1.0f / static_cast<float>(sampleCount);
#pragma omp parallel for
        for (long long pi = 0; pi < static_cast<long long>(pointCount); ++pi) {
            const size_t p = static_cast<size_t>(pi);
            for (size_t d = 0; d < dim; ++d) {
                meanError[p][d] *= invSample;
            }
        }

        // 对每个点的误差张量计算 elementwise L1 范数 S_i
        const size_t n = meanError.size();
        std::vector<float> norms(n, 0.0f);

#pragma omp parallel for
        for (long long i = 0; i < static_cast<long long>(n); ++i) {
            const size_t idx = static_cast<size_t>(i);
            float s = 0.0f;
            const auto& errVec = meanError[idx];
            for (float v: errVec) {
                s += std::fabs(v);
            }
            norms[idx] = s;
        }

        if (n == 0) { return norms; }

        // 计算 95 分位数 S_95
        std::vector<float> sorted = norms;
        const size_t idx95 = static_cast<size_t>(0.95f * static_cast<float>(n - 1));
        std::nth_element(sorted.begin(), sorted.begin() + idx95, sorted.end());
        const float S95 = sorted[idx95];

        // 归一化：\hat{S}_i = min(1, S_i / (S_95 + eps))
        std::vector<float> normalized(n, 0.0f);
        const float denom = S95 + eps;
        if (denom <= 0.0f) {
            // 极端情况下退化为全 0
            return normalized;
        }

#pragma omp parallel for
        for (long long i = 0; i < static_cast<long long>(n); ++i) {
            const size_t idx = static_cast<size_t>(i);
            float ratio = norms[idx] / denom;
            if (ratio > 1.0f) ratio = 1.0f;
            if (ratio < 0.0f) ratio = 0.0f;
            normalized[idx] = 1.0f - ratio;
        }

        return normalized;
    }

private:
    // 在给定的数据缓冲上计算指定点及其邻居之间的分量差值
    float GetDelta(const float* data, int idx, int dimIndex, int neighIndex,
                   const std::vector<igIndex>& neighborVerts) const {
        const float* idxData = data + static_cast<size_t>(idx) * m_ElementDim;
        const float* neighData = data + static_cast<size_t>(neighborVerts[neighIndex]) * m_ElementDim;

        return idxData[dimIndex] - neighData[dimIndex];

        //return data[idx * m_ElementDim + dimIndex] -
        //    data[neighborVerts[neighIndex] * m_ElementDim + dimIndex];
    }

    // Verificarlo 风格的随机扰动：在数值尺度下叠加高斯噪声（保留旧实现以便回退）
    /*
    static float VerificarloPerturb(float value) {
        const float magnitude = std::fabs(value);
        const float base = (magnitude > 0.0f ? magnitude : 1.0f);
        const float scale = base * 1e-6f; // 固定相对尺度扰动

        if (scale == 0.0f) {
            return value;
        }

        thread_local std::mt19937 generator(std::random_device{}());
        std::normal_distribution<float> distribution(0.0f, scale);

        return value + distribution(generator);
    }
    */

    // 使用 meshopt_quantizeFloat 进行量化扰动；N 表示保留的 mantissa 位数（0..23）
    static float MCAPerturb(float value, int N) {
        if (N < 0) N = 0;
        if (N > 23) N = 23;
        return meshopt_quantizeFloat(value, N);
    }

    Point MiddlePos(igIndex cellId) {
        Point p = {0.0f, 0.0f, 0.0f};
        int start, end;
        GetIdStartEnd(cellId, start, end);

        int numPoints = end - start;
        if (numPoints == 0) return p;

        float invNumPoints = 1.0f / numPoints;
        for (int i = start; i < end; i++) {
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

    Point GetPoint(igIndex pId) {
        const float* p = m_Points + pId * 3;
        return {p[0], p[1], p[2]};

        /*Point p = { 0.0f, 0.0f, 0.0f };
		p[0] = m_Points[pId * 3 + 0];
		p[1] = m_Points[pId * 3 + 1];
		p[2] = m_Points[pId * 3 + 2];
		return p;*/
    }

    Point GetDataPointPos(igIndex id) {
        if (m_dataType == DataType::AttachCell) {
            return MiddlePos(id);
        } else {
            return GetPoint(id);
        }
    }

    void GetNeighbourDataPointPos(igIndex id, std::vector<Point>& points, std::vector<igIndex>& neighborIndices) {
        points.reserve(128);
        neighborIndices.reserve(128);

        points.clear();
        neighborIndices.clear();

        switch (m_dataType) {
            case DataType::AttachCell: {
                auto neighNum = GetCellNeighbour(id, neighborIndices);
                points.resize(neighNum);
                for (int i = 0; i < neighNum; i++) { points[i] = MiddlePos(neighborIndices[i]); }
                break;
            }
            case DataType::Geom:
            case DataType::AttachPoint: {
                auto neighNum = GetPointRing(id, neighborIndices);
                points.resize(neighNum);
                for (int i = 0; i < neighNum; i++) { points[i] = GetPoint(neighborIndices[i]); }
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

    void GetIdStartEnd(igIndex cellId, int& start, int& end) {
        start = m_IsFixedCell ? cellId * m_FixedCellSize : m_Offset[cellId];
        end = m_IsFixedCell ? (cellId + 1) * m_FixedCellSize : m_Offset[cellId + 1];
    }

    int GetCellPointNum(igIndex cellId) {
        int start, end;
        GetIdStartEnd(cellId, start, end);

        return end - start;
    }

    int GetCellPointId(igIndex cellId, int index) {
        int start, end;
        GetIdStartEnd(cellId, start, end);

        return m_CellIds[start + index];
    }

    int GetCellNeighbour(igIndex cellId, std::vector<igIndex>& neighIndices) {
        std::unordered_set<igIndex> uniqueIndices;
        int pointNum = GetCellPointNum(cellId);

        for (int i = 0; i < pointNum; i++) {
            auto pid = GetCellPointId(cellId, i);
            for (int j = 0; j < m_Adj.counts[pid]; j++) { uniqueIndices.insert(m_Adj.data[m_Adj.offsets[pid] + j]); }
        }

        uniqueIndices.erase(cellId);
        for (igIndex pointId: uniqueIndices) { neighIndices.push_back(pointId); }

        return neighIndices.size();
    }

    int GetPointRing(igIndex pointId, std::vector<igIndex>& neighIndices) {
        std::unordered_set<igIndex> uniqueIndices;

        for (int i = 0; i < m_Adj.counts[pointId]; i++) {
            int cellId = m_Adj.data[m_Adj.offsets[pointId] + i];
            int pointNum = GetCellPointNum(cellId);

            for (int j = 0; j < pointNum; j++) { uniqueIndices.insert(GetCellPointId(cellId, j)); }
        }

        uniqueIndices.erase(pointId);
        for (igIndex pointId: uniqueIndices) { neighIndices.push_back(pointId); }

        return neighIndices.size();
    }

    template<typename Func>
    void ProgressParallelFor(int start, int end, float startProgress, float endProgress, Func&& process,
                             int numThreads = ThreadPool::GetDefaultThreadCount()) {
        int range = end - start;
        int chunkSize = range / numThreads;
        if (range < numThreads) {
            numThreads = range;
            chunkSize = 1;
        }
        // std::cout << "The number of threads uesd  is " << numThreads << '\n';
        std::vector<std::future<void>> futures;
        for (int i = 0; i < numThreads; ++i) {
            int chunkStart = start + i * chunkSize;
            int chunkEnd = (i == numThreads - 1) ? end : chunkStart + chunkSize;
            if (chunkStart == chunkEnd) continue;
            // 使用线程池提交任务
            //std::cout << chunkStart << " " << chunkEnd << " " << i << std::endl;
            futures.emplace_back(ThreadPool::Instance()->Commit([=]() { process(chunkStart, chunkEnd); }));
        }
        // 等待所有任务完成
        for (size_t i = 0; i < futures.size(); ++i) {
            futures[i].get();
            float progress =
                    startProgress + (static_cast<float>(i + 1) / futures.size()) * (endProgress - startProgress);
            UpdateProgress(progress);
        }
    }

    enum class DataType { AttachCell, AttachPoint, Geom };

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

    std::unique_ptr<MeshEncoderAdapter> m_adapter;
    std::unique_ptr<MeshCodecAdjacency> m_Adjacency;
    MeshCodecAdjacency::CellAdjacency m_Adj;

    // 当属性底层并非 float 时，转储为 float 缓冲以统一计算路径
    std::vector<float> m_AttrConverted;

    // 对于二级索引多面体网格，存储展开后的 volume->point 数据
    std::vector<igIndex> m_ExpandedCellIds;
    std::vector<unsigned int> m_ExpandedOffset;

    ProgressObserver* m_Progress{nullptr};

    void UpdateProgress(double p) {
        if (m_Progress) {
            m_Progress->UpdateProgress(p);
        } else {
            m_Progress = ProgressObserver::Instance();
        }
    }
};

IGAME_NAMESPACE_END
#endif