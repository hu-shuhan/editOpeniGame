#include "iGameCellArray.h"

IGAME_NAMESPACE_BEGIN

// Free all memory and initialize the array
void CellArray::Initialize() {
    m_Buffer->Initialize();
    m_Offsets->Initialize();
    m_Offsets->AddValue(0);
}

// Allocate memory, and the old memory is preserved. The array
// size will not change. '_Newsize' is the size of the array,
// is not the number of cells.
void CellArray::Reserve(const IGsize _Newsize) { m_Buffer->Reserve(_Newsize); }

// Allocate memory, and the old memory is preserved. The array
// size will change. '_Newsize'is the size of the array, is
// not the number of cells.
void CellArray::Resize(const IGsize _Newsize) {
    m_Buffer->Resize(_Newsize);
}

// Reset the array size, and the old memory will not change.
void CellArray::Reset() {
    m_Buffer->Reset();
    m_Offsets->Reset();
    m_DeleteMasker->Reset();
}

// Free up extra memory.
void CellArray::Squeeze() {
    m_Buffer->Squeeze();
    m_Offsets->Squeeze();
}

bool CellArray::ShallowCopy(CellArray::Pointer o) { return false; }
bool CellArray::DeepCopy(CellArray::Pointer o) {
    if (o == nullptr) return false;
    m_Buffer = IdArray::New();
    m_Buffer->DeepCopy(o->m_Buffer);
    m_Offsets->DeepCopy(o->m_Offsets);
    m_DeleteMasker->DeepCopy(o->m_DeleteMasker);
    m_NumberOfCells = o->m_NumberOfCells;
    m_FixedCellSize = o->m_FixedCellSize;
    m_UseOffsets = o->m_UseOffsets;
    //this->Modified();
    return true;
}

// '_Newsize' is the number of cells
void CellArray::SetNumberOfCells(const IGsize _Newsize) {
    m_NumberOfCells = _Newsize;
    m_DeleteMasker->Resize(_Newsize);
}

// Get the number of cells
IGsize CellArray::GetNumberOfCells() const noexcept { return m_NumberOfCells; }

// Get cell's index sequence.
// @Return: the size of sequence.
int CellArray::GetCellIds(const IGsize cellId, igIndex* cell) const {
    int ncells = this->GetCellSize(cellId);
    igIndex* ptr = m_Buffer->RawPointer() + this->GetBeginOffset(cellId);
    for (int i = 0; i < ncells; i++) { cell[i] = ptr[i]; }
    return ncells;
}

// Get cell's index sequence. Return the size of sequence.
int CellArray::GetCellIds(const IGsize cellId, const igIndex*& cell) const {
    cell = m_Buffer->RawPointer() + this->GetBeginOffset(cellId);
    return this->GetCellSize(cellId);
}

// Get cell's index sequence.
void CellArray::GetCellIds(const IGsize cellId, IdArray::Pointer cell) const {
    cell->Reset();
    IGuint begin = this->GetBeginOffset(cellId);
    int size = this->GetCellSize(cellId);
    igIndex* pos = m_Buffer->RawPointer() + begin;
    for (int i = 0; i < size; i++) { cell->AddId(pos[i]); }
}

// Set cell's index sequence.
// @param cell: index sequence
// @param ncell: the size of index sequence
void CellArray::SetCellIds(const IGsize cellId, igIndex* cell, int ncell) {
    IGuint begin = this->GetBeginOffset(cellId);
    igIndex* ptr = m_Buffer->RawPointer() + begin;
    for (int i = 0; i < ncell; i++) { ptr[i] = cell[i]; }
}

