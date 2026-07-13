#ifndef GEOMETRY_KERNEL_H
#define GEOMETRY_KERNEL_H
#include "HexTopologyKernel.h"

class V3f
{
public:
	V3f()
	{
		x = 0;
		y = 0;
		z = 0;
	}
	V3f(double x, double y, double z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
	inline bool operator==(const V3f& _v) const
	{
		if (sqrt((_v.x - x)*(_v.x - x) + (_v.y - y)*(_v.y - y) + (_v.z - z)*(_v.z - z)) < 1e-7)
			return true;
		else
			return false;
	}
	inline V3f operator%(const V3f& _rhs) const 
	{
		return
			V3f(y * _rhs.z - z * _rhs.y,
				z * _rhs.x - x * _rhs.z,
				x * _rhs.y - y * _rhs.x);
	}
	inline V3f operator+(const V3f& _rhs) const
	{
		return V3f(x+ _rhs.x,y+ _rhs.y,z+ _rhs.z);
	}
	inline V3f operator-(const V3f& _rhs) const
	{
		return V3f(x - _rhs.x, y - _rhs.y, z - _rhs.z);
	}
	inline V3f operator/(const double normal) const
	{
		return V3f(x / normal, y / normal, z / normal);
	}
	inline V3f operator*(const double normal) const
	{
		return V3f(x * normal, y * normal, z * normal);
	}
	inline double operator^(const V3f& _rhs) const 
	{
		return x * _rhs.x + y * _rhs.y + z * _rhs.z;
	}
	inline bool operator<(const V3f& _v) const 
	{
		if (fabs(x - _v.x) > 1.0e-7)
			return x < _v.x;
		else
		{
			if (fabs(y - _v.y) > 1.0e-7)
				return y < _v.y;
			else
			{
				if (fabs(z - _v.z) > 1.0e-7)
					return z < _v.z;
				else
					return false;
			}
		}
	}
	double x, y, z;
	bool is_valid = true;
	int lastid;
	int bezierid = 0;
public:
	inline double norm()
	{
		return sqrt(x*x + y*y + z*z);
	}
	inline double sqrnorm()
	{
		return sqrt(sqrt(x*x + y*y + z*z));
	}
	inline V3f normalize()
	{
		double norm = this->norm();
		return V3f(x / norm, y / norm, z / norm);
	}
};

inline V3f opx(const V3f& _rhs) 
{
	return V3f(_rhs.x, -_rhs.y, _rhs.z);
}
inline V3f opy(const V3f& _rhs) 
{
	return V3f(-_rhs.x, _rhs.y, _rhs.z);
}
inline V3f op0(const V3f& _rhs) 
{
	return V3f(-_rhs.x, -_rhs.y, _rhs.z);
}
inline double distance(V3f &_v1, V3f _v2)
{
	double dis = sqrt((_v1.x - _v2.x)*(_v1.x - _v2.x) + (_v1.y - _v2.y)*(_v1.y - _v2.y)
		+ (_v1.z - _v2.z)*(_v1.z - _v2.z));
	return dis;
}

namespace std
{
	template<> struct hash<V3f>
	{
		size_t operator()(const V3f &v)const
		{
			return hash<double>()(v.x) ^ hash<double>()(v.y) ^ hash<double>()(v.z);
		}
	};
}

class Edge {
	friend class HexV3fMesh;
public :
	VertexHandle from_h, to_h;
	bool is_valid = true;
	Edge() {

	}
	Edge(const VertexHandle from_h, const VertexHandle to_h) {
		this->from_h = from_h;
		this->to_h = to_h;
	}
	Edge& operator=(const Edge& edge) {
		this->from_h = edge.from_h;
		this->to_h = edge.to_h;
		return *this;
	}
	~Edge() {

	}
	inline bool operator==(const Edge& _e) const 
	{
		if ((_e.from_h == from_h) && (_e.to_h == to_h)) {
			return true;
		}
		else if ((_e.to_h == from_h) && (_e.from_h == to_h)) {
			return true;
		}
		return false;
	}
};
class Face {
	friend class HexV3fMesh;
public:
	std::vector<EdgeHandle> edges_;
	std::vector<VertexHandle> vs_f;
	bool is_valid = true;
	Face() {};
	explicit Face(const std::vector<EdgeHandle>& _edges, std::vector<VertexHandle> vs_):edges_(_edges),vs_f(vs_){
		std::sort(edges_.begin(),edges_.end(),compare_OVM());
	}
	Face& operator=(const Face& f) {
		this->edges_.assign(f.edges_.begin(), f.edges_.end());
		this->vs_f.assign(f.vs_f.begin(), f.vs_f.end());
		return *this;
	}
	~Face() {
	}
	inline bool operator==(const Face& _f) const
	{
		if (_f.vs_f[0].idx() + _f.vs_f[1].idx() + _f.vs_f[2].idx() + _f.vs_f[3].idx() !=
			vs_f[0].idx() + vs_f[1].idx() +  vs_f[2].idx() + vs_f[3].idx())
			return false;
		std::set<VertexHandle, compare_OVM> vs_f_c;
		for (uint16_t i = 0; i < 4; ++i) {
			vs_f_c.insert(_f.vs_f[i]);
		}
		for (uint16_t i = 0; i < 4; ++i) {
			vs_f_c.insert(vs_f[i]);
		}
		if (vs_f_c.size() == 4) return true;
		return false;
	}
};
class Cell {
	friend class HexV3fMesh;
public:
	std::vector<FaceHandle> faces_;
	std::vector<VertexHandle> vertex_;
	std::vector<EdgeHandle> edges_;
	bool is_valid = true;
	Cell() {};
	explicit Cell(const std::vector<FaceHandle>& _faces, const std::vector<VertexHandle>& _vs) :faces_(_faces),vertex_(_vs){
	}
	explicit Cell(const std::vector<FaceHandle>& _faces, const std::vector<VertexHandle>& _vs,
		const std::vector<EdgeHandle>& _es) :faces_(_faces), vertex_(_vs), edges_(_es){}
	Cell& operator=(const Cell& cell) {
		this->faces_.assign(cell.faces_.begin(), cell.faces_.end());
		this->vertex_.assign(cell.vertex_.begin(), cell.vertex_.end());
		this->edges_.assign(cell.edges_.begin(), cell.edges_.end());
		return *this;
	}
	~Cell() {
	}
	inline bool operator==(const Cell& cell) const
	{
		std::set<VertexHandle, compare_OVM> vs_c_c;
		for (uint16_t i = 0; i < 8; ++i) {
			vs_c_c.insert(cell.vertex_[i]);
		}
		for (uint16_t i = 0; i < 4; ++i) {
			vs_c_c.insert(vertex_[i]);
		}
		if (vs_c_c.size() == 8) return true;
		return false;
	}
};
	
class triangleMesh {
public:
	std::vector<V3f> vertices;
	std::vector<std::vector<int>> faces;
	std::unordered_map<int, std::vector<int>> e;
	std::unordered_map<int, std::vector<int>> ef;
	std::unordered_map<int, std::vector<int>> fe;
	void ReorderMesh();
	void DFS(int f, int v1, int v2);
	std::vector<bool> flag;

};

class HexV3fMesh {
public:
	std::map<V3f, int> quick_v;
	int dis_v = 0;
	std::vector<V3f> vertices_;
	std::vector<Edge> edges_;
	std::vector<Face> faces_;
	std::vector<Cell> cells_;
		
