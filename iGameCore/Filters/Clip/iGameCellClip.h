#include "iGameTetra.h"
#include "iGamePolyhedron.h"
#include "Quadratic/iGameQuadraticTetra.h"
#include "iGameCellArray.h"
#include "iGameAttributeSet.h"
#include"iGameVolumeMesh.h"
IGAME_NAMESPACE_BEGIN

namespace CellClip {

	struct TETRA_CLIP {
		int clip[7];
	};
	static TETRA_CLIP tetraCases[] = {
	  { { 0, 0, 0, 0, 0, 0, 0 } },        // 0
	  { { 4, 0, 3, 2, 100, 0, 0 } },      // 1
	  { { 4, 0, 1, 4, 101, 0, 0 } },      // 2
	  { { 6, 101, 1, 4, 100, 2, 3 } },    // 3
	  { { 4, 1, 2, 5, 102, 0, 0 } },      // 4
	  { { 6, 102, 5, 1, 100, 3, 0 } },    // 5
	  { { 6, 102, 2, 5, 101, 0, 4 } },    // 6
	  { { 6, 3, 4, 5, 100, 101, 102 } },  // 7
	  { { 4, 3, 4, 5, 103, 0, 0 } },      // 8
	  { { 6, 103, 4, 5, 100, 0, 2 } },    // 9
	  { { 6, 103, 5, 3, 101, 1, 0 } },    // 10
	  { { 6, 100, 101, 103, 2, 1, 5 } },  // 11
	  { { 6, 2, 102, 1, 3, 103, 4 } },    // 12
	  { { 6, 0, 1, 4, 100, 102, 103 } },  // 13
	  { { 6, 0, 3, 2, 101, 103, 102 } },  // 14
	  { { 4, 100, 101, 102, 103, 0, 0 } } // 15
	};
	struct TRIANGLE_CLIP {
		int clip[7];
	};
	static TRIANGLE_CLIP triangleCases[] = {
	  { { 0, 0, 0, 0, 0, 0, 0 } },        // 0
	  { { 4, 0, 3, 2, 100, 0, 0 } },      // 1
	  { { 4, 0, 1, 4, 101, 0, 0 } },      // 2
	  { { 6, 101, 1, 4, 100, 2, 3 } },    // 3
	  { { 4, 1, 2, 5, 102, 0, 0 } },      // 4
	  { { 6, 102, 5, 1, 100, 3, 0 } },    // 5
	  { { 6, 102, 2, 5, 101, 0, 4 } },    // 6
	  { { 6, 3, 4, 5, 100, 101, 102 } },  // 7
	  { { 4, 3, 4, 5, 103, 0, 0 } },      // 8
	  { { 6, 103, 4, 5, 100, 0, 2 } },    // 9
	  { { 6, 103, 5, 3, 101, 1, 0 } },    // 10
	  { { 6, 100, 101, 103, 2, 1, 5 } },  // 11
	  { { 6, 2, 102, 1, 3, 103, 4 } },    // 12
	  { { 6, 0, 1, 4, 100, 102, 103 } },  // 13
	  { { 6, 0, 3, 2, 101, 103, 102 } },  // 14
	  { { 4, 100, 101, 102, 103, 0, 0 } } // 15
	};
	struct InterpolateEdge {
		igIndex vh1;
		igIndex vh2;
		float t;
		InterpolateEdge(igIndex vh) {
			vh1 = vh;
			vh2 = -1;
			t = 0.0;
		}
		InterpolateEdge(igIndex _vh1, igIndex _vh2, float _t) {
			vh1 = _vh1;
			vh2 = _vh2;
			t = _t;
		}
	};

