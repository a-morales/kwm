#import <Cocoa/Cocoa.h>
#import "sharedworkspace.h"
#include "event.h"

#define internal static
#define local_persist static

/* NOTE(koekeishiya): Subscribe to necessary notifications from NSWorkspace */
@interface WorkspaceWatcher : NSObject {
}
- (id)init;
@end

internal std::map<pid_t, ax_application> *Applications;
internal WorkspaceWatcher *Watcher;

void SharedWorkspaceInitialize(std::map<pid_t, ax_application> *Apps)
{
    Applications = Apps;
    Watcher = [[WorkspaceWatcher alloc] init];
}

std::map<pid_t, std::string> SharedWorkspaceRunningApplications()
{
    std::map<pid_t, std::string> List;

    for(NSRunningApplication *Application in [[NSWorkspace sharedWorkspace] runningApplications])
    {
        pid_t PID = Application.processIdentifier;

        std::string Name = "[Unknown]";
        const char *NamePtr = [[Application localizedName] UTF8String];
        if(NamePtr)
            Name = NamePtr;

        List[PID] = Name;
    }

    return List;
}

void SharedWorkspaceActivateApplication(pid_t PID)
{
    NSRunningApplication *Application = [NSRunningApplication runningApplicationWithProcessIdentifier:PID];
    if(Application)
        [Application activateWithOptions:NSApplicationActivateIgnoringOtherApps];
}

bool SharedWorkspaceIsApplicationActive(pid_t PID)
{
    Boolean Result = NO;
    NSRunningApplication *Application = [NSRunningApplication runningApplicationWithProcessIdentifier:PID];
    if(Application)
        Result = [Application isActive];

    return Result == YES;
}

internal void
SharedWorkspaceDidActivateApplication(pid_t PID)
{
    pid_t *ProcessID = (pid_t *) malloc(sizeof(pid_t));
    if(ProcessID)
    {
        *ProcessID = PID;
        if(Applications->find(*ProcessID) != Applications->end())
        {
            AXLibConstructEvent(AXEvent_ApplicationActivated, ProcessID);
        }
        else
        {
            free(ProcessID);
        }
    }
}

/* NOTE(koekeishiya): Subscribe to necessary notifications from NSWorkspace */
@implementation WorkspaceWatcher
- (id)init
{
    if ((self = [super init]))
    {
       [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                selector:@selector(activeSpaceDidChange:)
                name:NSWorkspaceActiveSpaceDidChangeNotification
                object:nil];

       [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                selector:@selector(didLaunchApplication:)
                name:NSWorkspaceDidLaunchApplicationNotification
                object:nil];

       [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                selector:@selector(didTerminateApplication:)
                name:NSWorkspaceDidTerminateApplicationNotification
                object:nil];

       [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                selector:@selector(didActivateApplication:)
                name:NSWorkspaceDidActivateApplicationNotification
                object:nil];
    }

    return self;
}

- (void)dealloc
{
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
    [super dealloc];
}

- (void)activeSpaceDidChange:(NSNotification *)notification
{
    AXLibConstructEvent(AXEvent_SpaceChanged, NULL);
}

- (void)didLaunchApplication:(NSNotification *)notification
{
    pid_t PID = [[notification.userInfo objectForKey:NSWorkspaceApplicationKey] processIdentifier];
    std::string Name = [[[notification.userInfo objectForKey:NSWorkspaceApplicationKey] localizedName] UTF8String];
    (*Applications)[PID] = AXLibConstructApplication(PID, Name);
    AXLibInitializeApplication(&(*Applications)[PID]);
    AXLibConstructEvent(AXEvent_ApplicationLaunched, NULL);

    /* NOTE(koekeishiya): When an application is launched for the first time, the 'didActivateApplication' notification
                          is triggered first for some reason. Restore consistency by calling the appropriate function manually,
                          when the notification can be processed properly. */
    SharedWorkspaceDidActivateApplication(PID);
}

- (void)didTerminateApplication:(NSNotification *)notification
{
    pid_t PID = [[notification.userInfo objectForKey:NSWorkspaceApplicationKey] processIdentifier];
    std::map<pid_t, ax_application>::iterator It = Applications->find(PID);
    if(It != Applications->end())
    {
        AXLibDestroyApplication(&It->second);
        Applications->erase(PID);
        // AXLibConstructEvent(AXEvent_ApplicationTerminated, NULL);
    }
}

- (void)didActivateApplication:(NSNotification *)notification
{
    pid_t PID = [[notification.userInfo objectForKey:NSWorkspaceApplicationKey] processIdentifier];
    SharedWorkspaceDidActivateApplication(PID);
}

@end
