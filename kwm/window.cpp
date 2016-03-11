#include "window.h"
#include "display.h"
#include "space.h"
#include "tree.h"
#include "notifications.h"
#include "border.h"

#include <cmath>

extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_toggles KWMToggles;
extern kwm_mode KWMMode;
extern kwm_tiling KWMTiling;
extern kwm_cache KWMCache;
extern kwm_path KWMPath;
extern kwm_border MarkedBorder;
extern kwm_border FocusedBorder;

bool WindowsAreEqual(window_info *Window, window_info *Match)
{
    if(Window && Match)
    {
        if(Window->PID == Match->PID &&
           Window->WID == Match->WID &&
           Window->Layer == Match->Layer)
            return true;
    }

    return false;
}

void AllowRoleForApplication(std::string Application, std::string Role)
{
    std::map<std::string, std::vector<CFTypeRef> >::iterator It = KWMTiling.AllowedWindowRoles.find(Application);
    if(It == KWMTiling.AllowedWindowRoles.end())
        KWMTiling.AllowedWindowRoles[Application] = std::vector<CFTypeRef>();

    CFStringRef RoleRef = CFStringCreateWithCString(NULL, Role.c_str(), kCFStringEncodingMacRoman);
    KWMTiling.AllowedWindowRoles[Application].push_back(RoleRef);
}

bool IsAppSpecificWindowRole(window_info *Window, CFTypeRef Role, CFTypeRef SubRole)
{
    std::map<std::string, std::vector<CFTypeRef> >::iterator It = KWMTiling.AllowedWindowRoles.find(Window->Owner);
    if(It != KWMTiling.AllowedWindowRoles.end())
    {
        std::vector<CFTypeRef> &WindowRoles = It->second;
        for(std::size_t RoleIndex = 0; RoleIndex < WindowRoles.size(); ++RoleIndex)
        {
            if(CFEqual(Role, WindowRoles[RoleIndex]) || CFEqual(SubRole, WindowRoles[RoleIndex]))
                return true;
        }
    }

    return false;
}

std::vector<window_info> FilterWindowListAllDisplays()
{
    std::vector<window_info> FilteredWindowLst;
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.FocusLst.size(); ++WindowIndex)
    {
        if(ShouldWindowGainFocus(&KWMTiling.FocusLst[WindowIndex]))
        {
            CFTypeRef Role, SubRole;
            if(GetWindowRole(&KWMTiling.FocusLst[WindowIndex], &Role, &SubRole))
            {
                if((CFEqual(Role, kAXWindowRole) && CFEqual(SubRole, kAXStandardWindowSubrole)) ||
                   IsAppSpecificWindowRole(&KWMTiling.FocusLst[WindowIndex], Role, SubRole))
                        FilteredWindowLst.push_back(KWMTiling.FocusLst[WindowIndex]);
            }
        }
    }

    return FilteredWindowLst;
}

bool FilterWindowList(screen_info *Screen)
{
    std::vector<window_info> FilteredWindowLst;
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.WindowLst.size(); ++WindowIndex)
    {
        window_info *Window = &KWMTiling.WindowLst[WindowIndex];

        /* Note(koekeishiya):
         * Mission-Control mode is on and so we do not try to tile windows */
        if(Window->Owner == "Dock" && Window->Name == "")
        {
                ClearFocusedWindow();
                ClearMarkedWindow();
                return false;
        }

        if(Window->Layer == 0)
        {
            if(IsApplicationCapturedByScreen(Window))
            {
                int CapturedID = KWMTiling.CapturedAppLst[Window->Owner];
                screen_info *Screen = GetDisplayFromScreenID(CapturedID);
                if(Screen && Screen != GetDisplayOfWindow(Window))
                {
                    MoveWindowToDisplay(Window, CapturedID, false, true);
                    return false;
                }
            }

            screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
            if(Screen != ScreenOfWindow)
            {
                space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
                if(GetNodeFromWindowID(SpaceOfWindow->RootNode, Window->WID, SpaceOfWindow->Mode))
                    continue;
            }

            CFTypeRef Role, SubRole;
            if(GetWindowRole(Window, &Role, &SubRole))
            {
                if((CFEqual(Role, kAXWindowRole) && CFEqual(SubRole, kAXStandardWindowSubrole)) ||
                   IsAppSpecificWindowRole(Window, Role, SubRole))
                    FilteredWindowLst.push_back(KWMTiling.WindowLst[WindowIndex]);
            }
        }
    }

    KWMTiling.WindowLst = FilteredWindowLst;
    return true;
}

bool IsApplicationCapturedByScreen(window_info *Window)
{
    return KWMTiling.CapturedAppLst.find(Window->Owner) != KWMTiling.CapturedAppLst.end();
}

bool IsApplicationFloating(window_info *Window)
{
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.FloatingAppLst.size(); ++WindowIndex)
    {
        if(Window->Owner == KWMTiling.FloatingAppLst[WindowIndex])
            return true;
    }

    return false;
}

bool IsFocusedWindowFloating()
{
    return KWMFocus.Window && (IsWindowFloating(KWMFocus.Window->WID, NULL) || IsApplicationFloating(KWMFocus.Window));
}

bool IsWindowFloating(int WindowID, int *Index)
{
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.FloatingWindowLst.size(); ++WindowIndex)
    {
        if(WindowID == KWMTiling.FloatingWindowLst[WindowIndex])
        {
            if(Index)
                *Index = WindowIndex;

            return true;
        }
    }

    return false;
}

bool IsAnyWindowBelowCursor()
{
    CGPoint Cursor = GetCursorPos();
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.FocusLst.size(); ++WindowIndex)
    {
        window_info *Window = &KWMTiling.FocusLst[WindowIndex];
        if(Cursor.x >= Window->X &&
           Cursor.x <= Window->X + Window->Width &&
           Cursor.y >= Window->Y &&
           Cursor.y <= Window->Y + Window->Height)
            return true;
    }

    return false;
}

bool IsWindowBelowCursor(window_info *Window)
{
    Assert(Window, "IsWindowBelowCursor()")

    CGPoint Cursor = GetCursorPos();
    if(Cursor.x >= Window->X &&
       Cursor.x <= Window->X + Window->Width &&
       Cursor.y >= Window->Y &&
       Cursor.y <= Window->Y + Window->Height)
        return true;

    return false;
}

bool IsWindowOnActiveSpace(int WindowID)
{
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.WindowLst.size(); ++WindowIndex)
    {
        if(WindowID == KWMTiling.WindowLst[WindowIndex].WID)
        {
            DEBUG("IsWindowOnActiveSpace() window found")
            return true;
        }
    }

    DEBUG("IsWindowOnActiveSpace() window was not found")
    return false;
}

int GetFocusedWindowID()
{
    return (KWMFocus.Window && KWMFocus.Window->Layer == 0) ? KWMFocus.Window->WID : -1;
}

void ClearFocusedWindow()
{
    ClearBorder(&FocusedBorder);
    KWMFocus.Window = NULL;
    KWMFocus.Cache = KWMFocus.NULLWindowInfo;
}

bool FocusWindowOfOSX()
{
    if(IsSpaceTransitionInProgress() ||
       !IsActiveSpaceManaged())
        return false;

    static AXUIElementRef SystemWideElement = AXUIElementCreateSystemWide();

    AXUIElementRef App;
    AXUIElementCopyAttributeValue(SystemWideElement, kAXFocusedApplicationAttribute, (CFTypeRef*)&App);
    if(App)
    {
        AXUIElementRef WindowRef;
        AXError Error = AXUIElementCopyAttributeValue(App, kAXFocusedWindowAttribute, (CFTypeRef*)&WindowRef);
        CFRelease(App);

        if (Error == kAXErrorSuccess)
        {
            SetWindowRefFocus(WindowRef);
            CFRelease(WindowRef);
            return true;
        }
    }

    ClearFocusedWindow();
    return false;
}

