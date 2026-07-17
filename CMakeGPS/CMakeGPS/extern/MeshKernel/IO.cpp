#pragma once
#include "IO.h"

MeshKernel::SurfaceMesh MeshKernel::IO::ReadObjFile(const std::string& _InputFile, int& sides_num) {
    std::ifstream inputfile(_InputFile, std::ios::in);
    std::vector<iGameVertex> vertices;
    std::vector<std::vector<iGameVertexHandle>> faces;
    std::vector<std::vector<double>> normals;
    std::vector<std::vector<double>> uvs;
    std::unordered_map<int, int> V2N;   
    std::unordered_map<int, int> V2T;   
    std::string line;

    std::cout << "Reading " << _InputFile << " File" << std::endl;
    while (inputfile) {
        line.clear();
        getline(inputfile, line);
        if (line[0] == '#') {
            continue; 
        }
        std::stringstream linestream;
        linestream.str(line);

        std::string flag;
        linestream >> flag;
        if (flag == "v") {
            double x, y, z;
            linestream >> x >> y >> z;
            vertices.push_back(iGameVertex(x, y, z));
        } else if (flag == "f") {
            std::vector<std::string> vex;
            std::string tmp;
            while (linestream >> tmp) vex.push_back(tmp);
            auto n = vex.size();
            sides_num = n;
            std::vector<iGameVertexHandle> face(n);
            for (size_t i = 0; i < n; i++) {
                size_t idx = 0;
                while (idx < vex[i].length() && std::isdigit(vex[i][idx])) idx++;
                int vh = std::stoi(vex[i].substr(0, idx)) - 1;     
                face[i] = (iGameVertexHandle)(vh);
            }
            faces.push_back(face);
        } else if (flag == "vt") {
            double u, v;
            linestream >> u >> v;
            uvs.push_back({ u, v });
        } else if (flag == "vn") {
            double x, y, z;
            linestream >> x >> y >> z;
            normals.push_back({ x, y, z });
        }
    }
    if (!normals.empty()) {
        int ncnt = normals.size();
        for (int i = 0; i < vertices.size(); ++i) {
            int nidx = V2N[i];
            assert(nidx >= 0 && nidx < ncnt);
            vertices[i].setNormal(normals[nidx]);
        }
    }

    auto mesh = SurfaceMesh(vertices, faces);
    inputfile.close();
    return mesh;
}




std::string MeshKernel::IO::WriteOffString(const SurfaceMesh& _mesh) {
    std::stringstream ret;
    ReOrderiGameVertexHandle(_mesh);
    ret << "OFF" << std::endl;
    ret << _mesh.allvertices().size() << " " << _mesh.allfaces().size() << " " << _mesh.alledges().size() << std::endl;
    for (iGameVertexHandle vh : reorderedvh_) {
        iGameVertex v(_mesh.vertices(vh));
        ret << v.x() << " " << v.y() << " " << v.z() << std::endl;
    }
    auto allf = _mesh.allfaces();
    for (auto f : allf) {
        ret << f.second.size();
        for (int i = 0; i < f.second.size(); ++i) {
            ret << " " << newvh_[f.second.vh(i)];
        }
        ret << std::endl;
    }
    return std::string(ret.str());
}
MeshKernel::SurfaceMesh MeshKernel::IO::ReadOffFile(const std::string& _InputFile, int& sides_num) {
    std::ifstream inputfile(_InputFile, std::ios::in);
    std::vector<iGameVertex> vertices;
    std::vector<std::vector<iGameVertexHandle>> faces;
    std::vector<std::vector<double>> normals;
    std::vector<std::vector<double>> uvs;
    std::unordered_map<int, int> V2N;   
    std::unordered_map<int, int> V2T;   
    std::string line;
    int v_size, f_size, e_size;

    std::cout << "Reading " << _InputFile << " File" << std::endl;
    line.clear();
    getline(inputfile, line);
    std::stringstream linestream;

    if (line == "OFF") {
        line.clear();
        getline(inputfile, line);
        linestream.str(line);
        linestream >> v_size >> f_size >> e_size;
    }
    for (int i = 0; i < v_size; i++) {
        line.clear();
        getline(inputfile, line);
        std::stringstream linestream;
        linestream.str(line);
        double x, y, z;
        linestream >> x >> y >> z;
        vertices.push_back(iGameVertex(x, y, z));
    }
    for (int i = 0; i < f_size; i++) {
        line.clear();
        getline(inputfile, line);
        std::stringstream linestream;
        linestream.str(line);
        int v;
        linestream >> v;
        std::vector<iGameVertexHandle> face(v);
        sides_num = v;
        for (int i = 0; i < v; i++) {
            int temp;
            linestream >> temp;
            face[i] = (iGameVertexHandle)(temp);
        }
        faces.push_back(face);
    }
    printf("read file success, fcnt: %d, vcnt: %d, vtcnt: %d, vncnt: %d\n", faces.size(), vertices.size(), uvs.size(), normals.size());
    if (!normals.empty()) {
        int ncnt = normals.size();
        for (int i = 0; i < vertices.size(); ++i) {
            int nidx = V2N[i];
            assert(nidx >= 0 && nidx < ncnt);
            vertices[i].setNormal(normals[nidx]);
        }
    }

    auto mesh = SurfaceMesh(vertices, faces);
    inputfile.close();
    return mesh;
}



