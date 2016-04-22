#include "display.h"
#include "space.h"
#include "container.h"
#include "tree.h"
#include "application.h"
#include "window.h"
#include "helpers.h"

extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_tiling KWMTiling;
extern kwm_thread KWMThread;
extern kwm_toggles KWMToggles;

int GetDisplayIDFromIdentifier(CFStringRef Identifier)
{
    for(auto it = KWMTiling.DisplayMap.begin(); it != KWMTiling.DisplayMap.end(); it++)
    {
        CFStringRef activeID = it->second.Identifier;
        CFComparisonResult result = CFStringCompare(Identifier, activeID, 0);
        if (result == kCFCompareEqualTo)
            return it->first;
    }
    return 0;
}

void UpdateDisplayIDForDisplay(int oldDisplayIdx, int newDisplayIdx)
{
    auto it = KWMTiling.DisplayMap.find(oldDisplayIdx);
    screen_info Screen = it->second;
    KWMTiling.DisplayMap.erase(it);
    KWMTiling.DisplayMap[newDisplayIdx] = Screen;
}

void DisplayReconfigurationCallBack(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void *UserInfo)
{
    pthread_mutex_lock(&KWMThread.Lock);
    static int idx = 0;
    DEBUG("\n[" << idx << "] BEGIN DISPLAY CONFIGURATION CALLBACK");

    if (Flags & kCGDisplayAddFlag)
    {
        CFStringRef displayIdentifier = GetDisplayIdentifier(Display);
        int savedDisplayID = 0;
        if(displayIdentifier)
            savedDisplayID = GetDisplayIDFromIdentifier(displayIdentifier);
        if (!savedDisplayID) {
            DEBUG("New display detected! DisplayID: " << Display << " Index: " << KWMScreen.ActiveCount);
        }
        else if (savedDisplayID != Display) {
            DEBUG("This display being added had saved ID: " << savedDisplayID);
            UpdateDisplayIDForDisplay(savedDisplayID, Display);
        }
        else {
            DEBUG("The display that is being added is already there!");
        }
        // Display has been added
        RefreshActiveDisplays(true);
    }
    if (Flags & kCGDisplayRemoveFlag)
    {
        CFStringRef displayIdentifier = GetDisplayIdentifier(Display);
        int savedDisplayID = 0;
        if(displayIdentifier)
            savedDisplayID = GetDisplayIDFromIdentifier(displayIdentifier);
        if ((!savedDisplayID) || (savedDisplayID == Display)) {
            // Display has been removed
            if(CGDisplayIsAsleep(Display))
            {
                DEBUG("Display " << Display << " is asleep!");
                RefreshActiveDisplays(false);
            }
            else
            {
                DEBUG("Display has been removed! DisplayID: " << Display);
                std::map<int, space_info>::iterator It;
                for(It = KWMTiling.DisplayMap[Display].Space.begin(); It != KWMTiling.DisplayMap[Display].Space.end(); ++It)
                    DestroyNodeTree(It->second.RootNode);

                if(KWMTiling.DisplayMap[Display].Identifier)
                    CFRelease(KWMTiling.DisplayMap[Display].Identifier);

                KWMTiling.DisplayMap.erase(Display);
                RefreshActiveDisplays(true);
            }
        }
        else if (savedDisplayID != Display) {
            DEBUG("This display being removed had saved ID: " << savedDisplayID);
            RefreshActiveDisplays(true);
        }
    }
    if ((Flags & kCGDisplayDesktopShapeChangedFlag) || (Flags & kCGDisplayMovedFlag))
    {
        if(!CGDisplayIsAsleep(Display))
        {
            if (Flags & kCGDisplayDesktopShapeChangedFlag) {
                DEBUG("Display " << Display << " changed resolution");
            }
            else {
                DEBUG("Display " << Display << " moved");
            }
            screen_info *Screen = &KWMTiling.DisplayMap[Display];
            UpdateExistingScreenInfo(Screen, Display, Screen->ID);
            std::map<int, space_info>::iterator It;
            for(It = Screen->Space.begin(); It != Screen->Space.end(); ++It)
            {
                if(It->second.Managed || It->second.Settings.Mode == SpaceModeFloating)
                {
                    if(It->first == Screen->ActiveSpace)
                        UpdateSpaceOfScreen(&It->second, Screen);
                    else
                        It->second.NeedsUpdate = true;
                }
            }
        }
    }
    if (Flags & kCGDisplaySetMainFlag)
    {
        DEBUG("Display " << Display << " became main");
    }
    if (Flags & kCGDisplaySetModeFlag)
    {
        DEBUG("Display " << Display << " changed mode");
    }
    if (Flags & kCGDisplayEnabledFlag)
    {
        DEBUG("Display " << Display << " enabled");
    }
    if (Flags & kCGDisplayDisabledFlag)
    {
        DEBUG("Display " << Display << " disabled");
    }

    DEBUG("[" << idx << "] END DISPLAY CONFIGURATION CALLBACK\n");
    idx++;
    pthread_mutex_unlock(&KWMThread.Lock);
}

