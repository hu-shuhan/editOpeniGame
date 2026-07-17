#pragma once
#include "Mesh.h"
#include <fstream>
#include <string>
#include <sstream>
#include <array>

class MeshFileIO {
public:
	MeshFileIO(){};

	bool read_mesh_file(const std::string& filename, int& dimension, int& category);
	bool write_mesh_file(const std::string& filename, const MeshKernel::SurfaceMesh& mesh);
	bool write_mesh_file(const std::string& filename, const MeshKernel::VolumeMesh& mesh);
	bool get_mesh_data(std::vector<Vex>& res_vertices, std::vector<std::vector<VH>>& res_vhs);

private:
	std::vector<Vex> vertices;
	std::vector<std::vector<VH>> faces;
	std::vector<std::vector<VH>> cells;
	void get_strs(std::vector<std::string>& strs, std::stringstream& strstream) {
		std::string str;
		while (strstream >> str) {
			strs.push_back(str);
		}
	}
	
};