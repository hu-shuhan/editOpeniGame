#pragma once

#include <IQCore/igQtExportModule.h>

#include <QFrame>
#include <QString>
#include <QVector>
#include <QWidget>
#include <DataCodec/Common/DataCodecTypes.h>
#include <FeatureExtraction/iGameRegionFeatureBasisData.h>
#include <iGameDataObject.h>
#include <iGameModel.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace datacodec {
enum class DataCodecEncodeTier;
struct CodecControlParams;
struct CompressorConfig;
}

namespace datacodec::log {
struct AdapterSignatureOrderSet;
}

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QEvent;
class QLabel;
class QLineEdit;
class QMouseEvent;
class QListWidget;
class QPaintEvent;
class QPlainTextEdit;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QVBoxLayout;

class igQtDataCodecHistogramWidget : public QWidget {
public:
    explicit igQtDataCodecHistogramWidget(QWidget* parent = nullptr);

    void setRange(int lowerPercent, int upperPercent);
    void setHistogramView(const QVector<double>& bins, int backgroundCount, int totalCount);
    void setOccupiedBins(const QVector<double>& bins);
    void clearBins();
    void setEnabledVisual(bool enabled);
    void setDarkMode(bool darkMode);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_lowerPercent{76};
    int m_upperPercent{100};
    QVector<double> m_bins;
    QVector<double> m_occupiedBins;
    int m_backgroundCount{0};
    int m_totalCount{0};
    bool m_enabledVisual{false};
    bool m_darkMode{true};
};

class igQtDataCodecRangeSliderWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtDataCodecRangeSliderWidget(QWidget* parent = nullptr);

    int lowerValue() const { return m_lowerValue; }
    int upperValue() const { return m_upperValue; }
    bool isDragging() const { return m_dragTarget != DragTarget::None; }
    void setRangeValues(int lowerValue, int upperValue);
    void setSliderEnabled(bool enabled);
    void setDarkMode(bool darkMode);
    void setTrackInsets(int leftInset, int rightInset);

signals:
    void rangeChanged();
    void rangeEditingFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    enum class DragTarget {
        None,
        Lower,
        Upper
    };

    QRect trackRect() const;
    int positionForValue(int value) const;
    int valueForPosition(int x) const;
    DragTarget nearestHandle(const QPoint& pos) const;

    int m_lowerValue{76};
    int m_upperValue{100};
    int m_leftInset{16};
    int m_rightInset{16};
    bool m_enabled{false};
    bool m_darkMode{true};
    DragTarget m_dragTarget{DragTarget::None};
};

