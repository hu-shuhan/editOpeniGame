//
// Created by Sumzeek on 25/02/2024.
//

#include "Mesh.h"
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <array>

bool Mesh::ReadObjFile(const std::string& file_name) {
    if (file_name.empty()) {
        return false;
    }

    FILE* file_ = nullptr;
    if ((file_ = fopen(file_name.c_str(), "rb")), file_) {
        std::vector<char> buffer;

        auto GetFileSize = [] (FILE* file_) -> size_t {
            if (fseek(file_, SEEK_SET, SEEK_END) != 0) {
                return 0;
            }
            const size_t file_size = static_cast<size_t>(_ftelli64(file_));
            rewind(file_);
            return file_size;
        };
        auto ReadToBuffer = [&] (FILE* file_, std::vector<char>& buffer) -> bool {
            buffer.clear();
            const size_t file_size = GetFileSize(file_);
            if (file_size == 0) {
                return false;
            }

            buffer.resize(file_size);
            return fread(buffer.data(), 1, file_size, file_) == file_size;
        };

        if (ReadToBuffer(file_, buffer))
        {
            int numVertices = 0;
            int numFaces = 0;

            const std::string targetString = "\nf ";
            auto face_pos = std::search(buffer.begin(), buffer.end(), targetString.begin(), targetString.end());
            int idx = face_pos - buffer.begin();

            const char* v_begin = buffer.data();
            const char* v_end = v_begin + idx;
            const char* f_begin = v_end + 1;
            const char* f_end = v_begin + buffer.size();

            auto& vertices = this->positions;
            auto& faces = this->indices;

            // 找顶点
            while (v_begin < v_end) {
                // 查找行结束位置
                const char* lineEnd = strchr(v_begin, '\n');
                if (!lineEnd) {
                    lineEnd = v_end;
                }

                std::string line(v_begin, lineEnd);

                // 更新指针位置
                v_begin = lineEnd + 1;

                if (line.size() < 2) {
                    continue;
                }

                if (line[0] == 'v' && line[1] == ' ')
                {
                    std::array<float, 3> input_float;
                    if (sscanf(line.data() + 2, "%f%f%f", &input_float[0], &input_float[1], &input_float[2]) == 3)
                    {
                        ++numVertices;
                        vertices.push_back(vec3{ input_float[0], input_float[1], input_float[2] });
                    }
                    else
                    {
                        assert(false);
                    }
                }
            }

            // 找面
            while (f_begin < f_end) {
                // 查找行结束位置
                const char* lineEnd = strchr(f_begin, '\n');
                if (!lineEnd) {
                    lineEnd = f_end;
                }

                std::string line(f_begin, lineEnd);

                // 更新指针位置
                f_begin = lineEnd + 1;

                if (line.size() < 2) {
                    continue;
                }

                if (line[0] == 'f' && line[1] == ' ')
                {
                    std::array<int, 3> input_int;
                    if (sscanf(line.data() + 2, "%d%d%d", &input_int[0], &input_int[1], &input_int[2]) == 3)
                    {
                        ++numFaces;
                        faces.push_back(static_cast<uint32>(input_int[0] - 1));
                        faces.push_back(static_cast<uint32>(input_int[1] - 1));
                        faces.push_back(static_cast<uint32>(input_int[2] - 1));
                    }
                    else
                    {
                        assert(false);
                    }
                }
            }
            std::cout << "Read file success!" << std::endl;
            return true;
        }
    }
    else {
        std::cout << "Read file failure!" << std::endl;
        return false;
    }

}

//void VirtualMesh::build(Mesh& mesh) {
//    Timer timer;
//
//    print("\n# begin building virtual mesh\n\n");
//
//    auto& [pos,idx]=mesh;
//    timer.reset();
//    print("fixup mesh: ");
//
//    //使用简化器并设置大于三角形数目的目标，去除重复点与三角形
//    MeshSimplifier simplifier(pos.data(),pos.size(),idx.data(),idx.size());
//    simplifier.simplify(idx.size());
//    pos.resize(simplifier.remaining_num_vert());
//    idx.resize(simplifier.remaining_num_tri()*3);
//
//    print("{} us\n",timer.us());
//    print("verts: {}, tris: {}\n\n",pos.size(),idx.size()/3);
//
//    timer.reset();
//    print("clustering triangles: ");
//
//    //将三角形分组为三角形簇
//    cluster_triangles(pos,idx,clusters);
//    print("{} us\n\n",timer.us());
//
//    u32 level_offset=0,mip_level=0;
//
//    print("begin building cluster DAG\n\n");
//    while(1){
//        print("### level {} ###\n",mip_level);
//        print("num clusters: {}\n",clusters.size()-level_offset);
//        log_cluster_size(clusters.data(),level_offset,clusters.size());
//
//        u32 num_level_clusters=clusters.size()-level_offset;
//        if(num_level_clusters<=1) break;
//
//        u32 prev_cluster_num=clusters.size();
//        u32 prev_group_num=cluster_groups.size();
//
//        //将簇分组
//        timer.reset();
//        print("groupinging clusters: ");
//        group_clusters(
//                clusters,
//                level_offset,
//                num_level_clusters,
//                cluster_groups,
//                mip_level
//        );
//        print("{} us\n",timer.us());
//        print("num groups: {}\n",cluster_groups.size()-prev_group_num);
//        log_group_size(cluster_groups.data(),prev_group_num,cluster_groups.size());
//
//        //将组内簇合并并简化，生成上一级簇
//        timer.reset();
//        print("building parent clusters: ");
//        for(u32 i=prev_group_num;i<cluster_groups.size();i++){
//            build_parent_clusters(cluster_groups[i],clusters);
//        }
//        print("{} us\n",timer.us());
//
//        level_offset=prev_cluster_num;
//        mip_level++;
//
//        print("\n");
//    }
//    num_mip_level=mip_level+1;
//
//    print("end building cluster DAG\n");
//    print("total clusters: {}\n\n",clusters.size());
//
//    print("# end build virtual mesh\n\n");
//}