screen_info CreateDefaultScreenInfo(int DisplayIndex, int ScreenIndex)
{
    CGRect DisplayRect = CGDisplayBounds(DisplayIndex);
    screen_info Screen;

    Screen.Identifier = GetDisplayIdentifier(DisplayIndex);
    Screen.ID = ScreenIndex;
    Screen.ActiveSpace = -1;
    Screen.TrackSpaceChange = true;

    Screen.X = DisplayRect.origin.x;
    Screen.Y = DisplayRect.origin.y;
    Screen.Width = DisplayRect.size.width;
    Screen.Height = DisplayRect.size.height;

    Screen.Settings.Offset = KWMScreen.DefaultOffset;
    Screen.Settings.Mode = SpaceModeDefault;

    DEBUG("Creating screen info for display ID: " << DisplayIndex << " Resolution: " << Screen.Width << "x" << Screen.Height << " Origin: (" << Screen.X << "," << Screen.Y << ")");

    return Screen;
}

void UpdateExistingScreenInfo(screen_info *Screen, int DisplayIndex, int ScreenIndex)
{
    CGRect DisplayRect = CGDisplayBounds(DisplayIndex);
    Screen->ID = ScreenIndex;

    Screen->X = DisplayRect.origin.x;
    Screen->Y = DisplayRect.origin.y;
    Screen->Width = DisplayRect.size.width;
    Screen->Height = DisplayRect.size.height;

    Screen->Settings.Offset = KWMScreen.DefaultOffset;
    Screen->Settings.Mode = SpaceModeDefault;
}

CFStringRef GetDisplayIdentifier(int DisplayID)
{
    CFUUIDRef displayUUID = CGDisplayCreateUUIDFromDisplayID(DisplayID);
    if (!displayUUID)
        return NULL;
    CFStringRef Identifier = CFUUIDCreateString(NULL, displayUUID);
    CFRelease(displayUUID);
    return Identifier;
}

void GetActiveDisplays()
{
    KWMScreen.Displays = (CGDirectDisplayID*) malloc(sizeof(CGDirectDisplayID) * KWMScreen.MaxCount);
    CGGetActiveDisplayList(KWMScreen.MaxCount, KWMScreen.Displays, &KWMScreen.ActiveCount);
    for(std::size_t DisplayIndex = 0; DisplayIndex < KWMScreen.ActiveCount; ++DisplayIndex)
    {
        unsigned int DisplayID = KWMScreen.Displays[DisplayIndex];
        KWMTiling.DisplayMap[DisplayID] = CreateDefaultScreenInfo(DisplayID, DisplayIndex);;

        DEBUG("DisplayID " << DisplayID << " has index " << DisplayIndex << " and Identifier " << CFStringGetCStringPtr(KWMTiling.DisplayMap[DisplayID].Identifier,kCFStringEncodingUTF8));
    }

    KWMScreen.Current = GetDisplayOfMousePointer();
    KWMScreen.Current->ActiveSpace = GetActiveSpaceOfDisplay(KWMScreen.Current);
    ShouldActiveSpaceBeManaged();

    CGDisplayRegisterReconfigurationCallback(DisplayReconfigurationCallBack, NULL);
}

