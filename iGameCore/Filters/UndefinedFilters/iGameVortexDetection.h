
#ifndef VortexDetection_h
#define VortexDetection_h

#include "Eigen/Dense"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <cmath>
#include <omp.h>
#include <string>
#if defined(LibTorch_ENABLE)
#undef slots
#include <torch/torch.h>
#include <torch/script.h>
#include <ATen/ATen.h>
#define slots Q_SLOTS
#endif

#include "StreamView/iGameStreamTracer.h"
#include "UndefinedFilters/iGameVortexFilter.h"
#include <nanoflann.hpp>
#include <vector>


IGAME_NAMESPACE_BEGIN
class VortexDetection : public Filter {
public:
    I_OBJECT(VortexDetection);
    static Pointer New() { return new VortexDetection; }

    bool Execute() override;
#if defined(LibTorch_ENABLE)
    bool DetectionVortexWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index,
                                        std::string name);
    bool DetectionVortexWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index,
                                       std::string name);

    std::vector<torch::Tensor> extractPatches(const torch::Tensor& tensor, int patch_size, int stride);

    torch::Tensor sigmoid(const torch::Tensor& x);

    torch::Tensor run_prediction_on_block(const torch::Tensor& grid_tensor, const std::string& model_path,
                                          torch::jit::script::Module& model);
    // torch::Tensor run_prediction_on_block(const torch::Tensor& grid_tensor,torch::jit::script::Module& model);

    std::vector<torch::Tensor> extract_patches(const torch::Tensor& padded, int patch_size, int stride);

    torch::Tensor _hann3d(int patch_size);

    std::tuple<torch::Tensor, int, int, int> pad_tensor(const torch::Tensor& grid_tensor, int patch_size);

    torch::Tensor extract_patches_gpu_batched(
    const torch::Tensor& padded, int patch_size, int stride);

    torch::Tensor padTensor(const torch::Tensor& tensor, int pad_z, int pad_y, int pad_x);
    Vector3f nearestVector(const Vector3f& pos, const std::vector<Vector3f>& gridPoints,
                           const std::vector<Vector3f>& gridVelocities, bool& inside);

    torch::Tensor gaussian_kernel1d(float sigma, int radius);

    torch::Tensor gaussian_kernel3d(float sigma, int radius);

    torch::Tensor gaussian_filter3d(torch::Tensor input, float sigma, int radius);

    torch::Tensor gaussian_weights(const torch::Tensor& dists, float sigma = 1.5f);

    double compute_percentile_edge_length_from_cells(const std::vector<Vector3f>& points,
                                                     const std::vector<Volume*>& cells, double percentile);
    double compute_percentile_cell_volume(const std::vector<Vector3f>& points,
                                                    const std::vector<Volume*>& cells,double percentile);

    std::tuple<torch::Tensor, Eigen::Vector3f, std::vector<float>> process_blocks(const std::vector<Vector3f>& gridPoints,
                                                              const std::vector<Vector3f>& gridVelocities,
                                                              const Vector3f& min_pos, const Vector3f& max_pos,
                                                              const std::string& model_path,
                                                              int split,int nx,int ny,int nz,VolumeMesh::Pointer volume_Mesh,
                                                              AttributeSet* attributeSet,
                                                              int curIndex);

    ArrayObject::Pointer AttributeCell2Point(CellArray::Pointer Cell, ArrayObject::Pointer OriArray,
                                                          size_t PointNum);

    std::vector<float> ComputePointQ(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet,
                                                            int curIndex);

    torch::Tensor knn_smooth_labels(std::vector<float> data_val, const torch::Tensor& prob_vol_1, // [nz, ny, nx]
                                    const Eigen::Vector3f& min_pos, const Eigen::Vector3f& global_step,
                                    const std::vector<Eigen::Vector3f>& query_points,
                                    int k = 8);

    void EvaluatePredictMetrics(ArrayObject::Pointer Attributes_gc,
                                                 const std::vector<float>& Predict);


#endif

protected:
    VortexDetection();
    ~VortexDetection() override = default;

    VolumeMesh::Pointer volume_Mesh{};
    SurfaceMesh::Pointer surface_Mesh{};
    AttributeSet::Pointer attributeSet{};
    iGameStreamTracer::Pointer streamTracer{};

    int curIndex{-1};
    int curDim{-1};
    std::string name{};
};

IGAME_NAMESPACE_END
#endif