	std::vector<bool> v_bdy;

	std::vector<V3f> v_c_crease;
	std::vector<V3f> v_crease;
	std::vector<V3f> v3f_singular;
	std::vector<bool> v_cs;

	std::vector<std::set<EdgeHandle,compare_OVM>> neighbor_v;
	std::vector<std::set<FaceHandle, compare_OVM>> neighbor_v_f;
	std::vector<std::set<CellHandle, compare_OVM>> neighbor_v_c;
	std::vector<std::set<FaceHandle,compare_OVM>> neighbor_e;
	std::vector<std::set<CellHandle,compare_OVM>> neighbor_f;
	std::set<CellHandle> neighbor_e_c(EdgeHandle _e)
	{
		std::set<CellHandle> temp;
		std::set<FaceHandle, compare_OVM> ef = neighbor_e[_e];
		for (std::set<FaceHandle, compare_OVM>::iterator ef_it = ef.begin();ef_it != ef.end();ef_it++)
		{
			std::set<CellHandle, compare_OVM> efc = neighbor_f[*ef_it];
			for (std::set<CellHandle, compare_OVM>::iterator efc_it = efc.begin();efc_it != efc.end();efc_it++)
			{
				temp.insert(*efc_it);
			}
		}
		return temp;
	}
	std::set<CellHandle> neighbor_c_c(CellHandle _c)
	{
		std::set<CellHandle> temp;
		int cf_size = cells_[_c].faces_.size();
		for (int i = 0;i < cf_size;i++)
		{
			std::set<CellHandle, compare_OVM> cfc = neighbor_f[cells_[_c].faces_[i]];
			for (std::set<CellHandle, compare_OVM>::iterator cfc_it = cfc.begin();cfc_it != cfc.end();cfc_it++)
			{
				if (*cfc_it != _c)
				{
					temp.insert(*cfc_it);
					break;
				}
			}
		}
		return temp;
	}
	std::set<EdgeHandle> neighbor_v_bdy(VertexHandle _v)
	{
		std::set<EdgeHandle> v_e_bdy;
		std::set<EdgeHandle, compare_OVM> v_e = neighbor_v[_v];
		for (std::set<EdgeHandle, compare_OVM>::iterator v_e_it = v_e.begin();v_e_it != v_e.end();v_e_it++)
		{
			if (is_bdy(*v_e_it))
				v_e_bdy.insert(*v_e_it);
		}
		return v_e_bdy;
	}
	std::set<FaceHandle> neighbor_e_bdy(EdgeHandle _e)
	{
		std::set<FaceHandle> e_f_bdy;
		std::set<FaceHandle, compare_OVM> e_f = neighbor_e[_e];
		for (std::set<FaceHandle, compare_OVM>::iterator e_f_it = e_f.begin();e_f_it != e_f.end();e_f_it++)
		{
			if (is_bdy(*e_f_it))
				e_f_bdy.insert(*e_f_it);
		}
		return e_f_bdy;
	}

