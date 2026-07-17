#include"Mesh.h"
#include"string"

namespace MeshKernel { 
	
	void Mesh::initBBox() {
		double bbox_min_x = 99999999, bbox_min_y = 99999999, bbox_min_z = 99999999;
		double bbox_max_x = -99999999, bbox_max_y = -99999999, bbox_max_z = -99999999;
		for (auto& vp : vertices_) {
			bbox_min_x = std::min(bbox_min_x, vp.second.x());
			bbox_min_y = std::min(bbox_min_y, vp.second.y());
			bbox_min_z = std::min(bbox_min_z, vp.second.z());
			bbox_max_x = std::max(bbox_max_x, vp.second.x());
			bbox_max_y = std::max(bbox_max_y, vp.second.y());
			bbox_max_z = std::max(bbox_max_z, vp.second.z());
		}
		BBoxMin = iGameVertex(bbox_min_x, bbox_min_y, bbox_min_z);
		BBoxMax = iGameVertex(bbox_max_x, bbox_max_y, bbox_max_z);
	}

	bool Mesh::isConnected(iGameVertexHandle vh1, iGameVertexHandle vh2) {
		for (auto vh : NeighborVh(vh1)) {
			if (vh == vh2) return true;
		}
		return false;
	}

	bool Mesh::isConnected(iGameEdgeHandle eh1, iGameEdgeHandle eh2) {
		if (!isValid(eh1) || !isValid(eh2)) return false;
		auto e1 = edges_[eh1];
		auto e2 = edges_[eh2];
		auto vh1 = e1.vh1(), vh2 = e1.vh2();
		auto vh3 = e2.vh1(), vh4 = e2.vh2();
		return (vh1 == vh3 || vh1 == vh4 || vh2 == vh3 || vh2 == vh4);
	}

	bool Mesh::isConnected(iGameFaceHandle fh1, iGameFaceHandle fh2) {
		if (!isValid(fh1) || !isValid(fh2)) return false;
		auto& face1 = faces_[fh1];
		auto& face2 = faces_[fh2];
		const auto& ehs1 = face1.getEdgeHandle();
		for (const auto& eh : face2.getEdgeHandle()) {
			if (std::find(ehs1.begin(), ehs1.end(), eh) != ehs1.end()) {
				return true;
			}
		}
		return false;
	}

	double Mesh::getLength(iGameEdgeHandle eh) {
		assert(edges_.count(eh));
		auto& edge = edges_[eh];
		auto& v1 = vertices_[edge.vh1()];
		auto& v2 = vertices_[edge.vh2()];
		return (v1 - v2).norm();
	}

	void Mesh::genLength(iGameEdgeHandle eh) {
		auto& edge = edges_[eh];
		auto& v1 = vertices_[edge.vh1()];
		auto& v2 = vertices_[edge.vh2()];
		edge.setLength((v2 - v1).norm());
	}

	void Mesh::genAllEdgesLength() {
		for (auto& ep : this->edges_) {
			genLength(ep.first);
		}
	}

	iGameEdgeHandle Mesh::getEdgeHandle(iGameVertexHandle vh1, iGameVertexHandle vh2) {
		iGameEdgeHandle ret(-1);
		int vh_sum = vh1 + vh2;
		if (!isValid(vh1) || !isValid(vh2)) return ret;
		for (auto& eh : NeighborEh(vh1)) {
			auto& e = edges_[eh];
			if (vh_sum = e.vh1() + e.vh2()) {
				ret = eh;
				break;
			}
		}
		return ret;
	}

	iGameVertex Mesh::getEdgeMidpoint(iGameEdgeHandle eh) {
		if (!isValid(eh)) {
			std::cerr << "Edge handle is invalid!" << std::endl;
			return iGameVertex(0, 0, 0);
		}
		auto& edge = edges_[eh];
		auto& v1 = vertices_[edge.vh1()];
		auto& v2 = vertices_[edge.vh2()];
		return (v1 + v2) / 2;
	}

	iGameVertex Mesh::getFaceCenter(iGameFaceHandle fh) {
		if (!isValid(fh)) {
			std::cerr << "Face handle is invalid!" << std::endl;
			return iGameVertex(0, 0, 0);
		}
		auto& face = faces_[fh];
		auto vhs = face.getVertexHandle();
		iGameVertex center(0, 0, 0);
		for (auto& vh : vhs) {
			center += vertices_[vh];
		}
		return center / vhs.size();
	}


	iGameFaceHandle Mesh::AddFace(const std::vector<iGameVertexHandle>& _vhs) {
		std::vector<iGameEdgeHandle> ehs(_vhs.size());
		for (int i = 0; i < _vhs.size(); ++i) {
			if (i == 0) {
				ehs[i] = AddEdge(_vhs[_vhs.size() - 1], _vhs[i]);
			}
			else {
				ehs[i] = AddEdge(_vhs[i], _vhs[i - 1]);
			}
		}
		iGameFace f(_vhs, ehs);
		if (Face2Fh_.count(f)) {
			iGameFaceHandle fh = Face2Fh_[f];
			faces_[fh] = f;											 
			return Face2Fh_[f];
		}
		else {
			iGameFaceHandle fh = GenFaceHandle();						 
			faces_[fh] = f;											 
			Face2Fh_[f] = fh;										 
			AddFace2Neighbor(fh);									 
			return Face2Fh_[f];										 
		}
	}
	iGameEdgeHandle Mesh::AddEdge(const iGameVertexHandle& vh1, const iGameVertexHandle& vh2) {
		iGameEdge e(vh1, vh2);
		if (Edge2Eh_.count(e)) {
			return Edge2Eh_[e];
		}
		else {
			iGameEdgeHandle eh = GenEdgeHandle();						 
			edges_[eh] = e;											 
			Edge2Eh_[e] = eh;										 
			AddEdge2Neighbor(eh);									 
			return Edge2Eh_[e];										 
		}
	}
	iGameVertexHandle Mesh::AddVertex(const iGameVertex& _v) {
		if (Vertex2Vh_.count(_v)) return Vertex2Vh_[_v];
		iGameVertexHandle vh = GenVertexHandle();
		vertices_[vh] = _v;
		Vertex2Vh_[_v] = vh;
		return Vertex2Vh_[_v];
		
	}

