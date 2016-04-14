#import "Cocoa/Cocoa.h"
#import "types.h"
#include "notifications.h"

extern void UpdateActiveSpace();
extern screen_info *GetDisplayOfWindow(window_info *Window);
extern bool GetWindowFocusedByOSX(AXUIElementRef *WindowRef);
extern void SetKwmFocus(AXUIElementRef WindowRef);
extern void GiveFocusToScreen(unsigned int ScreenIndex, tree_node *Focus, bool Mouse, bool UpdateFocus);
extern window_info *GetWindowByID(int WindowID);
extern int GetWindowIDFromRef(AXUIElementRef WindowRef);
extern space_info *GetActiveSpaceOfScreen(screen_info *Screen);
extern tree_node *GetTreeNodeFromWindowIDOrLinkNode(tree_node *RootNode, int WindowID);
extern bool IsWindowFloating(int WindowID, int *Index);
extern bool IsFocusedWindowFloating();
extern void ClearFocusedWindow();
extern void ClearMarkedWindow();
extern bool FocusWindowOfOSX();

extern kwm_focus KWMFocus;
extern kwm_screen KWMScreen;
extern kwm_thread KWMThread;

@interface WorkspaceWatcher : NSObject {
}
- (id)init;
@end

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
                selector:@selector(didActivateApplication:)
                name:NSWorkspaceDidActivateApplicationNotification
                object:nil];

       [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                selector:@selector(didLaunchApplication:)
                name:NSWorkspaceDidLaunchApplicationNotification
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
    UpdateActiveSpace();
}

- (void)didLaunchApplication:(NSNotification *)notification
{
    FocusWindowOfOSX();
}

- (void)didActivateApplication:(NSNotification *)notification
{
    pthread_mutex_lock(&KWMThread.Lock);

    pid_t ProcessID = [[notification.userInfo objectForKey:NSWorkspaceApplicationKey] processIdentifier];
    if(ProcessID != -1)
    {
        window_info *Window = KWMFocus.Window;
        screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
        if((Window && Window->PID != ProcessID) || !Window)
        {
            AXUIElementRef OSXWindowRef;
            if(GetWindowFocusedByOSX(&OSXWindowRef))
            {
                window_info *OSXWindow = GetWindowByID(GetWindowIDFromRef(OSXWindowRef));
                screen_info *OSXScreen = GetDisplayOfWindow(OSXWindow);
                if(OSXWindow && OSXScreen)
                {
                    space_info *OSXSpace = GetActiveSpaceOfScreen(OSXScreen);
                    tree_node *TreeNode = GetTreeNodeFromWindowIDOrLinkNode(OSXSpace->RootNode, OSXWindow->WID);

                    bool Floating = IsWindowFloating(OSXWindow->WID, NULL) || OSXWindow->Float;
                    if(TreeNode || Floating)
                    {
                        bool SameScreen = OSXScreen == ScreenOfWindow;
                        if(!SameScreen)
                            GiveFocusToScreen(OSXScreen->ID, NULL, false, false);

                        if(OSXSpace->Managed)
                        {
                            SetKwmFocus(OSXWindowRef);
                            if(SameScreen && !Floating)
                                KWMFocus.InsertionPoint = KWMFocus.Cache;
                        }
                    }
                    else
                    {
                        ClearFocusedWindow();
                        ClearMarkedWindow();
                        DestroyApplicationNotifications();
                    }
                }
            }
            else
            {
                ClearFocusedWindow();
                ClearMarkedWindow();
                DestroyApplicationNotifications();
            }
        }
    }

    pthread_mutex_unlock(&KWMThread.Lock);
}

@end

void CreateWorkspaceWatcher(void *Watcher)
{
    WorkspaceWatcher *WSWatcher = [[WorkspaceWatcher alloc] init];
    Watcher = (void*)WSWatcher;
}

CFStringRef GetDisplayIdentifier(screen_info *Screen)
{
    if(Screen->Identifier)
        return Screen->Identifier;

    CGRect Frame = CGRectMake(Screen->X, Screen->Y, Screen->Width, Screen->Height);
    Screen->Identifier = CGSCopyBestManagedDisplayForRect(CGSDefaultConnection, Frame);
    return Screen->Identifier;
}

