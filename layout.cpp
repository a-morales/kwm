#include "kwm.h"

extern std::vector<spaces_info> spaces_lst;
extern std::vector<screen_info> display_lst;
extern std::vector<window_info> window_lst;
extern std::vector<screen_layout> screen_layout_lst;
extern std::vector<window_layout> layout_lst;

extern uint32_t max_display_count;
extern ProcessSerialNumber focused_psn;
extern AXUIElementRef focused_window_ref;
extern window_info focused_window;

void apply_layout_for_display(int screen_index)
{
    refresh_active_spaces_info();
    detect_window_below_cursor();

    std::vector<window_info*> screen_window_lst = get_all_windows_on_display(screen_index);
    screen_layout *layout_master = &screen_layout_lst[screen_index];

    int space_index = get_space_of_window(&focused_window);
    if(space_index == -1)
        return;

    int active_layout_index = spaces_lst[space_index].next_layout_index;
    spaces_lst[space_index].active_layout_index = active_layout_index;

    if(active_layout_index + 1 < layout_master->number_of_layouts)
        spaces_lst[space_index].next_layout_index = active_layout_index + 1;
    else
        spaces_lst[space_index].next_layout_index = 0;

    for(int window_index = 0; window_index < screen_window_lst.size(); ++window_index)
    {
        window_info *window = screen_window_lst[window_index];
        AXUIElementRef window_ref;
        if(get_window_ref(window, &window_ref))
        {
            if(window_index < layout_master->layouts[active_layout_index].layouts.size())
            {
                window_layout *layout = &layout_master->layouts[active_layout_index].layouts[window_index];
                window->layout = layout;
                window->layout_index = window_index;
                set_window_dimensions(window_ref, window, layout->x, layout->y, layout->width, layout->height); 
            }

            if(!windows_are_equal(&focused_window, window))
                CFRelease(window_ref);
        }
    }

    detect_window_below_cursor();
}

void cycle_focused_window_layout(int screen_index, int shift)
{
    std::vector<window_info*> screen_window_lst = get_all_windows_on_display(screen_index);
    screen_layout *layout_master = &screen_layout_lst[screen_index];

    get_layout_of_window(&focused_window);
    int space_index = get_space_of_window(&focused_window);
    if(space_index == -1)
        return;

    int active_layout_index = spaces_lst[space_index].active_layout_index;

    window_layout *focused_window_layout = focused_window.layout;
    int focused_window_layout_index = focused_window.layout_index;
    if(!focused_window_layout)
        return;

    int swap_with_window_layout_index = focused_window_layout_index + shift;

    if(swap_with_window_layout_index < 0 || 
        swap_with_window_layout_index >= layout_master->number_of_layouts)
            return;

    int swap_with_window_index = -1;
    window_layout *swap_with_window_layout = &layout_master->layouts[active_layout_index].layouts[swap_with_window_layout_index];
    for(int window_index = 0; window_index < screen_window_lst.size(); ++window_index)
    {
        window_info *window = screen_window_lst[window_index];
        if(window_has_layout(window, swap_with_window_layout))
        {
            swap_with_window_index = window_index;
            break;
        }
    }

    if(swap_with_window_index == -1)
        return;

    AXUIElementRef window_ref;
    window_info *window = screen_window_lst[swap_with_window_index];
    if(get_window_ref(window, &window_ref))
    {
        set_window_dimensions(window_ref, 
                window, 
                focused_window_layout->x, 
                focused_window_layout->y, 
                focused_window_layout->width, 
                focused_window_layout->height); 

        set_window_dimensions(focused_window_ref, 
                &focused_window, 
                swap_with_window_layout->x, 
                swap_with_window_layout->y, 
                swap_with_window_layout->width, 
                swap_with_window_layout->height); 

        CFRelease(window_ref);
    }
}

void cycle_window_inside_layout(int screen_index)
{
    detect_window_below_cursor();

    std::vector<window_info*> screen_window_lst = get_all_windows_on_display(screen_index);
    screen_layout *layout_master = &screen_layout_lst[screen_index];

    int space_index = get_space_of_window(&focused_window);
    if(space_index == -1)
        return;

    int active_layout_index = spaces_lst[space_index].active_layout_index;

    window_layout *focused_window_layout = focused_window.layout;
    if(!focused_window_layout)
        return;

    int max_layout_tiles = layout_master->layouts[active_layout_index].layouts.size();
    if(screen_window_lst.size() <= max_layout_tiles)
        return;

    AXUIElementRef window_ref;
    window_info *window = screen_window_lst[screen_window_lst.size()-1];
    if(get_window_ref(window, &window_ref))
    {
        set_window_dimensions(window_ref,
                window,
                focused_window_layout->x,
                focused_window_layout->y,
                focused_window_layout->width,
                focused_window_layout->height);

        ProcessSerialNumber newpsn;
        GetProcessForPID(window->pid, &newpsn);

        AXUIElementSetAttributeValue(window_ref, kAXMainAttribute, kCFBooleanTrue);
        AXUIElementSetAttributeValue(window_ref, kAXFocusedAttribute, kCFBooleanTrue);
        AXUIElementPerformAction (window_ref, kAXRaiseAction);

        SetFrontProcessWithOptions(&newpsn, kSetFrontProcessFrontWindowOnly);

        if(focused_window_ref != NULL)
            CFRelease(focused_window_ref);
        
        focused_window_ref = window_ref;
        focused_window = *window;
        focused_psn = newpsn;
    }
}