	std::vector<std::vector<V3f>> boundary_face_v;	
	std::vector<FaceHandle> boundary_face;
	std::vector<std::vector<int>> boundary_face_VID;
	std::vector<V3f> normal_boundary_face_v;
	std::vector<CellHandle> boundary_cellid;
	std::vector<V3f> oriv;
	V3f center;
	double r;
	double average_r;
	std::vector<V3f> cell_center;
	std::vector<double> cell_r;
	double minEdgeLen;
	double maxEdgeLen;
	V3f bbMin; 
	V3f bbMax;

	HexV3fMesh() {
			
	}
	~HexV3fMesh() {

	}
	HexV3fMesh(const HexV3fMesh& _mesh) {
		this->vertices_ = _mesh.vertices_;
		this->edges_ = _mesh.edges_;
		this->faces_ = _mesh.faces_;
		this->cells_ = _mesh.cells_;

		this->v_bdy = _mesh.v_bdy;
		this->neighbor_v = _mesh.neighbor_v;
		this->neighbor_v_c = _mesh.neighbor_v_c; 
		this->neighbor_v_f = _mesh.neighbor_v_f;
		this->neighbor_e = _mesh.neighbor_e;
		this->neighbor_f = _mesh.neighbor_f;
	}

	static const VertexHandle   InvalidVertexHandle;
	static const EdgeHandle     InvalidEdgeHandle;
	static const FaceHandle     InvalidFaceHandle;
	static const CellHandle     InvalidCellHandle;