void RefreshActiveDisplays(bool ShouldFocusScreen)
{
    if(KWMScreen.Displays)
        free(KWMScreen.Displays);

    KWMScreen.Displays = (CGDirectDisplayID*) malloc(sizeof(CGDirectDisplayID) * KWMScreen.MaxCount);
    CGGetActiveDisplayList(KWMScreen.MaxCount, KWMScreen.Displays, &KWMScreen.ActiveCount);
    for(std::size_t DisplayIndex = 0; DisplayIndex < KWMScreen.ActiveCount; ++DisplayIndex)
    {
        unsigned int DisplayID = KWMScreen.Displays[DisplayIndex];
        std::map<unsigned int, screen_info>::iterator It = KWMTiling.DisplayMap.find(DisplayID);

        if(It != KWMTiling.DisplayMap.end())
            UpdateExistingScreenInfo(&KWMTiling.DisplayMap[DisplayID], DisplayID, DisplayIndex);
        else
            KWMTiling.DisplayMap[DisplayID] = CreateDefaultScreenInfo(DisplayID, DisplayIndex);

        DEBUG("DisplayID " << DisplayID << " has index " << DisplayIndex << " and Identifier " << CFStringGetCStringPtr(KWMTiling.DisplayMap[DisplayID].Identifier,kCFStringEncodingUTF8));
    }

    if (ShouldFocusScreen)
    {
        screen_info *NewScreen = GetDisplayOfMousePointer();
        if(NewScreen)
            GiveFocusToScreen(NewScreen->ID, NULL, false, true);
    }
}

screen_info *GetDisplayFromScreenID(unsigned int ID)
{
    std::map<unsigned int, screen_info>::iterator It;
    for(It = KWMTiling.DisplayMap.begin(); It != KWMTiling.DisplayMap.end(); ++It)
    {
        screen_info *Screen = &It->second;
        if(Screen->ID == ID)
            return Screen;
    }

    return NULL;
}

screen_info *GetDisplayOfMousePointer()
{
    std::map<unsigned int, screen_info>::iterator It;
    for(It = KWMTiling.DisplayMap.begin(); It != KWMTiling.DisplayMap.end(); ++It)
    {
        CGPoint Cursor = GetCursorPos();
        screen_info *Screen = &It->second;
        if(Cursor.x >= Screen->X && Cursor.x <= Screen->X + Screen->Width &&
           Cursor.y >= Screen->Y && Cursor.y <= Screen->Y + Screen->Height)
               return Screen;
    }

    return NULL;
}

screen_info *GetDisplayOfWindow(window_info *Window)
{
    if(Window)
    {
        std::map<unsigned int, screen_info>::iterator It;
        for(It = KWMTiling.DisplayMap.begin(); It != KWMTiling.DisplayMap.end(); ++It)
        {
            screen_info *Screen = &It->second;
            if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width &&
               Window->Y >= Screen->Y && Window->Y <= Screen->Y + Screen->Height)
                return Screen;
        }

        return GetDisplayOfMousePointer();
    }

    return NULL;
}

std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex)
{
    screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
    std::vector<window_info*> ScreenWindowLst;
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.WindowLst.size(); ++WindowIndex)
    {
        window_info *Window = &KWMTiling.WindowLst[WindowIndex];
        if(IsWindowTilable(Window) &&
           !IsApplicationFloating(&KWMTiling.WindowLst[WindowIndex]) &&
           !IsWindowFloating(KWMTiling.WindowLst[WindowIndex].WID, NULL))
        {
            if(Screen == GetDisplayOfWindow(Window))
                ScreenWindowLst.push_back(Window);
        }
    }

    return ScreenWindowLst;
}

void UpdateSpaceOfScreen(space_info *Space, screen_info *Screen)
{
    if(Space->RootNode)
    {
        if(Space->Settings.Mode == SpaceModeBSP)
        {
            SetRootNodeContainer(Screen, Space->RootNode);
            CreateNodeContainers(Screen, Space->RootNode, true);
        }
        else if(Space->Settings.Mode == SpaceModeMonocle)
        {
            link_node *Link = Space->RootNode->List;
            while(Link)
            {
                SetLinkNodeContainer(Screen, Link);
                Link = Link->Next;
            }
        }

        ApplyTreeNodeContainer(Space->RootNode);
        Space->NeedsUpdate = false;
    }
}

