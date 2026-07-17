#pragma once
#include"Face.h"

namespace MeshKernel { 
	class iGameCell {
	public:
		iGameCell() :n_(0) {}
		iGameCell(const std::vector<iGameVertexHandle>& _vertices, const std::vector<iGameEdgeHandle>& _edges,
			 const std::vector<iGameFaceHandle>& _faces){
			assert(_faces.size() >= 4 && _vertices.size() >= 4);
			n_ = _faces.size();
			vertices_.assign(_vertices.begin(), _vertices.end());
			edges_.assign(_edges.begin(), _edges.end());
			faces_.assign(_faces.begin(), _faces.end());
		}
		iGameCell(const iGameCell& _c) {
			*this = _c;
		}
		inline iGameCell& operator=(const iGameCell& _c) {
			n_ = _c.n_;
			vertices_.assign(_c.vertices_.begin(), _c.vertices_.end());
			edges_.assign(_c.edges_.begin(), _c.edges_.end());
			faces_.assign(_c.faces_.begin(), _c.faces_.end());
			return *this;
		}
		bool operator==(const iGameCell& _c) const {
			return (n_ == _c.n_)  && isSameFaces(_c);
		}
		const iGameVertexHandle vh(int k) const { return vertices_[k]; }           
		const iGameEdgeHandle eh(int k) const { return edges_[k]; }
		const iGameFaceHandle fh(int k) const { return faces_[k]; }
		
		iGameVertexHandle& vh(int k) { return vertices_[k]; }                      
		iGameEdgeHandle& eh(int k) { return edges_[k]; }
		iGameFaceHandle& fh(int k) { return faces_[k]; }
		
		size_t vertices_size() const { return n_; }
		size_t faces_size() const { return faces_.size(); }
		size_t edges_size() const { return edges_.size(); }
		std::vector<iGameVertexHandle> getSortedVertexHandle() const {
			std::vector<iGameVertexHandle> sortedvertexhandle = vertices_;
			std::sort(sortedvertexhandle.begin(), sortedvertexhandle.end());
			return sortedvertexhandle;
		}
		std::vector<iGameVertexHandle> getVertexHandle() {
			std::vector<iGameVertexHandle> res = vertices_;
			return res;
		}
		std::vector<iGameVertexHandle> getVertexHandle() const {
			std::vector<iGameVertexHandle> res = vertices_;
			return res;
		}
		std::vector<iGameFaceHandle> getFaceHandle() {
			std::vector<iGameFaceHandle> res = faces_;
			return res;
		}
		std::vector<iGameFaceHandle> getFaceHandle() const {
			std::vector<iGameFaceHandle> res = faces_;
			return res;
		}
		std::vector<iGameEdgeHandle> getEdgeHandle() {
			std::vector<iGameEdgeHandle> res = edges_;
			return res;
		}
		std::vector<iGameEdgeHandle> getEdgeHandle() const {
			std::vector<iGameEdgeHandle> res = edges_;
			return res;
		}
		int getN() const { 
			return vertices_.size();
		}
		
	private:
		bool isSameFaces(const iGameCell& _c) const {
			std::unordered_set<iGameFaceHandle> fhset;
			for (const auto& f1 : _c.faces_) fhset.insert(f1);
			for (const auto& f2 : faces_) fhset.insert(f2);
			return fhset.size() == n_;
		}
	private:
		std::vector<iGameVertexHandle> vertices_;
		std::vector<iGameEdgeHandle> edges_;
		std::vector<iGameFaceHandle> faces_;
		size_t n_;                     
	};

}

namespace std
{
	template<> struct hash<MeshKernel::iGameCell>
	{
		size_t operator()(const MeshKernel::iGameCell& _c)const
		{
			size_t res = 0;
			assert(_c.vertices_size() >= 4);
			auto cv = _c.getSortedVertexHandle();
			for (int i = 0; i < cv.size(); ++i) {
				res ^= hash<int>()(cv[i]);
			}
			return res;
		}
	};
}