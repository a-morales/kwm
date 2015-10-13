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

void ApplyLayoutForDisplay(int ScreenIndex)
{
    RefreshActiveSpacesInfo();
    DetectWindowBelowCursor();

    std::vector<window_info*> ScreenWindowLst = GetAllWindowsOnDisplay(ScreenIndex);
    screen_layout *LayoutMaster = &ScreenLayoutLst[ScreenIndex];

    int SpaceIndex = GetSpaceOfWindow(&FocusedWindow);
    if(SpaceIndex == -1)
        return;

    int ActiveLayoutIndex = SpacesLst[SpaceIndex].next_layout_index;
    SpacesLst[SpaceIndex].active_layout_index = ActiveLayoutIndex;

    if(ActiveLayoutIndex + 1 < LayoutMaster->number_of_layouts)
        SpacesLst[SpaceIndex].next_layout_index = ActiveLayoutIndex + 1;
    else
        SpacesLst[SpaceIndex].next_layout_index = 0;

    for(int WindowIndex = 0; WindowIndex < ScreenWindowLst.size(); ++WindowIndex)
    {
        window_info *Window = ScreenWindowLst[WindowIndex];
        AXUIElementRef WindowRef;
        if(GetWindowRef(Window, &WindowRef))
        {
            if(WindowIndex < LayoutMaster->layouts[ActiveLayoutIndex].layouts.size())
            {
                window_layout *Layout = &LayoutMaster->layouts[ActiveLayoutIndex].layouts[WindowIndex];
                Window->layout = Layout;
                Window->layout_index = WindowIndex;
                SetWindowDimensions(WindowRef, Window, Layout->x, Layout->y, Layout->width, Layout->height); 
            }

            if(!WindowsAreEqual(&FocusedWindow, Window))
                CFRelease(WindowRef);
        }
    }

    DetectWindowBelowCursor();
}

void CycleFocusedWindowLayout(int ScreenIndex, int Shift)
{
    std::vector<window_info*> ScreenWindowLst = GetAllWindowsOnDisplay(ScreenIndex);
    screen_layout *LayoutMaster = &ScreenLayoutLst[ScreenIndex];

    GetLayoutOfWindow(&FocusedWindow);
    int SpaceIndex = GetSpaceOfWindow(&FocusedWindow);
    if(SpaceIndex == -1)
        return;

    int ActiveLayoutIndex = SpacesLst[SpaceIndex].active_layout_index;

    window_layout *FocusedWindowLayout = FocusedWindow.layout;
    int FocusedWindowLayoutIndex = FocusedWindow.layout_index;
    if(!FocusedWindowLayout)
        return;

    int SwapWithWindowLayoutIndex = FocusedWindowLayoutIndex + Shift;
    if(SwapWithWindowLayoutIndex < 0 || 
        SwapWithWindowLayoutIndex >= LayoutMaster->number_of_layouts)
            return;

    int SwapWithWindowIndex = -1;
    window_layout *SwapWithWindowLayout = &LayoutMaster->layouts[ActiveLayoutIndex].layouts[SwapWithWindowLayoutIndex];
    for(int WindowIndex = 0; WindowIndex < ScreenWindowLst.size(); ++WindowIndex)
    {
        window_info *Window = ScreenWindowLst[WindowIndex];
        if(WindowHasLayout(Window, SwapWithWindowLayout))
        {
            SwapWithWindowIndex = WindowIndex;
            break;
        }
    }

    if(SwapWithWindowIndex == -1)
        return;

    AXUIElementRef WindowRef;
    window_info *Window = ScreenWindowLst[SwapWithWindowIndex];
    if(GetWindowRef(Window, &WindowRef))
    {
        SetWindowDimensions(WindowRef, 
                Window, 
                FocusedWindowLayout->x, 
                FocusedWindowLayout->y, 
                FocusedWindowLayout->width, 
                FocusedWindowLayout->height); 

        SetWindowDimensions(FocusedWindowRef, 
                &FocusedWindow, 
                SwapWithWindowLayout->x, 
                SwapWithWindowLayout->y, 
                SwapWithWindowLayout->width, 
                SwapWithWindowLayout->height); 

        CFRelease(WindowRef);
    }
}

