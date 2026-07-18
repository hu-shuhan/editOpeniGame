#include "IQWidgets/igQtSearchInfoWidget.h"
#include "ui_igQtSearchInfo.h"

#include <QDockWidget>
#include <QHeaderView>
#include <QSignalBlocker>
#include <algorithm>
#include <cmath>

#include <iGameAttributeSet.h>
#include <iGameCell.h>
#include <iGameDataObject.h>
#include <iGameMacro.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

using namespace Ui;
using namespace iGame;

namespace {
QString MakeAttrKey(const QString& name, int component) {
    if (component < 0) { return QString("attr:%1").arg(name); }
    return QString("attr:%1:%2").arg(name).arg(component);
}

QString FormatNumber(double v) {
    return QString::number(v, 'g', 8);
}

QString SanitizeAttrName(const std::string& name, int index) {
    if (name.empty()) { return QString("Attribute_%1").arg(index); }
    return QString::fromStdString(name);
}

bool MatchValue(double propValue, const QString& operatorStr, double value) {
    if (operatorStr == "=") {
        return std::fabs(propValue - value) <= 1e-12 * std::max(1.0, std::fabs(value));
    }
    if (operatorStr == ">") { return propValue > value; }
    if (operatorStr == "<") { return propValue < value; }
    return false;
}
} // namespace

igQtSearchInfoWidget::igQtSearchInfoWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtSearchInfo), m_currentModelData(nullptr), m_currentDataType(0) {
    ui->setupUi(this);
    initUI();
    initConnections();
}

igQtSearchInfoWidget::~igQtSearchInfoWidget() { delete ui; }

QDockWidget* igQtSearchInfoWidget::createDockWidget(QWidget* parent) {
    QDockWidget* dockWidget = new QDockWidget("查找数据", parent);
    dockWidget->setObjectName("dockWidget_SearchInfo");
    dockWidget->setAllowedAreas(Qt::RightDockWidgetArea);

    igQtSearchInfoWidget* widget = new igQtSearchInfoWidget(dockWidget);
    dockWidget->setWidget(widget);
    return dockWidget;
}

void igQtSearchInfoWidget::setCurrentModelData(void* modelData) {
    m_currentModelData = modelData;
    refreshProperties();
    refreshData();
}

void igQtSearchInfoWidget::onDataTypeChanged() {
    m_currentDataType = ui->radioButton_Cells->isChecked() ? 1 : 0;
    refreshProperties();
    refreshData();
}

void igQtSearchInfoWidget::refreshProperties() {
    QSignalBlocker blocker(ui->comboBox_Property);
    ui->comboBox_Property->clear();

    if (!m_currentModelData) { return; }

    if (m_currentDataType == 0) {
        ui->comboBox_Property->addItem("Position X", "pos.x");
        ui->comboBox_Property->addItem("Position Y", "pos.y");
        ui->comboBox_Property->addItem("Position Z", "pos.z");
    } else {
        ui->comboBox_Property->addItem("Centroid X", "pos.x");
        ui->comboBox_Property->addItem("Centroid Y", "pos.y");
        ui->comboBox_Property->addItem("Centroid Z", "pos.z");
        ui->comboBox_Property->addItem("Point Count", "numPts");
    }

    const int attachment = (m_currentDataType == 0) ? IG_POINT : IG_CELL;
    collectAttributeColumns(attachment);

    for (const auto& col: m_attrColumns) {
        ui->comboBox_Property->addItem(col.header, col.key);
    }
}

void igQtSearchInfoWidget::collectAttributeColumns(int attachmentType) {
    m_attrColumns.clear();
    auto* dataObject = static_cast<DataObject*>(m_currentModelData);
    if (!dataObject || !dataObject->GetAttributeSet()) { return; }

    auto* attrSet = dataObject->GetAttributeSet();
    const size_t n = attrSet->GetNumberOfAttributes();
    for (size_t i = 0; i < n; ++i) {
        const auto& attr = attrSet->GetAttribute(static_cast<IGsize>(i));
        if (attr.IsNone() || attr.isDeleted || !attr.pointer) { continue; }
        if (attr.attachmentType != static_cast<IGenum>(attachmentType)) { continue; }

        const QString name = SanitizeAttrName(attr.pointer->GetName(), static_cast<int>(i));
        const int dim = std::max(1, attr.pointer->GetDimension());

        if (dim == 1) {
            AttrColumn col;
            col.key = MakeAttrKey(name, -1);
            col.header = name;
            col.attrIndex = static_cast<int>(i);
            col.component = 0;
            col.dimension = 1;
            m_attrColumns.push_back(col);
        } else {
            static const char* xyz[] = {"X", "Y", "Z"};
            for (int c = 0; c < dim; ++c) {
                AttrColumn col;
                col.key = MakeAttrKey(name, c);
                if (c < 3) {
                    col.header = QString("%1.%2").arg(name).arg(xyz[c]);
                } else {
                    col.header = QString("%1[%2]").arg(name).arg(c);
                }
                col.attrIndex = static_cast<int>(i);
                col.component = c;
                col.dimension = dim;
                m_attrColumns.push_back(col);
            }
        }
    }
}