int GetActiveSpaceOfDisplay(screen_info *Screen)
{
    int CurrentSpace = -1;
    NSString *CurrentIdentifier = (__bridge NSString *)GetDisplayIdentifier(Screen);

    CFArrayRef ScreenDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *ScreenDictionary in (__bridge NSArray *)ScreenDictionaries)
    {
        NSString *ScreenIdentifier = ScreenDictionary[@"Display Identifier"];
        if ([ScreenIdentifier isEqualToString:CurrentIdentifier])
        {
            CurrentSpace = [ScreenDictionary[@"Current Space"][@"id64"] intValue];
            break;
        }
    }

    CFRelease(ScreenDictionaries);
    return CurrentSpace;
}

int GetNumberOfSpacesOfDisplay(screen_info *Screen)
{
    int Result = 0;
    NSString *CurrentIdentifier = (__bridge NSString *)GetDisplayIdentifier(Screen);

    CFArrayRef ScreenDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *ScreenDictionary in (__bridge NSArray *)ScreenDictionaries)
    {
        NSString *ScreenIdentifier = ScreenDictionary[@"Display Identifier"];
        if ([ScreenIdentifier isEqualToString:CurrentIdentifier])
        {
            NSArray *Spaces = ScreenDictionary[@"Spaces"];
            Result = CFArrayGetCount((__bridge CFArrayRef)Spaces);
        }
    }

    CFRelease(ScreenDictionaries);
    return Result;
}

int GetSpaceNumberFromCGSpaceID(screen_info *Screen, int CGSpaceID)
{
    int Result = -1;
    NSString *CurrentIdentifier = (__bridge NSString *)GetDisplayIdentifier(Screen);

    CFArrayRef ScreenDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *ScreenDictionary in (__bridge NSArray *)ScreenDictionaries)
    {
        int SpaceIndex = 1;
        NSString *ScreenIdentifier = ScreenDictionary[@"Display Identifier"];
        if ([ScreenIdentifier isEqualToString:CurrentIdentifier])
        {
            NSArray *Spaces = ScreenDictionary[@"Spaces"];
            for(NSDictionary *SpaceDictionary in (__bridge NSArray *)Spaces)
            {
                int CurrentSpace = [SpaceDictionary[@"id64"] intValue];
                if(CurrentSpace == CGSpaceID)
                {
                    Result = SpaceIndex;
                    break;
                }

                ++SpaceIndex;
            }
        }
    }

    CFRelease(ScreenDictionaries);
    return Result;
}

int GetCGSpaceIDFromSpaceNumber(screen_info *Screen, int SpaceID)
{
    int Result = -1;
    NSString *CurrentIdentifier = (__bridge NSString *)GetDisplayIdentifier(Screen);

    CFArrayRef ScreenDictionaries = CGSCopyManagedDisplaySpaces(CGSDefaultConnection);
    for(NSDictionary *ScreenDictionary in (__bridge NSArray *)ScreenDictionaries)
    {
        int SpaceIndex = 1;
        NSString *ScreenIdentifier = ScreenDictionary[@"Display Identifier"];
        if ([ScreenIdentifier isEqualToString:CurrentIdentifier])
        {
            NSArray *Spaces = ScreenDictionary[@"Spaces"];
            for(NSDictionary *SpaceDictionary in (__bridge NSArray *)Spaces)
            {
                if(SpaceIndex == SpaceID)
                {
                    Result = [SpaceDictionary[@"id64"] intValue];
                    break;
                }
                ++SpaceIndex;
            }
        }
    }

    CFRelease(ScreenDictionaries);
    return Result;
}

extern "C" int CGSRemoveWindowsFromSpaces(int cid, CFArrayRef windows, CFArrayRef spaces);
extern "C" int CGSAddWindowsToSpaces(int cid, CFArrayRef windows, CFArrayRef spaces);
extern "C" void CGSHideSpaces(int cid, CFArrayRef spaces);
extern "C" void CGSShowSpaces(int cid, CFArrayRef spaces);
extern "C" void CGSManagedDisplaySetIsAnimating(int cid, CFStringRef display, bool animating);
extern "C" void CGSManagedDisplaySetCurrentSpace(int cid, CFStringRef display, int space);

