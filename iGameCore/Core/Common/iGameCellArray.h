#ifndef iGameCellArray_h
#define iGameCellArray_h

#include "iGameCell.h"
#include "iGameElementArray.h"
#include "iGameFlatArray.h"
#include "iGameMarker.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
class CellArray : public Object {
public:
    I_OBJECT(CellArray);
    static Pointer New() { return new CellArray; }

    // Free all memory and initialize the array
    void Initialize();

    // Reallocate memory, and the old memory is preserved. The array
    // size will not change. '_Newsize' is the size of the array,
    // is not the number of cells.
    void Reserve(const IGsize _Newsize);

    // Reallocate memory, and the old memory is preserved. The array
    // size will change. '_Newsize'is the size of the array, is
    // not the number of cells.
    void Resize(const IGsize _Newsize);

    // Reset the array size, and the old memory will not change.
    void Reset();

    // Free up extra memory.
    void Squeeze();

    bool ShallowCopy(CellArray::Pointer o) { return false; }
    bool DeepCopy(CellArray::Pointer o) {
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
    void SetNumberOfCells(const IGsize _Newsize);

    // Get the number of cells
    IGsize GetNumberOfCells() const noexcept;

    // Get cell's index sequence.
    // @Return: the size of sequence.
    int GetCellIds(const IGsize cellId, igIndex* cell);

    // Get cell's index sequence. Return the size of sequence.
    int GetCellIds(const IGsize cellId, const igIndex*& cell);

    // Get cell's index sequence.
    void GetCellIds(const IGsize cellId, IdArray::Pointer cell);

    // Set cell's index sequence.
    // @param cell: index sequence
    // @param ncell: the size of index sequence
    void SetCellIds(const IGsize cellId, igIndex* cell, int ncell);

    // Add cell's index sequence.
    // @Return: the index of cell.
    IGsize AddCellIds(igIndex* cell, int ncell);
    IGsize AddCellIds(IdArray::Pointer ids);
    IGsize AddCellId2(igIndex val0, igIndex val1);
    IGsize AddCellId3(igIndex val0, igIndex val1, igIndex val2);
    IGsize AddCellId4(igIndex val0, igIndex val1, igIndex val2, igIndex val3);
    IGsize AddCellId5(igIndex val0, igIndex val1, igIndex val2, igIndex val3,
                      igIndex val4);
    IGsize AddCellId6(igIndex val0, igIndex val1, igIndex val2, igIndex val3,
                      igIndex val4, igIndex val5);
    IGsize AddCellId7(igIndex val0, igIndex val1, igIndex val2, igIndex val3,
                      igIndex val4, igIndex val5, igIndex val6);
    IGsize AddCellId8(igIndex val0, igIndex val1, igIndex val2, igIndex val3,
                      igIndex val4, igIndex val5, igIndex val6, igIndex val7);
    IGsize AddCellId9(igIndex val0, igIndex val1, igIndex val2, igIndex val3,
                      igIndex val4, igIndex val5, igIndex val6, igIndex val7,
                      igIndex val8);

    // Replace one index.
    void ReplaceCellReference(const IGsize cellId, igIndex from, igIndex to);

    // Delete cell.
    void DeleteCell(const IGsize cellId);
    bool IsDeleted(const IGsize cellId);

    // Garbage collection to free the space that has been removed
    void GarbageCollection();

    IdArray::Pointer GetCellIdArray() { return m_Buffer; }
    UnsignedIntArray::Pointer GetOffset() { return m_Offsets; }
    void SetData(IdArray* ids, UnsignedIntArray* offsets) {
        m_Buffer = ids;
        m_Offsets = offsets;
        m_NumberOfCells = ids->GetNumberOfIds() - 1;
    }

    void SetFixedSize(int fixedSize) {
        this->m_UseOffsets = false;
        this->m_FixedCellSize = fixedSize;
    }
    size_t GetNumberOfCellIds() {
        return this->m_Buffer ? this->m_Buffer->GetNumberOfIds() : 0;
    };

    IGsize GetRealMemorySize() {
        if (!m_Buffer) return 0;
        IGsize res = m_Buffer->GetRealMemorySize();
        if (m_UseOffsets) { res += m_Offsets->GetRealMemorySize(); }
        if (m_DeleteMasker) { res += m_DeleteMasker->GetRealMemorySize(); }
        return res + sizeof(m_NumberOfCells) + sizeof(m_FixedCellSize) +
               sizeof(m_UseOffsets);
    }

protected:
    CellArray();
    ~CellArray() override = default;

    // Get cell's begin offset
    IGuint GetBeginOffset(const IGsize cellId) const;
    // Get cell's end offset
    IGuint GetEndOffset(const IGsize cellId) const;
    // Get cell's size
    IGuint GetCellSize(const IGsize cellId) const;


    IdArray::Pointer m_Buffer{}; // The cell's index array
    IGsize m_NumberOfCells{0};   // The number of cells

    UnsignedIntArray::Pointer
            m_Offsets{}; // The offset array to save cell's begin offset.

    // If all the cells are the same size, m_FixedCellSize will greater than zero.
    int m_FixedCellSize{-1};
    bool m_UseOffsets{false}; // Whether to use offset array or not
    DeleteMarker::Pointer m_DeleteMasker{}; // The cell's deletion masker.
};
IGAME_NAMESPACE_END
#endif