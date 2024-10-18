/**
 * @class   iGameStressDeformationFilter
 * @brief   iGameStressDeformationFilter's brief
 */

#pragma once
#include "iGameDataObject.h"
#include "iGameFilter.h"
IGAME_NAMESPACE_BEGIN

class StressDeformationFilter : public Filter{
public:
I_OBJECT(StressDeformationFilter)
    static Pointer New() {
        return new StressDeformationFilter;
    };

    bool Execute() override;

/*
 *  DSF = K * D_model / U_max
 *  D_model = cbrt(D_x * D_y * D_z), D_{x, y, z} is the model's Bounding Box's max minus min.
 *  U_max: max vertex offset in the model.
 *  Here assume that K is simply equal to 0.1
 * */
    bool CalculateIdealDSF();

protected:
    StressDeformationFilter();
    ~StressDeformationFilter() = default;

protected:


};
IGAME_NAMESPACE_END
