// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
// Regression trigger: the region filter's Preview button was present in code
// but absent from the launched dialog. Expected: it is visible and clickable,
// while ordinary filter dialogs retain their two-button layout.
// Fix: "feat: integrate region extraction with latest main". Find its SHA with
// git log --diff-filter=A --format="%h %s" -- Examples/Filter/Selection/TestExtractCellsByRegionDialog.cpp
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <QApplication>
#include <QPushButton>
#include <iostream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    std::cerr << "Creating region dialog\n";
    igQtFilterDialogDockWidget dialog(nullptr, true);
    dialog.setFilterTitle(QStringLiteral("按区域提取单元"));
    dialog.enablePreview(true);
    dialog.addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                        QStringLiteral("区域"),
                        std::vector<QString>{QStringLiteral("Box"), QStringLiteral("Sphere")});
    for (int i = 0; i < 7; ++i)
        dialog.addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                            QStringLiteral("坐标 %1").arg(i), QStringLiteral("0"));
    dialog.addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("所有顶点都在区域内"), QStringLiteral("true"));
    int previewCalls = 0, applyCalls = 0, closeCalls = 0;
    dialog.setPreviewFunctor([&] { ++previewCalls; });
    dialog.setApplyFunctor([&] { ++applyCalls; });
    dialog.setCloseFunctor([&] { ++closeCalls; });
    std::cerr << "Showing region dialog\n";
    dialog.show();
    app.processEvents();
    std::cerr << "Checking region buttons\n";

    auto* preview = dialog.findChild<QPushButton*>(QStringLiteral("previewButton"));
    auto* apply = dialog.findChild<QPushButton*>(QStringLiteral("applyButton"));
    if (!preview || !apply || !preview->isVisible() || !apply->isVisible() ||
        !dialog.rect().contains(preview->mapTo(&dialog, preview->rect().center()))) {
        std::cerr << "Preview or Apply is not visible in the region dialog\n";
        return 1;
    }
    if (argc > 1 && !dialog.grab().save(QString::fromLocal8Bit(argv[1]))) {
        std::cerr << "Could not save dialog evidence\n";
        return 2;
    }
    std::cerr << "Clicking region buttons\n";
    preview->click();
    apply->click();
    if (previewCalls != 1 || applyCalls != 1) {
        std::cerr << "Preview/Apply did not invoke callbacks\n";
        return 3;
    }
    dialog.close();
    if (closeCalls != 1) {
        std::cerr << "Close cleanup callback did not run\n";
        return 4;
    }
    igQtFilterDialogDockWidget ordinary;
    ordinary.show();
    app.processEvents();
    auto* ordinaryPreview = ordinary.findChild<QPushButton*>(QStringLiteral("previewButton"));
    if (!ordinaryPreview || ordinaryPreview->isVisible()) {
        std::cerr << "Preview leaked into an ordinary filter dialog\n";
        return 5;
    }
    ordinary.close();
    std::cout << "Region dialog Preview, Apply and Close passed\n";
    return 0;
}
