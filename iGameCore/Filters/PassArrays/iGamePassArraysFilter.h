#ifndef iGamePassArraysFilter_h
#define iGamePassArraysFilter_h

#include "iGameDataObject.h"
#include "iGameFilter.h"
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class PassArrays : public Filter {
public:
    I_OBJECT(PassArrays);
    static Pointer New() { return new PassArrays; }

    /* ---------- 点数据 (Point Data) 选择 ---------- */
    // 设置需要保留的点属性名称
    void SetPointArrayNames(const std::vector<std::string>& names);
    void AddPointArrayName(const std::string& name);
    const std::vector<std::string>& GetPointArrayNames() const { return m_PointArrayNames; }

    /* ---------- 单元数据 (Cell Data) 选择 ---------- */
    // 设置需要保留的单元属性名称
    void SetCellArrayNames(const std::vector<std::string>& names);
    void AddCellArrayName(const std::string& name);
    const std::vector<std::string>& GetCellArrayNames() const { return m_CellArrayNames; }

    // 同时设置点数据与单元数据的选择（两者分开保存，不会互相影响）
    void SetArrayNames(const std::vector<std::string>& pointNames,
                       const std::vector<std::string>& cellNames);

    // 清空所有选择（输出将不保留任何属性数组）
    void ClearArrayNames();

    bool Execute() override;

protected:
    PassArrays() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~PassArrays() override = default;

private:
    std::vector<std::string> m_PointArrayNames;
    std::vector<std::string> m_CellArrayNames;
};

IGAME_NAMESPACE_END

#endif
