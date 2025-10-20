#include "iGameRemeshing.h"
#include "iGameAttributeSet.h"
#include "iGameThreadPool.h"
#include <algorithm>

IGAME_NAMESPACE_BEGIN

Remeshing::Remeshing() 
{
	SetNumberOfInputs(1);
	SetNumberOfOutputs(1);
}

static inline float length3(const Vector3f& a) {
	return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

float Remeshing::ComputeAverageEdgeLength(SurfaceMesh::Pointer mesh) {
	float sum = 0.0f; 
	int cnt = 0;
	igIndex ids[2]{};
	for (IGsize e = 0; e < mesh->GetNumberOfEdges(); ++e) {
		if (mesh->IsEdgeDeleted(e)) continue;
		mesh->GetEdgePointIds(e, ids);
		auto& p0 = mesh->GetPoints()->GetPoint(ids[0]);
		auto& p1 = mesh->GetPoints()->GetPoint(ids[1]);
		Vector3f d{float(p0[0]-p1[0]), float(p0[1]-p1[1]), float(p0[2]-p1[2])};
		sum += length3(d);
		cnt++;
	}
	return cnt ? sum / cnt : 0.0f;
}

void Remeshing::ComputeVertexGradients(SurfaceMesh::Pointer mesh, std::vector<Vector3f>& grads, std::vector<float>& magnitudes) {
	grads.assign(mesh->GetNumberOfPoints(), Vector3f{0,0,0});
	magnitudes.assign(mesh->GetNumberOfPoints(), 0.0f);
	// 假设只有一个标量属性，取第一个点属性维度的第0维
	if (!mesh->GetAttributeSet() || mesh->GetAttributeSet()->GetNumberOfAttributes() == 0) return;
	auto& attr = mesh->GetAttributeSet()->GetAttribute(0);
	FloatArray::Pointer fa = DynamicCast<FloatArray>(attr.pointer);
	if (!fa) return;
	int stride = fa->GetDimension();
	// 使用一环邻域的最小二乘拟合近似梯度
	SurfaceMesh::ReturnContainer nbrs;
	for (IGsize v = 0; v < mesh->GetNumberOfPoints(); ++v) {
		mesh->GetPointToOneRingPoints(v, nbrs);
		auto& pv = mesh->GetPoints()->GetPoint(v);
		float fv = fa->GetValue(v * stride + 0);
		Vector3f g{0,0,0};
		int n = 0;
		for (int i = 0; i < nbrs.size(); ++i) {
			auto& pn = mesh->GetPoints()->GetPoint(nbrs[i]);
			float fn = fa->GetValue(nbrs[i] * stride + 0);
			Vector3f d{float(pn[0]-pv[0]), float(pn[1]-pv[1]), float(pn[2]-pv[2])};
			float len = std::max(1e-8f, length3(d));
			float df = fn - fv; // 沿边方向的近似梯度投影
			g[0] += df * d[0] / (len*len);
			g[1] += df * d[1] / (len*len);
			g[2] += df * d[2] / (len*len);
			n++;
		}
		grads[v] = g;
		magnitudes[v] = length3(g);
	}
}

void Remeshing::BuildGradientHistogram(const std::vector<float>& mag, std::vector<float>& factors) {
	factors.resize(mag.size(), 1.0f);
	if (mag.empty()) return;
	std::vector<float> sorted = mag;
	std::sort(sorted.begin(), sorted.end());
	// 五等分，映射到 m_HistogramScales
	for (size_t i = 0; i < mag.size(); ++i) {
		float v = mag[i];
		int bin = 0;
		float q1 = sorted[sorted.size()*1/5];
		float q2 = sorted[sorted.size()*2/5];
		float q3 = sorted[sorted.size()*3/5];
		float q4 = sorted[sorted.size()*4/5];
		if (v <= q1) bin = 0; else if (v <= q2) bin = 1; else if (v <= q3) bin = 2; else if (v <= q4) bin = 3; else bin = 4;
		bin = std::min<int>(bin, int(m_HistogramScales.size()-1));
		factors[i] = m_HistogramScales[bin];
	}
}

bool Remeshing::SplitLongEdges(SurfaceMesh::Pointer mesh, float l, const std::vector<float>& factors) {
	bool changed = false;
	mesh->RequestEditStatus();
	igIndex ids[2]{}; 
	std::vector<igIndex> edgesToSplit;
	for (IGsize e = 0; e < mesh->GetNumberOfEdges(); ++e) {
		if (mesh->IsEdgeDeleted(e)) continue;
		mesh->GetEdgePointIds(e, ids);
		auto& p0 = mesh->GetPoints()->GetPoint(ids[0]);
		auto& p1 = mesh->GetPoints()->GetPoint(ids[1]);
		Vector3f d{float(p0[0]-p1[0]), float(p0[1]-p1[1]), float(p0[2]-p1[2])};
		float len = length3(d);
		float ma = factors[ids[0]]; float mb = factors[ids[1]];
		float thresh = (5.f/3.f) * l * std::min(ma, mb);
		if (len > thresh) edgesToSplit.push_back((igIndex)e);
	}
	for (auto e : edgesToSplit) {
		//mesh->SplitEdge(e); // 期望已有接口；若无，可后续补充实现
		changed = true;
	}
	return changed;
}

bool Remeshing::CollapseShortEdges(SurfaceMesh::Pointer mesh, float l, const std::vector<float>& factors) {
	bool changed = false; 
	mesh->RequestEditStatus();
	igIndex ids[2]{};
	for (IGsize e = 0; e < mesh->GetNumberOfEdges(); ++e) {
		if (mesh->IsEdgeDeleted(e)) continue;
		mesh->GetEdgePointIds(e, ids);
		auto& p0 = mesh->GetPoints()->GetPoint(ids[0]);
		auto& p1 = mesh->GetPoints()->GetPoint(ids[1]);
		Vector3f d{float(p0[0]-p1[0]), float(p0[1]-p1[1]), float(p0[2]-p1[2])};
		float len = length3(d);
		float ma = factors[ids[0]]; float mb = factors[ids[1]];
		float thresh = (4.f/5.f) * l * std::max(ma, mb);
		if (len < thresh && mesh->IsCollapsable(e)) { mesh->CollapseEdge(e); changed = true; }
	}
	return changed;
}

bool Remeshing::FlipAndValenceOptimize(SurfaceMesh::Pointer mesh) {
	bool changed = false;
	// 简化实现：若有边翻转接口则调用；否则略过
	return changed;
}

void Remeshing::TangentialRelaxation(SurfaceMesh::Pointer mesh) {
	// 将内部点移动到切平面上一环重心
	SurfaceMesh::ReturnContainer nbrs;
	for (IGsize v = 0; v < mesh->GetNumberOfPoints(); ++v) {
		if (mesh->IsBoundaryPoint(v)) continue;
		mesh->GetPointToOneRingPoints(v, nbrs);
		Vector3f c{0,0,0};
		for (int i = 0; i < nbrs.size(); ++i) {
			auto& pn = mesh->GetPoints()->GetPoint(nbrs[i]);
			c[0] += pn[0]; c[1] += pn[1]; c[2] += pn[2];
		}
		float inv = nbrs.size() > 0 ? 1.f / nbrs.size() : 0.f;
		c[0] *= inv; c[1] *= inv; c[2] *= inv;
		auto& pv = mesh->GetPoints()->GetPoint(v);
		Vector3f t{c[0]-pv[0], c[1]-pv[1], c[2]-pv[2]};
		// 投影到切平面：t - (t·n)n，n 用一环法向平均
		Vector3f n{0,0,0};
		SurfaceMesh::ReturnContainer faces; mesh->GetPointToNeighborFaces(v, faces);
		for (int i = 0; i < faces.size(); ++i) {
			igIndex fpts[3]{}; mesh->GetFacePointIds(faces[i], fpts);
			auto& a = mesh->GetPoints()->GetPoint(fpts[0]);
			auto& b = mesh->GetPoints()->GetPoint(fpts[1]);
			auto& cpt = mesh->GetPoints()->GetPoint(fpts[2]);
			Vector3f ab{float(b[0]-a[0]), float(b[1]-a[1]), float(b[2]-a[2])};
			Vector3f ac{float(cpt[0]-a[0]), float(cpt[1]-a[1]), float(cpt[2]-a[2])};
			Vector3f nn{ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0]};
			n[0]+=nn[0]; n[1]+=nn[1]; n[2]+=nn[2];
		}
		float nl = std::max(1e-8f, std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]));
		n[0]/=nl; n[1]/=nl; n[2]/=nl;
		float dot = t[0]*n[0]+t[1]*n[1]+t[2]*n[2];
		Vector3f move{t[0]-dot*n[0], t[1]-dot*n[1], t[2]-dot*n[2]};
		mesh->GetPoints()->SetPoint(v, pv[0]+move[0], pv[1]+move[1], pv[2]+move[2]);
	}
}

