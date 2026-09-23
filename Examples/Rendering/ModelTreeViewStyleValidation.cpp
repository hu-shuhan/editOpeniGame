// BUG (2026-09-19): VERTEX data rendered as points, but binding it to the Qt model
// tree checked the surface button and added IG_SURFACE while leaving points unchecked.
// Verify first-load state, real button events, and rebinding without changing styles.
// 修复提交：与本测试首次加入的提交相同，主题为：
// fix(qt): initialize model display buttons from actual view style
// 查询提交号：git log --diff-filter=A --format="%h %s" -- Examples/Rendering/ModelTreeViewStyleValidation.cpp
#include <IQComponents/igQtModelTreeWidget.h>
#include <VTK XML/iGameVTUReader.h>
#include <iGameUnstructuredMesh.h>

#include <QApplication>
#include <QMouseEvent>

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using namespace iGame;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

UnstructuredMesh::Pointer ReadGrid(int cellType, const char* connectivity, int offset) {
    std::ostringstream xml;
    xml << "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\">"
           "<UnstructuredGrid><Piece NumberOfPoints=\"3\" NumberOfCells=\"1\">"
           "<Points><DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">"
           "0 0 0 1 0 0 0 1 0</DataArray></Points><Cells>"
           "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">"
        << connectivity << "</DataArray><DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">"
        << offset << "</DataArray><DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">"
        << cellType << "</DataArray></Cells></Piece></UnstructuredGrid></VTKFile>";
    const auto contents = xml.str();
    auto reader = iGameVTUReader::New();
    reader->SetMemoryBuffer(contents.data(), contents.size());
    Require(reader->Execute(), "VTU fixture could not be read");
    auto mesh = DynamicCast<UnstructuredMesh>(reader->GetOutput());
    Require(mesh != nullptr, "VTU fixture is not an unstructured mesh");
    return mesh;
}

// Button positions in the model tree: bounding box, points, wireframe, fill, selection.
HoverButton* Button(QTreeWidget& tree, ModelTreeWidgetItem* item, int position) {
    auto* widget = tree.itemWidget(item, 1);
    Require(widget && widget->layout(), "model display controls are missing");
    auto* slot = widget->layout()->itemAt(position + 1); // Leading stretch.
    auto* button = slot ? qobject_cast<HoverButton*>(slot->widget()) : nullptr;
    Require(button != nullptr, "model display button is missing");
    return button;
}

void CheckStyle(QTreeWidget& tree, ModelTreeWidgetItem* item, unsigned int expected) {
    auto draw = DynamicCast<DrawObject>(item->getModel()->GetDataObject());
    Require(draw->GetViewStyle() == expected, "binding the model changed its display style");
    Require(Button(tree, item, 1)->isChecked() == bool(expected & IG_POINTS), "point button disagrees with model");
    Require(Button(tree, item, 2)->isChecked() == bool(expected & IG_WIREFRAME), "wireframe button disagrees with model");
    Require(Button(tree, item, 3)->isChecked() == bool(expected & IG_SURFACE), "surface button disagrees with model");
}

void Click(HoverButton* button) {
    // HoverButton handles mousePressEvent directly instead of QPushButton::clicked.
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(5, 5), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(button, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(5, 5), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(button, &release);
}
} // namespace

int main(int argc, char** argv) {
    QApplication application(argc, argv);
    try {
        auto vertices = ReadGrid(1, "2", 1);
        Require(vertices->GetViewStyle() == IG_POINTS, "reader did not choose points for VERTEX data");
        auto vertexModel = Model::New();
        vertexModel->SetDataObject(vertices);
        auto triangle = ReadGrid(5, "0 1 2", 3);
        Require(triangle->GetViewStyle() == IG_SURFACE, "triangle default changed");
        auto triangleModel = Model::New();
        triangleModel->SetDataObject(triangle);

        QTreeWidget tree;
        tree.setColumnCount(2);
        auto* item = new ModelTreeWidgetItem(&tree);
        item->setModel(vertexModel);
        CheckStyle(tree, item, IG_POINTS);
        Require(!vertices->GetShellRenderingOption(), "binding reenabled surface extraction");

        Click(Button(tree, item, 1));
        CheckStyle(tree, item, 0);
        Click(Button(tree, item, 1));
        CheckStyle(tree, item, IG_POINTS);

        item->setModel(triangleModel);
        CheckStyle(tree, item, IG_SURFACE);
        Click(Button(tree, item, 2));
        CheckStyle(tree, item, IG_SURFACE | IG_WIREFRAME);
        Click(Button(tree, item, 3));
        CheckStyle(tree, item, IG_WIREFRAME);
        item->setModel(triangleModel);
        CheckStyle(tree, item, IG_WIREFRAME);

        triangle->SetViewStyle(IG_POINTS | IG_WIREFRAME | IG_SURFACE);
        item->setModel(triangleModel);
        CheckStyle(tree, item, IG_POINTS | IG_WIREFRAME | IG_SURFACE);
        triangle->SetViewStyle(0);
        item->setModel(triangleModel);
        CheckStyle(tree, item, 0);
        item->setModel(vertexModel);
        CheckStyle(tree, item, IG_POINTS);
        Require(triangle->GetViewStyle() == 0, "binding another model changed the previous model");
        std::cout << "PASS: VTU loading, initial Qt buttons, toggles, and model rebinding agree.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
