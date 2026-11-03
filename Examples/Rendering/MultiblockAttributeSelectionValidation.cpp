// 回归（待提交）：嵌套 VTM 叶块的数据有效，但属性点击没有更新选中对象和色条，
// 颜色范围停留在根对象默认 0..255；壳网格也未初始化拥有者 mapper。
// 使用用户模型逐块核对实际数组/缓存范围/mapper，发送真实鼠标事件，验证图例和面板。
// 包含全负 wedge 场，防止 DBL_MIN 被误用为最大值初始值；保留单模型和固定范围行为。
#include <IQCore/igQtMainWindow.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQComponents/igQtModelTreeWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtColorBarWidget.h>
#include <IQWidgets/igQtScalarViewWidget.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iGameInteractor.h>
#include <iGameSurfaceMesh.h>
#include <QApplication>
#include <QMouseEvent>
#include <QLabel>
#include <QDir>
#include <QSettings>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace iGame;
void Check(bool ok, const std::string& message) { if (!ok) throw std::runtime_error(message); }
void Near(double a, double b, const std::string& message) { Check(std::abs(a-b) < 1e-5, message + ": " + std::to_string(a) + " != " + std::to_string(b)); }
class CpuRenderer : public igQtModelDrawWidget {
public:
    CpuRenderer(QWidget* parent) : igQtModelDrawWidget(parent) {
        m_Scene = SceneManager::Instance()->NewScene();
        m_Interactor = Interactor::New();
        m_Interactor->Initialize(m_Scene);
        m_Scene->SetInteractor(m_Interactor);
    }
};
struct TreeRestore {
    igQtModelTreeWidget* tree; QWidget* parent;
    ~TreeRestore() { tree->hide(); tree->setParent(parent); }
};
std::map<std::string, DrawObject::Pointer> Leaves(DataObject::Pointer root) {
    std::map<std::string, DrawObject::Pointer> result;
    std::function<void(DataObject::Pointer)> walk = [&](DataObject::Pointer object) {
        if (object->HasSubDataObject()) {
            for (auto it=object->SubDataObjectIteratorBegin(); it!=object->SubDataObjectIteratorEnd(); ++it) walk(it->second);
        } else result[object->GetName()] = DynamicCast<DrawObject>(object);
    };
    walk(root); return result;
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(iGameQtMainWindow);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::currentPath()+"/multiblock-range-settings");
    try {
        DataObject::DeferDrawableConversionScope cpuOnly;
        igQtMainWindow window;
        window.rendererWidget = new CpuRenderer(&window);
        auto* scene = window.rendererWidget->GetScene();
        auto data = FileIO::ReadFile(argc > 1 ? argv[1] : "Models/multiblock_surface_test/multiblock_surface_test.vtm");
        Check(data != nullptr, "VTM read failed");
        auto leaves = Leaves(data);
        const std::map<std::string,std::pair<double,double>> expected = {
            {"hex_grid",{-4,12}}, {"tet_grid",{3,19}}, {"wedge_pair",{-16,-4}},
            {"pyramid_pair",{-8.5,3.5}}, {"surface_patch",{-3,12.4}}};
        Check(leaves.size()==expected.size(), "missing VTM leaves");
        for (const auto& entry : expected) {
            auto object = leaves.at(entry.first);
            auto attrs = object->GetAttributeSet();
            const int index = attrs->GetAttributeIndex("ProbeValue");
            Check(index >= 0, "missing ProbeValue");
            auto& attr = attrs->GetAttribute(index);
            double lo=1e100,hi=-1e100;
            for (IGsize i=0;i<attr.pointer->GetNumberOfValues();++i) { lo=std::min(lo,attr.pointer->GetValue(i)); hi=std::max(hi,attr.pointer->GetValue(i)); }
            Near(lo,entry.second.first,"read minimum "+entry.first);
            Near(hi,entry.second.second,"read maximum "+entry.first);
            auto range=attr.GetDataRange();
            Near(range->GetValue(2),lo,"cached minimum "+entry.first);
            Near(range->GetValue(3),hi,"cached maximum "+entry.first);
            object->ViewCloudPicture(scene,index,0);
            Near(object->GetColorMapper()->GetRange()[0],lo,"mapper minimum "+entry.first);
            Near(object->GetColorMapper()->GetRange()[1],hi,"mapper maximum "+entry.first);
            object->ViewCloudPicture(scene,-1);
            std::cout << entry.first << " raw/cache/mapper: [" << lo << ", " << hi << "]\n";
        }
        window.modelTreeWidget->addDataObjectToModelTree(data,Algorithm);
        auto* tree=window.findChild<igQtModelTreeWidget*>();
        Check(tree != nullptr,"missing tree");
        TreeRestore restore{tree,tree->parentWidget()};
        tree->setParent(nullptr); tree->resize(800,900); tree->show();
        std::vector<SubAttribTreeWidgetItem*> rows;
        std::function<void(QTreeWidgetItem*)> expand=[&](QTreeWidgetItem* item) {
            tree->expandItem(item);
            for (int i=0;i<item->childCount();++i) {
                auto* child=item->child(i);
                if (auto* attr=dynamic_cast<SubAttribTreeWidgetItem*>(child)) {
                    if (child->text(0)=="ProbeValue") rows.push_back(attr);
                } else expand(child);
            }
        };
        expand(tree->topLevelItem(0)); tree->doItemsLayout(); app.processEvents();
        Check(rows.size()==5,"missing lazy subattribute rows");
        auto* scalar=window.findChild<igQtScalarViewWidget*>();
        auto* bar=window.rendererWidget->getColorBarWidget(); bar->resize(150,250);
        for (int repeat=0; repeat<2; ++repeat) for (auto* row:rows) {
            auto* owner=dynamic_cast<SubObjectTreeWidgetItem*>(row->parent());
            auto object=owner->getDataObject();
            tree->scrollToItem(row); tree->doItemsLayout(); app.processEvents();
            const auto rect=tree->visualItemRect(row);
            const QPoint point(rect.left()+30,rect.center().y());
            Check(tree->itemAt(point)==row,"row not clickable");
            QMouseEvent press(QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(tree->viewport(),&press);
            QMouseEvent release(QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(tree->viewport(),&release);
            Check(tree->currentItem()==row,"subattribute context not selected");
            Check(window.modelTreeWidget->GetCurrentDataObject()==object,"context resolves to assembly root");
            const auto bounds=expected.at(object->GetName());
            Near(object->GetColorMapper()->GetRange()[0],bounds.first,"clicked minimum");
            Near(object->GetColorMapper()->GetRange()[1],bounds.second,"clicked maximum");
            auto* label=scalar->findChild<QLabel*>("labelScalarInfo");
            Check(label && label->text().contains(QString::number(bounds.first,'e',6)) && label->text().contains(QString::number(bounds.second,'e',6)),"scalar panel range/name stale");
            // paintEvent fits the legend width to the labels; settle that layout
            // before comparing pixels so a previous block's width is not captured.
            (void)bar->grab();
            const auto actual=bar->grab().toImage();
            bar->setCurrentDataObject(object);
            (void)bar->grab();
            const auto expectedLegend=bar->grab().toImage();
            Check(actual==expectedLegend,"legend did not follow clicked subblock: " + object->GetName());
            bar->setCurrentDataObject(nullptr);
        }
        // Fixed ranges remain fixed, including the owning mapper's stability override.
        auto hex=leaves.at("hex_grid");
        hex->GetColorMapper()->SetRange(100,200); hex->GetColorMapper()->SetRangeStable(true);
        hex->ViewCloudPicture(scene,-1); hex->ViewCloudPicture(scene,0,0);
        Near(hex->GetColorMapper()->GetRange()[0],100,"stable range minimum overwritten");
        Near(hex->GetColorMapper()->GetRange()[1],200,"stable range maximum overwritten");
        std::cout << "PASS multiblock data, owner ranges, negative values, repeated tree clicks, scalar panel, legend and fixed mapper\n";
        return 0;
    } catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
