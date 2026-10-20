#ifndef IGAME_DECIMATE_POLYLINE_FILTER_H
#define IGAME_DECIMATE_POLYLINE_FILTER_H

#include "iGameFilter.h"
#include <limits>
#include <string>

IGAME_NAMESPACE_BEGIN

class DecimatePolylineFilter : public Filter {
public:
    I_OBJECT(DecimatePolylineFilter);
    static Pointer New() { return new DecimatePolylineFilter; }

    enum class DecimationStrategy {
        Angle = 0,
        CustomField = 1,
        Distance = 2,
    };

    /** Desired fraction of points to remove, clamped to [0, 1]. */
    void SetTargetReduction(double value);
    double GetTargetReduction() const noexcept { return m_TargetReduction; }

    /** Largest removable-vertex error, clamped to the non-negative range. */
    void SetMaximumError(double value);
    double GetMaximumError() const noexcept { return m_MaximumError; }

    void SetDecimationStrategy(DecimationStrategy value) noexcept {
        m_DecimationStrategy = value;
    }
    DecimationStrategy GetDecimationStrategy() const noexcept {
        return m_DecimationStrategy;
    }

    /** Point-data array used by CustomField strategy. */
    void SetCustomFieldName(const std::string& value) { m_CustomFieldName = value; }
    const std::string& GetCustomFieldName() const noexcept { return m_CustomFieldName; }

    /** Return whether input contains explicit polyline cells this filter can process. */
    static bool CanProcessInput(DataObject::Pointer input);

    bool Execute() override;

protected:
    DecimatePolylineFilter();
    ~DecimatePolylineFilter() override = default;

private:
    double m_TargetReduction{0.9};
    double m_MaximumError{std::numeric_limits<double>::max()};
    DecimationStrategy m_DecimationStrategy{DecimationStrategy::Distance};
    std::string m_CustomFieldName;
};

IGAME_NAMESPACE_END
#endif
