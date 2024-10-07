#include "iGameTetra.h"
#include "iGameCellArray.h"
#include "iGameAttributeSet.h"
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


	static void Clip(Cell::Pointer cell, float* cellValues, Points::Pointer points, CellArray::Pointer connectivity,UnsignedIntArray::Pointer types,
		AttributeSet::Pointer inData, AttributeSet::Pointer outData, igIndex cellId)
	{
		int MASK[4] = { 1,2,4,8 };
		int i, j, CaseIndex = 0;
		igIndex pId = 0;
		igIndex vhs[IGAME_CELL_MAX_SIZE];
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
		for (i = 1; i <= ClipData[0]; i++) {

			if (ClipData[i] >= 100) {
				pId = ClipData[i] - 100;
				vhs[i - 1] = cell->GetPointId(pId);
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
				vhs[i - 1] = points->GetNumberOfPoints()-1;
			}
		}
		if (ClipData[0] == 4) {
			connectivity->AddCellIds(vhs, 4);
			types->AddValue(IG_TETRA);
		}
		else {
			connectivity->AddCellIds(vhs, 6);
			types->AddValue(IG_PRISM);
		}
	}
}




IGAME_NAMESPACE_END