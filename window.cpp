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
extern int MarkedWindowID;

window_info FocusedWindowCache;
int OldWindowListCount = -1;

extern "C" CGSConnectionID _CGSDefaultConnection(void);
#define CGSDefaultConnection _CGSDefaultConnection()

void WriteNameOfFocusedWindowToFile()
{
    std::string Command = "echo \"" + FocusedWindow->Owner + " - " + FocusedWindow->Name + "\" > focus.kwm";
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
        if(WindowLst[WindowIndex].Owner == "Dock" &&
           WindowLst[WindowIndex].Name == "")
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

bool IsWindowOnActiveSpace(window_info *Window)
{
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        if(WindowsAreEqual(Window, &WindowLst[WindowIndex]))
        {
            DEBUG("IsWindowOnActiveSpace() window found")
            return true;
        }
    }

    DEBUG("IsWindowOnActiveSpace() window was not found")
    return false;
}

bool IsSpaceTransitionInProgress()
{
    CFStringRef Display = CGSCopyManagedDisplayForSpace(CGSDefaultConnection, (CGSSpaceID)CurrentSpace);
    bool Result = CGSManagedDisplayIsAnimating(CGSDefaultConnection, (CFStringRef)Display);

    if(Result)
    {
        DEBUG("IsSpaceTransitionInProgress() Space transition detected")
    }

    return Result;
}

void FocusWindowBelowCursor()
{
    if(!IsSpaceTransitionInProgress())
    {
        for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
        {
            if(IsWindowBelowCursor(&WindowLst[WindowIndex]))
            {
                if(!WindowsAreEqual(FocusedWindow, &WindowLst[WindowIndex]))
                {
                    // Note: Memory leak related to this function-call
                    SetWindowFocus(&WindowLst[WindowIndex]);
                    DEBUG("FocusWindowBelowCursor() Current space: " << CurrentSpace)
                }
                else
                {
                    WriteNameOfFocusedWindowToFile();
                }
                break;
            }
        }
    }
}

void UpdateWindowTree()
{
    UpdateActiveWindowList();
    if(!IsSpaceTransitionInProgress() && FilterWindowList())
    {
        CreateWindowNodeTree();
        ShouldWindowNodeTreeUpdate();
    }
}

void UpdateActiveWindowList()
{
    OldWindowListCount = WindowLst.size();
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

        PrevSpace = CurrentSpace;
        CurrentSpace = CGSGetActiveSpace(CGSDefaultConnection);

        if(PrevSpace != CurrentSpace)
        {
            DEBUG("UpdateActiveWindowList() Space transition ended")
            FocusWindowBelowCursor();
        }
    }
}

void ShouldWindowNodeTreeUpdate()
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(WindowLst.empty())
        {
            Screen->Space.erase(CurrentSpace);
            return;
        }

        if(CurrentSpace != -1 && PrevSpace == CurrentSpace && OldWindowListCount != -1)
        {
            if(WindowLst.size() > OldWindowListCount)
            {
                DEBUG("ShouldWindowNodeTreeUpdate() Add Window")
                for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
                {
                    if(GetNodeFromWindowID(Screen->Space[CurrentSpace], WindowLst[WindowIndex].WID) == NULL)
                    {
                        AddWindowToTree(WindowLst[WindowIndex].WID);
                        SetWindowFocus(&WindowLst[WindowIndex]);
                    }
                }
            }
            else if(WindowLst.size() < OldWindowListCount)
            {
                DEBUG("ShouldWindowNodeTreeUpdate() Remove Window")
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
}

void CreateWindowNodeTree()
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen)
        {
            std::map<int, tree_node*>::iterator It = Screen->Space.find(CurrentSpace);
            if(It == Screen->Space.end() && !WindowLst.empty())
            {
                DEBUG("CreateWindowNodeTree() Create Tree")
                std::vector<int> WindowIDs = GetAllWindowIDsOnDisplay(Screen->ID);
                Screen->Space[CurrentSpace] = CreateTreeFromWindowIDList(Screen, WindowIDs);
                ApplyNodeContainer(Screen->Space[CurrentSpace]);
                FocusWindowBelowCursor();
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

        if(MarkedWindowID == -1 && WindowID == FocusedWindow->WID)
            return;

        DEBUG("AddWindowToTree() Create pair of leafs")
        bool UseFocusedContainer = IsWindowOnActiveSpace(FocusedWindow);

        if(MarkedWindowID == -1 && UseFocusedContainer)
        {
            CurrentNode = GetNodeFromWindowID(RootNode, FocusedWindow->WID);
        }
        else if(MarkedWindowID == -1 && !UseFocusedContainer)
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
            CurrentNode = GetNodeFromWindowID(RootNode, MarkedWindowID);
            MarkedWindowID = -1;
        }

        int SplitMode = ExportTable.KwmSplitMode == -1 ? GetOptimalSplitMode(CurrentNode) : ExportTable.KwmSplitMode;
        CreateLeafNodePair(Screen, CurrentNode, CurrentNode->WindowID, WindowID, SplitMode);
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
                    {
                        Parent->LeftChild = AccessChild->LeftChild;
                        Parent->LeftChild->Parent = Parent;

                        Parent->RightChild = AccessChild->RightChild;
                        Parent->RightChild->Parent = Parent;

                        CreateNodeContainers(Screen, Parent);
                    }

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
                    else
                    {
                        FocusWindowBelowCursor();
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

void ReflectWindowNodeTreeVertically()
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen && Screen->Space[CurrentSpace])
        {
            DEBUG("ReflectWindowNodeTreeVertically()")
            tree_node *Node = Screen->Space[CurrentSpace];

            SwapNodeChildPositions(Node);
            CreateNodeContainers(Screen, Node);
            ApplyNodeContainer(Node);
        }
    }
}

