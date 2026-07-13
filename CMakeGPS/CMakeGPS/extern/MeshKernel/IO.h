#pragma once
#include "Mesh.h"
#include <fstream>
#include <string>
#include <sstream>
#include <array>


namespace MeshKernel {
	class IO {
	public:
		VolumeMesh iGameReadVolumeFile(const std::string&); 
		VolumeMesh iGameReadVolumeFile_TopOptMeshGeneration(const std::string&);
		void iGameWriteVolumeFile(const VolumeMesh& _mesh, const std::string&); 

		TetMesh iGameReadTetMeshFile(const std::string&); 

		SurfaceMesh ReadObjFile(const std::string& _InputFile, int& sides_num);
		SurfaceMesh ReadOffFile(const std::string& _InputFile, int& sides_num);
		bool WriteObjFile(const SurfaceMesh& _mesh, const std::string& _OutputFile);
		bool WriteOffFile(const SurfaceMesh& _mesh, const std::string& _OutputFile);
		std::string WriteOffString(const SurfaceMesh& _mesh);
		TetMesh ReadMeshFileFromStr(const std::string& data);
		std::vector<std::string> SplitFileName(const std::string& fileName);

	private:
		void ReOrderiGameVertexHandle(const SurfaceMesh& _mesh);
		std::vector<iGameVertexHandle> reorderedvh_;                         
		std::unordered_map<iGameVertexHandle, std::size_t> newvh_;           
	};
}

