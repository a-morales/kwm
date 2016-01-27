#include "border.h"
#include "display.h"
#include "space.h"
#include "tree.h"
#include "window.h"

extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_tiling KWMTiling;
extern kwm_thread KWMThread;
extern kwm_toggles KWMToggles;
extern kwm_border FocusedBorder;

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
                DestroyNodeTree(It->second.RootNode, It->second.Mode);

            KWMTiling.DisplayMap.erase(Display);
        }

        RefreshActiveDisplays();
    }

    pthread_mutex_unlock(&KWMThread.Lock);
}

screen_info CreateDefaultScreenInfo(int DisplayIndex, int ScreenIndex)
{
    CGRect DisplayRect = CGDisplayBounds(DisplayIndex);
    screen_info Screen;

    Screen.ID = ScreenIndex;
    Screen.ForceContainerUpdate = false;
    Screen.ForceSpaceUpdate = false;
    Screen.ActiveSpace = -1;
    Screen.OldWindowListCount = -1;

    Screen.X = DisplayRect.origin.x;
    Screen.Y = DisplayRect.origin.y;
    Screen.Width = DisplayRect.size.width;
    Screen.Height = DisplayRect.size.height;

    Screen.Offset = KWMScreen.DefaultOffset;
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

    Screen->Offset = KWMScreen.DefaultOffset;
    Screen->ForceContainerUpdate = true;
    Screen->ForceSpaceUpdate = true;
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
        GiveFocusToScreen(NewScreen->ID, NULL, false);
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
            if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
                return Screen;
        }
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
        if(!IsApplicationFloating(&KWMTiling.WindowLst[WindowIndex]))
        {
            if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
                ScreenWindowLst.push_back(Window);
        }
    }

    return ScreenWindowLst;
}

std::vector<int> GetAllWindowIDsOnDisplay(int ScreenIndex)
{
    screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
    std::vector<int> ScreenWindowIDLst;
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.WindowLst.size(); ++WindowIndex)
    {
        window_info *Window = &KWMTiling.WindowLst[WindowIndex];
        if(!IsApplicationFloating(&KWMTiling.WindowLst[WindowIndex]))
        {
            if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
                ScreenWindowIDLst.push_back(Window->WID);
        }
    }

    return ScreenWindowIDLst;
}

void SetDefaultPaddingOfDisplay(const std::string &Side, int Offset)
{
    if(Side == "left")
        KWMScreen.DefaultOffset.PaddingLeft = Offset;
    else if(Side == "right")
        KWMScreen.DefaultOffset.PaddingRight = Offset;
    else if(Side == "top")
        KWMScreen.DefaultOffset.PaddingTop = Offset;
    else if(Side == "bottom")
        KWMScreen.DefaultOffset.PaddingBottom = Offset;
}

void SetDefaultGapOfDisplay(const std::string &Side, int Offset)
{
    if(Side == "vertical")
        KWMScreen.DefaultOffset.VerticalGap = Offset;
    else if(Side == "horizontal")
        KWMScreen.DefaultOffset.HorizontalGap = Offset;
}

void ChangePaddingOfDisplay(const std::string &Side, int Offset)
{
    screen_info *Screen = GetDisplayOfMousePointer();
    space_info *Space = &Screen->Space[Screen->ActiveSpace];

    if(Side == "left")
    {
        if(Space->Offset.PaddingLeft + Offset >= 0)
            Space->Offset.PaddingLeft += Offset;
    }
    else if(Side == "right")
    {
        if(Space->Offset.PaddingRight + Offset >= 0)
            Space->Offset.PaddingRight += Offset;
    }
    else if(Side == "top")
    {
        if(Space->Offset.PaddingTop + Offset >= 0)
            Space->Offset.PaddingTop += Offset;
    }
    else if(Side == "bottom")
    {
        if(Space->Offset.PaddingBottom + Offset >= 0)
            Space->Offset.PaddingBottom += Offset;
    }

    if(Space->RootNode)
    {
        if(Space->Mode == SpaceModeBSP)
        {
            SetRootNodeContainer(Screen, Space->RootNode);
            CreateNodeContainers(Screen, Space->RootNode, true);
        }
        else if(Space->Mode == SpaceModeMonocle)
        {
            tree_node *CurrentNode = Space->RootNode;
            while(CurrentNode)
            {
                SetRootNodeContainer(Screen, CurrentNode);
                CurrentNode = CurrentNode->RightChild;
            }
        }

        ApplyNodeContainer(Space->RootNode, Space->Mode);
    }
}

