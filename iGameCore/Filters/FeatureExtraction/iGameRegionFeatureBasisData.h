#ifndef iGameRegionFeatureBasisData_h
#define iGameRegionFeatureBasisData_h

#include "iGameDataObject.h"
#include "iGameType.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class RegionFeatureBasisData : public DataObject {
public:
    I_OBJECT(RegionFeatureBasisData);
    static Pointer New() { return new RegionFeatureBasisData; }

    void SetAttachmentType(IGenum attachmentType) { m_AttachmentType = attachmentType; }
    IGenum GetAttachmentType() const { return m_AttachmentType; }

    void SetFieldName(const std::string& fieldName) { m_FieldName = fieldName; }
    const std::string& GetFieldName() const { return m_FieldName; }

    void SetValues(std::vector<double> values);
    const std::vector<double>& GetValues() const { return m_Values; }
    std::size_t GetElementCount() const { return m_Values.size(); }

    bool HasValues() const { return !m_Values.empty(); }
    double GetMinValue() const { return m_MinValue; }
    double GetMaxValue() const { return m_MaxValue; }
    double ValueForPercent(int percent) const;

    void BuildHistogram(int binCount);
    const std::vector<double>& GetHistogram() const { return m_Histogram; }

    std::vector<igIndex> PickValueRange(double minValue, double maxValue) const;
    std::vector<igIndex> PickPercentRange(int lowerPercent, int upperPercent) const;

protected:
    RegionFeatureBasisData() = default;
    ~RegionFeatureBasisData() override = default;

private:
    IGenum m_AttachmentType{IG_NONE};
    std::string m_FieldName;
    std::vector<double> m_Values;
    std::vector<double> m_Histogram;
    double m_MinValue{0.0};
    double m_MaxValue{0.0};
};

IGAME_NAMESPACE_END

#endif