bool ShouldWindowGainFocus(window_info *Window)
{
    return Window->Layer == CGWindowLevelForKey(kCGNormalWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGFloatingWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGTornOffMenuWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGDockWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGMainMenuWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGMaximumWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGModalPanelWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGUtilityWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGOverlayWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGHelpWindowLevelKey) ||
           Window->Layer == CGWindowLevelForKey(kCGPopUpMenuWindowLevelKey) ||
           Window->Layer == 1490; // Note(koekeishiya): Unknown WindowLevelKey constant
}

void FocusFirstLeafNode()
{
    if(!DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    SetWindowFocusByNode(GetFirstLeafNode(Space->RootNode));
    MoveCursorToCenterOfFocusedWindow();
}

void FocusLastLeafNode()
{
    if(!DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    SetWindowFocusByNode(GetLastLeafNode(Space->RootNode));
    MoveCursorToCenterOfFocusedWindow();
}

void FocusWindowBelowCursor()
{
    if(IsSpaceTransitionInProgress() ||
       !IsActiveSpaceManaged())
           return;

    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.FocusLst.size(); ++WindowIndex)
    {
        if(KWMTiling.FocusLst[WindowIndex].Owner == "kwm-overlay")
            continue;

        if(KWMTiling.FocusLst[WindowIndex].Owner == "Dock" &&
           KWMTiling.FocusLst[WindowIndex].X == 0 &&
           KWMTiling.FocusLst[WindowIndex].Y == 0)
            continue;

        if(IsWindowBelowCursor(&KWMTiling.FocusLst[WindowIndex]) &&
           ShouldWindowGainFocus(&KWMTiling.FocusLst[WindowIndex]))
        {
            if(WindowsAreEqual(KWMFocus.Window, &KWMTiling.FocusLst[WindowIndex]))
                KWMFocus.Cache = KWMTiling.FocusLst[WindowIndex];
            else
                SetWindowFocus(&KWMTiling.FocusLst[WindowIndex]);

            return;
        }
    }
}

void UpdateWindowTree()
{
    if(IsSpaceTransitionInProgress() ||
       !IsActiveSpaceManaged())
        return;

    UpdateActiveWindowList(KWMScreen.Current);
    if(KWMToggles.EnableTilingMode &&
       FilterWindowList(KWMScreen.Current))
    {
        std::vector<window_info*> WindowsOnDisplay = GetAllWindowsOnDisplay(KWMScreen.Current->ID);
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);

        if(!IsActiveSpaceFloating())
        {
            if(!Space->Initialized)
                CreateWindowNodeTree(KWMScreen.Current, &WindowsOnDisplay);
            else if(Space->Initialized &&
                    !WindowsOnDisplay.empty() &&
                    !Space->RootNode)
                CreateWindowNodeTree(KWMScreen.Current, &WindowsOnDisplay);
            else if(Space->Initialized &&
                    !WindowsOnDisplay.empty() &&
                    Space->RootNode)
                ShouldWindowNodeTreeUpdate(KWMScreen.Current);
            else if(Space->Initialized &&
                    WindowsOnDisplay.empty() &&
                    Space->RootNode)
            {
                DestroyNodeTree(Space->RootNode, Space->Mode);
                Space->RootNode = NULL;
                ClearFocusedWindow();
            }
        }
    }
}

void UpdateActiveWindowList(screen_info *Screen)
{
    static CGWindowListOption OsxWindowListOption = kCGWindowListOptionOnScreenOnly |
                                                    kCGWindowListExcludeDesktopElements;

    Screen->OldWindowListCount = KWMTiling.WindowLst.size();
    KWMTiling.WindowLst.clear();

    CFArrayRef OsxWindowLst = CGWindowListCopyWindowInfo(OsxWindowListOption, kCGNullWindowID);
    if(!OsxWindowLst)
        return;

    CFIndex OsxWindowCount = CFArrayGetCount(OsxWindowLst);
    for(CFIndex WindowIndex = 0; WindowIndex < OsxWindowCount; ++WindowIndex)
    {
        CFDictionaryRef Elem = (CFDictionaryRef)CFArrayGetValueAtIndex(OsxWindowLst, WindowIndex);
        KWMTiling.WindowLst.push_back(window_info());
        CFDictionaryApplyFunction(Elem, GetWindowInfo, NULL);
    }
    CFRelease(OsxWindowLst);
    KWMTiling.FocusLst = KWMTiling.WindowLst;
}

void CreateWindowNodeTree(screen_info *Screen, std::vector<window_info*> *Windows)
{
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    if(!Space->Initialized)
    {
        Assert(Space, "CreateWindowNodeTree()")
        DEBUG("CreateWindowNodeTree() Create Space " << Screen->ActiveSpace)

        Space->Mode = GetSpaceModeOfDisplay(Screen->ID);
        if(Space->Mode == SpaceModeDefault)
            Space->Mode = KWMMode.Space;

        Space->Initialized = true;
        Space->Offset = Screen->Offset;
        Space->RootNode = CreateTreeFromWindowIDList(Screen, Windows);
    }
    else if(Space->Initialized)
    {
        Space->RootNode = CreateTreeFromWindowIDList(Screen, Windows);
        if(Space->RootNode)
        {
            if(Space->Mode == SpaceModeBSP)
            {
                SetRootNodeContainer(Screen, Space->RootNode);
                CreateNodeContainers(Screen, Space->RootNode, true);
            }
            else if(Space->Mode == SpaceModeMonocle)
            {
                tree_node *CurrentNode = Space->RootNode;
                while(CurrentNode)
                {
                    SetRootNodeContainer(Screen, CurrentNode);
                    CurrentNode = CurrentNode->RightChild;
                }
            }
        }
    }

    if(Space->RootNode)
    {
        ApplyNodeContainer(Space->RootNode, Space->Mode);
        FocusWindowBelowCursor();
    }
}

void ShouldWindowNodeTreeUpdate(screen_info *Screen)
{
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    if(Space->Mode == SpaceModeBSP)
        ShouldBSPTreeUpdate(Screen, Space);
    else if(Space->Mode == SpaceModeMonocle)
        ShouldMonocleTreeUpdate(Screen, Space);
}

void ShouldBSPTreeUpdate(screen_info *Screen, space_info *Space)
{
    if(KWMTiling.WindowLst.size() > Screen->OldWindowListCount)
    {
        window_info *FocusWindow = NULL;
        for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.WindowLst.size(); ++WindowIndex)
        {
            if(!GetNodeFromWindowID(Space->RootNode, KWMTiling.WindowLst[WindowIndex].WID, Space->Mode))
            {
                if(!IsApplicationFloating(&KWMTiling.WindowLst[WindowIndex]) &&
                   !IsWindowFloating(KWMTiling.WindowLst[WindowIndex].WID, NULL))
                {
                    DEBUG("ShouldBSPTreeUpdate() Add Window")
                    tree_node *Insert = GetFirstPseudoLeafNode(Space->RootNode);
                    if(Insert && (Insert->WindowID = KWMTiling.WindowLst[WindowIndex].WID))
                        ApplyNodeContainer(Insert, SpaceModeBSP);
                    else
                        AddWindowToBSPTree(Screen, KWMTiling.WindowLst[WindowIndex].WID);

                    FocusWindow = &KWMTiling.WindowLst[WindowIndex];
                }
            }
        }

        if(FocusWindow)
        {
            SetWindowFocus(FocusWindow);
            MoveCursorToCenterOfFocusedWindow();
        }
    }
    else if(KWMTiling.WindowLst.size() < Screen->OldWindowListCount)
    {
        std::vector<int> WindowIDsInTree;

        tree_node *CurrentNode = GetFirstLeafNode(Space->RootNode);
        while(CurrentNode)
        {
            if(CurrentNode->WindowID != -1)
                WindowIDsInTree.push_back(CurrentNode->WindowID);

            CurrentNode = GetNearestNodeToTheRight(CurrentNode, SpaceModeBSP);
        }

        for(std::size_t IDIndex = 0; IDIndex < WindowIDsInTree.size(); ++IDIndex)
        {
            bool Found = false;
            for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.WindowLst.size(); ++WindowIndex)
            {
                if(KWMTiling.WindowLst[WindowIndex].WID == WindowIDsInTree[IDIndex])
                {
                    Found = true;
                    break;
                }
            }

            if(!Found)
            {
                DEBUG("ShouldBSPTreeUpdate() Remove Window " << WindowIDsInTree[IDIndex])
                RemoveWindowFromBSPTree(Screen, WindowIDsInTree[IDIndex], false, true);
            }
        }

        if(!KWMFocus.Window)
            FocusWindowOfOSX();
    }
}

