#include "kwm.h"

extern uint32_t MaxDisplayCount;
extern uint32_t ActiveDisplaysCount;
extern CGDirectDisplayID ActiveDisplays[];

extern screen_info *Screen;
extern int CurrentSpace;

extern std::vector<screen_info> DisplayLst;
extern std::vector<window_info> WindowLst;
extern window_info *FocusedWindow;

void GetActiveDisplays()
{
    CGGetActiveDisplayList(MaxDisplayCount, (CGDirectDisplayID*)&ActiveDisplays, &ActiveDisplaysCount);
    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        CGRect DisplayRect = CGDisplayBounds(ActiveDisplays[DisplayIndex]);
        screen_info Screen;
        Screen.ID = DisplayIndex;
        Screen.ForceContainerUpdate = false;
        Screen.ActiveSpace = -1;
        Screen.OldWindowListCount = -1;

        Screen.X = DisplayRect.origin.x;
        Screen.Y = DisplayRect.origin.y;
        Screen.Width = DisplayRect.size.width;
        Screen.Height = DisplayRect.size.height;

        // Read from config at some point
        Screen.PaddingTop = 40;
        Screen.PaddingLeft = 20;
        Screen.PaddingRight = 20;
        Screen.PaddingBottom = 20;

        Screen.VerticalGap = 10;
        Screen.HorizontalGap = 10;

        DisplayLst.push_back(Screen);
    }

    Screen = GetDisplayOfMousePointer();
}

screen_info *GetDisplayOfMousePointer()
{
    CGEventRef Event = CGEventCreate(NULL);
    CGPoint Cursor = CGEventGetLocation(Event);
    CFRelease(Event);

    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        screen_info *Screen = &DisplayLst[DisplayIndex];
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
        for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
        {
            screen_info *Screen = &DisplayLst[DisplayIndex];
            if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
                return Screen;
        }
    }

    return NULL;
}

std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex)
{
    screen_info *Screen = &DisplayLst[ScreenIndex];
    std::vector<window_info*> ScreenWindowLst;
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        window_info *Window = &WindowLst[WindowIndex];
        if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
            ScreenWindowLst.push_back(Window);
    }

    return ScreenWindowLst;
}

std::vector<int> GetAllWindowIDsOnDisplay(int ScreenIndex)
{
    screen_info *Screen = &DisplayLst[ScreenIndex];
    std::vector<int> ScreenWindowIDLst;
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        window_info *Window = &WindowLst[WindowIndex];
        if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
            ScreenWindowIDLst.push_back(Window->WID);
    }

    return ScreenWindowIDLst;
}

void ChangePaddingOfDisplay(const std::string &Side, int Offset)
{
    screen_info *Screen = GetDisplayOfMousePointer();
    if(Side == "Left")
        Screen->Space[CurrentSpace].PaddingLeft += Offset;
    else if(Side == "Right")
        Screen->Space[CurrentSpace].PaddingRight += Offset;
    else if(Side == "Top")
        Screen->Space[CurrentSpace].PaddingTop += Offset;
    else if(Side == "Bottom")
        Screen->Space[CurrentSpace].PaddingBottom += Offset;

    SetRootNodeContainer(Screen, Screen->Space[CurrentSpace].RootNode);
    CreateNodeContainers(Screen, Screen->Space[CurrentSpace].RootNode);
    ApplyNodeContainer(Screen->Space[CurrentSpace].RootNode);
}

void ChangeGapOfDisplay(const std::string &Side, int Offset)
{
    screen_info *Screen = GetDisplayOfMousePointer();
    if(Side == "Vertical")
        Screen->Space[CurrentSpace].VerticalGap += Offset;
    else if(Side == "Horizontal")
        Screen->Space[CurrentSpace].HorizontalGap += Offset;

    CreateNodeContainers(Screen, Screen->Space[CurrentSpace].RootNode);
    ApplyNodeContainer(Screen->Space[CurrentSpace].RootNode);
}

void CycleFocusedWindowDisplay(int Shift)
{
    screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
    int NewScreenIndex = -1;

    if(Shift == 1)
        NewScreenIndex = (Screen->ID + 1 >= ActiveDisplaysCount) ? 0 : Screen->ID + 1;
    else if(Shift == -1)
        NewScreenIndex = (Screen->ID - 1 < 0) ? ActiveDisplaysCount - 1 : Screen->ID - 1;

    if(NewScreenIndex != Screen->ID)
    {
        screen_info *NewScreen = &DisplayLst[NewScreenIndex];
        AddWindowToTreeOfUnfocusedMonitor(NewScreen);
    }
}
