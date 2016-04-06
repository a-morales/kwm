#include "space.h"
#include "display.h"
#include "window.h"
#include "tree.h"
#include "border.h"
#include "keys.h"
#include "notifications.h"
#include "helpers.h"

extern kwm_mach KWMMach;
extern kwm_tiling KWMTiling;
extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_toggles KWMToggles;
extern kwm_mode KWMMode;
extern kwm_thread KWMThread;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;

void GetTagForMonocleSpace(space_info *Space, std::string &Tag)
{
    tree_node *Node = Space->RootNode;
    bool FoundFocusedWindow = false;
    int FocusedIndex = 0;
    int NumberOfWindows = 0;

    if(Node && KWMFocus.Window)
    {
        link_node *Link = Node->List;
        while(Link)
        {
            if(!FoundFocusedWindow)
                ++FocusedIndex;

            if(Link->WindowID == KWMFocus.Window->WID)
                FoundFocusedWindow = true;

            ++NumberOfWindows;
            Link = Link->Next;
        }
    }

    if(FoundFocusedWindow)
        Tag = "[" + std::to_string(FocusedIndex) + "/" + std::to_string(NumberOfWindows) + "]";
    else
        Tag = "[" + std::to_string(NumberOfWindows) + "]";
}

void GetTagForCurrentSpace(std::string &Tag)
{
    if(IsSpaceInitializedForScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Settings.Mode == SpaceModeBSP)
            Tag = "[bsp]";
        else if(Space->Settings.Mode == SpaceModeFloating)
            Tag = "[float]";
        else if(Space->Settings.Mode == SpaceModeMonocle)
            GetTagForMonocleSpace(Space, Tag);
    }
    else
    {
        if(KWMMode.Space == SpaceModeBSP)
            Tag = "[bsp]";
        else if(KWMMode.Space == SpaceModeFloating)
            Tag = "[float]";
        else if(KWMMode.Space == SpaceModeMonocle)
            Tag = "[monocle]";
    }
}

void MoveWindowToSpace(std::string SpaceID)
{
    if(!KWMFocus.Window)
        return;

    pthread_mutex_lock(&KWMThread.Lock);
    CGEventTapEnable(KWMMach.EventTap, false);
    DestroyApplicationNotifications();

    window_info *Window = KWMFocus.Window;
    bool WasWindowFloating = IsFocusedWindowFloating();
    if(!WasWindowFloating)
    {
        screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
        space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
        if(SpaceOfWindow->Settings.Mode == SpaceModeBSP)
            RemoveWindowFromBSPTree(ScreenOfWindow, Window->WID, false, false);
        else if(SpaceOfWindow->Settings.Mode == SpaceModeMonocle)
            RemoveWindowFromMonocleTree(ScreenOfWindow, Window->WID, false);
    }

    CGPoint CursorPos = GetCursorPos();
    CGPoint WindowPos = GetWindowPos(Window);
    CGPoint ClickPos = CGPointMake(WindowPos.x + 75, WindowPos.y + 7);

    CGEventRef ClickEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, ClickPos, kCGMouseButtonLeft);
    CGEventSetFlags(ClickEvent, 0);
    CGEventPost(kCGHIDEventTap, ClickEvent);
    CFRelease(ClickEvent);
    usleep(150000);

    KwmEmitKeystroke(KWMHotkeys.SpacesKey, SpaceID);
    usleep(300000);

    KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
    KWMScreen.Current->ActiveSpace = GetActiveSpaceOfDisplay(KWMScreen.Current);
    ShouldActiveSpaceBeManaged();
    UpdateActiveWindowList(KWMScreen.Current);

    ClickEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, ClickPos, kCGMouseButtonLeft);
    CGEventSetFlags(ClickEvent, 0);
    CGEventPost(kCGHIDEventTap, ClickEvent);
    CFRelease(ClickEvent);
    usleep(150000);

    if(!WasWindowFloating)
    {
        screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
        space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
        if(SpaceOfWindow->Settings.Mode == SpaceModeBSP)
            AddWindowToBSPTree(ScreenOfWindow, Window->WID);
        else if(SpaceOfWindow->Settings.Mode == SpaceModeMonocle)
            AddWindowToMonocleTree(ScreenOfWindow, Window->WID);
    }

    CGWarpMouseCursorPosition(CursorPos);
    CreateApplicationNotifications();
    CGEventTapEnable(KWMMach.EventTap, true);
    pthread_mutex_unlock(&KWMThread.Lock);
}

bool IsActiveSpaceFloating()
{
    return IsSpaceFloating(KWMScreen.Current->ActiveSpace);
}

bool IsSpaceFloating(int SpaceID)
{
    bool Result = false;

    if(IsSpaceInitializedForScreen(KWMScreen.Current))
    {
        std::map<int, space_info>::iterator It = KWMScreen.Current->Space.find(SpaceID);
        if(It != KWMScreen.Current->Space.end())
            Result = KWMScreen.Current->Space[SpaceID].Settings.Mode == SpaceModeFloating;
    }

    return Result;
}

space_info *GetActiveSpaceOfScreen(screen_info *Screen)
{
    space_info *Space = NULL;
    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);

    if(It == Screen->Space.end())
    {
        space_info Clear = {{{0}}};
        Screen->ActiveSpace = GetActiveSpaceOfDisplay(Screen);
        Screen->Space[Screen->ActiveSpace] = Clear;
        Space = &Screen->Space[Screen->ActiveSpace];
    }
    else
    {
        Space = &Screen->Space[Screen->ActiveSpace];
    }

    return Space;
}

