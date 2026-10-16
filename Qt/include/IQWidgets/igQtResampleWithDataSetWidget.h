#pragma once
#include "iGameDataObject.h"
#include "iGameModel.h"

#include <ui_ResampleWithDataSet.h>

#include <QWidget>

#include <string>

class igQtModelDialogWidget;

/**
 * @brief 重采样至数据集面板（对标 ParaView 的 Resample With Dataset）
 *
 * 面板提供两个模型输入：
 *   - 「被采样网格」：提供数据的网格（VTK 的 Source）；
 *   - 「采样点网格」：探针几何（VTK 的 Input），它的所有点作为采样位置，
 *     执行结果就是它的同类型深拷贝 + 采样得到的属性数组。
 *
 * 两个下拉框列出场景中的全部模型（Scene::GetAllModels），默认「采样点网格」取当前模型。
 * 输入模型被删除时只清空对应的下拉框，面板保持打开，用户可重新选择输入。
 */
class igQtResampleWithDataSet : public QWidget {
    Q_OBJECT

public:
    igQtResampleWithDataSet(igQtModelDialogWidget* modelTreeWidget, QWidget* parent = nullptr);
    ~igQtResampleWithDataSet() override;

    /** 显式指定模型树控件（为空时自动从顶层窗口查找） */
    void SetModelTreeWidget(igQtModelDialogWidget* modelTreeWidget);

public slots:
    /** 重新枚举场景中的模型填充两个下拉框（尽量保持原有选择） */
    void RefreshModelList();
    /** 供主窗口打开面板时调用：刷新模型列表，并把「采样点网格」默认设为当前模型 */
    void BindCurrentModel();
    /** 执行重采样，并把结果加入模型树 */
    void Execute();

protected:
    void showEvent(QShowEvent* event) override;

private:
    /** 查找主窗口中的模型树控件（避免使用构造时可能失效的指针） */
    igQtModelDialogWidget* ResolveModelTreeWidget();

    /** which: 0 = 被采样网格，1 = 采样点网格 */
    void DetachInputObserver(int which);
    void AttachInputObserver(int which, iGame::DataObject::Pointer object);
    /** 输入被删除：清空对应下拉框（面板不关闭） */
    void ClearInputSelection(int which);
    /** 下拉框当前选中的模型；没有选中返回 nullptr */
    iGame::Model* ResolveModel(int which) const;
    iGame::DataObject::Pointer ResolveDataObject(int which) const;
    /** 结果命名：base 与场景中已有模型重名时追加 _1、_2 … */
    std::string MakeUniqueResultName(const std::string& base) const;

    Ui::ResampleWithDataSetWidget* ui;
    igQtModelDialogWidget* m_ModelTreeWidget{nullptr};

    iGame::DataObject::Pointer m_SourceDataObject{nullptr};
    iGame::DataObject::Pointer m_ProbeDataObject{nullptr};
    unsigned long m_SourceObserverTag{0};
    unsigned long m_ProbeObserverTag{0};
};