	VertexHandle add_vertex(const V3f& _p);

	FaceHandle add_face(const std::vector<VertexHandle>& v_ve);
	CellHandle add_cell(const std::vector<VertexHandle>& v_ve);

	CellHandle add_cell(const std::vector<FaceHandle>& f_ve);

	double cal_angle(EdgeHandle e1, EdgeHandle e2, VertexHandle v)
	{
		V3f n1, n2;
		if (edges_[e1].from_h == v)
			n1 = vertices_[edges_[e1].to_h] - vertices_[v] ;
		else
			n1 =vertices_[edges_[e1].from_h] - vertices_[v];
		if (edges_[e2].from_h == v)
			n2 = vertices_[edges_[e2].to_h] - vertices_[v];
		else
			n2 = vertices_[edges_[e2].from_h] - vertices_[v];
		double temp = (n1.x * n2.x + n1.y * n2.y + n1.z * n2.z) / (n1.norm() * n2.norm());
		return acos(temp);
	}

	void examine_bdy() {
		v_bdy.resize(vertices_.size(),false);
		for (uint64_t i = 0; i < neighbor_f.size(); ++i) {
			if (neighbor_f[i].size() == 1) {
				for (uint16_t i_v = 0; i_v < 4; ++i_v) {
					v_bdy[faces_[i].vs_f[i_v]] = true;
				}
			}
		}
	}

	void examine_cs()
	{
		v_cs.resize(vertices_.size(), false);
	}

	bool is_cs(const VertexHandle& v_)
	{
		return v_cs[v_];
	}

	bool is_cs(const EdgeHandle& e_)
	{
		if (v_cs[edges_[e_].from_h] == true && v_cs[edges_[e_].to_h] == true)
			return true;
		else
			return false;
	}

	bool is_bdy(const VertexHandle& v_) {
		return v_bdy[v_];
	}
	bool is_bdy(const EdgeHandle& e_) {
		std::set<FaceHandle, compare_OVM> ef =neighbor_e[e_];
		for (std::set<FaceHandle, compare_OVM>::iterator ef_it = ef.begin();ef_it != ef.end();ef_it++)
		{
			if (is_bdy(*ef_it))
				return true;
		}
		return false;
	}
	bool is_bdy(const FaceHandle& f_) {
		return neighbor_f[f_].size()==1 ;
	}
	bool is_bdy(const FaceHandle& f_ , int) { 
		std::vector<VertexHandle> fv = faces_[f_].vs_f;
		for (std::vector<VertexHandle>::iterator fv_it = fv.begin();fv_it != fv.end();fv_it++)
		{
			if (v_bdy[*fv_it] == false)
				return false;
		}
		return true;
	}
	bool is_bdy(const CellHandle& c_) {
		for (uint16_t i = 0; i < 6; ++i) {
			if (is_bdy(cells_[c_].faces_[i])) {
				return true;
			}
		}
		return false;
	}

	bool non_intersect(const EdgeHandle& e1,const EdgeHandle& e2) {
		Edge edge_1 = this->edges_[e1];
		Edge edge_2 = this->edges_[e2];
		if (
			((edge_1.from_h != edge_2.from_h) && (edge_1.from_h != edge_2.to_h)) &&
			((edge_1.to_h != edge_2.from_h) && (edge_1.to_h != edge_2.to_h))
			) {
			return true;
		}
		return false;
	}

	V3f baryCenter(const CellHandle& cellHandle) {
		V3f v(0.00f,0.00f,0.00f);
		std::vector<VertexHandle> set_v = this->cells_[cellHandle].vertex_;
		for (uint16_t i = 0; i < 8; ++i) {
			v.x += this->vertices_[set_v[i]].x/8;
			v.y += this->vertices_[set_v[i]].y/8;
			v.z += this->vertices_[set_v[i]].z/8;
		}
		return v;
	}