bool IsSpaceInitializedForScreen(screen_info *Screen)
{
    if(!Screen)
        return false;

    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);
    if(It == Screen->Space.end())
        return false;
    else
        return It->second.Initialized;
}

bool DoesSpaceExistInMapOfScreen(screen_info *Screen)
{
    if(!Screen)
        return false;

    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);
    if(It == Screen->Space.end())
        return false;
    else
        return It->second.RootNode != NULL && It->second.Initialized;
}

bool IsSpaceTransitionInProgress()
{
    if(KWMScreen.Transitioning)
        return true;

    bool Result = false;
    std::map<unsigned int, screen_info>::iterator It;
    for(It = KWMTiling.DisplayMap.begin(); It != KWMTiling.DisplayMap.end(); ++It)
    {
        screen_info *Screen = &It->second;
        if(!Screen->Identifier)
            Screen->Identifier = GetDisplayIdentifier(Screen);

        Result = Result || CGSManagedDisplayIsAnimating(CGSDefaultConnection, Screen->Identifier);
    }

    if(Result)
    {
        DEBUG("IsSpaceTransitionInProgress() Space transition detected")
        KWMScreen.Transitioning = true;
        ClearFocusedWindow();
        ClearMarkedWindow();
    }

    return Result;
}

bool IsActiveSpaceManaged()
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    return Space->Managed;
}

void ShouldActiveSpaceBeManaged()
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    Space->Managed = CGSSpaceGetType(CGSDefaultConnection, KWMScreen.Current->ActiveSpace) == CGSSpaceTypeUser;
}

void FloatFocusedSpace()
{
    if(KWMToggles.EnableTilingMode &&
       !IsSpaceTransitionInProgress() &&
       IsActiveSpaceManaged())
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Settings.Mode == SpaceModeFloating)
            return;

        DestroyNodeTree(Space->RootNode);
        Space->RootNode = NULL;

        Space->Settings.Mode = SpaceModeFloating;
        Space->Initialized = true;
        ClearFocusedWindow();
    }
}

void TileFocusedSpace(space_tiling_option Mode)
{
    if(KWMToggles.EnableTilingMode &&
       !IsSpaceTransitionInProgress() &&
       IsActiveSpaceManaged() &&
       FilterWindowList(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Settings.Mode == Mode)
            return;

        DestroyNodeTree(Space->RootNode);
        Space->RootNode = NULL;

        Space->Settings.Mode = Mode;
        std::vector<window_info*> WindowsOnDisplay = GetAllWindowsOnDisplay(KWMScreen.Current->ID);
        CreateWindowNodeTree(KWMScreen.Current, &WindowsOnDisplay);
    }
}

void UpdateActiveSpace()
{
    pthread_mutex_lock(&KWMThread.Lock);
    Assert(KWMScreen.Current)

    KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
    KWMScreen.Current->ActiveSpace = GetActiveSpaceOfDisplay(KWMScreen.Current);
    ShouldActiveSpaceBeManaged();

    space_info *Space = NULL;
    if(KWMScreen.PrevSpace != KWMScreen.Current->ActiveSpace)
    {
        DEBUG("UpdateActiveSpace() Space transition ended " << KWMScreen.PrevSpace << " -> " << KWMScreen.Current->ActiveSpace)

        Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        UpdateActiveWindowList(KWMScreen.Current);
        if(Space->FocusedWindowID != 0)
        {
            FocusWindowByID(Space->FocusedWindowID);
            MoveCursorToCenterOfFocusedWindow();
        }
    }
    else
    {
        std::map<unsigned int, screen_info>::iterator It;
        for(It = KWMTiling.DisplayMap.begin(); It != KWMTiling.DisplayMap.end(); ++It)
        {
            screen_info *Screen = &It->second;
            if(Screen->ID == KWMScreen.Current->ID)
                continue;

            int ScreenCurrentSpace = Screen->ActiveSpace;
            int ScreenNewSpace = GetActiveSpaceOfDisplay(Screen);
            if(ScreenCurrentSpace != ScreenNewSpace)
            {
                DEBUG("space changed on monitor: " << Screen->ID)

                Screen->ActiveSpace = ScreenNewSpace;
                KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
                KWMScreen.Current = Screen;
                ShouldActiveSpaceBeManaged();
                Space = GetActiveSpaceOfScreen(Screen);
                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);
                break;
            }
        }
    }

    if(!Space || !Space->Managed || Space->Settings.Mode == SpaceModeFloating)
    {
        ClearFocusedWindow();
        ClearMarkedWindow();
        DestroyApplicationNotifications();
    }

    KWMScreen.Transitioning = false;
    pthread_mutex_unlock(&KWMThread.Lock);
}

space_settings *GetSpaceSettingsForDesktopID(int ScreenID, int DesktopID)
{
    space_identifier Lookup = { ScreenID, DesktopID };
    std::map<space_identifier, space_settings>::iterator It = KWMTiling.SpaceSettings.find(Lookup);
    if(It != KWMTiling.SpaceSettings.end())
        return &It->second;
    else
        return NULL;
}
