#include "kwm.h"

extern uint32_t MaxDisplayCount;
extern uint32_t ActiveDisplaysCount;
extern CGDirectDisplayID ActiveDisplays[];

extern screen_info *Screen;
extern std::map<unsigned int, screen_info> DisplayMap;
extern std::vector<window_info> WindowLst;
extern window_info *FocusedWindow;
extern container_offset DefaultContainerOffset;


extern pthread_mutex_t BackgroundLock;

void DisplayReconfigurationCallBack(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void *UserInfo)
{
    pthread_mutex_lock(&BackgroundLock);

    if (Flags & kCGDisplayAddFlag)
    {
        // Display has been added
        DEBUG("New display detected! DisplayID: " << Display << " Index: " << ActiveDisplaysCount)
        RefreshActiveDisplays();
    }
    else if (Flags & kCGDisplayRemoveFlag)
    {
        // Display has been removed
        DEBUG("Display has been removed! DisplayID: " << Display)
        std::map<int, space_info>::iterator It;
        for(It = DisplayMap[Display].Space.begin(); It != DisplayMap[Display].Space.end(); ++It)
            DestroyNodeTree(It->second.RootNode, It->second.Mode);

        DisplayMap.erase(Display);
        RefreshActiveDisplays();
    }

    pthread_mutex_unlock(&BackgroundLock);
}

screen_info CreateDefaultScreenInfo(int DisplayIndex, int ScreenIndex)
{
    CGRect DisplayRect = CGDisplayBounds(DisplayIndex);
    screen_info Screen;

    Screen.ID = ScreenIndex;
    Screen.ForceContainerUpdate = false;
    Screen.ActiveSpace = -1;
    Screen.OldWindowListCount = -1;

    Screen.X = DisplayRect.origin.x;
    Screen.Y = DisplayRect.origin.y;
    Screen.Width = DisplayRect.size.width;
    Screen.Height = DisplayRect.size.height;

    Screen.Offset = DefaultContainerOffset;
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

    Screen->Offset = DefaultContainerOffset;
    Screen->ForceContainerUpdate = true;
}

void GetActiveDisplays()
{
    CGGetActiveDisplayList(MaxDisplayCount, (CGDirectDisplayID*)&ActiveDisplays, &ActiveDisplaysCount);
    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        unsigned int DisplayID = ActiveDisplays[DisplayIndex];
        DisplayMap[DisplayID] = CreateDefaultScreenInfo(DisplayID, DisplayIndex);;

        DEBUG("DisplayID " << DisplayID << " has index " << DisplayIndex)
    }

    Screen = GetDisplayOfMousePointer();
    CGDisplayRegisterReconfigurationCallback(DisplayReconfigurationCallBack, NULL);
}

void RefreshActiveDisplays()
{
    CGGetActiveDisplayList(MaxDisplayCount, (CGDirectDisplayID*)&ActiveDisplays, &ActiveDisplaysCount);
    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        unsigned int DisplayID = ActiveDisplays[DisplayIndex];
        std::map<unsigned int, screen_info>::iterator It;

        if(It != DisplayMap.end())
            UpdateExistingScreenInfo(&DisplayMap[DisplayID], DisplayID, DisplayIndex);
        else
            DisplayMap[DisplayID] = CreateDefaultScreenInfo(DisplayID, DisplayIndex);

        DEBUG("DisplayID " << DisplayID << " has index " << DisplayIndex)
    }

    Screen = GetDisplayOfMousePointer();
}

screen_info *GetDisplayFromScreenID(int ID)
{
    std::map<unsigned int, screen_info>::iterator It;
    for(It = DisplayMap.begin(); It != DisplayMap.end(); ++It)
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
    for(It = DisplayMap.begin(); It != DisplayMap.end(); ++It)
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
        for(It = DisplayMap.begin(); It != DisplayMap.end(); ++It)
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
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        window_info *Window = &WindowLst[WindowIndex];
        if(!IsApplicationFloating(&WindowLst[WindowIndex]))
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
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        window_info *Window = &WindowLst[WindowIndex];
        if(!IsApplicationFloating(&WindowLst[WindowIndex]))
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
        DefaultContainerOffset.PaddingLeft = Offset;
    else if(Side == "right")
        DefaultContainerOffset.PaddingRight = Offset;
    else if(Side == "top")
        DefaultContainerOffset.PaddingTop = Offset;
    else if(Side == "bottom")
        DefaultContainerOffset.PaddingBottom = Offset;
}

void SetDefaultGapOfDisplay(const std::string &Side, int Offset)
{
    if(Side == "vertical")
        DefaultContainerOffset.VerticalGap = Offset;
    else if(Side == "horizontal")
        DefaultContainerOffset.HorizontalGap = Offset;
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

void CycleFocusedWindowDisplay(int Shift, bool Relative)
{
    screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
    int NewScreenIndex = -1;

    if(Relative)
    {
        if(Shift == 1)
            NewScreenIndex = (Screen->ID + 1 >= ActiveDisplaysCount) ? 0 : Screen->ID + 1;
        else if(Shift == -1)
            NewScreenIndex = (Screen->ID - 1 < 0) ? ActiveDisplaysCount - 1 : Screen->ID - 1;
    }
    else
    {
        NewScreenIndex = Shift;
    }

    if(NewScreenIndex != Screen->ID)
    {
        screen_info *NewScreen = GetDisplayFromScreenID(NewScreenIndex);
        AddWindowToTreeOfUnfocusedMonitor(NewScreen);
    }
}
