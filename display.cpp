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
    for(int display_index = 0; display_index < ActiveDisplaysCount; ++display_index)
    {
        CGRect display_rect = CGDisplayBounds(ActiveDisplays[display_index]);
        screen_info screen;
        screen.id = display_index;
        screen.x = display_rect.origin.x;
        screen.y = display_rect.origin.y;
        screen.width = display_rect.size.width;
        screen.height = display_rect.size.height;
        DisplayLst.push_back(screen);
    }
}

screen_info *GetDisplayOfWindow(window_info *window)
{
    for(int display_index = 0; display_index < ActiveDisplaysCount; ++display_index)
    {
        screen_info *screen = &DisplayLst[display_index];
        if(window->x >= screen->x && window->x <= screen->x + screen->width)
            return screen;
    }

    return NULL;
}

std::vector<window_info*> GetAllWindowsOnDisplay(int screen_index)
{
    screen_info *screen = &DisplayLst[screen_index];
    std::vector<window_info*> screen_WindowLst;
    for(int window_index = 0; window_index < WindowLst.size(); ++window_index)
    {
        window_info *window = &WindowLst[window_index];
        if(window->x >= screen->x && window->x <= screen->x + screen->width)
            screen_WindowLst.push_back(window);
    }

    return screen_WindowLst;
}

void CycleFocusedWindowDisplay(int shift)
{
    screen_info *cur_screen = GetDisplayOfWindow(&FocusedWindow);
    int new_screen_index;

    if(shift == 1)
        new_screen_index = (cur_screen->id + 1 >= ActiveDisplaysCount) ? 0 : cur_screen->id + 1;
    else if(shift == -1)
        new_screen_index = (cur_screen->id - 1 < 0) ? ActiveDisplaysCount - 1 : cur_screen->id - 1;
    else
        return;

    screen_info *screen = &DisplayLst[new_screen_index];
    SetWindowDimensions(FocusedWindowRef,
            &FocusedWindow,
            screen->x + 30, 
            screen->y + 40,
            FocusedWindow.width,
            FocusedWindow.height);
}
