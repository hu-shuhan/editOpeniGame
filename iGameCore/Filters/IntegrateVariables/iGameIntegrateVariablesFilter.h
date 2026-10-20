#ifndef iGameIntegrateVariablesFilter_h
#define iGameIntegrateVariablesFilter_h

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class IntegrateVariablesFilter : public Filter {
public:
    I_OBJECT(IntegrateVariablesFilter);
    static Pointer New() { return new IntegrateVariablesFilter; }

    bool Execute() override;

    void SetDivideAllCellDataByMeasure(bool value) {
        m_DivideAllCellDataByMeasure = value;
    }
    bool GetDivideAllCellDataByMeasure() const {
        return m_DivideAllCellDataByMeasure;
    }

    std::string GetMessage() const { return m_Message; }
    int GetIntegrationDimension() const { return m_IntegrationDimension; }
    int GetIntegratedCellCount() const { return m_IntegratedCellCount; }
    double GetIntegratedMeasure() const { return m_IntegratedMeasure; }
    std::string GetMeasureName() const;

protected:
    IntegrateVariablesFilter();
    ~IntegrateVariablesFilter() override = default;

private:
    bool m_DivideAllCellDataByMeasure{false};
    std::string m_Message;
    int m_IntegrationDimension{0};
    int m_IntegratedCellCount{0};
    double m_IntegratedMeasure{0.0};
};

IGAME_NAMESPACE_END

#endif
