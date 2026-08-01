#include <IQWidgets/igQtDataCodecCompressionWidget.h>

#include <QAbstractSpinBox>
#include <QAbstractItemView>
#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QSize>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QStringList>
#include <QThread>
#include <QVariant>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <DataCodec/API/Adapter/IEncodeAdapter.h>
#include <DataCodec/Filter/Adapter/iGameDataCodecAttributeCatalog.h>
#include <DataCodec/Filter/Localization/iGameDataCodecHostMessage.h>
#include <DataCodec/Filter/Output/iGameDataCodecOutputSinks.h>
#include <DataCodec/Filter/Telemetry/iGameDataCodecTelemetryCapture.h>
#include <DataCodec/API/Params/TimeSeriesControlParams.h>
#include <DataCodec/API/Params/NumericArrayParams.h>
#include <DataCodec/API/Params/ReferenceControlParams.h>
#include <DataCodec/Log/Analysis/AdapterPrecisionMetrics.h>
#include <DataCodec/Filter/Adapter/iGameBlockTreeAdapter.h>
#include <DataCodec/Log/Report/DataCodecProcessReportJson.h>
#include <FeatureExtraction/iGameRegionFeatureBasisFilter.h>
#include <IGDC/iGameDataCodecIOSettings.h>
#include <IGDC/iGameIGDCReader.h>
#include <IGDC/iGameIGDCWriter.h>
#include <IQWidgets/igQtDataCodecUiSink.h>
#include <iGameAttributeSet.h>
#include <iGameDataObject.h>
#include <iGamePointSet.h>
#include <iGameType.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <filesystem>
#include <iterator>
#include <limits>
#include <memory>
#include <map>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr int kHistogramEdgeInset = 16;
constexpr int kSmartHistogramLeftInset = 86;
constexpr int kHistogramBinCount = 34;
constexpr int kRawBackgroundDetectBinCount = 72;
constexpr int kRangeEndpointCount = 101;
constexpr int kMaxCustomRegionCount = 8;
constexpr int kDefaultCustomRegionSpan = 24;
constexpr double kBackgroundPeakRatio = 0.45;
constexpr double kBackgroundMergeRatio = 0.16;
constexpr double kDefaultGreenPrecisionRatio = 1.0e-5;
constexpr double kPrecisionGreenLimit = 1.0e-4;
constexpr double kPrecisionNormalLimit = 1.0e-3;
constexpr double kPrecisionWarnLimit = 1.0e-2;
constexpr double kPrecisionHighWarnLimit = 5.0e-2;
constexpr int kPrecisionModeAbs = 0;
constexpr int kPrecisionModeRel = 1;
constexpr auto kSettingsOrganization = "iGame";
constexpr auto kSettingsApplication = "iGameVis";
constexpr auto kEncodeSettingsGroup = "DataCodec/Encode";
constexpr auto kEncodePerformanceTierKey = "PerformanceTier";
constexpr auto kEncodeCompressionEnhancementKey = "CompressionEnhancement";
constexpr auto kEncodeZstdLevelKey = "ZstdLevel";
constexpr auto kEncodeGopFrameCountKey = "GopFrameCount";
constexpr auto kDefaultEncodeTier = ::datacodec::DataCodecEncodeTier::TimePriority;

struct DataCodecCompressionPerformanceSettings {
    ::datacodec::DataCodecEncodeTier tier{kDefaultEncodeTier};
    bool compressionEnhancement{false};
    int zstdLevel{3};
    int gopFrameCount{8};
};

QSettings CreateDataCodecCompressionSettings() {
    return QSettings(
        QSettings::IniFormat,
        QSettings::UserScope,
        QString::fromLatin1(kSettingsOrganization),
        QString::fromLatin1(kSettingsApplication));
}

::datacodec::DataCodecEncodeTier DecodeEncodeTierFromValue(const int value) {
    const auto tier = static_cast<::datacodec::DataCodecEncodeTier>(value);
    switch (tier) {
        case ::datacodec::DataCodecEncodeTier::TimePriority:
        case ::datacodec::DataCodecEncodeTier::Balanced:
        case ::datacodec::DataCodecEncodeTier::MemoryPriority:
            return tier;
    }
    return kDefaultEncodeTier;
}

DataCodecCompressionPerformanceSettings LoadDataCodecCompressionPerformanceSettings() {
    auto storage = CreateDataCodecCompressionSettings();
    storage.beginGroup(QString::fromLatin1(kEncodeSettingsGroup));
    const int storedTierValue = storage.value(
        QString::fromLatin1(kEncodePerformanceTierKey),
        static_cast<int>(kDefaultEncodeTier)).toInt();
    const auto tier = DecodeEncodeTierFromValue(storedTierValue);
    const auto definition = ::datacodec::MakeEncodeConfigurationParams(
        ::datacodec::DataCodecEncodeOptions{.tier = tier});
    const bool compressionEnhancement = storage.value(
        QString::fromLatin1(kEncodeCompressionEnhancementKey),
        false).toBool();
    const auto zstdLevel = qBound(
        1,
        storage.value(
            QString::fromLatin1(kEncodeZstdLevelKey),
            definition.pipelineControl.packageFields.zstdLevel).toInt(),
        22);
    const auto gopFrameCount = qMax(
        1,
        storage.value(QString::fromLatin1(kEncodeGopFrameCountKey), 8).toInt());
    storage.endGroup();
    return DataCodecCompressionPerformanceSettings{
        .tier = tier,
        .compressionEnhancement = compressionEnhancement,
        .zstdLevel = zstdLevel,
        .gopFrameCount = gopFrameCount,
    };
}

void SaveDataCodecCompressionPerformanceSettings(
        const DataCodecCompressionPerformanceSettings& settings) {
    auto storage = CreateDataCodecCompressionSettings();
    storage.beginGroup(QString::fromLatin1(kEncodeSettingsGroup));
    storage.setValue(
        QString::fromLatin1(kEncodePerformanceTierKey),
        static_cast<int>(settings.tier));
    storage.setValue(
        QString::fromLatin1(kEncodeCompressionEnhancementKey),
        settings.compressionEnhancement);
    storage.setValue(QString::fromLatin1(kEncodeZstdLevelKey), settings.zstdLevel);
    storage.setValue(QString::fromLatin1(kEncodeGopFrameCountKey), settings.gopFrameCount);
    storage.endGroup();
    storage.sync();
}

QWidget* resolveDataCodecFileDialogParent(QWidget* source) {
    if (auto* modal = QApplication::activeModalWidget(); modal != nullptr && modal != source) {
        return modal;
    }
    if (auto* active = QApplication::activeWindow(); active != nullptr && active != source) {
        return active;
    }
    if (source != nullptr) {
        if (auto* window = source->window(); window != nullptr && window != source && window->isWindow()) {
            return window;
        }
    }
    return nullptr;
}

class DataCodecComboItemDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), 36));
        return size;
    }
};

QString attachmentName(int attachmentType) {
    switch (attachmentType) {
    case IG_CELL: return QStringLiteral("单元数据");
    case IG_POINT: return QStringLiteral("点数据");
    default: return QStringLiteral("数据");
    }
}

QString localizedFeatureErrorMessage(const QString& message) {
    if (message == QLatin1String("input is empty")) return QStringLiteral("输入数据为空");
    if (message == QLatin1String("attribute set is empty")) return QStringLiteral("属性数据集合为空");
    if (message == QLatin1String("attribute is not found")) return QStringLiteral("未找到指定属性数据");
    if (message == QLatin1String("geometry is empty")) return QStringLiteral("坐标数据为空");
    if (message == QLatin1String("attribute is empty")) return QStringLiteral("属性数据为空");
    if (message == QLatin1String("attribute values are empty")) return QStringLiteral("属性数据值为空");
    if (message == QLatin1String("attribute component is out of range")) {
        return QStringLiteral("属性数据分量超出有效范围");
    }
    return message;
}

QString numericTypeName(IGenum arrayType) {
    switch (arrayType) {
    case IG_FloatArray: return QStringLiteral("float32");
    case IG_DoubleArray: return QStringLiteral("float64");
    case IG_IntArray:
    case IG_INTARRAY: return QStringLiteral("int32");
    case IG_UnsignedIntArray: return QStringLiteral("uint32");
    case IG_CharArray: return QStringLiteral("int8");
    case IG_UnsignedCharArray: return QStringLiteral("uint8");
    case IG_ShortArray: return QStringLiteral("int16");
    case IG_UnsignedShortArray: return QStringLiteral("uint16");
    case IG_LongLongArray: return QStringLiteral("int64");
    case IG_UnsignedLongLongArray: return QStringLiteral("uint64");
    default: return QStringLiteral("数值");
    }
}

QString numericTypeName(::datacodec::DataType dataType) {
    switch (dataType) {
    case ::datacodec::DataType::Float32: return QStringLiteral("float32");
    case ::datacodec::DataType::Float64: return QStringLiteral("float64");
    case ::datacodec::DataType::Int8: return QStringLiteral("int8");
    case ::datacodec::DataType::UInt8: return QStringLiteral("uint8");
    case ::datacodec::DataType::Int16: return QStringLiteral("int16");
    case ::datacodec::DataType::UInt16: return QStringLiteral("uint16");
    case ::datacodec::DataType::Int32: return QStringLiteral("int32");
    case ::datacodec::DataType::UInt32: return QStringLiteral("uint32");
    case ::datacodec::DataType::Int64: return QStringLiteral("int64");
    case ::datacodec::DataType::UInt64: return QStringLiteral("uint64");
    }
    return QStringLiteral("数值");
}

QString fieldDetailName(int attachmentType, const QString& numericType, int componentCount) {
    return QStringLiteral("%1 · %2 · %3 个分量")
            .arg(attachmentName(attachmentType), numericType, QString::number(qMax(1, componentCount)));
}

QString normalizeDataCodecOutputPath(QString path) {
    path = QDir::fromNativeSeparators(path.trimmed());
    if (path.isEmpty()) return path;

    const QFileInfo info(path);
    const QString suffix = info.suffix().toLower();
    if (suffix == QStringLiteral("igc")) return path;

    QString baseName = info.completeBaseName();
    if (baseName.isEmpty()) baseName = info.fileName();
    if (baseName.isEmpty()) baseName = QStringLiteral("output");
    return QDir(info.path()).filePath(baseName + QStringLiteral(".igc"));
}

QString resolveWrittenDataCodecPath(const QString& requestedPath) {
    QFileInfo requestedInfo(requestedPath);
    if (requestedInfo.isFile() && requestedInfo.size() > 0) return requestedInfo.filePath();

    QFileInfo igcInfo(QDir(requestedInfo.path()).filePath(requestedInfo.completeBaseName() + QStringLiteral(".igc")));
    if (igcInfo.isFile() && igcInfo.size() > 0) return igcInfo.filePath();

    return requestedPath;
}

QString makeCompressionReportDirectory(const QString& outputPath) {
    const QFileInfo info(outputPath);
    QString baseName = info.completeBaseName();
    if (baseName.isEmpty()) baseName = QStringLiteral("datacodec");
    return QDir(info.absolutePath()).filePath(baseName + QStringLiteral("_Encode_Report"));
}

QString formatByteSize(qint64 bytes) {
    if (bytes < 0) return QStringLiteral("未知");
    static constexpr double kUnit = 1024.0;
    double value = static_cast<double>(bytes);
    QString unit = QStringLiteral("B");
    if (value >= kUnit) {
        value /= kUnit;
        unit = QStringLiteral("KiB");
    }
    if (value >= kUnit) {
        value /= kUnit;
        unit = QStringLiteral("MiB");
    }
    if (value >= kUnit) {
        value /= kUnit;
        unit = QStringLiteral("GiB");
    }
    return QStringLiteral("%1 %2").arg(value, 0, 'f', unit == QStringLiteral("B") ? 0 : 2).arg(unit);
}

QString formatCompressionRatio(qint64 beforeBytes, qint64 afterBytes) {
    if (beforeBytes <= 0 || afterBytes <= 0) return QStringLiteral("未知");
    return QStringLiteral("%1%").arg(static_cast<double>(afterBytes) * 100.0 / static_cast<double>(beforeBytes), 0, 'f', 2);
}

std::string toUtf8StdString(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

QString dataCodecHostLogText(
    const ::datacodec::DataCodecLanguage language,
    const iGame::iGameDataCodecHostMessageId messageId,
    const std::initializer_list<iGame::iGameDataCodecHostMessageArgument> arguments = {}) {
    const auto text = iGame::iGameDataCodecHostMessage(
        language,
        messageId,
        arguments);
    return QString::fromUtf8(text.data(), static_cast<int>(text.size()));
}

struct DataCodecRunReportWriteResult {
    bool success{false};
    QStringList writtenPaths;
    std::string error;
};

DataCodecRunReportWriteResult writeDataCodecRunReports(
                             const std::shared_ptr<iGame::iGameDataCodecReportFileSink>& reportSink,
                             const std::string& reportFileTimestamp,
                             const ::datacodec::DataCodecProcessReport& processReport,
                             const std::optional<::datacodec::DataCodecErrorReport>& errorReport) {
    DataCodecRunReportWriteResult outcome;
    if (reportSink == nullptr) {
        outcome.error = "report sink is unavailable";
        return outcome;
    }
    const auto operationName = std::string(
        ::datacodec::TelemetryRunKindName(processReport.operation));
    const auto processResult = reportSink->WriteReportFile({
        .name = operationName + "_process_" + reportFileTimestamp,
        .mediaType = "application/json",
        .preferredExtension = ".json",
        .content = ::datacodec::SerializeDataCodecProcessReportJson(processReport),
    });
    if (!processResult.success) {
        outcome.error = processResult.error.empty()
            ? "failed to write process report"
            : processResult.error;
        return outcome;
    }
    outcome.writtenPaths.push_back(QString::fromUtf8(processResult.path.c_str()));
    if (errorReport.has_value() && !errorReport->errors.empty()) {
        const auto errorResult = reportSink->WriteReportFile({
            .name = operationName + "_errors_" + reportFileTimestamp,
            .mediaType = "application/json",
            .preferredExtension = ".json",
            .content = ::datacodec::SerializeDataCodecErrorReportJson(*errorReport),
        });
        if (!errorResult.success) {
            outcome.error = errorResult.error.empty()
                ? "failed to write error report"
                : errorResult.error;
            return outcome;
        }
        outcome.writtenPaths.push_back(QString::fromUtf8(errorResult.path.c_str()));
    }
    outcome.success = true;
    return outcome;
}

qint64 sourceFileSizeFromDataObject(const iGame::DataObject::Pointer& dataObject) {
    if (dataObject == nullptr || dataObject->GetProperties() == nullptr) return -1;
    auto properties = dataObject->GetProperties();
    if (auto property = properties->GetProperty("FilePath"); property != nullptr) {
        const QFileInfo sourceInfo(QString::fromStdString(property->Get<std::string>()));
        if (sourceInfo.isFile()) return sourceInfo.size();
    }
    if (auto property = properties->GetProperty("FileSize"); property != nullptr) {
        const auto bytes = property->Get<long long>();
        return bytes >= 0 ? static_cast<qint64>(bytes) : -1;
    }
    return -1;
}

iGame::DataObject::Pointer readDataCodecOutput(
        const QString& path,
        QString* error,
        std::shared_ptr<::datacodec::IRunRecordSink> telemetrySink) {
    auto reader = iGame::IGDCReader::New();
    auto options = iGame::DataCodecIOSettings::GetDefaultDecodeOptions();
    options.logging.enableFileLog = false;
    options.logging.enableConsoleLog = false;
    reader->SetDecodeOptions(options);
    reader->SetTelemetrySink(std::move(telemetrySink));
    reader->SetFilePath(toUtf8StdString(path));
    if (!reader->Execute()) {
        QStringList messages;
        for (const auto& message : reader->GetMessages()) {
            if (!message.text.empty()) messages.push_back(QString::fromStdString(message.text));
        }
        if (error != nullptr) {
            *error = messages.isEmpty() ? QStringLiteral("回读输出文件失败") : messages.join(QStringLiteral("; "));
        }
        return nullptr;
    }

    auto output = reader->GetOutput();
    if (output == nullptr && error != nullptr) {
        *error = QStringLiteral("回读输出对象为空");
    }
    return output;
}

QString formatPrecisionMetric(double value) {
    if (!std::isfinite(value)) return QStringLiteral("无法计算");
    return QString::number(value, 'g', 8);
}

void configurePrecisionSpin(QDoubleSpinBox* spin) {
    spin->setRange(0.0, 1.0e12);
    spin->setDecimals(8);
    spin->setSingleStep(0.0001);
    spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spin->setAccelerated(true);
}

QString precisionSpinToneStyle(const QColor& color) {
    return QStringLiteral(R"(
QDoubleSpinBox,
QDoubleSpinBox QLineEdit {
    color: %1;
}
QDoubleSpinBox:disabled,
QDoubleSpinBox QLineEdit:disabled {
    color: #7d7d7d;
}
)").arg(color.name());
}

void configureComboPopup(QComboBox* combo) {
    if (combo == nullptr) return;
    combo->setItemDelegate(new DataCodecComboItemDelegate(combo));
    combo->setMaxVisibleItems(8);
    if (combo->view() != nullptr) {
        combo->view()->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        combo->view()->setStyleSheet(QStringLiteral(R"(
QAbstractItemView::item {
    min-height: 36px;
    padding: 8px 10px;
}
)"));
    }
}

void clearLayout(QLayout* layout) {
    if (!layout) return;
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) widget->deleteLater();
        if (QLayout* childLayout = item->layout()) clearLayout(childLayout);
        delete item;
    }
}

double valueAtPercent(double minValue, double maxValue, int percent) {
    const double t = static_cast<double>(qBound(0, percent, 100)) / 100.0;
    return minValue + (maxValue - minValue) * t;
}

int histogramBinIndexForValue(double value, double minValue, double maxValue, int binCount) {
    if (binCount <= 1 || minValue == maxValue) return qMax(0, binCount / 2);
    const double range = maxValue - minValue;
    int bin = static_cast<int>((value - minValue) / range * static_cast<double>(binCount));
    return qBound(0, bin, binCount - 1);
}

QVector<double> buildNormalizedHistogram(const std::vector<double>& values, double minValue, double maxValue,
                                         int binCount) {
    QVector<double> bins(qMax(1, binCount), 0.0);
    if (values.empty()) return bins;
    if (minValue == maxValue) {
        bins[bins.size() / 2] = 1.0;
        return bins;
    }

    const double range = maxValue - minValue;
    for (double value : values) {
        if (!std::isfinite(value)) continue;
        int bin = static_cast<int>((value - minValue) / range * static_cast<double>(bins.size()));
        bin = qBound(0, bin, bins.size() - 1);
        bins[bin] += 1.0;
    }

    double maxDensity = 0.0;
    for (double density : bins) maxDensity = qMax(maxDensity, density);
    if (maxDensity <= std::numeric_limits<double>::epsilon()) return bins;
    for (double& density : bins) density /= maxDensity;
    return bins;
}
} // 匿名命名空间

igQtDataCodecHistogramWidget::igQtDataCodecHistogramWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(140);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void igQtDataCodecHistogramWidget::setRange(int lowerPercent, int upperPercent) {
    m_lowerPercent = qBound(0, qMin(lowerPercent, upperPercent), 100);
    m_upperPercent = qBound(0, qMax(lowerPercent, upperPercent), 100);
    update();
}

void igQtDataCodecHistogramWidget::setHistogramView(const QVector<double>& bins, int backgroundCount, int totalCount) {
    m_bins = bins;
    m_backgroundCount = qMax(0, backgroundCount);
    m_totalCount = qMax(0, totalCount);
    update();
}

void igQtDataCodecHistogramWidget::setOccupiedBins(const QVector<double>& bins) {
    m_occupiedBins = bins;
    update();
}

void igQtDataCodecHistogramWidget::clearBins() {
    m_bins.clear();
    m_occupiedBins.clear();
    m_backgroundCount = 0;
    m_totalCount = 0;
    update();
}

void igQtDataCodecHistogramWidget::setEnabledVisual(bool enabled) {
    m_enabledVisual = enabled;
    update();
}

void igQtDataCodecHistogramWidget::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode) return;
    m_darkMode = darkMode;
    update();
}

void igQtDataCodecHistogramWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF frame = rect().adjusted(1, 1, -1, -1);
    painter.setPen(m_darkMode ? QColor(60, 60, 64) : QColor(214, 210, 200));
    painter.setBrush(m_darkMode ? QColor(30, 30, 30) : QColor(255, 253, 248));
    painter.drawRoundedRect(frame, 6, 6);

    const bool hasFoldedBackground = m_totalCount > 0 && m_backgroundCount > 0;
    const int leftInset = hasFoldedBackground ? kSmartHistogramLeftInset : kHistogramEdgeInset;
    const QRect barRect = rect().adjusted(leftInset, 14, -kHistogramEdgeInset, -16);
    if (barRect.width() <= 0 || barRect.height() <= 0) return;
    if (m_bins.isEmpty()) return;

    if (hasFoldedBackground) {
        const QRect backgroundRect(16, 14, qMax(20, kSmartHistogramLeftInset - 28), qMax(1, height() - 30));
        const double ratio = static_cast<double>(m_backgroundCount) / static_cast<double>(m_totalCount);
        const QColor backgroundFill = m_darkMode ? QColor(206, 155, 62, 180) : QColor(201, 138, 36, 180);
        const int backgroundHeight = qBound(4, static_cast<int>(backgroundRect.height() * ratio), backgroundRect.height());
        painter.setPen(Qt::NoPen);
        painter.setBrush(backgroundFill);
        painter.drawRect(QRect(backgroundRect.left() + 12, backgroundRect.bottom() - backgroundHeight,
                               qMax(8, backgroundRect.width() - 24), backgroundHeight));
        painter.setPen(m_darkMode ? QColor(255, 221, 139) : QColor(116, 83, 22));
        painter.setFont(QFont(painter.font().family(), 8, QFont::Bold));
        painter.drawText(backgroundRect.adjusted(2, 0, -2, 0), Qt::AlignHCenter | Qt::AlignTop,
                         QStringLiteral("背景数值\n占比%1%").arg(qRound(ratio * 100.0)));
        painter.setPen(QPen(m_darkMode ? QColor(75, 82, 84) : QColor(205, 205, 198), 1));
        painter.drawLine(kSmartHistogramLeftInset - 10, barRect.top(), kSmartHistogramLeftInset - 10, barRect.bottom());
    }

    const int binCount = m_bins.size();
    const QColor active = m_enabledVisual ? (m_darkMode ? QColor(38, 162, 156) : QColor(0, 127, 121))
                                          : (m_darkMode ? QColor(93, 105, 102) : QColor(150, 164, 158));
    const QColor inactive = m_enabledVisual ? (m_darkMode ? QColor(70, 76, 74) : QColor(203, 213, 207))
                                            : (m_darkMode ? QColor(64, 66, 66) : QColor(224, 224, 220));
    const QColor occupied = m_enabledVisual ? (m_darkMode ? QColor(126, 102, 53) : QColor(180, 136, 58))
                                            : (m_darkMode ? QColor(82, 76, 62) : QColor(202, 184, 140));
    const QColor overlap = m_darkMode ? QColor(238, 91, 91) : QColor(198, 50, 50);
    for (int i = 0; i < binCount; ++i) {
        const double density = qBound(0.0, m_bins[i], 1.0);
        const int h = qBound(3, static_cast<int>(barRect.height() * density), barRect.height());
        const int x = barRect.left() + i * barRect.width() / binCount;
        const int nextX = barRect.left() + (i + 1) * barRect.width() / binCount;
        const double binLower = static_cast<double>(i) * 100.0 / static_cast<double>(binCount);
        const double binUpper = static_cast<double>(i + 1) * 100.0 / static_cast<double>(binCount);
        const bool selected = binUpper >= static_cast<double>(m_lowerPercent) &&
            binLower <= static_cast<double>(m_upperPercent);
        const double occupiedState = i < m_occupiedBins.size() ? m_occupiedBins[i] : 0.0;
        const bool occupiedBin = occupiedState >= 1.0;
        const bool overlapBin = occupiedState >= 2.0;
        QColor fill = inactive;
        if (overlapBin) {
            fill = overlap;
        } else if (selected) {
            fill = active;
        } else if (occupiedBin) {
            fill = occupied;
        }
        painter.setBrush(fill);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(x + 1, barRect.bottom() - h, qMax(2, nextX - x - 2), h), 2, 2);
    }
}

igQtDataCodecRangeSliderWidget::igQtDataCodecRangeSliderWidget(QWidget* parent) : QWidget(parent) {
    setFixedHeight(32);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
}

void igQtDataCodecRangeSliderWidget::setRangeValues(int lowerValue, int upperValue) {
    const int nextLower = qBound(0, qMin(lowerValue, upperValue), 100);
    const int nextUpper = qBound(0, qMax(lowerValue, upperValue), 100);
    if (m_lowerValue == nextLower && m_upperValue == nextUpper) return;
    m_lowerValue = nextLower;
    m_upperValue = nextUpper;
    update();
    emit rangeChanged();
}

void igQtDataCodecRangeSliderWidget::setSliderEnabled(bool enabled) {
    m_enabled = enabled;
    setEnabled(enabled);
    update();
}

void igQtDataCodecRangeSliderWidget::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode) return;
    m_darkMode = darkMode;
    update();
}

void igQtDataCodecRangeSliderWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect track = trackRect();
    const int centerY = track.center().y();
    const int leftHandleX = positionForValue(m_lowerValue);
    const int rightHandleX = positionForValue(m_upperValue);
    const QColor baseColor = m_enabled ? (m_darkMode ? QColor(66, 74, 94) : QColor(50, 57, 78))
                                       : (m_darkMode ? QColor(76, 76, 76) : QColor(190, 190, 186));
    const QColor activeColor = m_enabled ? (m_darkMode ? QColor(38, 162, 156) : QColor(0, 127, 121))
                                         : (m_darkMode ? QColor(92, 98, 96) : QColor(160, 164, 160));
    const QColor handleColor = m_enabled ? (m_darkMode ? QColor(174, 184, 180) : QColor(154, 165, 160))
                                         : (m_darkMode ? QColor(112, 116, 114) : QColor(186, 188, 184));

    painter.setPen(QPen(baseColor, 5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(track.left(), centerY, track.right(), centerY);
    painter.setPen(QPen(activeColor, 5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(leftHandleX, centerY, rightHandleX, centerY);
    painter.setPen(QPen(m_darkMode ? QColor(30, 30, 30) : QColor(245, 241, 232), 2));
    painter.setBrush(handleColor);
    painter.drawEllipse(QPoint(leftHandleX, centerY), 7, 7);
    painter.drawEllipse(QPoint(rightHandleX, centerY), 7, 7);
}

void igQtDataCodecRangeSliderWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_enabled || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_dragTarget = nearestHandle(event->pos());
    mouseMoveEvent(event);
}

void igQtDataCodecRangeSliderWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_enabled || m_dragTarget == DragTarget::None) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const int nextValue = valueForPosition(event->pos().x());
    if (m_dragTarget == DragTarget::Lower) {
        setRangeValues(qMin(nextValue, m_upperValue), m_upperValue);
    } else if (m_dragTarget == DragTarget::Upper) {
        setRangeValues(m_lowerValue, qMax(nextValue, m_lowerValue));
    }
}

void igQtDataCodecRangeSliderWidget::mouseReleaseEvent(QMouseEvent* event) {
    const bool wasDragging = m_dragTarget != DragTarget::None;
    m_dragTarget = DragTarget::None;
    if (wasDragging) emit rangeEditingFinished();
    QWidget::mouseReleaseEvent(event);
}

QRect igQtDataCodecRangeSliderWidget::trackRect() const {
    return rect().adjusted(m_leftInset, 0, -m_rightInset, 0);
}

void igQtDataCodecRangeSliderWidget::setTrackInsets(int leftInset, int rightInset) {
    const int nextLeft = qMax(0, leftInset);
    const int nextRight = qMax(0, rightInset);
    if (m_leftInset == nextLeft && m_rightInset == nextRight) return;
    m_leftInset = nextLeft;
    m_rightInset = nextRight;
    update();
}

int igQtDataCodecRangeSliderWidget::positionForValue(int value) const {
    const QRect track = trackRect();
    return track.left() + track.width() * qBound(0, value, 100) / 100;
}

int igQtDataCodecRangeSliderWidget::valueForPosition(int x) const {
    const QRect track = trackRect();
    if (track.width() <= 0) return 0;
    return qBound(0, (x - track.left()) * 100 / track.width(), 100);
}

igQtDataCodecRangeSliderWidget::DragTarget igQtDataCodecRangeSliderWidget::nearestHandle(const QPoint& pos) const {
    const int lowerDistance = std::abs(pos.x() - positionForValue(m_lowerValue));
    const int upperDistance = std::abs(pos.x() - positionForValue(m_upperValue));
    return lowerDistance <= upperDistance ? DragTarget::Lower : DragTarget::Upper;
}

igQtDataCodecCompressionWidget::FieldState::FieldState() : features(3) {}

igQtDataCodecCompressionWidget::igQtDataCodecCompressionWidget(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("DataCodecCompressionWidget"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(940, 840);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* body = new QWidget(this);
    auto* bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(14, 14, 14, 14);
    bodyLayout->setSpacing(14);

    auto* leftColumn = new QWidget(body);
    leftColumn->setFixedWidth(300);
    auto* leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);
    leftLayout->addWidget(createOutputPanel());
    leftLayout->addWidget(createFieldPanel(), 1);

    bodyLayout->addWidget(leftColumn, 0);
    bodyLayout->addWidget(createBasisPanel(), 1);

    root->addWidget(body, 1);
    root->addWidget(createStatusPanel(), 0);
    if (m_compressionEnhancementCheck != nullptr &&
        m_compressionEnhancementCheck->isChecked()) {
        publishStatus(dataCodecHostLogText(
            m_dataCodecLanguage,
            iGame::iGameDataCodecHostMessageId::CompressionEnhancementEnabled));
    }

    applyStyle();
    refreshFields();
    resetRegionState();
    refreshFeatureState();
}

void igQtDataCodecCompressionWidget::SetModel(iGame::Model::Pointer model) {
    ++m_featureRequestSerial;
    m_featureComputationActive = false;
    if (m_computeButton != nullptr) m_computeButton->setText(QStringLiteral("计算特征"));
    m_model = model;
    refreshFields();
    resetRegionState();
    refreshFeatureState();
    appendPredictionEncodingRecommendation();
}

bool igQtDataCodecCompressionWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_outputPathEdit && event != nullptr && event->type() == QEvent::Wheel) {
        auto* wheel = static_cast<QWheelEvent*>(event);
        const QPoint delta = !wheel->angleDelta().isNull() ? wheel->angleDelta() : wheel->pixelDelta();
        const int wheelDelta = delta.y() != 0 ? delta.y() : delta.x();
        if (wheelDelta != 0 && m_outputPathEdit->hasFocus()) {
            const int step = qMax(1, qAbs(wheelDelta) / 120) * 8;
            const int cursor = m_outputPathEdit->cursorPosition();
            const int textLength = m_outputPathEdit->text().size();
            const int direction = wheelDelta > 0 ? -1 : 1;
            m_outputPathEdit->setCursorPosition(qBound(0, cursor + direction * step, textLength));
            wheel->accept();
            return true;
        }
    }
    if (event != nullptr && event->type() == QEvent::MouseButtonPress) {
        bool ok = false;
        const int regionId = watched != nullptr ? watched->property("DataCodecRegionId").toInt(&ok) : 0;
        if (ok) {
            selectRegion(regionId);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

QWidget* igQtDataCodecCompressionWidget::createOutputPanel() {
    auto* panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("DataCodecPanel"));
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("输出"), panel);
    title->setObjectName(QStringLiteral("DataCodecSectionTitle"));
    titleRow->addWidget(title);
    titleRow->addStretch();
    layout->addLayout(titleRow);

    auto* outputLabel = new QLabel(QStringLiteral("输出文件"), panel);
    outputLabel->setObjectName(QStringLiteral("DataCodecLabel"));
    layout->addWidget(outputLabel);

    auto* pathRow = new QHBoxLayout;
    pathRow->setSpacing(6);
    m_outputPathEdit = new QLineEdit(panel);
    m_outputPathEdit->setReadOnly(true);
    m_outputPathEdit->setFocusPolicy(Qt::StrongFocus);
    m_outputPathEdit->installEventFilter(this);
    m_outputPathButton = new QPushButton(QStringLiteral("..."), panel);
    m_outputPathButton->setFixedWidth(34);
    pathRow->addWidget(m_outputPathEdit, 1);
    pathRow->addWidget(m_outputPathButton, 0);
    layout->addLayout(pathRow);

    auto* tierRow = new QHBoxLayout;
    auto* tierLabel = new QLabel(QStringLiteral("资源模式"), panel);
    tierLabel->setObjectName(QStringLiteral("DataCodecLabelStrong"));
    m_performanceCombo = new QComboBox(panel);
    m_performanceCombo->addItem(
        QStringLiteral("快速"),
        static_cast<int>(::datacodec::DataCodecEncodeTier::TimePriority));
    m_performanceCombo->addItem(
        QStringLiteral("标准"),
        static_cast<int>(::datacodec::DataCodecEncodeTier::Balanced));
    m_performanceCombo->addItem(
        QStringLiteral("低内存"),
        static_cast<int>(::datacodec::DataCodecEncodeTier::MemoryPriority));
    m_performanceCombo->setToolTip(
        QStringLiteral("设置编码速度、内存占用及默认 ZSTD压缩等级"));
    configureComboPopup(m_performanceCombo);
    tierRow->addWidget(tierLabel, 1);
    tierRow->addWidget(m_performanceCombo, 0);
    layout->addLayout(tierRow);

    m_compressionEnhancementCheck = new QCheckBox(QStringLiteral("压缩率增强"), panel);
    m_compressionEnhancementCheck->setToolTip(
        QStringLiteral("启用单元重排与扩展预测搜索以提高压缩率，不改变 ZSTD压缩等级与资源模式"));
    layout->addWidget(m_compressionEnhancementCheck);

    auto* zstdRow = new QHBoxLayout;
    auto* zstdLabel = new QLabel(QStringLiteral("ZSTD压缩等级"), panel);
    zstdLabel->setObjectName(QStringLiteral("DataCodecLabelStrong"));
    m_zstdLevelSpin = new QSpinBox(panel);
    m_zstdLevelSpin->setRange(1, 22);
    zstdRow->addWidget(zstdLabel, 1);
    zstdRow->addWidget(m_zstdLevelSpin, 0);
    layout->addLayout(zstdRow);

    m_gopControlRow = new QWidget(panel);
    auto* gopRow = new QHBoxLayout(m_gopControlRow);
    gopRow->setContentsMargins(0, 0, 0, 0);
    auto* gopLabel = new QLabel(QStringLiteral("关键帧间隔"), m_gopControlRow);
    gopLabel->setObjectName(QStringLiteral("DataCodecLabelStrong"));
    m_gopFrameCountSpin = new QSpinBox(m_gopControlRow);
    m_gopFrameCountSpin->setMinimum(1);
    m_gopFrameCountSpin->setToolTip(
        QStringLiteral("每隔指定数量的帧写入一个关键帧"));
    gopRow->addWidget(gopLabel, 1);
    gopRow->addWidget(m_gopFrameCountSpin, 0);
    layout->addWidget(m_gopControlRow);

    auto* batchLabel = new QLabel(QStringLiteral("批量设置"), panel);
    batchLabel->setObjectName(QStringLiteral("DataCodecLabelStrong"));
    layout->addWidget(batchLabel);

    m_applyLosslessAllButton = new QPushButton(QStringLiteral("全部无损"), panel);
    m_applyLossyAllButton = new QPushButton(QStringLiteral("全部有损"), panel);
    m_applyLossyAllButton->setToolTip(QStringLiteral("将全部数据的相对误差限设为 0.00001"));
    m_syncDefaultPrecisionButton = new QPushButton(QStringLiteral("应用默认误差限"), panel);
    auto* batchModeRow = new QHBoxLayout;
    batchModeRow->setSpacing(6);
    batchModeRow->addWidget(m_applyLosslessAllButton);
    batchModeRow->addWidget(m_applyLossyAllButton);
    layout->addLayout(batchModeRow);
    layout->addWidget(m_syncDefaultPrecisionButton);
    layout->addSpacing(2);

    auto* predictionLabel = new QLabel(QStringLiteral("预测编码"), panel);
    predictionLabel->setObjectName(QStringLiteral("DataCodecLabelStrong"));
    layout->addWidget(predictionLabel);

    m_intraAttributePredictionCheck = new QCheckBox(QStringLiteral("帧内数据预测编码"), panel);
    m_temporalAttributePredictionCheck = new QCheckBox(QStringLiteral("帧间数据预测编码"), panel);
    m_intraAttributePredictionCheck->setChecked(true);
    m_temporalAttributePredictionCheck->setChecked(true);
    layout->addWidget(m_intraAttributePredictionCheck);
    layout->addWidget(m_temporalAttributePredictionCheck);

    connect(m_outputPathButton, &QPushButton::clicked, this, &igQtDataCodecCompressionWidget::chooseOutputPath);
    connect(m_outputPathEdit, &QLineEdit::textChanged, this, &igQtDataCodecCompressionWidget::refreshFeatureState);
    connect(m_applyLosslessAllButton, &QPushButton::clicked, this,
            &igQtDataCodecCompressionWidget::applyLosslessModeToAllFields);
    connect(m_applyLossyAllButton, &QPushButton::clicked, this,
            &igQtDataCodecCompressionWidget::applyHighPrecisionLossyModeToAllFields);
    connect(m_syncDefaultPrecisionButton, &QPushButton::clicked, this,
            &igQtDataCodecCompressionWidget::syncCurrentDefaultPrecisionToAllFields);
    connect(m_intraAttributePredictionCheck, &QCheckBox::toggled, this,
            [this](const bool checked) {
                if (m_intraAttributePredictionCheck != nullptr &&
                    m_intraAttributePredictionCheck->isEnabled()) {
                    m_intraAttributePredictionPreferred = checked;
                    if (checked) appendPredictionEncodingRecommendation();
                }
            });
    connect(m_temporalAttributePredictionCheck, &QCheckBox::toggled, this,
            [this](const bool checked) {
                if (m_temporalAttributePredictionCheck != nullptr &&
                    m_temporalAttributePredictionCheck->isEnabled()) {
                    m_temporalAttributePredictionPreferred = checked;
                    if (checked) appendPredictionEncodingRecommendation();
                }
            });

    const auto performanceSettings = LoadDataCodecCompressionPerformanceSettings();
    const auto tierIndex = m_performanceCombo->findData(
        static_cast<int>(performanceSettings.tier));
    const auto defaultTierIndex = m_performanceCombo->findData(
        static_cast<int>(kDefaultEncodeTier));
    m_performanceCombo->setCurrentIndex(tierIndex >= 0 ? tierIndex : defaultTierIndex);
    m_compressionEnhancementCheck->setChecked(performanceSettings.compressionEnhancement);
    m_zstdLevelSpin->setValue(performanceSettings.zstdLevel);
    m_gopFrameCountSpin->setValue(performanceSettings.gopFrameCount);
    connect(m_performanceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this](const int) { applyPerformanceTierDefaults(); });
    connect(m_compressionEnhancementCheck, &QCheckBox::toggled, this,
        [this](const bool checked) {
            persistPerformanceSettings();
            if (checked) {
                publishStatus(dataCodecHostLogText(
                    m_dataCodecLanguage,
                    iGame::iGameDataCodecHostMessageId::CompressionEnhancementEnabled));
            }
        });
    connect(m_zstdLevelSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
        [this](const int) { persistPerformanceSettings(); });
    connect(m_gopFrameCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
        [this](const int) { persistPerformanceSettings(); });
    return panel;
}

QWidget* igQtDataCodecCompressionWidget::createFieldPanel() {
    auto* panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("DataCodecPanel"));
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("数据"), panel);
    title->setObjectName(QStringLiteral("DataCodecSectionTitle"));
    m_selectAllFieldsCheck = new QCheckBox(QStringLiteral("全选"), panel);
    m_selectAllFieldsCheck->setTristate(true);
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(m_selectAllFieldsCheck);
    layout->addLayout(titleRow);

    m_fieldList = new QListWidget(panel);
    m_fieldList->setObjectName(QStringLiteral("DataCodecFieldList"));
    m_fieldList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fieldList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fieldList->setTextElideMode(Qt::ElideNone);
    m_fieldList->setWordWrap(true);
    layout->addWidget(m_fieldList, 1);

    connect(m_fieldList, &QListWidget::currentRowChanged, this, &igQtDataCodecCompressionWidget::onFieldChanged);
    connect(m_fieldList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        if (item == nullptr || m_fieldList == nullptr) return;
        const int row = m_fieldList->row(item);
        ensureFieldStates();
        if (row < 0 || row >= m_fieldStates.size()) return;
        m_fieldStates[row].selected = item->checkState() == Qt::Checked;
        refreshSelectAllState();
        refreshSelectedRegion();
        refreshFieldItemLabels();
        refreshPredictionControls();
    });
    connect(m_selectAllFieldsCheck, &QCheckBox::stateChanged, this, [this](int state) {
        if (state == Qt::PartiallyChecked || m_fieldList == nullptr) return;
        ensureFieldStates();
        const bool selected = state == Qt::Checked;
        QSignalBlocker listBlocker(m_fieldList);
        for (int index = 0; index < m_fieldStates.size(); ++index) {
            m_fieldStates[index].selected = selected;
            if (auto* item = m_fieldList->item(index); item != nullptr) {
                item->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
            }
        }
        refreshSelectedRegion();
        refreshFieldItemLabels();
        refreshPredictionControls();
    });
    return panel;
}

QWidget* igQtDataCodecCompressionWidget::createBasisPanel() {
    auto* scope = new QFrame(this);
    scope->setObjectName(QStringLiteral("DataCodecBasisScope"));
    auto* layout = new QVBoxLayout(scope);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    auto* basisRow = new QHBoxLayout;
    auto* basisTitle = new QLabel(QStringLiteral("●  特征"), scope);
    basisTitle->setObjectName(QStringLiteral("DataCodecBasisTitle"));
    m_basisStateLabel = new QLabel(QStringLiteral("待计算"), scope);
    m_basisStateLabel->setObjectName(QStringLiteral("DataCodecTag"));
    basisRow->addWidget(basisTitle);
    basisRow->addWidget(m_basisStateLabel);
    basisRow->addStretch();
    layout->addLayout(basisRow);

    auto* basisControlRow = new QHBoxLayout;
    basisControlRow->setSpacing(8);
    m_basisCombo = new QComboBox(scope);
    m_basisCombo->addItems({QStringLiteral("模长"), QStringLiteral("相邻值标准差"), QStringLiteral("相邻最大差值")});
    configureComboPopup(m_basisCombo);
    m_computeButton = new QPushButton(QStringLiteral("计算特征"), scope);
    m_computeButton->setObjectName(QStringLiteral("DataCodecPrimaryButton"));
    basisControlRow->addWidget(m_basisCombo, 1);
    basisControlRow->addWidget(m_computeButton, 0);
    layout->addLayout(basisControlRow);

    layout->addWidget(createRegionPanel(), 0);
    layout->addWidget(createRegionBuilderPanel(), 1);

    connect(m_basisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtDataCodecCompressionWidget::onBasisChanged);
    connect(m_computeButton, &QPushButton::clicked, this, &igQtDataCodecCompressionWidget::computeFeature);
    return scope;
}

