//
// Created by Sumzeek on 05/02/2024.
//

#ifndef IGAMEVIEW_LITE_BUILDER_H
#define IGAMEVIEW_LITE_BUILDER_H

#include "Mesh.h"

class Builder{
public:
    explicit Builder(std::vector<float>* _vertices):
        vertices(_vertices){};

    void build(const std::string& file_name);
    void getGPUdata(int mode);


    inline uint32 getTriCount() {
        // 是原始的三角个数，简化lod之后可能会变化
        return myMesh.indices.size() / 3;
    }
    inline std::vector<vec3>& getBoundPointer(){
        return this->myMesh.positions;
    }
private:
    Mesh myMesh; //原始数据
    VirtualMesh myVirtualMesh; //Lod数据
    std::vector<float>* vertices; //GPU绘制数据
};

#endif //IGAMEVIEW_LITE_BUILDER_H