	FaceHandle adjacent_face_in_cell(const CellHandle& cellHandle, const EdgeHandle& edgeHandle, const FaceHandle& faceHandle) {
		std::vector<FaceHandle> faces = this->cells_[cellHandle].faces_;

		for (uint16_t i = 0; i < 6; ++i) {
			if (faces[i] == faceHandle) continue;
			std::vector<EdgeHandle> e_vec = this->faces_[faces[i]].edges_;
			if (std::count(e_vec.begin(), e_vec.end(), edgeHandle) != 0) {
				return faces[i];
			}
			
		}
		return InvalidFaceHandle;
	}

	uint8_t get_orientaion(FaceHandle face,CellHandle cell) {
		return std::distance(this->cells_[cell].faces_.begin(),
			std::find(this->cells_[cell].faces_.begin(), this->cells_[cell].faces_.end(), face));
	}

	FaceHandle get_opposite_face_in_cell(FaceHandle face, CellHandle cell) {
		uint8_t ori = get_orientaion(face,cell);
		return cells_[cell].faces_[ori % 2 ? ori - 1 : ori + 1];
	}

	EdgeHandle get_opposite_edge_in_face(EdgeHandle edge, FaceHandle face) {
		const std::vector<EdgeHandle>& edge_vec = faces_[face].edges_;
		for (uint8_t i = 0; i < 4; ++i) {
			if (
				((edges_[edge_vec[i]].from_h != edges_[edge].from_h) && (edges_[edge_vec[i]].to_h != edges_[edge].to_h))
				&& ((edges_[edge_vec[i]].from_h != edges_[edge].to_h) && (edges_[edge_vec[i]].to_h != edges_[edge].from_h))
				)return edge_vec[i];
		}
		return EdgeHandle(-1);
	}

	std::vector<EdgeHandle> get_3edge_in_corner(VertexHandle v , CellHandle cell) {
		
		std::vector<EdgeHandle> edge_vec;
		std::set<EdgeHandle, compare_OVM> edge_cell;
		for (uint8_t i = 0; i < 6; ++i) {
			std::vector<EdgeHandle> edges = faces_[cells_[cell].faces_[i]].edges_;
			for (uint8_t j = 0; j < 4; ++j) {
				edge_cell.insert(edges[j]);
			}
		}
		for (std::set<EdgeHandle, compare_OVM>::iterator iter = edge_cell.begin(); iter != edge_cell.end(); ++iter) {
			Edge edge = edges_[*iter];
			if ((edge.from_h == v) || (edge.to_h == v)) {
				edge_vec.push_back(*iter);
			}
		}
		return edge_vec;
	}

	std::vector<FaceHandle> get_3face_in_corner(VertexHandle v, CellHandle cell) {
		std::vector<FaceHandle> face;
		for (uint8_t i = 0; i < 6; ++i) {
			Face f = faces_[cells_[cell].faces_[i]];
			if (std::find(f.vs_f.begin(), f.vs_f.end(), v)!=f.vs_f.end()) {
				face.push_back(cells_[cell].faces_[i]);
			}
		}
		return face;
	}

	std::vector<FaceHandle> get_2face_edge(EdgeHandle edge, CellHandle cell) {
		std::vector<FaceHandle> _2face;
		std::vector<FaceHandle> c_f = cells_[cell].faces_;
		for (std::vector<FaceHandle>::iterator c_f_it = c_f.begin(); c_f_it != c_f.end(); c_f_it++)
		{
			if (is_e_in_f(edge, *c_f_it))
				_2face.push_back(*c_f_it);
		}
		return _2face;
	}
	std::vector<EdgeHandle> get_parallel_edges(EdgeHandle edge, CellHandle cell) {
		std::vector<EdgeHandle> _4edge;
		std::vector<FaceHandle> face_vec = get_2face_edge(edge, cell);
		face_vec.push_back(get_opposite_face_in_cell(face_vec[0],cell));
		face_vec.push_back(get_opposite_face_in_cell(face_vec[1], cell));

		std::set<EdgeHandle, compare_OVM> set;
		uint8_t count_temp = 0;
		for (uint8_t i = 0; i < face_vec.size(); ++i) {
			Face face = this->faces_[face_vec[i]];
			for (uint8_t j = 0; j < 4; ++j) {
				set.insert(face.edges_[j]);
				if (set.size() == count_temp) { 
					_4edge.push_back(face.edges_[j]);
				}
				else {
					++count_temp;
				}
			}
		}
		return _4edge;
	}