QWidget* igQtDataCodecCompressionWidget::createRegionPanel() {
    auto* panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("DataCodecInnerPanel"));
    panel->setMinimumHeight(300);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("选择区域"), panel);
    title->setObjectName(QStringLiteral("DataCodecSectionTitle"));
    m_addRegionButton = new QPushButton(QStringLiteral("+"), panel);
    m_addRegionButton->setFixedSize(30, 28);
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(m_addRegionButton);
    layout->addLayout(titleRow);

    m_regionScrollArea = new QScrollArea(panel);
    m_regionScrollArea->setObjectName(QStringLiteral("DataCodecRegionScroll"));
    m_regionScrollArea->setMinimumHeight(222);
    m_regionScrollArea->setWidgetResizable(true);
    m_regionScrollArea->setFrameShape(QFrame::NoFrame);
    m_regionScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_regionScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_regionScrollArea->setAutoFillBackground(false);
    m_regionScrollArea->setStyleSheet(QStringLiteral(R"(
QScrollArea#DataCodecRegionScroll {
    background: #252526;
    border: none;
}
QScrollArea#DataCodecRegionScroll QScrollBar:vertical {
    background: #1b1b1b;
    border: none;
    width: 12px;
    margin: 0;
}
QScrollArea#DataCodecRegionScroll QScrollBar::handle:vertical {
    background: #8a8a8a;
    border-radius: 6px;
    min-height: 24px;
}
QScrollArea#DataCodecRegionScroll QScrollBar::add-line:vertical,
QScrollArea#DataCodecRegionScroll QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollArea#DataCodecRegionScroll QScrollBar::add-page:vertical,
QScrollArea#DataCodecRegionScroll QScrollBar::sub-page:vertical {
    background: #252526;
}
)"));
    if (m_regionScrollArea->viewport()) {
        m_regionScrollArea->viewport()->setObjectName(QStringLiteral("DataCodecRegionViewport"));
        m_regionScrollArea->viewport()->setAutoFillBackground(false);
        m_regionScrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, true);
        m_regionScrollArea->viewport()->setStyleSheet(
                QStringLiteral("QWidget#DataCodecRegionViewport { background: #252526; border: none; }"));
    }
    m_regionRowsHost = new QWidget(m_regionScrollArea);
    m_regionRowsHost->setObjectName(QStringLiteral("DataCodecRegionRowsHost"));
    m_regionRowsHost->setAutoFillBackground(false);
    m_regionRowsHost->setAttribute(Qt::WA_StyledBackground, true);
    m_regionRowsHost->setStyleSheet(QStringLiteral("QWidget#DataCodecRegionRowsHost { background: #252526; }"));
    m_regionRowsLayout = new QVBoxLayout(m_regionRowsHost);
    m_regionRowsLayout->setContentsMargins(0, 0, 0, 0);
    m_regionRowsLayout->setSpacing(7);
    m_regionRowsLayout->addStretch();
    m_regionScrollArea->setWidget(m_regionRowsHost);
    layout->addWidget(m_regionScrollArea, 1);

    connect(m_addRegionButton, &QPushButton::clicked, this, &igQtDataCodecCompressionWidget::addRegion);
    return panel;
}

QWidget* igQtDataCodecCompressionWidget::createRegionBuilderPanel() {
    auto* panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("DataCodecInnerPanel"));
    panel->setMinimumHeight(260);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("区域误差限设置"), panel);
    title->setObjectName(QStringLiteral("DataCodecSectionTitle"));
    m_regionBuilderTitle = new QLabel(panel);
    m_regionBuilderTitle->setObjectName(QStringLiteral("DataCodecPlainTag"));
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(m_regionBuilderTitle);
    layout->addLayout(titleRow);

    auto* contentGrid = new QGridLayout;
    contentGrid->setColumnStretch(0, 3);
    contentGrid->setColumnStretch(1, 1);
    contentGrid->setHorizontalSpacing(10);
    contentGrid->setVerticalSpacing(7);

    auto* histLabel = new QLabel(QStringLiteral("特征直方图区间"), panel);
    histLabel->setObjectName(QStringLiteral("DataCodecLabel"));
    contentGrid->addWidget(histLabel, 0, 0);
    m_histogram = new igQtDataCodecHistogramWidget(panel);
    contentGrid->addWidget(m_histogram, 1, 0, 1, 1);

    m_rangeSlider = new igQtDataCodecRangeSliderWidget(panel);
    m_rangeSlider->setRangeValues(76, 100);
    contentGrid->addWidget(m_rangeSlider, 2, 0);

    auto* histogramLegend = new QLabel(panel);
    histogramLegend->setObjectName(QStringLiteral("DataCodecLabel"));
    histogramLegend->setTextFormat(Qt::RichText);
    histogramLegend->setText(QStringLiteral(
        "<span style='color:#26a29c'>■</span> 当前范围　"
        "<span style='color:#b4883a'>■</span> 已有区域　"
        "<span style='color:#ee5b5b'>■</span> 重叠范围"));
    contentGrid->addWidget(histogramLegend, 3, 0);

    auto* precisionPanel = new QWidget(panel);
    auto* precisionLayout = new QVBoxLayout(precisionPanel);
    precisionLayout->setContentsMargins(0, 0, 0, 0);
    precisionLayout->setSpacing(7);

    m_losslessModeCheck = new QCheckBox(QStringLiteral("无损压缩"), panel);
    precisionLayout->addWidget(m_losslessModeCheck);
    precisionLayout->addSpacing(3);

    m_precisionModeCombo = new QComboBox(panel);
    configureComboPopup(m_precisionModeCombo);
    configurePrecisionModeCombo(m_precisionModeCombo, kPrecisionModeAbs);
    precisionLayout->addWidget(m_precisionModeCombo);

    m_precisionSpin = new QDoubleSpinBox(panel);
    configurePrecisionSpin(m_precisionSpin);
    m_precisionSpin->setValue(0.004);
    precisionLayout->addWidget(m_precisionSpin);
    precisionLayout->addSpacing(10);
    precisionLayout->addWidget(createStatBox(m_selectedTotalLabel, QStringLiteral("选中数据项数 / 数据项总数")));
    precisionLayout->addWidget(createStatBox(m_overlapTotalLabel, QStringLiteral("重叠数据项数")));
    precisionLayout->addStretch();
    contentGrid->addWidget(precisionPanel, 0, 1, 4, 1);
    layout->addLayout(contentGrid);

    connect(m_rangeSlider, &igQtDataCodecRangeSliderWidget::rangeChanged, this,
            &igQtDataCodecCompressionWidget::onRangeChanged);
    connect(m_rangeSlider, &igQtDataCodecRangeSliderWidget::rangeEditingFinished, this,
            &igQtDataCodecCompressionWidget::commitRangeSelection);
    connect(m_precisionModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtDataCodecCompressionWidget::syncSelectedPrecisionMode);
    connect(m_precisionSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &igQtDataCodecCompressionWidget::syncSelectedPrecision);
    connect(m_losslessModeCheck, &QCheckBox::toggled, this,
            &igQtDataCodecCompressionWidget::applyLosslessMode);
    return panel;
}

QWidget* igQtDataCodecCompressionWidget::createStatusPanel() {
    auto* panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("DataCodecStatusPanel"));
    panel->setMinimumHeight(168);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(14, 8, 14, 10);
    layout->setSpacing(6);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("运行状态"), panel);
    title->setObjectName(QStringLiteral("DataCodecSectionTitle"));
    m_emitPerformanceCheck = new QCheckBox(QStringLiteral("生成性能报告"), panel);
    m_startEncodeButton = new QPushButton(QStringLiteral("开始压缩"), panel);
    m_startEncodeButton->setObjectName(QStringLiteral("DataCodecDarkButton"));
    header->addWidget(title);
    header->addStretch();
    header->addWidget(m_emitPerformanceCheck);
    header->addWidget(m_startEncodeButton);
    layout->addLayout(header);

    m_logEdit = new QPlainTextEdit(panel);
    m_logEdit->setObjectName(QStringLiteral("DataCodecLog"));
    m_logEdit->setReadOnly(true);
    m_logEdit->setMinimumHeight(104);
    m_logEdit->setMaximumHeight(132);
    layout->addWidget(m_logEdit);

    connect(m_startEncodeButton, &QPushButton::clicked, this, &igQtDataCodecCompressionWidget::startEncode);
    return panel;
}

QWidget* igQtDataCodecCompressionWidget::createStatBox(QLabel*& valueLabel, const QString& title) {
    auto* box = new QFrame(this);
    box->setObjectName(QStringLiteral("DataCodecStatBox"));
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(2);
    auto* label = new QLabel(title, box);
    label->setObjectName(QStringLiteral("DataCodecLabel"));
    valueLabel = new QLabel(QStringLiteral("0"), box);
    valueLabel->setObjectName(QStringLiteral("DataCodecStatValue"));
    layout->addWidget(label);
    layout->addWidget(valueLabel);
    return box;
}

QWidget* igQtDataCodecCompressionWidget::createRegionRow(const RegionItem& region, bool invalidRegion) {
    auto* row = new QFrame(this);
    const bool multiFrame = hasMultiFrameData();
    const bool regionEditingAllowed = !multiFrame || region.id == 0;
    row->setObjectName(QStringLiteral("DataCodecRegionRow"));
    const auto* featureState = currentFeatureState();
    row->setProperty("selected", featureState != nullptr && region.id == featureState->selectedRegionId);
    row->setProperty("base", region.id == 0);
    row->setProperty("invalid", invalidRegion);
    row->setProperty("DataCodecRegionId", region.id);
    row->setMinimumHeight(52);
    row->setCursor(regionEditingAllowed ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (regionEditingAllowed) row->installEventFilter(this);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(9, 7, 8, 7);
    layout->setSpacing(8);

    auto* nameBlock = new QWidget(row);
    nameBlock->setProperty("DataCodecRegionId", region.id);
    nameBlock->setCursor(regionEditingAllowed ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (regionEditingAllowed) nameBlock->installEventFilter(this);
    auto* nameLayout = new QVBoxLayout(nameBlock);
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(1);
    auto* nameLine = new QHBoxLayout;
    nameLine->setContentsMargins(0, 0, 0, 0);
    nameLine->setSpacing(6);
    auto* name = new QLabel(region.name, nameBlock);
    name->setObjectName(QStringLiteral("DataCodecRegionName"));
    name->setProperty("DataCodecRegionId", region.id);
    name->setCursor(regionEditingAllowed ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (regionEditingAllowed) name->installEventFilter(this);
    nameLine->addWidget(name, 0);
    if (invalidRegion) {
        auto* warning = new QLabel(QStringLiteral("!"), nameBlock);
        warning->setObjectName(QStringLiteral("DataCodecRegionWarning"));
        warning->setToolTip(QStringLiteral("该区域与其他自定义区域重叠，请调整范围后再编码"));
        warning->setProperty("DataCodecRegionId", region.id);
        warning->setCursor(regionEditingAllowed ? Qt::PointingHandCursor : Qt::ArrowCursor);
        if (regionEditingAllowed) warning->installEventFilter(this);
        nameLine->addWidget(warning, 0);
    }
    nameLine->addStretch();
    nameLayout->addLayout(nameLine);
    if (region.id == 0) {
        auto* description = new QLabel(QStringLiteral("默认设置 · 用于未被自定义区域覆盖的数据"), nameBlock);
        description->setObjectName(QStringLiteral("DataCodecRegionMeta"));
        description->setProperty("DataCodecRegionId", region.id);
        description->setCursor(regionEditingAllowed ? Qt::PointingHandCursor : Qt::ArrowCursor);
        if (regionEditingAllowed) description->installEventFilter(this);
        nameLayout->addWidget(description);
        row->setToolTip(QStringLiteral("默认误差限适用于全部数据，自定义区域内采用对应区域的误差限"));
    }
    layout->addWidget(nameBlock, 1);

    const qulonglong displayedCount = region.id == 0
        ? static_cast<qulonglong>(numericFieldElementCount(m_selectedFieldIndex))
        : static_cast<qulonglong>(qMax(0, region.elementCount));
    auto* count = new QLabel(QStringLiteral("%1 个数据项").arg(displayedCount), row);
    count->setObjectName(QStringLiteral("DataCodecRegionMeta"));
    count->setMinimumWidth(68);
    count->setProperty("DataCodecRegionId", region.id);
    count->setCursor(regionEditingAllowed ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (regionEditingAllowed) count->installEventFilter(this);
    layout->addWidget(count);

    auto createPrecisionCell = [&]() {
        auto* cell = new QWidget(row);
        cell->setFixedWidth(190);
        cell->setProperty("DataCodecRegionId", region.id);
        cell->setCursor(regionEditingAllowed ? Qt::PointingHandCursor : Qt::ArrowCursor);
        if (regionEditingAllowed) cell->installEventFilter(this);
        auto* cellLayout = new QHBoxLayout(cell);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        cellLayout->setSpacing(6);
        auto* modeCombo = new QComboBox(cell);
        configureComboPopup(modeCombo);
        configurePrecisionModeCombo(modeCombo, region.precisionMode);
        modeCombo->setFixedWidth(88);
        auto* spin = new QDoubleSpinBox(cell);
        configurePrecisionSpin(spin);
        const int mode = effectivePrecisionMode(region.precisionMode);
        spin->setValue(precisionValueForMode(region, mode));
        const bool basisComputed = featureState != nullptr && featureState->basisComputed;
        const bool precisionEditable = regionEditingAllowed &&
            (region.id == 0 || basisComputed) &&
            !losslessModeEnabled();
        modeCombo->setEnabled(precisionEditable);
        spin->setEnabled(precisionEditable);
        applyPrecisionSpinTone(spin, m_selectedFieldIndex, mode, precisionValueForMode(region, mode), spin->isEnabled());
        spin->setFixedWidth(78);
        cellLayout->addWidget(modeCombo);
        cellLayout->addWidget(spin);
        layout->addWidget(cell);
        return std::pair<QComboBox*, QDoubleSpinBox*>{modeCombo, spin};
    };

    const auto precisionControls = createPrecisionCell();
    auto* modeCombo = precisionControls.first;
    auto* precisionSpin = precisionControls.second;

    if (region.id != 0) {
        auto* deleteButton = new QPushButton(QStringLiteral("×"), row);
        deleteButton->setObjectName(QStringLiteral("DataCodecDeleteRegionButton"));
        deleteButton->setFixedSize(22, 22);
        deleteButton->setToolTip(QStringLiteral("删除区域"));
        deleteButton->setEnabled(regionEditingAllowed);
        layout->addWidget(deleteButton, 0);
        connect(deleteButton, &QPushButton::clicked, this,
                [this, id = region.id]() {
                    deleteRegion(id);
                });
    }

    connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, id = region.id, modeCombo, precisionSpin](int) {
        if (losslessModeEnabled()) return;
        const int mode = effectivePrecisionMode(modeCombo->currentData().toInt());
        updateRegionPrecisionMode(id, mode);
        const auto* regions = currentRegions();
        if (regions != nullptr) {
            for (const RegionItem& item : *regions) {
                if (item.id != id) continue;
                QSignalBlocker blocker(precisionSpin);
                const double value = precisionValueForMode(item, mode);
                precisionSpin->setValue(value);
                applyPrecisionSpinTone(precisionSpin, m_selectedFieldIndex, mode, value, precisionSpin->isEnabled());
                break;
            }
        }
        const auto* selectedRegions = currentRegions();
        const int selectedIndex = selectedRegionIndex();
        if (selectedRegions != nullptr && selectedIndex >= 0 && selectedIndex < selectedRegions->size()) {
            if (m_precisionModeCombo) {
                QSignalBlocker blocker(m_precisionModeCombo);
                configurePrecisionModeCombo(m_precisionModeCombo, mode);
            }
            if (m_precisionSpin) {
                QSignalBlocker blocker(m_precisionSpin);
                const double value = precisionValueForMode((*selectedRegions)[selectedIndex], mode);
                m_precisionSpin->setValue(value);
                applyPrecisionSpinTone(m_precisionSpin, m_selectedFieldIndex, mode, value, m_precisionSpin->isEnabled());
            }
        }
        refreshRegionRows();
    });
    connect(precisionSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this, id = region.id, modeCombo, precisionSpin](double value) {
        if (losslessModeEnabled()) return;
        const int selectedIndex = selectedRegionIndex();
        const int mode = effectivePrecisionMode(modeCombo->currentData().toInt());
        updateRegionPrecisionValue(id, mode, value);
        applyPrecisionSpinTone(precisionSpin, m_selectedFieldIndex, mode, value, precisionSpin->isEnabled());
        const auto* regions = currentRegions();
        if (regions != nullptr && selectedIndex >= 0 && selectedIndex < regions->size() &&
            (*regions)[selectedIndex].id == id && m_precisionSpin) {
            QSignalBlocker blocker(m_precisionSpin);
            m_precisionSpin->setValue(value);
            applyPrecisionSpinTone(m_precisionSpin, m_selectedFieldIndex, mode, value, m_precisionSpin->isEnabled());
        }
    });
    return row;
}

void igQtDataCodecCompressionWidget::applyStyle() {
    setStyleSheet(QStringLiteral(R"(
QWidget#DataCodecCompressionWidget {
    background: #1e1e1e;
}
QWidget {
    color: #d4d4d4;
    font-family: "Microsoft YaHei", "Segoe UI";
    font-size: 12px;
}
QFrame#DataCodecPanel,
QFrame#DataCodecInnerPanel {
    background: #252526;
    border: 1px solid #3c3c3c;
    border-radius: 8px;
}
QFrame#DataCodecBasisScope {
    background: #1f2524;
    border: 1px solid #22736f;
    border-radius: 8px;
}
QFrame#DataCodecStatusPanel {
    background: #242424;
    border-top: 1px solid #3c3c3c;
}
QScrollArea#DataCodecRegionScroll,
QWidget#DataCodecRegionViewport,
QWidget#DataCodecRegionRowsHost {
    background: #252526;
    border: none;
}
QLabel#DataCodecSectionTitle,
QLabel#DataCodecBasisTitle {
    font-weight: 800;
    color: #f0f0f0;
}
QLabel#DataCodecLabel,
QLabel#DataCodecRegionMeta {
    color: #a8adaf;
    font-weight: 700;
}
QLabel#DataCodecLabelStrong,
QLabel#DataCodecRegionName {
    font-weight: 800;
    color: #f0f0f0;
}
QLabel#DataCodecTag {
    background: #123f3d;
    color: #85ddd6;
    border-radius: 10px;
    padding: 2px 8px;
    font-weight: 800;
}
QLabel#DataCodecPlainTag {
    color: #d8d8d8;
    font-weight: 800;
}
QLineEdit,
QComboBox,
QDoubleSpinBox,
QSpinBox {
    min-height: 32px;
    background: #1e1e1e;
    color: #e6e6e6;
    border: 1px solid #3c3c3c;
    border-radius: 6px;
    padding: 0 8px;
    selection-background-color: #0e639c;
}
QLineEdit:read-only {
    color: #b8bcbe;
    background: #252526;
}
QLineEdit:disabled,
QComboBox:disabled,
QDoubleSpinBox:disabled,
QSpinBox:disabled {
    color: #7b7b7b;
    background: #303030;
    border-color: #3a3a3a;
}
QComboBox:disabled::drop-down {
    border-left: 1px solid #3a3a3a;
}
QLineEdit:focus,
QComboBox:focus,
QDoubleSpinBox:focus,
QSpinBox:focus {
    border-color: #26a29c;
}
QComboBox::drop-down {
    border-left: 1px solid #3c3c3c;
    width: 20px;
}
QComboBox QAbstractItemView {
    background: #252526;
    color: #d4d4d4;
    border: 1px solid #3c3c3c;
    selection-background-color: #3a3a3a;
    selection-color: #ffffff;
}
QComboBox QAbstractItemView::item {
    min-height: 36px;
    padding: 8px 10px;
}
QPushButton {
    min-height: 30px;
    background: #2d2d30;
    color: #e6e6e6;
    border: 1px solid #3c3c3c;
    border-radius: 6px;
    padding: 0 10px;
    font-weight: 800;
}
QPushButton:hover {
    background: #3a3a3d;
    border-color: #4a4a4a;
}
QPushButton:pressed {
    background: #45454a;
}
QPushButton:disabled {
    color: #777777;
    background: #252526;
    border-color: #333333;
}
QPushButton#DataCodecPrimaryButton {
    background: #0e7772;
    color: white;
    border-color: #26a29c;
}
QPushButton#DataCodecPrimaryButton:hover {
    background: #0f8c86;
}
QPushButton#DataCodecPrimaryButton:disabled {
    color: #777777;
    background: #252526;
    border-color: #333333;
}
QPushButton#DataCodecDarkButton {
    background: #0e639c;
    color: #ffffff;
    border-color: #0e639c;
}
QPushButton#DataCodecDarkButton:hover {
    background: #1177bb;
}
QPushButton#DataCodecDarkButton:disabled {
    color: #777777;
    background: #252526;
    border-color: #333333;
}
QListWidget#DataCodecFieldList {
    background: transparent;
    border: none;
    outline: none;
}
QListWidget#DataCodecFieldList::item {
    margin: 3px 0;
    padding: 8px;
    border: 1px solid #3c3c3c;
    border-radius: 7px;
    background: #1e1e1e;
    color: #d4d4d4;
}
QListWidget#DataCodecFieldList::item:selected {
    background: #123f3d;
    border-color: #26a29c;
    color: #ffffff;
}
QFrame#DataCodecRegionRow {
    background: #1e1e1e;
    border: 1px solid #3c3c3c;
    border-radius: 8px;
}
QFrame#DataCodecRegionRow[selected="true"] {
    border-color: #26a29c;
    background: #183331;
}
QFrame#DataCodecRegionRow[base="true"] {
    background: #2d271c;
}
QFrame#DataCodecRegionRow[base="true"][selected="true"] {
    border-color: #9b7840;
}
QFrame#DataCodecRegionRow[invalid="true"] {
    border-color: #ee5b5b;
}
QLabel#DataCodecRegionWarning {
    color: #ee5b5b;
    font-size: 16px;
    font-weight: 900;
}
QPushButton#DataCodecDeleteRegionButton {
    background: transparent;
    border: 1px solid #4a4a4a;
    border-radius: 11px;
    color: #d4d4d4;
    font-family: Arial;
    font-size: 12px;
    font-weight: 800;
    padding: 0px;
}
QPushButton#DataCodecDeleteRegionButton:hover {
    border-color: #dc7036;
    color: #ffffff;
    background: #3a241f;
}
QFrame#DataCodecStatBox {
    background: #1e1e1e;
    border: 1px solid #3c3c3c;
    border-radius: 7px;
}
QLabel#DataCodecStatValue {
    font-size: 16px;
    font-weight: 800;
    color: #ffffff;
}
QPlainTextEdit#DataCodecLog {
    background: #1e1e1e;
    border: 1px solid #3c3c3c;
    border-radius: 3px;
    font-family: Consolas, "Microsoft YaHei";
    color: #d4d4d4;
    selection-background-color: #0e639c;
}
QCheckBox {
    color: #d4d4d4;
}
QCheckBox:disabled {
    color: #7a7a7a;
}
QCheckBox::indicator {
    width: 14px;
    height: 14px;
}
QCheckBox::indicator:unchecked {
    background: #252526;
    border: 1px solid #5a5a5a;
    border-radius: 2px;
}
QCheckBox::indicator:checked {
    background: #0e639c;
    border: 1px solid #0e639c;
    border-radius: 2px;
}
)"));
}

