//
// Created by Sumzeek on 7/1/2024.
//

#pragma once

#include "OpenGL/GLShader.h"
#include "iGameCamera.h"
#include "iGameDataObject.h"
#include "iGameDrawObject.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

enum class ShaderType {
    BLINNPHONG = 0,
    PBR,
    NOLIGHT,
    PURECOLOR,
    SINGLEPASSWIREFRAME,
    TRANSPARENCYLINK,
    TRANSPARENCYSORT,
    VOLUMERENDERINGLINK,
    VOLUMERENDERINGSORT,
    AXES,
    FONT,
    ATTACHMENTRESOLVE,
    DEPTHREDUCE,
    MESHLETCULL,
    SCREEN,
    FXAA,
    CULLINGPHASE1,
    CULLINGPHASE2
};

class Scene;

class ShaderManager : public Object {
public:
    I_OBJECT(ShaderManager)
    static Pointer New() { return new ShaderManager; }

    struct CameraDataBuffer {
        alignas(16) igm::vec3 camera_position;
        alignas(4) int isOrtho;
        alignas(16) igm::mat4 view;
        alignas(16) igm::mat4 proj;
        alignas(16) igm::mat4 proj_view; // proj * view
    };

    struct ObjectDataBuffer {
        alignas(4) float transparent;
        alignas(16) igm::mat4 model;
        alignas(16) igm::mat4 normal; // transpose(inverse(model))
        alignas(16) igm::vec4 sphereBounds;
    };

    struct UniformBufferObjectBuffer {
        alignas(4) int useColor{0};
        alignas(4) int useNormalSmooth{0};
    };

    struct CullDataBuffer {
        alignas(16) igm::mat4 view_model;
        alignas(4) float P00, P11, zNear,
                zFar; // symmetric projection parameters
        alignas(16) igm::vec4
                frustum; // data for left/right/top/bottom frustum planes
        alignas(4) unsigned int pyramidWidth,
                pyramidHeight; // depth pyramid size in texels
    };

    GLShaderProgram::Pointer GetShader(ShaderType type);
    bool HasShader(ShaderType type);
    void UseShader(ShaderType type);

    void MapBufferBlock();

    void UpdateCameraBlock(Camera::Pointer camera);
    void UpdateCameraBlock(CameraDataBuffer buffer);

    void UpdateObjectBlock(DataObject::Pointer obj, igm::mat4 model);
    void UpdateObjectBlock(ObjectDataBuffer buffer);

    void UpdateUBOBlock(DataObject::Pointer obj);
    void UpdateUBOBlock(UniformBufferObjectBuffer buffer);

    void UpdateCullDataBuffer(Camera::Pointer camera, igm::mat4 model,
                              unsigned int depthPyramidWidth,
                              unsigned int depthPyramidHeight);
    void UpdateCullDataBuffer(CullDataBuffer buffer);
    GLBuffer::Pointer GetCullDataBuffer();

protected:
    ShaderManager();
    ~ShaderManager() override;

    GLShaderProgram::Pointer GetShaderWithType(ShaderType type);
    void SetShader(ShaderType type, GLShaderProgram::Pointer sp);
    GLShaderProgram::Pointer GenShader(ShaderType type);

    std::map<ShaderType, GLShaderProgram::Pointer> m_ShaderPrograms;

    GLBuffer::Pointer m_CameraDataBlock;
    GLBuffer::Pointer m_ObjectDataBlock;
    GLBuffer::Pointer m_UBOBlock;
    GLBuffer::Pointer m_CullDataBuffer;
};

IGAME_NAMESPACE_END
