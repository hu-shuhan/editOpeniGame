#ifndef iGamePointCoordinatesFilter_h
#define iGamePointCoordinatesFilter_h

#include "iGameFilter.h"
#include "iGameFlatArray.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class PointCoordinatesFilter
 * @brief Exposes point coordinates as a three-component point-data array.
 *
 * The output is an independent deep copy of the input data object with an
 * additional IG_VECTOR attribute attached to its points. The original input
 * model is never modified.
 */
class PointCoordinatesFilter : public Filter {
public:
    I_OBJECT(PointCoordinatesFilter);
    static Pointer New() { return new PointCoordinatesFilter; }

    bool Execute() override;

    void SetArrayName(const std::string& name);
    const std::string& GetArrayName() const noexcept { return m_ArrayName; }

    FloatArray::Pointer GetCoordinatesArray() const { return m_CoordinatesArray; }

protected:
    PointCoordinatesFilter();
    ~PointCoordinatesFilter() override = default;

private:
    std::string m_ArrayName{"Coordinates"};
    FloatArray::Pointer m_CoordinatesArray{};
};

IGAME_NAMESPACE_END

#endif