	iGameVertexHandle Mesh::DeleteVertex(iGameVertexHandle _vh) {
		if (!vertices_.count(_vh)) return iGameVertexHandle(-1);         
		else {
			auto ve = NeighborEhOfVertex_[_vh];
			for (iGameEdgeHandle eh : ve) {
				DeleteEdge(eh);
			}
			Vertex2Vh_.erase(vertices_[_vh]);
			vertices_.erase(_vh);
			NeighborEhOfVertex_.erase(_vh);
			NeighborFhOfVertex_.erase(_vh);
			return _vh;
		}

	}
	iGameEdgeHandle Mesh::DeleteEdge(iGameEdgeHandle _eh) {
		if (!edges_.count(_eh)) return iGameEdgeHandle(-1);              
		else {
			iGameEdge e(edges_[_eh]);
			for (int i = 0; i < 2; ++i) {
				iGameVertexHandle ev = e.vh(i);
				NeighborEhOfVertex_[ev].erase(_eh);                 
			}
			auto ef = NeighborFhOfEdge_[_eh];
			for (iGameFaceHandle fh : ef ) {
				DeleteFace(fh);
			}
			Edge2Eh_.erase(edges_[_eh]);
			edges_.erase(_eh);
			NeighborFhOfEdge_.erase(_eh);
			return _eh;
		}
	}
	iGameFaceHandle Mesh::DeleteFace(iGameFaceHandle _fh) {
		if (!faces_.count(_fh)) return iGameFaceHandle(-1);              
		else {                                                      
			iGameFace f(faces_[_fh]);
			for (int i = 0; i < f.size(); ++i) {
				iGameVertexHandle fv = f.vh(i);
				iGameEdgeHandle fe = f.eh(i);
				NeighborFhOfVertex_[fv].erase(_fh);                
				NeighborFhOfEdge_[fe].erase(_fh);                  
			}
			Face2Fh_.erase(faces_[_fh]);
			faces_.erase(_fh);
			return _fh;
		}
	}

	void Mesh::AddFace2Neighbor(const iGameFaceHandle& _fh)
	{
		iGameFace f = faces_[_fh];
		size_t n = f.size();
		for (int i = 0; i < n; ++i) {
			NeighborFhOfVertex_[f.vh(i)].insert(_fh);
		}
		for (int i = 0; i < n; ++i) {
			NeighborFhOfEdge_[f.eh(i)].insert(_fh);
		}
	}

	void Mesh::AddEdge2Neighbor(const iGameEdgeHandle& _eh)
	{
		iGameEdge e = edges_[_eh];
		NeighborEhOfVertex_[e.vh1()].insert(_eh);
		NeighborEhOfVertex_[e.vh2()].insert(_eh);

	}

	void Mesh::DeleteFace2Neighbor(const iGameFaceHandle& _fh){
		iGameFace f = faces_[_fh];
		size_t n = f.size();
		for (int i = 0; i < n; ++i) {
			NeighborFhOfVertex_[f.vh(i)].erase(_fh);
		}
		for (int i = 0; i < n; ++i) {
			NeighborFhOfEdge_[f.eh(i)].erase(_fh);
		}
	}
	void Mesh::DeleteEdge2Neighbor(const iGameEdgeHandle& _eh) {
		iGameEdge e = edges_[_eh];
		NeighborEhOfVertex_[e.vh1()].erase(_eh);
		NeighborEhOfVertex_[e.vh2()].erase(_eh);
	}

	Mesh & Mesh::operator=(const Mesh & _surfacemesh) {
		vertices_ = _surfacemesh.vertices_;
		edges_ = _surfacemesh.edges_;
		faces_ = _surfacemesh.faces_;
		Vertex2Vh_ = _surfacemesh.Vertex2Vh_;
		Edge2Eh_ = _surfacemesh.Edge2Eh_;
		Face2Fh_ = _surfacemesh.Face2Fh_;
		NeighborEhOfVertex_ = _surfacemesh.NeighborEhOfVertex_;
		NeighborFhOfVertex_ = _surfacemesh.NeighborFhOfVertex_;
		NeighborFhOfEdge_ = _surfacemesh.NeighborFhOfEdge_;
		VertexHandleID_ = _surfacemesh.VertexHandleID_;
		EdgeHandleID_ = _surfacemesh.EdgeHandleID_;
		FaceHandleID_ = _surfacemesh.FaceHandleID_;
		return *this;

	}

