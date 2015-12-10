#include "kwm.h"

static CGWindowListOption OsxWindowListOption = kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements;

extern std::vector<window_info> WindowLst;
extern std::vector<int> FloatingWindowLst;
extern export_table ExportTable;

extern ProcessSerialNumber FocusedPSN;
extern AXUIElementRef FocusedWindowRef;
extern window_info *FocusedWindow;

extern int CurrentSpace;
extern int PrevSpace;
extern int MarkedWindowID;
extern screen_info *Screen;

int OldScreenID = -1;
window_info FocusedWindowCache;

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
           Window->Layer == Match->Layer)
        {
            Result = true;
        }
    }

    return Result;
}

bool IsWindowAnElementOfAWindow(window_info *Window)
{
    bool Result = false;

    // If the window is not a webbrowser,
    // a window that is not an element of
    // an existing window should have a title.
    if(Window->Owner != "Firefox" &&
       Window->Owner != "Google Chrome" &&
       Window->Owner != "Safari" &&
       Window->Name == "")
    {
        Result = true;
    }

    return Result || IsWindowPartOfWebBrowser(Window);
}

bool IsWindowPartOfWebBrowser(window_info *Window)
{
    bool Result = false;

    // Any Firefox window that has a width lower than 335 is not a browser window
    if(Window->Owner == "Firefox" &&
       Window->Name == "" &&
       Window->Width < 335) 
            Result = true;

    // Any Safari window that has a width lower than 500 is not a 
    // browser window, but items such as the searchbar and so on
    else if((Window->Owner == "Safari" &&
            Window->Name == "") &&
            (Window->Width < 500 ||
             Window->Height < 232))
                 Result = true;

    // Any Google Chrome  window that has a width lower than 400
    // or height lower than 272 is not a browser window
    else if((Window->Owner == "Google Chrome" &&
            Window->Name == "") &&
            (Window->Width < 400 || 
            Window->Height < 272))
                 Result = true;

    return Result;
}

bool FilterWindowList(screen_info *Screen)
{
    bool Result = true;
    std::vector<window_info> FilteredWindowLst;

    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        // Mission-Control mode is on and so we do not try to tile windows
        if(WindowLst[WindowIndex].Owner == "Dock" &&
           WindowLst[WindowIndex].Name == "")
               Result = false;

        /*
        if(WindowLst[WindowIndex].Owner == "Safari" &&
           WindowLst[WindowIndex].Name == "")
        {
             DEBUG("WINDOW INFO: " << WindowLst[WindowIndex].Owner << " " <<
                                      WindowLst[WindowIndex].Name << " " <<
                                      WindowLst[WindowIndex].Width << " " <<
                                      WindowLst[WindowIndex].Height)
        }
        */

        if(WindowLst[WindowIndex].Layer == 0 &&
           Screen == GetDisplayOfWindow(&WindowLst[WindowIndex]))
        {
            if(IsWindowAnElementOfAWindow(&WindowLst[WindowIndex]))
                FloatingWindowLst.push_back(WindowLst[WindowIndex].WID);

            FilteredWindowLst.push_back(WindowLst[WindowIndex]);
        }
    }

    WindowLst = FilteredWindowLst;
    return Result;
}