void AddWindowToBSPTree(screen_info *Screen, int WindowID)
{
    if(!DoesSpaceExistInMapOfScreen(Screen))
        return;

    space_info *Space = GetActiveSpaceOfScreen(Screen);
    tree_node *RootNode = Space->RootNode;
    tree_node *CurrentNode = NULL;

    DEBUG("AddWindowToBSPTree() Create pair of leafs")
    bool UseFocusedContainer = KWMFocus.Window &&
                               IsWindowOnActiveSpace(KWMFocus.Window->WID) &&
                               KWMFocus.Window->WID != WindowID;

    bool DoNotUseMarkedContainer = IsWindowFloating(KWMScreen.MarkedWindow, NULL) ||
                                   (KWMScreen.MarkedWindow == WindowID);

    if(KWMScreen.MarkedWindow == -1 && UseFocusedContainer)
    {
        CurrentNode = GetNodeFromWindowID(RootNode, KWMFocus.Window->WID, Space->Mode);
    }
    else if(DoNotUseMarkedContainer || (KWMScreen.MarkedWindow == -1 && !UseFocusedContainer))
    {
        CurrentNode = GetFirstLeafNode(RootNode);
    }
    else
    {
        CurrentNode = GetNodeFromWindowID(RootNode, KWMScreen.MarkedWindow, Space->Mode);
        ClearMarkedWindow();
    }

    if(CurrentNode)
    {
        split_type SplitMode = KWMScreen.SplitMode == SPLIT_OPTIMAL ? GetOptimalSplitMode(CurrentNode) : KWMScreen.SplitMode;
        CreateLeafNodePair(Screen, CurrentNode, CurrentNode->WindowID, WindowID, SplitMode);
        ApplyNodeContainer(CurrentNode, Space->Mode);
    }
}

void AddWindowToBSPTree()
{
    if(!KWMScreen.Current)
        return;

    AddWindowToBSPTree(KWMScreen.Current, KWMFocus.Window->WID);
}

void RemoveWindowFromBSPTree(screen_info *Screen, int WindowID, bool Center, bool UpdateFocus)
{
    if(!DoesSpaceExistInMapOfScreen(Screen))
        return;

    space_info *Space = GetActiveSpaceOfScreen(Screen);
    tree_node *WindowNode = GetNodeFromWindowID(Space->RootNode, WindowID, Space->Mode);
    if(!WindowNode)
        return;

    tree_node *Parent = WindowNode->Parent;
    if(Parent && Parent->LeftChild && Parent->RightChild)
    {
        tree_node *AccessChild = IsRightChild(WindowNode) ? Parent->LeftChild : Parent->RightChild;
        tree_node *NewFocusNode = NULL;
        Parent->LeftChild = NULL;
        Parent->RightChild = NULL;

        DEBUG("RemoveWindowFromBSPTree() Parent && LeftChild && RightChild")
        Parent->WindowID = AccessChild->WindowID;
        if(AccessChild->LeftChild && AccessChild->RightChild)
        {
            Parent->LeftChild = AccessChild->LeftChild;
            Parent->LeftChild->Parent = Parent;

            Parent->RightChild = AccessChild->RightChild;
            Parent->RightChild->Parent = Parent;

            CreateNodeContainers(Screen, Parent, true);
            NewFocusNode = IsLeafNode(Parent->LeftChild) ? Parent->LeftChild : Parent->RightChild;
            while(!IsLeafNode(NewFocusNode))
                NewFocusNode = IsLeafNode(Parent->LeftChild) ? Parent->LeftChild : Parent->RightChild;
        }

        ApplyNodeContainer(Parent, Space->Mode);
        if(Center)
        {
            window_info *WindowInfo = GetWindowByID(WindowNode->WindowID);
            if(WindowInfo)
                CenterWindow(Screen, WindowInfo);
        }
        else
        {
            if(!NewFocusNode)
                NewFocusNode = Parent;

            if(UpdateFocus)
                SetWindowFocusByNode(NewFocusNode);
        }

        free(AccessChild);
        free(WindowNode);
    }
    else if(!Parent)
    {
        DEBUG("RemoveWindowFromBSPTree() !Parent")

        if(Center)
        {
            window_info *WindowInfo = GetWindowByID(WindowNode->WindowID);
            if(WindowInfo)
                CenterWindow(Screen, WindowInfo);
        }

        free(Space->RootNode);
        Space->RootNode = NULL;
    }
}

void RemoveWindowFromBSPTree()
{
    if(!KWMScreen.Current)
        return;

    RemoveWindowFromBSPTree(KWMScreen.Current, KWMFocus.Window->WID, true, true);
}

void ShouldMonocleTreeUpdate(screen_info *Screen, space_info *Space)
{
    if(KWMTiling.WindowLst.size() > Screen->OldWindowListCount)
    {
        DEBUG("ShouldMonocleTreeUpdate() Add Window")
        for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.WindowLst.size(); ++WindowIndex)
        {
            if(!GetNodeFromWindowID(Space->RootNode, KWMTiling.WindowLst[WindowIndex].WID, Space->Mode))
            {
                if(!IsApplicationFloating(&KWMTiling.WindowLst[WindowIndex]))
                {
                    AddWindowToMonocleTree(Screen, KWMTiling.WindowLst[WindowIndex].WID);
                    SetWindowFocus(&KWMTiling.WindowLst[WindowIndex]);
                    MoveCursorToCenterOfFocusedWindow();
                }
            }
        }
    }
    else if(KWMTiling.WindowLst.size() < Screen->OldWindowListCount)
    {
        DEBUG("ShouldMonocleTreeUpdate() Remove Window")
        std::vector<int> WindowIDsInTree;

        tree_node *CurrentNode = Space->RootNode;
        while(CurrentNode)
        {
            WindowIDsInTree.push_back(CurrentNode->WindowID);
            CurrentNode = GetNearestNodeToTheRight(CurrentNode, SpaceModeMonocle);
        }

        if(WindowIDsInTree.size() >= 2)
        {
            for(std::size_t IDIndex = 0; IDIndex < WindowIDsInTree.size(); ++IDIndex)
            {
                bool Found = false;
                for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.WindowLst.size(); ++WindowIndex)
                {
                    if(KWMTiling.WindowLst[WindowIndex].WID == WindowIDsInTree[IDIndex])
                    {
                        Found = true;
                        break;
                    }
                }

                if(!Found)
                    RemoveWindowFromMonocleTree(Screen, WindowIDsInTree[IDIndex], true);
            }
        }
        else
        {
            tree_node *WindowNode = GetNodeFromWindowID(Space->RootNode, WindowIDsInTree[0], SpaceModeMonocle);
            if(!WindowNode)
                return;

            free(WindowNode);
            Space->RootNode = NULL;
        }
    }
}