void igQtDataCodecCompressionWidget::resetRegionState() {
    ++m_featureRequestSerial;
    m_featureComputationActive = false;
    if (m_computeButton != nullptr) m_computeButton->setText(QStringLiteral("计算特征"));
    m_fieldStates.clear();
    ensureFieldStates();
    if (m_rangeSlider) {
        QSignalBlocker blocker(m_rangeSlider);
        m_rangeSlider->setRangeValues(76, 100);
    }
    if (m_histogram) m_histogram->clearBins();
    refreshFieldItemLabels();
    refreshBasisOptions();
    refreshRegionRows();
    refreshSelectedRegion();
    refreshStats();
}

void igQtDataCodecCompressionWidget::refreshFields() {
    m_fields.clear();

    auto dataObject = m_model != nullptr ? m_model->GetDataObject() : nullptr;
    std::vector<iGame::DataCodecEncodeAttributeDescriptor> descriptors;
    if (dataObject != nullptr) {
        iGame::CollectDataCodecEncodeRepresentativeAttributeCatalog(dataObject, descriptors);
    }
    const bool multiFrame = hasMultiFrameData();
    std::map<std::string, int> fieldByKey;
    for (const auto& descriptor : descriptors) {
        const int attachmentType = descriptor.attachment == ::datacodec::AttrAttachment::Cell
            ? IG_CELL
            : IG_POINT;
        const auto key = multiFrame
            ? descriptor.name
            : descriptor.name + "\n" +
                std::to_string(static_cast<std::uint32_t>(descriptor.attachment)) + "\n" +
                std::to_string(static_cast<std::uint32_t>(descriptor.dataType)) + "\n" +
                std::to_string(descriptor.componentCount);
        const auto iterator = fieldByKey.find(key);
        if (iterator == fieldByKey.end()) {
            const int fieldIndex = m_fields.size();
            fieldByKey.emplace(key, fieldIndex);
            NumericFieldItem field;
            field.name = QString::fromUtf8(descriptor.name.c_str());
            field.detail = fieldDetailName(
                attachmentType,
                numericTypeName(descriptor.dataType),
                descriptor.componentCount);
            field.sourceObject = descriptor.sourceObject;
            field.targets.push_back(descriptor.target);
            field.attachmentType = attachmentType;
            field.componentCount = qMax(1, descriptor.componentCount);
            m_fields.push_back(std::move(field));
        } else {
            m_fields[iterator->second].targets.push_back(descriptor.target);
        }
    }
    if (!m_fieldList) return;
    QSignalBlocker blocker(m_fieldList);
    m_fieldList->clear();
    if (m_fields.isEmpty()) {
        m_fieldStates.clear();
        m_fieldList->setEnabled(false);
        auto* item = new QListWidgetItem(QStringLiteral("未载入数据"));
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(QSize(274, 44));
        m_fieldList->addItem(item);
        m_selectedFieldIndex = -1;
        refreshSelectAllState();
        refreshPredictionControls();
        refreshBasisOptions();
        return;
    }
    m_fieldList->setEnabled(true);
    ensureFieldStates();
    for (int i = 0; i < m_fields.size(); ++i) {
        auto* item = new QListWidgetItem;
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_fieldStates[i].selected ? Qt::Checked : Qt::Unchecked);
        item->setText(fieldItemText(i));
        item->setSizeHint(QSize(274, 68));
        m_fieldList->addItem(item);
    }
    m_selectedFieldIndex = qBound(0, m_selectedFieldIndex, qMax(0, m_fields.size() - 1));
    m_fieldList->setCurrentRow(m_selectedFieldIndex);
    refreshSelectAllState();
    refreshPredictionControls();
    refreshBasisOptions();
}

void igQtDataCodecCompressionWidget::refreshFieldItemLabels() {
    if (!m_fieldList || m_fields.isEmpty()) return;
    ensureFieldStates();
    QSignalBlocker blocker(m_fieldList);
    for (int i = 0; i < m_fields.size() && i < m_fieldList->count(); ++i) {
        auto* item = m_fieldList->item(i);
        if (!item) continue;
        item->setCheckState(m_fieldStates[i].selected ? Qt::Checked : Qt::Unchecked);
        item->setText(fieldItemText(i));
        item->setSizeHint(QSize(274, 68));
    }
    if (m_selectedFieldIndex >= 0 && m_selectedFieldIndex < m_fieldList->count()) {
        m_fieldList->setCurrentRow(m_selectedFieldIndex);
    }
    refreshSelectAllState();
}

void igQtDataCodecCompressionWidget::refreshSelectAllState() {
    if (m_selectAllFieldsCheck == nullptr) return;
    const int selectedCount = static_cast<int>(std::count_if(
        m_fieldStates.begin(),
        m_fieldStates.end(),
        [](const FieldState& state) { return state.selected; }));
    Qt::CheckState state = Qt::Unchecked;
    if (!m_fieldStates.isEmpty() && selectedCount == m_fieldStates.size()) {
        state = Qt::Checked;
    } else if (selectedCount > 0) {
        state = Qt::PartiallyChecked;
    }
    QSignalBlocker blocker(m_selectAllFieldsCheck);
    m_selectAllFieldsCheck->setEnabled(!m_fieldStates.isEmpty() && !m_featureComputationActive);
    m_selectAllFieldsCheck->setCheckState(state);
}

void igQtDataCodecCompressionWidget::refreshBasisOptions() {
    if (!m_basisCombo) return;
    auto* fieldState = currentFieldState();
    if (fieldState != nullptr) {
        ensureFeatureStates(*fieldState);
    }
    const int currentIndex =
            fieldState != nullptr ? qBound(0, fieldState->currentBasisIndex, 2) : qBound(0, m_basisCombo->currentIndex(), 2);
    QSignalBlocker blocker(m_basisCombo);
    while (m_basisCombo->count() < 3) {
        m_basisCombo->addItem(QString());
    }
    while (m_basisCombo->count() > 3) {
        m_basisCombo->removeItem(m_basisCombo->count() - 1);
    }
    for (int i = 0; i < 3; ++i) {
        m_basisCombo->setItemText(i, basisOptionText(i, fieldState));
    }
    m_basisCombo->setCurrentIndex(currentIndex);
}

void igQtDataCodecCompressionWidget::refreshRegionRows() {
    if (!m_regionRowsLayout) return;
    clearLayout(m_regionRowsLayout);
    if (!hasNumericFields()) {
        if (m_regionRowsHost) m_regionRowsHost->setMinimumHeight(0);
        if (m_regionScrollArea) m_regionScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_regionRowsLayout->addStretch();
        return;
    }
    const bool multiFrame = hasMultiFrameData();
    const auto* regions = currentRegions();
    const int rowCount = regions != nullptr
        ? static_cast<int>(std::count_if(
              regions->begin(),
              regions->end(),
              [multiFrame](const RegionItem& region) { return !multiFrame || region.id == 0; }))
        : 0;
    if (m_regionRowsHost) {
        const int rowsHeight = rowCount * 52 + qMax(0, rowCount - 1) * m_regionRowsLayout->spacing();
        m_regionRowsHost->setMinimumHeight(rowsHeight);
        if (m_regionScrollArea) {
            m_regionScrollArea->setVerticalScrollBarPolicy(rowCount > 2 ? Qt::ScrollBarAlwaysOn
                                                                         : Qt::ScrollBarAsNeeded);
        }
    }
    if (regions == nullptr) {
        m_regionRowsLayout->addStretch();
        return;
    }
    const auto* featureState = currentFeatureState();
    const QVector<int> invalidIds = featureState != nullptr ? invalidRegionIdsForFeature(*featureState) : QVector<int>{};
    for (const RegionItem& region : *regions) {
        if (multiFrame && region.id != 0) continue;
        m_regionRowsLayout->addWidget(createRegionRow(region, invalidIds.contains(region.id)));
    }
    m_regionRowsLayout->addStretch();
}

void igQtDataCodecCompressionWidget::refreshSelectedRegion() {
    if (!hasNumericFields()) {
        if (m_regionBuilderTitle) m_regionBuilderTitle->clear();
        if (m_losslessModeCheck) {
            QSignalBlocker blocker(m_losslessModeCheck);
            m_losslessModeCheck->setChecked(false);
        }
        refreshFeatureState();
        return;
    }

    auto* fieldState = currentFieldState();
    auto* featureState = currentFeatureState();
    if (fieldState != nullptr) {
        ensureFeatureStates(*fieldState);
    }
    if (hasMultiFrameData() && featureState != nullptr && featureState->selectedRegionId != 0) {
        featureState->selectedRegionId = 0;
    }
    const int index = selectedRegionIndex();
    if (index < 0) return;
    const auto* regions = currentRegions();
    if (regions == nullptr) return;
    const RegionItem& region = (*regions)[index];
    if (featureState != nullptr) {
        featureState->rangeLower = region.id == 0 ? 0 : region.rangeLower;
        featureState->rangeUpper = region.id == 0 ? 100 : region.rangeUpper;
    }

    if (m_regionBuilderTitle) {
        m_regionBuilderTitle->setText(QStringLiteral("当前生效配置：%1 / %2")
                                              .arg(m_fields[m_selectedFieldIndex].name, currentBasisName()));
    }
    if (m_losslessModeCheck && fieldState != nullptr) {
        QSignalBlocker blocker(m_losslessModeCheck);
        m_losslessModeCheck->setChecked(fieldState->losslessMode);
    }
    if (m_rangeSlider && featureState != nullptr) {
        QSignalBlocker blocker(m_rangeSlider);
        m_rangeSlider->setRangeValues(featureState->rangeLower, featureState->rangeUpper);
    }
    const int mode = effectivePrecisionMode(region.precisionMode);
    if (m_precisionModeCombo) {
        QSignalBlocker blocker(m_precisionModeCombo);
        configurePrecisionModeCombo(m_precisionModeCombo, mode);
    }
    if (m_precisionSpin) {
        QSignalBlocker blocker(m_precisionSpin);
        const double value = precisionValueForMode(region, mode);
        m_precisionSpin->setValue(value);
        const bool basisComputed = featureState != nullptr && featureState->basisComputed;
        const bool multiFrame = hasMultiFrameData();
        const bool precisionEditable = hasNumericFields() &&
            fieldState != nullptr &&
            fieldState->selected &&
            (region.id == 0 || (!multiFrame && basisComputed)) &&
            !losslessModeEnabled();
        applyPrecisionSpinTone(m_precisionSpin, m_selectedFieldIndex, mode, value, precisionEditable);
    }
    refreshFeatureState();
}

void igQtDataCodecCompressionWidget::refreshFeatureState() {
    auto* fieldState = currentFieldState();
    if (fieldState != nullptr) {
        ensureFeatureStates(*fieldState);
    }
    auto* featureState = currentFeatureState();
    const bool selectedBase = featureState == nullptr || featureState->selectedRegionId == 0;
    const bool hasField = hasNumericFields();
    const bool fieldSelected = fieldState != nullptr && fieldState->selected;
    const bool hasSelectedField = hasSelectedFields();
    const bool losslessMode = losslessModeEnabled();
    const bool basisComputed = featureState != nullptr && featureState->basisComputed;
    const bool multiFrame = hasMultiFrameData();
    const bool hasData = !m_fields.isEmpty();
    const bool regionFeaturesEnabled = hasField && fieldSelected && !multiFrame;
    const int invalidRegionCount = invalidActiveRegionCount();
    const int unavailableRegionFeatureCount = unavailableActiveRegionFeatureCount();
    if (m_basisStateLabel) {
        m_basisStateLabel->setText(
            multiFrame
                ? QStringLiteral("时序数据使用统一误差限")
                : (basisComputed ? currentBasisName() + QStringLiteral(" 已计算")
                                 : QStringLiteral("待计算")));
    }
    if (m_performanceCombo) m_performanceCombo->setEnabled(hasData);
    if (m_zstdLevelSpin) m_zstdLevelSpin->setEnabled(hasData);
    if (m_fieldList) m_fieldList->setEnabled(hasData && !m_featureComputationActive);
    if (m_selectAllFieldsCheck) m_selectAllFieldsCheck->setEnabled(hasData && !m_featureComputationActive);
    if (m_basisCombo) m_basisCombo->setEnabled(regionFeaturesEnabled && !m_featureComputationActive);
    if (m_computeButton) {
        m_computeButton->setEnabled(regionFeaturesEnabled && !basisComputed && !m_featureComputationActive);
        m_computeButton->setText(m_featureComputationActive ? QStringLiteral("计算中...")
                                                           : QStringLiteral("计算特征"));
    }
    if (m_addRegionButton) {
        m_addRegionButton->setEnabled(hasData);
        m_addRegionButton->setToolTip(QStringLiteral("添加自定义区域"));
    }
    if (m_outputPathEdit) {
        m_outputPathEdit->setEnabled(hasSelectedField);
        if (!hasSelectedField && !m_outputPathEdit->text().isEmpty()) {
            m_outputPathEdit->clear();
        }
    }
    if (m_outputPathButton) m_outputPathButton->setEnabled(hasSelectedField);
    if (m_applyLosslessAllButton) m_applyLosslessAllButton->setEnabled(hasSelectedField);
    if (m_applyLossyAllButton) m_applyLossyAllButton->setEnabled(hasSelectedField);
    if (m_syncDefaultPrecisionButton) {
        m_syncDefaultPrecisionButton->setEnabled(hasField && fieldSelected && !losslessMode);
    }
    if (m_startEncodeButton) {
        m_startEncodeButton->setEnabled(
            hasSelectedField &&
            invalidRegionCount == 0 &&
            unavailableRegionFeatureCount == 0 &&
            !m_featureComputationActive);
        if (invalidRegionCount > 0) {
            m_startEncodeButton->setToolTip(QStringLiteral("请先消除所有自定义区域重叠"));
        } else if (unavailableRegionFeatureCount > 0) {
            m_startEncodeButton->setToolTip(QStringLiteral("请先重新计算自定义区域所需的特征"));
        } else if (m_featureComputationActive) {
            m_startEncodeButton->setToolTip(QStringLiteral("请等待特征计算完成"));
        } else {
            m_startEncodeButton->setToolTip(QString());
        }
    }
    if (m_emitPerformanceCheck) {
        m_emitPerformanceCheck->setEnabled(hasSelectedField && !multiFrame);
        if (multiFrame) m_emitPerformanceCheck->setChecked(false);
    }
    if (m_losslessModeCheck) m_losslessModeCheck->setEnabled(hasField && fieldSelected);
    if (m_histogram) {
        if (basisComputed && featureState != nullptr && featureState->featureBasis != nullptr) {
            const HistogramView& histogramView = currentHistogramView();
            m_histogram->setHistogramView(histogramView.bins, histogramView.backgroundCount, histogramView.totalCount);
            m_histogram->setOccupiedBins(currentOccupiedHistogramBins(histogramView));
            if (m_rangeSlider) {
                m_rangeSlider->setTrackInsets(histogramView.smart ? kSmartHistogramLeftInset : kHistogramEdgeInset,
                                              kHistogramEdgeInset);
            }
        } else {
            m_histogram->clearBins();
            if (m_rangeSlider) m_rangeSlider->setTrackInsets(kHistogramEdgeInset, kHistogramEdgeInset);
        }
        m_histogram->setEnabledVisual(
            regionFeaturesEnabled && basisComputed && !selectedBase && !m_featureComputationActive);
        m_histogram->setRange(m_rangeSlider ? m_rangeSlider->lowerValue() : 76,
                              m_rangeSlider ? m_rangeSlider->upperValue() : 100);
    }
    if (m_rangeSlider) {
        m_rangeSlider->setSliderEnabled(
            regionFeaturesEnabled && basisComputed && !selectedBase && !m_featureComputationActive);
    }
    const bool precisionEditable = hasField &&
        fieldSelected &&
        (selectedBase || (!multiFrame && basisComputed)) &&
        !losslessMode &&
        !m_featureComputationActive;
    if (m_precisionModeCombo) m_precisionModeCombo->setEnabled(precisionEditable);
    if (m_precisionSpin) {
        m_precisionSpin->setEnabled(precisionEditable);
        const int index = selectedRegionIndex();
        const auto* regions = currentRegions();
        if (regions != nullptr && index >= 0 && index < regions->size()) {
            const RegionItem& region = (*regions)[index];
            const int mode = effectivePrecisionMode(region.precisionMode);
            applyPrecisionSpinTone(m_precisionSpin, m_selectedFieldIndex, mode,
                                   precisionValueForMode(region, mode), precisionEditable);
        }
    }
    refreshBasisOptions();
    refreshStats();
}

void igQtDataCodecCompressionWidget::refreshStats() {
    const auto* featureState = currentFeatureState();
    const bool selectedBase = featureState == nullptr || featureState->selectedRegionId == 0;
    const bool hasField = hasNumericFields();
    const std::uint64_t total = hasField && featureState != nullptr && featureState->featureBasis != nullptr
                                    ? static_cast<std::uint64_t>(featureState->featureBasis->GetElementCount())
                                    : static_cast<std::uint64_t>(numericFieldElementCount(m_selectedFieldIndex));
    const std::uint64_t selected = selectedBase
        ? total
        : (hasField && featureState != nullptr && featureState->basisComputed
               ? featureSelectionCountForRange64(
                     *featureState,
                     featureState->rangeLower,
                     featureState->rangeUpper)
               : 0u);
    const int overlap = hasField && featureState != nullptr && featureState->basisComputed && !selectedBase
                                ? currentSelectionOverlapCount(featureState->selectedRegionId)
                                : 0;

    if (m_selectedTotalLabel) {
        m_selectedTotalLabel->setText(QStringLiteral("%1 / %2")
                                          .arg(static_cast<qulonglong>(selected))
                                          .arg(static_cast<qulonglong>(total)));
    }
    if (m_overlapTotalLabel) {
        m_overlapTotalLabel->setText(QString::number(overlap));
        m_overlapTotalLabel->setStyleSheet(overlap > 0 ? QStringLiteral("color: #ee5b5b;") : QString());
    }
}

bool igQtDataCodecCompressionWidget::hasNumericFields() const {
    return m_selectedFieldIndex >= 0 && m_selectedFieldIndex < m_fields.size();
}

bool igQtDataCodecCompressionWidget::hasSelectedFields() const {
    return std::any_of(
        m_fieldStates.begin(),
        m_fieldStates.end(),
        [](const FieldState& state) { return state.selected; });
}

std::vector<::datacodec::AttributeTarget>
igQtDataCodecCompressionWidget::selectedAttributeTargets() const {
    std::vector<::datacodec::AttributeTarget> targets;
    for (int index = 0; index < m_fields.size() && index < m_fieldStates.size(); ++index) {
        if (!m_fieldStates[index].selected) continue;
        targets.insert(
            targets.end(),
            m_fields[index].targets.begin(),
            m_fields[index].targets.end());
    }
    return targets;
}

bool igQtDataCodecCompressionWidget::hasMultiFrameData() const {
    const auto dataObject = m_model != nullptr ? m_model->GetDataObject() : nullptr;
    const auto timeFrames = dataObject != nullptr ? dataObject->PeekTimeFrames() : nullptr;
    return timeFrames != nullptr && timeFrames->GetTimeNum() > 1;
}

void igQtDataCodecCompressionWidget::appendPredictionEncodingRecommendation() {
    const bool intraFieldPredictionEnabled =
        m_intraAttributePredictionCheck != nullptr &&
        m_intraAttributePredictionCheck->isEnabled() &&
        m_intraAttributePredictionCheck->isChecked();
    const bool temporalFieldPredictionEnabled =
        m_temporalAttributePredictionCheck != nullptr &&
        m_temporalAttributePredictionCheck->isEnabled() &&
        m_temporalAttributePredictionCheck->isChecked();
    if (!intraFieldPredictionEnabled && !temporalFieldPredictionEnabled) return;

    publishStatus(dataCodecHostLogText(
        m_dataCodecLanguage,
        iGame::iGameDataCodecHostMessageId::PredictionEncodingRecommendation));
}

void igQtDataCodecCompressionWidget::refreshPredictionControls() {
    const bool hasFields = hasSelectedFields();
    const bool hasMultiFrame = hasFields && hasMultiFrameData();
    if (m_intraAttributePredictionCheck != nullptr) {
        QSignalBlocker blocker(m_intraAttributePredictionCheck);
        m_intraAttributePredictionCheck->setEnabled(hasFields);
        m_intraAttributePredictionCheck->setChecked(hasFields && m_intraAttributePredictionPreferred);
    }
    if (m_temporalAttributePredictionCheck != nullptr) {
        QSignalBlocker blocker(m_temporalAttributePredictionCheck);
        m_temporalAttributePredictionCheck->setEnabled(hasMultiFrame);
        m_temporalAttributePredictionCheck->setChecked(hasMultiFrame && m_temporalAttributePredictionPreferred);
    }
    refreshPerformanceControls();
}

void igQtDataCodecCompressionWidget::refreshPerformanceControls() {
    if (m_gopControlRow == nullptr || m_gopFrameCountSpin == nullptr) return;
    const auto dataObject = m_model != nullptr ? m_model->GetDataObject() : nullptr;
    const auto timeFrames = dataObject != nullptr ? dataObject->PeekTimeFrames() : nullptr;
    const auto availableFrameCount = timeFrames != nullptr
        ? timeFrames->GetTimeNum()
        : std::size_t{1};
    const auto frameCount = static_cast<int>(std::min(
        std::max<std::size_t>(std::size_t{1}, availableFrameCount),
        static_cast<std::size_t>(std::numeric_limits<int>::max())));
    const bool multiFrame = frameCount > 1;
    const bool hasData = hasNumericFields();
    if (m_compressionEnhancementCheck != nullptr) {
        m_compressionEnhancementCheck->setEnabled(hasData);
    }
    m_gopControlRow->setVisible(multiFrame);
    m_gopControlRow->setEnabled(multiFrame && hasData);
    QSignalBlocker blocker(m_gopFrameCountSpin);
    m_gopFrameCountSpin->setRange(1, frameCount);
}

