#include "kwm.h"

static CGWindowListOption OsxWindowListOption = kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements;

extern export_table ExportTable;
extern std::vector<window_info> WindowLst;
extern ProcessSerialNumber FocusedPSN;
extern AXUIElementRef FocusedWindowRef;
extern window_info *FocusedWindow;
extern int CurrentSpace;
extern int PrevSpace;

bool WindowsAreEqual(window_info *Window, window_info *Match)
{
    bool Result = Window == Match;

    if(Window && Match)
    {
        if(Window->X == Match->X &&
           Window->Y == Match->Y &&
           Window->Width == Match->Width &&
           Window->Height == Match->Height &&
           Window->Name == Match->Name)
               Result = true;
    }

    return Result;
}

void FilterWindowList()
{
    std::vector<window_info> FilteredWindowLst;
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        if(WindowLst[WindowIndex].Layer == 0)
        {
           FilteredWindowLst.push_back(WindowLst[WindowIndex]);
        }
    }
    WindowLst = FilteredWindowLst;
}

bool IsWindowBelowCursor(window_info *Window)
{
    bool Result = Window == NULL;

    if(Window)
    {
        CGEventRef Event = CGEventCreate(NULL);
        CGPoint Cursor = CGEventGetLocation(Event);
        CFRelease(Event);
        if(Cursor.x >= Window->X && 
           Cursor.x <= Window->X + Window->Width &&
           Cursor.y >= Window->Y &&
           Cursor.y <= Window->Y + Window->Height)
               Result = true;
    }
        
    return Result;
}

void DetectWindowBelowCursor()
{
    int OldWindowListCount = WindowLst.size();
    WindowLst.clear();

    CFArrayRef OsxWindowLst = CGWindowListCopyWindowInfo(OsxWindowListOption, kCGNullWindowID);
    if(OsxWindowLst)
    {
        CFIndex OsxWindowCount = CFArrayGetCount(OsxWindowLst);
        for(CFIndex WindowIndex = 0; WindowIndex < OsxWindowCount; ++WindowIndex)
        {
            CFDictionaryRef Elem = (CFDictionaryRef)CFArrayGetValueAtIndex(OsxWindowLst, WindowIndex);
            WindowLst.push_back(window_info());
            CFDictionaryApplyFunction(Elem, GetWindowInfo, NULL);
        }
        CFRelease(OsxWindowLst);

        FilterWindowList();
        CheckIfSpaceTransitionOccurred();

        for(int i = 0; i < WindowLst.size(); ++i)
        {
            if(IsWindowBelowCursor(&WindowLst[i]))
            {
                if(!WindowsAreEqual(FocusedWindow, &WindowLst[i]))
                {
                    int NewSpace = GetSpaceOfWindow(&WindowLst[i]);
                    if(NewSpace == -1)
                        AddWindowToSpace(WindowLst[i].WID, CurrentSpace);

                    DEBUG("DetectWindowBelowCursor() Current space: " << CurrentSpace)
                    if(CurrentSpace != -1)
                        CreateWindowNodeTree();

                    // Note: Memory leak related to this function-call
                    SetWindowFocus(&WindowLst[i]);
                }
                break;
            }
        }

        ShouldWindowNodeTreeUpdate(OldWindowListCount);
    }
}

void CheckIfSpaceTransitionOccurred()
{
    int ActiveSpace = -1;
    for(int Index = 0; Index < WindowLst.size(); ++Index)
    {
        int Temp = GetSpaceOfWindow(&WindowLst[Index]);
        if(Temp != -1)
        {
            ActiveSpace = Temp;
            break;
        }
    }

    if(ActiveSpace != -1)
    {
        PrevSpace = CurrentSpace;
        CurrentSpace = ActiveSpace;
    }
}

void ShouldWindowNodeTreeUpdate(int OldWindowListCount)
{
    if(OldWindowListCount != WindowLst.size() && !WindowLst.empty())
    {
        if(CurrentSpace != -1 && PrevSpace == CurrentSpace)
            RefreshWindowNodeTree();
    }
}

