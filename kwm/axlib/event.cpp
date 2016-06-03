#include "event.h"
#include <queue>
#include <unistd.h>

#define internal static

/* NOTE(koekeishiya): Replace with linked list (?) */
std::queue<ax_event> AXEventQueue;

EVENT_CALLBACK(AXEventWindowCreated);
EVENT_CALLBACK(AXEventWindowDestroyed);

EVENT_CALLBACK(AXEventWindowMoved);
EVENT_CALLBACK(AXEventWindowResized);
EVENT_CALLBACK(AXEventWindowMinimized);
EVENT_CALLBACK(AXEventWindowDeminimized);

EVENT_CALLBACK(AXEventKeyDown);

/* TODO(koekeishiya): Must be thread-safe */
void AXLibConstructEvent(ax_event_type Type, void *Context)
{
    ax_event Event = {};
    Event.Type = Type;
    Event.Context = Context;

    switch(Type)
    {
        case AXEvent_None:
        {
            printf("%d: This should not happen!\n", Type);
        } break;
        case AXEvent_Unknown:
        {
            printf("%d: Unknown\n", Type);
        } break;
        case AXEvent_WindowCreated:
        {
            printf("%d: WindowCreated\n", Type);
            Event.Handle = &AXEventWindowCreated;
        } break;
        case AXEvent_WindowDestroyed:
        {
            printf("%d: WindowDestroyed\n", Type);
            Event.Handle = &AXEventWindowDestroyed;
        } break;
        case AXEvent_WindowMoved:
        {
            printf("%d: WindowMoved\n", Type);
            Event.Handle = &AXEventWindowMoved;
        } break;
        case AXEvent_WindowResized:
        {
            printf("%d: WindowResized\n", Type);
            Event.Handle = &AXEventWindowResized;
        } break;
        case AXEvent_WindowMinimized:
        {
            printf("%d: WindowMinimized\n", Type);
            Event.Handle = &AXEventWindowMinimized;
        } break;
        case AXEvent_WindowDeminimized:
        {
            printf("%d: WindowDeminimized\n", Type);
            Event.Handle = &AXEventWindowDeminimized;
        } break;
        case AXEvent_KeyDown
        {
            printf("%d: KeyDown\n", Type);
            Event.Handle = &AXEventKeyDown;
        } break;
        default:
        {
            printf("%d: NYI\n", Type);
        } break;
    }

    if(Event.Handle)
        AXEventQueue.push(Event);
}

/* TODO(koekeishiya): Free event context after processing. Should the
 *                    callback function be responsible for this (?) */
internal void
AXLibDestroyEvent(ax_event *Event)
{
    free(Event->Context);
    Event->Handle = NULL;
    Event->Context = NULL;
    Event->Type = AXEvent_None;
}

/* TODO(koekeishiya): Uses dynamic dispatch to process events of any type.
                      Should this run on a separate thread by default (?) */
void AXLibProcessEventQueue()
{
    for(;;)
    {
        while(!AXEventQueue.empty())
        {
            ax_event Event = AXEventQueue.front();
            AXEventQueue.pop();

            (*Event.Handle)(&Event);
            AXLibDestroyEvent(&Event);
        }

        usleep(100000);
    }
}