	iGameVertex& Mesh::vertices(iGameVertexHandle _vh) {
		assert(vertices_.count(_vh));
		return vertices_[_vh];
	}
	const iGameVertex Mesh::vertices(iGameVertexHandle _vh) const {
		assert(vertices_.count(_vh));
		return vertices_.find(_vh)->second;                       
	}
	iGameEdge& Mesh::edges(iGameEdgeHandle _eh) {
		assert(edges_.count(_eh));
		return edges_[_eh];
	}
	const iGameEdge& Mesh::edges(iGameEdgeHandle _eh) const {
		assert(edges_.count(_eh));
		return edges_.find(_eh)->second;
	}
	iGameFace& Mesh::faces(iGameFaceHandle _fh) {
		assert(faces_.count(_fh));
		return faces_[_fh];
	}
	const iGameFace Mesh::faces(iGameFaceHandle _fh) const {
		assert(faces_.count(_fh));
		return faces_.find(_fh)->second;
	}

	const iGameVertexHandle Mesh::vertexhanle(iGameVertex _vertex) const {
		if (Vertex2Vh_.find(_vertex) != Vertex2Vh_.end()) return Vertex2Vh_.find(_vertex)->second;
		else return iGameVertexHandle(-1);
	}
	const iGameEdgeHandle Mesh::edgehandle(iGameEdge& _edge) const {
		if (Edge2Eh_.find(_edge) != Edge2Eh_.end()) return Edge2Eh_.find(_edge)->second;
		else return iGameEdgeHandle(-1);
	}
	const iGameFaceHandle Mesh::facehandle(iGameFace& _face) const {
		if (Face2Fh_.find(_face) != Face2Fh_.end()) return Face2Fh_.find(_face)->second;
		else return iGameFaceHandle(-1);
	}

	std::unordered_set<iGameVertexHandle> Mesh::NeighborVh(iGameVertexHandle _vh) {
		std::unordered_set<iGameVertexHandle> neighborvh;
		auto neighboreh = NeighborEh(_vh);
		if (neighboreh.size()) {
			for (iGameEdgeHandle eh : neighboreh) {
				if (edges_[eh].vh1() != _vh) neighborvh.insert(edges_[eh].vh1());
				if (edges_[eh].vh2() != _vh) neighborvh.insert(edges_[eh].vh2());
			}
		}
		return neighborvh;
	}
	std::unordered_set<iGameEdgeHandle>& Mesh::NeighborEh(iGameVertexHandle _vh) {
		if (NeighborEhOfVertex_.count(_vh)) return NeighborEhOfVertex_[_vh];
		else return empty_ehs;                
	}
	std::unordered_set<iGameFaceHandle>& Mesh::NeighborFh(iGameVertexHandle _vh) {
		if (NeighborFhOfVertex_.count(_vh)) return NeighborFhOfVertex_[_vh];
		else return empty_fhs;                
	}
	std::unordered_set<iGameEdgeHandle> Mesh::NeighborEh(iGameEdgeHandle _eh) {
		assert(edges_.count(_eh));                      
		std::unordered_set<iGameEdgeHandle> neighboreh;      
		int k = 0;                                      
		while (k < 2) {                                
			iGameVertexHandle vh = edges_[_eh].vh(k);
			auto vhneighboreh = NeighborEh(vh);         
			for (iGameEdgeHandle eh : vhneighboreh) {
				if (eh != _eh) neighboreh.insert(eh);
			}
			++k;                  
		}
		
		return neighboreh;
	}
	std::unordered_set<iGameFaceHandle>& Mesh::NeighborFh(iGameEdgeHandle _eh) {
		if (NeighborFhOfEdge_.count(_eh)) return NeighborFhOfEdge_[_eh];
		else return empty_fhs;                
	}
	std::unordered_set<iGameFaceHandle> Mesh::NeighborFh(iGameFaceHandle _fh) {
		assert(faces_.count(_fh));                      
		std::unordered_set<iGameFaceHandle> neigborface;
		int k = 0;                                      
		size_t facesize = faces_[_fh].size();
		while (k < facesize) {
			iGameEdgeHandle eh = faces_[_fh].eh(k);
			auto ehneighborfh = NeighborFh(eh);         
			for (iGameFaceHandle fh : ehneighborfh) {
				if (fh != _fh) neigborface.insert(fh);
			}
			++k;
		}
		return neigborface;
	}
	std::unordered_set<iGameFaceHandle> Mesh::Neighbor2Fh(iGameFaceHandle _fh) {
		assert(faces_.count(_fh));                      
		std::unordered_set<iGameFaceHandle> neigborface;
		auto v_indices = faces_[_fh].getVertexHandle();
		for (auto& v_idx : v_indices) {
			auto adjF = NeighborFh(v_idx);
			for (iGameFaceHandle fh : adjF) {
				if (fh != _fh) neigborface.insert(fh);
			}
		}
		return neigborface;
	}

    MeshKernel::iGameVertexHandle Mesh::NeighborVhFromEdge(iGameVertexHandle _vh, iGameEdgeHandle _eh) {
        assert(NeighborEh(_vh).count(_eh));      
        iGameVertexHandle vh = (edges_[_eh].vh(0) == _vh) ? edges_[_eh].vh(1) : edges_[_eh].vh(0);
        return vh;
    }
    MeshKernel::iGameEdgeHandle Mesh::NeighborEhFromVertex(iGameEdgeHandle _eh, iGameVertexHandle _vh){
        assert(NeighborEh(_vh).count(_eh));      
        if(!NeighborEh(_vh).count(_eh)) return EH{-1};
        if(NeighborEh(_vh).size() != 4) return EH{-1}; 
        std::vector<iGameEdgeHandle> n_ehs;
        auto n_fhs = NeighborFh(_eh);
        for(auto n_eh : NeighborEh(_vh)){
            if(n_eh != _eh) n_ehs.emplace_back(n_eh);
        }
        for(auto n_eh : n_ehs){
            bool not_neighbor = true;
            for(auto n_fh : NeighborFh(n_eh)){
                if(n_fhs.count(n_fh)){
                    not_neighbor = false;
                    break;
                }
            }
            if(not_neighbor) return n_eh;
        }
        return EH{-1};
    }

