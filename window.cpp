#include "kwm.h"

static CGWindowListOption OsxWindowListOption = kCGWindowListOptionOnScreenOnly |
                                                kCGWindowListExcludeDesktopElements;

extern std::vector<window_info> WindowLst;
extern export_table ExportTable;

extern ProcessSerialNumber FocusedPSN;
extern AXUIElementRef FocusedWindowRef;
extern window_info *FocusedWindow;

extern int CurrentSpace;
extern int PrevSpace;

void WriteNameOfFocusedWindowToFile()
{
    std::string Command = "echo '" + FocusedWindow->Owner + " - " + FocusedWindow->Name + "' > focus.kwm";
    system(Command.c_str());
}

bool WindowsAreEqual(window_info *Window, window_info *Match)
{
    bool Result = false;

    if(Window && Match)
    {
        if(Window->PID == Match->PID &&
           Window->WID == Match->WID &&
           Window->Owner == Match->Owner &&
           Window->Name == Match->Name &&
           Window->X == Match->X &&
           Window->Y == Match->Y &&
           Window->Width == Match->Width &&
           Window->Height == Match->Height &&
           Window->Layer == Match->Layer)
        {
            Result = true;
        }
    }

    return Result;
}

bool FilterWindowList()
{
    bool Result = true;
    std::vector<window_info> FilteredWindowLst;

    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        if(WindowLst[WindowIndex].Owner == "Dock")
            Result = false;

        if(WindowLst[WindowIndex].Layer == 0)
            FilteredWindowLst.push_back(WindowLst[WindowIndex]);
    }

    WindowLst = FilteredWindowLst;
    return Result;
}

bool IsWindowFloating(int WindowID)
{
    bool Result = false;

    for(int WindowIndex = 0; WindowIndex < ExportTable.FloatingWindowLst.size(); ++WindowIndex)
    {
        if(WindowID == ExportTable.FloatingWindowLst[WindowIndex])
        {
            DEBUG("IsWindowFloating(): floating " << WindowID)
            Result = true;
            break;
        }
    }

    return Result;
}

bool IsWindowBelowCursor(window_info *Window)
{
    bool Result = false;

    if(Window)
    {
        CGEventRef Event = CGEventCreate(NULL);
        CGPoint Cursor = CGEventGetLocation(Event);
        CFRelease(Event);

        if(Cursor.x >= Window->X && 
           Cursor.x <= Window->X + Window->Width &&
           Cursor.y >= Window->Y &&
           Cursor.y <= Window->Y + Window->Height)
        {
            Result = true;
        }
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

        if(!IsSpaceTransitionInProgress() && FilterWindowList())
        {
            CheckIfSpaceTransitionOccurred();
            FocusWindowBelowCursor();
            ShouldWindowNodeTreeUpdate(OldWindowListCount);
        }
    }
}

bool IsSpaceTransitionInProgress()
{
    bool Result = false;
    int Space = -1;

    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        int SpaceOfWindow = GetSpaceOfWindow(&WindowLst[WindowIndex]);
        if(Space == -1 && SpaceOfWindow != -1)
            Space = SpaceOfWindow;

        if(Space != -1 && SpaceOfWindow != -1)
        {
            if(Space != SpaceOfWindow)
            {
                DEBUG("IsSpaceTransitionInProgress() Space transition detected")
                Result = true;
                break;
            }
        }
    }

    return Result;
}

void CheckIfSpaceTransitionOccurred()
{
    if(!WindowLst.empty())
    {
        PrevSpace = CurrentSpace;
        for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
        {
            int Temp = GetSpaceOfWindow(&WindowLst[WindowIndex]);
            if(Temp != -1)
            {
                CurrentSpace = Temp;
                break;
            }
        }
    }
}

void FocusWindowBelowCursor()
{
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        if(IsWindowBelowCursor(&WindowLst[WindowIndex]))
        {
            if(!WindowsAreEqual(FocusedWindow, &WindowLst[WindowIndex]))
            {
                int NewSpace = GetSpaceOfWindow(&WindowLst[WindowIndex]);
                if(NewSpace == -1)
                    AddWindowToSpace(WindowLst[WindowIndex].WID, CurrentSpace);

                DEBUG("DetectWindowBelowCursor() Current space: " << CurrentSpace)
                if(CurrentSpace != -1)
                    CreateWindowNodeTree();

                // Note: Memory leak related to this function-call
                SetWindowFocus(&WindowLst[WindowIndex]);
            }
            else
            {
                WriteNameOfFocusedWindowToFile();
            }
            break;
        }
    }
}