void CycleWindowInsideLayout(int ScreenIndex)
{
    DetectWindowBelowCursor();
    GetLayoutOfWindow(&FocusedWindow);

    std::vector<window_info*> ScreenWindowLst = GetAllWindowsOnDisplay(ScreenIndex);
    screen_layout *LayoutMaster = &ScreenLayoutLst[ScreenIndex];

    int SpaceIndex = GetSpaceOfWindow(&FocusedWindow);
    if(SpaceIndex == -1)
        return;

    int ActiveLayoutIndex = SpacesLst[SpaceIndex].active_layout_index;

    window_layout *FocusedWindowLayout = FocusedWindow.layout;
    if(!FocusedWindowLayout)
        return;

    int MaxLayoutTiles = LayoutMaster->layouts[ActiveLayoutIndex].layouts.size();
    if(ScreenWindowLst.size() <= MaxLayoutTiles)
        return;

    AXUIElementRef WindowRef;
    window_info *Window = ScreenWindowLst[ScreenWindowLst.size()-1];
    
    if(GetWindowRef(Window, &WindowRef))
    {
        SetWindowDimensions(WindowRef,
                Window,
                FocusedWindowLayout->x,
                FocusedWindowLayout->y,
                FocusedWindowLayout->width,
                FocusedWindowLayout->height);

        ProcessSerialNumber NewPSN;
        GetProcessForPID(Window->pid, &NewPSN);

        AXUIElementSetAttributeValue(WindowRef, kAXMainAttribute, kCFBooleanTrue);
        AXUIElementSetAttributeValue(WindowRef, kAXFocusedAttribute, kCFBooleanTrue);
        AXUIElementPerformAction (WindowRef, kAXRaiseAction);

        SetFrontProcessWithOptions(&NewPSN, kSetFrontProcessFrontWindowOnly);

        if(FocusedWindowRef != NULL)
            CFRelease(FocusedWindowRef);
        
        FocusedWindowRef = WindowRef;
        FocusedWindow = *Window;
        FocusedPSN = NewPSN;
    }
}

window_layout *GetLayoutOfWindow(window_info *Window)
{
    window_layout *Result = NULL;

    int ScreenID = GetDisplayOfWindow(Window)->id;
    int SpaceIndex = GetSpaceOfWindow(&FocusedWindow);
    if(SpaceIndex == -1)
        return Result;

    int ActiveLayoutIndex = SpacesLst[SpaceIndex].active_layout_index;
    int MaxLayoutSize = ScreenLayoutLst[ScreenID].layouts[ActiveLayoutIndex].layouts.size();

    for(int LayoutIndex = 0; LayoutIndex < MaxLayoutSize; ++LayoutIndex)
    {
        window_layout *Layout = &ScreenLayoutLst[ScreenID].layouts[ActiveLayoutIndex].layouts[LayoutIndex];
        if(WindowHasLayout(Window, Layout))
        {
            Window->layout = Layout;
            Window->layout_index = LayoutIndex;
            Result = Layout;
            break;
        }
    }

    return Result;
}

bool WindowHasLayout(window_info *Window, window_layout *Layout)
{
    if((Window->x >= Layout->x - 15 && Window->x <= Layout->x + 15) &&
        (Window->y >= Layout->y - 15 && Window->y <= Layout->y + 15 ) &&
        (Window->width >= Layout->width - 15 && Window->width <= Layout->width + 15) &&
        (Window->height >= Layout->height - 15 && Window->height <= Layout->height + 15))
            return true;

    return false;
}

void SetWindowLayoutValues(window_layout *Layout, int X, int Y, int Width, int Height)
{
    Layout->x = X;
    Layout->y = Y;
    Layout->width = Width;
    Layout->height = Height;
}

