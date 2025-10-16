#ifndef IGAME_REMESHING_H
#define IGAME_REMESHING_H

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGamePointFinder.h"

IGAME_NAMESPACE_BEGIN
class Remeshing : public Filter {
public:
	I_OBJECT(Remeshing);
	static Pointer New() { return new Remeshing; }

	bool Execute() override;

	void SetTargetAverageScale(float s) { m_TargetScale = s; }
	void SetHistogramScales(const std::vector<float>& m) { m_HistogramScales = m; }
	void SetIterationCount(int n) { m_Iterations = n; }

protected:
	Remeshing();
	~Remeshing() override = default;

private:
	// helpers
	float ComputeAverageEdgeLength(SurfaceMesh::Pointer mesh);
	void ComputeVertexGradients(SurfaceMesh::Pointer mesh, std::vector<Vector3f>& grads, std::vector<float>& magnitudes);
	void BuildGradientHistogram(const std::vector<float>& mag, std::vector<float>& factors);
	bool SplitLongEdges(SurfaceMesh::Pointer mesh, float l, const std::vector<float>& factors);
	bool CollapseShortEdges(SurfaceMesh::Pointer mesh, float l, const std::vector<float>& factors);
	bool FlipAndValenceOptimize(SurfaceMesh::Pointer mesh);
	void TangentialRelaxation(SurfaceMesh::Pointer mesh);
	void ResampleAttributes(SurfaceMesh::Pointer mesh, SurfaceMesh::Pointer origin);

	float m_TargetScale{1.0f};
	int m_Iterations{5};
	std::vector<float> m_HistogramScales{1.8f, 1.4f, 1.0f, 0.8f, 0.6f};

	//using Vector = Vector3f;
 //   class Adjacency {
 //   public:
 //       // 三角形中一个顶点的对偶边
 //       struct DualEdge {
 //           int_t next;
 //           int_t prev;
 //       };

 //       std::vector<int_t> Offsets;
 //       std::vector<DualEdge> Data;

 //       int_t Num(int_t id) const { return Offsets[id + 1] - Offsets[id]; }
 //       int_t Begin(int_t id) const { return Offsets[id]; }
 //       int_t End(int_t id) const { return Offsets[id + 1]; }
 //   };
};
IGAME_NAMESPACE_END

#endif


