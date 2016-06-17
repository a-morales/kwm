#include "carbon.h"
#include "event.h"
#include "application.h"

#include <string>

#define internal static
internal std::map<pid_t, ax_application> *Applications;

/* NOTE(koekeishiya): A pascal string has the size of the string stored as the first byte. */
internal void
CopyPascalStringToC(ConstStr255Param Source, char *Destination)
{
    strncpy(Destination, (char *) Source + 1, Source[0]);
    Destination[Source[0]] = '\0';
}

internal void
CarbonApplicationLaunched(ProcessSerialNumber PSN)
{
    Str255 ProcessName;
    ProcessInfoRec ProcessInfo = {};
    ProcessInfo.processInfoLength = sizeof(ProcessInfoRec);
    ProcessInfo.processName = ProcessName;

    /* NOTE(koekeishiya): Deprecated, consider switching to
     * CFDictionaryRef ProcessInformationCopyDictionary(const ProcessSerialNumber *PSN, UInt32 infoToReturn) */
    GetProcessInformation(&PSN, &ProcessInfo);

    /* TODO(koekeishiya): Check if we should care about this process. */
    if((ProcessInfo.processMode & modeOnlyBackground) != 0)
        return;

    char ProcessNameCString[256] = {0};
    if(ProcessInfo.processName)
        CopyPascalStringToC(ProcessInfo.processName, ProcessNameCString);

    pid_t PID = 0;
    GetProcessPID(&PSN, &PID);
    std::string Name = ProcessNameCString;
    /*
    printf("Carbon: Application launched %s\n", Name.c_str());
    printf("%d: modeReserved\n", ProcessInfo.processMode & modeReserved);
    printf("%d: modeControlPanel\n", ProcessInfo.processMode & modeControlPanel);
    printf("%d: modeLaunchDontSwitch\n", ProcessInfo.processMode & modeLaunchDontSwitch);
    printf("%d: modeDeskAccessory\n", ProcessInfo.processMode & modeDeskAccessory);
    printf("%d: modeMultiLaunch\n", ProcessInfo.processMode & modeMultiLaunch);
    printf("%d: modeNeedSuspendResume\n", ProcessInfo.processMode & modeNeedSuspendResume);
    printf("%d: modeCanBackground\n", ProcessInfo.processMode & modeCanBackground);
    printf("%d: modeDoesActivateOnFGSwitch\n", ProcessInfo.processMode & modeDoesActivateOnFGSwitch);
    printf("%d: modeOnlyBackground\n", ProcessInfo.processMode & modeOnlyBackground);
    printf("%d: modeGetFrontClicks\n", ProcessInfo.processMode & modeGetFrontClicks);
    printf("%d: modeGetAppDiedMsg\n", ProcessInfo.processMode & modeGetAppDiedMsg);
    printf("%d: mode32BitCompatible\n", ProcessInfo.processMode & mode32BitCompatible);
    printf("%d: modeHighLevelEventAware\n", ProcessInfo.processMode & modeHighLevelEventAware);
    printf("%d: modeLocalAndRemoteHLEvents\n", ProcessInfo.processMode & modeLocalAndRemoteHLEvents);
    printf("%d: modeStationeryAware\n", ProcessInfo.processMode & modeStationeryAware);
    printf("%d: modeUseTextEditServices\n", ProcessInfo.processMode & modeUseTextEditServices);
    printf("%d: modeDisplayManagerAware\n", ProcessInfo.processMode & modeDisplayManagerAware);
    */

    (*Applications)[PID] = AXLibConstructApplication(PID, Name);
    AXLibInitializeApplication(&(*Applications)[PID]);

    /* NOTE(koekeishiya): Some applications fail to properly create notifications upon launch.
                          Retry after a specific amount of time has passed. */
    if(AXLibHasApplicationObserverNotification(&(*Applications)[PID]))
    {
        AXLibConstructEvent(AXEvent_ApplicationLaunched, &(*Applications)[PID]);
    }
    else
    {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(),
        ^{
            AXLibAddApplicationObserverNotificationFallback(&(*Applications)[PID]);
        });
    }
}

internal void
CarbonApplicationTerminated(ProcessSerialNumber PSN)
{
    /* NOTE(koekeishiya): We probably want to have way to lookup process PIDs from the PSN */
    std::map<pid_t, ax_application>::iterator It;
    for(It = Applications->begin(); It != Applications->end(); ++It)
    {
        ax_application *Application = &It->second;
        if(Application->PSN.lowLongOfPSN == PSN.lowLongOfPSN &&
           Application->PSN.highLongOfPSN == PSN.highLongOfPSN)
        {
            // printf("Carbon: Application terminated %s\n", Application->Name.c_str());
            pid_t PID = Application->PID;
            AXLibDestroyApplication(Application);
            Applications->erase(PID);

            /* TODO(koekeishiya): We probably want to pass an identifier for this application. */
            AXLibConstructEvent(AXEvent_ApplicationTerminated, NULL);
        }
    }
}

internal OSStatus
CarbonApplicationEventHandler(EventHandlerCallRef HandlerCallRef, EventRef Event, void *Refcon)
{
    ProcessSerialNumber PSN;
    if(GetEventParameter(Event, kEventParamProcessID, typeProcessSerialNumber, NULL, sizeof(PSN), NULL, &PSN) != noErr)
    {
        printf("CarbonEventHandler: Could not get event parameter in application event\n");
        return -1;
    }

    UInt32 Type = GetEventKind(Event);
    switch(Type)
    {
        case kEventAppLaunched:
        {
            CarbonApplicationLaunched(PSN);
        } break;
        case kEventAppTerminated:
        {
            CarbonApplicationTerminated(PSN);
        } break;
    }

    return noErr;
}

bool AXLibInitializeCarbonEventHandler(carbon_event_handler *Carbon, std::map<pid_t, ax_application> *AXApplications)
{
    Applications = AXApplications;

    Carbon->EventTarget = GetApplicationEventTarget();
    Carbon->EventHandler = NewEventHandlerUPP(CarbonApplicationEventHandler);
    Carbon->EventType[0].eventClass = kEventClassApplication;
    Carbon->EventType[0].eventKind = kEventAppLaunched;
    Carbon->EventType[1].eventClass = kEventClassApplication;
    Carbon->EventType[1].eventKind = kEventAppTerminated;

    /* TODO(koekeishiya): Do we fallback to NSWorkspaceNotifications if we cannot install the Carbon handle, or simply abort (?) */
    return InstallEventHandler(Carbon->EventTarget, Carbon->EventHandler, 2, Carbon->EventType, NULL, &Carbon->CurHandler) == noErr;
}