void ShouldWindowNodeTreeUpdate(int OldWindowListCount)
{
    if(WindowLst.empty() || !FocusedWindow)
        return;

    if(CurrentSpace != -1 && PrevSpace == CurrentSpace)
    {
        if(WindowLst.size() > OldWindowListCount)
        {
            screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
            for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
            {
                if(GetNodeFromWindowID(Screen->Space[CurrentSpace], WindowLst[WindowIndex].WID) == NULL)
                    AddWindowToTree(WindowLst[WindowIndex].WID);
            }
        }
        else if(WindowLst.size() < OldWindowListCount)
        {
            screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
            tree_node *RootNode = Screen->Space[CurrentSpace];
            std::vector<int> WindowIDsInTree;

            tree_node *CurrentNode = RootNode;
            while(CurrentNode->LeftChild)
                CurrentNode = CurrentNode->LeftChild;

            while(CurrentNode)
            {
                WindowIDsInTree.push_back(CurrentNode->WindowID);
                CurrentNode = GetNearestNodeToTheRight(CurrentNode);
            }

            for(int IDIndex = 0; IDIndex < WindowIDsInTree.size(); ++IDIndex)
            {
                bool Found = false;
                for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
                {
                    if(WindowLst[WindowIndex].WID == WindowIDsInTree[IDIndex])
                    {
                        Found = true;
                        break;
                    }
                }

                if(!Found)
                    RemoveWindowFromTree(WindowIDsInTree[IDIndex], false);
            }
        }
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

void AddWindowToTree(int WindowID)
{
    screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
    if(Screen && Screen->Space[CurrentSpace])
    {
        tree_node *RootNode = Screen->Space[CurrentSpace];
        tree_node *CurrentNode = RootNode;
        if(WindowID == FocusedWindow->WID)
        {
            while(!IsLeafNode(CurrentNode))
            {
                if(!IsLeafNode(CurrentNode->LeftChild) && IsLeafNode(CurrentNode->RightChild))
                    CurrentNode = CurrentNode->RightChild;
                else
                    CurrentNode = CurrentNode->LeftChild;
            }
        }
        else
        {
            CurrentNode = GetNodeFromWindowID(RootNode, FocusedWindow->WID);
        }

        DEBUG("AddWindowToTree() Create pair of leafs")
        CreateLeafNodePair(Screen, CurrentNode, CurrentNode->WindowID, WindowID, ExportTable.KwmSplitMode);
        CurrentNode->WindowID = -1;
        ApplyNodeContainer(CurrentNode);
    }
}

void AddWindowToTree()
{
    AddWindowToTree(FocusedWindow->WID);
}

void RemoveWindowFromTree(int WindowID, bool Center)
{
    screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
    if(Screen && Screen->Space[CurrentSpace])
    {
        tree_node *RootNode = Screen->Space[CurrentSpace];
        tree_node *WindowNode = GetNodeFromWindowID(RootNode, WindowID);

        if(WindowNode)
        {
            tree_node *Parent = WindowNode->Parent;
            if(Parent && Parent->LeftChild && Parent->RightChild)
            {
                tree_node *OldLeftChild = Parent->LeftChild;
                tree_node *OldRightChild = Parent->RightChild;
                tree_node *AccessChild;

                Parent->LeftChild = NULL;
                Parent->RightChild = NULL;

                if(OldRightChild == WindowNode)
                {
                    if(OldLeftChild)
                        AccessChild = OldLeftChild;
                }
                else
                {
                    if(OldRightChild)
                        AccessChild = OldRightChild;
                }

                if(AccessChild)
                {
                    DEBUG("RemoveWindowFromTree() " << FocusedWindow->Name)
                    Parent->WindowID = AccessChild->WindowID;
                    if(AccessChild->LeftChild && AccessChild->RightChild)
                        CreateLeafNodePair(Screen, Parent, AccessChild->LeftChild->WindowID, AccessChild->RightChild->WindowID, ExportTable.KwmSplitMode);

                    free(AccessChild);
                    free(WindowNode);
                    ApplyNodeContainer(Parent);

                    if(Center)
                    {
                        int NewX = Screen->Width / 4;
                        int NewY = Screen->Height / 4;
                        int NewWidth = Screen->Width / 2;
                        int NewHeight = Screen->Height / 2;
                        SetWindowDimensions(FocusedWindowRef, FocusedWindow, NewX, NewY, NewWidth, NewHeight);
                    }
                }
            }
        }
    }
}

void RemoveWindowFromTree()
{
    RemoveWindowFromTree(FocusedWindow->WID, true);
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
                    ResizeWindowToContainerSize(Screen->Space[CurrentSpace]);
                }
            }
            else
            {
                DEBUG("ToggleFocusedWindowFullscreen() Restore old size")
                Screen->Space[CurrentSpace]->WindowID = -1;

                Node = GetNodeFromWindowID(Screen->Space[CurrentSpace], FocusedWindow->WID);
                if(Node)
                    ResizeWindowToContainerSize(Node);
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

    WriteNameOfFocusedWindowToFile();
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

void ResizeWindowToContainerSize(tree_node *Node)
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

            CFRelease(WindowRef);
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
    
    if(!App)
    {
        DEBUG("GetWindowRef() Failed to get App for: " << Window->Name)
        return false;
    }

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
    CFRelease(App);
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
            else if(KeyStr == "kCGWindowOwnerName")
                WindowLst[WindowLst.size()-1].Owner = ValueStr;
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