    iGameFaceHandle Mesh::NeighborFhFromEdge(iGameFaceHandle _fh, iGameEdgeHandle _eh){
        assert(NeighborFh(_eh).count(_fh));      
        if (!NeighborFh(_eh).count(_fh)) return iGameFaceHandle{-1};
        for (iGameFaceHandle fh : NeighborFh(_eh)) {
            if (fh != _fh) return fh;
        }
        return iGameFaceHandle{-1};
    }

    iGameEdgeHandle Mesh::OppositeEhFromEdge(iGameFaceHandle _fh, iGameEdgeHandle _eh){
        assert(NeighborFh(_eh).count(_fh));      
        auto& f = faces(_fh);
        if(f.size() == 4){
            for(auto n_eh : f.getEdgeHandle()){
                if(n_eh == _eh) continue;
                bool notNeighbor = true;
                for(auto n_n_eh : NeighborEh(_eh)){
                    if(n_n_eh == n_eh){
                        notNeighbor = false;
                        break;
                    }
                }
                if(notNeighbor) return n_eh;
            }
        }
        else{
            for(auto n_eh : f.getEdgeHandle()){
                if(n_eh == _eh) continue;
                return n_eh;
            }
        }
        return iGameEdgeHandle{-1};
    }

    iGameEdgeHandle Mesh::getEhFromTwoVh(iGameVertexHandle vh1, iGameVertexHandle vh2) {
        iGameEdgeHandle tempEh{-1};
        for (auto eh : NeighborEh(vh1)) {
            auto& e = edges(eh);
            if (vh2 == (e.vh1() == vh1 ? e.vh2() : e.vh1())) {
                return eh;
            }
        }
        return tempEh;
    }

    iGameVertex Mesh::getEdgeVector(iGameEdge _e) {
        return vertices(_e.vh2()) - vertices(_e.vh1());
    }
}



namespace MeshKernel {
	void SurfaceMesh::InitMesh(const std::vector<iGameVertex>& _vertices,
		const std::vector<std::vector<iGameVertexHandle>>& _elements) {
		for (auto v : _vertices) {
			auto vh = AddVertex(iGameVertex(v.x(), v.y(), v.z()));

		}
		for (auto f : _elements) {
			AddFace(f);
		}
	}
	SurfaceMesh& SurfaceMesh::operator=(const SurfaceMesh& _surfacemesh) {
		if (this != &_surfacemesh) {
			Mesh::operator=(_surfacemesh);
		}
		return *this;
	}

	bool SurfaceMesh::isOnBoundary(iGameEdgeHandle eh) {
		auto fcnt = NeighborFh(eh).size();
		return fcnt == 1;
	}

	bool SurfaceMesh::isOnBoundary(iGameVertexHandle vh) {
		for (auto eh : NeighborEh(vh)) {
			if (isOnBoundary(eh)) {
				return true;
			}
		}
		return false;
	}

	bool SurfaceMesh::isOnBoundary(iGameFaceHandle fh) {
		auto face = faces(fh);
		for (auto eh : face.getEdgeHandle()) {
			if (isOnBoundary(eh)) {
				return true;
			}
		}
		return false;
	}

	bool SurfaceMesh::hasLoopBoundary() {

		std::unordered_map<EH, bool> visited;
		EH head_eh(-1);

		for (auto& ep : edges_) {
			if (isOnBoundary(ep.first)) {  
				if (visited.count(ep.first)) continue;
				if (head_eh != -1) return false; 
				head_eh = ep.first;
				EH pre_eh(-1), cur_eh(head_eh);
				do {
					visited[cur_eh] = true;
					int degree = 0;
					EH next_eh(-1);
					for (auto& adjeh : NeighborEh(cur_eh)) {
						if (isOnBoundary(adjeh)) {
							degree++;
							if (adjeh != pre_eh) {
								next_eh = adjeh;
							}
						}
					}
					if (degree != 2) return false; 
					pre_eh = cur_eh;
					cur_eh = next_eh;
				} while (cur_eh != head_eh);
				
			}
		}

		return head_eh != -1;

	}

	bool SurfaceMesh::isTriangleMesh() {
		for (auto& fp : faces_) {
			if (fp.second.getVertexHandle().size() != 3) return false;
		}
		return true;
	}

	void SurfaceMesh::updateAllHandles() {
		int vcnt = VertexSize(), fcnt = FaceSize();
		std::vector<MeshKernel::iGameVertex> newVertices;
		std::vector<std::vector<MeshKernel::iGameVertexHandle>> newFaces;
		std::unordered_map<int, int> mp;     
		int idx = 0;
		for (auto& fp : allfaces()) {
			auto vhs = fp.second.getVertexHandle();
			for (auto& vh : vhs) {
				if (!mp.count(vh)) {
					mp[vh] = idx++;
					newVertices.push_back(vertices_[vh]);
				}
				vh = iGameVertexHandle(mp[vh]);
			}
			newFaces.push_back(vhs);
		}

		*this = MeshKernel::SurfaceMesh(newVertices, newFaces);
	}

	

