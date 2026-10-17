#include "iGameMeshVertexImportance.h"
#include "iGameMeshSaliency.h"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include "iGameSurfaceMesh.h"

void CalculateMeshVertexImportance(std::vector<FVector>& V,
    std::vector<int>& F, const std::vector<std::vector<float>>& A,
    bool useEnhancedNormalization, std::vector<double>& sharpness,
    std::vector<double>& attrFeature, std::vector<double>& importance) {
    size_t N = V.size();

    // 1. 计算顶点锐度 (NormalCurvature)
    MeshSaliencyCalculator saliencyCalculator(V, F);
    saliencyCalculator.Execute();

    // 对顶点锐度做高分位截断归一化和 gamma 增强, 提升中低值区的可见性
    sharpness.resize(N);
    for (size_t i = 0; i < N; ++i) {
        sharpness[i] = saliencyCalculator.NormalCurvature[i];
    }

    const double sharpnessClipQuantile = 1;
    const double sharpnessGamma = 1;
    std::vector<double> sortedSharpness = sharpness;
    std::sort(sortedSharpness.begin(), sortedSharpness.end());

    double sMin = sortedSharpness.empty() ? 0.0 : sortedSharpness.front();
    size_t sharpnessQuantileIndex = sortedSharpness.empty()
                                            ? 0
                                            : static_cast<size_t>(
                                                      std::floor(sharpnessClipQuantile * (sortedSharpness.size() - 1)));
    double sMax = sortedSharpness.empty() ? 0.0 : sortedSharpness[sharpnessQuantileIndex];

    if (sMax - sMin > 1e-9) {
        for (size_t i = 0; i < N; ++i) {
            double clipped = std::min(std::max(sharpness[i], sMin), sMax);
            double normalized = (clipped - sMin) / (sMax - sMin);
            sharpness[i] = std::pow(normalized, sharpnessGamma);
        }
    } else {
        for (size_t i = 0; i < N; ++i) sharpness[i] = 0.0;
    }

    // 2. 按 chap03.tex 中（3）计算多属性的局部联合变化强度
    int D = (int)A.size();
    attrFeature.assign(N, 0.0);
    if (D > 0) {
        std::vector<std::vector<int>> adjacency(N);
        std::vector<std::vector<int>> vertexFaces(N);
        int faceCount = (int)F.size() / 3;
        for (int fi = 0; fi < faceCount; ++fi) {
            int i0 = F[fi * 3 + 0];
            int i1 = F[fi * 3 + 1];
            int i2 = F[fi * 3 + 2];

            adjacency[i0].push_back(i1);
            adjacency[i0].push_back(i2);
            adjacency[i1].push_back(i0);
            adjacency[i1].push_back(i2);
            adjacency[i2].push_back(i0);
            adjacency[i2].push_back(i1);

            vertexFaces[i0].push_back(fi);
            vertexFaces[i1].push_back(fi);
            vertexFaces[i2].push_back(fi);
        }

        for (size_t i = 0; i < N; ++i) {
            auto& neighbors = adjacency[i];
            if (neighbors.empty()) continue;

            std::sort(neighbors.begin(), neighbors.end());
            neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
            if (neighbors.empty()) continue;

            const double invNeighborCount = 1.0 / static_cast<double>(neighbors.size());
            std::vector<double> mean(D, 0.0);
            for (int neighbor : neighbors) {
                for (int d = 0; d < D; ++d) {
                    mean[d] += A[d][neighbor];
                }
            }
            for (int d = 0; d < D; ++d) {
                mean[d] *= invNeighborCount;
            }

            std::vector<double> covariance(static_cast<size_t>(D) * D, 0.0);
            std::vector<double> diff(D, 0.0);
            for (int neighbor : neighbors) {
                for (int d = 0; d < D; ++d) {
                    diff[d] = A[d][neighbor] - mean[d];
                }
                for (int p = 0; p < D; ++p) {
                    for (int q = 0; q < D; ++q) {
                        covariance[static_cast<size_t>(p) * D + q] += diff[p] * diff[q];
                    }
                }
            }

            double frobeniusNormSq = 0.0;
            for (double& value : covariance) {
                value *= invNeighborCount;
                frobeniusNormSq += value * value;
            }
            attrFeature[i] = std::sqrt(frobeniusNormSq);
        }

        if (useEnhancedNormalization) {
            const int smoothIterations = 5;
            const double mu = 0.5;
            const double eta = 1e-4;
            std::vector<double> smoothedAttrFeature = attrFeature;
            std::vector<double> delta(N, 0.0);

            for (int it = 0; it < smoothIterations; ++it) {
                std::fill(delta.begin(), delta.end(), 0.0);

                for (size_t i = 0; i < N; ++i) {
                    const auto& neighbors = adjacency[i];
                    if (neighbors.empty()) continue;

                    double Wi = 0.0;
                    double accum = 0.0;

                    for (int neighbor : neighbors) {
                        double cotAlpha = 0.0;
                        double cotBeta = 0.0;
                        int foundCount = 0;

                        for (int faceId : vertexFaces[i]) {
                            int f0 = F[faceId * 3 + 0];
                            int f1 = F[faceId * 3 + 1];
                            int f2 = F[faceId * 3 + 2];

                            int opposite = -1;
                            if ((f0 == (int)i && f1 == neighbor) || (f1 == (int)i && f0 == neighbor)) opposite = f2;
                            else if ((f1 == (int)i && f2 == neighbor) || (f2 == (int)i && f1 == neighbor)) opposite = f0;
                            else if ((f2 == (int)i && f0 == neighbor) || (f0 == (int)i && f2 == neighbor)) opposite = f1;

                            if (opposite < 0) continue;

                            const FVector& vi = V[i];
                            const FVector& vj = V[neighbor];
                            const FVector& vo = V[opposite];

                            FVector e1 = vi - vo;
                            FVector e2 = vj - vo;
                            double dot = e1.Dot(e2);
                            double crossLen = e1.Cross(e2).Length();
                            if (crossLen <= 1e-10) continue;

                            double cotVal = dot / crossLen;
                            if (foundCount == 0) cotAlpha = cotVal;
                            else cotBeta = cotVal;
                            foundCount++;
                            if (foundCount == 2) break;
                        }

                        if (foundCount == 0) continue;

                        double cotSum = cotAlpha + cotBeta;
                        double wij = (cotSum > 0.0) ? (0.5 * cotSum) : eta;
                        Wi += wij;
                        accum += wij * (smoothedAttrFeature[neighbor] - smoothedAttrFeature[i]);
                    }

                    if (Wi > 0.0) {
                        delta[i] = accum / Wi;
                    }
                }

                for (size_t i = 0; i < N; ++i) {
                    smoothedAttrFeature[i] = smoothedAttrFeature[i] + (1.0 - mu) * delta[i];
                }
            }

            for (size_t i = 0; i < N; ++i) {
                smoothedAttrFeature[i] = std::max(smoothedAttrFeature[i], attrFeature[i]);
            }
            attrFeature.swap(smoothedAttrFeature);
        }

        // 关闭增强时使用论文描述的普通 [0,1] 归一化。
        const double attrClipQuantile = useEnhancedNormalization ? 0.99 : 1.0;
        const double attrGamma = useEnhancedNormalization ? 0.8 : 1.0;
        std::vector<double> sortedAttrFeature = attrFeature;
        std::sort(sortedAttrFeature.begin(), sortedAttrFeature.end());

        double lMin = sortedAttrFeature.empty() ? 0.0 : sortedAttrFeature.front();
        size_t quantileIndex = sortedAttrFeature.empty()
                                       ? 0
                                       : static_cast<size_t>(std::floor(attrClipQuantile * (sortedAttrFeature.size() - 1)));
        double lMax = sortedAttrFeature.empty() ? 0.0 : sortedAttrFeature[quantileIndex];

        if (lMax - lMin > 1e-9) {
            for (size_t i = 0; i < N; ++i) {
                double clipped = std::min(std::max(attrFeature[i], lMin), lMax);
                double normalized = (clipped - lMin) / (lMax - lMin);
                attrFeature[i] = std::pow(normalized, attrGamma);
            }
        } else {
            for (size_t i = 0; i < N; ++i) attrFeature[i] = 0.0;
        }
    }

    // 3. 综合顶点重要度: α * sharpness + (1 - α) * attrFeature
    double alpha = 0.5;
    importance.resize(N);
    for (size_t i = 0; i < N; ++i) {
        // importance[i] = std::max(sharpness[i] , attrFeature[i]);
        importance[i] = (1 - alpha) * sharpness[i] + alpha * attrFeature[i];
    }

}