void ActivateSpaceWithoutTransition(std::string SpaceID)
{
    if(KWMScreen.Current)
    {
        int TotalSpaces = GetNumberOfSpacesOfDisplay(KWMScreen.Current);
        int ActiveSpace = GetSpaceNumberFromCGSpaceID(KWMScreen.Current, KWMScreen.Current->ActiveSpace);
        int DestinationSpaceID = ActiveSpace;
        if(SpaceID == "left")
            DestinationSpaceID = ActiveSpace > 1 ? ActiveSpace-1 : 1;
        else if(SpaceID == "right")
            DestinationSpaceID = ActiveSpace < TotalSpaces ? ActiveSpace+1 : TotalSpaces;
        else
            DestinationSpaceID = std::atoi(SpaceID.c_str());

        if(DestinationSpaceID != ActiveSpace &&
           DestinationSpaceID > 0 && DestinationSpaceID <= TotalSpaces)
        {
            int CGSpaceID = GetCGSpaceIDFromSpaceNumber(KWMScreen.Current, DestinationSpaceID);
            NSArray *NSArraySourceSpace = @[ @(KWMScreen.Current->ActiveSpace) ];
            NSArray *NSArrayDestinationSpace = @[ @(CGSpaceID) ];
            CGSManagedDisplaySetIsAnimating(CGSDefaultConnection, KWMScreen.Current->Identifier, true);
            CGSShowSpaces(CGSDefaultConnection, (__bridge CFArrayRef)NSArrayDestinationSpace);
            CGSHideSpaces(CGSDefaultConnection, (__bridge CFArrayRef)NSArraySourceSpace);
            CGSManagedDisplaySetCurrentSpace(CGSDefaultConnection, KWMScreen.Current->Identifier, CGSpaceID);
            CGSManagedDisplaySetIsAnimating(CGSDefaultConnection, KWMScreen.Current->Identifier, false);
        }
    }
}

void RemoveWindowFromSpace(int SpaceID, int WindowID)
{
    NSArray *NSArrayWindow = @[ @(WindowID) ];
    NSArray *NSArraySourceSpace = @[ @(SpaceID) ];
    CGSRemoveWindowsFromSpaces(CGSDefaultConnection, (__bridge CFArrayRef)NSArrayWindow, (__bridge CFArrayRef)NSArraySourceSpace);
}

void AddWindowToSpace(int SpaceID, int WindowID)
{
    NSArray *NSArrayWindow = @[ @(WindowID) ];
    NSArray *NSArrayDestinationSpace = @[ @(SpaceID) ];
    CGSAddWindowsToSpaces(CGSDefaultConnection, (__bridge CFArrayRef)NSArrayWindow, (__bridge CFArrayRef)NSArrayDestinationSpace);
}

void MoveWindowBetweenSpaces(int SourceSpaceID, int DestinationSpaceID, int WindowID)
{
    int SourceCGSpaceID = GetCGSpaceIDFromSpaceNumber(KWMScreen.Current, SourceSpaceID);
    int DestinationCGSpaceID = GetCGSpaceIDFromSpaceNumber(KWMScreen.Current, DestinationSpaceID);
    RemoveWindowFromSpace(SourceCGSpaceID, WindowID);
    AddWindowToSpace(DestinationCGSpaceID, WindowID);
}

void MoveFocusedWindowToSpace(std::string SpaceID)
{
    if(KWMScreen.Current && KWMFocus.Window)
    {
        int TotalSpaces = GetNumberOfSpacesOfDisplay(KWMScreen.Current);
        int ActiveSpace = GetSpaceNumberFromCGSpaceID(KWMScreen.Current, KWMScreen.Current->ActiveSpace);
        int DestinationSpaceID = ActiveSpace;
        if(SpaceID == "left")
            DestinationSpaceID = ActiveSpace > 1 ? ActiveSpace-1 : 1;
        else if(SpaceID == "right")
            DestinationSpaceID = ActiveSpace < TotalSpaces ? ActiveSpace+1 : TotalSpaces;
        else
            DestinationSpaceID = std::atoi(SpaceID.c_str());

        MoveWindowBetweenSpaces(ActiveSpace, DestinationSpaceID, KWMFocus.Window->WID);
    }
}