void ChangeGapOfDisplay(const std::string &Side, int Offset)
{
    screen_info *Screen = GetDisplayOfMousePointer();
    space_info *Space = &Screen->Space[Screen->ActiveSpace];

    if(Side == "vertical")
    {
        if(Space->Offset.VerticalGap + Offset >= 0)
            Space->Offset.VerticalGap += Offset;
    }
    else if(Side == "horizontal")
    {
        if(Space->Offset.HorizontalGap + Offset >= 0)
            Space->Offset.HorizontalGap += Offset;
    }

    if(Space->RootNode && Space->Mode == SpaceModeBSP)
    {
        CreateNodeContainers(Screen, Space->RootNode, true);
        ApplyNodeContainer(Space->RootNode, Space->Mode);
    }
}

void SetSpaceModeOfDisplay(unsigned int ScreenIndex, std::string Mode)
{
    if(Mode == "bsp")
        KWMTiling.DisplayMode[ScreenIndex] = SpaceModeBSP;
    else if(Mode == "monocle")
        KWMTiling.DisplayMode[ScreenIndex] = SpaceModeMonocle;
    else if(Mode == "float")
        KWMTiling.DisplayMode[ScreenIndex] = SpaceModeFloating;
}

space_tiling_option GetSpaceModeOfDisplay(unsigned int ScreenIndex)
{
    std::map<unsigned int, space_tiling_option>::iterator It = KWMTiling.DisplayMode.find(ScreenIndex);
    if(It == KWMTiling.DisplayMode.end())
        return SpaceModeDefault;
    else
        return It->second;
}

int GetIndexOfNextScreen()
{
    return KWMScreen.Current->ID + 1 >= KWMScreen.ActiveCount ? 0 : KWMScreen.Current->ID + 1;
}

int GetIndexOfPrevScreen()
{
    return KWMScreen.Current->ID == 0 ? KWMScreen.ActiveCount - 1 : KWMScreen.Current->ID - 1;
}

void ActivateScreen(screen_info *Screen, bool Mouse)
{
    CGPoint CursorPos = CGPointMake(Screen->X + (Screen->Width / 2), Screen->Y + (Screen->Height / 2));
    CGPoint ClickPos = CursorPos;

    if(Mouse)
        CursorPos = GetCursorPos();

    CGEventRef MoveEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, ClickPos, kCGMouseButtonLeft);
    CGEventSetFlags(MoveEvent, 0);
    CGEventPost(kCGHIDEventTap, MoveEvent);
    CFRelease(MoveEvent);

    MoveEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, ClickPos, kCGMouseButtonLeft);
    CGEventSetFlags(MoveEvent, 0);
    CGEventPost(kCGHIDEventTap, MoveEvent);
    CFRelease(MoveEvent);

    MoveEvent = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, CursorPos, kCGMouseButtonLeft);
    CGEventSetFlags(MoveEvent, 0);
    CGEventPost(kCGHIDEventTap, MoveEvent);
    CFRelease(MoveEvent);
}

