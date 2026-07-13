#include "MeshFileIO.h"

bool MeshFileIO::read_mesh_file(const std::string& filename, int& dimension, int& category) {

	std::ifstream inputfile(filename, std::ios::in);
	if (!inputfile.good()) {
		std::cerr << "Error: File stream is bad.\n";
		return false;
	}
	std::cout << "MeshFileIO: Reading " << filename << "...\n";
	dimension = -1;
	category = -1;
	std::string line;
	while (inputfile) {
		line.clear();
		getline(inputfile, line);
		std::stringstream linestream;
		linestream.str(line);
		std::string str;
		linestream >> str;
		if (str[0] == 'D') { 

			linestream >> dimension;
			std::cout << "MeshFileIO: Dimension = " << dimension << std::endl;

		} else if (str == "Vertices") { 

			line.clear();
			getline(inputfile, line);
			int vcnt = std::stoi(line);
			std::cout << "MeshFileIO: NumVertices = " << vcnt << std::endl;
			vertices = std::vector<Vex>{};
			vertices.reserve(vcnt);
			double x, y, z, flag;
			for (int i = 0; i < vcnt; ++i) {
				line.clear();
				getline(inputfile, line);
				std::stringstream strs;
				strs.str(line);
				strs >> x >> y >> z;
				vertices.push_back(Vex(x, y, z));
			}

		} else if (str[0] == 'T' && str[1] == 'r') { 
			category = 3;
			std::cout << "MeshFileIO: Category = Triangle Mesh" << std::endl;

			line.clear();
			getline(inputfile, line);
			int fcnt = std::stoi(line);
			std::cout << "MeshFileIO: NumFaces = " << fcnt << std::endl;

			faces = std::vector<std::vector<VH>>{};
			faces.reserve(fcnt);
			for (int i = 0; i < fcnt; ++i) {
				line.clear();
				getline(inputfile, line);
				std::stringstream strs;
				strs.str(line);
				int vid0, vid1, vid2;
				strs >> vid0 >> vid1 >> vid2;
				std::vector<VH> face{ VH(vid0 - 1), VH(vid1 - 1), VH(vid2 - 1) };
				faces.push_back(face);
			}


		} else if (str[0] == 'S') { 
			category = 4;
			std::cout << "MeshFileIO: Category = Square Mesh" << std::endl;
			
			line.clear();
			getline(inputfile, line);
			int fcnt = std::stoi(line);
			std::cout << "MeshFileIO: NumFaces = " << fcnt << std::endl;

			faces = std::vector<std::vector<VH>>{};
			faces.reserve(fcnt);
			for (int i = 0; i < fcnt; ++i) {
				line.clear();
				getline(inputfile, line);
				std::stringstream strs;
				strs.str(line);
				int vid0, vid1, vid2, vid3;
				strs >> vid0 >> vid1 >> vid2 >> vid3;
				std::vector<VH> face{ VH(vid0 - 1), VH(vid1 - 1), VH(vid2 - 1), VH(vid3 - 1) };
				faces.push_back(face);
			}

		} else if (str[0] == 'T' && str[1] == 'e') { 
			category = 4;
			std::cout << "MeshFileIO: Category = Tetrahedral Mesh" << std::endl;
			
			line.clear();
			getline(inputfile, line);
			int ccnt = std::stoi(line);
			std::cout << "MeshFileIO: NumCells = " << ccnt << std::endl;

			cells = std::vector<std::vector<VH>>{};
			cells.reserve(ccnt);
			for (int i = 0; i < ccnt; ++i) {
				line.clear();
				getline(inputfile, line);
				std::stringstream strs;
				strs.str(line);
				std::vector<std::string> vids;
				get_strs(vids, strs);
				std::vector<VH> cell;
				for (int j = 0; j + 1 < vids.size(); ++j) {
					cell.push_back(VH(std::stoi(vids[j]) - 1));
				}
				cells.push_back(cell);
			}

		} else if (str[0] == 'H') { 
			category = 6;
			std::cout << "MeshFileIO: Category = Hexahedral Mesh" << std::endl;

			line.clear();
			getline(inputfile, line);
			int ccnt = std::stoi(line);
			std::cout << "MeshFileIO: NumCells = " << ccnt << std::endl;
			
			cells = std::vector<std::vector<VH>> {};
			cells.reserve(ccnt);
			for (int i = 0; i < ccnt; ++i) {
				line.clear();
				getline(inputfile, line);
				std::stringstream strs;
				strs.str(line);
				std::vector<std::string> vids;
				get_strs(vids, strs);
				std::vector<VH> cell;
				for (int j = 0; j + 1 < vids.size(); ++j) {
					cell.push_back(VH(std::stoi(vids[j]) - 1));
				}
				cells.push_back(cell);
			}
			break;
		}
	}

	return true;
}