void AddWindowToMonocleTree(screen_info *Screen, int WindowID)
{
    if(!DoesSpaceExistInMapOfScreen(Screen))
        return;

    space_info *Space = GetActiveSpaceOfScreen(Screen);
    tree_node *CurrentNode = GetLastLeafNode(Space->RootNode);
    tree_node *NewNode = CreateRootNode();
    SetRootNodeContainer(Screen, NewNode);

    NewNode->WindowID = WindowID;
    CurrentNode->RightChild = NewNode;
    NewNode->LeftChild = CurrentNode;

    ApplyNodeContainer(NewNode, SpaceModeMonocle);
}

void RemoveWindowFromMonocleTree(screen_info *Screen, int WindowID, bool UpdateFocus)
{
    if(!DoesSpaceExistInMapOfScreen(Screen))
        return;

    space_info *Space = GetActiveSpaceOfScreen(Screen);
    tree_node *WindowNode = GetNodeFromWindowID(Space->RootNode, WindowID, Space->Mode);
    if(!WindowNode)
        return;

    tree_node *Prev = WindowNode->LeftChild;
    tree_node *Next = WindowNode->RightChild;
    tree_node *NewFocusNode = Prev;

    if(Prev)
        Prev->RightChild = Next;

    if(Next)
        Next->LeftChild = Prev;

    if(WindowNode == Space->RootNode)
    {
        Space->RootNode = Next;
        NewFocusNode = Next;
    }

    if(UpdateFocus)
    {
        SetWindowFocusByNode(NewFocusNode);
        MoveCursorToCenterOfFocusedWindow();
    }

    free(WindowNode);
}

void AddWindowToTreeOfUnfocusedMonitor(screen_info *Screen, window_info *Window, bool UpdateFocus)
{
    screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
    if(!Screen || !Window || Screen == ScreenOfWindow)
        return;

    space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
    if(SpaceOfWindow->Mode == SpaceModeBSP)
        RemoveWindowFromBSPTree(ScreenOfWindow, Window->WID, false, false);
    else if(SpaceOfWindow->Mode == SpaceModeMonocle)
        RemoveWindowFromMonocleTree(ScreenOfWindow, Window->WID, false);

    if(Window->WID == KWMScreen.MarkedWindow)
        ClearMarkedWindow();

    space_info *Space = GetActiveSpaceOfScreen(Screen);
    if(Space->RootNode)
    {
        if(Space->Mode == SpaceModeBSP)
        {
            DEBUG("AddWindowToTreeOfUnfocusedMonitor() BSP Space")
            tree_node *CurrentNode = GetFirstLeafNode(Space->RootNode);
            split_type SplitMode = KWMScreen.SplitMode == SPLIT_OPTIMAL ? GetOptimalSplitMode(CurrentNode) : KWMScreen.SplitMode;

            CreateLeafNodePair(Screen, CurrentNode, CurrentNode->WindowID, Window->WID, SplitMode);
            ApplyNodeContainer(CurrentNode, Space->Mode);
        }
        else if(Space->Mode == SpaceModeMonocle)
        {
            DEBUG("AddWindowToTreeOfUnfocusedMonitor() Monocle Space")
            tree_node *CurrentNode = GetLastLeafNode(Space->RootNode);
            tree_node *NewNode = CreateRootNode();
            SetRootNodeContainer(Screen, NewNode);

            NewNode->WindowID = Window->WID;
            CurrentNode->RightChild = NewNode;
            NewNode->LeftChild = CurrentNode;
            ResizeWindowToContainerSize(NewNode);
        }
    }
    else
    {
        std::vector<window_info*> Windows;
        Windows.push_back(Window);
        CreateWindowNodeTree(Screen, &Windows);
    }

    if(UpdateFocus)
    {
        SetWindowFocus(Window);
        MoveCursorToCenterOfFocusedWindow();
        UpdateActiveScreen();
    }
}

void ToggleWindowFloating(int WindowID, bool Center)
{
    Assert(KWMScreen.Current, "ToggleWindowFloating()")

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    if(IsWindowOnActiveSpace(WindowID) &&
       Space->Mode == SpaceModeBSP)
    {
        int WindowIndex;
        if(IsWindowFloating(WindowID, &WindowIndex))
        {
            KWMTiling.FloatingWindowLst.erase(KWMTiling.FloatingWindowLst.begin() + WindowIndex);
            AddWindowToBSPTree(KWMScreen.Current, WindowID);

            if(KWMMode.Focus != FocusModeDisabled && KWMMode.Focus != FocusModeAutofocus && KWMToggles.StandbyOnFloat)
                KWMMode.Focus = FocusModeAutoraise;
        }
        else
        {
            KWMTiling.FloatingWindowLst.push_back(WindowID);
            RemoveWindowFromBSPTree(KWMScreen.Current, WindowID, Center, Center);

            if(KWMMode.Focus != FocusModeDisabled && KWMMode.Focus != FocusModeAutofocus && KWMToggles.StandbyOnFloat)
                KWMMode.Focus = FocusModeStandby;
        }
    }
}

void ToggleFocusedWindowFloating()
{
    if(KWMFocus.Window)
        ToggleWindowFloating(KWMFocus.Window->WID, true);
}

void ToggleFocusedWindowParentContainer()
{
    if(!KWMFocus.Window || !DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    if(Space->Mode != SpaceModeBSP)
        return;

    tree_node *Node = GetNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID, Space->Mode);
    if(Node && Node->Parent)
    {
        if(IsLeafNode(Node) && Node->Parent->WindowID == -1)
        {
            DEBUG("ToggleFocusedWindowParentContainer() Set Parent Container")
            Node->Parent->WindowID = Node->WindowID;
            ResizeWindowToContainerSize(Node->Parent);
            UpdateBorder("focused");
        }
        else
        {
            DEBUG("ToggleFocusedWindowParentContainer() Restore Window Container")
            Node->Parent->WindowID = -1;
            ResizeWindowToContainerSize(Node);
            UpdateBorder("focused");
        }
    }
}

void ToggleFocusedWindowFullscreen()
{
    if(!KWMFocus.Window || !DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    if(Space->Mode == SpaceModeBSP && !IsLeafNode(Space->RootNode))
    {
        tree_node *Node = NULL;
        if(Space->RootNode->WindowID == -1)
        {
            Node = GetNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID, Space->Mode);
            if(Node)
            {
                DEBUG("ToggleFocusedWindowFullscreen() Set fullscreen")
                Space->RootNode->WindowID = Node->WindowID;
                ResizeWindowToContainerSize(Space->RootNode);
                UpdateBorder("focused");
            }
        }
        else
        {
            DEBUG("ToggleFocusedWindowFullscreen() Restore old size")
            Space->RootNode->WindowID = -1;

            Node = GetNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID, Space->Mode);
            if(Node)
            {
                ResizeWindowToContainerSize(Node);
                UpdateBorder("focused");
            }
        }
    }
}

void DetachAndReinsertWindow(int WindowID, int Degrees)
{
    if(WindowID == KWMScreen.MarkedWindow)
    {
        int Marked = KWMScreen.MarkedWindow;
        if(Marked == -1 || (KWMFocus.Window && Marked == KWMFocus.Window->WID))
            return;

        ToggleWindowFloating(Marked, false);
        ClearMarkedWindow();
        ToggleWindowFloating(Marked, false);
        MoveCursorToCenterOfFocusedWindow();
    }
    else
    {
        if(WindowID == KWMScreen.MarkedWindow ||
           WindowID == -1)
            return;

        window_info InsertWindow = {};
        if(FindClosestWindow(Degrees, &InsertWindow, false))
        {
            ToggleWindowFloating(WindowID, false);
            KWMScreen.MarkedWindow = InsertWindow.WID;
            ToggleWindowFloating(WindowID, false);
            MoveCursorToCenterOfFocusedWindow();
        }
    }
}

