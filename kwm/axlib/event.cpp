#include "event.h"
#include <queue>
#include <unistd.h>
#include <stdlib.h>

#define internal static

/* NOTE(koekeishiya): Replace with linked list (?) */
std::queue<ax_event> AXEventQueue;

/* TODO(koekeishiya): Construct an ax_event with the appropriate callback
 *                    through macro expansion.
 *
 *                    Remove me. */
internal void
TestEventMacro()
{
    AXLibConstructEvent(AXEvent_WindowCreated, NULL);
}

/* TODO(koekeishiya): Defines the callback for event_type AXEvent_WindowCreated.
 *                    These callbacks should be defined in user-code and has been
 *                    marked as external inside axlib/event.h */
EVENT_CALLBACK(Callback_AXEvent_WindowCreated)
{
}

/* TODO(koekeishiya): Must be thread-safe. Called through AXLibConstructEvent macro */
void AXLibAddEvent(ax_event Event)
{
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
