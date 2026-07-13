#pragma once
#include "Cell.h"
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Sparse>

#ifndef IGMAE_KERNEL_SIMPLIFY
#define IGMAE_KERNEL_SIMPLIFY
typedef MeshKernel::iGameVertex Vex;
typedef MeshKernel::iGameVertex Vec;
typedef MeshKernel::iGameVertexHandle VH;
typedef MeshKernel::iGameEdgeHandle EH;
typedef MeshKernel::iGameFaceHandle FH;
typedef MeshKernel::iGameCellHandle CH;
#endif

namespace MeshKernel { 
	class Mesh {
	public:
		iGameVertex BBoxMin, BBoxMax;
		

		void initBBox();
		inline bool isValid(iGameVertexHandle _vh) { return vertices_.count(_vh); }
		inline bool isValid(iGameEdgeHandle _eh) { return edges_.count(_eh); }
		inline bool isValid(iGameFaceHandle _fh) { return faces_.count(_fh); }


		iGameVertex& vertices(iGameVertexHandle _vh);
		const iGameVertex vertices(iGameVertexHandle _vh) const;               
		iGameEdge& edges(iGameEdgeHandle _eh);
		const iGameEdge& edges(iGameEdgeHandle _eh) const;
		iGameFace& faces(iGameFaceHandle _fh);
		const iGameFace faces(iGameFaceHandle _fh) const;

		size_t vsize() const { return vertices_.size(); }
		size_t esize() const { return edges_.size(); }
		size_t fsize() const { return faces_.size(); }
		const std::unordered_map<iGameVertexHandle, iGameVertex>& allvertices() const { return vertices_; }
		const std::unordered_map<iGameEdgeHandle, iGameEdge>& alledges() const { return edges_; }
		const std::unordered_map<iGameFaceHandle, iGameFace>& allfaces() const { return faces_; }

		const iGameVertexHandle vertexhanle(iGameVertex _vertex) const;
		const iGameEdgeHandle edgehandle(iGameEdge& _edge) const;
		const iGameFaceHandle facehandle(iGameFace& _face) const;
		std::unordered_set<iGameVertexHandle> NeighborVh(iGameVertexHandle _vh);
		std::unordered_set<iGameEdgeHandle>& NeighborEh(iGameVertexHandle _vh);
		std::unordered_set<iGameFaceHandle>& NeighborFh(iGameVertexHandle _vh);
		std::unordered_set<iGameEdgeHandle> NeighborEh(iGameEdgeHandle _eh);
		std::unordered_set<iGameFaceHandle>& NeighborFh(iGameEdgeHandle _eh);
		std::unordered_set<iGameFaceHandle> NeighborFh(iGameFaceHandle _fh);   
		std::unordered_set<iGameFaceHandle> Neighbor2Fh(iGameFaceHandle _fh);   

        iGameVertexHandle NeighborVhFromEdge(iGameVertexHandle _vh, iGameEdgeHandle _eh);
        iGameEdgeHandle NeighborEhFromVertex(iGameEdgeHandle _eh, iGameVertexHandle _vh);

        iGameFaceHandle NeighborFhFromEdge(iGameFaceHandle _fh, iGameEdgeHandle _eh);
        iGameEdgeHandle OppositeEhFromEdge(iGameFaceHandle _fh, iGameEdgeHandle _eh);
        iGameEdgeHandle getEhFromTwoVh(iGameVertexHandle _vh1, iGameVertexHandle _vh2);

        iGameVertex getEdgeVector(iGameEdge _e);

		iGameVertexHandle AddVertex(const iGameVertex& _v);
		iGameEdgeHandle AddEdge(const iGameVertexHandle& _vh1, const iGameVertexHandle& _vh2);
		iGameFaceHandle AddFace(const std::vector<iGameVertexHandle>& _vhs);

		iGameVertexHandle DeleteVertex(iGameVertexHandle _vh);
		iGameEdgeHandle DeleteEdge(iGameEdgeHandle _eh);
		iGameFaceHandle DeleteFace(iGameFaceHandle _fh);


		iGameVertexHandle GenVertexHandle() { return (iGameVertexHandle)VertexHandleID_++; }
		iGameEdgeHandle GenEdgeHandle() { return (iGameEdgeHandle)EdgeHandleID_++; }
		iGameFaceHandle GenFaceHandle() { return (iGameFaceHandle)FaceHandleID_++; }