	void SurfaceMesh::genNormal(iGameFaceHandle fh) {
		auto& face = faces(fh);
		auto vex = face.getVertexHandle();
		assert(vex.size() >= 3 && "should be a face not a line");
		std::vector<Eigen::Vector3d> pos(3, Eigen::Vector3d::Zero());
		for (int i = 0; i < 3; ++i) {
			auto& v = vertices(vex[i]);
			pos[i][0] = v.x();
			pos[i][1] = v.y();
			pos[i][2] = v.z();
		}
		Eigen::Vector3d N = (pos[1] - pos[0]).cross(pos[2] - pos[0]);
		N.normalize();
		face.setNormal(N[0], N[1], N[2]);
	}

	void SurfaceMesh::genNormal(iGameVertexHandle vh) {
		auto& v = vertices(vh);
		Eigen::Vector3d N = Eigen::Vector3d::Zero();
		auto adjFH = NeighborFh(vh);
		for (auto& fh : adjFH) {
			auto& face = faces(fh);
			Eigen::Vector3d faceN = Eigen::Vector3d(face.getNormalX(), face.getNormalY(), face.getNormalZ());
			N += faceN;
		}
		N /= adjFH.size();
		N.normalize();
		v.setNormal(N[0], N[1], N[2]);
	}

	void SurfaceMesh::genAllFacesNormal() {
		for (auto& fp : this->faces_) {
			genNormal(fp.first);
		}
	}

	void SurfaceMesh::genAllVerticesNormal() {
		this->genAllFacesNormal();
		for (auto& vp : this->vertices_) {
			genNormal(vp.first);
		}
	}

	size_t SurfaceMesh::getBoundaryVerticesCount() {
		size_t cnt = 0;
		for (auto& vp : vertices_) {
			if (isOnBoundary(vp.first))
				cnt++;
		}
		return cnt;
	}

}


namespace MeshKernel {
	void VolumeMesh::InitMesh(const std::vector<iGameVertex>& _vertices,
		const std::vector<std::vector<iGameVertexHandle>>& _elements) {
		for (auto v : _vertices) {
			auto vh = AddVertex(iGameVertex(v.x(), v.y(), v.z()));
		}
		for (auto c : _elements) {
			AddCell(c);
		}
	}
	VolumeMesh& VolumeMesh::operator=(const VolumeMesh& _volumemesh) {
		vertices_ = _volumemesh.vertices_;
		edges_ = _volumemesh.edges_;
		faces_ = _volumemesh.faces_;
		cells_ = _volumemesh.cells_;
		Vertex2Vh_ = _volumemesh.Vertex2Vh_;
		Edge2Eh_ = _volumemesh.Edge2Eh_;
		Face2Fh_ = _volumemesh.Face2Fh_;
		Cell2Ch_ = _volumemesh.Cell2Ch_;
		NeighborEhOfVertex_ = _volumemesh.NeighborEhOfVertex_;
		NeighborFhOfVertex_ = _volumemesh.NeighborFhOfVertex_;
		NeighborChOfVertex_ = _volumemesh.NeighborChOfVertex_;
		NeighborFhOfEdge_ = _volumemesh.NeighborFhOfEdge_;
		NeighborChOfEdge_ = _volumemesh.NeighborChOfEdge_;
		NeighborChOfFace_ = _volumemesh.NeighborChOfFace_;
		VertexHandleID_ = _volumemesh.VertexHandleID_;
		EdgeHandleID_ = _volumemesh.EdgeHandleID_;
		FaceHandleID_ = _volumemesh.FaceHandleID_;
		CellHandleID_ = _volumemesh.CellHandleID_;
		surface_faces_ = _volumemesh.surface_faces_;
		return *this;
	}
	iGameCell& VolumeMesh::cells(iGameCellHandle _ch) {
		assert(cells_.count(_ch));
		return cells_[_ch];
	}
	const iGameCell VolumeMesh::cells(iGameCellHandle _ch) const {
		assert(cells_.count(_ch));
		return cells_.find(_ch)->second;
	}
	const iGameCellHandle VolumeMesh::cellhandle(iGameCell& _cell) const {
		if (Cell2Ch_.find(_cell) != Cell2Ch_.end()) return Cell2Ch_.find(_cell)->second;
		else return iGameCellHandle(-1);
	}
	std::unordered_set<iGameCellHandle> VolumeMesh::NeighborCh(iGameVertexHandle _vh) {
		if (NeighborChOfVertex_.count(_vh)) return NeighborChOfVertex_[_vh];
		else return std::unordered_set<iGameCellHandle>();
	}
	std::unordered_set<iGameCellHandle> VolumeMesh::NeighborCh(iGameEdgeHandle _eh) {
		if (NeighborChOfEdge_.count(_eh)) return NeighborChOfEdge_[_eh];
		else return std::unordered_set<iGameCellHandle>();
	}
	std::unordered_set<iGameCellHandle> VolumeMesh::NeighborCh(iGameFaceHandle _fh) {
		if (NeighborChOfFace_.count(_fh)) return NeighborChOfFace_[_fh];
		else return std::unordered_set<iGameCellHandle>();
	}
	std::unordered_set<iGameCellHandle> VolumeMesh::NeighborCh(iGameCellHandle _ch) {
		assert(cells_.count(_ch));                      
		std::unordered_set<iGameCellHandle> neigborcell;
		int k = 0;                                      
		size_t facesize = cells_[_ch].faces_size();
		while (k < facesize) {
			iGameFaceHandle fh = cells_[_ch].fh(k);
			auto fhneighborch = NeighborCh(fh);         
			for (iGameCellHandle ch : fhneighborch) {
				if (ch != _ch) neigborcell.insert(ch);
			}
			++k;
		}
		return neigborcell;
	}

