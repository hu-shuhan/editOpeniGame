#ifndef iGameCommand_h
#define iGameCommand_h

#include "iGameAbstractObject.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

#define iGameAllEventsMacro()    \
    add_event(ModifiedEvent)     \
    add_event(DeleteEvent)       \
    add_event(BeginEvent)        \
    add_event(EndEvent)          \
    add_event(UpdateEvent)       \
    add_event(ProgressEvent)     \
    add_event(ChangeSceneEvent)  \
    add_event(ChangeScalarNameEvent)  \
    add_event(PointPickedEvent)  \
    add_event(PointsSelectionEvent)  \
    add_event(FacePickedEvent)  \
    add_event(FacesSelectionEvent)  \

class Command : public AbstractObject {
public:
    I_OBJECT(Command)
    static Pointer New() { return new Command; }

#define add_event(Enum) Enum,
    enum EventIds
    {
        NoEvent = 0,
        iGameAllEventsMacro()
    };
#undef add_event

    virtual void Execute(Object* caller, unsigned long eventId, void* callData)
    {

    };

    static const char* GetStringFromEventId(unsigned long event);

    static unsigned long GetEventIdFromString(const char* event);

protected:
    Command() { };
    ~Command() { };

};

IGAME_NAMESPACE_END
#endif