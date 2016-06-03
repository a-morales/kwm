#include "event.h"
#include <queue>
#include <unistd.h>
#include <stdlib.h>

#define internal static

/* NOTE(koekeishiya): Replace with linked list (?) */
internal std::queue<ax_event> AXEventQueue;

/* TODO(koekeishiya): Must be thread-safe. Called through AXLibConstructEvent macro */
void AXLibAddEvent(ax_event Event)
{
    if(Event.Handle)
        AXEventQueue.push(Event);
}

/* NOTE(koekeishiya): Free event context after processing. */
internal void
AXLibDestroyEvent(ax_event *Event)
{
    if(Event->Context)
        free(Event->Context);

    Event->Handle = NULL;
    Event->Context = NULL;
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
