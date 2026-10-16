#pragma once

#include "iGameFilter.h"
#include "iGameFlatArray.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class PointAndCellIdsFilter : public Filter {
public:
    I_OBJECT(PointAndCellIdsFilter);
    static Pointer New() { return new PointAndCellIdsFilter; }

    bool Execute() override;

    void SetGeneratePointIds(bool value);
    bool GetGeneratePointIds() const { return m_GeneratePointIds; }

    void SetGenerateCellIds(bool value);
    bool GetGenerateCellIds() const { return m_GenerateCellIds; }

    void SetPointIdsArrayName(const std::string& name);
    const std::string& GetPointIdsArrayName() const { return m_PointIdsArrayName; }

    void SetCellIdsArrayName(const std::string& name);
    const std::string& GetCellIdsArrayName() const { return m_CellIdsArrayName; }

    LongLongArray::Pointer GetPointIdsArray() const { return m_PointIdsArray; }
    LongLongArray::Pointer GetCellIdsArray() const { return m_CellIdsArray; }

    const std::string& GetMessage() const { return m_Message; }

protected:
    PointAndCellIdsFilter();
    ~PointAndCellIdsFilter() override = default;

private:
    bool m_GeneratePointIds{true};
    bool m_GenerateCellIds{true};

    std::string m_PointIdsArrayName{"vtkPointIds"};
    std::string m_CellIdsArrayName{"vtkCellIds"};

    LongLongArray::Pointer m_PointIdsArray{nullptr};
    LongLongArray::Pointer m_CellIdsArray{nullptr};

    std::string m_Message;
};

IGAME_NAMESPACE_END
