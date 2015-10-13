#include "kwm.h"

extern std::vector<spaces_info> SpacesLst;
extern std::vector<screen_info> DisplayLst;
extern std::vector<window_info> WindowLst;
extern std::vector<screen_layout> ScreenLayoutLst;
extern std::vector<window_layout> LayoutLst;

extern uint32_t MaxDisplayCount;
extern ProcessSerialNumber FocusedPSN;
extern AXUIElementRef FocusedWindowRef;
extern window_info FocusedWindow;

void ApplyLayoutForDisplay(int screen_index)
{
    RefreshActiveSpacesInfo();
    DetectWindowBelowCursor();

    std::vector<window_info*> screen_WindowLst = GetAllWindowsOnDisplay(screen_index);
    screen_layout *layout_master = &ScreenLayoutLst[screen_index];

    int space_index = GetSpaceOfWindow(&FocusedWindow);
    if(space_index == -1)
        return;

    int active_layout_index = SpacesLst[space_index].next_layout_index;
    SpacesLst[space_index].active_layout_index = active_layout_index;

    if(active_layout_index + 1 < layout_master->number_of_layouts)
        SpacesLst[space_index].next_layout_index = active_layout_index + 1;
    else
        SpacesLst[space_index].next_layout_index = 0;

    for(int window_index = 0; window_index < screen_WindowLst.size(); ++window_index)
    {
        window_info *window = screen_WindowLst[window_index];
        AXUIElementRef window_ref;
        if(GetWindowRef(window, &window_ref))
        {
            if(window_index < layout_master->layouts[active_layout_index].layouts.size())
            {
                window_layout *layout = &layout_master->layouts[active_layout_index].layouts[window_index];
                window->layout = layout;
                window->layout_index = window_index;
                SetWindowDimensions(window_ref, window, layout->x, layout->y, layout->width, layout->height); 
            }

            if(!WindowsAreEqual(&FocusedWindow, window))
                CFRelease(window_ref);
        }
    }

    DetectWindowBelowCursor();
}

void CycleFocusedWindowLayout(int screen_index, int shift)
{
    std::vector<window_info*> screen_WindowLst = GetAllWindowsOnDisplay(screen_index);
    screen_layout *layout_master = &ScreenLayoutLst[screen_index];

    GetLayoutOfWindow(&FocusedWindow);
    int space_index = GetSpaceOfWindow(&FocusedWindow);
    if(space_index == -1)
        return;

    int active_layout_index = SpacesLst[space_index].active_layout_index;

    window_layout *FocusedWindow_layout = FocusedWindow.layout;
    int FocusedWindow_layout_index = FocusedWindow.layout_index;
    if(!FocusedWindow_layout)
        return;

    int swap_with_window_layout_index = FocusedWindow_layout_index + shift;

    if(swap_with_window_layout_index < 0 || 
        swap_with_window_layout_index >= layout_master->number_of_layouts)
            return;

    int swap_with_window_index = -1;
    window_layout *swap_with_window_layout = &layout_master->layouts[active_layout_index].layouts[swap_with_window_layout_index];
    for(int window_index = 0; window_index < screen_WindowLst.size(); ++window_index)
    {
        window_info *window = screen_WindowLst[window_index];
        if(WindowHasLayout(window, swap_with_window_layout))
        {
            swap_with_window_index = window_index;
            break;
        }
    }

    if(swap_with_window_index == -1)
        return;

    AXUIElementRef window_ref;
    window_info *window = screen_WindowLst[swap_with_window_index];
    if(GetWindowRef(window, &window_ref))
    {
        SetWindowDimensions(window_ref, 
                window, 
                FocusedWindow_layout->x, 
                FocusedWindow_layout->y, 
                FocusedWindow_layout->width, 
                FocusedWindow_layout->height); 

        SetWindowDimensions(FocusedWindowRef, 
                &FocusedWindow, 
                swap_with_window_layout->x, 
                swap_with_window_layout->y, 
                swap_with_window_layout->width, 
                swap_with_window_layout->height); 

        CFRelease(window_ref);
    }
}

