#include "kwm.h"

extern uint32_t MaxDisplayCount;
extern uint32_t ActiveDisplaysCount;
extern CGDirectDisplayID ActiveDisplays[];

extern std::vector<screen_info> DisplayLst;
extern std::vector<window_info> WindowLst;
extern AXUIElementRef FocusedWindowRef;
extern window_info FocusedWindow;

void GetActiveDisplays()
{
    CGGetActiveDisplayList(MaxDisplayCount, (CGDirectDisplayID*)&ActiveDisplays, &ActiveDisplaysCount);
    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        CGRect DisplayRect = CGDisplayBounds(ActiveDisplays[DisplayIndex]);
        screen_info Screen;
        Screen.id = DisplayIndex;
        Screen.x = DisplayRect.origin.x;
        Screen.y = DisplayRect.origin.y;
        Screen.width = DisplayRect.size.width;
        Screen.height = DisplayRect.size.height;
        DisplayLst.push_back(Screen);
    }
}

screen_info *GetDisplayOfWindow(window_info *Window)
{
    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        screen_info *Screen = &DisplayLst[DisplayIndex];
        if(Window->x >= Screen->x && Window->x <= Screen->x + Screen->width)
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
        if(Window->x >= Screen->x && Window->x <= Screen->x + Screen->width)
            ScreenWindowLst.push_back(Window);
    }

    return ScreenWindowLst;
}

void CycleFocusedWindowDisplay(int Shift)
{
    screen_info *Screen = GetDisplayOfWindow(&FocusedWindow);
    int NewScreenIndex;

    if(Shift == 1)
        NewScreenIndex = (Screen->id + 1 >= ActiveDisplaysCount) ? 0 : Screen->id + 1;
    else if(Shift == -1)
        NewScreenIndex = (Screen->id - 1 < 0) ? ActiveDisplaysCount - 1 : Screen->id - 1;
    else
        return;

    screen_info *NewScreen = &DisplayLst[NewScreenIndex];
    SetWindowDimensions(FocusedWindowRef,
            &FocusedWindow,
            NewScreen->x + 30, 
            NewScreen->y + 40,
            FocusedWindow.width,
            FocusedWindow.height);
}
