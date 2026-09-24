#ifndef IGAMEVIS_RESAMPLETOLINE_INTERACTOR_H
#define IGAMEVIS_RESAMPLETOLINE_INTERACTOR_H

#include"iGameInteractor.h"

IGAME_NAMESPACE_BEGIN

class ResampleToLineInteractor : public Interactor {
public:
    I_OBJECT(ResampleToLineInteractor);
    static Pointer New() { return new ResampleToLineInteractor; }

    protected:
    ResampleToLineInteractor() = default;
        ~ResampleToLineInteractor() override = default;
};

IGAME_NAMESPACE_END

#endif //IGAMEVIS_INTRERACTOR_H