void SwapFocusedWindowWithMarked()
{
    if(!KWMFocus.Window || KWMScreen.MarkedWindow == KWMFocus.Window->WID || KWMScreen.MarkedWindow == -1)
        return;

    if(DoesSpaceExistInMapOfScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        tree_node *FocusedWindowNode = GetNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID, Space->Mode);
        if(FocusedWindowNode)
        {
            tree_node *NewFocusNode = GetNodeFromWindowID(Space->RootNode, KWMScreen.MarkedWindow, Space->Mode);
            if(NewFocusNode)
            {
                SwapNodeWindowIDs(FocusedWindowNode, NewFocusNode);
                MoveCursorToCenterOfFocusedWindow();
            }
        }
    }

    ClearMarkedWindow();
}

void SwapFocusedWindowWithNearest(int Shift)
{
    if(!KWMFocus.Window || !DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    tree_node *FocusedWindowNode = GetNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID, Space->Mode);
    if(FocusedWindowNode)
    {
        tree_node *NewFocusNode = NULL;;

        if(Shift == 1)
            NewFocusNode = GetNearestNodeToTheRight(FocusedWindowNode, Space->Mode);
        else if(Shift == -1)
            NewFocusNode = GetNearestNodeToTheLeft(FocusedWindowNode, Space->Mode);

        if(NewFocusNode)
        {
            SwapNodeWindowIDs(FocusedWindowNode, NewFocusNode);
            MoveCursorToCenterOfFocusedWindow();

            if(FocusedWindowNode->WindowID == KWMScreen.MarkedWindow ||
               NewFocusNode->WindowID == KWMScreen.MarkedWindow)
                   UpdateBorder("marked");
        }
    }
}

void SwapFocusedWindowDirected(int Degrees)
{
    if(!KWMFocus.Window || !DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    tree_node *FocusedWindowNode = GetNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID, Space->Mode);
    if(FocusedWindowNode)
    {
        tree_node *NewFocusNode = NULL;
        if(Space->Mode == SpaceModeBSP)
        {
            window_info SwapWindow = {};
            if(FindClosestWindow(Degrees, &SwapWindow, KWMMode.Cycle == CycleModeScreen))
                NewFocusNode = GetNodeFromWindowID(Space->RootNode, SwapWindow.WID, Space->Mode);
        }
        else if(Space->Mode == SpaceModeMonocle)
        {
            if(Degrees == 90)
                NewFocusNode = GetNearestNodeToTheRight(FocusedWindowNode, Space->Mode);
            else if(Degrees == 270)
                NewFocusNode = GetNearestNodeToTheLeft(FocusedWindowNode, Space->Mode);
        }

        if(NewFocusNode)
        {
            SwapNodeWindowIDs(FocusedWindowNode, NewFocusNode);
            MoveCursorToCenterOfFocusedWindow();

            if(FocusedWindowNode->WindowID == KWMScreen.MarkedWindow ||
               NewFocusNode->WindowID == KWMScreen.MarkedWindow)
                UpdateBorder("marked");
        }
    }
}

bool WindowIsInDirection(window_info *A, window_info *B, int Degrees, bool Wrap)
{
    if(Wrap)
    {
        if(Degrees == 0 || Degrees == 180)
            return A->Y != B->Y && fmax(A->X, B->X) < fmin(B->X + B->Width, A->X + A->Width);
        else if(Degrees == 90 || Degrees == 270)
            return A->X != B->X && fmax(A->Y, B->Y) < fmin(B->Y + B->Height, A->Y + A->Height);
    }
    else
    {
        if(Degrees == 0)
            return B->Y + B->Height < A->Y;
        else if(Degrees == 90)
            return B->X > A->X + A->Width;
        else if(Degrees == 180)
            return B->Y > A->Y + A->Height;
        else if(Degrees == 270)
            return B->X + B->Width < A->X;
    }

    return false;
}

void GetCenterOfWindow(window_info *Window, int *X, int *Y)
{
    *X = Window->X + Window->Width / 2;
    *Y = Window->Y + Window->Height / 2;
}

double GetWindowDistance(window_info *A, window_info *B)
{
    double Dist = INT_MAX;

    if(A && B)
    {
        int X1, Y1, X2, Y2;
        GetCenterOfWindow(A, &X1, &Y1);
        GetCenterOfWindow(B, &X2, &Y2);

        int ScoreX = X1 >= X2 - 15 && X1 <= X2 + 15 ? 1 : 11;
        int ScoreY = Y1 >= Y2 - 10 && Y1 <= Y2 + 10 ? 1 : 22;
        int Weight = ScoreX * ScoreY;
        Dist = std::sqrt(std::pow(X2-X1, 2) + std::pow(Y2-Y1, 2)) + Weight;
    }

    return Dist;
}

bool FindClosestWindow(int Degrees, window_info *Target, bool Wrap)
{
    *Target = KWMFocus.Cache;
    window_info *Match = KWMFocus.Window;
    std::vector<window_info> Windows = KWMTiling.WindowLst;

    int MatchX, MatchY;
    GetCenterOfWindow(Match, &MatchX, &MatchY);

    double MinDist = INT_MAX;
    for(int Index = 0; Index < Windows.size(); ++Index)
    {
        if(!WindowsAreEqual(Match, &Windows[Index]) &&
           WindowIsInDirection(Match, &Windows[Index], Degrees, Wrap))
        {
            window_info FocusWindow = Windows[Index];

            if(Wrap)
            {
                int WindowX, WindowY;
                GetCenterOfWindow(&Windows[Index], &WindowX, &WindowY);

                window_info WrappedWindow = Windows[Index];
                if(Degrees == 0 && MatchY < WindowY)
                    WrappedWindow.Y -= KWMScreen.Current->Height;
                else if(Degrees == 180 && MatchY > WindowY)
                    WrappedWindow.Y += KWMScreen.Current->Height;
                else if(Degrees == 90 && MatchX > WindowX)
                    WrappedWindow.X += KWMScreen.Current->Width;
                else if(Degrees == 270 && MatchX < WindowX)
                    WrappedWindow.X -= KWMScreen.Current->Width;

                FocusWindow = WrappedWindow;
            }

            double Dist = GetWindowDistance(Match, &FocusWindow);
            if(Dist < MinDist)
            {
                MinDist = Dist;
                *Target = Windows[Index];
            }
        }
    }

    return MinDist != INT_MAX;
}