void CreateWindowNodeTree()
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen)
        {
            if(Screen->Space[CurrentSpace] == NULL)
            {
                DEBUG("CreateWindowNodeTree() Create Tree")
                std::vector<int> WindowIDs = GetAllWindowIDsOnDisplay(Screen->ID);
                Screen->Space[CurrentSpace] = CreateTreeFromWindowIDList(Screen, WindowIDs);
                ApplyNodeContainer(Screen->Space[CurrentSpace]);
            }
        }
    }
}

void RefreshWindowNodeTree()
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen)
        {
            if(Screen->Space[CurrentSpace])
            {
                DestroyNodeTree(Screen->Space[CurrentSpace]);

                DEBUG("RefreshWindowNodeTree() Destroy and Create Tree")
                std::vector<int> WindowIDs = GetAllWindowIDsOnDisplay(Screen->ID);
                Screen->Space[CurrentSpace] = CreateTreeFromWindowIDList(Screen, WindowIDs);
                ApplyNodeContainer(Screen->Space[CurrentSpace]);
            }
        }
    }
}

void ToggleFocusedWindowFullscreen()
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen && Screen->Space[CurrentSpace])
        {
            tree_node *Node;
            if(Screen->Space[CurrentSpace]->WindowID == -1)
            {
                Node = GetNodeFromWindowID(Screen->Space[CurrentSpace], FocusedWindow->WID);
                if(Node)
                {
                    DEBUG("ToggleFocusedWindowFullscreen() Set fullscreen")
                    Screen->Space[CurrentSpace]->WindowID = Node->WindowID;
                    ResizeWindow(Screen->Space[CurrentSpace]);
                }
            }
            else
            {
                DEBUG("ToggleFocusedWindowFullscreen() Restore old size")
                Screen->Space[CurrentSpace]->WindowID = -1;

                Node = GetNodeFromWindowID(Screen->Space[CurrentSpace], FocusedWindow->WID);
                if(Node)
                    ResizeWindow(Node);
            }
        }
    }
}

void SwapFocusedWindowWithNearest(int Shift)
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen && Screen->Space[CurrentSpace])
        {
            tree_node *FocusedWindowNode = GetNodeFromWindowID(Screen->Space[CurrentSpace], FocusedWindow->WID);
            if(FocusedWindowNode)
            {
                tree_node *NewFocusNode;

                if(Shift == 1)
                    NewFocusNode = GetNearestNodeToTheRight(FocusedWindowNode);
                else if(Shift == -1)
                    NewFocusNode = GetNearestNodeToTheLeft(FocusedWindowNode);

                if(NewFocusNode)
                    SwapNodeWindowIDs(FocusedWindowNode, NewFocusNode);
            }
        }
    }
}

void ShiftWindowFocus(int Shift)
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen && Screen->Space[CurrentSpace])
        {
            tree_node *FocusedWindowNode = GetNodeFromWindowID(Screen->Space[CurrentSpace], FocusedWindow->WID);
            if(FocusedWindowNode)
            {
                tree_node *NewFocusNode;

                if(Shift == 1)
                    NewFocusNode = GetNearestNodeToTheRight(FocusedWindowNode);
                else if(Shift == -1)
                    NewFocusNode = GetNearestNodeToTheLeft(FocusedWindowNode);

                if(NewFocusNode)
                {
                    window_info *NewWindow = GetWindowByID(NewFocusNode->WindowID);
                    if(NewWindow)
                    {
                        DEBUG("ShiftWindowFocus() changing focus to " << NewWindow->Name)
                        SetWindowFocus(NewWindow);
                    }
                }
            }
        }
    }
}

void CloseWindowByRef(AXUIElementRef WindowRef)
{
    AXUIElementRef ActionClose;
    AXUIElementCopyAttributeValue(WindowRef, kAXCloseButtonAttribute, (CFTypeRef*)&ActionClose);
    AXUIElementPerformAction(ActionClose, kAXPressAction);
}

void CloseWindow(window_info *Window)
{
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
        CloseWindowByRef(WindowRef);
}