void CycleWindowInsideLayout(int screen_index)
{
    DetectWindowBelowCursor();
    GetLayoutOfWindow(&FocusedWindow);

    std::vector<window_info*> screen_WindowLst = GetAllWindowsOnDisplay(screen_index);
    screen_layout *layout_master = &ScreenLayoutLst[screen_index];

    int space_index = GetSpaceOfWindow(&FocusedWindow);
    if(space_index == -1)
        return;

    int active_layout_index = SpacesLst[space_index].active_layout_index;

    window_layout *FocusedWindow_layout = FocusedWindow.layout;
    if(!FocusedWindow_layout)
        return;

    int max_layout_tiles = layout_master->layouts[active_layout_index].layouts.size();
    if(screen_WindowLst.size() <= max_layout_tiles)
        return;

    AXUIElementRef window_ref;
    window_info *window = screen_WindowLst[screen_WindowLst.size()-1];
    
    if(GetWindowRef(window, &window_ref))
    {
        SetWindowDimensions(window_ref,
                window,
                FocusedWindow_layout->x,
                FocusedWindow_layout->y,
                FocusedWindow_layout->width,
                FocusedWindow_layout->height);

        ProcessSerialNumber newpsn;
        GetProcessForPID(window->pid, &newpsn);

        AXUIElementSetAttributeValue(window_ref, kAXMainAttribute, kCFBooleanTrue);
        AXUIElementSetAttributeValue(window_ref, kAXFocusedAttribute, kCFBooleanTrue);
        AXUIElementPerformAction (window_ref, kAXRaiseAction);

        SetFrontProcessWithOptions(&newpsn, kSetFrontProcessFrontWindowOnly);

        if(FocusedWindowRef != NULL)
            CFRelease(FocusedWindowRef);
        
        FocusedWindowRef = window_ref;
        FocusedWindow = *window;
        FocusedPSN = newpsn;
    }
}

window_layout *GetLayoutOfWindow(window_info *window)
{
    window_layout *result = NULL;

    int screen_id = GetDisplayOfWindow(window)->id;

    int space_index = GetSpaceOfWindow(&FocusedWindow);
    if(space_index == -1)
        return result;

    int active_layout_index = SpacesLst[space_index].active_layout_index;
    int max_layout_size = ScreenLayoutLst[screen_id].layouts[active_layout_index].layouts.size();

    for(int layout_index = 0; layout_index < max_layout_size; ++layout_index)
    {
        window_layout *layout = &ScreenLayoutLst[screen_id].layouts[active_layout_index].layouts[layout_index];
        if(WindowHasLayout(window, layout))
        {
            window->layout = layout;
            window->layout_index = layout_index;
            result = layout;
            break;
        }
    }

    return result;
}

bool WindowHasLayout(window_info *window, window_layout *layout)
{
    if((window->x >= layout->x - 15 && window->x <= layout->x + 15) &&
        (window->y >= layout->y - 15 && window->y <= layout->y + 15 ) &&
        (window->width >= layout->width - 15 && window->width <= layout->width + 15) &&
        (window->height >= layout->height - 15 && window->height <= layout->height + 15))
            return true;

    return false;
}

void SetWindowLayoutValues(window_layout *layout, int x, int y, int width, int height)
{
    layout->x = x;
    layout->y = y;
    layout->width = width;
    layout->height = height;
}