window_layout *get_layout_of_window(window_info *window)
{
    window_layout *result = NULL;

    int screen_id = get_display_of_window(window)->id;

    int space_index = get_space_of_window(&focused_window);
    if(space_index == -1)
        return result;

    int active_layout_index = spaces_lst[space_index].active_layout_index;
    int max_layout_size = screen_layout_lst[screen_id].layouts[active_layout_index].layouts.size();

    for(int layout_index = 0; layout_index < max_layout_size; ++layout_index)
    {
        window_layout *layout = &screen_layout_lst[screen_id].layouts[active_layout_index].layouts[layout_index];
        if(window_has_layout(window, layout))
        {
            window->layout = layout;
            window->layout_index = layout_index;
            result = layout;
            break;
        }
    }

    return result;
}

bool window_has_layout(window_info *window, window_layout *layout)
{
    if((window->x >= layout->x - 15 && window->x <= layout->x + 15) &&
        (window->y >= layout->y - 15 && window->y <= layout->y + 15 ) &&
        (window->width >= layout->width - 15 && window->width <= layout->width + 15) &&
        (window->height >= layout->height - 15 && window->height <= layout->height + 15))
            return true;

    return false;
}

void set_window_layout_values(window_layout *layout, int x, int y, int width, int height)
{
    layout->x = x;
    layout->y = y;
    layout->width = width;
    layout->height = height;
}

window_layout get_window_layout_for_screen(int screen_index, const std::string &name)
{
    window_layout layout;
    layout.name = "invalid";

    screen_info *screen = &display_lst[screen_index];
    if(screen)
    {
        for(int layout_index = 0; layout_index < layout_lst.size(); ++layout_index)
        {
            if(layout_lst[layout_index].name == name)
            {
                layout = layout_lst[layout_index];
                break;
            }
        }

        if(name == "fullscreen")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    (screen->width - (layout.gap_vertical * 2)), 
                    (screen->height - (layout.gap_y * 1.5f)));
        else if(name == "left vertical split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    (screen->height - (layout.gap_y * 1.5f)));
        else if(name == "right vertical split")
            set_window_layout_values(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    (screen->height - (layout.gap_y * 1.5f)));
        else if(name == "upper horizontal split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    (screen->width - (layout.gap_vertical * 2)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "lower horizontal split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + ((screen->height / 2) + (layout.gap_horizontal * 0.5f) + (layout.gap_y / 8)), 
                    (screen->width - (layout.gap_vertical * 2)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "upper left split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "lower left split")
            set_window_layout_values(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + ((screen->height / 2) + (layout.gap_horizontal * 0.5f) + (layout.gap_y / 8)), 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "upper right split")
            set_window_layout_values(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "lower right split")
            set_window_layout_values(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + ((screen->height / 2) + (layout.gap_horizontal * 0.5f) + (layout.gap_y / 8)), 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
    }

    return layout;
}

void init_window_layouts()
{
    window_layout screen_vertical_split;
    screen_vertical_split.gap_x = 30;
    screen_vertical_split.gap_y = 40;
    screen_vertical_split.gap_vertical = 30;
    screen_vertical_split.gap_horizontal = 30;

    screen_vertical_split.name = "fullscreen";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "left vertical split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "right vertical split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "upper horizontal split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "lower horizontal split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "upper left split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "lower left split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "upper right split";
    layout_lst.push_back(screen_vertical_split);

    screen_vertical_split.name = "lower right split";
    layout_lst.push_back(screen_vertical_split);

    // Screen Layouts
    for(int screen_index = 0; screen_index < max_display_count; ++screen_index)
    {
        screen_layout layout_master;

        window_group_layout layout_tall_left;
        layout_tall_left.layouts.push_back(get_window_layout_for_screen(screen_index, "left vertical split"));
        layout_tall_left.layouts.push_back(get_window_layout_for_screen(screen_index, "upper right split"));
        layout_tall_left.layouts.push_back(get_window_layout_for_screen(screen_index, "lower right split"));
        layout_master.layouts.push_back(layout_tall_left);

        window_group_layout layout_tall_right;
        layout_tall_right.layouts.push_back(get_window_layout_for_screen(screen_index, "upper left split"));
        layout_tall_right.layouts.push_back(get_window_layout_for_screen(screen_index, "lower left split"));
        layout_tall_right.layouts.push_back(get_window_layout_for_screen(screen_index, "right vertical split"));
        layout_master.layouts.push_back(layout_tall_right);

        window_group_layout layout_vertical_split;
        layout_vertical_split.layouts.push_back(get_window_layout_for_screen(screen_index, "left vertical split"));
        layout_vertical_split.layouts.push_back(get_window_layout_for_screen(screen_index, "right vertical split"));
        layout_master.layouts.push_back(layout_vertical_split);

        window_group_layout layout_quarters;
        layout_quarters.layouts.push_back(get_window_layout_for_screen(screen_index, "upper left split"));
        layout_quarters.layouts.push_back(get_window_layout_for_screen(screen_index, "lower left split"));
        layout_quarters.layouts.push_back(get_window_layout_for_screen(screen_index, "upper right split"));
        layout_quarters.layouts.push_back(get_window_layout_for_screen(screen_index, "lower right split"));
        layout_master.layouts.push_back(layout_quarters);

        window_group_layout layout_fullscreen;
        layout_fullscreen.layouts.push_back(get_window_layout_for_screen(screen_index, "fullscreen"));
        layout_master.layouts.push_back(layout_fullscreen);

        layout_master.number_of_layouts = layout_master.layouts.size();;
        screen_layout_lst.push_back(layout_master);
    }
}