window_layout GetWindowLayoutForScreen(int ScreenIndex, const std::string &Name)
{
    window_layout Layout;
    Layout.name = "invalid";

    screen_info *Screen = &DisplayLst[ScreenIndex];
    if(Screen)
    {
        for(int LayoutIndex = 0; LayoutIndex < LayoutLst.size(); ++LayoutIndex)
        {
            if(LayoutLst[LayoutIndex].name == Name)
            {
                Layout = LayoutLst[LayoutIndex];
                break;
            }
        }

        if(Name == "fullscreen")
            SetWindowLayoutValues(&Layout, 
                    Screen->x + Layout.gap_x, 
                    Screen->y + Layout.gap_y, 
                    (Screen->width - (Layout.gap_vertical * 2)), 
                    (Screen->height - (Layout.gap_y * 1.5f)));
        else if(Name == "left vertical split")
            SetWindowLayoutValues(&Layout, 
                    Screen->x + Layout.gap_x, 
                    Screen->y + Layout.gap_y, 
                    ((Screen->width / 2) - (Layout.gap_vertical * 1.5f)), 
                    (Screen->height - (Layout.gap_y * 1.5f)));
        else if(Name == "right vertical split")
            SetWindowLayoutValues(&Layout, 
                    Screen->x + ((Screen->width / 2) + (Layout.gap_vertical * 0.5f)), 
                    Screen->y + Layout.gap_y, 
                    ((Screen->width / 2) - (Layout.gap_vertical * 1.5f)), 
                    (Screen->height - (Layout.gap_y * 1.5f)));
        else if(Name == "upper horizontal split")
            SetWindowLayoutValues(&Layout, 
                    Screen->x + Layout.gap_x, 
                    Screen->y + Layout.gap_y, 
                    (Screen->width - (Layout.gap_vertical * 2)), 
                    ((Screen->height / 2) - (Layout.gap_horizontal * 1.0f)));
        else if(Name == "lower horizontal split")
            SetWindowLayoutValues(&Layout, 
                    Screen->x + Layout.gap_x, 
                    Screen->y + ((Screen->height / 2) + (Layout.gap_horizontal * 0.5f) + (Layout.gap_y / 8)), 
                    (Screen->width - (Layout.gap_vertical * 2)), 
                    ((Screen->height / 2) - (Layout.gap_horizontal * 1.0f)));
        else if(Name == "upper left split")
            SetWindowLayoutValues(&Layout, 
                    Screen->x + Layout.gap_x, 
                    Screen->y + Layout.gap_y, 
                    ((Screen->width / 2) - (Layout.gap_vertical * 1.5f)), 
                    ((Screen->height / 2) - (Layout.gap_horizontal * 1.0f)));
        else if(Name == "lower left split")
            SetWindowLayoutValues(&Layout, 
                    Screen->x + Layout.gap_x, 
                    Screen->y + ((Screen->height / 2) + (Layout.gap_horizontal * 0.5f) + (Layout.gap_y / 8)), 
                    ((Screen->width / 2) - (Layout.gap_vertical * 1.5f)), 
                    ((Screen->height / 2) - (Layout.gap_horizontal * 1.0f)));
        else if(Name == "upper right split")
            SetWindowLayoutValues(&Layout, 
                    Screen->x + ((Screen->width / 2) + (Layout.gap_vertical * 0.5f)), 
                    Screen->y + Layout.gap_y, 
                    ((Screen->width / 2) - (Layout.gap_vertical * 1.5f)), 
                    ((Screen->height / 2) - (Layout.gap_horizontal * 1.0f)));
        else if(Name == "lower right split")
            SetWindowLayoutValues(&Layout, 
                    Screen->x + ((Screen->width / 2) + (Layout.gap_vertical * 0.5f)), 
                    Screen->y + ((Screen->height / 2) + (Layout.gap_horizontal * 0.5f) + (Layout.gap_y / 8)), 
                    ((Screen->width / 2) - (Layout.gap_vertical * 1.5f)), 
                    ((Screen->height / 2) - (Layout.gap_horizontal * 1.0f)));
    }

    return Layout;
}

void InitWindowLayouts()
{
    window_layout ScreenVerticalSplit;
    ScreenVerticalSplit.gap_x = 30;
    ScreenVerticalSplit.gap_y = 40;
    ScreenVerticalSplit.gap_vertical = 30;
    ScreenVerticalSplit.gap_horizontal = 30;

    ScreenVerticalSplit.name = "fullscreen";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.name = "left vertical split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.name = "right vertical split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.name = "upper horizontal split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.name = "lower horizontal split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.name = "upper left split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.name = "lower left split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.name = "upper right split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.name = "lower right split";
    LayoutLst.push_back(ScreenVerticalSplit);

    // Screen Layouts
    for(int ScreenIndex = 0; ScreenIndex < MaxDisplayCount; ++ScreenIndex)
    {
        screen_layout LayoutMaster;

        window_group_layout LayoutTallLeft;
        LayoutTallLeft.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "left vertical split"));
        LayoutTallLeft.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "upper right split"));
        LayoutTallLeft.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "lower right split"));
        LayoutMaster.layouts.push_back(LayoutTallLeft);

        window_group_layout LayoutTallRight;
        LayoutTallRight.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "upper left split"));
        LayoutTallRight.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "lower left split"));
        LayoutTallRight.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "right vertical split"));
        LayoutMaster.layouts.push_back(LayoutTallRight);

        window_group_layout LayoutVerticalSplit;
        LayoutVerticalSplit.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "left vertical split"));
        LayoutVerticalSplit.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "right vertical split"));
        LayoutMaster.layouts.push_back(LayoutVerticalSplit);

        window_group_layout LayoutQuarters;
        LayoutQuarters.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "upper left split"));
        LayoutQuarters.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "lower left split"));
        LayoutQuarters.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "upper right split"));
        LayoutQuarters.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "lower right split"));
        LayoutMaster.layouts.push_back(LayoutQuarters);

        window_group_layout LayoutFullscreen;
        LayoutFullscreen.layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "fullscreen"));
        LayoutMaster.layouts.push_back(LayoutFullscreen);

        LayoutMaster.number_of_layouts = LayoutMaster.layouts.size();;
        ScreenLayoutLst.push_back(LayoutMaster);
    }
}
