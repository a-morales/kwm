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

void DisplayReconfigurationCallBack(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void *UserInfo)
{
    pthread_mutex_lock(&KWMThread.Lock);

    if (Flags & kCGDisplayAddFlag)
    {
        // Display has been added
        DEBUG("New display detected! DisplayID: " << Display << " Index: " << KWMScreen.ActiveCount)
        RefreshActiveDisplays();
    }
    else if (Flags & kCGDisplayRemoveFlag)
    {
        // Display has been removed
        if(CGDisplayIsAsleep(Display))
        {
            DEBUG("Display " << Display << " is asleep!")
        }
        else
        {
            DEBUG("Display has been removed! DisplayID: " << Display)
            std::map<int, space_info>::iterator It;
            for(It = KWMTiling.DisplayMap[Display].Space.begin(); It != KWMTiling.DisplayMap[Display].Space.end(); ++It)
                DestroyNodeTree(It->second.RootNode);

            if(KWMTiling.DisplayMap[Display].Identifier)
                CFRelease(KWMTiling.DisplayMap[Display].Identifier);

            KWMTiling.DisplayMap.erase(Display);
        }

        RefreshActiveDisplays();
    }
    else if (Flags & kCGDisplayDesktopShapeChangedFlag)
    {
        DEBUG("Display " << Display << " changed resolution")
        screen_info *Screen = &KWMTiling.DisplayMap[Display];
        UpdateExistingScreenInfo(Screen, Display, Screen->ID);
        std::map<int, space_info>::iterator It;
        for(It = Screen->Space.begin(); It != Screen->Space.end(); ++It)
            UpdateSpaceOfScreen(&It->second, Screen);
    }

    pthread_mutex_unlock(&KWMThread.Lock);
}

screen_info CreateDefaultScreenInfo(int DisplayIndex, int ScreenIndex)
{
    CGRect DisplayRect = CGDisplayBounds(DisplayIndex);
    screen_info Screen;

    Screen.Identifier = NULL;
    Screen.ID = ScreenIndex;
    Screen.ActiveSpace = -1;
    Screen.OldWindowListCount = -1;

    Screen.X = DisplayRect.origin.x;
    Screen.Y = DisplayRect.origin.y;
    Screen.Width = DisplayRect.size.width;
    Screen.Height = DisplayRect.size.height;

    Screen.Settings.Offset = KWMScreen.DefaultOffset;
    Screen.Settings.Mode = SpaceModeDefault;
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

void GetActiveDisplays()
{
    KWMScreen.Displays = (CGDirectDisplayID*) malloc(sizeof(CGDirectDisplayID) * KWMScreen.MaxCount);
    CGGetActiveDisplayList(KWMScreen.MaxCount, KWMScreen.Displays, &KWMScreen.ActiveCount);
    for(std::size_t DisplayIndex = 0; DisplayIndex < KWMScreen.ActiveCount; ++DisplayIndex)
    {
        unsigned int DisplayID = KWMScreen.Displays[DisplayIndex];
        KWMTiling.DisplayMap[DisplayID] = CreateDefaultScreenInfo(DisplayID, DisplayIndex);;

        DEBUG("DisplayID " << DisplayID << " has index " << DisplayIndex)
    }

    KWMScreen.Current = GetDisplayOfMousePointer();
    KWMScreen.Current->ActiveSpace = GetActiveSpaceOfDisplay(KWMScreen.Current);
    ShouldActiveSpaceBeManaged();

    CGDisplayRegisterReconfigurationCallback(DisplayReconfigurationCallBack, NULL);
}

void RefreshActiveDisplays()
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

        DEBUG("DisplayID " << DisplayID << " has index " << DisplayIndex)
    }

    screen_info *NewScreen = GetDisplayOfMousePointer();
    if(NewScreen)
        GiveFocusToScreen(NewScreen->ID, NULL, false, true);
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
        if(!IsApplicationFloating(&KWMTiling.WindowLst[WindowIndex]) &&
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
              " -> " << Screen->ActiveSpace)

        if(UpdateFocus)
        {
            if(Space->Initialized && FocusNode)
            {
                DEBUG("Populated Screen 'Window -f Focus'")

                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);
                SetWindowFocusByNode(FocusNode);
                MoveCursorToCenterOfFocusedWindow();
            }
            else if(Space->Initialized && Space->RootNode)
            {
                DEBUG("Populated Screen Key/Mouse Focus")

                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);

                bool WindowBelowCursor = IsAnyWindowBelowCursor();
                if(Mouse && !WindowBelowCursor)
                    ClearFocusedWindow();
                else if(Mouse && WindowBelowCursor)
                    FocusWindowBelowCursor();

                if(!Mouse)
                {
                    if(Space->FocusedWindowID == 0)
                    {
                        void *FocusNode = NULL;
                        GetFirstLeafNode(Space->RootNode, (void**)&FocusNode);
                        if(Space->Settings.Mode == SpaceModeBSP)
                            Space->FocusedWindowID = ((tree_node*)FocusNode)->WindowID;
                        else if(Space->Settings.Mode == SpaceModeMonocle)
                            Space->FocusedWindowID = ((link_node*)FocusNode)->WindowID;
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
                    DEBUG("Uninitialized Screen")
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

void MoveWindowToDisplay(window_info *Window, int Shift, bool Relative, bool UpdateFocus)
{
    int NewScreenIndex = -1;

    if(Relative)
        NewScreenIndex = Shift == 1 ? GetIndexOfNextScreen() : GetIndexOfPrevScreen();
    else
        NewScreenIndex = Shift;

    screen_info *NewScreen = GetDisplayFromScreenID(NewScreenIndex);
    if(NewScreen)
    {
        if(IsWindowFloating(Window->WID, NULL) || IsApplicationFloating(Window))
            CenterWindow(NewScreen, Window);
        else
            AddWindowToTreeOfUnfocusedMonitor(NewScreen, Window, UpdateFocus);
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
        DEBUG("UpdateActiveScreen() Active Display Changed")

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