bool IsWindowFloating(int WindowID, int *Index)
{
    bool Result = false;

    for(int WindowIndex = 0; WindowIndex < FloatingWindowLst.size(); ++WindowIndex)
    {
        if(WindowID == FloatingWindowLst[WindowIndex])
        {
            DEBUG("IsWindowFloating(): floating " << WindowID)
            Result = true;

            if(Index)
                *Index = WindowIndex;

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

bool DoesSpaceExistInMapOfScreen(screen_info *Screen)
{
    std::map<int, space_info>::iterator It = Screen->Space.find(CurrentSpace);
    if(It == Screen->Space.end())
        return false;
    else
        return It->second.RootNode != NULL;
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

bool IsSpaceSystemOrFullscreen()
{
    bool Result = CGSSpaceGetType(CGSDefaultConnection, CurrentSpace) != CGSSpaceTypeUser;
    if(Result)
    {
        DEBUG("IsSpaceSystemOrFullscreen() Space is not user created")
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
                break;
            }
        }
    }
}

void UpdateWindowTree()
{
    OldScreenID = Screen->ID;
    Screen = GetDisplayOfMousePointer();
    if(Screen)
    {
        UpdateActiveWindowList(Screen);
        if(!IsSpaceTransitionInProgress() && !IsSpaceSystemOrFullscreen() && FilterWindowList(Screen))
        {
            std::map<int, space_info>::iterator It = Screen->Space.find(CurrentSpace);
            if(It == Screen->Space.end() && !WindowLst.empty())
                CreateWindowNodeTree(Screen);
            else if(It != Screen->Space.end() && !WindowLst.empty())
                ShouldWindowNodeTreeUpdate(Screen);
            else if(It != Screen->Space.end() && WindowLst.empty())
                Screen->Space.erase(CurrentSpace);
        }
    }
}

void UpdateActiveWindowList(screen_info *Screen)
{
    Screen->OldWindowListCount = WindowLst.size();
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

        if(OldScreenID != Screen->ID)
        {
            do
            {
                CurrentSpace = CGSGetActiveSpace(CGSDefaultConnection);
                usleep(200000);
            } while(PrevSpace == CurrentSpace);
            DEBUG("UpdateActiveWindowList() Active Display Changed")
            Screen->ActiveSpace = CurrentSpace;
        }

        if(PrevSpace != CurrentSpace)
        {
            DEBUG("UpdateActiveWindowList() Space transition ended")
            FocusWindowBelowCursor();
        }

        if(Screen->ForceContainerUpdate)
        {
            ApplyNodeContainer(Screen->Space[Screen->ActiveSpace].RootNode);
            Screen->ForceContainerUpdate = false;
        }
    }
}

void CreateWindowNodeTree(screen_info *Screen)
{
    DEBUG("CreateWindowNodeTree() Create Tree")
    std::vector<int> WindowIDs = GetAllWindowIDsOnDisplay(Screen->ID);

    space_info SpaceInfo;
    SpaceInfo.RootNode = CreateTreeFromWindowIDList(Screen, WindowIDs);

    SpaceInfo.PaddingTop = Screen->PaddingTop;
    SpaceInfo.PaddingBottom = Screen->PaddingBottom;
    SpaceInfo.PaddingLeft = Screen->PaddingLeft;
    SpaceInfo.PaddingRight = Screen->PaddingRight;

    SpaceInfo.VerticalGap = Screen->VerticalGap;
    SpaceInfo.HorizontalGap = Screen->HorizontalGap;

    Screen->Space[CurrentSpace] = SpaceInfo;
    ApplyNodeContainer(Screen->Space[CurrentSpace].RootNode);
    FocusWindowBelowCursor();
}

void ShouldWindowNodeTreeUpdate(screen_info *Screen)
{
    if(CurrentSpace != -1 && PrevSpace == CurrentSpace && Screen->OldWindowListCount != -1)
    {
        if(WindowLst.size() > Screen->OldWindowListCount)
        {
            DEBUG("ShouldWindowNodeTreeUpdate() Add Window")
            for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
            {
                if(GetNodeFromWindowID(Screen->Space[CurrentSpace].RootNode, WindowLst[WindowIndex].WID) == NULL)
                {
                    if(!IsWindowFloating(WindowLst[WindowIndex].WID, NULL))
                    {
                        AddWindowToTree(Screen, WindowLst[WindowIndex].WID);
                        SetWindowFocus(&WindowLst[WindowIndex]);
                    }
                }
            }
        }
        else if(WindowLst.size() < Screen->OldWindowListCount)
        {
            DEBUG("ShouldWindowNodeTreeUpdate() Remove Window")
            std::vector<int> WindowIDsInTree;

            tree_node *CurrentNode = Screen->Space[CurrentSpace].RootNode;
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
                    RemoveWindowFromTree(Screen, WindowIDsInTree[IDIndex], false);
            }
        }
    }
}

void AddWindowToTree(screen_info *Screen, int WindowID)
{
    if(Screen && DoesSpaceExistInMapOfScreen(Screen))
    {
        tree_node *RootNode = Screen->Space[CurrentSpace].RootNode;
        tree_node *CurrentNode = RootNode;

        DEBUG("AddWindowToTree() Create pair of leafs")
        bool UseFocusedContainer = FocusedWindow &&
                                   IsWindowOnActiveSpace(FocusedWindow) &&
                                   FocusedWindow->WID != WindowID;

        bool DoNotUseMarkedContainer = IsWindowFloating(MarkedWindowID, NULL) ||
                                       (MarkedWindowID == WindowID);

        if(MarkedWindowID == -1 && UseFocusedContainer)
        {
            CurrentNode = GetNodeFromWindowID(RootNode, FocusedWindow->WID);
        }
        else if(DoNotUseMarkedContainer || (MarkedWindowID == -1 && !UseFocusedContainer))
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

        if(CurrentNode)
        {
            int SplitMode = ExportTable.KwmSplitMode == -1 ? GetOptimalSplitMode(CurrentNode) : ExportTable.KwmSplitMode;
            CreateLeafNodePair(Screen, CurrentNode, CurrentNode->WindowID, WindowID, SplitMode);
            ApplyNodeContainer(CurrentNode);
        }
    }
}

void AddWindowToTree()
{
    if(Screen)
        AddWindowToTree(Screen, FocusedWindow->WID);
}