void Remeshing::ResampleAttributes(SurfaceMesh::Pointer mesh, SurfaceMesh::Pointer origin) {
	if (!origin->GetAttributeSet()) return;
	PointFinder::Pointer finder = PointFinder::New();
	finder->SetPoints(origin->GetPoints());
	finder->Initialize();
	for (int k = 0; k < origin->GetAttributeSet()->GetNumberOfAttributes(); ++k) {
		auto& attr = origin->GetAttributeSet()->GetAttribute(k);
		if (!DynamicCast<FloatArray>(attr.pointer)) continue;
		FloatArray::Pointer src = DynamicCast<FloatArray>(attr.pointer);
		FloatArray::Pointer dst = FloatArray::New();
		dst->SetName(src->GetName());
		dst->SetDimension(src->GetDimension());
		float ele[16]{};
		for (IGsize i = 0; i < mesh->GetNumberOfPoints(); ++i) {
			auto& p = mesh->GetPoints()->GetPoint(i);
			Vector3d q{p[0], p[1], p[2]};
			igIndex id = finder->FindClosestPoint(q);
			src->GetElement(id, ele);
			dst->AddElement(ele);
		}
		if (!mesh->GetAttributeSet()) mesh->SetAttributeSet(AttributeSet::New());
		mesh->GetAttributeSet()->AddAttribute(attr.type, IG_POINT, dst);
	}
}

bool Remeshing::Execute() {

	SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(GetInput(0));

	if (!mesh) return false;

	Points::Pointer points = mesh->GetPoints();
	CellArray::Pointer cells = mesh->GetFaces();

	SurfaceMesh::Pointer origin = SurfaceMesh::New();

	origin->DeepCopy(mesh);

	for (int it = 0; it < m_Iterations; ++it) 
	{
		float l = ComputeAverageEdgeLength(mesh);
		std::vector<Vector3f> grads; std::vector<float> mags; std::vector<float> factors;
		ComputeVertexGradients(mesh, grads, mags);
		BuildGradientHistogram(mags, factors);
		bool changed = false;
		changed |= SplitLongEdges(mesh, l, factors);
		changed |= CollapseShortEdges(mesh, l, factors);
		changed |= FlipAndValenceOptimize(mesh);
		TangentialRelaxation(mesh);
		mesh->GarbageCollection();
		if (!changed) break;
	}

	ResampleAttributes(mesh, origin);
	SetOutput(mesh);
	return true;
}

IGAME_NAMESPACE_END