	static void Clip(Tetra::Pointer cell, float* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int MASK[4] = { 1,2,4,8 };
		int i, j, CaseIndex = 0;
		igIndex pId = 0;
		igIndex vhs[IGAME_CELL_MAX_SIZE] = {};
		igIndex vcnt = 0;
		const int* vert = nullptr;
		igIndex vh1 = 0, vh2 = 0;
		Point v1, v2, p;
		double deltaValue = 0.0, t = 0.0;
		for (i = 0; i < 4; i++) {
			if (cellValues[i] <= 0.0) {
				CaseIndex |= MASK[i];
			}
		}
		if (CaseIndex == 0 || CaseIndex == 15) {
			return;
		}
		auto ClipData = (tetraCases + CaseIndex)->clip;

		if (m_slice == false) {
			for (i = 1; i <= ClipData[0]; i++) {
				if (ClipData[i] >= 100) {
					pId = ClipData[i] - 100;
					//vhs[i - 1] = cell->GetPointId(pId);
					points->AddPoint(cell->GetPoint(pId));
					vhs[i - 1] = points->GetNumberOfPoints() - 1;
					OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(pId)));
				}
				else {
					vert = Tetra::edges[ClipData[i]];
					deltaValue = cellValues[vert[1]] - cellValues[vert[0]];
					if (deltaValue > 0) {
						vh1 = vert[0];
						vh2 = vert[1];
					}
					else {
						vh2 = vert[0];
						vh1 = vert[1];
						deltaValue = -deltaValue;
					}
					t = (deltaValue == 0.0 ? 0.0 : -cellValues[vh1] / deltaValue);

					v1 = cell->GetPoint(vh1);
					v2 = cell->GetPoint(vh2);
					p = v1 + (v2 - v1) * t;
					points->AddPoint(p);
					vhs[i - 1] = points->GetNumberOfPoints() - 1;
					OriginEdge.emplace_back(InterpolateEdge(vh1, vh2, t));
				}
			}
			//std::cout<<vhs[0]<<" "<<vhs[1]<<' ' << vhs[2] << " " << vhs[3] << '\n';
			if (ClipData[0] == 4) {
				connectivity->AddCellIds(vhs, 4);
				types->AddValue(IG_TETRA);
				originCell.emplace_back(cellId);
			}
			else if (ClipData[0] == 6) {
				connectivity->AddCellIds(vhs, 6);
				types->AddValue(IG_PRISM);
				originCell.emplace_back(cellId);
			}
		}
		else {
			int nPts = 0;
			for (i = 1; i <= ClipData[0]; i++) {
				if (ClipData[i] < 100) {
					vert = Tetra::edges[ClipData[i]];
					deltaValue = cellValues[vert[1]] - cellValues[vert[0]];
					if (deltaValue > 0) {
						vh1 = vert[0];
						vh2 = vert[1];
					}
					else {
						vh2 = vert[0];
						vh1 = vert[1];
						deltaValue = -deltaValue;
					}
					t = (deltaValue == 0.0 ? 0.0 : -cellValues[vh1] / deltaValue);

					v1 = cell->GetPoint(vh1);
					v2 = cell->GetPoint(vh2);
					p = v1 + (v2 - v1) * t;
					points->AddPoint(p);
					vhs[nPts++] = points->GetNumberOfPoints() - 1;
					OriginEdge.emplace_back(InterpolateEdge(vh1, vh2, t));
				}
			}
			//std::cout<<vhs[0]<<" "<<vhs[1]<<' ' << vhs[2] << " " << vhs[3] << '\n';
			if (nPts == 3) {
				connectivity->AddCellIds(vhs, nPts);
				types->AddValue(IG_TRIANGLE);
				originCell.emplace_back(cellId);
			}
			else if (nPts == 4) {
				std::swap(vhs[2], vhs[3]);
				connectivity->AddCellIds(vhs, nPts);
				types->AddValue(IG_QUAD);
				originCell.emplace_back(cellId);
			}
		}
	}



	static void Clip(QuadraticTetra::Pointer cell, float* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		Tetra::Pointer tetra = Tetra::New();
		float tetvalues[4] = {};
		igIndex pid = 0;
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				pid = QuadraticTetra::SubTetras[0][i][j];
				tetra->Points->SetPoint(j, cell->Points->GetPoint(pid));
				tetra->PointIds->SetId(j, cell->PointIds->GetId(pid));
				tetvalues[j] = cellValues[pid];
			}
			Clip(tetra, tetvalues, points, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice);
		}
	}


	static void Clip(Volume::Pointer cell, float* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		Tetra::Pointer tetra = Tetra::New();
		float tetvalues[4] = {};
		int PointNum = cell->GetNumberOfPoints();
		int i = 0, allOut = 1, allIn = 1;
		float value = 0.0;
		for (i = 0; i < PointNum; i++)
		{
			value = cellValues[i];
			if (value >= 0.0) {
				allOut = 0;
			}
			else {
				allIn = 0;
			}
		}
		if (allOut) {
			return;
		}
		else if (allIn) {
			return;
			connectivity->AddCellIds(cell->PointIds);
			types->AddValue(cell->GetCellType());
		}
		else {

			for (;;) {
				Clip(tetra, tetvalues, points, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice);
			}
		}
	}
	static void Clip(Polyhedron::Pointer cell, float* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{

	}
}




IGAME_NAMESPACE_END