bool MeshFileIO::write_mesh_file(const std::string& filename, const MeshKernel::SurfaceMesh& mesh) {

	std::ofstream off(filename.c_str(), std::ios::out);

	if (!off.good()) {
		std::cerr << "Error: Could not open file " << filename << " for writing!" << std::endl;
		off.close();
		return false;
	}

	off << "MeshVersionFormatted 1" << std::endl;
	off << "Dimension 2" << std::endl;

	int n_vertices(mesh.vsize());
	off << "Vertices" << std::endl;
	off << n_vertices << std::endl;
	for (int i = 0; i < n_vertices; ++i) {
		auto& v = mesh.vertices(VH(i));
		off << v.x() << " "
			<< v.y() << " "
			<< v.z() << " 1"
			<< std::endl;
	}

	int n_faces(mesh.fsize());
	auto& _face = mesh.faces(FH(0));
	if (_face.size() == 4) {
		off << "Square" << std::endl;
	} else if (_face.size() == 3) {
		off << "Triangle" << std::endl;
	}
	off << n_faces << std::endl;
	for (int i = 0; i < n_faces; ++i) {
		auto& c = mesh.faces(FH(i));
		const auto& vhs = c.getVertexHandle();
		for (auto& vh : vhs) {
			off << int(vh + 1) << " ";
		}
		off << "1" << std::endl;
	}
	off << std::endl;
	off.close();

	return true;
}

bool MeshFileIO::write_mesh_file(const std::string& filename, const MeshKernel::VolumeMesh& mesh) {

	std::ofstream off(filename.c_str(), std::ios::out);

	if (!off.good()) {
		std::cerr << "Error: Could not open file " << filename << " for writing!" << std::endl;
		off.close();
		return false;
	}

	off << "MeshVersionFormatted 1" << std::endl;
	off << "Dimension 3" << std::endl;

	int n_vertices(mesh.vsize());
	off << "Vertices" << std::endl;
	off << n_vertices << std::endl;
	for (int i = 0; i < n_vertices; ++i) {
		auto& v = mesh.vertices(VH(i));
		off << v.x() << " "
			<< v.y() << " "
			<< v.z() << " 1"
			<< std::endl;
	}

	int n_cells(mesh.csize());
	auto& _cell = mesh.cells(CH(0));
	if (_cell.faces_size() == 4) {
		off << "Tetrahedra" << std::endl;
	} else if (_cell.faces_size() == 6) {
		off << "Hexahedra" << std::endl;
	}
	off << n_cells << std::endl;
	for (int i = 0; i < n_cells; ++i) {
		auto& c = mesh.cells(CH(i));
		const auto& vhs = c.getVertexHandle();
		for (auto& vh : vhs) {
			off << int(vh + 1) << " ";
		}
		off << "1" << std::endl;
	}
	off << std::endl;
	off.close();

	return true;
}

bool MeshFileIO::get_mesh_data(std::vector<Vex>& res_vertices, std::vector<std::vector<VH>>& res_vhs) {
	res_vertices = vertices;
	if (!faces.empty() && cells.empty()) res_vhs = faces;
	if (!cells.empty()) res_vhs = cells;
	return !res_vertices.empty() && !res_vhs.empty();
}