		size_t VertexSize() { return vertices_.size(); }
		size_t EdgeSize() {return edges_.size(); }
		size_t FaceSize() { return faces_.size(); }

		iGameVertex getFaceCenter(iGameFaceHandle fh);
		iGameVertex getEdgeMidpoint(iGameEdgeHandle eh);

		void genAllEdgesLength();
		void genLength(iGameEdgeHandle);
		double getLength(iGameEdgeHandle);

		bool isConnected(iGameFaceHandle fh1, iGameFaceHandle fh2);
		bool isConnected(iGameEdgeHandle eh1, iGameEdgeHandle eh2);
		bool isConnected(iGameVertexHandle vh1, iGameVertexHandle vh2);

		iGameEdgeHandle getEdgeHandle(iGameVertexHandle vh1, iGameVertexHandle vh2);


	protected:
		int VertexHandleID_ = 0;
		int EdgeHandleID_ = 0;
		int FaceHandleID_ = 0;
		std::unordered_set<iGameVertexHandle> empty_vhs;
		std::unordered_set<iGameEdgeHandle> empty_ehs;
		std::unordered_set<iGameFaceHandle> empty_fhs;

		void AddFace2Neighbor(const iGameFaceHandle& _fh);
		void AddEdge2Neighbor(const iGameEdgeHandle& _eh);
		void DeleteFace2Neighbor(const iGameFaceHandle& _fh);
		void DeleteEdge2Neighbor(const iGameEdgeHandle& _eh);
	protected:
		std::unordered_map<iGameVertexHandle, iGameVertex> vertices_;
		std::unordered_map<iGameEdgeHandle, iGameEdge> edges_;
		std::unordered_map<iGameFaceHandle, iGameFace> faces_;

		std::unordered_map<iGameVertex, iGameVertexHandle> Vertex2Vh_;
		std::unordered_map<iGameEdge, iGameEdgeHandle> Edge2Eh_;
		std::unordered_map<iGameFace, iGameFaceHandle> Face2Fh_;

		std::unordered_map<iGameVertexHandle, std::unordered_set<iGameEdgeHandle>> NeighborEhOfVertex_;          
		std::unordered_map<iGameVertexHandle, std::unordered_set<iGameFaceHandle>> NeighborFhOfVertex_;          
		std::unordered_map<iGameEdgeHandle, std::unordered_set<iGameFaceHandle>> NeighborFhOfEdge_;              
	protected:
		virtual void InitMesh(const std::vector<iGameVertex>& _vertices,
			const std::vector<std::vector<iGameVertexHandle>>& _elements) = 0;                               
		Mesh& operator=(const Mesh& _mesh);
	};

	class SurfaceMesh : public Mesh {
	public:
		SurfaceMesh() {};
		SurfaceMesh(const std::vector<iGameVertex>& _vertices, const std::vector<std::vector<iGameVertexHandle>>& _faces) {
			InitMesh(_vertices, _faces);
		}
		virtual void InitMesh(const std::vector<iGameVertex>& _vertices,
			const std::vector<std::vector<iGameVertexHandle>>& _elements) override;
		SurfaceMesh& operator=(const SurfaceMesh& _surfacemesh);

		bool isOnBoundary(iGameEdgeHandle);
		bool isOnBoundary(iGameVertexHandle);
		bool isOnBoundary(iGameFaceHandle);

		bool hasLoopBoundary();

		void genNormal(iGameFaceHandle);
		void genNormal(iGameVertexHandle);
		void genAllFacesNormal();
		void genAllVerticesNormal(); 

		void updateAllHandles();

		bool isTriangleMesh();
		size_t getBoundaryVerticesCount();
	};

	class VolumeMesh : public Mesh {
	public:
		VolumeMesh() {};
		VolumeMesh(const std::vector<iGameVertex>& _vertices, const std::vector<std::vector<iGameVertexHandle>>& _cells) {
			InitMesh(_vertices, _cells);
		}
		
		virtual void InitMesh(const std::vector<iGameVertex>& _vertices,
			const std::vector<std::vector<iGameVertexHandle>>& _elements) override;
		VolumeMesh& operator=(const VolumeMesh& _volumemesh);