window_layout GetWindowLayoutForScreen(int screen_index, const std::string &name)
{
    window_layout layout;
    layout.name = "invalid";

    screen_info *screen = &DisplayLst[screen_index];
    if(screen)
    {
        for(int layout_index = 0; layout_index < LayoutLst.size(); ++layout_index)
        {
            if(LayoutLst[layout_index].name == name)
            {
                layout = LayoutLst[layout_index];
                break;
            }
        }

        if(name == "fullscreen")
            SetWindowLayoutValues(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    (screen->width - (layout.gap_vertical * 2)), 
                    (screen->height - (layout.gap_y * 1.5f)));
        else if(name == "left vertical split")
            SetWindowLayoutValues(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    (screen->height - (layout.gap_y * 1.5f)));
        else if(name == "right vertical split")
            SetWindowLayoutValues(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    (screen->height - (layout.gap_y * 1.5f)));
        else if(name == "upper horizontal split")
            SetWindowLayoutValues(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    (screen->width - (layout.gap_vertical * 2)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "lower horizontal split")
            SetWindowLayoutValues(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + ((screen->height / 2) + (layout.gap_horizontal * 0.5f) + (layout.gap_y / 8)), 
                    (screen->width - (layout.gap_vertical * 2)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "upper left split")
            SetWindowLayoutValues(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "lower left split")
            SetWindowLayoutValues(&layout, 
                    screen->x + layout.gap_x, 
                    screen->y + ((screen->height / 2) + (layout.gap_horizontal * 0.5f) + (layout.gap_y / 8)), 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "upper right split")
            SetWindowLayoutValues(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + layout.gap_y, 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
        else if(name == "lower right split")
            SetWindowLayoutValues(&layout, 
                    screen->x + ((screen->width / 2) + (layout.gap_vertical * 0.5f)), 
                    screen->y + ((screen->height / 2) + (layout.gap_horizontal * 0.5f) + (layout.gap_y / 8)), 
                    ((screen->width / 2) - (layout.gap_vertical * 1.5f)), 
                    ((screen->height / 2) - (layout.gap_horizontal * 1.0f)));
    }

    return layout;
}

void InitWindowLayouts()
{
    window_layout screen_vertical_split;
    screen_vertical_split.gap_x = 30;
    screen_vertical_split.gap_y = 40;
    screen_vertical_split.gap_vertical = 30;
    screen_vertical_split.gap_horizontal = 30;

    screen_vertical_split.name = "fullscreen";
    LayoutLst.push_back(screen_vertical_split);

    screen_vertical_split.name = "left vertical split";
    LayoutLst.push_back(screen_vertical_split);

    screen_vertical_split.name = "right vertical split";
    LayoutLst.push_back(screen_vertical_split);

    screen_vertical_split.name = "upper horizontal split";
    LayoutLst.push_back(screen_vertical_split);

    screen_vertical_split.name = "lower horizontal split";
    LayoutLst.push_back(screen_vertical_split);

    screen_vertical_split.name = "upper left split";
    LayoutLst.push_back(screen_vertical_split);

    screen_vertical_split.name = "lower left split";
    LayoutLst.push_back(screen_vertical_split);

    screen_vertical_split.name = "upper right split";
    LayoutLst.push_back(screen_vertical_split);

    screen_vertical_split.name = "lower right split";
    LayoutLst.push_back(screen_vertical_split);

    // Screen Layouts
    for(int screen_index = 0; screen_index < MaxDisplayCount; ++screen_index)
    {
        screen_layout layout_master;

        window_group_layout layout_tall_left;
        layout_tall_left.layouts.push_back(GetWindowLayoutForScreen(screen_index, "left vertical split"));
        layout_tall_left.layouts.push_back(GetWindowLayoutForScreen(screen_index, "upper right split"));
        layout_tall_left.layouts.push_back(GetWindowLayoutForScreen(screen_index, "lower right split"));
        layout_master.layouts.push_back(layout_tall_left);

        window_group_layout layout_tall_right;
        layout_tall_right.layouts.push_back(GetWindowLayoutForScreen(screen_index, "upper left split"));
        layout_tall_right.layouts.push_back(GetWindowLayoutForScreen(screen_index, "lower left split"));
        layout_tall_right.layouts.push_back(GetWindowLayoutForScreen(screen_index, "right vertical split"));
        layout_master.layouts.push_back(layout_tall_right);

        window_group_layout layout_vertical_split;
        layout_vertical_split.layouts.push_back(GetWindowLayoutForScreen(screen_index, "left vertical split"));
        layout_vertical_split.layouts.push_back(GetWindowLayoutForScreen(screen_index, "right vertical split"));
        layout_master.layouts.push_back(layout_vertical_split);

        window_group_layout layout_quarters;
        layout_quarters.layouts.push_back(GetWindowLayoutForScreen(screen_index, "upper left split"));
        layout_quarters.layouts.push_back(GetWindowLayoutForScreen(screen_index, "lower left split"));
        layout_quarters.layouts.push_back(GetWindowLayoutForScreen(screen_index, "upper right split"));
        layout_quarters.layouts.push_back(GetWindowLayoutForScreen(screen_index, "lower right split"));
        layout_master.layouts.push_back(layout_quarters);

        window_group_layout layout_fullscreen;
        layout_fullscreen.layouts.push_back(GetWindowLayoutForScreen(screen_index, "fullscreen"));
        layout_master.layouts.push_back(layout_fullscreen);

        layout_master.number_of_layouts = layout_master.layouts.size();;
        ScreenLayoutLst.push_back(layout_master);
    }
}