::datacodec::DataCodecEncodeTier
igQtDataCodecCompressionWidget::selectedPerformanceTier() const {
    if (m_performanceCombo == nullptr) {
        return kDefaultEncodeTier;
    }
    return DecodeEncodeTierFromValue(m_performanceCombo->currentData().toInt());
}

void igQtDataCodecCompressionWidget::applyPerformanceTierDefaults() {
    const auto definition = ::datacodec::MakeEncodeConfigurationParams(
        ::datacodec::DataCodecEncodeOptions{.tier = selectedPerformanceTier()});
    if (m_zstdLevelSpin != nullptr) {
        QSignalBlocker blocker(m_zstdLevelSpin);
        m_zstdLevelSpin->setValue(definition.pipelineControl.packageFields.zstdLevel);
    }
    persistPerformanceSettings();
}

void igQtDataCodecCompressionWidget::persistPerformanceSettings() const {
    const int zstdLevel = m_zstdLevelSpin != nullptr ? m_zstdLevelSpin->value() : 3;
    const int gopFrameCount = m_gopFrameCountSpin != nullptr
        ? m_gopFrameCountSpin->value()
        : 8;
    SaveDataCodecCompressionPerformanceSettings({
        .tier = selectedPerformanceTier(),
        .compressionEnhancement = m_compressionEnhancementCheck != nullptr &&
            m_compressionEnhancementCheck->isChecked(),
        .zstdLevel = zstdLevel,
        .gopFrameCount = gopFrameCount,
    });
}

void igQtDataCodecCompressionWidget::appendLog(
        const QString& text) {
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    const QString line = QStringLiteral("[%1] %2").arg(time, text);
    if (!m_logEdit) return;
    m_logEdit->appendPlainText(line);
    if (auto* scrollBar = m_logEdit->verticalScrollBar()) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void igQtDataCodecCompressionWidget::publishStatus(
        const QString& text,
        const ::datacodec::DataCodecStatusSeverity severity) {
    appendLog(text);
    iGame::iGameSpdlogDataCodecConsoleSink console;
    console.SubmitConsoleStatus(::datacodec::DataCodecStatusRecord{
        .severity = severity,
        .language = m_dataCodecLanguage,
        .text = toUtf8StdString(text),
    });
}

void igQtDataCodecCompressionWidget::chooseOutputPath() {
    if (!hasSelectedFields()) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::LoadDataBeforeOutputPath),
            ::datacodec::DataCodecStatusSeverity::Warning);
        return;
    }

    const bool multiFrame = hasMultiFrameData();
    const QString path = QFileDialog::getSaveFileName(resolveDataCodecFileDialogParent(this),
                                                       multiFrame
                                                           ? QStringLiteral("选择序列压缩文件的名称前缀")
                                                           : QStringLiteral("选择压缩文件"),
                                                       m_outputPathEdit ? m_outputPathEdit->text() : QString(),
                                                       QStringLiteral("压缩文件 (*.igc)"));
    if (path.isEmpty()) return;
    const QString normalizedPath = normalizeDataCodecOutputPath(path);
    if (m_outputPathEdit) m_outputPathEdit->setText(normalizedPath);
    publishStatus(dataCodecHostLogText(
        m_dataCodecLanguage,
        iGame::iGameDataCodecHostMessageId::OutputPathSet,
        {{"path", toUtf8StdString(normalizedPath)}}));
}

void igQtDataCodecCompressionWidget::startEncode() {
    if (!hasSelectedFields()) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::SelectCompressionData),
            ::datacodec::DataCodecStatusSeverity::Warning);
        return;
    }
    if (m_featureComputationActive) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::WaitForFeatureComputation),
            ::datacodec::DataCodecStatusSeverity::Warning);
        return;
    }
    const int unavailableRegionFeatureCount = unavailableActiveRegionFeatureCount();
    if (unavailableRegionFeatureCount > 0) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::MissingRegionFeatures,
                {{"count", std::to_string(unavailableRegionFeatureCount)}}),
            ::datacodec::DataCodecStatusSeverity::Warning);
        refreshFeatureState();
        return;
    }
    const int invalidRegionCount = invalidActiveRegionCount();
    if (invalidRegionCount > 0) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::OverlappingRegionsBlockCompression,
                {{"count", std::to_string(invalidRegionCount)}}),
            ::datacodec::DataCodecStatusSeverity::Warning);
        refreshFeatureState();
        return;
    }
    if (m_outputPathEdit == nullptr || m_outputPathEdit->text().trimmed().isEmpty()) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::SetOutputPath),
            ::datacodec::DataCodecStatusSeverity::Warning);
        return;
    }
    auto dataObject = m_model != nullptr ? m_model->GetDataObject() : nullptr;
    if (dataObject == nullptr) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::CompressionNoData),
            ::datacodec::DataCodecStatusSeverity::Error);
        return;
    }

    const QString outputPath = normalizeDataCodecOutputPath(m_outputPathEdit->text());
    const bool multiFrame = hasMultiFrameData();
    if (m_outputPathEdit->text() != outputPath) {
        m_outputPathEdit->setText(outputPath);
    }

    const QFileInfo outputInfo(outputPath);
    if (!QDir().mkpath(outputInfo.absolutePath())) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::CreateOutputDirectoryFailed,
                {{"path", toUtf8StdString(outputInfo.absolutePath())}}),
            ::datacodec::DataCodecStatusSeverity::Error);
        return;
    }
    const bool outputPerformance = !multiFrame && m_emitPerformanceCheck != nullptr &&
        m_emitPerformanceCheck->isChecked();
    const qint64 initialBeforeBytes = outputPerformance
        ? sourceFileSizeFromDataObject(dataObject)
        : -1;
    const std::string compressionRatioNote = outputPerformance
        ? iGame::iGameDataCodecHostMessage(
            m_dataCodecLanguage,
            iGame::iGameDataCodecHostMessageId::CompressionRatioCalculationNote)
        : std::string{};
    ::datacodec::DataCodecEncodeOptions options;
    options.tier = selectedPerformanceTier();
    options.enableCompressionEnhancement =
        m_compressionEnhancementCheck != nullptr &&
        m_compressionEnhancementCheck->isChecked();
    if (m_zstdLevelSpin != nullptr) {
        options.packageZstdLevel = m_zstdLevelSpin->value();
    }
    if (multiFrame && m_gopFrameCountSpin != nullptr) {
        options.temporalKeyFrameInterval = static_cast<std::uint32_t>(
            m_gopFrameCountSpin->value());
    }

    auto writer = iGame::IGDCWriter::New();
    writer->SetAttributeTargets(selectedAttributeTargets());
    auto definition = ::datacodec::MakeEncodeConfigurationParams(options);
    definition.language = m_dataCodecLanguage;
    applyCompressionControlToDataCodec(definition.controlParams);
    const auto reportConfiguration =
        ::datacodec::MakeDataCodecEncodeReportConfiguration(
            options.tier,
            options.enableCompressionEnhancement,
            definition);
    writer->SetEncodeControls(definition);
    QString reportDirectory;
    if (outputPerformance) {
        reportDirectory = makeCompressionReportDirectory(outputPath);
        if (!QDir().mkpath(reportDirectory)) {
            publishStatus(
                dataCodecHostLogText(
                    m_dataCodecLanguage,
                    iGame::iGameDataCodecHostMessageId::CreateReportDirectoryFailed,
                    {{"path", toUtf8StdString(reportDirectory)}}),
                ::datacodec::DataCodecStatusSeverity::Error);
            return;
        }
    }
    std::shared_ptr<iGame::iGameDataCodecReportFileSink> reportSink;
    std::string reportFileTimestamp;
    if (outputPerformance) {
        reportSink = std::make_shared<iGame::iGameDataCodecReportFileSink>(
            std::filesystem::u8path(toUtf8StdString(reportDirectory)),
            iGame::iGameDataCodecReportFileMode::Replace);
        reportFileTimestamp = ::datacodec::MakeDataCodecReportFileTimestampUtc();
        const ::datacodec::DataCodecProcessReport runningReport{
            .operation = ::datacodec::TelemetryRunKind::Encode,
            .generatedAtUtc = toUtf8StdString(
                QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)),
            .objectName = toUtf8StdString(outputInfo.fileName()),
            .completed = false,
            .success = false,
            .inputBytes = static_cast<std::uint64_t>(
                std::max<qint64>(initialBeforeBytes, 0)),
            .summaryNote = compressionRatioNote,
            .details = {
                {"outputPath", toUtf8StdString(outputPath)},
            },
            .processes = {
                ::datacodec::DataCodecProcessNode{
                    .name = "EncodeData",
                    .completed = false,
                },
            },
            .configuration = reportConfiguration,
        };
        const auto runningReportResult = writeDataCodecRunReports(
            reportSink,
            reportFileTimestamp,
            runningReport,
            std::nullopt);
        if (!runningReportResult.success) {
            publishStatus(
                dataCodecHostLogText(
                    m_dataCodecLanguage,
                    iGame::iGameDataCodecHostMessageId::WriteReportFailed),
                ::datacodec::DataCodecStatusSeverity::Error);
            return;
        }
    }

    QPointer<igQtDataCodecCompressionWidget> self(this);
    ::datacodec::DataCodecOutputSinks outputSinks;
    outputSinks.ui = std::make_shared<igQtDataCodecUiSink>(
        this,
        [self](
            const QString& text,
            const ::datacodec::DataCodecStatusSeverity) {
            if (self) {
                self->appendLog(text);
            }
        });
    outputSinks.console = std::make_shared<iGame::iGameSpdlogDataCodecConsoleSink>();
    outputSinks.progress = std::make_shared<iGame::iGameDataCodecProgressBarSink>();
    writer->SetOutputSinks(std::move(outputSinks));
    iGame::iGameDataCodecTelemetryCapture telemetryCapture;
    auto sessionInterests = ::datacodec::kRunLifecycleRecordMask |
        ::datacodec::RunRecordKind::Message;
    auto collectionRequests = ::datacodec::RunCollectionMask{0u};
    if (outputPerformance) {
        sessionInterests = sessionInterests |
            ::datacodec::RunRecordKind::StageTiming |
            ::datacodec::RunRecordKind::ResourceUsage |
            ::datacodec::RunRecordKind::Artifact;
        collectionRequests = ::datacodec::RunCollectionBit(
            ::datacodec::RunCollectionKind::MemoryTrace);
        telemetryCapture.CaptureRemapOrders();
    }
    telemetryCapture.CaptureSessions(
        sessionInterests,
        collectionRequests,
        ::datacodec::TelemetrySessionDetail::ProcessSummary);
    writer->SetTelemetrySink(telemetryCapture.Sink());
    QVector<NumericFieldItem> precisionFields;
    for (int index = 0; index < m_fields.size() && index < m_fieldStates.size(); ++index) {
        if (m_fieldStates[index].selected) {
            precisionFields.push_back(m_fields[index]);
        }
    }
    const QString compressorName = QStringLiteral("SZ3");
    if (auto* progress = iGame::ProgressObserver::Instance(); progress != nullptr) {
        progress->UpdateProgress(0.0);
        progress->UpdateText("");
    }
    if (m_startEncodeButton != nullptr) {
        m_startEncodeButton->setEnabled(false);
    }

    auto complete = [self](
        QStringList messages,
        const ::datacodec::DataCodecStatusSeverity severity =
            ::datacodec::DataCodecStatusSeverity::Info) {
        if (!self) return;
        QMetaObject::invokeMethod(self, [
            self,
            messages = std::move(messages),
            severity]() {
            if (!self) return;
            for (const auto& message : messages) {
                self->publishStatus(message, severity);
            }
            self->refreshFeatureState();
        }, Qt::QueuedConnection);
    };

    auto* worker = QThread::create([
        writer,
        dataObject,
        outputPath,
        outputPerformance,
        reportSink,
        reportFileTimestamp,
        reportConfiguration,
        compressionRatioNote,
        initialBeforeBytes,
        precisionFields,
        compressorName,
        logLanguage = m_dataCodecLanguage,
        telemetryCapture,
        complete = std::move(complete)]() mutable {
        qint64 beforeBytes = initialBeforeBytes;
        QStringList writtenPaths;
        qint64 totalWrittenBytes = 0;
        qint64 encodeElapsedMs = 0;
        bool encodeSucceeded = false;
        QString writtenPath = outputPath;

        const auto writeEncodeReports = [&outputPerformance,
                                         &reportSink,
                                         &reportFileTimestamp,
                                         &reportConfiguration,
                                         &compressionRatioNote,
                                         &telemetryCapture,
                                         &beforeBytes,
                                         &totalWrittenBytes,
                                         &encodeElapsedMs,
                                         &encodeSucceeded,
                                         &writtenPath,
                                         &compressorName](
                const bool reportSuccess,
                std::vector<::datacodec::DataCodecProcessNode> additionalProcesses,
                std::vector<::datacodec::TelemetryMessageRecord> additionalMessages,
                std::string fallbackError) {
            DataCodecRunReportWriteResult skipped{.success = true};
            if (!outputPerformance) {
                return skipped;
            }
            const auto sessions = telemetryCapture.SnapshotCompletedTelemetrySessions();
            auto messages = telemetryCapture.SnapshotMessages();
            messages.insert(
                messages.end(),
                std::make_move_iterator(additionalMessages.begin()),
                std::make_move_iterator(additionalMessages.end()));
            if (beforeBytes <= 0) {
                std::uint64_t sourceBytes = 0u;
                std::uint64_t inputBytes = 0u;
                for (const auto& session : sessions) {
                    if (session.runKind != ::datacodec::TelemetryRunKind::Encode) {
                        continue;
                    }
                    sourceBytes = std::max(sourceBytes, session.sourceBytes);
                    inputBytes = std::max(inputBytes, session.inputBytes);
                }
                beforeBytes = static_cast<qint64>(
                    sourceBytes > 0u ? sourceBytes : inputBytes);
            }
            auto generatedAtUtc = toUtf8StdString(
                QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
            const auto firstEncodeSession = std::find_if(
                sessions.begin(),
                sessions.end(),
                [](const auto& session) {
                    return session.runKind == ::datacodec::TelemetryRunKind::Encode &&
                        !session.generatedAtUtc.empty();
                });
            if (firstEncodeSession != sessions.end()) {
                generatedAtUtc = firstEncodeSession->generatedAtUtc;
            }
            ::datacodec::DataCodecProcessNode encodeProcess{
                .name = "EncodeData",
                .success = encodeSucceeded,
                .elapsedMs = static_cast<double>(encodeElapsedMs),
                .inputBytes = static_cast<std::uint64_t>(std::max<qint64>(beforeBytes, 0)),
                .outputBytes = static_cast<std::uint64_t>(std::max<qint64>(totalWrittenBytes, 0)),
                .children = ::datacodec::BuildTelemetryProcessNodes(
                    sessions,
                    ::datacodec::TelemetryRunKind::Encode),
            };
            ::datacodec::CompleteDataCodecProcessNodeMemory(encodeProcess);
            ::datacodec::DataCodecProcessReport processReport{
                .operation = ::datacodec::TelemetryRunKind::Encode,
                .generatedAtUtc = generatedAtUtc,
                .objectName = QFileInfo(writtenPath).fileName().toUtf8().toStdString(),
                .success = reportSuccess,
                .elapsedMs = static_cast<double>(encodeElapsedMs),
                .inputBytes = static_cast<std::uint64_t>(std::max<qint64>(beforeBytes, 0)),
                .outputBytes = static_cast<std::uint64_t>(std::max<qint64>(totalWrittenBytes, 0)),
                .summaryNote = compressionRatioNote,
                .details = {
                    {"outputPath", toUtf8StdString(writtenPath)},
                    {"compressor", toUtf8StdString(compressorName)},
                },
                .configuration = reportConfiguration,
            };
            processReport.processes.push_back(std::move(encodeProcess));
            for (auto& process : additionalProcesses) {
                processReport.elapsedMs += process.elapsedMs;
                processReport.processes.push_back(std::move(process));
            }
            ::datacodec::CompleteDataCodecProcessReportMemory(processReport);
            std::optional<::datacodec::DataCodecErrorReport> errorReport;
            if (!reportSuccess) {
                auto builtErrorReport = ::datacodec::BuildDataCodecErrorReport(
                    ::datacodec::TelemetryRunKind::Encode,
                    generatedAtUtc,
                    processReport.objectName,
                    sessions,
                    messages,
                    std::move(fallbackError));
                for (auto& error : builtErrorReport.errors) {
                    if (error.code == "encode.verification.decode") {
                        error.phasePath = {"Encode", "Verification", "DecodeOutput"};
                    } else if (error.code == "encode.verification.precision") {
                        error.phasePath = {"Encode", "Verification", "PrecisionCompare"};
                    }
                }
                errorReport = std::move(builtErrorReport);
            }
            return writeDataCodecRunReports(
                reportSink,
                reportFileTimestamp,
                processReport,
                errorReport);
        };

        try {
            QElapsedTimer encodeTimer;
            encodeTimer.start();
            const bool ok = writer->WriteToFile(dataObject, toUtf8StdString(outputPath));
            encodeElapsedMs = encodeTimer.elapsed();

            const auto writerPaths = writer->GetWrittenFilePaths();
            bool allOutputsValid = !writerPaths.empty();
            for (const auto& writerOutputPath: writerPaths) {
                const QFileInfo info(QString::fromUtf8(writerOutputPath.c_str()));
                if (!info.isFile() || info.size() <= 0) {
                    allOutputsValid = false;
                    break;
                }
                writtenPaths.push_back(info.filePath());
                totalWrittenBytes += info.size();
            }
            writtenPath = writtenPaths.isEmpty()
                ? resolveWrittenDataCodecPath(outputPath)
                : writtenPaths.front();
            const QFileInfo writtenInfo(writtenPath);
            encodeSucceeded = ok && allOutputsValid &&
                writtenInfo.isFile() && writtenInfo.size() > 0;

            if (!encodeSucceeded) {
                const auto recordMessages = telemetryCapture.SnapshotMessages();
                const auto reportOutcome = writeEncodeReports(
                    false,
                    {},
                    {},
                    "encoding did not produce a valid output file");
                QStringList failureMessages;
                for (const auto& message : recordMessages) {
                    if (message.severity != ::datacodec::TelemetryMessageSeverity::Error ||
                        message.text.empty()) {
                        continue;
                    }
                    failureMessages.push_back(dataCodecHostLogText(
                        logLanguage,
                        iGame::iGameDataCodecHostMessageId::CompressionFailedWithDetail,
                        {{"detail", message.text}}));
                }
                failureMessages.removeDuplicates();
                if (failureMessages.isEmpty()) {
                    failureMessages.push_back(dataCodecHostLogText(
                        logLanguage,
                        iGame::iGameDataCodecHostMessageId::CompressionNoOutputFile));
                }
                if (!reportOutcome.success) {
                    failureMessages.push_back(dataCodecHostLogText(
                        logLanguage,
                        iGame::iGameDataCodecHostMessageId::WriteReportFailed));
                }
                complete(
                    std::move(failureMessages),
                    ::datacodec::DataCodecStatusSeverity::Error);
                return;
            }

            QStringList messages;
            if (outputPerformance) {
                const qint64 afterBytes = totalWrittenBytes;
                const QString compressionRatio = formatCompressionRatio(beforeBytes, afterBytes);
                QString decodeError;
                QElapsedTimer decodeTimer;
                decodeTimer.start();
                auto decodedObject = readDataCodecOutput(
                    writtenInfo.filePath(),
                    &decodeError,
                    telemetryCapture.Sink());
                const qint64 decodeElapsedMs = decodeTimer.elapsed();
                const auto remapOrders = telemetryCapture.TakeRemapOrders();
                ::datacodec::log::AdapterSignatureOrderSet orderSet;
                orderSet.pointOrders = remapOrders.pointOrders;
                orderSet.cellOrders = remapOrders.cellOrders;
                QElapsedTimer precisionTimer;
                precisionTimer.start();
                auto precisionReports = buildPrecisionReports(
                    precisionFields,
                    dataObject,
                    decodedObject,
                    orderSet);
                const qint64 precisionElapsedMs = precisionTimer.elapsed();
                if (!decodeError.isEmpty()) {
                    for (auto& report : precisionReports) {
                        report.ok = false;
                        report.error = decodeError;
                    }
                }
                const QStringList precisionStatusLines = buildPrecisionStatusLines(precisionReports);
                const auto failedPrecisionCount = static_cast<std::size_t>(std::count_if(
                    precisionReports.begin(),
                    precisionReports.end(),
                    [](const auto& report) { return !report.ok; }));
                std::uint64_t checkedValueCount = 0u;
                double maxAbsoluteError = 0.0;
                double maxRmsAbsoluteError = 0.0;
                double maxPointRelativeError = 0.0;
                double maxRangeRelativeError = 0.0;
                bool hasRangeRelativeError = false;
                for (const auto& report : precisionReports) {
                    if (report.ok) {
                        const auto elementCount = static_cast<std::uint64_t>(report.elementCount);
                        const auto componentCount = static_cast<std::uint64_t>(
                            std::max(report.componentCount, 0));
                        const auto maximumCount = std::numeric_limits<std::uint64_t>::max();
                        const auto fieldValueCount = componentCount > 0u &&
                                elementCount <= maximumCount / componentCount
                            ? elementCount * componentCount
                            : maximumCount;
                        checkedValueCount = fieldValueCount <= maximumCount - checkedValueCount
                            ? checkedValueCount + fieldValueCount
                            : maximumCount;
                        maxAbsoluteError = std::max(
                            maxAbsoluteError,
                            report.maxAbsError);
                        maxRmsAbsoluteError = std::max(
                            maxRmsAbsoluteError,
                            report.rmsAbsError);
                        maxPointRelativeError = std::max(
                            maxPointRelativeError,
                            report.maxPointRelativeError);
                        if (report.hasRangeRelativeError) {
                            hasRangeRelativeError = true;
                            maxRangeRelativeError = std::max(
                                maxRangeRelativeError,
                                report.rangeRelativeError);
                        }
                    }
                }
                const bool decodeSucceeded = decodedObject != nullptr && decodeError.isEmpty();
                const bool precisionSucceeded = failedPrecisionCount == 0u;
                ::datacodec::DataCodecProcessNode decodeProcess{
                    .name = "DecodeOutput",
                    .success = decodeSucceeded,
                    .elapsedMs = static_cast<double>(decodeElapsedMs),
                    .inputBytes = static_cast<std::uint64_t>(std::max<qint64>(afterBytes, 0)),
                    .children = ::datacodec::BuildTelemetryProcessNodes(
                        telemetryCapture.SnapshotCompletedTelemetrySessions(),
                        ::datacodec::TelemetryRunKind::Decode),
                };
                ::datacodec::CompleteDataCodecProcessNodeMemory(decodeProcess);
                ::datacodec::DataCodecProcessNode precisionProcess{
                    .name = "PrecisionCompare",
                    .success = precisionSucceeded,
                    .elapsedMs = static_cast<double>(precisionElapsedMs),
                    .details = {
                        {"fieldCount", static_cast<std::uint64_t>(precisionReports.size())},
                        {"failedFieldCount", static_cast<std::uint64_t>(failedPrecisionCount)},
                        {"checkedValueCount", checkedValueCount},
                        {"maxAbsoluteError", maxAbsoluteError},
                        {"maxRmsAbsoluteError", maxRmsAbsoluteError},
                        {"maxPointRelativeError", maxPointRelativeError},
                        {"hasRangeRelativeError", hasRangeRelativeError},
                        {"maxRangeRelativeError", maxRangeRelativeError},
                    },
                };
                ::datacodec::DataCodecProcessNode verificationProcess{
                    .name = "Verification",
                    .success = decodeSucceeded && precisionSucceeded,
                    .elapsedMs = static_cast<double>(decodeElapsedMs + precisionElapsedMs),
                    .children = {
                        std::move(decodeProcess),
                        std::move(precisionProcess),
                    },
                };
                ::datacodec::CompleteDataCodecProcessNodeMemory(verificationProcess);

                std::vector<::datacodec::TelemetryMessageRecord> verificationErrors;
                if (!decodeSucceeded) {
                    verificationErrors.push_back({
                        .severity = ::datacodec::TelemetryMessageSeverity::Error,
                        .origin = "EncodeVerification",
                        .code = "encode.verification.decode",
                        .language = logLanguage,
                        .text = "编码输出验证解码失败",
                        .technicalDetail = toUtf8StdString(decodeError),
                    });
                }
                if (!precisionSucceeded) {
                    const auto firstFailure = std::find_if(
                        precisionReports.begin(),
                        precisionReports.end(),
                        [](const auto& report) { return !report.ok; });
                    std::string detail = "failedFieldCount=" +
                        std::to_string(failedPrecisionCount);
                    if (firstFailure != precisionReports.end()) {
                        detail += "; firstField=" + toUtf8StdString(firstFailure->fieldName) +
                            "; reason=" + toUtf8StdString(firstFailure->error);
                    }
                    verificationErrors.push_back({
                        .severity = ::datacodec::TelemetryMessageSeverity::Error,
                        .origin = "EncodeVerification",
                        .code = "encode.verification.precision",
                        .language = logLanguage,
                        .text = "编码输出精度验证失败",
                        .technicalDetail = std::move(detail),
                    });
                }
                const auto reportOutcome = writeEncodeReports(
                    verificationProcess.success,
                    {std::move(verificationProcess)},
                    std::move(verificationErrors),
                    "encoding verification failed without a detailed error record");
                if (!reportOutcome.success) {
                    complete(
                        {dataCodecHostLogText(
                            logLanguage,
                            iGame::iGameDataCodecHostMessageId::WriteReportFailed)},
                        ::datacodec::DataCodecStatusSeverity::Error);
                    return;
                }

                messages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::CompressionReportBegin));
                messages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::SizeBeforeCompression,
                    {{"size", toUtf8StdString(formatByteSize(beforeBytes))}}));
                messages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::SizeAfterCompression,
                    {{"size", toUtf8StdString(formatByteSize(afterBytes))}}));
                messages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::CompressedSizeRatio,
                    {{"ratio", toUtf8StdString(compressionRatio)}}));
                messages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::CompressionTime,
                    {{"milliseconds", std::to_string(encodeElapsedMs)}}));
                messages.append(precisionStatusLines);
                for (const QString& path : reportOutcome.writtenPaths) {
                    messages.push_back(dataCodecHostLogText(
                        logLanguage,
                        iGame::iGameDataCodecHostMessageId::TelemetryDataPath,
                        {{"path", toUtf8StdString(path)}}));
                }
                messages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::CompressionReportEnd));
            }
            if (writtenPaths.size() > 1) {
                messages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::SequenceOutputCount,
                    {{"count", std::to_string(writtenPaths.size())}}));
                messages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::FirstFrameFile,
                    {{"path", toUtf8StdString(writtenPaths.front())}}));
                messages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::LastFrameFile,
                    {{"path", toUtf8StdString(writtenPaths.back())}}));
            }
            complete(std::move(messages));
        } catch (const std::exception& exception) {
            std::vector<::datacodec::TelemetryMessageRecord> errors{
                {
                    .severity = ::datacodec::TelemetryMessageSeverity::Critical,
                    .origin = "EncodeWorker",
                    .code = "encode.worker.exception",
                    .language = logLanguage,
                    .text = "编码任务发生未处理异常",
                    .technicalDetail = exception.what(),
                },
            };
            const auto reportOutcome = writeEncodeReports(
                false,
                {},
                std::move(errors),
                exception.what());
            QStringList failureMessages{
                dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::CompressionFailedWithDetail,
                    {{"detail", exception.what()}}),
            };
            if (!reportOutcome.success) {
                failureMessages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::WriteReportFailed));
            }
            complete(
                std::move(failureMessages),
                ::datacodec::DataCodecStatusSeverity::Error);
        } catch (...) {
            std::vector<::datacodec::TelemetryMessageRecord> errors{
                {
                    .severity = ::datacodec::TelemetryMessageSeverity::Critical,
                    .origin = "EncodeWorker",
                    .code = "encode.worker.unknown-exception",
                    .language = logLanguage,
                    .text = "编码任务发生未知异常",
                },
            };
            const auto reportOutcome = writeEncodeReports(
                false,
                {},
                std::move(errors),
                "encoding failed with an unknown exception");
            QStringList failureMessages{
                dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::CompressionFailedWithDetail,
                    {{"detail", "unknown exception"}}),
            };
            if (!reportOutcome.success) {
                failureMessages.push_back(dataCodecHostLogText(
                    logLanguage,
                    iGame::iGameDataCodecHostMessageId::WriteReportFailed));
            }
            complete(
                std::move(failureMessages),
                ::datacodec::DataCodecStatusSeverity::Error);
        }
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void igQtDataCodecCompressionWidget::computeFeature() {
    if (!hasNumericFields() || hasMultiFrameData() || m_featureComputationActive) return;
    auto* fieldState = currentFieldState();
    auto* featureState = currentFeatureState();
    if (fieldState == nullptr || featureState == nullptr) return;
    const int fieldIndex = m_selectedFieldIndex;
    const int basisIndex = qBound(0, fieldState->currentBasisIndex, 2);
    const NumericFieldItem field = m_fields[fieldIndex];
    auto dataObject = field.sourceObject;
    if (dataObject == nullptr) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::FeatureComputationNoData),
            ::datacodec::DataCodecStatusSeverity::Error);
        return;
    }

    iGame::RegionFeatureBasisFilter::BasisMode basisMode =
        iGame::RegionFeatureBasisFilter::BasisMode::Magnitude;
    switch (basisIndex) {
    case 1:
        basisMode = iGame::RegionFeatureBasisFilter::BasisMode::LocalStdDev;
        break;
    case 2:
        basisMode = iGame::RegionFeatureBasisFilter::BasisMode::Jump;
        break;
    default:
        break;
    }

    const IGsize elementCount = numericFieldElementCount(fieldIndex);
    publishStatus(dataCodecHostLogText(
        m_dataCodecLanguage,
        iGame::iGameDataCodecHostMessageId::FeatureComputationStarted,
        {
            {"field", toUtf8StdString(field.name)},
            {"basis", toUtf8StdString(currentBasisName())},
            {"count", std::to_string(elementCount)},
        }));
    m_featureComputationActive = true;
    const std::uint64_t requestSerial = ++m_featureRequestSerial;
    if (m_computeButton != nullptr) m_computeButton->setText(QStringLiteral("计算中..."));
    refreshFeatureState();

    QPointer<igQtDataCodecCompressionWidget> self(this);
    auto* worker = QThread::create([
        self,
        dataObject,
        fieldIndex,
        basisIndex,
        basisMode,
        fieldName = toUtf8StdString(field.name),
        attachmentType = field.attachmentType,
        requestSerial]() mutable {
        iGame::RegionFeatureBasisData::Pointer output;
        FeatureAnalysis analysis;
        QString error;

        auto filter = iGame::RegionFeatureBasisFilter::New();
        filter->SetInput(dataObject);
        filter->SetHistogramBinCount(0);
        filter->SetBasisMode(basisMode);
        filter->SetAttributeByName(fieldName, attachmentType, -1);
        if (!filter->Execute()) {
            error = localizedFeatureErrorMessage(QString::fromStdString(filter->GetMessage()));
        } else {
            output = filter->GetFeatureBasisData();
            if (output == nullptr || !output->HasValues()) {
                error = QStringLiteral("未生成特征值");
                output = nullptr;
            } else {
                analysis = buildFeatureAnalysis(output);
            }
        }

        if (!self) return;
        QMetaObject::invokeMethod(self, [
            self,
            fieldIndex,
            basisIndex,
            requestSerial,
            output,
            analysis = std::move(analysis),
            error = std::move(error)]() mutable {
            if (!self || requestSerial != self->m_featureRequestSerial) return;
            self->m_featureComputationActive = false;
            if (self->m_computeButton != nullptr) {
                self->m_computeButton->setText(QStringLiteral("计算特征"));
            }
            if (fieldIndex < 0 || fieldIndex >= self->m_fieldStates.size()) {
                self->refreshFeatureState();
                return;
            }

            FieldState& targetField = self->m_fieldStates[fieldIndex];
            self->ensureFeatureStates(targetField);
            if (basisIndex < 0 || basisIndex >= targetField.features.size()) {
                self->refreshFeatureState();
                return;
            }
            FeatureState& targetFeature = targetField.features[basisIndex];
            if (!error.isEmpty()) {
                targetFeature.featureBasis = nullptr;
                targetFeature.analysis = {};
                targetFeature.basisComputed = false;
                self->publishStatus(
                    dataCodecHostLogText(
                        self->m_dataCodecLanguage,
                        iGame::iGameDataCodecHostMessageId::FeatureComputationFailed,
                        {{"detail", toUtf8StdString(error)}}),
                    ::datacodec::DataCodecStatusSeverity::Error);
            } else {
                for (int index = 0; index < targetField.features.size(); ++index) {
                    if (index == basisIndex) continue;
                    targetField.features[index].featureBasis = nullptr;
                    targetField.features[index].analysis = {};
                    targetField.features[index].basisComputed = false;
                }
                targetFeature.featureBasis = output;
                targetFeature.analysis = std::move(analysis);
                targetFeature.basisComputed = true;
                for (RegionItem& region : targetFeature.regions) {
                    if (region.id == 0) continue;
                    region.elementCount = self->featureSelectionCountForRange(
                        targetFeature,
                        region.rangeLower,
                        region.rangeUpper);
                }
                self->publishStatus(
                    dataCodecHostLogText(
                        self->m_dataCodecLanguage,
                        iGame::iGameDataCodecHostMessageId::FeatureComputationCompleted));
            }

            self->refreshRegionRows();
            self->refreshFieldItemLabels();
            self->refreshSelectedRegion();
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void igQtDataCodecCompressionWidget::addRegion() {
    if (!hasNumericFields()) return;
    auto* fieldState = currentFieldState();
    auto* featureState = currentFeatureState();
    auto* regions = currentRegions();
    QStringList reasons;
    if (hasMultiFrameData()) {
        reasons.push_back(dataCodecHostLogText(
            m_dataCodecLanguage,
            iGame::iGameDataCodecHostMessageId::TimeSeriesCustomRegionUnsupported));
    } else {
        if (fieldState == nullptr || !fieldState->selected) {
            reasons.push_back(dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::SelectCurrentField));
        }
        if (fieldState != nullptr && fieldState->losslessMode) {
            reasons.push_back(dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::DisableLosslessCompression));
        }
        if (m_featureComputationActive) {
            reasons.push_back(dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::WaitUntilFeatureComputationCompleted));
        } else if (featureState == nullptr || !featureState->basisComputed ||
                   featureState->featureBasis == nullptr) {
            reasons.push_back(dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::ComputeCurrentFeature));
        }
        if (featureState != nullptr && customRegionCount(*featureState) >= kMaxCustomRegionCount) {
            reasons.push_back(dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::CustomRegionLimitReached));
        }
    }
    const QString compressorName = QStringLiteral("SZ3");
    if (regions == nullptr) {
        reasons.push_back(dataCodecHostLogText(
            m_dataCodecLanguage,
            iGame::iGameDataCodecHostMessageId::CustomRegionStateUnavailable));
    }
    if (!reasons.isEmpty()) {
        const auto separator = m_dataCodecLanguage ==
                ::datacodec::DataCodecLanguage::English
            ? QStringLiteral("; ")
            : QStringLiteral("；");
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::CannotAddCustomRegion,
                {{"reason", toUtf8StdString(reasons.join(separator))}}),
            ::datacodec::DataCodecStatusSeverity::Warning);
        return;
    }

    int desiredSpan = kDefaultCustomRegionSpan;
    if (featureState->selectedRegionId != 0) {
        desiredSpan = qBound(
            0,
            qAbs(featureState->rangeUpper - featureState->rangeLower),
            100);
    }
    QVector<QPair<int, int>> occupiedRanges;
    occupiedRanges.reserve(customRegionCount(*featureState));
    for (const RegionItem& region : featureState->regions) {
        if (region.id == 0) continue;
        occupiedRanges.push_back({
            qBound(0, qMin(region.rangeLower, region.rangeUpper), 100),
            qBound(0, qMax(region.rangeLower, region.rangeUpper), 100),
        });
    }
    std::sort(
        occupiedRanges.begin(),
        occupiedRanges.end(),
        [](const QPair<int, int>& left, const QPair<int, int>& right) {
            if (left.first != right.first) return left.first < right.first;
            return left.second < right.second;
        });

    std::optional<QPair<int, int>> availableRange;
    int cursor = 0;
    for (const auto& occupied : occupiedRanges) {
        if (cursor < occupied.first) {
            availableRange = QPair<int, int>{
                cursor,
                qMin(occupied.first - 1, cursor + desiredSpan),
            };
            break;
        }
        cursor = qMax(cursor, occupied.second + 1);
        if (cursor > 100) break;
    }
    if (!availableRange.has_value() && cursor <= 100) {
        availableRange = QPair<int, int>{cursor, qMin(100, cursor + desiredSpan)};
    }
    if (!availableRange.has_value()) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::CustomRegionRangeUnavailable),
            ::datacodec::DataCodecStatusSeverity::Warning);
        return;
    }
    featureState->rangeLower = availableRange->first;
    featureState->rangeUpper = availableRange->second;

    int nextId = 1;
    for (const RegionItem& region : *regions) nextId = qMax(nextId, region.id + 1);
    const int overlap = currentSelectionOverlapCount();
    const int selectedCount = featureSelectionCountForRange(*featureState, featureState->rangeLower, featureState->rangeUpper);
    const double absPrecision = losslessModeEnabled() ? 0.0 : 0.0008;
    const double relPrecision = losslessModeEnabled() ? 0.0 : 0.0004;
    const int precisionMode = currentFieldState() != nullptr
                                      ? effectivePrecisionMode(currentFieldState()->defaultPrecisionMode)
                                      : kPrecisionModeAbs;
    regions->push_back(RegionItem{nextId, QStringLiteral("区域 %1").arg(nextId), QString(),
                                  absPrecision, relPrecision, precisionMode, selectedCount,
                                  featureState->rangeLower, featureState->rangeUpper});
    if (!regions->isEmpty()) {
        RegionItem& region = regions->back();
        region.ruleSummary = QString();
    }
    if (overlap > 0) {
        publishStatus(
            dataCodecHostLogText(
                m_dataCodecLanguage,
                iGame::iGameDataCodecHostMessageId::CustomRegionOverlap,
                {{"count", std::to_string(overlap)}}),
            ::datacodec::DataCodecStatusSeverity::Warning);
    }
    selectRegion(nextId);
    refreshFieldItemLabels();
    refreshBasisOptions();
}