void ToggleFocusedWindowParentContainer()
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen && Screen->Space[CurrentSpace])
        {
            tree_node *Node = GetNodeFromWindowID(Screen->Space[CurrentSpace], FocusedWindow->WID);
            if(Node && Node->Parent)
            {
                if(IsLeafNode(Node) && Node->Parent->WindowID == -1)
                {
                    DEBUG("ToggleFocusedWindowParentContainer() Set Parent Container")
                    Node->Parent->WindowID = Node->WindowID;
                    ResizeWindowToContainerSize(Node->Parent);
                }
                else
                {
                    DEBUG("ToggleFocusedWindowParentContainer() Restore Window Container")
                    Node->Parent->WindowID = -1;
                    ResizeWindowToContainerSize(Node);
                }
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

void MarkWindowContainer()
{
    if(FocusedWindow)
    {
        DEBUG("MarkWindowContainer() Marked " << FocusedWindow->Name)
        MarkedWindowID = FocusedWindow->WID;
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
    FocusedWindowCache = *Window;
    FocusedWindow = &FocusedWindowCache;

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

void MoveContainerSplitter(int SplitMode, int Offset)
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen && Screen->Space[CurrentSpace])
        {
            tree_node *Root = Screen->Space[CurrentSpace];
            if(IsLeafNode(Root) || Root->WindowID != -1)
                return;

            tree_node *LeftChild = Root->LeftChild;
            tree_node *RightChild = Root->RightChild;

            if(LeftChild->Container.Type == 1 && SplitMode == 1)
            {
                DEBUG("MoveContainerSplitter() Vertical")

                LeftChild->Container.Width += Offset;
                RightChild->Container.X += Offset;
                RightChild->Container.Width -= Offset;
            }
            else if(LeftChild->Container.Type == 3 && SplitMode == 2)
            {
                DEBUG("MoveContainerSplitter() Horizontal")

                LeftChild->Container.Height += Offset;
                RightChild->Container.Y += Offset;
                RightChild->Container.Height -= Offset;
            }
            else
            {
                DEBUG("MoveContainerSplitter() Invalid")
                return;
            }

            ResizeNodeContainer(Screen, LeftChild);
            ResizeNodeContainer(Screen, RightChild);

            ApplyNodeContainer(Root);
        }
    }
}

void ResizeWindowToContainerSize(tree_node *Node)
{
    window_info *Window = GetWindowByID(Node->WindowID);

    if(Window)
    {
        int Retries = 0;
        bool Result = false;

        AXUIElementRef WindowRef;
        while(!Result && Retries++ < 3)
            Result = GetWindowRef(Window, &WindowRef);

        if(Result)
        {
            SetWindowDimensions(WindowRef, Window,
                        Node->Container.X, Node->Container.Y, 
                        Node->Container.Width, Node->Container.Height);

            CFRelease(WindowRef);
        }
        else
        {
            DEBUG("GetWindowRef() Failed for window " << Window->Name)
        }
    }
}

void ResizeWindowToContainerSize()
{
    if(FocusedWindow)
    {
        screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
        if(Screen && Screen->Space[CurrentSpace])
        {
            tree_node *Node = GetNodeFromWindowID(Screen->Space[CurrentSpace], FocusedWindow->WID);
            if(Node)
                ResizeWindowToContainerSize(Node);
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
    else
    {
        DEBUG("GetWindowRef() Could not get AppWindowLst")
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
