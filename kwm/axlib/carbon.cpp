#include "carbon.h"
#include "event.h"
#include "application.h"

#include <string>

#define internal static
internal std::map<pid_t, ax_application> *Applications;

/* TODO(koekeishiya): The processes we observe can be of the following types.
                      We probably do not care about tracking daemons.
   enum {
       modeReserved = 0x01000000,
       modeControlPanel = 0x00080000,
       modeLaunchDontSwitch = 0x00040000,
       modeDeskAccessory = 0x00020000,
       modeMultiLaunch = 0x00010000,
       modeNeedSuspendResume = 0x00004000,
       modeCanBackground = 0x00001000,
       modeDoesActivateOnFGSwitch = 0x00000800,
       modeOnlyBackground = 0x00000400,
       modeGetFrontClicks = 0x00000200,
       modeGetAppDiedMsg = 0x00000100,
       mode32BitCompatible = 0x00000080,
       modeHighLevelEventAware = 0x00000040,
       modeLocalAndRemoteHLEvents = 0x00000020,
       modeStationeryAware = 0x00000010,
       modeUseTextEditServices = 0x00000008,
       modeDisplayManagerAware = 0x00000004
};

*/

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
    /* if((ProcessInfo.processMode & modeOnlyBackground) != 0)
        return;
    */

    char ProcessNameCString[256] = {0};
    if(ProcessInfo.processName)
        CopyPascalStringToC(ProcessInfo.processName, ProcessNameCString);


    pid_t PID = 0;
    GetProcessPID(&PSN, &PID);
    std::string Name = ProcessNameCString;
    printf("Carbon: Application launched %s\n", Name.c_str());

    (*Applications)[PID] = AXLibConstructApplication(PID, Name);
    AXLibInitializeApplication(&(*Applications)[PID]);

    AXLibConstructEvent(AXEvent_ApplicationLaunched, &(*Applications)[PID]);
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
            printf("Carbon: Application terminated %s\n", Application->Name.c_str());
            pid_t PID = Application->PID;
            AXLibDestroyApplication(Application);
            Applications->erase(PID);

            /* TODO(koekeishiya): We probably want to pass an identifier for this application. */
            // AXLibConstructEvent(AXEvent_ApplicationTerminated, NULL);
        }
    }
}

internal OSStatus
CarbonApplicationEventHandler(EventHandlerCallRef HandlerCallRef, EventRef Event, void *Refcon)
{
    UInt32 Type = GetEventKind(Event);
    ProcessSerialNumber PSN;
    if(GetEventParameter(Event, kEventParamProcessID, typeProcessSerialNumber, NULL, sizeof(PSN), NULL, &PSN) != noErr) {
        printf("CarbonEventHandler: Could not get event parameter in application event\n");
        return -1;
    }

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

    if(InstallEventHandler(Carbon->EventTarget,
                           Carbon->EventHandler, 2,
                           Carbon->EventType, NULL,
                           &Carbon->CurHandler) != noErr)
    {
        return false;
    }

    return true;
}
