/**
 * @class   iGameRandomVectors
 * @brief   "Random Vectors" filter
 *          Adds a random 3-component vector (BrownianVectors) to each point
 *          of the input mesh. The direction is a random unit vector and the
 *          magnitude is uniformly distributed in [minimumSpeed, maximumSpeed].
 */

#pragma once

#include "iGameFilter.h"
#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN

class RandomVectorsFilter : public Filter {
public:
    I_OBJECT(RandomVectorsFilter)
    static Pointer New() { return new RandomVectorsFilter; }

    bool Execute() override;

    void SetMinimumSpeed(double speed);
    void SetMaximumSpeed(double speed);
    double GetMinimumSpeed() const;
    double GetMaximumSpeed() const;

protected:
    RandomVectorsFilter();
    ~RandomVectorsFilter() override = default;

protected:
    double m_MinimumSpeed{0.0};
    double m_MaximumSpeed{1.0};
};

IGAME_NAMESPACE_END
