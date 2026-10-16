#pragma once

#include <iGameFilter.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

// Validity state bitmask, strictly aligned with VTK vtkCellValidator::State
// (VTK 9.3 / ParaView). A cell may have several issues at once; the stored
// value is the bitwise OR of these flags. 0 means Valid.
enum ValidityState : unsigned short {
    Validity_Valid                       = 0x00,
    Validity_WrongNumberOfPoints         = 0x01,
    Validity_IntersectingEdges           = 0x02,
    Validity_IntersectingFaces           = 0x04,
    Validity_NoncontiguousEdges          = 0x08,
    Validity_Nonconvex                   = 0x10,
    Validity_FacesAreOrientedIncorrectly = 0x20,
};

class ValidateCellsFilter : public Filter {
public:
    I_OBJECT(ValidateCellsFilter);
    static Pointer New() { return new ValidateCellsFilter; }

    bool Execute() override;

    /// 输出单元标量名称，与 ParaView 保持一致。
    static const char* ValidityStateArrayName() { return "ValidityState"; }

    /// 将位掩码解析为“边相交、非凸”这样的可读文本。
    static std::string GetValidityStateText(int state);

    /// 每个被检查元素（UnstructuredMesh 的 cell，或 SurfaceMesh 的 face）的状态。
    const std::vector<short>& GetValidityStates() const { return m_ValidityStates; }

    /// 几何无效单元 id 列表（state != Valid）。
    const std::vector<igIndex>& GetInvalidCellIds() const { return m_InvalidCellIds; }

    int GetCheckedCellCount() const { return static_cast<int>(m_ValidityStates.size()); }
    int GetInvalidCellCount() const { return static_cast<int>(m_InvalidCellIds.size()); }
    int GetValidCellCount() const {
        return GetCheckedCellCount() - GetInvalidCellCount();
    }

    const std::string& GetLastError() const { return m_LastError; }

protected:
    ValidateCellsFilter();
    ~ValidateCellsFilter() override = default;

private:
    void ResetResults(IGsize count);
    void RecordCell(IGsize cellId, short state);
    void ApplyResultAttribute(DataObject* output);

    std::vector<short> m_ValidityStates;
    std::vector<igIndex> m_InvalidCellIds;
    std::string m_LastError;
};

IGAME_NAMESPACE_END