void igQtDataCodecCompressionWidget::selectRegion(int regionId) {
    if (hasMultiFrameData() && regionId != 0) return;
    auto* featureState = currentFeatureState();
    if (featureState == nullptr) return;
    featureState->selectedRegionId = regionId;
    refreshSelectedRegion();
    refreshRegionRows();
}

void igQtDataCodecCompressionWidget::onFieldChanged(int row) {
    if (row < 0 || row >= m_fields.size()) return;
    if (row == m_selectedFieldIndex) return;
    m_selectedFieldIndex = row;
    auto* fieldState = currentFieldState();
    if (fieldState != nullptr) {
        ensureFeatureStates(*fieldState);
    }
    refreshBasisOptions();
    refreshRegionRows();
    refreshSelectedRegion();
    refreshFeatureState();
    refreshFieldItemLabels();
}

void igQtDataCodecCompressionWidget::onBasisChanged(int index) {
    if (index < 0 || hasMultiFrameData()) return;
    auto* fieldState = currentFieldState();
    if (fieldState == nullptr) return;
    fieldState->currentBasisIndex = qBound(0, index, 2);
    ensureFeatureStates(*fieldState);
    refreshBasisOptions();
    refreshRegionRows();
    refreshSelectedRegion();
    refreshFeatureState();
    refreshFieldItemLabels();
}

void igQtDataCodecCompressionWidget::onRangeChanged() {
    if (!m_rangeSlider || hasMultiFrameData()) return;
    const int lower = m_rangeSlider->lowerValue();
    const int upper = m_rangeSlider->upperValue();
    auto* featureState = currentFeatureState();
    if (featureState != nullptr) {
        featureState->rangeLower = lower;
        featureState->rangeUpper = upper;
    }
    if (m_histogram) m_histogram->setRange(lower, upper);
    if (m_rangeSlider->isDragging()) {
        refreshStats();
        return;
    }
    commitRangeSelection();
}

void igQtDataCodecCompressionWidget::commitRangeSelection() {
    if (!m_rangeSlider || hasMultiFrameData()) return;
    auto* featureState = currentFeatureState();
    if (featureState != nullptr && featureState->basisComputed && featureState->featureBasis != nullptr &&
        featureState->selectedRegionId != 0) {
        const int index = selectedRegionIndex();
        auto* regions = currentRegions();
        if (regions != nullptr && index >= 0 && index < regions->size()) {
            (*regions)[index].rangeLower = featureState->rangeLower;
            (*regions)[index].rangeUpper = featureState->rangeUpper;
            (*regions)[index].elementCount =
                    featureSelectionCountForRange(*featureState, featureState->rangeLower, featureState->rangeUpper);
            (*regions)[index].ruleSummary = QString();
            refreshRegionRows();
        }
    }
    refreshFeatureState();
    refreshFieldItemLabels();
}

void igQtDataCodecCompressionWidget::deleteRegion(int regionId) {
    if (regionId == 0 || hasMultiFrameData()) return;
    auto* featureState = currentFeatureState();
    auto* regions = currentRegions();
    if (featureState == nullptr || regions == nullptr) return;

    for (int i = 0; i < regions->size(); ++i) {
        if ((*regions)[i].id != regionId) continue;
        regions->removeAt(i);
        if (featureState->selectedRegionId == regionId) {
            featureState->selectedRegionId = 0;
        }
        refreshSelectedRegion();
        refreshRegionRows();
        refreshFeatureState();
        refreshFieldItemLabels();
        refreshBasisOptions();
        return;
    }
}

void igQtDataCodecCompressionWidget::syncSelectedPrecisionMode() {
    const int index = selectedRegionIndex();
    if (index < 0 || !m_precisionModeCombo) return;
    if (losslessModeEnabled()) return;
    const auto* regions = currentRegions();
    if (regions == nullptr || index >= regions->size()) return;
    const int regionId = (*regions)[index].id;
    const int mode = effectivePrecisionMode(m_precisionModeCombo->currentData().toInt());
    updateRegionPrecisionMode(regionId, mode);
    const auto* updatedRegions = currentRegions();
    if (updatedRegions == nullptr || index >= updatedRegions->size()) return;
    if (m_precisionSpin) {
        QSignalBlocker blocker(m_precisionSpin);
        const double value = precisionValueForMode((*updatedRegions)[index], mode);
        m_precisionSpin->setValue(value);
        applyPrecisionSpinTone(m_precisionSpin, m_selectedFieldIndex, mode, value, m_precisionSpin->isEnabled());
    }
    refreshRegionRows();
    refreshFieldItemLabels();
}

void igQtDataCodecCompressionWidget::syncSelectedPrecision() {
    const int index = selectedRegionIndex();
    if (index < 0 || !m_precisionSpin || !m_precisionModeCombo) return;
    if (losslessModeEnabled()) return;
    const auto* regions = currentRegions();
    if (regions == nullptr || index >= regions->size()) return;
    const int regionId = (*regions)[index].id;
    const int mode = effectivePrecisionMode(m_precisionModeCombo->currentData().toInt());
    const double value = m_precisionSpin->value();
    updateRegionPrecisionValue(regionId, mode, value);
    applyPrecisionSpinTone(m_precisionSpin, m_selectedFieldIndex, mode, value, m_precisionSpin->isEnabled());
    refreshRegionRows();
    refreshFieldItemLabels();
}

void igQtDataCodecCompressionWidget::applyLosslessMode(bool enabled) {
    auto* fieldState = currentFieldState();
    if (fieldState == nullptr) return;
    fieldState->losslessMode = enabled;
    syncDefaultRegionToFeatures(*fieldState);
    refreshSelectedRegion();
    refreshRegionRows();
    refreshFieldItemLabels();
}

void igQtDataCodecCompressionWidget::applyLosslessModeToAllFields() {
    if (!hasNumericFields()) return;
    ensureFieldStates();
    for (FieldState& state : m_fieldStates) {
        state.losslessMode = true;
        syncDefaultRegionToFeatures(state);
    }
    publishStatus(dataCodecHostLogText(
        m_dataCodecLanguage,
        iGame::iGameDataCodecHostMessageId::AllFieldsLossless));
    refreshSelectedRegion();
    refreshRegionRows();
    refreshFieldItemLabels();
    refreshFeatureState();
}

