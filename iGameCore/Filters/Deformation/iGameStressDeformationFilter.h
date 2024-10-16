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

//    Pointer GetOutput() override {
//        return Filter::GetOutput();
//    }

protected:
    StressDeformationFilter();
    ~StressDeformationFilter() = default;

protected:


};
IGAME_NAMESPACE_END
