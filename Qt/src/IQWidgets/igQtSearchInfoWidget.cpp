#include "IQWidgets/igQtSearchInfoWidget.h"
#include "ui_igQtSearchInfo.h"

#include <QDockWidget>
#include <QHeaderView>
#include <QSignalBlocker>
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

void igQtSearchInfoWidget::refreshData() {
    if (!m_currentModelData) {
        ui->tableWidget_Results->clearContents();
        ui->tableWidget_Results->setRowCount(0);
        m_allData.clear();
        return;
    }
    readModelData();
}

void igQtSearchInfoWidget::initUI() {
    ui->tableWidget_Results->setSortingEnabled(true);
    ui->tableWidget_Results->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_Results->setAlternatingRowColors(true);
    rebuildTableHeaders();
}

void igQtSearchInfoWidget::initConnections() {
    connect(ui->pushButton_Query, &QPushButton::clicked, this, &igQtSearchInfoWidget::onQueryButtonClicked);
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

void igQtSearchInfoWidget::readModelData() {
    if (!m_currentModelData) { return; }
    m_allData.clear();

    const int attachment = (m_currentDataType == 0) ? IG_POINT : IG_CELL;
    collectAttributeColumns(attachment);

    if (m_currentDataType == 0) {
        readPointData();
    } else {
        readCellData();
    }

    rebuildTableHeaders();
    populateResultsTable(m_allData);
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

void igQtSearchInfoWidget::appendAttributeValues(QMap<QString, QVariant>& item, int elementIndex, int /*attachmentType*/) {
    for (const auto& col: m_attrColumns) {
        item[col.key] = readAttributeComponent(col.attrIndex, elementIndex, col.component);
    }
}

void igQtSearchInfoWidget::readPointData() {
    auto* dataObject = static_cast<DataObject*>(m_currentModelData);
    auto points = dataObject->GetPoints();
    if (!points) { return; }

    const int numPoints = static_cast<int>(points->GetNumberOfPoints());
    m_allData.reserve(numPoints);
    for (int i = 0; i < numPoints; ++i) {
        const auto point = points->GetPoint(i);
        QMap<QString, QVariant> item;
        item["id"] = i;
        item["x"] = point[0];
        item["y"] = point[1];
        item["z"] = point[2];
        appendAttributeValues(item, i, IG_POINT);
        m_allData.append(item);
    }
}

void igQtSearchInfoWidget::readCellData() {
    auto* dataObject = static_cast<DataObject*>(m_currentModelData);
    auto points = dataObject->GetPoints();

    auto appendCell = [&](int cellId, const igIndex* ids, int npts) {
        QMap<QString, QVariant> item;
        item["id"] = cellId;
        item["numPts"] = npts;

        QStringList idList;
        idList.reserve(npts);
        double cx = 0.0, cy = 0.0, cz = 0.0;
        int valid = 0;
        for (int k = 0; k < npts; ++k) {
            idList << QString::number(ids[k]);
            if (points && ids[k] >= 0 && static_cast<IGsize>(ids[k]) < points->GetNumberOfPoints()) {
                const auto p = points->GetPoint(ids[k]);
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
        item["pts"] = idList.join(",");
        item["x"] = cx;
        item["y"] = cy;
        item["z"] = cz;
        appendAttributeValues(item, cellId, IG_CELL);
        m_allData.append(item);
    };

    igIndex vhs[IGAME_CELL_MAX_SIZE];

    if (auto* surface = DynamicCast<SurfaceMesh>(dataObject)) {
        const int n = static_cast<int>(surface->GetNumberOfFaces());
        m_allData.reserve(n);
        for (int i = 0; i < n; ++i) {
            const int npts = surface->GetFacePointIds(i, vhs);
            appendCell(i, vhs, npts);
        }
        return;
    }

    if (auto* volume = DynamicCast<VolumeMesh>(dataObject)) {
        auto* cells = volume->GetVolumes();
        if (!cells) { return; }
        const int n = static_cast<int>(volume->GetNumberOfVolumes());
        m_allData.reserve(n);
        for (int i = 0; i < n; ++i) {
            const int npts = cells->GetCellIds(i, vhs);
            appendCell(i, vhs, npts);
        }
        return;
    }

    if (auto* umesh = DynamicCast<UnstructuredMesh>(dataObject)) {
        const int n = static_cast<int>(umesh->GetNumberOfCells());
        m_allData.reserve(n);
        for (int i = 0; i < n; ++i) {
            const int npts = umesh->GetCellPointIds(i, vhs);
            appendCell(i, vhs, npts);
        }
        return;
    }

    // Fallback: generic cell array if present
    if (auto cells = dataObject->GetCellArray()) {
        const int n = static_cast<int>(cells->GetNumberOfCells());
        m_allData.reserve(n);
        for (int i = 0; i < n; ++i) {
            const int npts = cells->GetCellIds(i, vhs);
            appendCell(i, vhs, npts);
        }
    }
}

void igQtSearchInfoWidget::executeQuery() {
    if (m_allData.isEmpty()) { return; }

    const QString property = ui->comboBox_Property->currentData().toString();
    const QString operatorStr = ui->comboBox_Operator->currentText();
    const QString valueStr = ui->lineEdit_Value->text().trimmed();

    if (property.isEmpty() || valueStr.isEmpty()) {
        populateResultsTable(m_allData);
        return;
    }

    bool ok = false;
    const double value = valueStr.toDouble(&ok);
    if (!ok) {
        populateResultsTable(m_allData);
        return;
    }

    QList<QMap<QString, QVariant>> results;
    results.reserve(m_allData.size());

    for (const QMap<QString, QVariant>& item: m_allData) {
        double propValue = 0.0;
        if (property == "pos.x") {
            propValue = item.value("x").toDouble();
        } else if (property == "pos.y") {
            propValue = item.value("y").toDouble();
        } else if (property == "pos.z") {
            propValue = item.value("z").toDouble();
        } else if (property == "numPts") {
            propValue = item.value("numPts").toDouble();
        } else if (item.contains(property)) {
            propValue = item.value(property).toDouble();
        } else {
            continue;
        }

        bool match = false;
        if (operatorStr == "=") {
            match = std::fabs(propValue - value) <= 1e-12 * std::max(1.0, std::fabs(value));
        } else if (operatorStr == ">") {
            match = (propValue > value);
        } else if (operatorStr == "<") {
            match = (propValue < value);
        }

        if (match) { results.append(item); }
    }

    populateResultsTable(results);
}

void igQtSearchInfoWidget::populateResultsTable(const QList<QMap<QString, QVariant>>& results) {
    auto* table = ui->tableWidget_Results;
    const bool sorting = table->isSortingEnabled();
    table->setSortingEnabled(false);
    table->setUpdatesEnabled(false);

    table->clearContents();
    table->setRowCount(results.size());
    table->setColumnCount(m_headers.size());
    table->setHorizontalHeaderLabels(m_headers);

    for (int i = 0; i < results.size(); ++i) {
        const QMap<QString, QVariant>& item = results[i];
        int col = 0;

        if (m_currentDataType == 0) {
            table->setItem(i, col++, new QTableWidgetItem(QString::number(item.value("id").toInt())));
            table->setItem(i, col++, new QTableWidgetItem(FormatNumber(item.value("x").toDouble())));
            table->setItem(i, col++, new QTableWidgetItem(FormatNumber(item.value("y").toDouble())));
            table->setItem(i, col++, new QTableWidgetItem(FormatNumber(item.value("z").toDouble())));
        } else {
            table->setItem(i, col++, new QTableWidgetItem(QString::number(item.value("id").toInt())));
            table->setItem(i, col++, new QTableWidgetItem(QString::number(item.value("numPts").toInt())));
            table->setItem(i, col++, new QTableWidgetItem(item.value("pts").toString()));
            table->setItem(i, col++, new QTableWidgetItem(FormatNumber(item.value("x").toDouble())));
            table->setItem(i, col++, new QTableWidgetItem(FormatNumber(item.value("y").toDouble())));
            table->setItem(i, col++, new QTableWidgetItem(FormatNumber(item.value("z").toDouble())));
        }

        for (const auto& attrCol: m_attrColumns) {
            table->setItem(i, col++, new QTableWidgetItem(FormatNumber(item.value(attrCol.key).toDouble())));
        }
    }

    table->setUpdatesEnabled(true);
    table->setSortingEnabled(sorting);
    table->resizeColumnsToContents();
}