class IG_QT_MODULE_EXPORT igQtDataCodecCompressionWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtDataCodecCompressionWidget(QWidget* parent = nullptr);

    void SetModel(iGame::Model::Pointer model);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct NumericFieldItem {
        QString name;
        QString detail;
        iGame::DataObject::Pointer sourceObject;
        std::vector<::datacodec::AttributeTarget> targets;
        int attachmentType{0};
        int componentCount{1};
    };

    struct RegionItem {
        int id{0};
        QString name;
        QString ruleSummary;
        double absPrecision{0.004};
        double relPrecision{0.002};
        int precisionMode{0};
        int elementCount{0};
        int rangeLower{0};
        int rangeUpper{100};
    };

    struct HistogramView {
        QVector<double> bins;
        bool smart{false};
        int backgroundCount{0};
        int totalCount{0};
        double valueMin{0.0};
        double valueMax{0.0};
        double backgroundMin{0.0};
        double backgroundMax{0.0};
    };

    struct FeatureAnalysis {
        HistogramView histogram;
        std::array<std::uint64_t, 101u> lessThanEndpointCounts{};
        std::array<std::uint64_t, 101u> greaterThanEndpointCounts{};
        std::uint64_t effectiveValueCount{0u};
        bool ready{false};
    };

    struct FeatureState {
        iGame::RegionFeatureBasisData::Pointer featureBasis;
        mutable FeatureAnalysis analysis;
        QVector<RegionItem> regions;
        int selectedRegionId{0};
        bool basisComputed{false};
        int rangeLower{76};
        int rangeUpper{100};
    };

    struct FieldState {
        QVector<FeatureState> features;
        int currentBasisIndex{0};
        double defaultAbsPrecision{0.0001};
        double defaultRelPrecision{0.0001};
        double initialDefaultAbsPrecision{0.0001};
        double initialDefaultRelPrecision{0.0001};
        double valueRange{0.0};
        int defaultPrecisionMode{0};
        bool losslessMode{true};
        bool initialLosslessMode{true};
        bool precisionDefaultsInitialized{false};
        bool selected{true};

        FieldState();
    };

    struct PrecisionFieldReport {
        QString fieldName;
        QString detail;
        bool ok{false};
        QString error;
        IGsize elementCount{0};
        int componentCount{0};
        double maxAbsError{0.0};
        double rmsAbsError{0.0};
        double maxPointRelativeError{0.0};
        double rangeRelativeError{0.0};
        bool hasRangeRelativeError{false};
        double originalMin{0.0};
        double originalMax{0.0};
    };

    QWidget* createOutputPanel();
    QWidget* createFieldPanel();
    QWidget* createBasisPanel();
    QWidget* createRegionPanel();
    QWidget* createRegionBuilderPanel();
    QWidget* createStatusPanel();
    QWidget* createStatBox(QLabel*& valueLabel, const QString& title);
    QWidget* createRegionRow(const RegionItem& region, bool invalidRegion);

    void applyStyle();
    void resetRegionState();
    void refreshFields();
    void refreshFieldItemLabels();
    void refreshSelectAllState();
    void refreshBasisOptions();
    void refreshRegionRows();
    void refreshSelectedRegion();
    void refreshFeatureState();
    void refreshStats();
    void refreshPredictionControls();
    void refreshPerformanceControls();
    void applyPerformanceTierDefaults();
    void persistPerformanceSettings() const;
    [[nodiscard]] ::datacodec::DataCodecEncodeTier selectedPerformanceTier() const;
    bool hasNumericFields() const;
    bool hasSelectedFields() const;
    std::vector<::datacodec::AttributeTarget> selectedAttributeTargets() const;
    bool hasMultiFrameData() const;
    void appendPredictionEncodingRecommendation();
    void appendLog(const QString& text);
    void chooseOutputPath();
    void startEncode();
    void computeFeature();
    void addRegion();
    void selectRegion(int regionId);
    void onFieldChanged(int row);
    void onBasisChanged(int index);
    void onRangeChanged();
    void commitRangeSelection();
    void deleteRegion(int regionId);
    void syncSelectedPrecisionMode();
    void syncSelectedPrecision();
    void applyLosslessMode(bool enabled);
    void applyLosslessModeToAllFields();
    void applyHighPrecisionLossyModeToAllFields();
    void syncCurrentDefaultPrecisionToAllFields();
    bool losslessModeEnabled() const;
    int featureSelectionCountForRange(const FeatureState& featureState, int lower, int upper) const;
    std::uint64_t featureSelectionCountForRange64(const FeatureState& featureState, int lower, int upper) const;
    const HistogramView& currentHistogramView() const;
    const HistogramView& histogramViewForFeature(const FeatureState& featureState) const;
    QVector<double> currentOccupiedHistogramBins(const HistogramView& view) const;
    int currentSelectionOverlapCount(int ignoredRegionId = -1) const;
    int selectionOverlapCountForRange(const FeatureState& featureState, int lower, int upper, int ignoredRegionId) const;
    QVector<int> invalidRegionIdsForFeature(const FeatureState& featureState) const;
    bool isRegionInvalid(int regionId) const;
    int invalidActiveRegionCount() const;
    int unavailableActiveRegionFeatureCount() const;
    int customRegionCount(const FeatureState& featureState) const;

    FieldState* currentFieldState();
    const FieldState* currentFieldState() const;
    FeatureState* currentFeatureState();
    const FeatureState* currentFeatureState() const;
    QVector<RegionItem>* currentRegions();
    const QVector<RegionItem>* currentRegions() const;
    RegionItem makeDefaultRegion(const FieldState& state) const;
    void ensureFieldStates();
    void ensureFieldStateInitialized(int fieldIndex);
    void initializeFieldStateDefaults(FieldState& state, int fieldIndex) const;
    double computeFieldValueRange(int fieldIndex) const;
    IGsize numericFieldElementCount(int fieldIndex) const;
    void ensureFeatureStates(FieldState& state) const;
    void syncDefaultRegionToFeatures(FieldState& state) const;
    QString basisBaseName(int index) const;
    QString basisOptionText(int index, const FieldState* state) const;
    QString fieldItemText(int index) const;
    QString fieldStatusText(const FieldState& state) const;
    int explicitRegionCount(const FieldState& state) const;
    bool fieldHasSavedState(const FieldState& state) const;
    int effectivePrecisionMode(int mode) const;
    QString precisionModeName(int mode) const;
    QString precisionValueText(double value) const;
    QString regionPrecisionText(const RegionItem& region) const;
    ::datacodec::CompressorConfig makeCompressorConfig(int mode, double value) const;
    void applyCompressionControlToDataCodec(
        ::datacodec::CodecControlParams& controlParams);
    static QVector<PrecisionFieldReport> buildPrecisionReports(
        const QVector<NumericFieldItem>& fields,
        const iGame::DataObject::Pointer& original,
        const iGame::DataObject::Pointer& decoded,
        const ::datacodec::log::AdapterSignatureOrderSet& orderSet);
    static QStringList buildPrecisionStatusLines(const QVector<PrecisionFieldReport>& reports);
    static QString buildPrecisionReportText(
        const QVector<PrecisionFieldReport>& reports,
        const QString& compressorName);
    double precisionValueForMode(const RegionItem& region, int mode) const;
    void setPrecisionValueForMode(RegionItem& region, int mode, double value) const;
    double normalizedPrecisionValue(int fieldIndex, int mode, double value) const;
    void applyPrecisionSpinTone(QDoubleSpinBox* spin, int fieldIndex, int mode, double value, bool enabled) const;
    void configurePrecisionModeCombo(QComboBox* combo, int mode) const;
    void updateRegionPrecisionMode(int regionId, int mode);
    void updateRegionPrecisionValue(int regionId, int mode, double value);
    static FeatureAnalysis buildFeatureAnalysis(const iGame::RegionFeatureBasisData::Pointer& featureBasis);
    QString currentBasisName() const;
    int selectedRegionIndex() const;
    bool hasExplicitRegions() const;

    iGame::Model::Pointer m_model;
    QVector<NumericFieldItem> m_fields;
    QVector<FieldState> m_fieldStates;
    int m_selectedFieldIndex{0};

    QLineEdit* m_outputPathEdit{nullptr};
    QPushButton* m_outputPathButton{nullptr};
    QComboBox* m_performanceCombo{nullptr};
    QCheckBox* m_compressionEnhancementCheck{nullptr};
    QSpinBox* m_zstdLevelSpin{nullptr};
    QWidget* m_gopControlRow{nullptr};
    QSpinBox* m_gopFrameCountSpin{nullptr};
    QPushButton* m_applyLosslessAllButton{nullptr};
    QPushButton* m_applyLossyAllButton{nullptr};
    QPushButton* m_syncDefaultPrecisionButton{nullptr};
    QCheckBox* m_intraAttributePredictionCheck{nullptr};
    QCheckBox* m_temporalAttributePredictionCheck{nullptr};
    QCheckBox* m_selectAllFieldsCheck{nullptr};
    QListWidget* m_fieldList{nullptr};
    QComboBox* m_basisCombo{nullptr};
    QLabel* m_basisStateLabel{nullptr};
    QPushButton* m_computeButton{nullptr};
    QPushButton* m_addRegionButton{nullptr};
    QScrollArea* m_regionScrollArea{nullptr};
    QWidget* m_regionRowsHost{nullptr};
    QVBoxLayout* m_regionRowsLayout{nullptr};
    QLabel* m_regionBuilderTitle{nullptr};
    igQtDataCodecHistogramWidget* m_histogram{nullptr};
    igQtDataCodecRangeSliderWidget* m_rangeSlider{nullptr};
    QCheckBox* m_losslessModeCheck{nullptr};
    QComboBox* m_precisionModeCombo{nullptr};
    QDoubleSpinBox* m_precisionSpin{nullptr};
    QLabel* m_selectedTotalLabel{nullptr};
    QLabel* m_overlapTotalLabel{nullptr};
    QCheckBox* m_emitPerformanceCheck{nullptr};
    QPushButton* m_startEncodeButton{nullptr};
    QPlainTextEdit* m_logEdit{nullptr};
    bool m_intraAttributePredictionPreferred{true};
    bool m_temporalAttributePredictionPreferred{true};
    bool m_featureComputationActive{false};
    std::uint64_t m_featureRequestSerial{0u};
};
