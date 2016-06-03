#ifndef AX_EVENT_H
#define AX_EVENT_H

struct ax_event;

#define EVENT_CALLBACK(name) void name(ax_event *Event)
typedef EVENT_CALLBACK(EventCallback);

extern EVENT_CALLBACK(Callback_AXEvent_WindowCreated);
extern EVENT_CALLBACK(Callback_AXEvent_WindowDestroyed);
extern EVENT_CALLBACK(Callback_AXEvent_WindowMoved);
extern EVENT_CALLBACK(Callback_AXEvent_WindowResized);
extern EVENT_CALLBACK(Callback_AXEvent_WindowMinimized);
extern EVENT_CALLBACK(Callback_AXEvent_WindowDeminimized);
extern EVENT_CALLBACK(Callback_AXEvent_KeyDown);
extern EVENT_CALLBACK(Callback_AXEvent_MouseMoved);


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
    AXEvent_MouseMoved,

    AXEvent_Count
};

struct ax_event
{
    ax_event_type Type;
    EventCallback *Handle;
    void *Context;
};

void AXLibAddEvent(ax_event Event);
void AXLibProcessEventQueue();

#define AXLibConstructEvent(EventType, EventContext) \
    do { ax_event Event = {}; \
         Event.Type = EventType; \
         Event.Context = EventContext; \
         if((EventType != AXEvent_None) && \
            (EventType != AXEvent_Unknown)) \
         { \
             Event.Handle = &Callback_##EventType; \
             AXLibAddEvent(Event); \
         } \
       } while(0)

#endif
