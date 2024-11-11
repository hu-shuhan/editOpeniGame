#ifndef iGameTetraDecimation_h
#define iGameTetraDecimation_h

#include "iGameFaceTable.h"
#include "iGameFilter.h"
#include "iGameModel.h"
#include "iGamePriorityQueue.h"
#include "iGameVolumeMesh.h"
#include <unordered_map>
#include <iGameQuadric.h>

IGAME_NAMESPACE_BEGIN
class TetraDecimation : public Filter {
public:
	I_OBJECT(TetraDecimation);
	static Pointer New() { return new TetraDecimation; }

	void SetModel(Model::Pointer model) { this->model = model; }

	bool Execute() override {
		clock_t begin = clock();
		UpdateProgress(0);
		mesh = DynamicCast<VolumeMesh>(GetInput(0));
		if (mesh == nullptr) {
			return false;
		}
		mesh->RequestEditStatus();

		std::cout << mesh->GetNumberOfPoints() << std::endl;
		std::cout << mesh->GetNumberOfEdges() << std::endl;
		std::cout << mesh->GetNumberOfFaces() << std::endl;
		std::cout << mesh->GetNumberOfVolumes() << std::endl << std::endl;

		optimalPos.resize(mesh->GetNumberOfFaces() * 10);

		for (int i = 0; i < mesh->GetNumberOfFaces(); i++)
		{
			this->InsertFaceToHeap(i);
		}

		int needEliminatedNum = mesh->GetNumberOfFaces() / 2;
		for (int totalEliminated = 0; totalEliminated < 12000; totalEliminated++)
		{
			heap->update();
			if (heap->empty()) {
				break;
			}
			igIndex faceId = heap->top().handle;
			double pri = heap->top().priority;
			heap->pop();

			igIndex f[3];
			mesh->GetFacePointIds(faceId, f);

			if (!IsCollapsableFace(faceId))
				continue;

			igIndex faceIds[256]{};
			for (int i = 0; i < 3; i++) {
				int size = mesh->GetPointToNeighborFaces(f[i], faceIds);
				for (int j = 0; j < size; j++) {
					heap->remove(faceIds[j]);
				}
			}

			this->CollapseFace(faceId);

			int newId = mesh->GetNumberOfPoints() - 1;
			// std::cout << mesh->IsBoundaryPoint(f[0]) << std::endl;
			int size = mesh->GetPointToNeighborFaces(newId, faceIds);
			for (int i = 0; i < size; i++) {
				this->InsertFaceToHeap(faceIds[i], true);
			}
		}

		mesh->GarbageCollection(true);
		std::cout << mesh->GetNumberOfPoints() << std::endl;
		std::cout << mesh->GetNumberOfEdges() << std::endl;
		std::cout << mesh->GetNumberOfFaces() << std::endl;
		std::cout << mesh->GetNumberOfVolumes() << std::endl;

		SurfaceMesh::Pointer out = SurfaceMesh::New();
		out->SetPoints(mesh->GetPoints());
		CellArray::Pointer faces = CellArray::New();
		mesh->RequestEditStatus();

		std::cout << clock() - begin << std::endl;

		Point planePoint{ 0.464607, 0.159425, - 0.010579 };
		//std::cout << planePoint << std::endl;
		Vector3f normal(1, 0, 0);
		//for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
		//	if (mesh->IsBoundaryFace(i)) {
		//		igIndex face[3]{};
		//		int size = mesh->GetFacePointIds(i, face);
		//		faces->AddCellIds(face, 3);
		//	}
		//}
		for (int i = 0; i < mesh->GetNumberOfVolumes(); i++) {
			igIndex v[4]{};
			int size = mesh->GetVolumePointIds(i, v);
			bool f = true;
			for (int j = 0; j < 4; j++) {
				if (SignedDistanceToPlane(mesh->GetPoint(v[j]), planePoint, normal) < 0) {
					f = false;
					break;
				}
			}
			if (f) {
				igIndex ff[4]{};
				int size = mesh->GetVolumeFaceIds(i, ff);
				for (int j = 0; j < 4; j++) {
					igIndex face[3]{};
					mesh->GetFacePointIds(ff[j], face);
					faces->AddCellIds(face, 3);
				}

			}
		}


		out->SetFaces(faces);
		out->SetName(mesh->GetName() + "*");
		SetOutput(0, out);


		UpdateProgress(1);
		return true;
	}

protected:
	TetraDecimation() {
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
		heap = PriorityQueue::New();
		ids = IdArray::New();
		faceIds = IdArray::New();
		origPri = DoubleArray::New();
	}
	~TetraDecimation() override = default;