void igQtSearchInfoWidget::onQueryButtonClicked() { executeQuery(); }

void igQtSearchInfoWidget::onPrevPageClicked() {
    if (m_currentPage <= 0) { return; }
    loadPage(m_currentPage - 1);
}

void igQtSearchInfoWidget::onNextPageClicked() {
    if (m_currentPage + 1 >= pageCount()) { return; }
    loadPage(m_currentPage + 1);
}

int igQtSearchInfoWidget::pageCount() const {
    if (m_totalMatches <= 0) { return 0; }
    return (m_totalMatches + kPageSize - 1) / kPageSize;
}

void igQtSearchInfoWidget::clearResults() {
    m_query = QuerySpec{};
    m_totalMatches = 0;
    m_currentPage = 0;
    m_pageIds.clear();
    ui->tableWidget_Results->clearContents();
    ui->tableWidget_Results->setRowCount(0);
    updatePaginationControls();
}

void igQtSearchInfoWidget::refreshData() {
    clearResults();
    if (!m_currentModelData) { return; }

    const int attachment = (m_currentDataType == 0) ? IG_POINT : IG_CELL;
    collectAttributeColumns(attachment);
    rebuildTableHeaders();
    // No filter: paginate and show all elements automatically.
    loadPage(0);
}

void igQtSearchInfoWidget::initUI() {
    ui->tableWidget_Results->setSortingEnabled(false);
    ui->tableWidget_Results->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_Results->setAlternatingRowColors(true);
    rebuildTableHeaders();
    updatePaginationControls();
}

void igQtSearchInfoWidget::initConnections() {
    connect(ui->pushButton_Query, &QPushButton::clicked, this, &igQtSearchInfoWidget::onQueryButtonClicked);
    connect(ui->pushButton_PrevPage, &QPushButton::clicked, this, &igQtSearchInfoWidget::onPrevPageClicked);
    connect(ui->pushButton_NextPage, &QPushButton::clicked, this, &igQtSearchInfoWidget::onNextPageClicked);
    connect(ui->radioButton_Points, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) { onDataTypeChanged(); }
    });
    connect(ui->radioButton_Cells, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) { onDataTypeChanged(); }
    });
}

void igQtSearchInfoWidget::rebuildTableHeaders() {
    m_headers.clear();
    if (m_currentDataType == 0) {
        m_headers << "ID" << "X" << "Y" << "Z";
    } else {
        m_headers << "ID" << "NumPts" << "Points" << "CentroidX" << "CentroidY" << "CentroidZ";
    }
    for (const auto& col: m_attrColumns) { m_headers << col.header; }

    ui->tableWidget_Results->setColumnCount(m_headers.size());
    ui->tableWidget_Results->setHorizontalHeaderLabels(m_headers);
}

void igQtSearchInfoWidget::updatePaginationControls() {
    const int pages = pageCount();
    const int displayPage = (pages > 0) ? (m_currentPage + 1) : 0;
    ui->label_PageInfo->setText(
            QString("第 %1 / %2 页（共 %3 条，每页 %4）").arg(displayPage).arg(pages).arg(m_totalMatches).arg(kPageSize));
    ui->pushButton_PrevPage->setEnabled(m_currentPage > 0);
    ui->pushButton_NextPage->setEnabled(pages > 0 && m_currentPage + 1 < pages);
}

double igQtSearchInfoWidget::readAttributeComponent(int attrIndex, int elementIndex, int component) const {
    auto* dataObject = static_cast<DataObject*>(m_currentModelData);
    if (!dataObject || !dataObject->GetAttributeSet()) { return 0.0; }

    const auto& attr = dataObject->GetAttributeSet()->GetAttribute(static_cast<IGsize>(attrIndex));
    if (attr.IsNone() || !attr.pointer) { return 0.0; }

    const int dim = std::max(1, attr.pointer->GetDimension());
    const IGsize valueIndex = static_cast<IGsize>(elementIndex) * static_cast<IGsize>(dim) + static_cast<IGsize>(component);
    if (valueIndex >= attr.pointer->GetNumberOfValues()) { return 0.0; }
    return attr.pointer->GetValue(valueIndex);
}