// Add cell's index sequence.
// @Return: the index of cell.
IGsize CellArray::AddCellIds(igIndex* cell, int ncell) {
    IGuint beginOffset = this->GetBeginOffset(m_NumberOfCells);
    IGuint endOffset = beginOffset + ncell;
    if (endOffset > m_Buffer->GetNumberOfIds()) {
        const IGsize newSize = std::max(endOffset, (beginOffset * 2));
        m_Buffer->Resize(newSize);
    }

    igIndex* ptr = m_Buffer->RawPointer() + beginOffset;
    for (int i = 0; i < ncell; i++) { ptr[i] = cell[i]; }
    m_DeleteMasker->AddTag();
    if (m_UseOffsets) {
        m_Offsets->AddValue(endOffset);
    } else {
        if (m_NumberOfCells == 0) {
            m_FixedCellSize = ncell;
        } else if (m_FixedCellSize != ncell) {
            for (int i = 0; i < m_NumberOfCells; i++) {
                m_Offsets->AddValue((i + 1) * m_FixedCellSize);
            }
            m_Offsets->AddValue(endOffset);
            m_UseOffsets = true;
        }
    }

    return m_NumberOfCells++;
}
IGsize CellArray::AddCellIds(const igIndex* cell, int ncell) {
    IGuint beginOffset = this->GetBeginOffset(m_NumberOfCells);
    IGuint endOffset = beginOffset + ncell;
    if (endOffset > m_Buffer->GetNumberOfIds()) {
        const IGsize newSize = std::max(endOffset, (beginOffset * 2));
        m_Buffer->Resize(newSize);
    }

    igIndex* ptr = m_Buffer->RawPointer() + beginOffset;
    for (int i = 0; i < ncell; i++) { ptr[i] = cell[i]; }
    m_DeleteMasker->AddTag();
    if (m_UseOffsets) {
        m_Offsets->AddValue(endOffset);
    }
    else {
        if (m_NumberOfCells == 0) {
            m_FixedCellSize = ncell;
        }
        else if (m_FixedCellSize != ncell) {
            for (int i = 0; i < m_NumberOfCells; i++) {
                m_Offsets->AddValue((i + 1) * m_FixedCellSize);
            }
            m_Offsets->AddValue(endOffset);
            m_UseOffsets = true;
        }
    }

    return m_NumberOfCells++;
}
IGsize CellArray::AddCellIds(IdArray::Pointer ids) {
    if (ids == nullptr) return IGsize(-1);
    return AddCellIds(ids->RawPointer(), ids->GetNumberOfIds());
}

IGsize CellArray::AddCellId2(igIndex val0, igIndex val1) {
    igIndex cell[2]{val0, val1};
    return this->AddCellIds(cell, 2);
}
IGsize CellArray::AddCellId3(igIndex val0, igIndex val1, igIndex val2) 
{
    igIndex cell[3]{val0, val1, val2};
    return this->AddCellIds(cell, 3);
}

IGsize CellArray::AddCellId4(igIndex val0, igIndex val1, igIndex val2,
                             igIndex val3) {
    igIndex cell[4]{val0, val1, val2, val3};
    return this->AddCellIds(cell, 4);
}

IGsize CellArray::AddCellId5(igIndex val0, igIndex val1, igIndex val2,
                             igIndex val3, igIndex val4) {
    igIndex cell[5]{val0, val1, val2, val3, val4};
    return this->AddCellIds(cell, 5);
}

IGsize CellArray::AddCellId6(igIndex val0, igIndex val1, igIndex val2,
                             igIndex val3, igIndex val4, igIndex val5) {
    igIndex cell[6]{val0, val1, val2, val3, val4, val5};
    return this->AddCellIds(cell, 6);
}

IGsize CellArray::AddCellId7(igIndex val0, igIndex val1, igIndex val2,
                             igIndex val3, igIndex val4, igIndex val5,
                             igIndex val6) {
    igIndex cell[7]{val0, val1, val2, val3, val4, val5, val6};
    return this->AddCellIds(cell, 7);
}

IGsize CellArray::AddCellId8(igIndex val0, igIndex val1, igIndex val2,
                             igIndex val3, igIndex val4, igIndex val5,
                             igIndex val6, igIndex val7) {
    igIndex cell[8]{val0, val1, val2, val3, val4, val5, val6, val7};
    return this->AddCellIds(cell, 8);
}

IGsize CellArray::AddCellId9(igIndex val0, igIndex val1, igIndex val2,
                             igIndex val3, igIndex val4, igIndex val5,
                             igIndex val6, igIndex val7, igIndex val8) {
    igIndex cell[9]{val0, val1, val2, val3, val4, val5, val6, val7, val8};
    return this->AddCellIds(cell, 9);
}