void igQtDataCodecCompressionWidget::applyHighPrecisionLossyModeToAllFields() {
    if (!hasNumericFields()) return;
    ensureFieldStates();
    for (FieldState& state : m_fieldStates) {
        state.losslessMode = false;
        state.defaultPrecisionMode = kPrecisionModeRel;
        state.defaultRelPrecision = kDefaultGreenPrecisionRatio;
        ensureFeatureStates(state);
        for (FeatureState& feature : state.features) {
            for (RegionItem& region : feature.regions) {
                region.precisionMode = kPrecisionModeRel;
                region.relPrecision = kDefaultGreenPrecisionRatio;
            }
        }
    }
    publishStatus(dataCodecHostLogText(
        m_dataCodecLanguage,
        iGame::iGameDataCodecHostMessageId::AllFieldsHighPrecisionLossy));
    refreshSelectedRegion();
    refreshRegionRows();
    refreshFieldItemLabels();
    refreshFeatureState();
}

void igQtDataCodecCompressionWidget::syncCurrentDefaultPrecisionToAllFields() {
    if (!hasNumericFields()) return;
    ensureFieldStates();
    auto* sourceState = currentFieldState();
    if (sourceState == nullptr) return;

    const int mode = effectivePrecisionMode(sourceState->defaultPrecisionMode);
    const double absPrecision = sourceState->defaultAbsPrecision;
    const double relPrecision = sourceState->defaultRelPrecision;
    for (FieldState& state : m_fieldStates) {
        state.losslessMode = false;
        state.defaultPrecisionMode = mode;
        state.defaultAbsPrecision = absPrecision;
        state.defaultRelPrecision = relPrecision;
        ensureFeatureStates(state);
        for (FeatureState& feature : state.features) {
            for (RegionItem& region : feature.regions) {
                region.precisionMode = mode;
            }
        }
        syncDefaultRegionToFeatures(state);
    }
    publishStatus(dataCodecHostLogText(
        m_dataCodecLanguage,
        iGame::iGameDataCodecHostMessageId::DefaultPrecisionApplied));
    refreshSelectedRegion();
    refreshRegionRows();
    refreshFieldItemLabels();
    refreshFeatureState();
}

bool igQtDataCodecCompressionWidget::losslessModeEnabled() const {
    const auto* fieldState = currentFieldState();
    return fieldState != nullptr && fieldState->losslessMode;
}

int igQtDataCodecCompressionWidget::featureSelectionCountForRange(
        const FeatureState& featureState,
        int lower,
        int upper) const {
    const auto count = featureSelectionCountForRange64(featureState, lower, upper);
    return static_cast<int>(std::min<std::uint64_t>(
        count,
        static_cast<std::uint64_t>(std::numeric_limits<int>::max())));
}

std::uint64_t igQtDataCodecCompressionWidget::featureSelectionCountForRange64(
        const FeatureState& featureState,
        int lower,
        int upper) const {
    if (!featureState.basisComputed || featureState.featureBasis == nullptr) return 0u;
    const auto& analysis = featureState.analysis.ready
        ? featureState.analysis
        : (featureState.analysis = buildFeatureAnalysis(featureState.featureBasis));
    if (!analysis.ready) return 0u;
    const int boundedLower = qBound(0, qMin(lower, upper), 100);
    const int boundedUpper = qBound(0, qMax(lower, upper), 100);
    const std::uint64_t excluded =
        analysis.lessThanEndpointCounts[static_cast<std::size_t>(boundedLower)] +
        analysis.greaterThanEndpointCounts[static_cast<std::size_t>(boundedUpper)];
    return excluded >= analysis.effectiveValueCount
        ? 0u
        : analysis.effectiveValueCount - excluded;
}

QVector<double> igQtDataCodecCompressionWidget::currentOccupiedHistogramBins(const HistogramView& view) const {
    QVector<double> bins(qMax(1, view.bins.size()), 0.0);
    const auto* featureState = currentFeatureState();
    const auto* regions = currentRegions();
    if (featureState == nullptr || featureState->featureBasis == nullptr || regions == nullptr || view.bins.isEmpty()) {
        return bins;
    }

    const int ignoredRegionId = featureState->selectedRegionId;
    const bool showOverlap = ignoredRegionId != 0;
    const int selectedLower = qMin(featureState->rangeLower, featureState->rangeUpper);
    const int selectedUpper = qMax(featureState->rangeLower, featureState->rangeUpper);
    for (const RegionItem& region : *regions) {
        if (region.id == 0 || region.id == ignoredRegionId) continue;
        const int regionLower = qMin(region.rangeLower, region.rangeUpper);
        const int regionUpper = qMax(region.rangeLower, region.rangeUpper);
        const int rangeOverlapLower = qMax(selectedLower, regionLower);
        const int rangeOverlapUpper = qMin(selectedUpper, regionUpper);
        const bool rangesOverlap =
            showOverlap &&
            rangeOverlapLower <= rangeOverlapUpper &&
            featureSelectionCountForRange64(*featureState, rangeOverlapLower, rangeOverlapUpper) > 0u;
        for (int bin = 0; bin < bins.size(); ++bin) {
            const double binLower = static_cast<double>(bin) * 100.0 / static_cast<double>(bins.size());
            const double binUpper = static_cast<double>(bin + 1) * 100.0 / static_cast<double>(bins.size());
            const int overlapLower = qMax(regionLower, qBound(0, static_cast<int>(std::floor(binLower)), 100));
            const int overlapUpper = qMin(regionUpper, qBound(0, static_cast<int>(std::ceil(binUpper)), 100));
            if (overlapLower > overlapUpper) continue;
            bins[bin] = qMax(bins[bin], 1.0);
            if (!rangesOverlap) continue;
            const int binOverlapLower = qMax(
                rangeOverlapLower,
                qBound(0, static_cast<int>(std::floor(binLower)), 100));
            const int binOverlapUpper = qMin(
                rangeOverlapUpper,
                qBound(0, static_cast<int>(std::ceil(binUpper)), 100));
            if (binOverlapLower <= binOverlapUpper) bins[bin] = 2.0;
        }
    }
    return bins;
}

int igQtDataCodecCompressionWidget::currentSelectionOverlapCount(int ignoredRegionId) const {
    const auto* featureState = currentFeatureState();
    if (featureState == nullptr) return 0;
    return selectionOverlapCountForRange(*featureState, featureState->rangeLower, featureState->rangeUpper, ignoredRegionId);
}

int igQtDataCodecCompressionWidget::selectionOverlapCountForRange(
        const FeatureState& featureState,
        int lower,
        int upper,
        int ignoredRegionId) const {
    if (!featureState.basisComputed || featureState.featureBasis == nullptr) {
        return 0;
    }

    const int rangeLower = qBound(0, qMin(lower, upper), 100);
    const int rangeUpper = qBound(0, qMax(lower, upper), 100);
    QVector<QPair<int, int>> intersections;
    for (const RegionItem& region : featureState.regions) {
        if (region.id == 0 || region.id == ignoredRegionId) continue;
        const int overlapLower = qMax(rangeLower, qMin(region.rangeLower, region.rangeUpper));
        const int overlapUpper = qMin(rangeUpper, qMax(region.rangeLower, region.rangeUpper));
        if (overlapLower > overlapUpper) continue;
        if (featureSelectionCountForRange64(featureState, overlapLower, overlapUpper) == 0u) continue;
        intersections.push_back({overlapLower, overlapUpper});
    }
    if (intersections.isEmpty()) return 0;

    std::sort(
        intersections.begin(),
        intersections.end(),
        [](const QPair<int, int>& left, const QPair<int, int>& right) {
            if (left.first != right.first) return left.first < right.first;
            return left.second < right.second;
        });
    std::uint64_t overlap = 0u;
    int mergedLower = intersections.front().first;
    int mergedUpper = intersections.front().second;
    for (int index = 1; index < intersections.size(); ++index) {
        const auto& next = intersections[index];
        if (next.first <= mergedUpper) {
            mergedUpper = qMax(mergedUpper, next.second);
            continue;
        }
        overlap += featureSelectionCountForRange64(featureState, mergedLower, mergedUpper);
        mergedLower = next.first;
        mergedUpper = next.second;
    }
    overlap += featureSelectionCountForRange64(featureState, mergedLower, mergedUpper);
    return static_cast<int>(std::min<std::uint64_t>(
        overlap,
        static_cast<std::uint64_t>(std::numeric_limits<int>::max())));
}

QVector<int> igQtDataCodecCompressionWidget::invalidRegionIdsForFeature(const FeatureState& featureState) const {
    QVector<int> ids;
    for (int leftIndex = 0; leftIndex < featureState.regions.size(); ++leftIndex) {
        const RegionItem& left = featureState.regions[leftIndex];
        if (left.id == 0) continue;
        for (int rightIndex = leftIndex + 1; rightIndex < featureState.regions.size(); ++rightIndex) {
            const RegionItem& right = featureState.regions[rightIndex];
            if (right.id == 0) continue;
            const int overlapLower = qMax(
                qMin(left.rangeLower, left.rangeUpper),
                qMin(right.rangeLower, right.rangeUpper));
            const int overlapUpper = qMin(
                qMax(left.rangeLower, left.rangeUpper),
                qMax(right.rangeLower, right.rangeUpper));
            if (overlapLower > overlapUpper) continue;
            if (!ids.contains(left.id)) ids.push_back(left.id);
            if (!ids.contains(right.id)) ids.push_back(right.id);
        }
    }
    return ids;
}

bool igQtDataCodecCompressionWidget::isRegionInvalid(int regionId) const {
    const auto* featureState = currentFeatureState();
    if (featureState == nullptr || regionId == 0) return false;
    return invalidRegionIdsForFeature(*featureState).contains(regionId);
}

int igQtDataCodecCompressionWidget::invalidActiveRegionCount() const {
    if (hasMultiFrameData()) return 0;
    int count = 0;
    for (int fieldIndex = 0; fieldIndex < m_fieldStates.size(); ++fieldIndex) {
        const FieldState& fieldState = m_fieldStates[fieldIndex];
        if (!fieldState.selected) continue;
        if (fieldState.losslessMode) continue;
        if (fieldState.features.isEmpty()) continue;
        const int featureIndex = qBound(0, fieldState.currentBasisIndex, fieldState.features.size() - 1);
        count += invalidRegionIdsForFeature(fieldState.features[featureIndex]).size();
    }
    return count;
}

int igQtDataCodecCompressionWidget::unavailableActiveRegionFeatureCount() const {
    if (hasMultiFrameData()) return 0;
    int count = 0;
    for (const FieldState& fieldState : m_fieldStates) {
        if (!fieldState.selected || fieldState.losslessMode || fieldState.features.isEmpty()) continue;
        const int featureIndex = qBound(
            0,
            fieldState.currentBasisIndex,
            fieldState.features.size() - 1);
        const FeatureState& featureState = fieldState.features[featureIndex];
        if (customRegionCount(featureState) == 0) continue;
        if (!featureState.basisComputed || featureState.featureBasis == nullptr) ++count;
    }
    return count;
}

int igQtDataCodecCompressionWidget::customRegionCount(const FeatureState& featureState) const {
    return static_cast<int>(std::count_if(
        featureState.regions.begin(),
        featureState.regions.end(),
        [](const RegionItem& region) { return region.id != 0; }));
}

igQtDataCodecCompressionWidget::FeatureAnalysis igQtDataCodecCompressionWidget::buildFeatureAnalysis(
        const iGame::RegionFeatureBasisData::Pointer& featureBasis) {
    FeatureAnalysis analysis;
    if (featureBasis == nullptr) return analysis;

    const std::vector<double>& values = featureBasis->GetValues();
    HistogramView& view = analysis.histogram;
    view.totalCount = static_cast<int>(values.size());
    if (values.empty()) {
        analysis.ready = true;
        return analysis;
    }

    bool haveFiniteValue = false;
    std::size_t finiteCount = 0u;
    view.valueMin = std::numeric_limits<double>::infinity();
    view.valueMax = -std::numeric_limits<double>::infinity();
    for (const double value : values) {
        if (!std::isfinite(value)) continue;
        haveFiniteValue = true;
        ++finiteCount;
        view.valueMin = std::min(view.valueMin, value);
        view.valueMax = std::max(view.valueMax, value);
    }
    if (!haveFiniteValue) {
        view.valueMin = 0.0;
        view.valueMax = 0.0;
        analysis.ready = true;
        return analysis;
    }
    view.bins = buildNormalizedHistogram(values, view.valueMin, view.valueMax, kHistogramBinCount);
    if (view.valueMin != view.valueMax && finiteCount >= 32u) {
        QVector<int> counts(kRawBackgroundDetectBinCount, 0);
        const double range = view.valueMax - view.valueMin;
        for (double value : values) {
            if (!std::isfinite(value)) continue;
            int bin = static_cast<int>((value - view.valueMin) / range * static_cast<double>(counts.size()));
            bin = qBound(0, bin, counts.size() - 1);
            counts[bin] += 1;
        }

        int peakIndex = 0;
        for (int i = 1; i < counts.size(); ++i) {
            if (counts[i] > counts[peakIndex]) peakIndex = i;
        }
        const double peakRatio = static_cast<double>(counts[peakIndex]) / static_cast<double>(finiteCount);
        if (peakRatio >= kBackgroundPeakRatio) {
            const int mergeLimit = qMax(1, static_cast<int>(counts[peakIndex] * kBackgroundMergeRatio));
            int leftIndex = peakIndex;
            int rightIndex = peakIndex;
            while (leftIndex > 0 && counts[leftIndex - 1] >= mergeLimit) --leftIndex;
            while (rightIndex + 1 < counts.size() && counts[rightIndex + 1] >= mergeLimit) ++rightIndex;

            const double binWidth = range / static_cast<double>(counts.size());
            const double backgroundMin =
                view.valueMin + static_cast<double>(leftIndex) * binWidth - binWidth * 0.5;
            const double backgroundMax =
                view.valueMin + static_cast<double>(rightIndex + 1) * binWidth + binWidth * 0.5;

            double effectiveMin = std::numeric_limits<double>::infinity();
            double effectiveMax = -std::numeric_limits<double>::infinity();
            std::size_t effectiveCount = 0u;
            int backgroundCount = 0;
            for (double value : values) {
                if (!std::isfinite(value)) continue;
                if (value >= backgroundMin && value <= backgroundMax) {
                    ++backgroundCount;
                    continue;
                }
                ++effectiveCount;
                effectiveMin = std::min(effectiveMin, value);
                effectiveMax = std::max(effectiveMax, value);
            }

            if (effectiveCount >= 8u &&
                backgroundCount >= static_cast<int>(finiteCount * kBackgroundPeakRatio)) {
                view.smart = true;
                view.backgroundCount = backgroundCount;
                view.backgroundMin = backgroundMin;
                view.backgroundMax = backgroundMax;
                view.valueMin = effectiveMin;
                view.valueMax = effectiveMax;
                QVector<double> effectiveBins(kHistogramBinCount, 0.0);
                for (const double value : values) {
                    if (!std::isfinite(value)) continue;
                    if (value >= backgroundMin && value <= backgroundMax) continue;
                    effectiveBins[histogramBinIndexForValue(
                        value,
                        view.valueMin,
                        view.valueMax,
                        effectiveBins.size())] += 1.0;
                }
                double maxDensity = 0.0;
                for (const double density : effectiveBins) maxDensity = qMax(maxDensity, density);
                if (maxDensity > std::numeric_limits<double>::epsilon()) {
                    for (double& density : effectiveBins) density /= maxDensity;
                }
                view.bins = std::move(effectiveBins);
            }
        }
    }

    std::array<double, kRangeEndpointCount> thresholds{};
    for (int percent = 0; percent < kRangeEndpointCount; ++percent) {
        thresholds[static_cast<std::size_t>(percent)] =
            valueAtPercent(view.valueMin, view.valueMax, percent);
    }
    std::array<std::int64_t, kRangeEndpointCount + 1> lessThanDifference{};
    std::array<std::int64_t, kRangeEndpointCount + 1> greaterThanDifference{};
    for (const double value : values) {
        if (!std::isfinite(value)) continue;
        if (view.smart && value >= view.backgroundMin && value <= view.backgroundMax) continue;
        ++analysis.effectiveValueCount;
        const auto firstNotLess = std::lower_bound(thresholds.begin(), thresholds.end(), value);
        const int firstNotLessIndex = static_cast<int>(firstNotLess - thresholds.begin());
        if (firstNotLessIndex > 0) {
            greaterThanDifference[0] += 1;
            greaterThanDifference[static_cast<std::size_t>(firstNotLessIndex)] -= 1;
        }

        auto firstGreater = firstNotLess;
        if (firstGreater != thresholds.end() && *firstGreater == value) {
            firstGreater = std::upper_bound(firstGreater, thresholds.end(), value);
        }
        const int firstGreaterIndex = static_cast<int>(firstGreater - thresholds.begin());
        if (firstGreaterIndex < kRangeEndpointCount) {
            lessThanDifference[static_cast<std::size_t>(firstGreaterIndex)] += 1;
            lessThanDifference[static_cast<std::size_t>(kRangeEndpointCount)] -= 1;
        }
    }
    std::int64_t lessThanCount = 0;
    std::int64_t greaterThanCount = 0;
    for (int endpoint = 0; endpoint < kRangeEndpointCount; ++endpoint) {
        lessThanCount += lessThanDifference[static_cast<std::size_t>(endpoint)];
        greaterThanCount += greaterThanDifference[static_cast<std::size_t>(endpoint)];
        analysis.lessThanEndpointCounts[static_cast<std::size_t>(endpoint)] =
            static_cast<std::uint64_t>(std::max<std::int64_t>(0, lessThanCount));
        analysis.greaterThanEndpointCounts[static_cast<std::size_t>(endpoint)] =
            static_cast<std::uint64_t>(std::max<std::int64_t>(0, greaterThanCount));
    }
    analysis.ready = true;
    return analysis;
}

const igQtDataCodecCompressionWidget::HistogramView&
igQtDataCodecCompressionWidget::histogramViewForFeature(const FeatureState& featureState) const {
    if (!featureState.analysis.ready && featureState.featureBasis != nullptr) {
        featureState.analysis = buildFeatureAnalysis(featureState.featureBasis);
    }
    return featureState.analysis.histogram;
}

const igQtDataCodecCompressionWidget::HistogramView&
igQtDataCodecCompressionWidget::currentHistogramView() const {
    static const HistogramView emptyView;
    const auto* featureState = currentFeatureState();
    if (featureState == nullptr || featureState->featureBasis == nullptr) return emptyView;
    return histogramViewForFeature(*featureState);
}

igQtDataCodecCompressionWidget::FieldState* igQtDataCodecCompressionWidget::currentFieldState() {
    if (!hasNumericFields()) return nullptr;
    ensureFieldStates();
    ensureFieldStateInitialized(m_selectedFieldIndex);
    return &m_fieldStates[m_selectedFieldIndex];
}

const igQtDataCodecCompressionWidget::FieldState* igQtDataCodecCompressionWidget::currentFieldState() const {
    if (!hasNumericFields() || m_selectedFieldIndex >= m_fieldStates.size()) return nullptr;
    return &m_fieldStates[m_selectedFieldIndex];
}

igQtDataCodecCompressionWidget::FeatureState* igQtDataCodecCompressionWidget::currentFeatureState() {
    auto* fieldState = currentFieldState();
    if (fieldState == nullptr) return nullptr;
    ensureFeatureStates(*fieldState);
    return &fieldState->features[qBound(0, fieldState->currentBasisIndex, 2)];
}

const igQtDataCodecCompressionWidget::FeatureState* igQtDataCodecCompressionWidget::currentFeatureState() const {
    const auto* fieldState = currentFieldState();
    if (fieldState == nullptr || fieldState->features.isEmpty()) return nullptr;
    return &fieldState->features[qBound(0, fieldState->currentBasisIndex, qMax(0, fieldState->features.size() - 1))];
}

QVector<igQtDataCodecCompressionWidget::RegionItem>* igQtDataCodecCompressionWidget::currentRegions() {
    auto* featureState = currentFeatureState();
    return featureState != nullptr ? &featureState->regions : nullptr;
}

const QVector<igQtDataCodecCompressionWidget::RegionItem>* igQtDataCodecCompressionWidget::currentRegions() const {
    const auto* featureState = currentFeatureState();
    return featureState != nullptr ? &featureState->regions : nullptr;
}

igQtDataCodecCompressionWidget::RegionItem igQtDataCodecCompressionWidget::makeDefaultRegion(
        const FieldState& state) const {
    const double absPrecision = state.losslessMode ? 0.0 : state.defaultAbsPrecision;
    const double relPrecision = state.losslessMode ? 0.0 : state.defaultRelPrecision;
    return RegionItem{0, QStringLiteral("默认区域"), QString(),
                      absPrecision, relPrecision, effectivePrecisionMode(state.defaultPrecisionMode), 0, 0, 100};
}

void igQtDataCodecCompressionWidget::ensureFieldStates() {
    if (m_fields.isEmpty()) {
        m_fieldStates.clear();
        return;
    }
    if (m_fieldStates.size() != m_fields.size()) {
        m_fieldStates.resize(m_fields.size());
    }
}

void igQtDataCodecCompressionWidget::ensureFieldStateInitialized(int fieldIndex) {
    if (fieldIndex < 0 || fieldIndex >= m_fieldStates.size()) return;
    FieldState& state = m_fieldStates[fieldIndex];
    initializeFieldStateDefaults(state, fieldIndex);
    ensureFeatureStates(state);
}

void igQtDataCodecCompressionWidget::initializeFieldStateDefaults(FieldState& state, int fieldIndex) const {
    if (state.precisionDefaultsInitialized) return;
    state.valueRange = computeFieldValueRange(fieldIndex);
    state.initialDefaultAbsPrecision = state.defaultAbsPrecision;
    state.initialDefaultRelPrecision = state.defaultRelPrecision;
    state.initialLosslessMode = state.losslessMode;
    state.precisionDefaultsInitialized = true;
}