int igQtSearchInfoWidget::getElementCount() const {
    auto* dataObject = static_cast<DataObject*>(m_currentModelData);
    if (!dataObject) { return 0; }

    if (m_currentDataType == 0) {
        auto points = dataObject->GetPoints();
        return points ? static_cast<int>(points->GetNumberOfPoints()) : 0;
    }

    if (auto* surface = DynamicCast<SurfaceMesh>(dataObject)) {
        return static_cast<int>(surface->GetNumberOfFaces());
    }
    if (auto* volume = DynamicCast<VolumeMesh>(dataObject)) {
        return static_cast<int>(volume->GetNumberOfVolumes());
    }
    if (auto* umesh = DynamicCast<UnstructuredMesh>(dataObject)) {
        return static_cast<int>(umesh->GetNumberOfCells());
    }
    if (auto cells = dataObject->GetCellArray()) {
        return static_cast<int>(cells->GetNumberOfCells());
    }
    return 0;
}

bool igQtSearchInfoWidget::readCellGeometry(int cellId, int& numPts, QString& ptsCsv, double& cx, double& cy,
                                            double& cz) const {
    auto* dataObject = static_cast<DataObject*>(m_currentModelData);
    if (!dataObject) { return false; }

    auto points = dataObject->GetPoints();
    igIndex vhs[IGAME_CELL_MAX_SIZE];
    int npts = 0;

    if (auto* surface = DynamicCast<SurfaceMesh>(dataObject)) {
        npts = surface->GetFacePointIds(cellId, vhs);
    } else if (auto* volume = DynamicCast<VolumeMesh>(dataObject)) {
        auto* cells = volume->GetVolumes();
        if (!cells) { return false; }
        npts = cells->GetCellIds(cellId, vhs);
    } else if (auto* umesh = DynamicCast<UnstructuredMesh>(dataObject)) {
        npts = umesh->GetCellPointIds(cellId, vhs);
    } else if (auto cells = dataObject->GetCellArray()) {
        npts = cells->GetCellIds(cellId, vhs);
    } else {
        return false;
    }

    numPts = npts;
    QStringList idList;
    idList.reserve(npts);
    cx = cy = cz = 0.0;
    int valid = 0;
    for (int k = 0; k < npts; ++k) {
        idList << QString::number(vhs[k]);
        if (points && vhs[k] >= 0 && static_cast<IGsize>(vhs[k]) < points->GetNumberOfPoints()) {
            const auto p = points->GetPoint(vhs[k]);
            cx += p[0];
            cy += p[1];
            cz += p[2];
            ++valid;
        }
    }
    if (valid > 0) {
        cx /= valid;
        cy /= valid;
        cz /= valid;
    }
    ptsCsv = idList.join(",");
    return true;
}

bool igQtSearchInfoWidget::readPropertyValue(int elementId, const QString& property, double& outValue) const {
    auto* dataObject = static_cast<DataObject*>(m_currentModelData);
    if (!dataObject) { return false; }

    if (m_currentDataType == 0) {
        auto points = dataObject->GetPoints();
        if (!points || elementId < 0 || static_cast<IGsize>(elementId) >= points->GetNumberOfPoints()) {
            return false;
        }
        if (property == "pos.x" || property == "pos.y" || property == "pos.z") {
            const auto point = points->GetPoint(elementId);
            outValue = (property == "pos.x") ? point[0] : (property == "pos.y") ? point[1] : point[2];
            return true;
        }
    } else {
        int numPts = 0;
        QString ptsCsv;
        double cx = 0.0, cy = 0.0, cz = 0.0;
        if (!readCellGeometry(elementId, numPts, ptsCsv, cx, cy, cz)) { return false; }
        if (property == "pos.x") {
            outValue = cx;
            return true;
        }
        if (property == "pos.y") {
            outValue = cy;
            return true;
        }
        if (property == "pos.z") {
            outValue = cz;
            return true;
        }
        if (property == "numPts") {
            outValue = static_cast<double>(numPts);
            return true;
        }
    }

    for (const auto& col: m_attrColumns) {
        if (col.key == property) {
            outValue = readAttributeComponent(col.attrIndex, elementId, col.component);
            return true;
        }
    }
    return false;
}

