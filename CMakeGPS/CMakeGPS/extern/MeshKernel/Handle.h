#pragma once
#include <iostream>

namespace MeshKernel { 
	class iGameHandle {
	public:
		explicit iGameHandle(int _idx) : idx_(_idx) {};
		iGameHandle& operator=(const iGameHandle& _h) {
			idx_ = _h.idx_;
			return *this;
		}

		iGameHandle& operator=(int _idx) {
			idx_ = _idx;
			return *this;
		}

		inline bool is_valid() const { return idx_ != -1; }
		inline bool operator<(const iGameHandle& _h) const { return (this->idx_ < _h.idx_); }

		inline bool operator<(int _idx) const { return idx_ < _idx; }

		inline bool operator>(const iGameHandle& _h) const { return (this->idx_ > _h.idx_); }

		inline bool operator>(int _idx) const { return idx_ > _idx; }

		inline bool operator==(const iGameHandle& _h) const { return _h.idx_ == this->idx_; }

		inline bool operator!=(const iGameHandle& _h) const { return _h.idx_ != this->idx_; }

		inline const int& idx() const { return idx_; }      

		void idx(const int& _idx) { idx_ = _idx; }      

		inline operator int() const { return idx_; } 

		void reset() { idx_ = -1; }  

	private:
		int idx_;
	};

	class iGameVertexHandle : public iGameHandle { public: explicit iGameVertexHandle(int _idx = -1) : iGameHandle(_idx) {} };
	class iGameEdgeHandle : public iGameHandle { public: explicit iGameEdgeHandle(int _idx = -1) : iGameHandle(_idx) {} };
	class iGameFaceHandle : public iGameHandle { public: explicit iGameFaceHandle(int _idx = -1) : iGameHandle(_idx) {} };
	class iGameCellHandle : public iGameHandle { public: explicit iGameCellHandle(int _idx = -1) : iGameHandle(_idx) {} };
}

namespace std
{
	template<>
	struct hash<MeshKernel::iGameVertexHandle>
	{
		size_t operator()(const MeshKernel::iGameVertexHandle& h)const
		{
			return hash<int>()(h.idx());
		}
	};
	template<>
	struct hash<MeshKernel::iGameEdgeHandle>
	{
		size_t operator()(const MeshKernel::iGameEdgeHandle& h)const
		{
			return hash<int>()(h.idx());
		}
	};
	template<>
	struct hash<MeshKernel::iGameFaceHandle>
	{
		size_t operator()(const MeshKernel::iGameFaceHandle& h)const
		{
			return hash<int>()(h.idx());
		}
	};
	template<>
	struct hash<MeshKernel::iGameCellHandle>
	{
		size_t operator()(const MeshKernel::iGameCellHandle& h)const
		{
			return hash<int>()(h.idx());
		}
	};

}