bool MeshKernel::IO::WriteObjFile(const SurfaceMesh& _mesh, const std::string& _OutputFile) {
    std::ofstream outputfile(_OutputFile, std::ios::out);
    ReOrderiGameVertexHandle(_mesh);
    for (iGameVertexHandle vh : reorderedvh_) {
        iGameVertex v(_mesh.vertices(vh));
        outputfile << "v " << v.x() << " " << v.y() << " " << v.z() << std::endl;
    }
    auto allf = _mesh.allfaces();
    for (auto f : allf) {
        outputfile << "f";
        for (int i = 0; i < f.second.size(); ++i) {
            outputfile << " " << newvh_[f.second.vh(i)] + 1;
        }
        outputfile << std::endl;
    }
    outputfile.close();
    return true;
}

bool MeshKernel::IO::WriteOffFile(const SurfaceMesh& _mesh, const std::string& _OutputFile) {
    std::ofstream outputfile(_OutputFile, std::ios::out);
    ReOrderiGameVertexHandle(_mesh);
    outputfile << "OFF" << std::endl;
    outputfile << _mesh.allvertices().size() << " " << _mesh.allfaces().size() << " " << _mesh.alledges().size() << std::endl;
    for (iGameVertexHandle vh : reorderedvh_) {
        iGameVertex v(_mesh.vertices(vh));
        outputfile << v.x() << " " << v.y() << " " << v.z() << std::endl;
    }
    auto allf = _mesh.allfaces();
    for (auto f : allf) {
        outputfile << f.second.size();
        for (int i = 0; i < f.second.size(); ++i) {
            outputfile << " " << newvh_[f.second.vh(i)];
        }
        outputfile << std::endl;
    }
    outputfile.close();
    return true;
}

std::vector<std::string> MeshKernel::IO::SplitFileName(const std::string& fileName)
{
    std::vector<std::string> s;
    s.resize(3);
    if (fileName.size()) {
        int idot = (int)fileName.find_last_of('.');
        int islash = (int)fileName.find_last_of("/\\");
        if (idot == (int)std::string::npos) idot = -1;
        if (islash == (int)std::string::npos) islash = -1;
        if (idot > 0) s[2] = fileName.substr(idot);
        if (islash > 0) s[0] = fileName.substr(0, islash + 1);
        s[1] =
                fileName.substr(s[0].size(), fileName.size() - s[0].size() - s[2].size());
    }
    return s;
}

void MeshKernel::IO::ReOrderiGameVertexHandle(const SurfaceMesh& _mesh) {
    auto allv = _mesh.allvertices();
    int idx = 0;
    for (auto v : allv) {
        reorderedvh_.push_back(v.first);
        newvh_[v.first] = idx++;
    }
}