	VertexHandle find_op_vertex(VertexHandle _v, FaceHandle _f);
	

	bool is_singular(EdgeHandle _e);
	bool is_singular(VertexHandle _v);
	bool is_f_in_c(FaceHandle _f, CellHandle _c);
	bool is_e_in_f(EdgeHandle _e, FaceHandle _f);
	bool is_e_in_c(EdgeHandle _e, CellHandle _c);
	bool is_two_e_in_same_face(EdgeHandle e1, EdgeHandle e2);
	bool is_two_e_in_same_cell(EdgeHandle e1, EdgeHandle e2);
	bool is_two_f_in_same_cell(FaceHandle f1, FaceHandle f2);
	bool merge_v();
	CellHandle delete_cell(CellHandle cellHandle);

	void examine_crease();
	int val(VertexHandle v1, VertexHandle v2, bool _is_bdy);
	int val(VertexHandle v, bool _is_bdy);
	int dim(VertexHandle v1, VertexHandle v2);
	void examine_singular();

	void examine_bdfv();
	void cal_cen();
	void cal_cell_cen(); 
	void cal_bdnorm();
	void cal_mami_ed();
	V3f cal_norm(FaceHandle f);
	void cal_oriv()
	{
		int ver_size = vertices_.size();
		for (int i = 0; i < ver_size; i++)
		{
			if(vertices_[i].is_valid == true)
				oriv.push_back(vertices_[i]);
		}
	}
	V3f cal_face_center(FaceHandle f)
	{
		V3f v(0, 0, 0);
		for (int i = 0; i < 4; i++)
		{
			v = v + vertices_[faces_[f].vs_f[i]];
		}
		v = v / 4;
		return v;
	}
	V3f cal_edge_center(EdgeHandle i)
	{
		V3f v1 = vertices_[edges_[i].from_h];
		V3f v2 = vertices_[edges_[i].to_h];
		return (v1 + v2) / 2;
	}
	void smooth_mesh();
};



class OurSkelNode {
public:
	OurSkelNode() {};
	OurSkelNode(int _index, V3f _pos, double _radius, bool _isvalid) :index(_index), pos(_pos), radius(_radius), is_valid(_isvalid) {};
public:
	int index;
	V3f pos;
	double radius; 
	bool is_valid;  
	int Indegree; 
	std::vector<int> neighbor;
	int newIdx = -1;
	double angle = 0.0;  
};

class OurSkel {
public:
	std::vector<OurSkelNode> SkelAll;
	V3f center;
	double r; 
public:
	OurSkel() {};
	OurSkel& operator=(const OurSkel& skel) {
		this->SkelAll.assign(skel.SkelAll.begin(), skel.SkelAll.end());
		this->center = skel.center;
		this->r = skel.r;
		return *this;
	}
	void CalBoundingBox() {
		double average_r = 0.0;
		double cen_x = 0, cen_y = 0, cen_z = 0;
		double max = 0;
		double max_x = -9999, max_y = -9999, max_z = -9999;
		double min_x = 9999, min_y = 9999, min_z = 9999;
		int num = SkelAll.size();
		for (auto it = SkelAll.begin(); it != SkelAll.end(); it++)
		{
			cen_x += (*it).pos.x;
			cen_y += (*it).pos.y;
			cen_z += (*it).pos.z;
			num++;
		}
		center.x = cen_x / num;
		center.y = cen_y / num;
		center.z = cen_z / num;
		for (auto it = SkelAll.begin(); it != SkelAll.end(); it++)
		{
			double maxr = ((*it).pos - center).norm();
			average_r += maxr;
			if (maxr > max)
				max = maxr;
			if ((*it).pos.x > max_x) max_x = (*it).pos.x;
			if ((*it).pos.x < min_x) min_x = (*it).pos.x;
			if ((*it).pos.y > max_y) max_y = (*it).pos.y;
			if ((*it).pos.y < min_y) min_y = (*it).pos.y;
			if ((*it).pos.z > max_z) max_z = (*it).pos.z;
			if ((*it).pos.z < min_z) min_z = (*it).pos.z;
		}
		average_r /= num;
		r = max;
		V3f bbMax = V3f(max_x, max_y, max_z);
		V3f bbMin = V3f(min_x, min_y, min_z);
	}