void ShiftWindowFocusDirected(int Degrees)
{
    if(!KWMFocus.Window || !DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    if(Space->Mode == SpaceModeBSP)
    {
        window_info NewFocusWindow = {};
        if((KWMMode.Cycle == CycleModeDisabled &&
            FindClosestWindow(Degrees, &NewFocusWindow, false)) ||
           (KWMMode.Cycle == CycleModeScreen &&
            FindClosestWindow(Degrees, &NewFocusWindow, true)))
        {
            SetWindowFocus(&NewFocusWindow);
            MoveCursorToCenterOfFocusedWindow();
        }
        else if(KWMMode.Cycle == CycleModeAll)
        {
            if(FindClosestWindow(Degrees, &NewFocusWindow, true))
            {
                int ScreenIndex = KWMScreen.Current->ID;
                if(Degrees == 90 && WindowIsInDirection(KWMFocus.Window, &NewFocusWindow, 270, false))
                    ScreenIndex = GetIndexOfNextScreen();
                else if(Degrees == 270 && WindowIsInDirection(KWMFocus.Window, &NewFocusWindow, 90, false))
                    ScreenIndex = GetIndexOfPrevScreen();

                if(ScreenIndex == KWMScreen.Current->ID)
                {
                    SetWindowFocus(&NewFocusWindow);
                    MoveCursorToCenterOfFocusedWindow();
                }
                else
                {
                    screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
                    space_info *Space = GetActiveSpaceOfScreen(Screen);
                    tree_node *RootNode = Space->RootNode;
                    tree_node *FocusNode = Degrees == 90 ? GetFirstLeafNode(RootNode) : GetLastLeafNode(RootNode);
                    if(FocusNode)
                        GiveFocusToScreen(ScreenIndex, FocusNode, false);
                }
            }
        }
    }
    else if(Space->Mode == SpaceModeMonocle)
    {
        if(Degrees == 90)
            ShiftWindowFocus(1);
        else if(Degrees == 270)
            ShiftWindowFocus(-1);
    }
}

void ShiftWindowFocus(int Shift)
{
    if(!KWMFocus.Window || !DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    tree_node *FocusedWindowNode = GetNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID, Space->Mode);
    if(FocusedWindowNode)
    {
        tree_node *FocusNode = NULL;

        if(Shift == 1)
        {
            FocusNode = GetNearestNodeToTheRight(FocusedWindowNode, Space->Mode);
            while(IsPseudoNode(FocusNode))
                FocusNode = GetNearestNodeToTheRight(FocusNode, Space->Mode);

            if(KWMMode.Cycle == CycleModeScreen && !FocusNode)
            {
                FocusNode = GetFirstLeafNode(Space->RootNode);
                while(IsPseudoNode(FocusNode))
                    FocusNode = GetNearestNodeToTheRight(FocusNode, Space->Mode);
            }
            else if(KWMMode.Cycle == CycleModeAll && !FocusNode)
            {
                int ScreenIndex = GetIndexOfNextScreen();
                screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
                space_info *Space = GetActiveSpaceOfScreen(Screen);
                FocusNode = GetFirstLeafNode(Space->RootNode);
                if(FocusNode)
                {
                    GiveFocusToScreen(ScreenIndex, FocusNode, false);
                    return;
                }
            }
        }
        else if(Shift == -1)
        {
            FocusNode = GetNearestNodeToTheLeft(FocusedWindowNode, Space->Mode);
            while(IsPseudoNode(FocusNode))
                FocusNode = GetNearestNodeToTheLeft(FocusNode, Space->Mode);

            if(KWMMode.Cycle == CycleModeScreen && !FocusNode)
            {
                FocusNode = GetLastLeafNode(Space->RootNode);
                while(IsPseudoNode(FocusNode))
                    FocusNode = GetNearestNodeToTheLeft(FocusNode, Space->Mode);
            }
            else if(KWMMode.Cycle == CycleModeAll && !FocusNode)
            {
                int ScreenIndex = GetIndexOfPrevScreen();
                screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
                space_info *Space = GetActiveSpaceOfScreen(Screen);
                FocusNode = GetLastLeafNode(Space->RootNode);
                if(FocusNode)
                {
                    GiveFocusToScreen(ScreenIndex, FocusNode, false);
                    return;
                }
            }
        }

        SetWindowFocusByNode(FocusNode);
        MoveCursorToCenterOfFocusedWindow();
    }
}

void FocusWindowByID(int WindowID)
{
    window_info *Window = GetWindowByID(WindowID);
    if(Window)
    {
        screen_info *Screen = GetDisplayOfWindow(Window);
        if(Screen == KWMScreen.Current)
        {
            SetWindowFocus(Window);
            MoveCursorToCenterOfFocusedWindow();
        }

        if(Screen != KWMScreen.Current && IsSpaceInitializedForScreen(Screen))
        {
            space_info *Space = GetActiveSpaceOfScreen(Screen);
            tree_node *Root = Space->RootNode;
            tree_node *Node = GetNodeFromWindowID(Root, WindowID, Space->Mode);
            if(Node)
                GiveFocusToScreen(Screen->ID, Node, false);
        }
    }
}

void MoveCursorToCenterOfWindow(window_info *Window)
{
    Assert(Window, "MoveCursorToCenterOfWindow()")
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
    {
        CGPoint WindowPos = GetWindowPos(WindowRef);
        CGSize WindowSize = GetWindowSize(WindowRef);
        CGWarpMouseCursorPosition(CGPointMake(WindowPos.x + WindowSize.width / 2, WindowPos.y + WindowSize.height / 2));
    }
}

void MoveCursorToCenterOfFocusedWindow()
{
    if(KWMToggles.UseMouseFollowsFocus && KWMFocus.Window && !IsActiveSpaceFloating())
        MoveCursorToCenterOfWindow(KWMFocus.Window);
}

void ClearMarkedWindow()
{
    KWMScreen.MarkedWindow = -1;
    ClearBorder(&MarkedBorder);
}

void MarkWindowContainer(window_info *Window)
{
    if(Window)
    {
        if(KWMScreen.MarkedWindow == Window->WID)
        {
            DEBUG("MarkWindowContainer() Unmarked " << Window->Name)
            ClearMarkedWindow();
        }
        else
        {
            DEBUG("MarkWindowContainer() Marked " << Window->Name)
            KWMScreen.MarkedWindow = Window->WID;
            UpdateBorder("marked");
        }
    }
}

void MarkFocusedWindowContainer()
{
    MarkWindowContainer(KWMFocus.Window);
}

void SetWindowRefFocus(AXUIElementRef WindowRef)
{
    int OldProcessPID = KWMFocus.Window ? KWMFocus.Window->PID : -1;

    KWMFocus.Cache = GetWindowByRef(WindowRef);
    if(WindowsAreEqual(&KWMFocus.Cache, &KWMFocus.NULLWindowInfo))
    {
        KWMFocus.Window = NULL;
        ClearBorder(&FocusedBorder);
        return;
    }

    KWMFocus.Window = &KWMFocus.Cache;
    ProcessSerialNumber NewPSN;
    GetProcessForPID(KWMFocus.Window->PID, &NewPSN);
    KWMFocus.PSN = NewPSN;

    AXUIElementSetAttributeValue(WindowRef, kAXMainAttribute, kCFBooleanTrue);
    AXUIElementSetAttributeValue(WindowRef, kAXFocusedAttribute, kCFBooleanTrue);
    AXUIElementPerformAction(WindowRef, kAXRaiseAction);

    if(KWMMode.Focus != FocusModeAutofocus && KWMMode.Focus != FocusModeStandby)
        SetFrontProcessWithOptions(&KWMFocus.PSN, kSetFrontProcessFrontWindowOnly);

    if(!IsActiveSpaceFloating())
    {
        if(OldProcessPID != KWMFocus.Window->PID ||
           !KWMFocus.Observer)
            CreateApplicationNotifications();

        if(ShouldWindowGainFocus(KWMFocus.Window))
            UpdateBorder("focused");
    }

    if(KWMToggles.EnableTilingMode)
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        Space->FocusedNode = GetNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID, Space->Mode);
    }

    DEBUG("SetWindowRefFocus() Focused Window: " << KWMFocus.Window->Name << " " << KWMFocus.Window->X << "," << KWMFocus.Window->Y)
    if(KWMMode.Focus != FocusModeDisabled &&
       KWMMode.Focus != FocusModeAutofocus &&
       KWMToggles.StandbyOnFloat)
        KWMMode.Focus = IsFocusedWindowFloating() ? FocusModeStandby : FocusModeAutoraise;
}

void SetWindowFocus(window_info *Window)
{
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
        SetWindowRefFocus(WindowRef);
}

void SetWindowFocusByNode(tree_node *Node)
{
    if(Node)
    {
        window_info *Window = GetWindowByID(Node->WindowID);
        if(Window)
        {
            DEBUG("SetWindowFocusByNode()")
            SetWindowFocus(Window);
        }
    }
}

