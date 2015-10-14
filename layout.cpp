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

    int ActiveLayoutIndex = SpacesLst[SpaceIndex].NextLayoutIndex;
    SpacesLst[SpaceIndex].ActiveLayoutIndex = ActiveLayoutIndex;

    if(ActiveLayoutIndex + 1 < LayoutMaster->NumberOfLayouts)
        SpacesLst[SpaceIndex].NextLayoutIndex = ActiveLayoutIndex + 1;
    else
        SpacesLst[SpaceIndex].NextLayoutIndex = 0;

    for(int WindowIndex = 0; WindowIndex < ScreenWindowLst.size(); ++WindowIndex)
    {
        window_info *Window = ScreenWindowLst[WindowIndex];
        AXUIElementRef WindowRef;
        if(GetWindowRef(Window, &WindowRef))
        {
            if(WindowIndex < LayoutMaster->Layouts[ActiveLayoutIndex].Layouts.size())
            {
                window_layout *Layout = &LayoutMaster->Layouts[ActiveLayoutIndex].Layouts[WindowIndex];
                Window->Layout = Layout;
                Window->LayoutIndex = WindowIndex;
                SetWindowDimensions(WindowRef, Window, Layout->X, Layout->Y, Layout->Width, Layout->Height); 
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

    int ActiveLayoutIndex = SpacesLst[SpaceIndex].ActiveLayoutIndex;

    window_layout *FocusedWindowLayout = FocusedWindow.Layout;
    int FocusedWindowLayoutIndex = FocusedWindow.LayoutIndex;
    if(!FocusedWindowLayout)
        return;

    int SwapWithWindowLayoutIndex = FocusedWindowLayoutIndex + Shift;
    if(SwapWithWindowLayoutIndex < 0 || 
        SwapWithWindowLayoutIndex >= LayoutMaster->NumberOfLayouts)
            return;

    int SwapWithWindowIndex = -1;
    window_layout *SwapWithWindowLayout = &LayoutMaster->Layouts[ActiveLayoutIndex].Layouts[SwapWithWindowLayoutIndex];
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
                FocusedWindowLayout->X, 
                FocusedWindowLayout->Y, 
                FocusedWindowLayout->Width, 
                FocusedWindowLayout->Height); 

        SetWindowDimensions(FocusedWindowRef, 
                &FocusedWindow, 
                SwapWithWindowLayout->X, 
                SwapWithWindowLayout->Y, 
                SwapWithWindowLayout->Width, 
                SwapWithWindowLayout->Height); 

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

    int ActiveLayoutIndex = SpacesLst[SpaceIndex].ActiveLayoutIndex;

    window_layout *FocusedWindowLayout = FocusedWindow.Layout;
    if(!FocusedWindowLayout)
        return;

    int MaxLayoutTiles = LayoutMaster->Layouts[ActiveLayoutIndex].Layouts.size();
    if(ScreenWindowLst.size() <= MaxLayoutTiles)
        return;

    AXUIElementRef WindowRef;
    window_info *Window = ScreenWindowLst[ScreenWindowLst.size()-1];
    
    if(GetWindowRef(Window, &WindowRef))
    {
        SetWindowDimensions(WindowRef,
                Window,
                FocusedWindowLayout->X,
                FocusedWindowLayout->Y,
                FocusedWindowLayout->Width,
                FocusedWindowLayout->Height);

        ProcessSerialNumber NewPSN;
        GetProcessForPID(Window->PID, &NewPSN);

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

    int ScreenID = GetDisplayOfWindow(Window)->ID;
    int SpaceIndex = GetSpaceOfWindow(&FocusedWindow);
    if(SpaceIndex == -1)
        return Result;

    int ActiveLayoutIndex = SpacesLst[SpaceIndex].ActiveLayoutIndex;
    int MaxLayoutSize = ScreenLayoutLst[ScreenID].Layouts[ActiveLayoutIndex].Layouts.size();

    for(int LayoutIndex = 0; LayoutIndex < MaxLayoutSize; ++LayoutIndex)
    {
        window_layout *Layout = &ScreenLayoutLst[ScreenID].Layouts[ActiveLayoutIndex].Layouts[LayoutIndex];
        if(WindowHasLayout(Window, Layout))
        {
            Window->Layout = Layout;
            Window->LayoutIndex = LayoutIndex;
            Result = Layout;
            break;
        }
    }

    return Result;
}

bool WindowHasLayout(window_info *Window, window_layout *Layout)
{
    if((Window->X >= Layout->X - 15 && Window->X <= Layout->X + 15) &&
        (Window->Y >= Layout->Y - 15 && Window->Y <= Layout->Y + 15 ) &&
        (Window->Width >= Layout->Width - 15 && Window->Width <= Layout->Width + 15) &&
        (Window->Height >= Layout->Height - 15 && Window->Height <= Layout->Height + 15))
            return true;

    return false;
}

void SetWindowLayoutValues(window_layout *Layout, int X, int Y, int Width, int Height)
{
    Layout->X = X;
    Layout->Y = Y;
    Layout->Width = Width;
    Layout->Height = Height;
}

window_layout GetWindowLayoutForScreen(int ScreenIndex, const std::string &Name)
{
    window_layout Layout;
    Layout.Name = "invalid";

    screen_info *Screen = &DisplayLst[ScreenIndex];
    if(Screen)
    {
        for(int LayoutIndex = 0; LayoutIndex < LayoutLst.size(); ++LayoutIndex)
        {
            if(LayoutLst[LayoutIndex].Name == Name)
            {
                Layout = LayoutLst[LayoutIndex];
                break;
            }
        }

        if(Name == "fullscreen")
            SetWindowLayoutValues(&Layout, 
                    Screen->X + Layout.GapX, 
                    Screen->Y + Layout.GapY, 
                    (Screen->Width - (Layout.GapVertical * 2)), 
                    (Screen->Height - (Layout.GapY * 1.5f)));
        else if(Name == "left vertical split")
            SetWindowLayoutValues(&Layout, 
                    Screen->X + Layout.GapX, 
                    Screen->Y + Layout.GapY, 
                    ((Screen->Width / 2) - (Layout.GapVertical * 1.5f)), 
                    (Screen->Height - (Layout.GapY * 1.5f)));
        else if(Name == "right vertical split")
            SetWindowLayoutValues(&Layout, 
                    Screen->X + ((Screen->Width / 2) + (Layout.GapVertical * 0.5f)), 
                    Screen->Y + Layout.GapY, 
                    ((Screen->Width / 2) - (Layout.GapVertical * 1.5f)), 
                    (Screen->Height - (Layout.GapY * 1.5f)));
        else if(Name == "upper horizontal split")
            SetWindowLayoutValues(&Layout, 
                    Screen->X + Layout.GapX, 
                    Screen->Y + Layout.GapY, 
                    (Screen->Width - (Layout.GapVertical * 2)), 
                    ((Screen->Height / 2) - (Layout.GapHorizontal * 1.0f)));
        else if(Name == "lower horizontal split")
            SetWindowLayoutValues(&Layout, 
                    Screen->X + Layout.GapX, 
                    Screen->Y + ((Screen->Height / 2) + (Layout.GapHorizontal * 0.5f) + (Layout.GapY / 8)), 
                    (Screen->Width - (Layout.GapVertical * 2)), 
                    ((Screen->Height / 2) - (Layout.GapHorizontal * 1.0f)));
        else if(Name == "upper left split")
            SetWindowLayoutValues(&Layout, 
                    Screen->X + Layout.GapX, 
                    Screen->Y + Layout.GapY, 
                    ((Screen->Width / 2) - (Layout.GapVertical * 1.5f)), 
                    ((Screen->Height / 2) - (Layout.GapHorizontal * 1.0f)));
        else if(Name == "lower left split")
            SetWindowLayoutValues(&Layout, 
                    Screen->X + Layout.GapX, 
                    Screen->Y + ((Screen->Height / 2) + (Layout.GapHorizontal * 0.5f) + (Layout.GapY / 8)), 
                    ((Screen->Width / 2) - (Layout.GapVertical * 1.5f)), 
                    ((Screen->Height / 2) - (Layout.GapHorizontal * 1.0f)));
        else if(Name == "upper right split")
            SetWindowLayoutValues(&Layout, 
                    Screen->X + ((Screen->Width / 2) + (Layout.GapVertical * 0.5f)), 
                    Screen->Y + Layout.GapY, 
                    ((Screen->Width / 2) - (Layout.GapVertical * 1.5f)), 
                    ((Screen->Height / 2) - (Layout.GapHorizontal * 1.0f)));
        else if(Name == "lower right split")
            SetWindowLayoutValues(&Layout, 
                    Screen->X + ((Screen->Width / 2) + (Layout.GapVertical * 0.5f)), 
                    Screen->Y + ((Screen->Height / 2) + (Layout.GapHorizontal * 0.5f) + (Layout.GapY / 8)), 
                    ((Screen->Width / 2) - (Layout.GapVertical * 1.5f)), 
                    ((Screen->Height / 2) - (Layout.GapHorizontal * 1.0f)));
    }

    return Layout;
}

void InitWindowLayouts()
{
    window_layout ScreenVerticalSplit;
    ScreenVerticalSplit.GapX = 30;
    ScreenVerticalSplit.GapY = 40;
    ScreenVerticalSplit.GapVertical = 30;
    ScreenVerticalSplit.GapHorizontal = 30;

    ScreenVerticalSplit.Name = "fullscreen";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.Name = "left vertical split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.Name = "right vertical split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.Name = "upper horizontal split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.Name = "lower horizontal split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.Name = "upper left split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.Name = "lower left split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.Name = "upper right split";
    LayoutLst.push_back(ScreenVerticalSplit);

    ScreenVerticalSplit.Name = "lower right split";
    LayoutLst.push_back(ScreenVerticalSplit);

    // Screen Layouts
    for(int ScreenIndex = 0; ScreenIndex < MaxDisplayCount; ++ScreenIndex)
    {
        screen_layout LayoutMaster;

        window_group_layout LayoutTallLeft;
        LayoutTallLeft.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "left vertical split"));
        LayoutTallLeft.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "upper right split"));
        LayoutTallLeft.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "lower right split"));
        LayoutMaster.Layouts.push_back(LayoutTallLeft);

        window_group_layout LayoutTallRight;
        LayoutTallRight.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "upper left split"));
        LayoutTallRight.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "lower left split"));
        LayoutTallRight.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "right vertical split"));
        LayoutMaster.Layouts.push_back(LayoutTallRight);

        window_group_layout LayoutVerticalSplit;
        LayoutVerticalSplit.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "left vertical split"));
        LayoutVerticalSplit.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "right vertical split"));
        LayoutMaster.Layouts.push_back(LayoutVerticalSplit);

        window_group_layout LayoutQuarters;
        LayoutQuarters.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "upper left split"));
        LayoutQuarters.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "lower left split"));
        LayoutQuarters.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "upper right split"));
        LayoutQuarters.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "lower right split"));
        LayoutMaster.Layouts.push_back(LayoutQuarters);

        window_group_layout LayoutFullscreen;
        LayoutFullscreen.Layouts.push_back(GetWindowLayoutForScreen(ScreenIndex, "fullscreen"));
        LayoutMaster.Layouts.push_back(LayoutFullscreen);

        LayoutMaster.NumberOfLayouts = LayoutMaster.Layouts.size();;
        ScreenLayoutLst.push_back(LayoutMaster);
    }
}
