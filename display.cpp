#include "kwm.h"

extern uint32_t MaxDisplayCount;
extern uint32_t ActiveDisplaysCount;
extern CGDirectDisplayID ActiveDisplays[];

extern std::vector<screen_info> DisplayLst;
extern std::vector<window_info> WindowLst;
extern AXUIElementRef FocusedWindowRef;
extern window_info *FocusedWindow;

void GetActiveDisplays()
{
    CGGetActiveDisplayList(MaxDisplayCount, (CGDirectDisplayID*)&ActiveDisplays, &ActiveDisplaysCount);
    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        CGRect DisplayRect = CGDisplayBounds(ActiveDisplays[DisplayIndex]);
        screen_info Screen;
        Screen.ID = DisplayIndex;
        Screen.X = DisplayRect.origin.x;
        Screen.Y = DisplayRect.origin.y;
        Screen.Width = DisplayRect.size.width;
        Screen.Height = DisplayRect.size.height;

        // Read from config at some point
        Screen.PaddingTop = 30;
        Screen.PaddingLeft = 20;
        Screen.PaddingRight = 20;
        Screen.PaddingBottom = 20;

        DisplayLst.push_back(Screen);
    }
}

screen_info *GetDisplayOfWindow(window_info *Window)
{
    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        screen_info *Screen = &DisplayLst[DisplayIndex];
        if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
            return Screen;
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

void CycleFocusedWindowDisplay(int Shift)
{
    screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
    int NewScreenIndex;

    if(Shift == 1)
        NewScreenIndex = (Screen->ID + 1 >= ActiveDisplaysCount) ? 0 : Screen->ID + 1;
    else if(Shift == -1)
        NewScreenIndex = (Screen->ID - 1 < 0) ? ActiveDisplaysCount - 1 : Screen->ID - 1;
    else
        return;

    screen_info *NewScreen = &DisplayLst[NewScreenIndex];
    SetWindowDimensions(FocusedWindowRef,
            FocusedWindow,
            NewScreen->X + 30, 
            NewScreen->Y + 40,
            FocusedWindow->Width,
            FocusedWindow->Height);
}
