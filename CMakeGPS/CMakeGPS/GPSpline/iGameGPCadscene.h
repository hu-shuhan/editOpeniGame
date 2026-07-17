#pragma once

#include <vector>

#include "Util/iGameGP_Macros.h"
#include "iGameGPSplinePatchSurface.h"
#include "iGameGPSplineSurface.h"

#include <GPHelperIO/iGameGP_Surface_Convert.h>

#include <glm/glm.hpp>

#include "iGameGPSurfaceControlPoints.h"
#include "iGameGPTessellationFactor.h"


GPSTART

class CadSceneGP {

public:
    struct MaterialSide {
        glm::vec4 ambient;
        glm::vec4 diffuse;
        glm::vec4 specular;
        glm::vec4 emissive;
    };

    struct Material {
        MaterialSide sides[2];
        uint32_t _pad[8 * 4];

        Material() : _pad{} { memset((void*) this, 0, sizeof(Material)); }
    };

    struct MatrixNode {
        glm::mat4 worldMatrix;
        glm::mat4 worldMatrixIT;
        glm::mat4 objectMatrix;
        glm::vec4 bboxMin;
        glm::vec4 bboxMax;
        glm::ivec3 _pad0;
        float winding;
        glm::vec4 color;
    };

    struct Object {
        int matrixIndex;
        int geometryIndex;
        size_t surfaceIndex;
        size_t p;
        size_t q;
    };

public:
    inline bool is_valid() { return !(host_gpsurfaces.empty() && host_patchsurfaces.empty()); }
    CadSceneGP() = default;
    ~CadSceneGP() { release(); }

    CadSceneGP(const CadSceneGP&) = delete;
    CadSceneGP& operator=(const CadSceneGP&) = delete;
    CadSceneGP(CadSceneGP&&) = delete;
    CadSceneGP& operator=(CadSceneGP&&) = delete;

    void release() noexcept;
    void init_cuda(uint8_t* vkDeviceUUID, size_t UUID_SIZE);

    std::vector<gpmesh::GPSplinePatchSurface>& init_scene(gpbezier::SurfaceConvertHelper& helper, int pq);

    void init_CUDA_map_mode(bool bCompact) { bCompactCUDAMapBuffer = bCompact; }
    inline size_t get_all_patch_num() const {
        if (host_patchsurfaces.empty()) {
            return 0;
        } else {
            auto [p, q] = host_patchsurfaces[0].get_max_p_q();

            return p * q * host_patchsurfaces.size();
        }
    }

    inline size_t get_patchsurface_num() const { return host_patchsurfaces.size(); }

    inline std::pair<size_t, size_t> get_patch_max_p_q() const {
        assert(host_patchsurfaces.size() > 0);
        return host_patchsurfaces[0].get_max_p_q();
    }

    bool bCompactCUDAMapBuffer = true;


    real_t* m_host_positionx_ptr = nullptr;
    real_t* m_host_normalx_ptr = nullptr;
    real_t* m_device_positionx_ptr = nullptr;
    real_t* m_device_normalx_ptr = nullptr;

    real_t* m_host_scalarx_ptr = nullptr;
    real_t* m_device_scalarx_ptr = nullptr;
    std::vector<real_t*> m_cuda_ptr_arr;
    std::vector<real_t*> m_cuda_normal_arr;
    std::vector<real_t*> m_cuda_scalar_arr;

public:
    std::vector<GPSplineSurface> host_gpsurfaces;

    std::vector<GPSplinePatchSurface> host_patchsurfaces;


    std::vector<uint16_t> iboDataCCW;
    std::vector<uint16_t> iboDataCW;

    std::vector<Material> m_materials;
    std::vector<MatrixNode> m_matrices;
    std::vector<Object> m_objects;

private:
    GPSplineSurface* device_ptr_gpsurfaces = nullptr;

    int m_threads = INVALID32;

    std::vector<void*> m_cuda_extern_mem_arr;

    size_t m_num_surface = INVALID64;

    GPSurfaceControlPoint* m_device_surface_control_points_arr = nullptr;
    GPSurfaceControlPoint* m_host_mapping_surface_control_points_arr = nullptr;

    GPSurfaceControlPoint* m_device_surface_control_points_extended_arr = nullptr;
    GPSurfaceControlPoint* m_host_mapping_surface_control_points_extended_arr = nullptr;

    GPSurfaceControlPoint* m_device_surface_scalar_points_arr = nullptr;
    GPSurfaceControlPoint* m_host_mapping_surface_scalar_points_arr = nullptr;

    GPSurfaceControlPoint* m_device_surface_scalar_points_extended_arr = nullptr;
    GPSurfaceControlPoint* m_host_mapping_surface_scalar_points_extended_arr = nullptr;

    std::vector<GPPatchData*> m_host_mapping_surface_patch_data_arr;
    GPPatchData** m_device_surface_patch_data_arr = nullptr;

    GPTessellationFactor m_tessellation_factor;

    real_t* m_device_delta_u = nullptr;
    real_t* m_device_delta_v = nullptr;

    real_t* m_host_delta_u = nullptr;
    real_t* m_host_delta_v = nullptr;

    size_t* m_device_p = nullptr;
    size_t* m_device_q = nullptr;

    size_t* m_host_p = nullptr;
    size_t* m_host_q = nullptr;

    std::vector<void*> m_cuda_streams;
    std::vector<void*> m_cuda_events;
};

GPEND