void RemoveWindowFromTree(screen_info *Screen, int WindowID, bool Center)
{
    if(Screen && DoesSpaceExistInMapOfScreen(Screen))
    {
        tree_node *WindowNode = GetNodeFromWindowID(Screen->Space[CurrentSpace].RootNode, WindowID);

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
                        int NewX = Screen->X + Screen->Width / 4;
                        int NewY = Screen->Y + Screen->Height / 4;
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
    if(Screen)
        RemoveWindowFromTree(Screen, FocusedWindow->WID, true);
}

void AddWindowToTreeOfUnfocusedMonitor(screen_info *Screen)
{
    if(Screen->Space[Screen->ActiveSpace].RootNode)
    {
        tree_node *RootNode = Screen->Space[Screen->ActiveSpace].RootNode;
        tree_node *CurrentNode = RootNode;

        DEBUG("AddWindowToTreeOfUnfocusedMonitor() Create pair of leafs")

            while(!IsLeafNode(CurrentNode))
            {
                if(!IsLeafNode(CurrentNode->LeftChild) && IsLeafNode(CurrentNode->RightChild))
                    CurrentNode = CurrentNode->RightChild;
                else
                    CurrentNode = CurrentNode->LeftChild;
            }

        int SplitMode = ExportTable.KwmSplitMode == -1 ? GetOptimalSplitMode(CurrentNode) : ExportTable.KwmSplitMode;
        CreateLeafNodePair(Screen, CurrentNode, CurrentNode->WindowID, FocusedWindow->WID, SplitMode);
        ResizeWindowToContainerSize(CurrentNode->RightChild);
        Screen->ForceContainerUpdate = true;
    }
}

void ReflectWindowNodeTreeVertically()
{
    if(Screen && DoesSpaceExistInMapOfScreen(Screen))
    {
        DEBUG("ReflectWindowNodeTreeVertically()")

        SwapNodeChildPositions(Screen->Space[CurrentSpace].RootNode);
        CreateNodeContainers(Screen, Screen->Space[CurrentSpace].RootNode);
        ApplyNodeContainer(Screen->Space[CurrentSpace].RootNode);
    }
}

void ToggleFocusedWindowFloating()
{
    if(FocusedWindow && IsWindowOnActiveSpace(FocusedWindow))
    {
        int WindowIndex;
        if(IsWindowFloating(FocusedWindow->WID, &WindowIndex))
        {
            FloatingWindowLst.erase(FloatingWindowLst.begin() + WindowIndex);
            ExportTable.KwmFocusMode = FocusModeAutoraise;
            AddWindowToTree();
        }
        else
        {
            FloatingWindowLst.push_back(FocusedWindow->WID);
            ExportTable.KwmFocusMode = FocusModeAutofocus;
            RemoveWindowFromTree();
        }
    }
}

void ToggleFocusedWindowParentContainer()
{
    if(FocusedWindow)
    {
        if(Screen && DoesSpaceExistInMapOfScreen(Screen))
        {
            tree_node *Node = GetNodeFromWindowID(Screen->Space[CurrentSpace].RootNode, FocusedWindow->WID);
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
        if(Screen && DoesSpaceExistInMapOfScreen(Screen) && !IsLeafNode(Screen->Space[CurrentSpace].RootNode))
        {
            tree_node *Node;
            if(Screen->Space[CurrentSpace].RootNode->WindowID == -1)
            {
                Node = GetNodeFromWindowID(Screen->Space[CurrentSpace].RootNode, FocusedWindow->WID);
                if(Node)
                {
                    DEBUG("ToggleFocusedWindowFullscreen() Set fullscreen")
                    Screen->Space[CurrentSpace].RootNode->WindowID = Node->WindowID;
                    ResizeWindowToContainerSize(Screen->Space[CurrentSpace].RootNode);
                }
            }
            else
            {
                DEBUG("ToggleFocusedWindowFullscreen() Restore old size")
                Screen->Space[CurrentSpace].RootNode->WindowID = -1;

                Node = GetNodeFromWindowID(Screen->Space[CurrentSpace].RootNode, FocusedWindow->WID);
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
        if(Screen && DoesSpaceExistInMapOfScreen(Screen))
        {
            tree_node *FocusedWindowNode = GetNodeFromWindowID(Screen->Space[CurrentSpace].RootNode, FocusedWindow->WID);
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
        if(Screen && DoesSpaceExistInMapOfScreen(Screen))
        {
            tree_node *FocusedWindowNode = GetNodeFromWindowID(Screen->Space[CurrentSpace].RootNode, FocusedWindow->WID);
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

    AXUIElementSetAttributeValue(FocusedWindowRef, kAXMainAttribute, kCFBooleanTrue);
    AXUIElementSetAttributeValue(FocusedWindowRef, kAXFocusedAttribute, kCFBooleanTrue);
    AXUIElementPerformAction(FocusedWindowRef, kAXRaiseAction);

    if(ExportTable.KwmFocusMode == FocusModeAutoraise)
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
    if(Screen && DoesSpaceExistInMapOfScreen(Screen))
    {
        tree_node *Root = Screen->Space[CurrentSpace].RootNode;
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
        if(Screen && DoesSpaceExistInMapOfScreen(Screen))
        {
            tree_node *Node = GetNodeFromWindowID(Screen->Space[CurrentSpace].RootNode, FocusedWindow->WID);
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
                    int AppWindowRefWID = -1;
                    _AXUIElementGetWindow(AppWindowRef, &AppWindowRefWID);
                    if(AppWindowRefWID == Window->WID)
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