bool IsWindowNonResizable(AXUIElementRef WindowRef, window_info *Window, CFTypeRef NewWindowPos, CFTypeRef NewWindowSize)
{
    Assert(WindowRef, "IsWindowNonResizable() WindowRef")
    Assert(Window, "IsWindowNonResizable() Window")

    AXError PosError = kAXErrorFailure;
    AXError SizeError = AXUIElementSetAttributeValue(WindowRef, kAXSizeAttribute, NewWindowSize);
    if(SizeError == kAXErrorSuccess)
    {
        PosError = AXUIElementSetAttributeValue(WindowRef, kAXPositionAttribute, NewWindowPos);
        SizeError = AXUIElementSetAttributeValue(WindowRef, kAXSizeAttribute, NewWindowSize);
    }

    if(PosError != kAXErrorSuccess || SizeError != kAXErrorSuccess)
    {
        KWMTiling.FloatingWindowLst.push_back(Window->WID);
        screen_info *Screen = GetDisplayOfWindow(Window);
        if(DoesSpaceExistInMapOfScreen(Screen))
        {
            space_info *Space = GetActiveSpaceOfScreen(Screen);
            if(Space->Mode == SpaceModeBSP)
                RemoveWindowFromBSPTree(Screen, Window->WID, false, false);
            else if(Space->Mode == SpaceModeMonocle)
                RemoveWindowFromMonocleTree(Screen, Window->WID, false);
        }

        return true;
    }

    return false;
}

void CenterWindowInsideNodeContainer(AXUIElementRef WindowRef, int *Xptr, int *Yptr, int *Wptr, int *Hptr)
{
    CGPoint WindowOrigin = GetWindowPos(WindowRef);
    CGSize WindowOGSize = GetWindowSize(WindowRef);

    int &X = *Xptr, &Y = *Yptr, &Width = *Wptr, &Height = *Hptr;
    int XDiff = (X + Width) - (WindowOrigin.x + WindowOGSize.width);
    int YDiff = (Y + Height) - (WindowOrigin.y + WindowOGSize.height);

    if(XDiff > 0 || YDiff > 0)
    {
        double XOff = XDiff / 2.0f;
        X += XOff > 0 ? XOff : 0;
        Width -= XOff > 0 ? XOff : 0;

        double YOff = YDiff / 2.0f;
        Y += YOff > 0 ? YOff : 0;
        Height -= YOff > 0 ? YOff : 0;

        CGPoint WindowPos = CGPointMake(X, Y);
        CFTypeRef NewWindowPos = (CFTypeRef)AXValueCreate(kAXValueCGPointType, (const void*)&WindowPos);

        CGSize WindowSize = CGSizeMake(Width, Height);
        CFTypeRef NewWindowSize = (CFTypeRef)AXValueCreate(kAXValueCGSizeType, (void*)&WindowSize);

        if(NewWindowPos)
        {
            AXUIElementSetAttributeValue(WindowRef, kAXPositionAttribute, NewWindowPos);
            CFRelease(NewWindowPos);
        }

        if(NewWindowSize)
        {
            AXUIElementSetAttributeValue(WindowRef, kAXSizeAttribute, NewWindowSize);
            CFRelease(NewWindowSize);
        }
    }
}

void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height)
{
    CGPoint WindowPos = CGPointMake(X, Y);
    CFTypeRef NewWindowPos = (CFTypeRef)AXValueCreate(kAXValueCGPointType, (const void*)&WindowPos);

    CGSize WindowSize = CGSizeMake(Width, Height);
    CFTypeRef NewWindowSize = (CFTypeRef)AXValueCreate(kAXValueCGSizeType, (void*)&WindowSize);

    if(!NewWindowPos || !NewWindowSize)
        return;

    Assert(WindowRef, "SetWindowDimensions() WindowRef")
    Assert(Window, "SetWindowDimensions() Window")

    DEBUG("SetWindowDimensions()")
    bool UpdateWindowInfo = true;
    if(KWMTiling.FloatNonResizable)
    {
        if(IsWindowNonResizable(WindowRef, Window, NewWindowPos, NewWindowSize))
            UpdateWindowInfo = false;
        else
            CenterWindowInsideNodeContainer(WindowRef, &X, &Y, &Width, &Height);

    }
    else
    {
        AXUIElementSetAttributeValue(WindowRef, kAXPositionAttribute, NewWindowPos);
        AXUIElementSetAttributeValue(WindowRef, kAXSizeAttribute, NewWindowSize);
        CenterWindowInsideNodeContainer(WindowRef, &X, &Y, &Width, &Height);
    }

    if(UpdateWindowInfo)
    {
        Window->X = X;
        Window->Y = Y;
        Window->Width = Width;
        Window->Height = Height;
        UpdateBorder("focused");
    }

    CFRelease(NewWindowPos);
    CFRelease(NewWindowSize);
}

void CenterWindow(screen_info *Screen, window_info *Window)
{
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
    {
        int NewX = Screen->X + Screen->Width / 4;
        int NewY = Screen->Y + Screen->Height / 4;
        int NewWidth = Screen->Width / 2;
        int NewHeight = Screen->Height / 2;
        SetWindowDimensions(WindowRef, Window, NewX, NewY, NewWidth, NewHeight);
    }
}

void MoveFloatingWindow(int X, int Y)
{
    if(!KWMFocus.Window ||
       (!IsWindowFloating(KWMFocus.Window->WID, NULL) &&
       !IsApplicationFloating(KWMFocus.Window)))
        return;

    AXUIElementRef WindowRef;
    if(GetWindowRef(KWMFocus.Window, &WindowRef))
    {
        CGPoint WindowPos = GetWindowPos(WindowRef);
        WindowPos.x += X;
        WindowPos.y += Y;

        CFTypeRef NewWindowPos = (CFTypeRef)AXValueCreate(kAXValueCGPointType, (const void*)&WindowPos);
        if(NewWindowPos)
        {
            AXUIElementSetAttributeValue(WindowRef, kAXPositionAttribute, NewWindowPos);
            CFRelease(NewWindowPos);
        }
    }
}

void ModifyContainerSplitRatio(double Offset)
{
    if(DoesSpaceExistInMapOfScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        tree_node *Root = Space->RootNode;
        if(IsLeafNode(Root) || Root->WindowID != -1)
            return;

        tree_node *Node = GetNodeFromWindowID(Root, KWMFocus.Window->WID, Space->Mode);
        if(Node && Node->Parent)
        {
            if(Node->Parent->SplitRatio + Offset > 0.0 &&
               Node->Parent->SplitRatio + Offset < 1.0)
            {
                Node->Parent->SplitRatio += Offset;
                ResizeNodeContainer(KWMScreen.Current, Node->Parent);
                ApplyNodeContainer(Node->Parent, Space->Mode);
            }
        }
    }
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

            if(WindowsAreEqual(Window, KWMFocus.Window))
                KWMFocus.Cache = *Window;
        }
    }
}

void ResizeWindowToContainerSize(window_info *Window)
{
    Assert(Window, "ResizeWindowToContainerSize()")
    if(DoesSpaceExistInMapOfScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        tree_node *Node = GetNodeFromWindowID(Space->RootNode, Window->WID, Space->Mode);
        if(Node)
            ResizeWindowToContainerSize(Node);
    }
}

void ResizeWindowToContainerSize()
{
    if(KWMFocus.Window)
        ResizeWindowToContainerSize(KWMFocus.Window);
}

CGPoint GetCursorPos()
{
    CGEventRef Event = CGEventCreate(NULL);
    CGPoint Cursor = CGEventGetLocation(Event);
    CFRelease(Event);

    return Cursor;
}

std::string GetUTF8String(CFStringRef Temp)
{
    std::string Result;

    if(!CFStringGetCStringPtr(Temp, kCFStringEncodingUTF8))
    {
        CFIndex Length = CFStringGetLength(Temp);
        CFIndex Bytes = 4 * Length + 1;
        char *TempUTF8StringPtr = (char*) malloc(Bytes);

        CFStringGetCString(Temp, TempUTF8StringPtr, Bytes, kCFStringEncodingUTF8);
        if(TempUTF8StringPtr)
        {
            Result = TempUTF8StringPtr;
            free(TempUTF8StringPtr);
        }
    }

    return Result;
}

std::string GetWindowTitle(AXUIElementRef WindowRef)
{
    CFStringRef Temp;
    std::string WindowTitle;
    AXUIElementCopyAttributeValue(WindowRef, kAXTitleAttribute, (CFTypeRef*)&Temp);

    if(Temp)
    {
        WindowTitle = GetUTF8String(Temp);
        if(WindowTitle.empty())
            WindowTitle = CFStringGetCStringPtr(Temp, kCFStringEncodingMacRoman);

        CFRelease(Temp);
    }

    return WindowTitle;
}

