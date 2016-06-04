#include "event.h"
#include <unistd.h>
#include <stdlib.h>

#define internal static

internal ax_event_loop EventLoop = {};

/* TODO(koekeishiya): Must be thread-safe. Called through AXLibConstructEvent macro */
void AXLibAddEvent(ax_event Event)
{
    if(Event.Handle)
        EventLoop.Queue.push(Event);
}

/* TODO(koekeishiya): Should event context be freed (?) */
internal void
AXLibDestroyEvent(ax_event *Event)
{
    if(Event->Context)
        free(Event->Context);

    Event->Handle = NULL;
    Event->Context = NULL;
}

/* NOTE(koekeishiya): Uses dynamic dispatch to process events of any type. */
internal void *
AXLibProcessEventQueue(void *)
{
    while(EventLoop.Running)
    {
        while(!EventLoop.Queue.empty())
        {
            ax_event Event = EventLoop.Queue.front();
            EventLoop.Queue.pop();

            (*Event.Handle)(&Event);
            AXLibDestroyEvent(&Event);
        }

        usleep(100000);
    }

    return NULL;
}

void AXLibStartEventLoop()
{
    if(!EventLoop.Running)
    {
        EventLoop.Running = true;
        pthread_create(&EventLoop.Worker, NULL, &AXLibProcessEventQueue, NULL);
    }
}

void AXLibStopEventLoop()
{
    if(EventLoop.Running)
    {
        EventLoop.Running = false;
        pthread_join(EventLoop.Worker, NULL);
    }
}