	double SignedDistanceToPlane(const Point& p, const Point& planePoint,
		const Vector3f& normal) {
		Vector3f diff = p - planePoint;
		return DotProduct(diff, normal);
	}

	bool CheckFlip(const Point& p1, const Point& p2, const Point& pA,
		const Point& pB, const Point& pC) {
		Vector3f v1 = pB - pA;
		Vector3f v2 = pC - pA;
		Vector3f normal = CrossProduct(v1, v2);

		double d1 = SignedDistanceToPlane(p1, pA, normal);
		double d2 = SignedDistanceToPlane(p2, pA, normal);

		return (d1 * d2 < 0);
	}

	bool IsCollapsableFace(igIndex faceId) {
		igIndex f[3], ids[256];
		mesh->GetFacePointIds(faceId, f);
		Point newPoint = optimalPos[faceId];

		std::map<igIndex, int> volumeIds;
		for (int i = 0; i < 3; i++) {
			int size = mesh->GetPointToNeighborVolumes(f[i], ids);
			for (int j = 0; j < size; j++) {
				igIndex volume[4]{};
				mesh->GetVolumePointIds(ids[j], volume);
				int count = 0, a;
				for (int u = 0; u < 4; u++) {
					for (int v = 0; v < 3; v++) {
						if (f[v] == volume[u]) {
							count++;
							a = u;
							break;
						}
					}
				}
				if (count == 1) {
					volumeIds.insert(std::make_pair(ids[j], a));
				}
			}
		}

		ElementArray<Point>::Pointer vec = ElementArray<Point>::New();
		for (auto& id : volumeIds) {
			igIndex volume[4]{};
			mesh->GetVolumePointIds(id.first, volume);
			Point m;
			vec->Reset();
			for (int i = 0; i < 4; i++) {
				if (i == id.second) {
					m = mesh->GetPoint(volume[i]);
				}
				else {
					vec->AddElement(mesh->GetPoint(volume[i]));
				}
			}
			if (CheckFlip(m, newPoint, vec->GetElement(0), vec->GetElement(1),
				vec->GetElement(2))) {
				return false;
			}
		}

		// FaceTable::Pointer volumes = FaceTable::New();
		// for (auto id : volumeIds) {
		//	igIndex volume[4]{};
		//	mesh->GetVolumePointIds(id, volume);
		//	Vector<igIndex, 4> tempv(volume);
		//	int count = 0;
		//	for (int i = 0; i < 4; i++) {
		//		for (int j = 0; j < 3; j++) {
		//			if (f[j] == volume[i]) {
		//				tempv[i] = f[0];
		//				count++;
		//				break;
		//			}
		//		}
		//	}
		//	if (count == 1) {
		//		if (volumes->IsFace(tempv.pointer(), 4) != -1) {
		//			return false;
		//		}
		//		else {
		//			volumes->InsertFace(tempv.pointer(), 4);
		//		}
		//	}
		// }

		return true;
	}