void SetWindowRefFocus(AXUIElementRef WindowRef, window_info *Window)
{
    ProcessSerialNumber NewPSN;
    GetProcessForPID(Window->PID, &NewPSN);

    if(FocusedWindowRef != NULL)
        CFRelease(FocusedWindowRef);

    FocusedPSN = NewPSN;
    FocusedWindowRef = WindowRef;
    FocusedWindow = Window;

    ExportTable.FocusedWindowRef = FocusedWindowRef;
    ExportTable.FocusedWindow = FocusedWindow;

    AXUIElementSetAttributeValue(FocusedWindowRef, kAXMainAttribute, kCFBooleanTrue);
    AXUIElementSetAttributeValue(FocusedWindowRef, kAXFocusedAttribute, kCFBooleanTrue);
    AXUIElementPerformAction(FocusedWindowRef, kAXRaiseAction);

    if(ExportTable.KwmFocusMode == FocusAutoraise)
        SetFrontProcessWithOptions(&FocusedPSN, kSetFrontProcessFrontWindowOnly);

    DEBUG("SetWindowRefFocus() Focused Window: " << FocusedWindow->Name)
}

void SetWindowFocus(window_info *Window)
{
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
        SetWindowRefFocus(WindowRef, Window);
}

void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height)
{
    CGPoint WindowPos = CGPointMake(X, Y);
    CFTypeRef NewWindowPos = (CFTypeRef)AXValueCreate(kAXValueCGPointType, (const void*)&WindowPos);

    CGSize WindowSize = CGSizeMake(Width, Height);
    CFTypeRef NewWindowSize = (CFTypeRef)AXValueCreate(kAXValueCGSizeType, (void*)&WindowSize);

    AXUIElementSetAttributeValue(WindowRef, kAXPositionAttribute, NewWindowPos);
    AXUIElementSetAttributeValue(WindowRef, kAXSizeAttribute, NewWindowSize);

    Window->X = X;
    Window->Y = Y;
    Window->Width = Width;
    Window->Height = Height;

    DEBUG("SetWindowDimensions() Window " << Window->Name << ": " << Window->X << "," << Window->Y)

    if(NewWindowPos != NULL)
        CFRelease(NewWindowPos);
    if(NewWindowSize != NULL)
        CFRelease(NewWindowSize);
}

void ResizeWindow(tree_node *Node)
{
    window_info *Window = GetWindowByID(Node->WindowID);
    if(Window)
    {
        AXUIElementRef WindowRef;
        if(GetWindowRef(Window, &WindowRef))
        {
            SetWindowDimensions(WindowRef, Window,
                    Node->Container.X, Node->Container.Y, 
                    Node->Container.Width, Node->Container.Height);
        }
    }
}

std::string GetWindowTitle(AXUIElementRef WindowRef)
{
    CFStringRef Temp;
    std::string WindowTitle;

    AXUIElementCopyAttributeValue(WindowRef, kAXTitleAttribute, (CFTypeRef*)&Temp);
    if(CFStringGetCStringPtr(Temp, kCFStringEncodingMacRoman))
        WindowTitle = CFStringGetCStringPtr(Temp, kCFStringEncodingMacRoman);

    if(Temp != NULL)
        CFRelease(Temp);

    return WindowTitle;
}

CGSize GetWindowSize(AXUIElementRef WindowRef)
{
    AXValueRef Temp;
    CGSize WindowSize;

    AXUIElementCopyAttributeValue(WindowRef, kAXSizeAttribute, (CFTypeRef*)&Temp);
    AXValueGetValue(Temp, kAXValueCGSizeType, &WindowSize);

    if(Temp != NULL)
        CFRelease(Temp);

    return WindowSize;
}

CGPoint GetWindowPos(AXUIElementRef WindowRef)
{
    AXValueRef Temp;
    CGPoint WindowPos;

    AXUIElementCopyAttributeValue(WindowRef, kAXPositionAttribute, (CFTypeRef*)&Temp);
    AXValueGetValue(Temp, kAXValueCGPointType, &WindowPos);

    if(Temp != NULL)
        CFRelease(Temp);

    return WindowPos;
}

