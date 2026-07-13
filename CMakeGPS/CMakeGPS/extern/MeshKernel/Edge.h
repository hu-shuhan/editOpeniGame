#pragma once
#include "Handle.h"
#include "Vertex.h"
#include <iostream>


namespace MeshKernel { 
	class iGameEdge {
	public:
		iGameEdge() :vertices_(2, (iGameVertexHandle)-1) {}
		iGameEdge(const iGameEdge& _e) {
			vertices_ = _e.vertices_;
		}
		iGameEdge(const iGameVertexHandle& _vh1, const iGameVertexHandle& _vh2) :
			vertices_{ _vh1,_vh2 } {
		}

		inline iGameEdge& operator=(const iGameEdge& _e) {
			vertices_ = _e.vertices_;
			return *this;
		}
		inline bool operator==(const iGameEdge& _e) const {
			return ((vh1() == _e.vh1() && vh2() == _e.vh2())
				|| (vh1() == _e.vh2() && vh2() == _e.vh1()));
		}

		const iGameVertexHandle vh1() const { return vertices_[0]; }
		const iGameVertexHandle vh2() const { return vertices_[1]; }
		iGameVertexHandle& vh1() { return vertices_[0]; }
		iGameVertexHandle& vh2() { return vertices_[1]; }

		const iGameVertexHandle vh(int k) const { 
			assert(k <= 1 && k >= 0);
			return vertices_[k]; 
		}
		iGameVertexHandle& vh(int k) {
			assert(k <= 1 && k >= 0);
			return vertices_[k];
		}

		inline void SortVertices() {
			if (vertices_[0] > vertices_[1]) {
				std::swap(vertices_[0], vertices_[1]);
			}
		}

		inline void setWeight(double w) {
			weight = w;
		}

		inline double getWeight() {
			return weight;
		}

		inline void setLength(double l) {
			length = l;
		}

		inline double getLength() {
			return length;
		}

	private:
		std::vector<iGameVertexHandle> vertices_;
		double weight = 0.f;
		double length = 0.f;

	};

}

namespace std
{
	template<> struct hash<MeshKernel::iGameEdge>
	{
		size_t operator()(const MeshKernel::iGameEdge& _e)const
		{
			if (_e.vh1() <= _e.vh2()) {
				return hash<int>()(_e.vh1()) ^ hash<int>()(_e.vh2());
			}
			else {
				return hash<int>()(_e.vh2()) ^ hash<int>()(_e.vh1());
			}
			
		}
	};
}