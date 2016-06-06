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

/* NOTE(koekeishiya): Initialize required mutexes and condition for the event-loop */
internal bool
AXLibInitializeEventLoop()
{
   if(pthread_mutex_init(&EventLoop.WorkerLock, NULL) != 0)
   {
       return false;
   }

   if(pthread_mutex_init(&EventLoop.StateLock, NULL) != 0)
   {
       pthread_mutex_destroy(&EventLoop.WorkerLock);
       return false;
   }

   if(pthread_cond_init(&EventLoop.State, NULL) != 0)
   {
        pthread_mutex_destroy(&EventLoop.WorkerLock);
        pthread_mutex_destroy(&EventLoop.StateLock);
        return false;
   }

   return true;
}

/* NOTE(koekeishiya): Destroy mutexes and condition used by the event-loop */
internal void
AXLibTerminateEventLoop()
{
    pthread_cond_destroy(&EventLoop.State);
    pthread_mutex_destroy(&EventLoop.StateLock);
    pthread_mutex_destroy(&EventLoop.WorkerLock);
}

bool AXLibStartEventLoop()
{
    if(!EventLoop.Running && AXLibInitializeEventLoop())
    {
        EventLoop.Running = true;
        pthread_create(&EventLoop.Worker, NULL, &AXLibProcessEventQueue, NULL);
        return true;
    }

    return false;
}

void AXLibStopEventLoop()
{
    if(EventLoop.Running)
    {
        EventLoop.Running = false;
        pthread_cond_signal(&EventLoop.State);
        pthread_join(EventLoop.Worker, NULL);
        AXLibTerminateEventLoop();
    }
}