void MeshKernel::IO::iGameWriteVolumeFile(const VolumeMesh& _mesh, const std::string& _filename) {


    std::ofstream off(_filename.c_str(), std::ios::out);

    if (!off.good()) {
        std::cerr << "Error: Could not open file " << _filename << " for writing!" << std::endl;
        off.close();
        return;
    }

    off << "MeshVersionFormatted 1" << std::endl;
    off << "Dimension 3" << std::endl;
    uint64_t n_vertices(_mesh.vsize());
    off << "Vertices" << std::endl;
    off << n_vertices << std::endl;

    for (uint64_t v_it = 0; v_it < n_vertices; ++v_it) {
        auto& v = _mesh.vertices((MeshKernel::iGameVertexHandle)v_it);
        off <<  v.x() << " "
            <<  v.y() << " "
            <<  v.z() << " "
            << "-1" << std::endl;
    }
    uint64_t n_faces(_mesh.fsize());
    uint64_t n_cells(_mesh.csize());
    off << "Hexahedra" << std::endl;
    off << n_cells << std::endl;

    for (auto& cp : _mesh.allcells()) {
        auto vhs = cp.second.getVertexHandle();
        for (auto& vh : vhs) {
            off << (int)vh + 1 << " ";
        }
        off << "1" << std::endl;
    }

    off << "End" << std::endl;
    off.close();

}

MeshKernel::VolumeMesh MeshKernel::IO::iGameReadVolumeFile_TopOptMeshGeneration(const std::string& _InputFile) {
    std::ifstream inputfile(_InputFile, std::ios::in);
    std::vector<iGameVertex> vertices;
    std::vector<std::vector<iGameVertexHandle>> cells;
    std::string line;
    std::cout << "ok" << std::endl;
    VolumeMesh mesh;
    int cnt = 0;
    std::cout << cnt << std::endl;
    while (inputfile) {
        std::cout << cnt++ << std::endl;
        line.clear();
        getline(inputfile, line);
        std::stringstream linestream;
        linestream.str(line);
        std::vector<iGameVertexHandle> vhs;
        for (int i = 0; i < 8; ++i) {
            double x, y, z;
            linestream >> x >> y >> z;
            iGameVertex v(x, y, z);
            auto vh = mesh.AddVertex(iGameVertex(v.x(), v.y(), v.z()));
            vhs.push_back(vh);
        }
        mesh.AddCell(vhs);
    }

    return mesh;
}

MeshKernel::VolumeMesh MeshKernel::IO::iGameReadVolumeFile(const std::string& _InputFile) {

    std::ifstream inputfile(_InputFile, std::ios::in);
    std::vector<iGameVertex> vertices;
    std::vector<std::vector<iGameVertexHandle>> cells;
    std::string line;
    std::vector<int> invalid_vhs;  

    while (inputfile) {
        line.clear();
        getline(inputfile, line); 
        if (line[0] == '#') continue;  

        else if (line[0] == 'V') {
            line.clear();
            getline(inputfile, line); 
            int num = stoi(line); 
            for (int i = 0; i < num; ++i) {
                line.clear();
                getline(inputfile, line);
                std::stringstream linestream;
                linestream.str(line);
                double x, y, z;
                int tag = -1;
                linestream >> x >> y >> z >> tag;
                iGameVertex vv(x, y, z);
                vertices.push_back(vv);
                if (tag == 0)
                    invalid_vhs.push_back(vertices.size() - 1);
            }
        }

        else if (line[0] == 'H') {
            line.clear();
            getline(inputfile, line);
            std::stringstream linestream;
            int num = stoi(line);
            for (int i = 0; i < num; i++) {
                line.clear();
                getline(inputfile, line);
                linestream.str(line);
                std::vector<iGameVertexHandle> cellH;
                int n = 0;
                for (auto& c : line) if (c == ' ') n++;
                while (n--) {
                    int t;
                    linestream >> t;
                    cellH.push_back(iGameVertexHandle(t - 1));   
                }
                cells.push_back(cellH);
            }
        }
    }
    inputfile.close();
    auto volume_mesh = VolumeMesh(vertices, cells);
    return volume_mesh;
}

