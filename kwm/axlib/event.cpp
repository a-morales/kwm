#include "event.h"
#include <unistd.h>
#include <stdlib.h>

#define internal static

internal ax_event_loop EventLoop = {};

/* NOTE(koekeishiya): Must be thread-safe! Called through AXLibConstructEvent macro */
void AXLibAddEvent(ax_event Event)
{
    if(EventLoop.Running && Event.Handle)
    {
        pthread_mutex_lock(&EventLoop.WorkerLock);
        EventLoop.Queue.push(Event);

        pthread_cond_signal(&EventLoop.State);
        pthread_mutex_unlock(&EventLoop.WorkerLock);
    }
}

/* NOTE(koekeishiya): This function is currently unused and should probably be removed (?)
                      The callback is responsible for cleaning up the context provided for an event. */
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
        pthread_mutex_lock(&EventLoop.StateLock);
        while(!EventLoop.Queue.empty())
        {
            pthread_mutex_lock(&EventLoop.WorkerLock);
            ax_event Event = EventLoop.Queue.front();
            EventLoop.Queue.pop();
            pthread_mutex_unlock(&EventLoop.WorkerLock);

            (*Event.Handle)(&Event);
        }

        while(EventLoop.Queue.empty() && EventLoop.Running)
            pthread_cond_wait(&EventLoop.State, &EventLoop.StateLock);

        pthread_mutex_unlock(&EventLoop.StateLock);
    }

    return NULL;
}

void AXLibStartEventLoop()
{
    if((!EventLoop.Running) &&
       (pthread_mutex_init(&EventLoop.WorkerLock, NULL) == 0) &&
       (pthread_mutex_init(&EventLoop.StateLock, NULL) == 0) &&
       (pthread_cond_init(&EventLoop.State, NULL) == 0))
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
        pthread_cond_signal(&EventLoop.State);
        pthread_join(EventLoop.Worker, NULL);

        pthread_mutex_destroy(&EventLoop.WorkerLock);
        pthread_mutex_destroy(&EventLoop.StateLock);
        pthread_cond_destroy(&EventLoop.State);
    }
}
