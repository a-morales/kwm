#include "kwm.h"

extern uint32_t max_display_count;
extern uint32_t active_displays_count;
extern CGDirectDisplayID active_displays[];

extern std::vector<screen_info> display_lst;
extern std::vector<window_info> window_lst;
extern AXUIElementRef focused_window_ref;
extern window_info focused_window;

void get_active_displays()
{
    CGGetActiveDisplayList(max_display_count, (CGDirectDisplayID*)&active_displays, &active_displays_count);
    for(int display_index = 0; display_index < active_displays_count; ++display_index)
    {
        CGRect display_rect = CGDisplayBounds(active_displays[display_index]);
        screen_info screen;
        screen.id = display_index;
        screen.x = display_rect.origin.x;
        screen.y = display_rect.origin.y;
        screen.width = display_rect.size.width;
        screen.height = display_rect.size.height;
        display_lst.push_back(screen);
    }
}

screen_info *get_display_of_window(window_info *window)
{
    for(int display_index = 0; display_index < active_displays_count; ++display_index)
    {
        screen_info *screen = &display_lst[display_index];
        if(window->x >= screen->x && window->x <= screen->x + screen->width)
            return screen;
    }

    return NULL;
}

std::vector<window_info*> get_all_windows_on_display(int screen_index)
{
    screen_info *screen = &display_lst[screen_index];
    std::vector<window_info*> screen_window_lst;
    for(int window_index = 0; window_index < window_lst.size(); ++window_index)
    {
        window_info *window = &window_lst[window_index];
        if(window->x >= screen->x && window->x <= screen->x + screen->width)
            screen_window_lst.push_back(window);
    }

    return screen_window_lst;
}

void cycle_focused_window_display(int shift)
{
    screen_info *cur_screen = get_display_of_window(&focused_window);
    int new_screen_index;

    if(shift == 1)
        new_screen_index = (cur_screen->id + 1 >= active_displays_count) ? 0 : cur_screen->id + 1;
    else if(shift == -1)
        new_screen_index = (cur_screen->id - 1 < 0) ? active_displays_count - 1 : cur_screen->id - 1;
    else
        return;

    screen_info *screen = &display_lst[new_screen_index];
    set_window_dimensions(focused_window_ref,
            &focused_window,
            screen->x + 30, 
            screen->y + 40,
            focused_window.width,
            focused_window.height);
}
