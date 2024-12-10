#ifndef iGameCellClip_h
#define iGameCellClip_h

#include "iGameTetra.h"
#include "iGamePolyhedron.h"
#include "iGameQuad.h"
#include "Quadratic/iGameQuadraticTriangle.h"
#include "Quadratic/iGameQuadraticQuad.h"
#include "Quadratic/iGameQuadraticTetra.h"
#include "Quadratic/iGameQuadraticHexahedron.h"
#include "Quadratic/iGameQuadraticPrism.h"
#include "Quadratic/iGameQuadraticPyramid.h"
#include "iGameCellArray.h"
#include "iGameAttributeSet.h"
#include "iGameVolumeMesh.h"
#include "iGamePointFinder.h"
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

	struct InterpolateEdge {
		igIndex vh1;
		igIndex vh2;
		double t;

		InterpolateEdge() : vh1(-1), vh2(-1), t(0.0f) {}
		InterpolateEdge(igIndex vh) {
			vh1 = vh;
			vh2 = -1;
			t = 0.0f;
		}
		InterpolateEdge(igIndex _vh1, igIndex _vh2, double _t) {
			vh1 = _vh1;
			vh2 = _vh2;
			t = _t;
		}
	};

	static void Clip(Tetra::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false, bool isMustClip = false)
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
					OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
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
					OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
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



	static void Clip(Tetra::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
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
		auto ClipData = (tetraCases + CaseIndex)->clip;
		if (m_slice == false) {
			for (i = 1; i <= ClipData[0]; i++) {
				if (ClipData[i] >= 100) {
					pId = ClipData[i] - 100;
					vhs[i - 1] = pointFinder->InsertUniquePoint(cell->GetPoint(pId));
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
					vhs[i - 1] = pointFinder->InsertUniquePoint(p);
					OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
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
					vhs[nPts++] = pointFinder->InsertUniquePoint(p);
					OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
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




	static void Clip(Polyhedron::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		auto fcnt = cell->GetNumberOfFaces();
		auto topVh = cell->GetPointId(0);
		auto originVhs = cell->m_PointIds->RawPointer();
		Cell::Pointer face = nullptr;
		bool isCount = false;

		igIndex st = 0;
		igIndex ed = 0;
		igIndex vhs[IGAME_CELL_MAX_SIZE] = { 0 };
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = { 0 };
		tetra->m_PointIds->SetId(3, topVh);
		tetra->m_Points->SetPoint(3, cell->GetPoint(0));
		tetvalues[3] = cellValues[0];
		for (int i = 0; i < fcnt; i++) {
			isCount = false;
			st = cell->m_FaceOffset->GetId(i);
			ed = cell->m_FaceOffset->GetId(i + 1);
			for (j = st; j < ed && !isCount; j++) {
				if (originVhs[j] == topVh) {
					isCount = true;
				}
			}
			if (!isCount) {
				for (j = st; j < ed - 2; j++) {
					tetra->m_PointIds->SetId(2, originVhs[st]);
					tetra->m_PointIds->SetId(1, originVhs[j + 1]);
					tetra->m_PointIds->SetId(0, originVhs[j + 2]);
					tetra->m_Points->SetPoint(2, cell->GetPoint(st));
					tetra->m_Points->SetPoint(1, cell->GetPoint(j + 1));
					tetra->m_Points->SetPoint(0, cell->GetPoint(j + 2));
					tetvalues[2] = cellValues[st];
					tetvalues[1] = cellValues[j + 1];
					tetvalues[0] = cellValues[j + 2];
					Clip(tetra, tetvalues, points, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice, true);
				}
			}
		}
	}

	static void Clip(Polyhedron::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		auto fcnt = cell->GetNumberOfFaces();
		auto topVh = cell->GetPointId(0);
		auto originVhs = cell->m_PointIds->RawPointer();
		Cell::Pointer face = nullptr;
		bool isCount = false;
		igIndex st = 0;
		igIndex ed = 0;
		igIndex vhs[IGAME_CELL_MAX_SIZE] = { 0 };
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = { 0 };
		tetra->m_PointIds->SetId(3, topVh);
		tetra->m_Points->SetPoint(3, cell->GetPoint(0));
		tetvalues[3] = cellValues[0];
		for (int i = 0; i < fcnt; i++) {
			isCount = false;
			st = cell->m_FaceOffset->GetId(i);
			ed = cell->m_FaceOffset->GetId(i + 1);
			for (j = st; j < ed && !isCount; j++) {
				if (originVhs[j] == topVh) {
					isCount = true;
				}
			}
			if (!isCount) {
				for (j = st; j < ed - 2; j++) {
					tetra->m_PointIds->SetId(2, originVhs[st]);
					tetra->m_PointIds->SetId(1, originVhs[j + 1]);
					tetra->m_PointIds->SetId(0, originVhs[j + 2]);
					tetra->m_Points->SetPoint(2, cell->GetPoint(st));
					tetra->m_Points->SetPoint(1, cell->GetPoint(j + 1));
					tetra->m_Points->SetPoint(0, cell->GetPoint(j + 2));
					tetvalues[2] = cellValues[st];
					tetvalues[1] = cellValues[j + 1];
					tetvalues[0] = cellValues[j + 2];
					Clip(tetra, tetvalues, pointFinder, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice);
				}
			}
		}
	}


	static void Clip(Volume::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, double* pointValues, bool m_slice = false)
	{
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		int PointNum = cell->GetNumberOfPoints();
		int i = 0, j = 0;
		auto tetras = cell->clipCelltoTetra();
		for (int i = 0; i < tetras.size(); i++) {
			tetra->m_Points = tetras[i]->m_Points;
			tetra->m_PointIds = tetras[i]->m_PointIds;
			for (j = 0; j < 4; j++) {
				tetvalues[j] = pointValues[tetra->m_PointIds->GetId(j)];
			}
			Clip(tetra, tetvalues, points, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice, true);
		}
	}


	static void Clip(Volume::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, double* pointValues, bool m_slice = false)
	{
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		int PointNum = cell->GetNumberOfPoints();
		int i = 0, j = 0;
		auto tetras = cell->clipCelltoTetra();
		for (int i = 0; i < tetras.size(); i++) {
			tetra->m_Points = tetras[i]->m_Points;
			tetra->m_PointIds = tetras[i]->m_PointIds;
			for (j = 0; j < 4; j++) {
				tetvalues[j] = pointValues[tetra->m_PointIds->GetId(j)];
			}
			Clip(tetra, tetvalues, pointFinder, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice);
		}
	}


	static void Clip(QuadraticTetra::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		igIndex pid = 0;
		for (i = 0; i < 8; i++)
		{
			for (j = 0; j < 4; j++)
			{
				pid = QuadraticTetra::SubTetras[0][i][j];
				tetra->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				tetra->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				tetvalues[j] = cellValues[pid];
			}
			Clip(tetra, tetvalues, points, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice, true);
		}
	}
	
	static void Clip(QuadraticTetra::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		igIndex pid = 0;
		for (i = 0; i < 8; i++)
		{
			for (j = 0; j < 4; j++)
			{
				pid = QuadraticTetra::SubTetras[0][i][j];
				tetra->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				tetra->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				tetvalues[j] = cellValues[pid];
			}
			Clip(tetra, tetvalues, pointFinder, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice);
		}
	}

	
	static void Clip(QuadraticPrism::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		igIndex pid = 0;


	}

	static void Clip(QuadraticPrism::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		igIndex pid = 0;


	}

	static void Clip(QuadraticPyramid::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		igIndex pid = 0;

	}

	static void Clip(QuadraticPyramid::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder , CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		igIndex pid = 0;

	}

	static void Clip(QuadraticHexahedron::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		igIndex pid = 0;

	}

	static void Clip(QuadraticHexahedron::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0, vcnt = cell->GetNumberOfPoints();
		Tetra::Pointer tetra = Tetra::New();
		double tetvalues[4] = {};
		igIndex pid = 0;

	}


	struct TRIANGLE_CLIP {
		int clip[7];
	};
	static TRIANGLE_CLIP triangleCases[] = {
           { { -1, -1, -1, -1, -1, -1, -1 } },   // 0
           { { 0, 2, 100, -1, -1, -1, -1 } },    // 1
           { { 1, 0, 101, -1, -1, -1, -1 } },    // 2
           { { 1, 2, 100, 1, 100, 101, -1 } },   // 3
           { { 2, 1, 102, -1, -1, -1, -1 } },    // 4
           { { 0, 1, 102, 102, 100, 0, -1 } },   // 5
           { { 0, 101, 2, 2, 101, 102, -1 } },   // 6
           { { 100, 101, 102, -1, -1, -1, -1 } } // 7
	};
	static void Clip(Triangle::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false, bool isMustClip = false)
	{
		int MASK[3] = { 1,2,4 };
		int i, j, CaseIndex = 0;
		igIndex pId = 0;
		igIndex vhs[IGAME_CELL_MAX_SIZE] = {};
		igIndex vcnt = 0;
		const int* vert = nullptr;
		igIndex vh1 = 0, vh2 = 0;
		Point v1, v2, p;
		double deltaValue = 0.0, t = 0.0;
		for (i = 0; i < 3; i++) {
			if (cellValues[i] <= 0.0) {
				CaseIndex |= MASK[i];
			}
		}
		auto ClipData = (triangleCases + CaseIndex)->clip;

		if (m_slice == false) {
			for (; ClipData[0] > -1; ClipData += 3) {
				for (int i = 0; i < 3; i++) {
					if (ClipData[i] >= 100) {
						pId = ClipData[i] - 100;
						points->AddPoint(cell->GetPoint(pId));
						vhs[i] = points->GetNumberOfPoints() - 1;
						OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(pId)));
					}
					else {
						int vert[2] = { ClipData[i],(ClipData[i] + 1) % 3 };
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
						vhs[i] = points->GetNumberOfPoints() - 1;
						OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
					}
				}
				//std::cout<<vhs[0]<<' '<<vhs[1]<<' ' << vhs[2] << '\n';
				connectivity->AddCellIds(vhs, 3);
				types->AddValue(IG_TRIANGLE);
				originCell.emplace_back(cellId);
			}
		}
		else {
			if (ClipData[0] < 0) {
				return;
			}
			int nPts = 0;
			for (int i = 0; i < 3; i++) {
				if (ClipData[i] < 100) {
					int vert[2] = { ClipData[i],(ClipData[i] + 1) % 3 };
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
					OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
				}
			}
			connectivity->AddCellIds(vhs, 2);
			types->AddValue(IG_LINE);
			originCell.emplace_back(cellId);
		}
	}


	static void Clip(Triangle::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false, bool isMustClip = false)
	{
		int MASK[3] = { 1,2,4 };
		int i, j, CaseIndex = 0;
		igIndex pId = 0;
		igIndex vhs[IGAME_CELL_MAX_SIZE] = {};
		igIndex vcnt = 0;
		const int* vert = nullptr;
		igIndex vh1 = 0, vh2 = 0;
		Point v1, v2, p;
		double deltaValue = 0.0, t = 0.0;
		for (i = 0; i < 3; i++) {
			if (cellValues[i] <= 0.0) {
				CaseIndex |= MASK[i];
			}
		}
		auto ClipData = (triangleCases + CaseIndex)->clip;

		if (m_slice == false) {
			for (; ClipData[0] > -1; ClipData += 3) {
				for (int i = 0; i < 3; i++) {
					if (ClipData[i] >= 100) {
						pId = ClipData[i] - 100;
						vhs[i] = pointFinder->InsertUniquePoint(cell->GetPoint(pId));
						OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(pId)));
					}
					else {
						int vert[2] = { ClipData[i],(ClipData[i] + 1) % 3 };
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
						vhs[i] = pointFinder->InsertUniquePoint(p);
						OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
					}
				}
				//std::cout<<vhs[0]<<' '<<vhs[1]<<' ' << vhs[2] << '\n';
				connectivity->AddCellIds(vhs, 3);
				types->AddValue(IG_TRIANGLE);
				originCell.emplace_back(cellId);
			}
		}
		else {
			if (ClipData[0] < 0) {
				return;
			}
			int nPts = 0;
			for (int i = 0; i < 3; i++) {
				if (ClipData[i] < 100) {
					int vert[2] = { ClipData[i],(ClipData[i] + 1) % 3 };
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
					vhs[nPts++] = pointFinder->InsertUniquePoint(p);
					OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
				}
			}
			connectivity->AddCellIds(vhs, 2);
			types->AddValue(IG_LINE);
			originCell.emplace_back(cellId);
		}
	}

	struct QUAD_CLIP {
		int clip[14];
	};
	static QUAD_CLIP quadCases[] = {
	       { { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },    // 0
	       { { 3, 100, 0, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },      // 1
	       { { 3, 101, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },      // 2
	       { { 4, 100, 101, 1, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },     // 3
	       { { 3, 102, 2, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },      // 4
	       { { 3, 100, 0, 3, 3, 102, 2, 1, -1, -1, -1, -1, -1, -1 } },        // 5
	       { { 4, 101, 102, 2, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },     // 6
	       { { 3, 100, 101, 3, 3, 101, 2, 3, 3, 101, 102, 2, -1, -1 } },      // 7
	       { { 3, 103, 3, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },      // 8
	       { { 4, 100, 0, 2, 103, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },     // 9
	       { { 3, 101, 1, 0, 3, 103, 3, 2, -1, -1, -1, -1, -1, -1 } },        // 10
	       { { 3, 100, 101, 1, 3, 100, 1, 2, 3, 100, 2, 103, -1, -1 } },      // 11
	       { { 4, 102, 103, 3, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } },     // 12
	       { { 3, 100, 0, 103, 3, 0, 1, 103, 3, 1, 102, 103, -1, -1 } },      // 13
	       { { 3, 0, 101, 102, 3, 0, 102, 3, 3, 102, 103, 3, -1, -1 } },      // 14
	       { { 4, 100, 101, 102, 103, -1, -1, -1, -1, -1, -1, -1, -1, -1 } }, // 15
	};
	static void Clip(Quad::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0;
		Triangle::Pointer triangle = Triangle::New();
		double trivalues[3] = {};
		igIndex pid = 0;
		int nPts = cell->GetNumberOfPoints();
		for (i = 0; i < nPts - 2; i++) {
			for (j = 0; j < 3; j++) {
				pid = j == 0 ? 0 : i + j;
				triangle->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				triangle->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				trivalues[j] = cellValues[pid];
			}
			Clip(triangle, trivalues, points, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice, true);
		}

		return;
		//int MASK[4] = { 1,2,4,8 };
		//int i, j, CaseIndex = 0;
		//igIndex pId = 0;
		//igIndex vhs[IGAME_CELL_MAX_SIZE] = {};
		//igIndex vcnt = 0;
		//const int* vert = nullptr;
		//igIndex vh1 = 0, vh2 = 0;
		//Point v1, v2, p;
		//double deltaValue = 0.0, t = 0.0;
		//for (i = 0; i < 4; i++) {
		//	if (cellValues[i] <= 0.0) {
		//		CaseIndex |= MASK[i];
		//	}
		//}
		//if (CaseIndex == 0 || CaseIndex == 15) {
		//	return;
		//}
		//auto ClipData = (quadCases + CaseIndex)->clip;

		//if (m_slice == false) {
		//	for (; ClipData[0] > -1; ClipData += ClipData[0] + 1) {
		//		for (int i = 0; i < ClipData[0]; i++) {
		//			if (ClipData[i] >= 100) {
		//				pId = ClipData[i] - 100;
		//				points->AddPoint(cell->GetPoint(pId));
		//				vhs[i] = points->GetNumberOfPoints() - 1;
		//				OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(pId)));
		//			}
		//			else {
		//				int vert[2] = { ClipData[i],(ClipData[i] + 1) % 4 };
		//				deltaValue = cellValues[vert[1]] - cellValues[vert[0]];
		//				if (deltaValue > 0) {
		//					vh1 = vert[0];
		//					vh2 = vert[1];
		//				}
		//				else {
		//					vh2 = vert[0];
		//					vh1 = vert[1];
		//					deltaValue = -deltaValue;
		//				}
		//				t = (deltaValue == 0.0 ? 0.0 : -cellValues[vh1] / deltaValue);
		//				v1 = cell->GetPoint(vh1);
		//				v2 = cell->GetPoint(vh2);
		//				p = v1 + (v2 - v1) * t;
		//				points->AddPoint(p);
		//				vhs[i] = points->GetNumberOfPoints() - 1;
		//				OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
		//			}
		//		}
		//		if (ClipData[0] == 3) {
		//			if (vhs[0] == vhs[1] || vhs[0] == vhs[2] || vhs[1] == vhs[2])
		//			{
		//				continue;
		//			}
		//			connectivity->AddCellIds(vhs, 3);
		//			types->AddValue(IG_TRIANGLE);
		//			originCell.emplace_back(cellId);
		//		}
		//		else if (ClipData[0] == 4) {
		//			if ((vhs[0] == vhs[3] && vhs[1] == vhs[2]) || (vhs[0] == vhs[1] && vhs[3] == vhs[2]))
		//			{
		//				continue;
		//			}
		//			connectivity->AddCellIds(vhs, 4);
		//			types->AddValue(IG_TRIANGLE);
		//			originCell.emplace_back(cellId);
		//		}
		//	}
		//}
	}


	static void Clip(Quad::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		int i = 0, j = 0;
		Triangle::Pointer triangle = Triangle::New();
		double trivalues[3] = {};
		igIndex pid = 0;
		int nPts = cell->GetNumberOfPoints();
		for (i = 0; i < nPts - 2; i++) {
			for (j = 0; j < 3; j++) {
				pid = j == 0 ? 0 : i + j;
				triangle->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				triangle->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				trivalues[j] = cellValues[pid];
			}
			Clip(triangle, trivalues, pointFinder, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice, true);
		}
		return;
		//int MASK[4] = { 1,2,4,8 };
		//int i, j, CaseIndex = 0;
		//igIndex pId = 0;
		//igIndex vhs[IGAME_CELL_MAX_SIZE] = {};
		//igIndex vcnt = 0;
		//const int* vert = nullptr;
		//igIndex vh1 = 0, vh2 = 0;
		//Point v1, v2, p;
		//double deltaValue = 0.0, t = 0.0;
		//for (i = 0; i < 4; i++) {
		//	if (cellValues[i] <= 0.0) {
		//		CaseIndex |= MASK[i];
		//	}
		//}
		//if (CaseIndex == 0 || CaseIndex == 15) {
		//	return;
		//}
		//auto ClipData = (quadCases + CaseIndex)->clip;

		//if (m_slice == false) {
		//	for (; ClipData[0] > -1; ClipData += ClipData[0] + 1) {
		//		for (int i = 0; i < ClipData[0]; i++) {
		//			if (ClipData[i] >= 100) {
		//				pId = ClipData[i] - 100;
		//				points->AddPoint(cell->GetPoint(pId));
		//				vhs[i] = points->GetNumberOfPoints() - 1;
		//				OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(pId)));
		//			}
		//			else {
		//				int vert[2] = { ClipData[i],(ClipData[i] + 1) % 4 };
		//				deltaValue = cellValues[vert[1]] - cellValues[vert[0]];
		//				if (deltaValue > 0) {
		//					vh1 = vert[0];
		//					vh2 = vert[1];
		//				}
		//				else {
		//					vh2 = vert[0];
		//					vh1 = vert[1];
		//					deltaValue = -deltaValue;
		//				}
		//				t = (deltaValue == 0.0 ? 0.0 : -cellValues[vh1] / deltaValue);
		//				v1 = cell->GetPoint(vh1);
		//				v2 = cell->GetPoint(vh2);
		//				p = v1 + (v2 - v1) * t;
		//				points->AddPoint(p);
		//				vhs[i] = points->GetNumberOfPoints() - 1;
		//				OriginEdge.emplace_back(InterpolateEdge(cell->GetPointId(vh1), cell->GetPointId(vh2), t));
		//			}
		//		}
		//		if (ClipData[0] == 3) {
		//			if (vhs[0] == vhs[1] || vhs[0] == vhs[2] || vhs[1] == vhs[2])
		//			{
		//				continue;
		//			}
		//			connectivity->AddCellIds(vhs, 3);
		//			types->AddValue(IG_TRIANGLE);
		//			originCell.emplace_back(cellId);
		//		}
		//		else if (ClipData[0] == 4) {
		//			if ((vhs[0] == vhs[3] && vhs[1] == vhs[2]) || (vhs[0] == vhs[1] && vhs[3] == vhs[2]))
		//			{
		//				continue;
		//			}
		//			connectivity->AddCellIds(vhs, 4);
		//			types->AddValue(IG_TRIANGLE);
		//			originCell.emplace_back(cellId);
		//		}
		//	}
		//}
	}


	static void Clip(Polygon::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		Triangle::Pointer triangle = Triangle::New();
		double trivalues[3] = {};
		igIndex pid = 0;
		int nPts = cell->GetNumberOfPoints();
		for (int i = 0; i < nPts - 2; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				pid = j == 0 ? 0 : i + j;
				triangle->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				triangle->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				trivalues[j] = cellValues[pid];
			}
			Clip(triangle, trivalues, points, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice, true);
		}
	}
	

	static void Clip(Polygon::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		Triangle::Pointer triangle = Triangle::New();
		double trivalues[3] = {};
		igIndex pid = 0;
		int nPts = cell->GetNumberOfPoints();
		for (int i = 0; i < nPts - 2; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				pid = j == 0 ? 0 : i + j;
				triangle->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				triangle->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				trivalues[j] = cellValues[pid];
			}
			Clip(triangle, trivalues, pointFinder, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice, true);
		}
	}



	static void Clip(QuadraticTriangle::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		Triangle::Pointer triangle = Triangle::New();
		double trivalues[3] = {};
		igIndex pid = 0;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				pid = QuadraticQuad::SubTriangles[i][j];
				triangle->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				triangle->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				trivalues[j] = cellValues[pid];
			}
			Clip(triangle, trivalues, points, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice, true);
		}
	}
	

	static void Clip(QuadraticTriangle::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		Triangle::Pointer triangle = Triangle::New();
		double trivalues[3] = {};
		igIndex pid = 0;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				pid = QuadraticQuad::SubTriangles[i][j];
				triangle->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				triangle->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				trivalues[j] = cellValues[pid];
			}
			Clip(triangle, trivalues, pointFinder, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice);
		}
	}


	static void Clip(QuadraticQuad::Pointer cell, double* cellValues, Points::Pointer points, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		Triangle::Pointer triangle = Triangle::New();
		double trivalues[3] = {};
		igIndex pid = 0;
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				pid = QuadraticQuad::SubTriangles[i][j];
				triangle->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				triangle->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				trivalues[j] = cellValues[pid];
			}
			Clip(triangle, trivalues, points, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice, true);
		}
	}
	

	static void Clip(QuadraticQuad::Pointer cell, double* cellValues, PointFinder::Pointer pointFinder, CellArray::Pointer connectivity, UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId, std::vector<InterpolateEdge>& OriginEdge, std::vector<igIndex>& originCell, bool m_slice = false)
	{
		Triangle::Pointer triangle = Triangle::New();
		double trivalues[3] = {};
		igIndex pid = 0;
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				pid = QuadraticQuad::SubTriangles[i][j];
				triangle->m_Points->SetPoint(j, cell->m_Points->GetPoint(pid));
				triangle->m_PointIds->SetId(j, cell->m_PointIds->GetId(pid));
				trivalues[j] = cellValues[pid];
			}
			Clip(triangle, trivalues, pointFinder, connectivity, types, inData, outData, cellId, OriginEdge, originCell, m_slice);
		}
	}
}




IGAME_NAMESPACE_END
#endif iGameCellClip_h