	int DeleteNode_Skel(int NodeIdx) {
		if (SkelAll[NodeIdx].Indegree != 2) {
			return 0;
		}
		else {
			int former, later;
			former = SkelAll[NodeIdx].neighbor[0];
			later = SkelAll[NodeIdx].neighbor[1];
			for (int i = 0; i < SkelAll[former].neighbor.size(); i++) {
				if (SkelAll[former].neighbor[i] == NodeIdx) {
					SkelAll[former].neighbor[i] = later;
				}
			}
			for (int i = 0; i < SkelAll[later].neighbor.size(); i++) {
				if (SkelAll[later].neighbor[i] == NodeIdx) {
					SkelAll[later].neighbor[i] = former;
				}
			}
			SkelAll[NodeIdx].is_valid = false;
			return 1;
		}
	}
	void CalAllAngle() {
		for (int i = 0; i < SkelAll.size(); i++) {
			if (SkelAll[i].neighbor.size() == 2 && SkelAll[i].is_valid) {
				int former, later;
				former = SkelAll[i].neighbor[0];
				while (!SkelAll[former].is_valid) {
					former = SkelAll[former].neighbor[0];
				}
				later = SkelAll[i].neighbor[1];
				while (!SkelAll[later].is_valid) {
					later = SkelAll[later].neighbor[1];
				}

				auto lvec = SkelAll[former].pos - SkelAll[i].pos;
				auto rvec = SkelAll[later].pos - SkelAll[i].pos;
				double angle = (180 * acos((lvec ^ rvec) / (lvec.norm() * rvec.norm()))) / (3.141592658);
				SkelAll[i].angle = angle;
			}
		}
	}
	void OPDelete() {
		int level = 1;  
		static int levelTop = 9;

		while (level < levelTop) {
			CalAllAngle();
			for (int i = 0; i < SkelAll.size(); i++) {
				if (SkelAll[i].is_valid && SkelAll[i].angle >= 180 - 5 * level) {
					DeleteNode_Skel(i);
				}
			}
			level++;
		}
		levelTop += 3;
	}

	void writeSimpleSkel(std::string OutputFile) {
		OurSkel newskel;
		int tIndex = 0;
		std::vector<int> vv;
		std::map<int, int> vm;
		for (int i = 0; i < SkelAll.size(); i++) {
			if (SkelAll[i].is_valid) {
				vm[i] = tIndex;
				tIndex++;
				vv.push_back(i);
			}
		}
		std::ofstream output2(OutputFile);
		output2 << "ID Cx Cy Cz RADIUS #NEIGHBORS NEIGHBORS_LIST\n";
		output2 << tIndex << "\n";
		for (int i = 0; i < tIndex; ++i) {

			std::vector<int> nn;
			for (auto it : SkelAll[vv[i]].neighbor) {
				if (SkelAll[it].is_valid) {
					nn.push_back(vm[it]);
				}
			}
			output2 << i << " " << SkelAll[vv[i]].pos.x << " " << SkelAll[vv[i]].pos.y << " " << SkelAll[vv[i]].pos.z << " "
				<< SkelAll[vv[i]].radius << " " << nn.size();
			for (auto iter : nn) {
				output2 << " " << iter;
			}
			output2 << "\n";
		}
		output2.close();
	}

};

#endif  
