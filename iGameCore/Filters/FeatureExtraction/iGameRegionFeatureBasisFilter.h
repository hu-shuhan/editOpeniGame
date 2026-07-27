#ifndef iGameRegionFeatureBasisFilter_h
#define iGameRegionFeatureBasisFilter_h

#include "iGameFilter.h"
#include "iGameRegionFeatureBasisData.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class AttributeSet;
class ArrayObject;

class RegionFeatureBasisFilter : public Filter {
public:
    I_OBJECT(RegionFeatureBasisFilter);
    static Pointer New() { return new RegionFeatureBasisFilter; }

    enum class BasisMode {
        Magnitude,
        LocalStdDev,
        Jump
    };

    void SetGeometryField();
    void SetAttributeByIndex(int index, IGenum attachmentType = IG_NONE, int component = -1);
    void SetAttributeByName(const std::string& name, IGenum attachmentType = IG_NONE, int component = -1);
    void SetBasisMode(BasisMode mode) { m_Mode = mode; }
    void SetHistogramBinCount(int binCount) { m_HistogramBinCount = binCount; }

    RegionFeatureBasisData::Pointer GetFeatureBasisData() const { return m_OutputData; }
    std::string GetMessage() const { return m_Message; }

    bool Execute() override;

protected:
    RegionFeatureBasisFilter();
    ~RegionFeatureBasisFilter() override = default;

private:
    struct AttributeLookupResult {
        int index{-1};
        AttributeSet::Attribute* attribute{nullptr};
    };

    AttributeLookupResult FindAttribute(AttributeSet* attributeSet) const;
    bool BuildGeometryValues(DataObject::Pointer input, std::vector<double>& values, IGenum& attachmentType);
    bool BuildAttributeValues(AttributeSet::Attribute* attribute, std::vector<double>& values, IGenum& attachmentType);
    std::vector<double> ApplyBasisMode(const std::vector<double>& baseValues) const;

    bool m_UseGeometry{true};
    int m_AttributeIndex{-1};
    IGenum m_AttachmentType{IG_NONE};
    int m_Component{-1};
    int m_HistogramBinCount{34};
    BasisMode m_Mode{BasisMode::Magnitude};
    std::string m_AttributeName;
    std::string m_Message;
    RegionFeatureBasisData::Pointer m_OutputData;
};

IGAME_NAMESPACE_END

#endif
