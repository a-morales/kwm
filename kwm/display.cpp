#include "display.h"
#include "space.h"
#include "tree.h"
#include "window.h"

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
                DestroyNodeTree(It->second.RootNode, It->second.Mode);

            if(KWMTiling.DisplayMap[Display].Identifier)
                CFRelease(KWMTiling.DisplayMap[Display].Identifier);

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

    Screen.Identifier = NULL;
    Screen.ID = ScreenIndex;
    Screen.ForceContainerUpdate = false;
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
    space_info *Space = GetActiveSpaceOfScreen(Screen);

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
    space_info *Space = GetActiveSpaceOfScreen(Screen);

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

void GiveFocusToScreen(int ScreenIndex, tree_node *Focus, bool Mouse)
{
    screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
    if(Screen && Screen != KWMScreen.Current)
    {
        KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
        KWMScreen.Current = Screen;

        Screen->ActiveSpace = GetActiveSpaceOfDisplay(Screen);
        ShouldActiveSpaceBeManaged();

        space_info *Space = GetActiveSpaceOfScreen(Screen);
        tree_node *FocusFirstNode = NULL;

        DEBUG("GiveFocusToScreen() " << ScreenIndex << \
              ": Space transition ended " << KWMScreen.PrevSpace << \
              " -> " << Screen->ActiveSpace)

        if(Space->Initialized)
            FocusFirstNode = GetFirstLeafNode(Space->RootNode);

        if(Space->Initialized && Focus)
        {
            DEBUG("Populated Screen 'Window -f Focus'")

            UpdateActiveWindowList(Screen);
            FilterWindowList(Screen);
            SetWindowFocusByNode(Focus);
            MoveCursorToCenterOfFocusedWindow();
        }
        else if(Space->Initialized && FocusFirstNode)
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
                SetWindowFocusByNode(FocusFirstNode);
                MoveCursorToCenterOfFocusedWindow();
            }
        }
        else
        {
            if(!Space->Initialized ||
               Space->Mode == SpaceModeFloating ||
               Space->RootNode == NULL)
            {
                DEBUG("Uninitialized Screen")
                ClearFocusedWindow();

                if(Space->Mode != SpaceModeFloating && Mouse && KWMFocus.Window)
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