	iGameCellHandle VolumeMesh::AddCell (const std::vector<iGameVertexHandle>& _vhs) {
		int facecnt = _vhs.size() == 8 ? 6 : 4;
		int edgecnt = _vhs.size() == 8 ? 12 : 6;
		std::vector<iGameFaceHandle> fhs(facecnt,(iGameFaceHandle)0);
		std::vector<std::vector<int>> faceform(facecnt);
		if (facecnt == 6) {
			faceform = { {0,3,2,1},{0,4,7,3},{0,1,5,4},{4,5,6,7},{1,2,6,5},{2,3,7,6}};
		}
		else {
			faceform = { {0,1,3},{1,2,3},{2,0,3},{0,2,1}};
		}
		for (int i = 0; i < facecnt; ++i) {
			std::vector<iGameVertexHandle> facevertices(faceform[i].size());
			for (int j = 0; j < faceform[i].size(); ++j) {
				facevertices[j] = _vhs[faceform[i][j]];
			}
			fhs[i] = AddFace(facevertices);
		}
		std::vector<iGameEdgeHandle> ehs(edgecnt);
		if (edgecnt == 12) {
			ehs = { faces_[fhs[0]].eh(0),faces_[fhs[0]].eh(1),faces_[fhs[0]].eh(2),faces_[fhs[0]].eh(3),
			faces_[fhs[3]].eh(0),faces_[fhs[3]].eh(1),faces_[fhs[3]].eh(2),faces_[fhs[3]].eh(3),
			faces_[fhs[1]].eh(1), faces_[fhs[1]].eh(3), faces_[fhs[4]].eh(2), faces_[fhs[4]].eh(0) };
		} 
		else { 
			ehs = { faces_[fhs[3]].eh(0),faces_[fhs[3]].eh(1),faces_[fhs[3]].eh(2),
			faces_[fhs[0]].eh(0), faces_[fhs[1]].eh(0), faces_[fhs[2]].eh(0) };
		}
		iGameCell c(_vhs, ehs, fhs);
		if (Cell2Ch_.count(c)) {
			return Cell2Ch_[c];
		}
		else {
			iGameCellHandle ch = GenCellHandle();						 
			cells_[ch] = c;											 
			Cell2Ch_[c] = ch;										 
			AddCell2Neighbor(ch);									 
			return Cell2Ch_[c];										 
		}
	}

	iGameCellHandle VolumeMesh::AddCell(const std::vector< std::vector<iGameVertexHandle>>& _vhs) {
		if (_vhs.size() < 4) return iGameCellHandle(-1);
		std::vector<MeshKernel::iGameVertexHandle> vertices;
		std::vector<MeshKernel::iGameEdgeHandle> edges;
		std::vector<MeshKernel::iGameFaceHandle> faces;
		for (int i = 0; i < _vhs.size(); i++) {
			faces.push_back(AddFace(_vhs[i]));
			for (int j = 0; j < _vhs[i].size(); j++) {
				if (j == 0) {
					auto eh = AddEdge(_vhs[i][_vhs[i].size() - 1], _vhs[i][j]);
					auto it = std::find(edges.begin(), edges.end(), eh);
					if (it == edges.end())
						edges.push_back(eh);
				} 				else {
					auto eh = AddEdge(_vhs[i][j], _vhs[i][j - 1]);
					auto it = std::find(edges.begin(), edges.end(), eh);
					if (it == edges.end())
						edges.push_back(eh);
				}
				auto it = std::find(vertices.begin(), vertices.end(), _vhs[i][j]);
				if (it == vertices.end())
					vertices.push_back(_vhs[i][j]);
			}
		}
		iGameCell c(vertices, edges, faces);
		iGameCellHandle ch = GenCellHandle();
		cells_[ch] = c;
		return ch;
	}