void SetDefaultPaddingOfDisplay(container_offset Offset)
{
    KWMScreen.DefaultOffset.PaddingTop = Offset.PaddingTop;
    KWMScreen.DefaultOffset.PaddingBottom = Offset.PaddingBottom;
    KWMScreen.DefaultOffset.PaddingLeft = Offset.PaddingLeft;
    KWMScreen.DefaultOffset.PaddingRight = Offset.PaddingRight;
}

void SetDefaultGapOfDisplay(container_offset Offset)
{
    KWMScreen.DefaultOffset.VerticalGap = Offset.VerticalGap;
    KWMScreen.DefaultOffset.HorizontalGap = Offset.HorizontalGap;
}

void ChangePaddingOfDisplay(const std::string &Side, int Offset)
{
    screen_info *Screen = GetDisplayOfMousePointer();
    space_info *Space = GetActiveSpaceOfScreen(Screen);

    if(Side == "all")
    {
        if(Space->Settings.Offset.PaddingLeft + Offset >= 0)
            Space->Settings.Offset.PaddingLeft += Offset;

        if(Space->Settings.Offset.PaddingRight + Offset >= 0)
            Space->Settings.Offset.PaddingRight += Offset;

        if(Space->Settings.Offset.PaddingTop + Offset >= 0)
            Space->Settings.Offset.PaddingTop += Offset;

        if(Space->Settings.Offset.PaddingBottom + Offset >= 0)
            Space->Settings.Offset.PaddingBottom += Offset;
    }
    else if(Side == "left")
    {
        if(Space->Settings.Offset.PaddingLeft + Offset >= 0)
            Space->Settings.Offset.PaddingLeft += Offset;
    }
    else if(Side == "right")
    {
        if(Space->Settings.Offset.PaddingRight + Offset >= 0)
            Space->Settings.Offset.PaddingRight += Offset;
    }
    else if(Side == "top")
    {
        if(Space->Settings.Offset.PaddingTop + Offset >= 0)
            Space->Settings.Offset.PaddingTop += Offset;
    }
    else if(Side == "bottom")
    {
        if(Space->Settings.Offset.PaddingBottom + Offset >= 0)
            Space->Settings.Offset.PaddingBottom += Offset;
    }

    UpdateSpaceOfScreen(Space, Screen);
}

void ChangeGapOfDisplay(const std::string &Side, int Offset)
{
    screen_info *Screen = GetDisplayOfMousePointer();
    space_info *Space = GetActiveSpaceOfScreen(Screen);

    if(Side == "all")
    {
        if(Space->Settings.Offset.VerticalGap + Offset >= 0)
            Space->Settings.Offset.VerticalGap += Offset;

        if(Space->Settings.Offset.HorizontalGap + Offset >= 0)
            Space->Settings.Offset.HorizontalGap += Offset;
    }
    else if(Side == "vertical")
    {
        if(Space->Settings.Offset.VerticalGap + Offset >= 0)
            Space->Settings.Offset.VerticalGap += Offset;
    }
    else if(Side == "horizontal")
    {
        if(Space->Settings.Offset.HorizontalGap + Offset >= 0)
            Space->Settings.Offset.HorizontalGap += Offset;
    }

    UpdateSpaceOfScreen(Space, Screen);
}

int GetIndexOfNextScreen()
{
    return KWMScreen.Current->ID + 1 >= KWMScreen.ActiveCount ? 0 : KWMScreen.Current->ID + 1;
}

int GetIndexOfPrevScreen()
{
    return KWMScreen.Current->ID == 0 ? KWMScreen.ActiveCount - 1 : KWMScreen.Current->ID - 1;
}