void CalculateSurfaceMeshVertexImportance(iGame::SurfaceMesh* mesh,
    std::vector<FVector>& vertices, std::vector<int>& triangles,
    bool enhanced, std::vector<double>& importance) {
    using namespace iGame;
    std::vector<std::vector<float>> normalizedAttributes;
    // 提取并归一化属性
    for (int i = 0; i < mesh->GetAttributeSet()->GetNumberOfAttributes(); i++) {
        auto& attr = mesh->GetAttributeSet()->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer || attr.attachmentType != IG_POINT ||
            attr.pointer->GetNumberOfElements() != mesh->GetNumberOfPoints()) continue;
        const auto& name = attr.pointer->GetName();
        if (name == "VertexSharpness" || name == "AttrFeature" || name == "VertexImportance") continue;
        int dim = attr.pointer->GetDimension();
        if (dim == 1) {
            double val_max = -FLT_MAX;
            double val_min = FLT_MAX;
            for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
                double val = attr.pointer->GetValue(j);
                val_max = val_max < val ? val : val_max;
                val_min = val_min > val ? val : val_min;
            }
            if (val_min == val_max) {
                std::vector<float> data(mesh->GetNumberOfPoints(), 0.f);
                normalizedAttributes.push_back(std::move(data));

            } else {
                std::vector<float> data;
                for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
                    data.push_back((attr.pointer->GetValue(j) - val_min) / (val_max - val_min));
                }
                normalizedAttributes.push_back(std::move(data));

            }
        } else {
            double max_norm = -FLT_MAX;
            for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
                double norm = 0.0;
                for (int d = 0; d < dim; d++) {
                    double val = attr.pointer->GetValue(j * dim + d);
                    norm += val * val;
                }
                norm = std::sqrt(norm);
                max_norm = max_norm < norm ? norm : max_norm;
            }
            if (max_norm < 1e-9) {
                for (int d = 0; d < dim; d++) {
                    std::vector<float> data(mesh->GetNumberOfPoints(), 0.f);
                    normalizedAttributes.push_back(std::move(data));

                }
            } else {
                for (int d = 0; d < dim; d++) {
                    std::vector<float> data;
                    for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
                        data.push_back(attr.pointer->GetValue(j * dim + d) / max_norm);
                    }
                    normalizedAttributes.push_back(std::move(data));

                }
            }
        }
    }

    std::vector<double> sharpness, feature;
    CalculateMeshVertexImportance(vertices, triangles, normalizedAttributes,
                                  enhanced, sharpness, feature, importance);
}