	void CollapseFace(igIndex faceId) {
		igIndex f[3], ids[256]{};
		std::vector<Vector<igIndex, 4>> vec;
		mesh->GetFacePointIds(faceId, f);

		//Point v0 = mesh->GetPoint(f[0]);
		//Point v1 = mesh->GetPoint(f[1]);
		//Point v2 = mesh->GetPoint(f[2]);
		//mesh->SetPoint(f[0], (v0 + v1 + v2) / 3);
		mesh->SetPoint(f[0], optimalPos[faceId]);

		std::set<igIndex> edgeIds;
		std::set<igIndex> faceIds;
		std::set<igIndex> volumeIds;
		for (int i = 0; i < 3; i++) {
			int size = mesh->GetPointToNeighborEdges(f[i], ids);
			for (int j = 0; j < size; j++) {
				edgeIds.insert(ids[j]);
			}
			size = mesh->GetPointToNeighborFaces(f[i], ids);
			for (int j = 0; j < size; j++) {
				faceIds.insert(ids[j]);
			}
			size = mesh->GetPointToNeighborVolumes(f[i], ids);
			for (int j = 0; j < size; j++) {
				volumeIds.insert(ids[j]);
			}
		}

		EdgeTable::Pointer edges = EdgeTable::New();
		std::vector<int> edgeMap;
		for (auto id : edgeIds) {
			igIndex e[2]{};
			mesh->GetEdgePointIds(id, e);
			Vector<igIndex, 2> tempe(e);
			int count = 0;
			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 3; j++) {
					if (f[j] == e[i]) {
						tempe[i] = f[0];
						count++;
						break;
					}
				}
			}
			if (count == 1) {
				if (edges->IsEdge(tempe[0], tempe[1]) == -1) {
					edges->InsertEdge(tempe[0], tempe[1]);
					edgeMap.push_back(id);
				}
			}
		}

		FaceTable::Pointer faces = FaceTable::New();
		std::vector<int> faceMap;
		for (auto id : faceIds) {
			igIndex face[3]{};
			mesh->GetFacePointIds(id, face);
			Vector<igIndex, 3> tempf(face);
			int count = 0;

			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					if (f[j] == face[i]) {
						tempf[i] = f[0];
						count++;
						break;
					}
				}
			}
			if (count == 1) {
				if (faces->IsFace(tempf.pointer(), 3) == -1) {
					faces->InsertFace(tempf.pointer(), 3);
					faceMap.push_back(id);
				}
			}
		}

		FaceTable::Pointer volumes = FaceTable::New();
		std::vector<int> volMap;
		std::set<int> unable;
		for (auto id : volumeIds) {
			igIndex volume[4]{};
			mesh->GetVolumePointIds(id, volume);
			Vector<igIndex, 4> tempv(volume);
			int count = 0;
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 3; j++) {
					if (f[j] == volume[i]) {
						tempv[i] = f[0];
						count++;
						break;
					}
				}
			}
			if (count == 1) {
				int idx{};
				if ((idx = volumes->IsFace(tempv.pointer(), 4)) == -1) {
					volumes->InsertFace(tempv.pointer(), 4);
					volMap.push_back(id);
					vec.push_back(tempv);
				}
				else {
					unable.insert(idx);
					// igIndex fids[4]{}, t[2]{};
					// mesh->GetVolumeFaceIds(id, fids);
					// for (int i = 0; i < 4; i++) {
					//	int size = mesh->GetFaceToNeighborVolumes(fids[i], t);
					//	if (size == 2 && ((t[0] == id && t[1] == idx) || (t[1] == id &&
					//t[0] == idx))) { 		mesh->DeleteFace(fids[i]); 		break;
					//	}
					// }
				}
			}
		}

		for (auto id : edgeIds) {
			mesh->DeleteEdge(id);
		}

		auto edges_ = edges->GetOutput();
		for (int i = 0; i < edges_->GetNumberOfCells(); i++) {
			igIndex e[2]{};
			edges_->GetCellIds(i, e);
			mesh->ReplaceEdgeReference(edgeMap[i], e[0], e[1]);
		}

		auto faces_ = faces->GetOutput();
		for (int i = 0; i < faces_->GetNumberOfCells(); i++) {
			igIndex face[3]{};
			faces_->GetCellIds(i, face);
			igIndex edgeIds[3]{};
			mesh->GetFaceEdgeIds(faceMap[i], edgeIds);
			for (int j = 0; j < 3; j++) {
				int idx;
				if ((idx = edges->IsEdge(face[j], face[(j + 1) % 3])) != -1) {
					edgeIds[j] = edgeMap[idx];
				}
				else {
					edgeIds[j] = mesh->GetEdgeIdFormPointIds(face[j], face[(j + 1) % 3]);
				}
			}
			mesh->ReplaceFaceReference(faceMap[i], face, 3, edgeIds);
		}

		auto volumes_ = volumes->GetOutput();
		for (int i = 0; i < volumes_->GetNumberOfCells(); i++) {
			if (unable.count(i) > 0)
				continue;
			igIndex v[4]{};
			volumes_->GetCellIds(i, v);
			igIndex edgeIds[6]{};
			igIndex faceIds[4]{};
			const igIndex* index;
			mesh->GetVolumeEdgeIds(volMap[i], edgeIds);
			mesh->GetVolumeFaceIds(volMap[i], faceIds);
			for (int j = 0; j < 6; j++) {
				Tetra::EdgePointIds(j, index);
				int idx;
				if ((idx = edges->IsEdge(v[index[0]], v[index[1]])) != -1) {
					edgeIds[j] = edgeMap[idx];
				}
				// else {
				//	edgeIds[j] = mesh->GetEdgeIdFormPointIds(v[index[0]],
				//v[index[1]]);
				// }
			}
			for (int j = 0; j < 4; j++) {
				Tetra::FacePointIds(j, index);
				int idx;
				igIndex face[3]{ v[index[0]], v[index[1]], v[index[2]] };
				if ((idx = faces->IsFace(face, 3)) != -1) {
					faceIds[j] = faceMap[idx];
				}
				// else {
				//	faceIds[j] = mesh->GetFaceIdFormPointIds(face, 3);
				// }
			}
			mesh->ReplaceVolumeReference(volMap[i], v, 4, edgeIds, 6, faceIds, 4);
		}
	}

	void CollapseFace2(igIndex faceId) {
		igIndex f[3], ids[256]{};
		std::vector<Vector<igIndex, 4>> vec;
		mesh->GetFacePointIds(faceId, f);

		int newId = mesh->AddPoint(optimalPos[faceId]);

		std::set<igIndex> volumeIds;
		for (int i = 0; i < 3; i++) {
			int size = mesh->GetPointToNeighborVolumes(f[i], ids);
			for (int j = 0; j < size; j++) {
				volumeIds.insert(ids[j]);
			}
		}

		FaceTable::Pointer volumes = FaceTable::New();
		//std::set<int> unable;
		for (auto id : volumeIds) {
			igIndex volume[4]{};
			mesh->GetVolumePointIds(id, volume);
			Vector<igIndex, 4> tempv(volume);
			int count = 0;
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 3; j++) {
					if (f[j] == volume[i]) {
						tempv[i] = newId;
						count++;
						break;
					}
				}
			}
			if (count == 1) {
				int idx{};
				if ((idx = volumes->IsFace(tempv.pointer(), 4)) == -1) {
					volumes->InsertFace(tempv.pointer(), 4);
					vec.push_back(tempv);
				}
				//else {
				//	unable.insert(idx);
				//}
			}
		}

		mesh->DeletePoint(f[0]);
		mesh->DeletePoint(f[1]);
		mesh->DeletePoint(f[2]);

		std::vector<igIndex> ve;
		for (int i = 0; i < vec.size(); i++) {
			// if (unable.count(i) > 0) continue;
			ve.push_back(mesh->AddVolume(vec[i].pointer(), 4));
		}
	}

	bool InsertFaceToHeap(igIndex faceId, bool f = false) {

		int type = EvaluateFace(faceId);

		double priority = 0.0;
		
		if (type == 0) { // 内部三角形
			priority = ComputePriority(faceId, type);
		}
		else if (type == 1) {

		}
		else if (type == 2) {
			priority = ComputePriority(faceId, type);
		}
		else if (type == 3) {
			
		}

		if (priority != 0.0) {
			heap->push(-priority, faceId, 1);
			return true;
		}
		return false;
	}

	int EvaluateFace(igIndex faceId) {
		int type = 0;
		igIndex f[3];
		mesh->GetFacePointIds(faceId, f);
		type += mesh->IsBoundaryPoint(f[0]) ? 1 : 0;
		type += mesh->IsBoundaryPoint(f[1]) ? 1 : 0;
		type += mesh->IsBoundaryPoint(f[2]) ? 1 : 0;

		return type;
	}

	double ComputePriority2(igIndex faceId) {
		igIndex f[3], fe[3], e[2];
		mesh->GetFacePointIds(faceId, f);
		mesh->GetFaceEdgeIds(faceId, fe);
		igIndex edgeId;
		for (int i = 0; i < 3; i++) {
			if (mesh->IsBoundaryEdge(fe[i])) {
				edgeId = fe[i];
				break;
			}
		}
		mesh->GetEdgePointIds(edgeId, e);
		Point v0 = mesh->GetPoint(e[0]);
		Point v1 = mesh->GetPoint(e[1]);
		Quadric<double> q = QuadricEdge(edgeId);

		//IdArray::Pointer faceIds2 = IdArray::New();
		igIndex fid[2]{}, faceIds[256]{};
		int k = 0;
		int size = mesh->GetEdgeToNeighborFaces(edgeId, faceIds);
		for (int i = 0; i < size; i++) {
			if (mesh->IsBoundaryFace(faceIds[i])) {
				fid[k++] = faceIds[i];
			}
		}
		
		//k = 0;
		//mesh->GetEdgeToOneRingFaces(edgeId, faceIds2);
		//for (int i = 0; i < faceIds2->GetNumberOfIds(); i++) {
		//	if (mesh->IsBoundaryFace(faceIds2->GetId(i))) {
		//		int id = faceIds2->GetId(i);
		//		faceIds2->SetId(k, id);
		//		k++;
		//	}
		//}

		//double origQuality = std::numeric_limits<double>::max();
		//for (int i = 0; i < k; ++i) {
		//	auto id = faceIds2->GetId(i);
		//	if (id != fid[0] && id != fid[1]) {
		//		origQuality = std::min(origQuality, QualityFace(id));
		//	}
		//}

		optimalPos[faceId] = ComputePosition(edgeId, q);
		mesh->SetPoint(e[0], optimalPos[faceId]);
		mesh->SetPoint(e[1], optimalPos[faceId]);

		//double newQuality = std::numeric_limits<double>::max();
		//for (int i = 0; i < k; ++i) {
		//	auto id = faceIds2->GetId(i);
		//	if (id != fid[0] && id != fid[1]) {
		//		newQuality = std::min(newQuality, QualityFace(id));
		//	}
		//}

		mesh->SetPoint(e[0], v0);
		mesh->SetPoint(e[1], v1);

		double error = q.apply(optimalPos[edgeId]);
		//double priority = error / newQuality;
		return error;
	}

	Vector3d ComputePosition(igIndex edgeId, Quadric<double>& q)
	{
		igIndex e[2];
		mesh->GetEdgePointIds(edgeId, e);
		Vector3d newPos = (mesh->GetPoint(e[0]) + mesh->GetPoint(e[1])) / 2.0;
		
		if (q.apply(newPos) > 1e-15) {
			q.findMinimum(newPos);
		}
		return newPos;
	}

	Quadric<double> QuadricEdge(igIndex edgeId)
	{
		igIndex e[2];
		mesh->GetEdgePointIds(edgeId, e);

		IdArray::Pointer faceIds = IdArray::New();
		Quadric<double> qs[2];
		for (int i = 0; i < 2; i++) {
			mesh->GetPointToNeighborFaces(e[0], faceIds);
			qs[i].setZero();
			for (int j = 0; j < faceIds->GetNumberOfIds(); j++) {
				if (mesh->IsBoundaryFace(faceIds->GetId(j))) {
					qs[i] += QuadricFace(faceIds->GetId(j));
				}
			}
		}

		Quadric<double> q = qs[0];
		q += qs[1];
		return q;
	}

	Quadric<double> QuadricFace(igIndex faceId)
	{
		igIndex f[3];
		mesh->GetFacePointIds(faceId, f);
		Point v0 = mesh->GetPoint(f[0]);
		Point v1 = mesh->GetPoint(f[1]);
		Point v2 = mesh->GetPoint(f[2]);

		Vector3f d10 = v1 - v0;
		Vector3f d20 = v2 - v0;

		Vector3d normal = CrossProduct(d10, d20);
		double d = normal.dot(v0);
		Quadric<double> q;
		q.byPlane(normal, -d);
		return q;
	}

	double QualityFace(igIndex faceId)
	{
		igIndex f[3];
		mesh->GetFacePointIds(faceId, f);
		Point v0 = mesh->GetPoint(f[0]);
		Point v1 = mesh->GetPoint(f[1]);
		Point v2 = mesh->GetPoint(f[2]);

		Vector3f d10 = v1 - v0;
		Vector3f d20 = v2 - v0;
		Vector3f d12 = v1 - v2;

		Vector3f normal = CrossProduct(d10, d20);

		double a = normal.norm();
		if (a == 0) return 0;
		double b = std::max(d10.squaredNorm(), std::max(d20.squaredNorm(), d12.squaredNorm()));
		if (b == 0) return 0;
		return a / b;
	}

	double ComputePriority(igIndex faceId, int type) {
		igIndex f[3];
		mesh->GetFacePointIds(faceId, f);
		Point v0 = mesh->GetPoint(f[0]);
		Point v1 = mesh->GetPoint(f[1]);
		Point v2 = mesh->GetPoint(f[2]);

		ids->Reset();
		faceIds->Reset();
		mesh->GetFaceToOneRingFaces(faceId, ids);
		for (int i = 0; i < ids->Size(); i++) {
			igIndex nei_f[3];
			mesh->GetFacePointIds(ids->GetId(i), nei_f);
			int count = 0;
			for (int m = 0; m < 3; m++) {
				for (int n = 0; n < 3; n++) {
					if (f[m] == nei_f[n]) {
						count++;
						break;
					}
				}
			}
			if (count == 1) {
				faceIds->AddId(ids->GetId(i));
			}
		}

		origPri->Reset();
		for (int i = 0; i < faceIds->Size(); i++) {
			origPri->AddValue(this->GetTriFormFactor(faceIds->GetId(i)));
		}

		Point newPos = (v0 + v1 + v2) / 3;
		if (type == 1) {
			for (int i = 0; i < 3; i++) {
				if (mesh->IsBoundaryPoint(f[i])) {
					newPos = mesh->GetPoint(f[i]);
					break;
				}
			}
		}
		else if (type == 2) {
			for (int i = 0; i < 3; i++) {
				if (!mesh->IsBoundaryPoint(f[i])) {
					newPos = (mesh->GetPoint(f[(i + 1) % 3]) + mesh->GetPoint(f[(i + 2) % 3])) / 2;
					break;
				}
			}
		}
		
		optimalPos[faceId] = newPos;

		mesh->SetPoint(f[0], newPos);
		mesh->SetPoint(f[1], newPos);
		mesh->SetPoint(f[2], newPos);

		double priority = 0.0;
		for (int i = 0; i < faceIds->Size(); i++) {
			priority = std::max(priority,
				std::abs(origPri->GetValue(i) -
					this->GetTriFormFactor(faceIds->GetId(i))));
		}

		mesh->SetPoint(f[0], v0);
		mesh->SetPoint(f[1], v1);
		mesh->SetPoint(f[2], v2);

		return priority;
	}

	double GetTriFormFactor(igIndex faceId) {
		igIndex face[3]{};
		int size = mesh->GetFacePointIds(faceId, face);
		Point v0 = mesh->GetPoint(face[0]);
		Point v1 = mesh->GetPoint(face[1]);
		Point v2 = mesh->GetPoint(face[2]);

		double a = (v0 - v1).length();
		double b = (v0 - v2).length();
		double c = (v1 - v2).length();

		if (a == 0.0 || b == 0.0 || c == 0.0) {
			return 0.0;
		}

		double q = (a + b + c) / 2;

		return 4 * (q - a) * (q - b) * (q - c) / (a * b * c);
	}

	Model::Pointer model;
	VolumeMesh::Pointer mesh;
	PriorityQueue::Pointer heap;
	std::vector<Vector3d> optimalPos;
	IdArray::Pointer ids, faceIds;
	DoubleArray::Pointer origPri;
};
IGAME_NAMESPACE_END
#endif