MeshKernel::TetMesh MeshKernel::IO::iGameReadTetMeshFile(const std::string& _InputFile) {

    std::ifstream inputfile(_InputFile, std::ios::in);
    std::vector<iGameVertex> vertices;
    std::vector<std::vector<iGameVertexHandle>> cells;
    std::string line;
    std::cout << "Is Reading : " << _InputFile << " File." << std::endl;

    std::vector<int> invalid_vhs;  

    while (inputfile) {
        line.clear();
        getline(inputfile, line); 
        if (line[0] == '#') continue;  

        else if (line[0] == 'V') {
            line.clear();
            getline(inputfile, line); 
            int num = stoi(line); 
            std::cout << "该模型文件中点的个数为 : " << num << std::endl;
            for (int i = 0; i < num; ++i) {
                line.clear();
                getline(inputfile, line);
                std::stringstream linestream;
                linestream.str(line);
                double x, y, z;
                int tag = -1;
                linestream >> x >> y >> z >> tag;
                iGameVertex vv(x, y, z);
                vertices.push_back(vv);
            }
        }

        else if (line[0] == 'T') {
            line.clear();
            getline(inputfile, line);
            std::stringstream linestream;
            int num = stoi(line);
            std::cout << "该模型文件中体的个数为 : " << num << std::endl;
            for (int i = 0; i < num; i++) {
                line.clear();
                getline(inputfile, line);
                linestream.str(line);
                std::vector<iGameVertexHandle> cellH;
                int n = 0;
                n = 4;
                while (n--) {
                    int t;
                    linestream >> t;
                    cellH.push_back(iGameVertexHandle(t - 1));   
                }
                cells.push_back(cellH);
            }
        }
    }
    inputfile.close();
    auto volume_mesh = TetMesh(vertices, cells);
    std::cout << "the number of vertices is " << volume_mesh.VertexSize() << std::endl;
    std::cout << "the number of cells is " << volume_mesh.CellSize() << std::endl;
    return volume_mesh;

}

MeshKernel::TetMesh MeshKernel::IO::ReadMeshFileFromStr(const std::string& data) {
    std::stringstream ss(data);
    std::vector<iGameVertex> vertices;
    std::vector<std::vector<iGameVertexHandle>> surface_faces;
    std::vector<std::vector<std::vector<iGameVertexHandle>>> cells;
    std::vector<std::vector<iGameVertexHandle>> faces;
    std::string str;
    enum State
    {
        USELESS = 1, VERTEX, FACE, TET, EDGE
    }state = USELESS;
    while (std::getline(ss, str)) {
        int len = str.length();
        std::vector<std::string>info;
        std::string s;
        for (int i = 0; i < len; i++) {
            if (str[i] == '#')break;
            if (str[i] == ' ') {
                if (s.length() > 0)
                    info.push_back(s);
                s = "";
            }             else {
                s.push_back(str[i]);
            }
        }
        if (s.length() > 0)
            info.push_back(s);
        if (info.size() == 0)continue;
        if (info[0] == "MeshVersionFormatted") {
            state = USELESS;
        }         else if (info[0] == "Dimension") {
            std::getline(ss, str);
            state = USELESS;
        }         else if (info[0] == "Vertices") {
            std::getline(ss, str);
            state = VERTEX;
        }         else if (info[0] == "Tetrahedra") {
            std::getline(ss, str);
            state = TET;
        }         else if (info[0] == "Triangles") {
            std::getline(ss, str);
            state = FACE;
        }         else if (info[0] == "Edges") {
            std::getline(ss, str);
            state = EDGE;
        }         else if (info[0] == "End") {
            state = USELESS;
        }         else {
            if (state == USELESS) {
                continue;
            }             else if (state == VERTEX) {
                vertices.push_back(MeshKernel::iGameVertex(std::stod(info[0])
                        , std::stod(info[1]), std::stod(info[2])));
            }             else if (state == TET) {
                faces.push_back(std::vector<MeshKernel::iGameVertexHandle>{
                        (iGameVertexHandle)(std::stoi(info[0]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[1]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[2]) - 1)
                });
                faces.push_back(std::vector<MeshKernel::iGameVertexHandle>{
                        (iGameVertexHandle)(std::stoi(info[0]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[2]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[3]) - 1)
                });
                faces.push_back(std::vector<MeshKernel::iGameVertexHandle>{
                        (iGameVertexHandle)(std::stoi(info[0]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[3]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[1]) - 1)
                });
                faces.push_back(std::vector<MeshKernel::iGameVertexHandle>{
                        (iGameVertexHandle)(std::stoi(info[1]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[3]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[2]) - 1)
                });
                cells.push_back(faces);
                faces.clear();
            }             else if (state == FACE) {
                surface_faces.push_back(std::vector<MeshKernel::iGameVertexHandle>{
                        (iGameVertexHandle)(std::stoi(info[0]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[1]) - 1)
                        , (iGameVertexHandle)(std::stoi(info[2]) - 1)
                });
            }
        }
    }
    return TetMesh(vertices, cells, surface_faces);
}