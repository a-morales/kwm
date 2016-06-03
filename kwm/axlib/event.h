#ifndef AX_EVENT_H
#define AX_EVENT_H

#define EVENT_CALLBACK(name) void name(ax_event *Event)
typedef EVENT_CALLBACK(EventCallback);

enum ax_event_type
{
    AXEvent_None,
    AXEvent_Unknown,

    AXEvent_WindowCreated,
    AXEvent_WindowDestroyed,

    AXEvent_WindowMoved,
    AXEvent_WindowResized,
    AXEvent_WindowMinimized,
    AXEvent_WindowDeminimized,

    AXEvent_KeyDown,

    AXEvent_Count
};

struct ax_event
{
    ax_event_type Type;
    EventCallback *Handle;
    void *Context;
};

void AXLibConstructEvent(ax_event_type Type, void *Context);
void AXLibProcessEventQueue();

#endif