CGSize GetWindowSize(AXUIElementRef WindowRef)
{
    AXValueRef Temp;
    CGSize WindowSize;

    AXUIElementCopyAttributeValue(WindowRef, kAXSizeAttribute, (CFTypeRef*)&Temp);
    if(Temp)
    {
        AXValueGetValue(Temp, kAXValueCGSizeType, &WindowSize);
        CFRelease(Temp);
    }

    return WindowSize;
}

CGPoint GetWindowPos(window_info *Window)
{
    CGPoint Result = {};
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
        Result = GetWindowPos(WindowRef);

    return Result;
}

CGPoint GetWindowPos(AXUIElementRef WindowRef)
{
    AXValueRef Temp;
    CGPoint WindowPos;

    AXUIElementCopyAttributeValue(WindowRef, kAXPositionAttribute, (CFTypeRef*)&Temp);
    if(Temp)
    {
        AXValueGetValue(Temp, kAXValueCGPointType, &WindowPos);
        CFRelease(Temp);
    }

    return WindowPos;
}

window_info GetWindowByRef(AXUIElementRef WindowRef)
{
    Assert(WindowRef, "GetWindowByRef()")
    window_info *Window = GetWindowByID(GetWindowIDFromRef(WindowRef));
    return Window ? *Window : KWMFocus.NULLWindowInfo;
}

inline int GetWindowIDFromRef(AXUIElementRef WindowRef)
{
    int WindowRefWID = -1;
    _AXUIElementGetWindow(WindowRef, &WindowRefWID);
    return WindowRefWID;
}

window_info *GetWindowByID(int WindowID)
{
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.FocusLst.size(); ++WindowIndex)
    {
        if(KWMTiling.FocusLst[WindowIndex].WID == WindowID)
            return &KWMTiling.FocusLst[WindowIndex];
    }

    return NULL;
}

bool GetWindowRole(window_info *Window, CFTypeRef *Role, CFTypeRef *SubRole)
{
    bool Result = false;

    std::map<int, window_role>::iterator It = KWMCache.WindowRole.find(Window->WID);
    if(It != KWMCache.WindowRole.end())
    {
        *Role = KWMCache.WindowRole[Window->WID].Role;
        *SubRole = KWMCache.WindowRole[Window->WID].SubRole;
        Result = true;
    }
    else
    {
        AXUIElementRef WindowRef;
        if(GetWindowRef(Window, &WindowRef))
        {
            AXUIElementCopyAttributeValue(WindowRef, kAXRoleAttribute, (CFTypeRef *)Role);
            AXUIElementCopyAttributeValue(WindowRef, kAXSubroleAttribute, (CFTypeRef *)SubRole);
            window_role RoleEntry = { *Role, *SubRole };
            KWMCache.WindowRole[Window->WID] = RoleEntry;
            Result = true;
        }
    }

    return Result;
}

bool GetWindowRef(window_info *Window, AXUIElementRef *WindowRef)
{
    if(Window->Owner == "Dock")
        return false;

    if(GetWindowRefFromCache(Window, WindowRef))
        return true;

    AXUIElementRef App = AXUIElementCreateApplication(Window->PID);
    if(!App)
    {
        DEBUG("GetWindowRef() Failed to get App for: " << Window->Name)
        return false;
    }

    CFArrayRef AppWindowLst;
    AXUIElementCopyAttributeValue(App, kAXWindowsAttribute, (CFTypeRef*)&AppWindowLst);
    if(!AppWindowLst)
    {
        DEBUG("GetWindowRef() Could not get AppWindowLst")
        return false;
    }

    bool Found = false;
    FreeWindowRefCache(Window->PID);
    CFIndex AppWindowCount = CFArrayGetCount(AppWindowLst);
    for(CFIndex WindowIndex = 0; WindowIndex < AppWindowCount; ++WindowIndex)
    {
        AXUIElementRef AppWindowRef = (AXUIElementRef)CFArrayGetValueAtIndex(AppWindowLst, WindowIndex);
        if(AppWindowRef)
        {
            KWMCache.WindowRefs[Window->PID].push_back(AppWindowRef);
            if(!Found)
            {
                if(GetWindowIDFromRef(AppWindowRef) == Window->WID)
                {
                    *WindowRef = AppWindowRef;
                    Found = true;
                }
            }
        }
    }

    CFRelease(App);
    return Found;
}

bool IsApplicationInCache(int PID, std::vector<AXUIElementRef> *Elements)
{
    bool Result = false;
    std::map<int, std::vector<AXUIElementRef> >::iterator It = KWMCache.WindowRefs.find(PID);

    if(It != KWMCache.WindowRefs.end())
    {
        *Elements = It->second;
        Result = true;
    }

    return Result;
}

bool GetWindowRefFromCache(window_info *Window, AXUIElementRef *WindowRef)
{
    std::vector<AXUIElementRef> Elements;
    bool IsCached = IsApplicationInCache(Window->PID, &Elements);

    if(IsCached)
    {
        for(std::size_t ElementIndex = 0; ElementIndex < Elements.size(); ++ElementIndex)
        {
            if(GetWindowIDFromRef(Elements[ElementIndex]) == Window->WID)
            {
                *WindowRef = Elements[ElementIndex];
                return true;
            }
        }
    }

    if(!IsCached)
        KWMCache.WindowRefs[Window->PID] = std::vector<AXUIElementRef>();

    return false;
}

void FreeWindowRefCache(int PID)
{
    std::map<int, std::vector<AXUIElementRef> >::iterator It = KWMCache.WindowRefs.find(PID);

    if(It != KWMCache.WindowRefs.end())
    {
        int NumElements = KWMCache.WindowRefs[PID].size();
        for(int RefIndex = 0; RefIndex < NumElements; ++RefIndex)
            CFRelease(KWMCache.WindowRefs[PID][RefIndex]);

        KWMCache.WindowRefs[PID].clear();
    }
}

void GetWindowInfo(const void *Key, const void *Value, void *Context)
{
    CFStringRef K = (CFStringRef)Key;
    std::string KeyStr = CFStringGetCStringPtr(K, kCFStringEncodingMacRoman);
    CFTypeID ID = CFGetTypeID(Value);
    if(ID == CFStringGetTypeID())
    {
        CFStringRef V = (CFStringRef)Value;
        std::string ValueStr = GetUTF8String(V);
        if(ValueStr.empty() && CFStringGetCStringPtr(V, kCFStringEncodingMacRoman))
            ValueStr = CFStringGetCStringPtr(V, kCFStringEncodingMacRoman);

        if(KeyStr == "kCGWindowName")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Name = ValueStr;
        else if(KeyStr == "kCGWindowOwnerName")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Owner = ValueStr;
    }
    else if(ID == CFNumberGetTypeID())
    {
        int MyInt;
        CFNumberRef V = (CFNumberRef)Value;
        CFNumberGetValue(V, kCFNumberSInt64Type, &MyInt);
        if(KeyStr == "kCGWindowNumber")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].WID = MyInt;
        else if(KeyStr == "kCGWindowOwnerPID")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].PID = MyInt;
        else if(KeyStr == "kCGWindowLayer")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Layer = MyInt;
        else if(KeyStr == "X")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].X = MyInt;
        else if(KeyStr == "Y")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Y = MyInt;
        else if(KeyStr == "Width")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Width = MyInt;
        else if(KeyStr == "Height")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Height = MyInt;
    }
    else if(ID == CFDictionaryGetTypeID())
    {
        CFDictionaryRef Elem = (CFDictionaryRef)Value;
        CFDictionaryApplyFunction(Elem, GetWindowInfo, NULL);
    }
}