double igQtDataCodecCompressionWidget::computeFieldValueRange(int fieldIndex) const {
    if (fieldIndex < 0 || fieldIndex >= m_fields.size()) return 0.0;
    const NumericFieldItem& field = m_fields[fieldIndex];
    auto dataObject = field.sourceObject;
    if (dataObject == nullptr) return 0.0;

    double minValue = std::numeric_limits<double>::infinity();
    double maxValue = -std::numeric_limits<double>::infinity();
    auto consumeValue = [&](double value) {
        if (!std::isfinite(value)) return;
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    };

    if (auto* attributeSet = dataObject->GetAttributeSet()) {
        auto attributes = attributeSet->GetAllAttributes();
        if (attributes == nullptr) return 0.0;
        for (int i = 0; i < attributes->GetNumberOfElements(); ++i) {
            auto& attr = attributes->GetElement(i);
            if (attr.isDeleted || attr.pointer == nullptr) continue;
            if (attr.attachmentType != field.attachmentType) continue;
            if (attr.pointer->GetName() != field.name.toStdString()) continue;
            const int dimension = attr.pointer->GetDimension();
            const int readComponent = dimension == 1 ? 0 : -1;
            for (IGsize element = 0; element < attr.pointer->GetNumberOfElements(); ++element) {
                consumeValue(attr.pointer->GetElementValue(element, readComponent));
            }
            break;
        }
    }

    if (!std::isfinite(minValue) || !std::isfinite(maxValue) || maxValue <= minValue) return 0.0;
    return maxValue - minValue;
}

IGsize igQtDataCodecCompressionWidget::numericFieldElementCount(int fieldIndex) const {
    if (fieldIndex < 0 || fieldIndex >= m_fields.size()) return 0;
    const NumericFieldItem& field = m_fields[fieldIndex];
    auto* attributeSet = field.sourceObject != nullptr
        ? field.sourceObject->GetAttributeSet()
        : nullptr;
    if (attributeSet == nullptr) return 0;
    auto attributes = attributeSet->GetAllAttributes();
    if (attributes == nullptr) return 0;
    const std::string fieldName = toUtf8StdString(field.name);
    for (int index = 0; index < attributes->GetNumberOfElements(); ++index) {
        auto& attribute = attributes->GetElement(index);
        if (attribute.isDeleted || attribute.pointer == nullptr) continue;
        if (attribute.attachmentType != field.attachmentType) continue;
        if (attribute.pointer->GetName() != fieldName) continue;
        return attribute.pointer->GetNumberOfElements();
    }
    return 0;
}

void igQtDataCodecCompressionWidget::ensureFeatureStates(FieldState& state) const {
    if (state.features.size() != 3) {
        state.features.resize(3);
    }
    state.currentBasisIndex = qBound(0, state.currentBasisIndex, 2);
    syncDefaultRegionToFeatures(state);
}

void igQtDataCodecCompressionWidget::syncDefaultRegionToFeatures(FieldState& state) const {
    const RegionItem defaultRegion = makeDefaultRegion(state);
    for (FeatureState& feature : state.features) {
        if (feature.regions.isEmpty()) {
            feature.regions.push_back(defaultRegion);
        } else {
            bool foundDefault = false;
            for (RegionItem& region : feature.regions) {
                if (region.id != 0) continue;
                region = defaultRegion;
                foundDefault = true;
                break;
            }
            if (!foundDefault) {
                feature.regions.prepend(defaultRegion);
            }
        }

        bool selectedExists = false;
        for (const RegionItem& region : feature.regions) {
            if (region.id == feature.selectedRegionId) {
                selectedExists = true;
                break;
            }
        }
        if (!selectedExists) feature.selectedRegionId = 0;
    }
}

QString igQtDataCodecCompressionWidget::basisBaseName(int index) const {
    switch (index) {
    case 1: return QStringLiteral("相邻值标准差");
    case 2: return QStringLiteral("相邻最大差值");
    default: return QStringLiteral("模长");
    }
}

QString igQtDataCodecCompressionWidget::basisOptionText(int index, const FieldState* state) const {
    Q_UNUSED(state);
    return basisBaseName(index);
}

QString igQtDataCodecCompressionWidget::fieldItemText(int index) const {
    if (index < 0 || index >= m_fields.size()) return QString();
    QString text = m_fields[index].name + QStringLiteral("\n") + m_fields[index].detail;
    if (index < m_fieldStates.size()) {
        const QString status = fieldStatusText(m_fieldStates[index]);
        if (!status.isEmpty()) text += QStringLiteral(" · ") + status;
    }
    return text;
}

QString igQtDataCodecCompressionWidget::fieldStatusText(const FieldState& state) const {
    return state.losslessMode ? QStringLiteral("无损") : QStringLiteral("有损");
}

int igQtDataCodecCompressionWidget::explicitRegionCount(const FieldState& state) const {
    int count = 0;
    for (const FeatureState& feature : state.features) {
        count += qMax(0, feature.regions.size() - 1);
    }
    return count;
}

bool igQtDataCodecCompressionWidget::fieldHasSavedState(const FieldState& state) const {
    if (!state.selected) return true;
    if (state.losslessMode != state.initialLosslessMode) return true;
    if (state.defaultPrecisionMode != kPrecisionModeAbs) return true;
    if (!qFuzzyCompare(state.defaultAbsPrecision + 1.0, state.initialDefaultAbsPrecision + 1.0)) return true;
    if (!qFuzzyCompare(state.defaultRelPrecision + 1.0, state.initialDefaultRelPrecision + 1.0)) return true;
    if (explicitRegionCount(state) > 0) return true;
    for (const FeatureState& feature : state.features) {
        if (feature.basisComputed) return true;
    }
    return false;
}

int igQtDataCodecCompressionWidget::effectivePrecisionMode(int mode) const {
    return mode == kPrecisionModeRel ? kPrecisionModeRel : kPrecisionModeAbs;
}

QString igQtDataCodecCompressionWidget::precisionModeName(int mode) const {
    return effectivePrecisionMode(mode) == kPrecisionModeRel ? QStringLiteral("相对误差限")
                                                            : QStringLiteral("绝对误差限");
}

QString igQtDataCodecCompressionWidget::precisionValueText(double value) const {
    return QString::number(value, 'f', 8);
}

QString igQtDataCodecCompressionWidget::regionPrecisionText(const RegionItem& region) const {
    const int mode = effectivePrecisionMode(region.precisionMode);
    return QStringLiteral("%1 %2").arg(precisionModeName(mode), precisionValueText(precisionValueForMode(region, mode)));
}

::datacodec::CompressorConfig igQtDataCodecCompressionWidget::makeCompressorConfig(
        const int mode,
        const double value) const {
    ::datacodec::CompressorConfig compressor;
    const int nextMode = effectivePrecisionMode(mode);
    compressor.options[nextMode == kPrecisionModeRel ? "pressio:rel" : "pressio:abs"] = value;
    return compressor;
}

void
igQtDataCodecCompressionWidget::applyCompressionControlToDataCodec(
        ::datacodec::CodecControlParams& controlParams) {
    ensureFieldStates();
    const bool multiFrame = hasMultiFrameData();
    const bool enableIntraAttributePrediction =
        m_intraAttributePredictionCheck != nullptr &&
        m_intraAttributePredictionCheck->isChecked();
    const bool enableTemporalAttributePrediction =
        multiFrame &&
        m_temporalAttributePredictionCheck != nullptr &&
        m_temporalAttributePredictionCheck->isChecked();
    controlParams.attrReference.enabled =
        enableIntraAttributePrediction || enableTemporalAttributePrediction;
    if (!enableIntraAttributePrediction) {
        controlParams.attrReference.intraField.codec =
            ::datacodec::IntraFieldReferenceCodec::Disabled;
    } else if (controlParams.attrReference.intraField.codec ==
               ::datacodec::IntraFieldReferenceCodec::Disabled) {
        controlParams.attrReference.intraField.codec =
            ::datacodec::IntraFieldReferenceControlParams{}.codec;
    }
    if (!enableTemporalAttributePrediction) {
        controlParams.attrReference.temporalField.codec =
            ::datacodec::TemporalFieldReferenceCodec::Disabled;
    } else if (controlParams.attrReference.temporalField.codec ==
               ::datacodec::TemporalFieldReferenceCodec::Disabled) {
        controlParams.attrReference.temporalField.codec =
            ::datacodec::TemporalFieldReferenceControlParams{}.codec;
    }

    for (int fieldIndex = 0; fieldIndex < m_fields.size() && fieldIndex < m_fieldStates.size(); ++fieldIndex) {
        const NumericFieldItem& field = m_fields[fieldIndex];
        FieldState& fieldState = m_fieldStates[fieldIndex];
        if (!fieldState.selected) continue;
        ensureFeatureStates(fieldState);

        const RegionItem defaultRegion = makeDefaultRegion(fieldState);
        const int defaultMode = effectivePrecisionMode(defaultRegion.precisionMode);
        ::datacodec::NumericArrayControlParams numericControl =
            controlParams.defaultAttrControl;
        numericControl.regionControl.defaultPrecision =
            ::datacodec::MakeNumericArrayRegionPrecision(
                makeCompressorConfig(defaultMode, precisionValueForMode(defaultRegion, defaultMode)));
        numericControl.regionControl.regions.clear();
        numericControl.regionRuns.clear();

        // 区域元素编号只对应当前驻留帧
        // 多帧任务仅共享每项属性数据的默认误差限
        if (!fieldState.losslessMode && !multiFrame) {
            const std::size_t elementCount = static_cast<std::size_t>(
                numericFieldElementCount(fieldIndex));
            const FeatureState& featureState =
                fieldState.features[qBound(0, fieldState.currentBasisIndex, qMax(0, fieldState.features.size() - 1))];

            struct EncodingRegionRange {
                double minimum{0.0};
                double maximum{0.0};
                std::uint32_t regionId{0u};
            };
            std::vector<EncodingRegionRange> encodingRegions;
            encodingRegions.reserve(static_cast<std::size_t>(customRegionCount(featureState)));

            if (featureState.basisComputed && featureState.featureBasis != nullptr && elementCount > 0u) {
                const HistogramView& histogramView = histogramViewForFeature(featureState);
                for (const RegionItem& region : featureState.regions) {
                    if (region.id == 0) continue;
                    if (featureSelectionCountForRange64(
                            featureState,
                            region.rangeLower,
                            region.rangeUpper) == 0u) {
                        continue;
                    }

                    const double lowerValue = valueAtPercent(
                        histogramView.valueMin,
                        histogramView.valueMax,
                        region.rangeLower);
                    const double upperValue = valueAtPercent(
                        histogramView.valueMin,
                        histogramView.valueMax,
                        region.rangeUpper);
                    const int regionMode = effectivePrecisionMode(region.precisionMode);
                    numericControl.regionControl.regions.push_back(
                        ::datacodec::MakeNumericArrayRegionPrecision(
                            makeCompressorConfig(
                                regionMode,
                                precisionValueForMode(region, regionMode))));
                    encodingRegions.push_back(EncodingRegionRange{
                        .minimum = qMin(lowerValue, upperValue),
                        .maximum = qMax(lowerValue, upperValue),
                        .regionId = static_cast<std::uint32_t>(
                            numericControl.regionControl.regions.size()),
                    });
                }
            }

            if (!encodingRegions.empty()) {
                const HistogramView& histogramView = histogramViewForFeature(featureState);
                const std::vector<double>& featureValues = featureState.featureBasis->GetValues();
                const std::size_t scanCount = std::min(elementCount, featureValues.size());
                std::uint32_t currentRegionId = 0u;
                std::size_t currentRunBegin = 0u;

                auto appendCurrentRun = [&](const std::size_t runEnd) {
                    if (currentRegionId == 0u || currentRunBegin >= runEnd) return;
                    numericControl.regionRuns.push_back(::datacodec::RegionRun{
                        .begin = static_cast<::datacodec::ParamSize>(currentRunBegin),
                        .count = static_cast<::datacodec::ParamSize>(runEnd - currentRunBegin),
                        .regionId = currentRegionId,
                    });
                };

                for (std::size_t elementIndex = 0u; elementIndex < scanCount; ++elementIndex) {
                    const double value = featureValues[elementIndex];
                    std::uint32_t nextRegionId = 0u;
                    if (std::isfinite(value) &&
                        !(histogramView.smart &&
                          value >= histogramView.backgroundMin &&
                          value <= histogramView.backgroundMax)) {
                        for (const auto& region : encodingRegions) {
                            if (value >= region.minimum && value <= region.maximum) {
                                nextRegionId = region.regionId;
                                break;
                            }
                        }
                    }
                    if (nextRegionId == currentRegionId) continue;
                    appendCurrentRun(elementIndex);
                    currentRegionId = nextRegionId;
                    currentRunBegin = elementIndex;
                }
                appendCurrentRun(scanCount);
            }
        }

        const std::string fieldName = toUtf8StdString(field.name);
        controlParams.attrControl[fieldName] = std::move(numericControl);
    }
}

QVector<igQtDataCodecCompressionWidget::PrecisionFieldReport>
igQtDataCodecCompressionWidget::buildPrecisionReports(
        const QVector<NumericFieldItem>& fields,
        const iGame::DataObject::Pointer& original,
        const iGame::DataObject::Pointer& decoded,
        const ::datacodec::log::AdapterSignatureOrderSet& orderSet) {
    QVector<PrecisionFieldReport> reports;
    reports.reserve(fields.size());

    auto makeFailure = [](const NumericFieldItem& field, const QString& message) {
        PrecisionFieldReport report;
        report.fieldName = field.name;
        report.detail = field.detail;
        report.ok = false;
        report.error = message;
        report.componentCount = qMax(1, field.componentCount);
        return report;
    };

    if (original == nullptr || decoded == nullptr) {
        const QString message = decoded == nullptr ? QStringLiteral("回读输出对象为空") : QStringLiteral("原始对象为空");
        for (const auto& field : fields) {
            reports.push_back(makeFailure(field, message));
        }
        return reports;
    }

    iGame::iGameBlockTreeAdapter originalTree(original);
    iGame::iGameBlockTreeAdapter decodedTree(decoded);
    const auto probe = ::datacodec::log::ComputeAdapterPrecisionMetrics(
        originalTree, decodedTree, &orderSet);
    auto matchesField = [](const NumericFieldItem& field,
                           const ::datacodec::log::NumericFieldPrecisionMetric& metric) {
        const bool pointAttr = field.attachmentType == IG_POINT &&
            metric.kind == ::datacodec::log::NumericFieldPrecisionKind::PointAttribute;
        const bool cellAttr = field.attachmentType == IG_CELL &&
            metric.kind == ::datacodec::log::NumericFieldPrecisionKind::CellAttribute;
        return (pointAttr || cellAttr) && metric.name == toUtf8StdString(field.name);
    };

    QString probeFailure;
    if (!probe.status.passed && !probe.status.failures.empty()) {
        const auto& failure = probe.status.failures.front();
        probeFailure = QString::fromStdString(failure.check + ": " + failure.message);
    }

    for (const auto& field : fields) {
        PrecisionFieldReport report;
        report.fieldName = field.name;
        report.detail = field.detail;
        report.ok = true;
        report.componentCount = qMax(1, field.componentCount);
        report.originalMin = std::numeric_limits<double>::infinity();
        report.originalMax = -std::numeric_limits<double>::infinity();

        bool found = false;
        double sumSq = 0.0;
        std::uint64_t valueCount = 0u;
        for (const auto& metric : probe.fields) {
            if (!matchesField(field, metric)) continue;
            found = true;
            if (!metric.ok) {
                report.ok = false;
                report.error = QString::fromStdString(metric.error);
                break;
            }

            report.elementCount += static_cast<IGsize>(metric.tupleCount);
            report.componentCount = qMax(report.componentCount, static_cast<int>(metric.componentCount));
            report.maxAbsError = std::max(report.maxAbsError, metric.maxAbsError);
            report.maxPointRelativeError = std::max(report.maxPointRelativeError, metric.maxPointRelativeError);
            report.originalMin = std::min(report.originalMin, metric.expectedMin);
            report.originalMax = std::max(report.originalMax, metric.expectedMax);
            sumSq += metric.sumSquaredError;
            valueCount += metric.valueCount;
        }

        if (!found) {
            reports.push_back(makeFailure(
                field,
                probeFailure.isEmpty() ? QStringLiteral("DataCodec 误差检测未找到数据") : probeFailure));
            continue;
        }
        if (!report.ok) {
            reports.push_back(report);
            continue;
        }
        if (valueCount == 0u) {
            report.originalMin = 0.0;
            report.originalMax = 0.0;
            report.hasRangeRelativeError = true;
        } else {
            report.rmsAbsError = std::sqrt(sumSq / static_cast<double>(valueCount));
            const double range = report.originalMax - report.originalMin;
            if (range > 0.0) {
                report.rangeRelativeError = report.maxAbsError / range;
                report.hasRangeRelativeError = true;
            } else if (report.maxAbsError == 0.0) {
                report.rangeRelativeError = 0.0;
                report.hasRangeRelativeError = true;
            }
        }
        reports.push_back(report);
    }
    return reports;
}

QStringList igQtDataCodecCompressionWidget::buildPrecisionStatusLines(
        const QVector<PrecisionFieldReport>& reports) {
    QStringList lines;
    for (const auto& report : reports) {
        if (!report.ok) {
            lines.push_back(QStringLiteral("%1 相对误差统计失败").arg(report.fieldName));
            continue;
        }
        lines.push_back(QStringLiteral("%1 最大逐值相对误差：%2")
                                .arg(report.fieldName, formatPrecisionMetric(report.maxPointRelativeError)));
    }
    return lines;
}

double igQtDataCodecCompressionWidget::precisionValueForMode(const RegionItem& region, int mode) const {
    return effectivePrecisionMode(mode) == kPrecisionModeRel ? region.relPrecision : region.absPrecision;
}

void igQtDataCodecCompressionWidget::setPrecisionValueForMode(RegionItem& region, int mode, double value) const {
    if (effectivePrecisionMode(mode) == kPrecisionModeRel) {
        region.relPrecision = value;
    } else {
        region.absPrecision = value;
    }
}

double igQtDataCodecCompressionWidget::normalizedPrecisionValue(int fieldIndex, int mode, double value) const {
    if (value <= 0.0) return 0.0;
    if (effectivePrecisionMode(mode) == kPrecisionModeRel) return value;
    if (fieldIndex < 0 || fieldIndex >= m_fieldStates.size()) return value;
    const double range = m_fieldStates[fieldIndex].valueRange;
    if (range <= std::numeric_limits<double>::epsilon()) return 0.0;
    return value / range;
}

void igQtDataCodecCompressionWidget::applyPrecisionSpinTone(
        QDoubleSpinBox* spin,
        int fieldIndex,
        int mode,
        double value,
        bool enabled) const {
    if (spin == nullptr) return;
    if (!enabled) {
        spin->setStyleSheet(precisionSpinToneStyle(QColor(125, 125, 125)));
        spin->setToolTip(QString());
        return;
    }

    const double normalized = normalizedPrecisionValue(fieldIndex, mode, value);
    QColor color(86, 214, 131);
    if (normalized <= kPrecisionGreenLimit) {
        color = QColor(86, 214, 131);
    } else if (normalized <= kPrecisionNormalLimit) {
        color = QColor(98, 201, 214);
    } else if (normalized <= kPrecisionWarnLimit) {
        color = QColor(226, 190, 78);
    } else if (normalized <= kPrecisionHighWarnLimit) {
        color = QColor(232, 137, 64);
    } else {
        color = QColor(238, 91, 91);
    }
    spin->setStyleSheet(precisionSpinToneStyle(color));
    spin->setToolTip(QStringLiteral("归一化误差限：%1").arg(QString::number(normalized, 'g', 4)));
}

void igQtDataCodecCompressionWidget::configurePrecisionModeCombo(QComboBox* combo, int mode) const {
    if (combo == nullptr) return;
    QSignalBlocker blocker(combo);
    combo->clear();
    combo->addItem(QStringLiteral("绝对误差限"), kPrecisionModeAbs);
    combo->addItem(QStringLiteral("相对误差限"), kPrecisionModeRel);
    const int targetMode = effectivePrecisionMode(mode);
    const int index = qMax(0, combo->findData(targetMode));
    combo->setCurrentIndex(index);
    combo->setToolTip(QStringLiteral("同一数据的默认区域和自定义区域统一使用此误差限类型"));
}

void igQtDataCodecCompressionWidget::updateRegionPrecisionMode(int regionId, int mode) {
    Q_UNUSED(regionId);
    auto* fieldState = currentFieldState();
    if (fieldState == nullptr) return;
    const int nextMode = effectivePrecisionMode(mode);
    fieldState->defaultPrecisionMode = nextMode;
    for (FeatureState& feature : fieldState->features) {
        for (RegionItem& region : feature.regions) {
            region.precisionMode = nextMode;
        }
    }
    syncDefaultRegionToFeatures(*fieldState);
}

void igQtDataCodecCompressionWidget::updateRegionPrecisionValue(int regionId, int mode, double value) {
    auto* fieldState = currentFieldState();
    auto* featureState = currentFeatureState();
    if (fieldState == nullptr || featureState == nullptr) return;
    const int nextMode = effectivePrecisionMode(mode);
    if (regionId == 0) {
        if (nextMode == kPrecisionModeRel) {
            fieldState->defaultRelPrecision = value;
        } else {
            fieldState->defaultAbsPrecision = value;
        }
        syncDefaultRegionToFeatures(*fieldState);
        return;
    }
    for (RegionItem& region : featureState->regions) {
        if (region.id == regionId) {
            setPrecisionValueForMode(region, nextMode, value);
            break;
        }
    }
}

QString igQtDataCodecCompressionWidget::currentBasisName() const {
    const auto* fieldState = currentFieldState();
    return basisBaseName(fieldState != nullptr ? fieldState->currentBasisIndex : 0);
}

int igQtDataCodecCompressionWidget::selectedRegionIndex() const {
    const auto* featureState = currentFeatureState();
    const auto* regions = currentRegions();
    if (featureState == nullptr || regions == nullptr) return -1;
    for (int i = 0; i < regions->size(); ++i) {
        if ((*regions)[i].id == featureState->selectedRegionId) return i;
    }
    return -1;
}

bool igQtDataCodecCompressionWidget::hasExplicitRegions() const {
    const auto* regions = currentRegions();
    return regions != nullptr && regions->size() > 1;
}
