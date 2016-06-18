#ifndef AXLIB_EVENT_H
#define AXLIB_EVENT_H

#include <pthread.h>
#include <queue>

struct ax_event;

#define EVENT_CALLBACK(name) void name(ax_event *Event)
typedef EVENT_CALLBACK(EventCallback);

/* NOTE(koekeishiya): Declare ax_event_type callbacks as external functions.
 *                    These callbacks should be defined in user-code as necessary. */
extern EVENT_CALLBACK(Callback_AXEvent_ApplicationLaunched);
extern EVENT_CALLBACK(Callback_AXEvent_ApplicationTerminated);
extern EVENT_CALLBACK(Callback_AXEvent_ApplicationActivated);

extern EVENT_CALLBACK(Callback_AXEvent_WindowCreated);
extern EVENT_CALLBACK(Callback_AXEvent_WindowDestroyed);
extern EVENT_CALLBACK(Callback_AXEvent_WindowFocused);
extern EVENT_CALLBACK(Callback_AXEvent_WindowMoved);
extern EVENT_CALLBACK(Callback_AXEvent_WindowResized);
extern EVENT_CALLBACK(Callback_AXEvent_WindowMinimized);
extern EVENT_CALLBACK(Callback_AXEvent_WindowDeminimized);

extern EVENT_CALLBACK(Callback_AXEvent_DisplayAdded);
extern EVENT_CALLBACK(Callback_AXEvent_DisplayRemoved);
extern EVENT_CALLBACK(Callback_AXEvent_DisplayMoved);
extern EVENT_CALLBACK(Callback_AXEvent_DisplayResized);

extern EVENT_CALLBACK(Callback_AXEvent_DisplayChanged);
extern EVENT_CALLBACK(Callback_AXEvent_SpaceChanged);

extern EVENT_CALLBACK(Callback_AXEvent_HotkeyPressed);
extern EVENT_CALLBACK(Callback_AXEvent_MouseMoved);

enum ax_event_type
{
    AXEvent_ApplicationLaunched,
    AXEvent_ApplicationTerminated,
    AXEvent_ApplicationActivated,

    AXEvent_WindowCreated,
    AXEvent_WindowDestroyed,

    AXEvent_WindowFocused,
    AXEvent_WindowMoved,
    AXEvent_WindowResized,
    AXEvent_WindowMinimized,
    AXEvent_WindowDeminimized,

    AXEvent_DisplayAdded,
    AXEvent_DisplayRemoved,
    AXEvent_DisplayMoved,
    AXEvent_DisplayResized,

    AXEvent_DisplayChanged,
    AXEvent_SpaceChanged,

    AXEvent_HotkeyPressed,
    AXEvent_MouseMoved,
};

struct ax_event
{
    ax_event_type Type;
    EventCallback *Handle;
    bool Intrinsic;
    void *Context;
};

struct ax_event_loop
{
    pthread_cond_t State;
    pthread_mutex_t StateLock;
    pthread_mutex_t WorkerLock;
    pthread_t Worker;
    bool Running;
    std::queue<ax_event> Queue;
};

bool AXLibStartEventLoop();
void AXLibStopEventLoop();
void AXLibAddEvent(ax_event Event);

/* NOTE(koekeishiya): Construct an ax_event with the appropriate callback through macro expansion. */
#define AXLibConstructEvent(EventType, EventContext, EventIntrinsic) \
    do { ax_event Event = {}; \
         Event.Type = EventType; \
         Event.Context = EventContext; \
         Event.Intrinsic = EventIntrinsic; \
         Event.Handle = &Callback_##EventType; \
         AXLibAddEvent(Event); \
       } while(0)

#endif