window_info *GetWindowByID(int WindowID)
{
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        if(WindowLst[WindowIndex].WID == WindowID)
            return &WindowLst[WindowIndex];
    }

    return NULL;
}

bool GetWindowRef(window_info *Window, AXUIElementRef *WindowRef)
{
    AXUIElementRef App = AXUIElementCreateApplication(Window->PID);
    bool Found = false;
    
    if(App)
    {
        CFArrayRef AppWindowLst;
        AXUIElementCopyAttributeValue(App, kAXWindowsAttribute, (CFTypeRef*)&AppWindowLst);
        if(AppWindowLst)
        {
            CFIndex DoNotFree = -1;
            CFIndex AppWindowCount = CFArrayGetCount(AppWindowLst);
            for(CFIndex WindowIndex = 0; WindowIndex < AppWindowCount; ++WindowIndex)
            {
                AXUIElementRef AppWindowRef = (AXUIElementRef)CFArrayGetValueAtIndex(AppWindowLst, WindowIndex);
                if(AppWindowRef != NULL)
                {
                    if(!Found)
                    {
                        std::string AppWindowTitle = GetWindowTitle(AppWindowRef);
                        CGPoint AppWindowPos = GetWindowPos(AppWindowRef);

                        if(Window->X == AppWindowPos.x && 
                           Window->Y == AppWindowPos.y &&
                            Window->Name == AppWindowTitle)
                        {
                            *WindowRef = AppWindowRef;
                            DoNotFree = WindowIndex;
                            Found = true;
                        }

                    }

                    if(WindowIndex != DoNotFree)
                        CFRelease(AppWindowRef);
                }
            }
        }
        else
        {
            DEBUG("GetWindowRef() Failed to get AppWindowLst for: " << Window->Name)
        }
        CFRelease(App);
    }
    else
    {
        DEBUG("GetWindowRef() Failed to get App for: " << Window->Name)
    }

    return Found;
}

void GetWindowInfo(const void *Key, const void *Value, void *Context)
{
    CFStringRef K = (CFStringRef)Key;
    std::string KeyStr = CFStringGetCStringPtr(K, kCFStringEncodingMacRoman);

    CFTypeID ID = CFGetTypeID(Value);
    if(ID == CFStringGetTypeID())
    {
        CFStringRef V = (CFStringRef)Value;
        if(CFStringGetCStringPtr(V, kCFStringEncodingMacRoman))
        {
            std::string ValueStr = CFStringGetCStringPtr(V, kCFStringEncodingMacRoman);
            if(KeyStr == "kCGWindowName")
                WindowLst[WindowLst.size()-1].Name = ValueStr;
        }
    }
    else if(ID == CFNumberGetTypeID())
    {
        int MyInt;
        CFNumberRef V = (CFNumberRef)Value;
        CFNumberGetValue(V, kCFNumberSInt64Type, &MyInt);
        if(KeyStr == "kCGWindowNumber")
            WindowLst[WindowLst.size()-1].WID = MyInt;
        else if(KeyStr == "kCGWindowOwnerPID")
            WindowLst[WindowLst.size()-1].PID = MyInt;
        else if(KeyStr == "kCGWindowLayer")
            WindowLst[WindowLst.size()-1].Layer = MyInt;
        else if(KeyStr == "X")
            WindowLst[WindowLst.size()-1].X = MyInt;
        else if(KeyStr == "Y")
            WindowLst[WindowLst.size()-1].Y = MyInt;
        else if(KeyStr == "Width")
            WindowLst[WindowLst.size()-1].Width = MyInt;
        else if(KeyStr == "Height")
            WindowLst[WindowLst.size()-1].Height = MyInt;
    }
    else if(ID == CFDictionaryGetTypeID())
    {
        CFDictionaryRef Elem = (CFDictionaryRef)Value;
        CFDictionaryApplyFunction(Elem, GetWindowInfo, NULL);
    } 
}