void igQtSearchInfoWidget::fillTableRow(int row, int elementId) {
    auto* table = ui->tableWidget_Results;
    auto* dataObject = static_cast<DataObject*>(m_currentModelData);
    if (!dataObject) { return; }

    int col = 0;
    if (m_currentDataType == 0) {
        double x = 0.0, y = 0.0, z = 0.0;
        if (auto points = dataObject->GetPoints()) {
            const auto p = points->GetPoint(elementId);
            x = p[0];
            y = p[1];
            z = p[2];
        }
        table->setItem(row, col++, new QTableWidgetItem(QString::number(elementId)));
        table->setItem(row, col++, new QTableWidgetItem(FormatNumber(x)));
        table->setItem(row, col++, new QTableWidgetItem(FormatNumber(y)));
        table->setItem(row, col++, new QTableWidgetItem(FormatNumber(z)));
    } else {
        int numPts = 0;
        QString ptsCsv;
        double cx = 0.0, cy = 0.0, cz = 0.0;
        readCellGeometry(elementId, numPts, ptsCsv, cx, cy, cz);
        table->setItem(row, col++, new QTableWidgetItem(QString::number(elementId)));
        table->setItem(row, col++, new QTableWidgetItem(QString::number(numPts)));
        table->setItem(row, col++, new QTableWidgetItem(ptsCsv));
        table->setItem(row, col++, new QTableWidgetItem(FormatNumber(cx)));
        table->setItem(row, col++, new QTableWidgetItem(FormatNumber(cy)));
        table->setItem(row, col++, new QTableWidgetItem(FormatNumber(cz)));
    }

    for (const auto& attrCol: m_attrColumns) {
        const double v = readAttributeComponent(attrCol.attrIndex, elementId, attrCol.component);
        table->setItem(row, col++, new QTableWidgetItem(FormatNumber(v)));
    }
}

void igQtSearchInfoWidget::executeQuery() {
    if (!m_currentModelData) {
        clearResults();
        return;
    }

    const QString property = ui->comboBox_Property->currentData().toString();
    const QString operatorStr = ui->comboBox_Operator->currentText();
    const QString valueStr = ui->lineEdit_Value->text().trimmed();

    m_query = QuerySpec{};
    if (!property.isEmpty() && !valueStr.isEmpty()) {
        bool ok = false;
        const double value = valueStr.toDouble(&ok);
        if (ok) {
            m_query.property = property;
            m_query.op = operatorStr;
            m_query.value = value;
            m_query.active = true;
        }
    }

    rebuildTableHeaders();
    loadPage(0);
}

void igQtSearchInfoWidget::loadPage(int page) {
    m_pageIds.clear();
    m_totalMatches = 0;
    m_currentPage = 0;

    if (!m_currentModelData) {
        populateResultsTable();
        updatePaginationControls();
        return;
    }

    const int count = getElementCount();
    const int start = page * kPageSize;
    const int end = start + kPageSize;
    m_pageIds.reserve(kPageSize);

    if (!m_query.active) {
        // No condition: page by contiguous element ids.
        m_totalMatches = count;
        const int pages = pageCount();
        m_currentPage = (pages > 0) ? std::min(std::max(page, 0), pages - 1) : 0;
        const int pageStart = m_currentPage * kPageSize;
        const int pageEnd = std::min(pageStart + kPageSize, count);
        for (int id = pageStart; id < pageEnd; ++id) { m_pageIds.push_back(id); }
    } else {
        int matchIndex = 0;
        for (int id = 0; id < count; ++id) {
            double propValue = 0.0;
            if (!readPropertyValue(id, m_query.property, propValue)) { continue; }
            if (!MatchValue(propValue, m_query.op, m_query.value)) { continue; }

            if (matchIndex >= start && matchIndex < end) { m_pageIds.push_back(id); }
            ++matchIndex;
        }

        m_totalMatches = matchIndex;
        const int pages = pageCount();
        m_currentPage = (pages > 0) ? std::min(page, pages - 1) : 0;
        if (pages > 0 && page >= pages) {
            loadPage(pages - 1);
            return;
        }
    }

    populateResultsTable();
    updatePaginationControls();
}

void igQtSearchInfoWidget::populateResultsTable() {
    auto* table = ui->tableWidget_Results;
    table->setUpdatesEnabled(false);

    table->clearContents();
    table->setRowCount(m_pageIds.size());
    table->setColumnCount(m_headers.size());
    table->setHorizontalHeaderLabels(m_headers);

    for (int i = 0; i < m_pageIds.size(); ++i) {
        fillTableRow(i, m_pageIds[i]);
    }

    table->setUpdatesEnabled(true);
    table->resizeColumnsToContents();
}