// Replace one index.
void CellArray::ReplaceCellReference(const IGsize cellId, igIndex from,
                                     igIndex to) {
    IGuint begin = this->GetBeginOffset(cellId);
    int size = this->GetCellSize(cellId);
    igIndex* pos = m_Buffer->RawPointer() + begin;
    for (int i = 0; i < size; i++) {
        if (pos[i] == from) {
            pos[i] = to;
            break;
        }
    }
}

// Delete cell.
void CellArray::DeleteCell(const IGsize cellId) {
    m_DeleteMasker->MarkDeleted(cellId);
}

bool CellArray::IsDeleted(const IGsize cellId) {
    return m_DeleteMasker->IsDeleted(cellId);
}

// Garbage collection to free the space that has been removed
void CellArray::GarbageCollection() {
    igIndex i, j, k = 0, begin = 0;
    igIndex* pos = m_Buffer->RawPointer();
    for (i = 0; i < m_NumberOfCells; i++) {
        if (!IsDeleted(i)) {
            int size = this->GetCellSize(i);
            igIndex* ptr = m_Buffer->RawPointer() + this->GetBeginOffset(i);
            for (j = 0; j < size; j++) { pos[j] = ptr[j]; }
            pos += size;
            begin += size;
            if (m_UseOffsets) { m_Offsets->SetValue(k + 1, begin); }
            k++;
        }
    }
    m_Buffer->Resize(begin);
    if (m_UseOffsets || k == 0) { m_Offsets->Resize(k + 1); }
    m_DeleteMasker->Resize(k);
}

IdArray::Pointer CellArray::GetCellIdArray() { return m_Buffer; }
UnsignedIntArray::Pointer CellArray::GetOffset() { return m_Offsets; }
void CellArray::SetData(IdArray::Pointer ids, UnsignedIntArray::Pointer offsets) {
    m_Buffer = ids;
    m_Offsets = offsets;
    m_NumberOfCells = offsets->GetNumberOfValues() - 1;
    this->m_UseOffsets = true;
}
void CellArray::SetData(IdArray::Pointer ids, int fixedSize) {
    m_Buffer = ids;
    m_NumberOfCells = m_Buffer->GetNumberOfIds()/fixedSize;
    this->m_UseOffsets = false;
    this->m_FixedCellSize = fixedSize;
}

void CellArray::SetFixedSize(int fixedSize) {
    this->m_UseOffsets = false;
    this->m_FixedCellSize = fixedSize;
}
size_t CellArray::GetNumberOfCellIds() { return this->GetEndOffset(this->m_NumberOfCells - 1); };

IGsize CellArray::GetRealMemorySize() {
    if (!m_Buffer) return 0;
    IGsize res = m_Buffer->GetRealMemorySize();
    if (m_UseOffsets) { res += m_Offsets->GetRealMemorySize(); }
    if (m_DeleteMasker) { res += m_DeleteMasker->GetRealMemorySize(); }
    return res + sizeof(m_NumberOfCells) + sizeof(m_FixedCellSize) + sizeof(m_UseOffsets);
}

CellArray::CellArray() {
    m_Buffer = IdArray::New();
    m_DeleteMasker = DeleteMarker::New();
    m_Offsets = UnsignedIntArray::New();
    m_Offsets->AddValue(0);
};

IGuint CellArray::GetStartOffset(const IGsize cellId) const {
    return this->GetBeginOffset(cellId);
}
	// Get cell's begin offset
IGuint CellArray::GetBeginOffset(const IGsize cellId) const {
    return m_UseOffsets ? m_Offsets->GetValue(cellId)
                        : m_FixedCellSize * cellId;
}

// Get cell's end offset
IGuint CellArray::GetEndOffset(const IGsize cellId) const {
    return m_UseOffsets ? m_Offsets->GetValue(cellId + 1)
                        : m_FixedCellSize * (cellId + 1);
}

// Get cell's size
IGuint CellArray::GetCellSize(const IGsize cellId) const {
    return m_UseOffsets ? m_Offsets->GetValue(cellId + 1) -
                                  m_Offsets->GetValue(cellId)
                        : m_FixedCellSize;
}

bool CellArray::IsUseOffSet() const { return m_UseOffsets; }

IGAME_NAMESPACE_END