	public:
		iGameCell& cells(iGameCellHandle _ch);
		const iGameCell cells(iGameCellHandle _ch) const;
		inline bool isValid(iGameVertexHandle _vh) { return vertices_.count(_vh); }
		inline bool isValid(iGameEdgeHandle _eh) { return edges_.count(_eh); }
		inline bool isValid(iGameFaceHandle _fh) { return faces_.count(_fh); }
		inline bool isValid(iGameCellHandle _ch) { return cells_.count(_ch); }

		size_t csize() const { return cells_.size(); }
		size_t CellSize() const { return cells_.size(); }
		const std::unordered_map <iGameCellHandle, iGameCell> & allcells() const { return cells_; }
		
		const iGameCellHandle cellhandle(iGameCell& _cell) const;

		std::unordered_set<iGameCellHandle> NeighborCh(iGameVertexHandle _vh); 
		std::unordered_set<iGameCellHandle> NeighborCh(iGameEdgeHandle _eh); 
		std::unordered_set<iGameCellHandle> NeighborCh(iGameFaceHandle _fh); 
		std::unordered_set<iGameCellHandle> NeighborCh(iGameCellHandle _ch); 

		iGameCellHandle AddCell(const std::vector<iGameVertexHandle>& _vhs); 
		iGameCellHandle AddCell(const std::vector< std::vector<iGameVertexHandle>>& _vhs);

		iGameVertexHandle DeleteVertex(const iGameVertexHandle& _vh); 
		iGameEdgeHandle DeleteEdge(const iGameEdgeHandle& _eh); 
		iGameFaceHandle DeleteFace(const iGameFaceHandle& _fh); 
		iGameCellHandle DeleteCell(const iGameCellHandle& _ch); 

		iGameCellHandle GenCellHandle() { return (iGameCellHandle)CellHandleID_++; }

		void updateAllHandles();

		bool isOnBoundary(iGameCellHandle ch);
		bool isOnBoundary(iGameFaceHandle fh);
		bool isOnBoundary(iGameEdgeHandle eh);
		bool isOnBoundary(iGameVertexHandle vh);

		
		iGameVertex getCellCenter(iGameCellHandle ch);

		iGameVertex getQuadNormal(iGameFaceHandle fh);
		double getQuadArea(iGameFaceHandle fh);

		std::vector<std::vector<iGameVertexHandle>> surface_faces_; 
	protected:
		int CellHandleID_ = 0;
		std::unordered_set<iGameCellHandle> empty_chs;
		
		void AddCell2Neighbor(const iGameCellHandle& _ch);
		void DeleteCell2Neighbor(const iGameCellHandle& _ch);

		std::unordered_map<iGameCellHandle, iGameCell> cells_;
		std::unordered_map<iGameCell, iGameCellHandle> Cell2Ch_;

		std::unordered_map<iGameVertexHandle, std::unordered_set<iGameCellHandle>> NeighborChOfVertex_;          
		std::unordered_map<iGameEdgeHandle, std::unordered_set<iGameCellHandle>> NeighborChOfEdge_;              
		std::unordered_map<iGameFaceHandle, std::unordered_set<iGameCellHandle>> NeighborChOfFace_;              
	};

	class TriMesh : SurfaceMesh{};
	class QuadMesh : SurfaceMesh{};

	class TetMesh : public VolumeMesh {
	public:
		TetMesh(const std::vector<iGameVertex>& _vertices, const std::vector<std::vector<iGameVertexHandle>>& _cells) {
			InitMesh(_vertices, _cells);
			printf("init tetrahedron success\n");
		}
		TetMesh(const std::vector<iGameVertex>& _vertices, std::vector<std::vector<std::vector<iGameVertexHandle>>>& _cells,
			const std::vector<std::vector<iGameVertexHandle>>& _surface_faces) {
			InitHedra(_vertices, _cells, _surface_faces);
			printf("init tetrahedron success\n");
		}

		void InitHedra(const std::vector<iGameVertex>& _vertices, std::vector<std::vector<std::vector<iGameVertexHandle>>>& _cells,
			const std::vector<std::vector<iGameVertexHandle>>& _surface_faces);
		void genNormal(iGameFaceHandle);
		void genNormal(iGameVertexHandle);
		void genAllFacesNormal();
		void genAllVerticesNormal();
		
	};


	class HexMesh : VolumeMesh{};
}