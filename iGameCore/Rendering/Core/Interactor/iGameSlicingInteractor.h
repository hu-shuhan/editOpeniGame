#ifndef OPENIGAME_SLICING_INTERACTOR_H
#define OPENIGAME_SLICING_INTERACTOR_H

#include "iGameInteractor.h"

IGAME_NAMESPACE_BEGIN

class SlicingInteractor : public Interactor {
public:
    I_OBJECT(SlicingInteractor);
    static Pointer New() { return new SlicingInteractor; }

protected:
    SlicingInteractor() = default;
    ~SlicingInteractor() override = default;
};

IGAME_NAMESPACE_END

#endif //OPENIGAME_INTERACTOR_H