void GiveFocusToScreen(int ScreenIndex, tree_node *Focus, bool Mouse)
{
    screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
    if(Screen)
    {
        DEBUG("GiveFocusToScreen() " << ScreenIndex)
        tree_node *FocusFirstNode = NULL;
        bool Initialized = IsSpaceInitializedForScreen(Screen);
        if(Initialized)
            FocusFirstNode = GetFirstLeafNode(Screen->Space[Screen->ActiveSpace].RootNode);

        if(Initialized && Focus)
        {
            DEBUG("Populated Screen 'Window -f Focus'")
            if(KWMScreen.PrevSpace != Screen->ActiveSpace)
                DEBUG("UpdateActiveWindowList() Space transition ended " << KWMScreen.PrevSpace << " -> " << Screen->ActiveSpace)

            KWMScreen.PrevSpace = Screen->ActiveSpace;
            KWMScreen.OldScreenID = Screen->ID;
            KWMScreen.Current = Screen;

            CGWarpMouseCursorPosition(CGPointMake(Focus->Container.X + Focus->Container.Width / 2,
                                                  Focus->Container.Y + Focus->Container.Height / 2));

            UpdateActiveWindowList(Screen);
            FilterWindowList(Screen);
            KWMScreen.ForceRefreshFocus = true;
            FocusWindowBelowCursor();
            KWMScreen.ForceRefreshFocus = false;
        }
        else if(Initialized && FocusFirstNode)
        {
            DEBUG("Populated Screen Key/Mouse Focus")
            if(KWMScreen.PrevSpace != Screen->ActiveSpace)
                DEBUG("UpdateActiveWindowList() Space transition ended " << KWMScreen.PrevSpace << " -> " << Screen->ActiveSpace)

            KWMScreen.PrevSpace = Screen->ActiveSpace;
            KWMScreen.OldScreenID = Screen->ID;
            KWMScreen.Current = Screen;

            bool WindowBelowCursor = IsAnyWindowBelowCursor();
            if(Mouse && !WindowBelowCursor)
            {
                ActivateScreen(Screen, Mouse);
                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);
            }
            else if(Mouse && WindowBelowCursor)
            {
                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);
                FocusWindowBelowCursor();
            }
            else
            {
                CGWarpMouseCursorPosition(CGPointMake(FocusFirstNode->Container.X + FocusFirstNode->Container.Width / 2,
                                                      FocusFirstNode->Container.Y + FocusFirstNode->Container.Height / 2));
                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);
            }

            KWMScreen.ForceRefreshFocus = true;
            FocusWindowBelowCursor();
            KWMScreen.ForceRefreshFocus = false;
        }
        else
        {
            if(!Initialized ||
               Screen->Space[Screen->ActiveSpace].Mode == SpaceModeFloating ||
               Screen->Space[Screen->ActiveSpace].RootNode == NULL)
            {
                Screen->ActiveSpace = CGSGetActiveSpace(CGSDefaultConnection);
                if(KWMScreen.PrevSpace != Screen->ActiveSpace)
                    DEBUG("UpdateActiveWindowList() Space transition ended " << KWMScreen.PrevSpace << " -> " << Screen->ActiveSpace)

                if(Screen->Space[Screen->ActiveSpace].Mode == SpaceModeFloating)
                {
                    DEBUG("Space is floating")
                    ActivateScreen(Screen, Mouse);
                    ClearBorder(FocusedBorder);
                    return;
                }

                DEBUG("Empty Screen")
                KWMScreen.PrevSpace = Screen->ActiveSpace;
                KWMScreen.OldScreenID = Screen->ID;
                KWMScreen.Current = Screen;
                KWMScreen.UpdateSpace = true;
                ActivateScreen(Screen, Mouse);

                if(Mouse && KWMFocus.Window)
                {
                    if(IsWindowFloating(KWMFocus.Window->WID, NULL))
                        ToggleFocusedWindowFloating();
                }
            }
        }
    }
}

void CaptureApplicationToScreen(int ScreenID, std::string Application)
{
    std::map<std::string, int>::iterator It = KWMTiling.CapturedAppLst.find(Application);
    if(It == KWMTiling.CapturedAppLst.end())
    {
        screen_info *Screen = GetDisplayFromScreenID(ScreenID);
        if(Screen)
        {
            KWMTiling.CapturedAppLst[Application] = ScreenID;
            DEBUG("CaptureApplicationToScreen() " << ScreenID << " " << Application)
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
    if(NewScreen)
        AddWindowToTreeOfUnfocusedMonitor(NewScreen, Window);
}

container_offset CreateDefaultScreenOffset()
{
    container_offset Offset = { 40, 20, 20, 20, 10, 10 };
    return Offset;
}