void GiveFocusToScreen(unsigned int ScreenIndex, tree_node *FocusNode, bool Mouse, bool UpdateFocus)
{
    screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
    if(Screen && Screen != KWMScreen.Current)
    {
        KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
        KWMScreen.Current = Screen;

        Screen->ActiveSpace = GetActiveSpaceOfDisplay(Screen);
        ShouldActiveSpaceBeManaged();
        space_info *Space = GetActiveSpaceOfScreen(Screen);

        DEBUG("GiveFocusToScreen() " << ScreenIndex << \
              ": Space transition ended " << KWMScreen.PrevSpace << \
              " -> " << Screen->ActiveSpace);

        if(UpdateFocus)
        {
            if(Space->Initialized && FocusNode)
            {
                DEBUG("Populated Screen 'Window -f Focus'");

                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);
                SetWindowFocusByNode(FocusNode);
                MoveCursorToCenterOfFocusedWindow();
            }
            else if(Space->Initialized && Space->RootNode)
            {
                DEBUG("Populated Screen Key/Mouse Focus");

                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);

                bool WindowBelowCursor = IsAnyWindowBelowCursor();
                if(Mouse && !WindowBelowCursor)
                    ClearFocusedWindow();
                else if(Mouse && WindowBelowCursor)
                    FocusWindowBelowCursor();

                if(!Mouse)
                {
                    if(Space->FocusedWindowID == -1)
                    {
                        if(Space->Settings.Mode == SpaceModeBSP)
                        {
                            void *FocusNode = NULL;
                            GetFirstLeafNode(Space->RootNode, (void**)&FocusNode);
                            Space->FocusedWindowID = ((tree_node*)FocusNode)->WindowID;
                        }
                        else if(Space->Settings.Mode == SpaceModeMonocle)
                        {
                            if(Space->RootNode->List)
                                Space->FocusedWindowID = Space->RootNode->List->WindowID;
                        }
                    }

                    FocusWindowByID(Space->FocusedWindowID);
                    MoveCursorToCenterOfFocusedWindow();
                }
            }
            else
            {
                if(!Space->Initialized ||
                   Space->Settings.Mode == SpaceModeFloating ||
                   !Space->RootNode)
                {
                    DEBUG("Uninitialized Screen");
                    ClearFocusedWindow();

                    if(!Mouse)
                        CGWarpMouseCursorPosition(CGPointMake(Screen->X + (Screen->Width / 2), Screen->Y + (Screen->Height / 2)));

                    if(Space->Settings.Mode != SpaceModeFloating && !Space->RootNode)
                    {
                        CGPoint ClickPos = GetCursorPos();
                        CGEventRef ClickEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, ClickPos, kCGMouseButtonLeft);
                        CGEventSetFlags(ClickEvent, 0);
                        CGEventPost(kCGHIDEventTap, ClickEvent);
                        CFRelease(ClickEvent);

                        ClickEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, ClickPos, kCGMouseButtonLeft);
                        CGEventSetFlags(ClickEvent, 0);
                        CGEventPost(kCGHIDEventTap, ClickEvent);
                        CFRelease(ClickEvent);
                    }
                }
            }
        }
    }
}

void MoveWindowToDisplay(window_info *Window, int Shift, bool Relative)
{
    int NewScreenIndex = -1;

    if(Relative)
        NewScreenIndex = Shift == 1 ? GetIndexOfNextScreen() : GetIndexOfPrevScreen();
    else
        NewScreenIndex = Shift;

    screen_info *NewScreen = GetDisplayFromScreenID(NewScreenIndex);
    if(NewScreen && NewScreen != KWMScreen.Current)
    {
        space_info *SpaceOfWindow = GetActiveSpaceOfScreen(KWMScreen.Current);
        SpaceOfWindow->FocusedWindowID = -1;

        if(IsWindowFloating(Window->WID, NULL) || IsApplicationFloating(Window))
            CenterWindow(NewScreen, Window);
        else
            AddWindowToTreeOfUnfocusedMonitor(NewScreen, Window);
    }
}

container_offset CreateDefaultScreenOffset()
{
    container_offset Offset = { 40, 20, 20, 20, 10, 10 };
    return Offset;
}

void UpdateActiveScreen()
{
    screen_info *Screen = GetDisplayOfMousePointer();
    if(Screen && KWMScreen.Current && KWMScreen.Current != Screen)
    {
        DEBUG("UpdateActiveScreen() Active Display Changed");

        ClearMarkedWindow();
        GiveFocusToScreen(Screen->ID, NULL, true, true);
    }
}

space_settings *GetSpaceSettingsForDisplay(unsigned int ScreenID)
{
    std::map<unsigned int, space_settings>::iterator It = KWMTiling.DisplaySettings.find(ScreenID);
    if(It != KWMTiling.DisplaySettings.end())
        return &It->second;
    else
        return NULL;
}