	iGameVertexHandle VolumeMesh::DeleteVertex(const iGameVertexHandle& _vh) {
		if (!vertices_.count(_vh)) return iGameVertexHandle(-1);         
		else {
			auto ve = NeighborEhOfVertex_[_vh];
			for (iGameEdgeHandle eh : ve) {
				DeleteEdge(eh);
			}
			Vertex2Vh_.erase(vertices_[_vh]);
			vertices_.erase(_vh);
			NeighborEhOfVertex_.erase(_vh);
			NeighborFhOfVertex_.erase(_vh);
			NeighborChOfVertex_.erase(_vh);
			return _vh;
		}
	}
	iGameEdgeHandle VolumeMesh::DeleteEdge(const iGameEdgeHandle& _eh) {
		if (!edges_.count(_eh)) return iGameEdgeHandle(-1);              
		else {
			iGameEdge e(edges_[_eh]);
			for (int i = 0; i < 2; ++i) {
				iGameVertexHandle ev = e.vh(i);
				NeighborEhOfVertex_[ev].erase(_eh);                 
			}
			auto ef = NeighborFhOfEdge_[_eh];
			for (iGameFaceHandle fh : ef) {
				DeleteFace(fh);
			}
			Edge2Eh_.erase(edges_[_eh]);
			edges_.erase(_eh);
			NeighborFhOfEdge_.erase(_eh);
			NeighborChOfEdge_.erase(_eh);
			return _eh;
		}
	}
	iGameFaceHandle VolumeMesh::DeleteFace(const iGameFaceHandle& _fh) {
		if (!faces_.count(_fh)) return iGameFaceHandle(-1);              
		else {                                                      
			iGameFace f(faces_[_fh]);
			for (int i = 0; i < f.size(); ++i) {
				iGameVertexHandle fv = f.vh(i);
				iGameEdgeHandle fe = f.eh(i);
				NeighborFhOfVertex_[fv].erase(_fh);                
				NeighborFhOfEdge_[fe].erase(_fh);                  
			}
			auto fc = NeighborChOfFace_[_fh];
			for (iGameCellHandle ch : fc) {
				DeleteCell(ch);
			}
			Face2Fh_.erase(faces_[_fh]);
			faces_.erase(_fh);
			NeighborChOfFace_.erase(_fh);
			return _fh;
		}
	}
	iGameCellHandle VolumeMesh::DeleteCell(const iGameCellHandle& _ch) {
		if (!cells_.count(_ch)) return iGameCellHandle(-1);             
		else {                                                     
			iGameCell c(cells_[_ch]);
			std::vector<int> vsize = { 4,8 };
			std::vector<int> esize = { 6,12 };
			std::vector<int> fsize = { 4,6 };
			int volumeType = c.vertices_size() == 4 ? 0 : 1;
			for (int i = 0; i < vsize[volumeType]; ++i) {
				iGameVertexHandle cv = c.vh(i);
				NeighborChOfVertex_[cv].erase(_ch);                               
			}
			for (int i = 0; i < esize[volumeType]; ++i) {
				iGameEdgeHandle ce = c.eh(i);
				NeighborChOfEdge_[ce].erase(_ch);
			}
			for (int i = 0; i < fsize[volumeType]; ++i) {
				iGameFaceHandle cf = c.fh(i);
				NeighborChOfFace_[cf].erase(_ch);
			}
			Cell2Ch_.erase(cells_[_ch]);
			cells_.erase(_ch);
			return _ch;
		}
	}

	void VolumeMesh::AddCell2Neighbor(const iGameCellHandle& _ch)
	{
		iGameCell c(cells_[_ch]);
		std::vector<int> vsize = { 4,8 };
		std::vector<int> esize = { 6,12 };
		std::vector<int> fsize = { 4,6 };
		int volumeType = c.vertices_size() == 4 ? 0 : 1;
		for (int i = 0; i < vsize[volumeType]; ++i) {
			NeighborChOfVertex_[c.vh(i)].insert(_ch);
		}
		for (int i = 0; i < esize[volumeType]; ++i) {
			NeighborChOfEdge_[c.eh(i)].insert(_ch);
		}
		for (int i = 0; i < fsize[volumeType]; ++i) {
			NeighborChOfFace_[c.fh(i)].insert(_ch);
		}
	}

	void VolumeMesh::DeleteCell2Neighbor(const iGameCellHandle& _ch) {
		if (!cells_.count(_ch)) return;
		iGameCell c(cells_[_ch]);
		for (auto& fh : c.getFaceHandle()) {
			NeighborChOfFace_[fh].erase(_ch);
		}
		for (auto& eh : c.getEdgeHandle()) {
			NeighborChOfEdge_[eh].erase(_ch);
		}
		for (auto& vh : c.getVertexHandle()) {
			NeighborChOfVertex_[vh].erase(_ch);
		}
	}

	bool VolumeMesh::isOnBoundary(iGameCellHandle ch) {
		assert(cells_.count(ch));
		auto fhs = cells_[ch].getFaceHandle();
		for (auto& fh : fhs) {
			if (NeighborCh(fh).size() == 1) {
				return true;
			}
		}
		return false;
	}

	bool VolumeMesh::isOnBoundary(iGameFaceHandle fh) {
		assert(faces_.count(fh));
		return NeighborCh(fh).size() == 1;     
	}

	bool VolumeMesh::isOnBoundary(iGameEdgeHandle eh) {
		assert(edges_.count(eh));
		for (auto& fh : NeighborFhOfEdge_[eh]) {
			if (NeighborCh(fh).size() == 1) {
				return true;
			}
		}
		return false;
	}

	bool VolumeMesh::isOnBoundary(iGameVertexHandle vh) {
		if (vertices_.count(vh) == 0) std::cout << "isOnBoundary Wrong" << std::endl;
		assert(vertices_.count(vh));
		for (auto& fh : NeighborFhOfVertex_[vh]) {
			if (NeighborCh(fh).size() == 1) {
				return true;
			}
		}
		return false;
	}

	iGameVertex VolumeMesh::getCellCenter(iGameCellHandle ch) {
		iGameVertex center(0, 0, 0);
		const auto& cell = cells_[ch];
		auto vhs = cell.getVertexHandle();
		for (auto& vh : vhs) {
			center = center + vertices_[vh];
		}
		return center / vhs.size();
	}

	double VolumeMesh::getQuadArea(iGameFaceHandle fh) {
		const auto& face = faces_[fh];
		auto vhs = face.getVertexHandle();
		assert(vhs.size() == 4);
		double quad_area = 0;
		const auto& v1 = vertices_[vhs[0]];
		const auto& v2 = vertices_[vhs[1]];
		const auto& v3 = vertices_[vhs[2]];
		const auto& v4 = vertices_[vhs[3]];
		auto vec13 = v3 - v1;
		auto vec12 = v2 - v1;
		auto vec14 = v4 - v1;
		quad_area += (vec12 % vec13).norm() * 0.5;
		quad_area += (vec14 % vec13).norm() * 0.5;
		quad_area += (vec12 % vec14).norm() * 0.5;
		auto vec23 = v3 - v2;
		auto vec24 = v4 - v2;
		quad_area += (vec23 % vec24).norm() * 0.5;
		return quad_area / 2;
	}

	iGameVertex VolumeMesh::getQuadNormal(iGameFaceHandle fh) {
		iGameVertex N(0, 0, 0);
		const auto& face = faces_[fh];
		auto vhs = face.getVertexHandle();
		auto chs = NeighborCh(fh);
		if (chs.size() != 1) return N;
		assert(vhs.size() == 4);
		auto cell_cenetr = getCellCenter(*chs.begin());
		const auto& v1 = vertices_[vhs[0]];
		const auto& v2 = vertices_[vhs[1]];
		const auto& v3 = vertices_[vhs[2]];
		const auto& v4 = vertices_[vhs[3]];
		auto face_center = (v1 + v2 + v3 + v4) / 4;
		auto out_dir = face_center - cell_cenetr;
		out_dir = out_dir.normalized();
		auto vec13 = v3 - v1;
		auto vec12 = v2 - v1;
		auto vec14 = v4 - v1;
		auto vec23 = v3 - v2;
		auto vec24 = v4 - v2;

		auto N123 = vec12 % vec13;
		double area1 = N123.norm() * 0.5;
		N123 = N123.normalized();
		if (N123 * out_dir < 0) N123 = N123 * -1;

		auto N134 = vec14 % vec13;
		double area2 = N134.norm() * 0.5;
		N134 = N134.normalized();
		if (N134 * out_dir < 0) N134 = N134 * -1;

		auto N124 = vec12 % vec14;
		double area3 = N124.norm() * 0.5;
		N124 = N124.normalized();
		if (N124 * out_dir < 0) N124 = N124 * -1;

		auto N234 = vec23 % vec24;
		double area4 = N234.norm() * 0.5;
		N234 = N234.normalized();
		if (N234 * out_dir < 0) N234 = N234 * -1;
		N = (N123 * area1 + N134 * area2 + N124 * area3 + N234 * area4);
		N = N.normalized();
		return N;
	}

	void VolumeMesh::updateAllHandles() {

		std::vector<MeshKernel::iGameVertex> newVertices;
		std::vector<std::vector<MeshKernel::iGameVertexHandle>> newCells;
		std::unordered_map<int, int> mp;     
		int idx = 0;
		for (auto& cp : cells_) {
			auto vhs = cp.second.getVertexHandle();
			for (auto& vh : vhs) {
				if (!mp.count(vh)) {
					mp[vh] = idx++;
					newVertices.push_back(vertices_[vh]);
				}
				vh = iGameVertexHandle(mp[vh]);
			}
			newCells.push_back(vhs);
		}

		*this = MeshKernel::VolumeMesh(newVertices, newCells);

	}

}

namespace MeshKernel {

	void TetMesh::InitHedra(const std::vector<iGameVertex>& _vertices, std::vector<std::vector<std::vector<iGameVertexHandle>>>& _cells,
		const std::vector<std::vector<iGameVertexHandle>>& _surface_faces) {
		for (auto v : _vertices) {
			auto vh = AddVertex(iGameVertex(v.x(), v.y(), v.z()));
		}
		for (auto ce : _cells) {
			AddCell(ce);
		}
		surface_faces_ = _surface_faces;
	}

	void TetMesh::genNormal(iGameFaceHandle fh) {
		auto& face = faces(fh);
		auto vex = face.getVertexHandle();
		std::vector<Eigen::Vector3d> pos(3, Eigen::Vector3d::Zero());
		for (int i = 0; i < 3; ++i) {
			auto& v = vertices(vex[i]);
			pos[i][0] = v.x();
			pos[i][1] = v.y();
			pos[i][2] = v.z();
		}
		Eigen::Vector3d N = (pos[1] - pos[0]).cross(pos[2] - pos[0]);
		N.normalize();
		face.setNormal(N[0], N[1], N[2]);
	}

	void TetMesh::genNormal(iGameVertexHandle vh) {
		auto& v = vertices(vh);
		Eigen::Vector3d N = Eigen::Vector3d::Zero();
		auto adjFH = NeighborFh(vh);
		for (auto& fh : adjFH) {
			auto& face = faces(fh);
			Eigen::Vector3d faceN = Eigen::Vector3d(face.getNormalX(), face.getNormalY(), face.getNormalZ());
			N += faceN;
		}
		N /= adjFH.size();
		N.normalize();
		v.setNormal(N[0], N[1], N[2]);
	}

	void TetMesh::genAllFacesNormal() {
		for (auto& fp : this->faces_) {
			if (isOnBoundary(fp.first)) {
				genNormal(fp.first);
			}
		}
	}

	void TetMesh::genAllVerticesNormal() {
		this->genAllFacesNormal();
		for (auto& vp : this->vertices_) {
			if (isOnBoundary(vp.first)) {
				genNormal(vp.first);
			}
		}
	}

}

namespace MeshKernel {
	
}


