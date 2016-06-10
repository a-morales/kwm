#include "window.h"
#include "container.h"
#include "node.h"
#include "display.h"
#include "space.h"
#include "tree.h"
#include "application.h"
#include "border.h"
#include "helpers.h"
#include "rules.h"
#include "serializer.h"

#include "axlib/axlib.h"

#include <cmath>

extern std::map<CFStringRef, space_info> WindowTree;

extern ax_state AXState;
extern ax_display *FocusedDisplay;
extern ax_application *FocusedApplication;

extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_toggles KWMToggles;
extern kwm_mode KWMMode;
extern kwm_tiling KWMTiling;
extern kwm_cache KWMCache;
extern kwm_path KWMPath;
extern kwm_thread KWMThread;
extern kwm_border MarkedBorder;
extern kwm_border FocusedBorder;

EVENT_CALLBACK(Callback_AXEvent_DisplayAdded)
{
    DEBUG("AXEvent_DisplayAdded");
}

EVENT_CALLBACK(Callback_AXEvent_DisplayRemoved)
{
    DEBUG("AXEvent_DisplayRemoved");
}

EVENT_CALLBACK(Callback_AXEvent_DisplayResized)
{
    DEBUG("AXEvent_DisplayResized");
}

EVENT_CALLBACK(Callback_AXEvent_DisplayMoved)
{
    DEBUG("AXEvent_DisplayMoved");
}

EVENT_CALLBACK(Callback_AXEvent_DisplayChanged)
{
    FocusedDisplay = AXLibMainDisplay();
    printf("%d: AXEvent_DisplayChanged\n", FocusedDisplay->ArrangementID);
}

/* TODO(koekeishiya): If we trigger a space changed event through cmd+tab, we receive the 'didApplicationActivate'
                      notification before the 'didActiveSpaceChange' notification. If a space has not been visited
                      before, this will cause us to end up on that space with a unsynchronized focused application state. */
EVENT_CALLBACK(Callback_AXEvent_SpaceChanged)
{
    DEBUG("AXEvent_SpaceChanged");
    AXLibRunningApplications();
    ax_display *Display  = AXLibMainDisplay();
    if(Display)
    {
        FocusedDisplay = Display;
        FocusedDisplay->Space = AXLibGetActiveSpace(FocusedDisplay);
        printf("Display: CGDirectDisplayID %d, Arrangement %d\n", FocusedDisplay->ID, FocusedDisplay->ArrangementID);
        printf("Space: CGSSpaceID %d\n", FocusedDisplay->Space->ID);

        CreateWindowNodeTree(FocusedDisplay);
        ShouldBSPTreeUpdate(FocusedDisplay);
    }
    else
    {
        printf("What the fuck.. Display was null (?)");
    }

    /* NOTE(koekeishiya): Print the name and application of all windows currently visible. */
    /*
       std::vector<ax_window *> VisibleWindows = AXLibGetAllVisibleWindows();
       printf("VISIBLE WINDOWS: %zd\n", VisibleWindows.size());
       for(int Index = 0; Index < VisibleWindows.size(); ++Index)
       printf("%s - %s\n", VisibleWindows[Index]->Application->Name.c_str(), VisibleWindows[Index]->Name.c_str());
   */
}

/* TODO(koekeishiya): Is this interesting (?)
EVENT_CALLBACK(Callback_AXEvent_SpaceCreated)
{
    ax_space *Space = (ax_space *) Event->Context;
}
*/

EVENT_CALLBACK(Callback_AXEvent_ApplicationLaunched)
{
    ax_application *Application = (ax_application *) Event->Context;
    DEBUG("AXEvent_ApplicationLaunched: " << Application->Name);
    if(Application->Focus)
    {
        ax_display *Display = AXLibWindowDisplay(Application->Focus);
        if(Display)
        {
            AddWindowToBSPTree(Display, Application->Focus->ID);
        }
    }
}

EVENT_CALLBACK(Callback_AXEvent_ApplicationTerminated)
{
    DEBUG("AXEvent_ApplicationTerminated");

    /* TODO(koekeishiya): We probably want to flag every display for an update, as the application
                          in question could have had windows on several displays and spaces. */
    ax_display *Display = AXLibMainDisplay();
    if(Display)
        ShouldBSPTreeUpdate(Display);
}

EVENT_CALLBACK(Callback_AXEvent_ApplicationActivated)
{
    ax_application *Application = (ax_application *) Event->Context;

    FocusedApplication = Application;
    FocusedApplication->Focus = AXLibGetFocusedWindow(FocusedApplication);

    if(FocusedApplication->Focus)
    {
        DEBUG("AXEvent_ApplicationActivated: " << FocusedApplication->Name << " - " << FocusedApplication->Focus->Name);
    }
    else
    {
        DEBUG("AXEvent_ApplicationActivated: " << FocusedApplication->Name << " - [No Focused Window]");
    }

    UpdateBorder("focused");
}

EVENT_CALLBACK(Callback_AXEvent_WindowCreated)
{
    ax_window *Window = (ax_window *) Event->Context;
    if(Window && AXLibFindApplicationWindow(Window->Application, Window->ID))
    {
        DEBUG("AXEvent_WindowCreated: " << Window->Application->Name << " - " << Window->Name);
        if(AXLibIsWindowStandard(Window))
        {
            ax_display *Display = AXLibWindowDisplay(Window);
            if(Display)
                AddWindowToBSPTree(Display, Window->ID);
        }
    }
}

EVENT_CALLBACK(Callback_AXEvent_WindowDestroyed)
{
    ax_window *Window = (ax_window *) Event->Context;
    DEBUG("AXEvent_WindowDestroyed: " << Window->Application->Name << " - " << Window->Name);
    ax_display *Display = AXLibWindowDisplay(Window);
    if(Display)
        RemoveWindowFromBSPTree(Display, Window->ID);

    AXLibDestroyWindow(Window);
    UpdateBorder("focused");
    UpdateBorder("marked");
}

EVENT_CALLBACK(Callback_AXEvent_WindowFocused)
{
    if(FocusedApplication && FocusedApplication->Focus)
    {
        DEBUG("AXEvent_WindowFocused: " << FocusedApplication->Name << " - " << FocusedApplication->Focus->Name);
    }

    UpdateBorder("focused");
    UpdateBorder("marked");
}

EVENT_CALLBACK(Callback_AXEvent_WindowMoved)
{
    ax_window *Window = (ax_window *) Event->Context;
    if(Window && AXLibFindApplicationWindow(Window->Application, Window->ID))
    {
        DEBUG("AXEvent_WindowMoved: " << Window->Application->Name << " - " << Window->Name);
        UpdateBorder("focused");
        UpdateBorder("marked");
    }
}

EVENT_CALLBACK(Callback_AXEvent_WindowResized)
{
    ax_window *Window = (ax_window *) Event->Context;
    if(Window && AXLibFindApplicationWindow(Window->Application, Window->ID))
    {
        DEBUG("AXEvent_WindowResized: " << Window->Application->Name << " - " << Window->Name);
        UpdateBorder("focused");
        UpdateBorder("marked");
    }
}

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

std::vector<window_info> FilterWindowListAllDisplays()
{
    std::vector<window_info> FilteredWindowLst;
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.FocusLst.size(); ++WindowIndex)
    {
        CFTypeRef Role, SubRole;
        if(GetWindowRole(&KWMTiling.FocusLst[WindowIndex], &Role, &SubRole))
        {
            if((CFEqual(Role, kAXWindowRole) && CFEqual(SubRole, kAXStandardWindowSubrole)) ||
               IsWindowSpecificRole(&KWMTiling.FocusLst[WindowIndex], Role, SubRole))
                    FilteredWindowLst.push_back(KWMTiling.FocusLst[WindowIndex]);
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
                return false;

        if(Window->Layer == 0)
        {
            if(EnforceWindowRules(Window))
                return false;

            screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
            if(ScreenOfWindow && Screen != ScreenOfWindow)
            {
                space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
                if(!SpaceOfWindow->Initialized ||
                   SpaceOfWindow->Settings.Mode == SpaceModeFloating ||
                   GetTreeNodeFromWindowID(SpaceOfWindow->RootNode, Window->WID) ||
                   GetLinkNodeFromWindowID(SpaceOfWindow->RootNode, Window->WID))
                    continue;
            }

            CFTypeRef Role, SubRole;
            if(GetWindowRole(Window, &Role, &SubRole))
            {
                if((CFEqual(Role, kAXWindowRole) && CFEqual(SubRole, kAXStandardWindowSubrole)) ||
                   IsWindowSpecificRole(Window, Role, SubRole))
                    FilteredWindowLst.push_back(KWMTiling.WindowLst[WindowIndex]);
            }
        }
    }

    KWMTiling.WindowLst = FilteredWindowLst;
    return true;
}

bool IsFocusedWindowFloating()
{
    return KWMFocus.Window && IsWindowFloating(KWMFocus.Window->WID, NULL);
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
    Assert(Window);

    CGPoint Cursor = GetCursorPos();
    if(Cursor.x >= Window->X &&
       Cursor.x <= Window->X + Window->Width &&
       Cursor.y >= Window->Y &&
       Cursor.y <= Window->Y + Window->Height)
        return true;

    return false;
}

bool IsWindowOnActiveSpace(int WindowID) { }

void ClearFocusedWindow()
{
    ClearBorder(&FocusedBorder);
    KWMFocus.Window = NULL;
    KWMFocus.Cache = KWMFocus.NULLWindowInfo;
}

bool FocusWindowOfOSX() { }

void FocusWindowBelowCursor()
{
    if(IsSpaceTransitionInProgress() ||
       !IsActiveSpaceManaged())
           return;

    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.FocusLst.size(); ++WindowIndex)
    {
        /* Note(koekeishiya): Allow focus-follows-mouse to ignore Launchpad */
        if(KWMTiling.FocusLst[WindowIndex].Owner == "Dock" &&
           KWMTiling.FocusLst[WindowIndex].Name == "LPSpringboard")
            return;

        /* Note(koekeishiya): Allow focus-follows-mouse to work when the dock is visible */
        if(KWMTiling.FocusLst[WindowIndex].Owner == "Dock" &&
           KWMTiling.FocusLst[WindowIndex].X == 0 &&
           KWMTiling.FocusLst[WindowIndex].Y == 0)
            continue;

        if(IsWindowBelowCursor(&KWMTiling.FocusLst[WindowIndex]))
        {
            CFTypeRef Role, SubRole;
            if(GetWindowRole(&KWMTiling.FocusLst[WindowIndex], &Role, &SubRole))
            {
                if((CFEqual(Role, kAXWindowRole) && CFEqual(SubRole, kAXStandardWindowSubrole)) ||
                   IsWindowSpecificRole(&KWMTiling.FocusLst[WindowIndex], Role, SubRole))
                {
                    if(WindowsAreEqual(KWMFocus.Window, &KWMTiling.FocusLst[WindowIndex]))
                        KWMFocus.Cache = KWMTiling.FocusLst[WindowIndex];
                    else
                        SetWindowFocus(&KWMTiling.FocusLst[WindowIndex]);
                }
            }
            return;
        }
    }
}

void UpdateWindowTree() { }

void UpdateActiveWindowList(screen_info *Screen) { }

std::vector<int> GetAllWindowIDsInTree(space_info *Space)
{
    std::vector<int> Windows;
    if(Space->Settings.Mode == SpaceModeBSP)
    {
        tree_node *CurrentNode = NULL;
        GetFirstLeafNode(Space->RootNode, (void**)&CurrentNode);
        while(CurrentNode)
        {
            if(CurrentNode->WindowID != -1)
                Windows.push_back(CurrentNode->WindowID);

            link_node *Link = CurrentNode->List;
            while(Link)
            {
                Windows.push_back(Link->WindowID);
                Link = Link->Next;
            }

            CurrentNode = GetNearestTreeNodeToTheRight(CurrentNode);
        }
    }
    else if(Space->Settings.Mode == SpaceModeMonocle)
    {
        link_node *Link = Space->RootNode->List;
        while(Link)
        {
            Windows.push_back(Link->WindowID);
            Link = Link->Next;
        }
    }

    return Windows;
}

std::vector<ax_window *> GetAllAXWindowsNotInTree(ax_display *Display, std::vector<int> &WindowIDsInTree)
{
    std::vector<ax_window *> Windows;
    std::vector<ax_window *> AXWindows = AXLibGetAllVisibleWindows();
    for(std::size_t WindowIndex = 0; WindowIndex < AXWindows.size(); ++WindowIndex)
    {
        bool Found = false;
        ax_window *AXWindow = AXWindows[WindowIndex];
        ax_display *DisplayOfWindow = AXLibWindowDisplay(AXWindow);
        for(std::size_t IDIndex = 0; IDIndex < WindowIDsInTree.size(); ++IDIndex)
        {
            if(AXWindow->ID == WindowIDsInTree[IDIndex])
            {
                Found = true;
                break;
            }
        }

        if(!Found && DisplayOfWindow == Display)
            Windows.push_back(AXWindow);
    }

    return Windows;
}

std::vector<window_info*> GetAllWindowsNotInTree(std::vector<int> &WindowIDsInTree)
{
    std::vector<window_info*> Windows;
    for(std::size_t WindowIndex = 0; WindowIndex < KWMTiling.WindowLst.size(); ++WindowIndex)
    {
        bool Found = false;
        for(std::size_t IDIndex = 0; IDIndex < WindowIDsInTree.size(); ++IDIndex)
        {
            if(KWMTiling.WindowLst[WindowIndex].WID == WindowIDsInTree[IDIndex])
            {
                Found = true;
                break;
            }
        }

        if(!Found)
            Windows.push_back(&KWMTiling.WindowLst[WindowIndex]);
    }

    return Windows;
}

std::vector<uint32_t> GetAllAXWindowIDsToRemoveFromTree(std::vector<int> &WindowIDsInTree)
{
    std::vector<uint32_t> Windows;
    std::vector<ax_window *> AXWindows = AXLibGetAllVisibleWindows();

    for(std::size_t IDIndex = 0; IDIndex < WindowIDsInTree.size(); ++IDIndex)
    {
        bool Found = false;
        for(std::size_t WindowIndex = 0; WindowIndex < AXWindows.size(); ++WindowIndex)
        {
            if(AXWindows[WindowIndex]->ID == WindowIDsInTree[IDIndex])
            {
                Found = true;
                break;
            }
        }

        if(!Found)
            Windows.push_back(WindowIDsInTree[IDIndex]);
    }

    return Windows;
}

std::vector<int> GetAllWindowIDsToRemoveFromTree(std::vector<int> &WindowIDsInTree)
{
    std::vector<int> Windows;
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
            Windows.push_back(WindowIDsInTree[IDIndex]);
    }

    return Windows;
}

#define internal static
internal std::vector<uint32_t> GetAllWindowIDSOnDisplay(ax_display *Display)
{
    std::vector<uint32_t> Windows;
    std::vector<ax_window*> VisibleWindows = AXLibGetAllVisibleWindows();
    for(int Index = 0; Index < VisibleWindows.size(); ++Index)
    {
        ax_window *Window = VisibleWindows[Index];
        ax_display *DisplayOfWindow = AXLibWindowDisplay(Window);
        if(DisplayOfWindow == Display)
        {
            Windows.push_back(Window->ID);
        }
    }

    return Windows;
}

#define local_persist static
void CreateWindowNodeTree(ax_display *Display)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(!SpaceInfo->RootNode)
    {
        local_persist container_offset Offset = { 100, 100, 100, 100, 20, 20 };
        SpaceInfo->Settings.Offset = Offset;
        std::vector<uint32_t> Windows = GetAllWindowIDSOnDisplay(Display);
        SpaceInfo->RootNode = CreateTreeFromWindowIDList(Display, &Windows);
        ApplyTreeNodeContainer(SpaceInfo->RootNode);
    }
}

void CreateWindowNodeTree(screen_info *Screen, std::vector<window_info*> *Windows)
{
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    if(!Space->Initialized)
    {
        Assert(Space);
        DEBUG("CreateWindowNodeTree() Create Space " << Screen->ActiveSpace);

        Space->FocusedWindowID = -1;
        int DesktopID = GetSpaceNumberFromCGSpaceID(Screen, Screen->ActiveSpace);
        space_settings *SpaceSettings = GetSpaceSettingsForDesktopID(Screen->ID, DesktopID);
        if(SpaceSettings)
        {
            Space->Settings = *SpaceSettings;
        }
        else
        {
            SpaceSettings = GetSpaceSettingsForDisplay(Screen->ID);
            if(SpaceSettings)
                Space->Settings = *SpaceSettings;
            else
                Space->Settings = Screen->Settings;
        }

        if(Space->Settings.Mode == SpaceModeDefault)
            Space->Settings.Mode = KWMMode.Space;

        Space->Initialized = true;
        Space->NeedsUpdate = false;

        if(Space->Settings.Layout.empty() || Space->Settings.Mode != SpaceModeBSP)
            Space->RootNode = CreateTreeFromWindowIDList(Screen, Windows);
        else
            LoadBSPTreeFromFile(Screen, Space->Settings.Layout);
    }
    else if(Space->Initialized)
    {
        Space->FocusedWindowID = -1;

        if(Space->Settings.Layout.empty() || Space->Settings.Mode != SpaceModeBSP)
            Space->RootNode = CreateTreeFromWindowIDList(Screen, Windows);
        else
            LoadBSPTreeFromFile(Screen, Space->Settings.Layout);

        if(Space->RootNode)
        {
            if(Space->Settings.Mode == SpaceModeBSP)
            {
                SetRootNodeContainer(Screen, Space->RootNode);
                CreateNodeContainers(Screen, Space->RootNode, true);
            }
            else if(Space->Settings.Mode == SpaceModeMonocle)
            {
                link_node *Link = Space->RootNode->List;
                while(Link)
                {
                    SetLinkNodeContainer(Screen, Link);
                    Link = Link->Next;
                }
            }
        }
    }

    if(Space->RootNode)
    {
        ApplyTreeNodeContainer(Space->RootNode);
        FocusWindowBelowCursor();
    }
}

void ShouldWindowNodeTreeUpdate(screen_info *Screen)
{
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    if(Space->Settings.Mode == SpaceModeBSP)
        ShouldBSPTreeUpdate(Screen, Space);
    else if(Space->Settings.Mode == SpaceModeMonocle)
        ShouldMonocleTreeUpdate(Screen, Space);
}

void ShouldBSPTreeUpdate(ax_display *Display)
{
    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(Space->RootNode)
    {
        std::vector<int> WindowIDsInTree = GetAllWindowIDsInTree(Space);
        std::vector<uint32_t> WindowsToRemove = GetAllAXWindowIDsToRemoveFromTree(WindowIDsInTree);

        for(std::size_t WindowIndex = 0; WindowIndex < WindowsToRemove.size(); ++WindowIndex)
        {
            DEBUG("ShouldBSPTreeUpdate() Remove Window " << WindowsToRemove[WindowIndex]);
            RemoveWindowFromBSPTree(Display, WindowsToRemove[WindowIndex]);
        }
    }
}

void ShouldBSPTreeUpdate(screen_info *Screen, space_info *Space) { }

void AddWindowToBSPTree(ax_display *Display, int WindowID)
{
    space_info *Space = &WindowTree[Display->Space->Identifier];
    tree_node *RootNode = Space->RootNode;
    tree_node *CurrentNode = NULL;
    GetFirstLeafNode(RootNode, (void**)&CurrentNode);
    if(CurrentNode)
    {
        split_type SplitMode = KWMScreen.SplitMode == SPLIT_OPTIMAL ? GetOptimalSplitMode(CurrentNode) : KWMScreen.SplitMode;
        CreateLeafNodePair(Display, CurrentNode, CurrentNode->WindowID, WindowID, SplitMode);
        ApplyTreeNodeContainer(CurrentNode);
    }
}

void AddWindowToBSPTree(screen_info *Screen, int WindowID)
{
    if(!DoesSpaceExistInMapOfScreen(Screen))
        return;

    space_info *Space = GetActiveSpaceOfScreen(Screen);
    tree_node *RootNode = Space->RootNode;
    tree_node *CurrentNode = NULL;

    DEBUG("AddWindowToBSPTree() Create pair of leafs");
    window_info *InsertionPoint = !WindowsAreEqual(&KWMFocus.InsertionPoint, &KWMFocus.NULLWindowInfo) ? &KWMFocus.InsertionPoint : NULL;
    bool UseFocusedContainer = InsertionPoint &&
                               IsWindowOnActiveSpace(InsertionPoint->WID) &&
                               InsertionPoint->WID != WindowID;

    bool DoNotUseMarkedContainer = IsWindowFloating(KWMScreen.MarkedWindow.WID, NULL) ||
                                   (KWMScreen.MarkedWindow.WID == WindowID);

    if((KWMScreen.MarkedWindow.WID == -1 ||
       KWMScreen.MarkedWindow.WID == 0) &&
       UseFocusedContainer)
    {
        CurrentNode = GetTreeNodeFromWindowIDOrLinkNode(RootNode, InsertionPoint->WID);
    }
    else if(DoNotUseMarkedContainer ||
           ((KWMScreen.MarkedWindow.WID == -1 || KWMScreen.MarkedWindow.WID == 0) &&
           !UseFocusedContainer))
    {
        GetFirstLeafNode(RootNode, (void**)&CurrentNode);
    }
    else
    {
        CurrentNode = GetTreeNodeFromWindowIDOrLinkNode(RootNode, KWMScreen.MarkedWindow.WID);
        ClearMarkedWindow();
    }

    if(CurrentNode)
    {
        if(CurrentNode->Type == NodeTypeTree)
        {
            split_type SplitMode = KWMScreen.SplitMode == SPLIT_OPTIMAL ? GetOptimalSplitMode(CurrentNode) : KWMScreen.SplitMode;
            CreateLeafNodePair(Screen, CurrentNode, CurrentNode->WindowID, WindowID, SplitMode);
            ApplyTreeNodeContainer(CurrentNode);
        }
        else if(CurrentNode->Type == NodeTypeLink)
        {
            link_node *Link = CurrentNode->List;
            if(Link)
            {
                while(Link->Next)
                    Link = Link->Next;

                link_node *NewLink = CreateLinkNode();
                NewLink->Container = CurrentNode->Container;

                NewLink->WindowID = WindowID;
                Link->Next = NewLink;
                NewLink->Prev = Link;
                ResizeWindowToContainerSize(NewLink);
            }
            else
            {
                CurrentNode->List = CreateLinkNode();
                CurrentNode->List->Container = CurrentNode->Container;
                CurrentNode->List->WindowID = WindowID;
                ResizeWindowToContainerSize(CurrentNode->List);
            }
        }
    }
}

void AddWindowToBSPTree()
{
    if(!KWMScreen.Current)
        return;

    AddWindowToBSPTree(KWMScreen.Current, KWMFocus.Window->WID);
}

void RemoveWindowFromBSPTree(ax_display *Display, int WindowID)
{
    space_info *Space = &WindowTree[Display->Space->Identifier];
    tree_node *WindowNode = GetTreeNodeFromWindowID(Space->RootNode, WindowID);
    if(!WindowNode)
        return;

    tree_node *Parent = WindowNode->Parent;
    if(Parent && Parent->LeftChild && Parent->RightChild)
    {
        tree_node *AccessChild = IsRightChild(WindowNode) ? Parent->LeftChild : Parent->RightChild;
        tree_node *NewFocusNode = NULL;
        Parent->LeftChild = NULL;
        Parent->RightChild = NULL;

        Parent->WindowID = AccessChild->WindowID;
        Parent->Type = AccessChild->Type;
        Parent->List = AccessChild->List;

        if(AccessChild->LeftChild && AccessChild->RightChild)
        {
            Parent->LeftChild = AccessChild->LeftChild;
            Parent->LeftChild->Parent = Parent;

            Parent->RightChild = AccessChild->RightChild;
            Parent->RightChild->Parent = Parent;

            CreateNodeContainers(Display, Parent, true);
            NewFocusNode = IsLeafNode(Parent->LeftChild) ? Parent->LeftChild : Parent->RightChild;
            while(!IsLeafNode(NewFocusNode))
                NewFocusNode = IsLeafNode(Parent->LeftChild) ? Parent->LeftChild : Parent->RightChild;
        }

        ResizeLinkNodeContainers(Parent);
        ApplyTreeNodeContainer(Parent);
        free(AccessChild);
        free(WindowNode);
    }
    else if(!Parent)
    {
        free(Space->RootNode);
        Space->RootNode = NULL;
    }
}

void RemoveWindowFromBSPTree(screen_info *Screen, int WindowID, bool Center, bool UpdateFocus)
{
    if(!DoesSpaceExistInMapOfScreen(Screen))
        return;

    space_info *Space = GetActiveSpaceOfScreen(Screen);
    tree_node *WindowNode = GetTreeNodeFromWindowID(Space->RootNode, WindowID);
    if(!WindowNode)
    {
        link_node *Link = GetLinkNodeFromWindowID(Space->RootNode, WindowID);
        tree_node *Root = GetTreeNodeFromLink(Space->RootNode, Link);
        if(Link)
        {
            link_node *Prev = Link->Prev;
            link_node *Next = Link->Next;
            link_node *NewFocusNode = Prev;

            Link->Prev = NULL;
            Link->Next = NULL;

            if(Prev)
                Prev->Next = Next;

            if(!Prev)
                Root->List = Next;

            if(Next)
                Next->Prev = Prev;

            if(Link == Root->List)
                Root->List = NULL;

            if(UpdateFocus)
            {
                if(NewFocusNode)
                    SetWindowFocusByNode(NewFocusNode);
                else
                    SetWindowFocusByNode(Root);

                MoveCursorToCenterOfFocusedWindow();
            }

            free(Link);
        }

        return;
    }

    tree_node *Parent = WindowNode->Parent;
    if(Parent && Parent->LeftChild && Parent->RightChild)
    {
        tree_node *AccessChild = IsRightChild(WindowNode) ? Parent->LeftChild : Parent->RightChild;
        tree_node *NewFocusNode = NULL;
        Parent->LeftChild = NULL;
        Parent->RightChild = NULL;

        DEBUG("RemoveWindowFromBSPTree() Parent && LeftChild && RightChild");
        Parent->WindowID = AccessChild->WindowID;
        Parent->Type = AccessChild->Type;
        Parent->List = AccessChild->List;

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

        ResizeLinkNodeContainers(Parent);
        ApplyTreeNodeContainer(Parent);
        if(Center)
        {
            window_info *WindowInfo = GetWindowByID(WindowID);
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
        DEBUG("RemoveWindowFromBSPTree() !Parent");

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
    std::vector<int> WindowIDsInTree = GetAllWindowIDsInTree(Space);
    std::vector<window_info*> WindowsToAdd = GetAllWindowsNotInTree(WindowIDsInTree);
    std::vector<int> WindowsToRemove = GetAllWindowIDsToRemoveFromTree(WindowIDsInTree);

    for(std::size_t WindowIndex = 0; WindowIndex < WindowsToRemove.size(); ++WindowIndex)
    {
        DEBUG("ShouldBSPTreeUpdate() Remove Window " << WindowsToRemove[WindowIndex]);
        RemoveWindowFromMonocleTree(Screen, WindowsToRemove[WindowIndex], false, true);
    }

    for(std::size_t WindowIndex = 0; WindowIndex < WindowsToAdd.size(); ++WindowIndex)
    {
        if(IsWindowTilable(WindowsToAdd[WindowIndex]) &&
           !IsWindowFloating(WindowsToAdd[WindowIndex]->WID, NULL))
        {
            DEBUG("ShouldMonocleTreeUpdate() Add Window");
            AddWindowToMonocleTree(Screen, WindowsToAdd[WindowIndex]->WID);
            SetWindowFocus(WindowsToAdd[WindowIndex]);
            MoveCursorToCenterOfFocusedWindow();
        }
    }
}

void AddWindowToMonocleTree(screen_info *Screen, int WindowID)
{
    if(!DoesSpaceExistInMapOfScreen(Screen))
        return;

    space_info *Space = GetActiveSpaceOfScreen(Screen);
    link_node *Link = Space->RootNode->List;
    while(Link->Next)
        Link = Link->Next;

    link_node *NewLink = CreateLinkNode();
    SetLinkNodeContainer(Screen, NewLink);

    NewLink->WindowID = WindowID;
    Link->Next = NewLink;
    NewLink->Prev = Link;

    ResizeWindowToContainerSize(NewLink);
}

void RemoveWindowFromMonocleTree(screen_info *Screen, int WindowID, bool Center, bool UpdateFocus)
{
    if(!DoesSpaceExistInMapOfScreen(Screen))
        return;

    space_info *Space = GetActiveSpaceOfScreen(Screen);
    link_node *Link = GetLinkNodeFromTree(Space->RootNode, WindowID);

    if(Link)
    {
        link_node *Prev = Link->Prev;
        link_node *Next = Link->Next;
        link_node *NewFocusNode = Prev;

        if(Prev)
            Prev->Next = Next;

        if(Next)
            Next->Prev = Prev;

        if(Link == Space->RootNode->List)
        {
            free(Link);
            Space->RootNode->List = Next;
            NewFocusNode = Next;

            if(!Space->RootNode->List)
            {
                free(Space->RootNode);
                Space->RootNode = NULL;
                Space->FocusedWindowID = -1;
            }
        }
        else
        {
            free(Link);
        }

        if(Center)
        {
            window_info *WindowInfo = GetWindowByID(WindowID);
            if(WindowInfo)
                CenterWindow(Screen, WindowInfo);
        }

        if(UpdateFocus && NewFocusNode)
        {
            SetWindowFocusByNode(NewFocusNode);
            MoveCursorToCenterOfFocusedWindow();
        }
    }
}

void AddWindowToTreeOfUnfocusedMonitor(screen_info *Screen, window_info *Window)
{
    screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
    if(!Screen || !Window || Screen == ScreenOfWindow)
        return;

    space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
    if(SpaceOfWindow->Settings.Mode == SpaceModeBSP)
        RemoveWindowFromBSPTree(ScreenOfWindow, Window->WID, false, false);
    else if(SpaceOfWindow->Settings.Mode == SpaceModeMonocle)
        RemoveWindowFromMonocleTree(ScreenOfWindow, Window->WID, false, false);

    if(Window->WID == KWMScreen.MarkedWindow.WID)
        ClearMarkedWindow();

    bool UpdateFocus = true;
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    if(Space->RootNode)
    {
        if(Space->Settings.Mode == SpaceModeBSP)
        {
            DEBUG("AddWindowToTreeOfUnfocusedMonitor() BSP Space");
            tree_node *CurrentNode = NULL;
            GetFirstLeafNode(Space->RootNode, (void**)&CurrentNode);
            split_type SplitMode = KWMScreen.SplitMode == SPLIT_OPTIMAL ? GetOptimalSplitMode(CurrentNode) : KWMScreen.SplitMode;

            CreateLeafNodePair(Screen, CurrentNode, CurrentNode->WindowID, Window->WID, SplitMode);
            ApplyTreeNodeContainer(CurrentNode);
        }
        else if(Space->Settings.Mode == SpaceModeMonocle)
        {
            DEBUG("AddWindowToTreeOfUnfocusedMonitor() Monocle Space");
            link_node *Link = Space->RootNode->List;
            while(Link->Next)
                Link = Link->Next;

            link_node *NewLink = CreateLinkNode();
            SetLinkNodeContainer(Screen, NewLink);

            NewLink->WindowID = Window->WID;
            Link->Next = NewLink;
            NewLink->Prev = Link;
            ResizeWindowToContainerSize(NewLink);
        }
    }
    else
    {
        std::vector<window_info*> Windows;
        Windows.push_back(Window);
        CreateWindowNodeTree(Screen, &Windows);
        UpdateFocus = false;
    }

    if(UpdateFocus)
    {
        GiveFocusToScreen(Screen->ID, NULL, false, false);
        SetWindowFocus(Window);
        MoveCursorToCenterOfFocusedWindow();
    }
}

void ToggleWindowFloating(int WindowID, bool Center)
{
    Assert(KWMScreen.Current);

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    if(IsWindowOnActiveSpace(WindowID))
    {
        int WindowIndex;
        if(IsWindowFloating(WindowID, &WindowIndex))
        {
            KWMTiling.FloatingWindowLst.erase(KWMTiling.FloatingWindowLst.begin() + WindowIndex);
            if(Space->Settings.Mode == SpaceModeBSP)
                AddWindowToBSPTree(KWMScreen.Current, WindowID);
            else if(Space->Settings.Mode == SpaceModeMonocle)
                AddWindowToMonocleTree(KWMScreen.Current, WindowID);

            if(KWMMode.Focus != FocusModeDisabled && KWMToggles.StandbyOnFloat)
                KWMMode.Focus = FocusModeAutoraise;
        }
        else
        {
            KWMTiling.FloatingWindowLst.push_back(WindowID);
            if(Space->Settings.Mode == SpaceModeBSP)
                RemoveWindowFromBSPTree(KWMScreen.Current, WindowID, Center, false);
            else if(Space->Settings.Mode == SpaceModeMonocle)
                RemoveWindowFromMonocleTree(KWMScreen.Current, WindowID, Center, false);

            if(KWMMode.Focus != FocusModeDisabled && KWMToggles.StandbyOnFloat)
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
    if(Space->Settings.Mode != SpaceModeBSP)
        return;

    tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID);
    if(Node && Node->Parent)
    {
        if(IsLeafNode(Node) && Node->Parent->WindowID == -1)
        {
            DEBUG("ToggleFocusedWindowParentContainer() Set Parent Container");
            Node->Parent->WindowID = Node->WindowID;
            ResizeWindowToContainerSize(Node->Parent);
        }
        else
        {
            DEBUG("ToggleFocusedWindowParentContainer() Restore Window Container");
            Node->Parent->WindowID = -1;
            ResizeWindowToContainerSize(Node);
        }

        if(KWMTiling.LockToContainer)
        {
            UpdateBorder("focused");
        }
    }
}

void ToggleFocusedWindowFullscreen()
{
    if(!KWMFocus.Window || !DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    if(Space->Settings.Mode == SpaceModeBSP && !IsLeafNode(Space->RootNode))
    {
        tree_node *Node = NULL;
        if(Space->RootNode->WindowID == -1)
        {
            Node = GetTreeNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID);
            if(Node)
            {
                DEBUG("ToggleFocusedWindowFullscreen() Set fullscreen");
                Space->RootNode->WindowID = Node->WindowID;
                ResizeWindowToContainerSize(Space->RootNode);
            }
        }
        else
        {
            DEBUG("ToggleFocusedWindowFullscreen() Restore old size");
            Space->RootNode->WindowID = -1;

            Node = GetTreeNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID);
            if(Node)
            {
                ResizeWindowToContainerSize(Node);
            }
        }

        if(KWMTiling.LockToContainer)
        {
            UpdateBorder("focused");
        }
    }
}

bool IsWindowFullscreen(window_info *Window)
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    return Space->RootNode && Space->RootNode->WindowID == Window->WID;
}

bool IsWindowParentContainer(window_info *Window)
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, Window->WID);
    return Node && Node->Parent && Node->Parent->WindowID == Window->WID;
}

void LockWindowToContainerSize(window_info *Window)
{
    if(Window)
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, Window->WID);

        if(IsWindowFullscreen(Window))
            ResizeWindowToContainerSize(Space->RootNode);
        else if(IsWindowParentContainer(Window))
            ResizeWindowToContainerSize(Node->Parent);
        else
            ResizeWindowToContainerSize();
    }
}

void DetachAndReinsertWindow(int WindowID, int Degrees)
{
    if(WindowID == KWMScreen.MarkedWindow.WID)
    {
        int Marked = KWMScreen.MarkedWindow.WID;
        if(Marked == -1 || (KWMFocus.Window && Marked == KWMFocus.Window->WID))
            return;

        ToggleWindowFloating(Marked, false);
        ClearMarkedWindow();
        ToggleWindowFloating(Marked, false);
        MoveCursorToCenterOfFocusedWindow();
    }
    else
    {
        if(WindowID == KWMScreen.MarkedWindow.WID ||
           WindowID == -1)
            return;

        window_info InsertWindow = {};
        if(FindClosestWindow(Degrees, &InsertWindow, false))
        {
            ToggleWindowFloating(WindowID, false);
            KWMScreen.MarkedWindow = InsertWindow;
            ToggleWindowFloating(WindowID, false);
            MoveCursorToCenterOfFocusedWindow();
        }
    }
}

void SwapFocusedWindowWithMarked()
{
    if(!KWMFocus.Window || KWMScreen.MarkedWindow.WID == KWMFocus.Window->WID ||
       KWMScreen.MarkedWindow.WID == -1 || KWMScreen.MarkedWindow.WID == 0)
        return;

    if(DoesSpaceExistInMapOfScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        tree_node *TreeNode = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, KWMFocus.Window->WID);

        if(TreeNode)
        {
            tree_node *NewFocusNode = GetTreeNodeFromWindowID(Space->RootNode, KWMScreen.MarkedWindow.WID);
            if(NewFocusNode)
            {
                SwapNodeWindowIDs(TreeNode, NewFocusNode);
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
    if(Space->Settings.Mode == SpaceModeMonocle)
    {
        link_node *Link = GetLinkNodeFromTree(Space->RootNode, KWMFocus.Window->WID);
        if(Link)
        {
            link_node *ShiftNode = Shift == 1 ? Link->Next : Link->Prev;
            if(KWMMode.Cycle == CycleModeScreen && !ShiftNode)
            {
                Space->RootNode->Type = NodeTypeLink;
                if(Shift == 1)
                    GetFirstLeafNode(Space->RootNode, (void**)&ShiftNode);
                else
                    GetLastLeafNode(Space->RootNode, (void**)&ShiftNode);
                Space->RootNode->Type = NodeTypeTree;
            }

            if(ShiftNode)
            {
                SwapNodeWindowIDs(Link, ShiftNode);
                MoveCursorToCenterOfFocusedWindow();
            }
        }
    }
    else if(Space->Settings.Mode == SpaceModeBSP)
    {
        tree_node *TreeNode = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, KWMFocus.Window->WID);
        if(TreeNode)
        {
            tree_node *NewFocusNode = NULL;;

            if(Shift == 1)
                NewFocusNode = GetNearestTreeNodeToTheRight(TreeNode);
            else if(Shift == -1)
                NewFocusNode = GetNearestTreeNodeToTheLeft(TreeNode);

            if(NewFocusNode)
            {
                SwapNodeWindowIDs(TreeNode, NewFocusNode);
                MoveCursorToCenterOfFocusedWindow();
            }
        }
    }
}

void SwapFocusedWindowDirected(int Degrees)
{
    if(!KWMFocus.Window || !DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    if(Space->Settings.Mode == SpaceModeMonocle)
    {
        if(Degrees == 90)
            SwapFocusedWindowWithNearest(1);
        else if(Degrees == 270)
            SwapFocusedWindowWithNearest(-1);
    }
    else if(Space->Settings.Mode == SpaceModeBSP)
    {
        tree_node *TreeNode = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, KWMFocus.Window->WID);
        if(TreeNode)
        {
            tree_node *NewFocusNode = NULL;
            window_info SwapWindow = {};
            if(FindClosestWindow(Degrees, &SwapWindow, KWMMode.Cycle == CycleModeScreen))
                NewFocusNode = GetTreeNodeFromWindowID(Space->RootNode, SwapWindow.WID);

            if(NewFocusNode)
            {
                SwapNodeWindowIDs(TreeNode, NewFocusNode);
                MoveCursorToCenterOfFocusedWindow();
            }
        }
    }
}

bool WindowIsInDirection(window_info *WindowA, window_info *WindowB, int Degrees)
{
    Assert(KWMScreen.Current);
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    tree_node *NodeA = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, WindowA->WID);
    tree_node *NodeB = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, WindowB->WID);

    if(!NodeA || !NodeB || NodeA == NodeB)
        return false;

    node_container *A = &NodeA->Container;
    node_container *B = &NodeB->Container;

    if(Degrees == 0 || Degrees == 180)
        return A->Y != B->Y && fmax(A->X, B->X) < fmin(B->X + B->Width, A->X + A->Width);
    else if(Degrees == 90 || Degrees == 270)
        return A->X != B->X && fmax(A->Y, B->Y) < fmin(B->Y + B->Height, A->Y + A->Height);

    return false;
}

void GetCenterOfWindow(window_info *Window, int *X, int *Y)
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    tree_node *Node = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, Window->WID);
    if(Node)
    {
        *X = Node->Container.X + Node->Container.Width / 2;
        *Y = Node->Container.Y + Node->Container.Height / 2;
    }
    else
    {
        *X = -1;
        *Y = -1;
    }
}

double GetWindowDistance(window_info *A, window_info *B, int Degrees, bool Wrap)
{
    double Rank = INT_MAX;

    int X1, Y1, X2, Y2;
    GetCenterOfWindow(A, &X1, &Y1);
    GetCenterOfWindow(B, &X2, &Y2);

    if(Wrap)
    {
        if(Degrees == 0 && Y1 < Y2)
            Y2 -= KWMScreen.Current->Height;
        else if(Degrees == 180 && Y1 > Y2)
            Y2 += KWMScreen.Current->Height;
        else if(Degrees == 90 && X1 > X2)
            X2 += KWMScreen.Current->Width;
        else if(Degrees == 270 && X1 < X2)
            X2 -= KWMScreen.Current->Width;
    }

    double DeltaX = X2 - X1;
    double DeltaY = Y2 - Y1;
    double Angle = std::atan2(DeltaY, DeltaX);
    double Distance = std::hypot(DeltaX, DeltaY);
    double DeltaA = 0;

    if((Degrees == 0 && DeltaY >= 0) ||
       (Degrees == 90 && DeltaX <= 0) ||
       (Degrees == 180 && DeltaY <= 0) ||
       (Degrees == 270 && DeltaX >= 0))
        return INT_MAX;

    if(Degrees == 0)
        DeltaA = -M_PI_2 - Angle;
    else if(Degrees == 180)
        DeltaA = M_PI_2 - Angle;
    else if(Degrees == 90)
        DeltaA = 0.0 - Angle;
    else if(Degrees == 270)
        DeltaA = M_PI - std::fabs(Angle);

    Rank = Distance / std::cos(DeltaA / 2.0);
    return Rank;
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
           WindowIsInDirection(Match, &Windows[Index], Degrees) &&
           !IsWindowFloating(Windows[Index].WID, NULL))
        {
            double Dist = GetWindowDistance(Match, &Windows[Index], Degrees, Wrap);
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
    if(Space->Settings.Mode == SpaceModeBSP)
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
    }
    else if(Space->Settings.Mode == SpaceModeMonocle)
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
    if(Space->Settings.Mode == SpaceModeMonocle)
    {
        link_node *Link = GetLinkNodeFromTree(Space->RootNode, KWMFocus.Window->WID);
        if(Link)
        {
            link_node *FocusNode = Shift == 1 ? Link->Next : Link->Prev;
            if(KWMMode.Cycle == CycleModeScreen && !FocusNode)
            {
                Space->RootNode->Type = NodeTypeLink;
                if(Shift == 1)
                    GetFirstLeafNode(Space->RootNode, (void**)&FocusNode);
                else
                    GetLastLeafNode(Space->RootNode, (void**)&FocusNode);
                Space->RootNode->Type = NodeTypeTree;
            }

            if(FocusNode)
            {
                SetWindowFocusByNode(FocusNode);
                MoveCursorToCenterOfFocusedWindow();
            }
        }
    }
    else if(Space->Settings.Mode == SpaceModeBSP)
    {
        tree_node *TreeNode = GetTreeNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID);
        if(TreeNode)
        {
            tree_node *FocusNode = NULL;

            if(Shift == 1)
            {
                FocusNode = GetNearestTreeNodeToTheRight(TreeNode);
                while(IsPseudoNode(FocusNode))
                    FocusNode = GetNearestTreeNodeToTheRight(FocusNode);

                if(KWMMode.Cycle == CycleModeScreen && !FocusNode)
                {
                    GetFirstLeafNode(Space->RootNode, (void**)&FocusNode);
                    while(IsPseudoNode(FocusNode))
                        FocusNode = GetNearestTreeNodeToTheRight(FocusNode);
                }
            }
            else if(Shift == -1)
            {
                FocusNode = GetNearestTreeNodeToTheLeft(TreeNode);
                while(IsPseudoNode(FocusNode))
                    FocusNode = GetNearestTreeNodeToTheLeft(FocusNode);

                if(KWMMode.Cycle == CycleModeScreen && !FocusNode)
                {
                    GetLastLeafNode(Space->RootNode, (void**)&FocusNode);
                    while(IsPseudoNode(FocusNode))
                        FocusNode = GetNearestTreeNodeToTheLeft(FocusNode);
                }
            }

            SetWindowFocusByNode(FocusNode);
            MoveCursorToCenterOfFocusedWindow();
        }
    }
}

void ShiftSubTreeWindowFocus(int Shift)
{
    if(!KWMFocus.Window || !DoesSpaceExistInMapOfScreen(KWMScreen.Current))
        return;

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    if(Space->Settings.Mode == SpaceModeBSP)
    {
        link_node *Link = GetLinkNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID);
        tree_node *Root = GetTreeNodeFromLink(Space->RootNode, Link);
        if(Link)
        {
            link_node *FocusNode = NULL;
            if(Shift == 1)
            {
                FocusNode = Link->Next;
                SetWindowFocusByNode(FocusNode);
            }
            else
            {
                FocusNode = Link->Prev;
                if(FocusNode)
                    SetWindowFocusByNode(FocusNode);
                else
                    SetWindowFocusByNode(Root);
            }

            MoveCursorToCenterOfFocusedWindow();
        }
        else if(Shift == 1)
        {
            tree_node *Root = GetTreeNodeFromWindowID(Space->RootNode, KWMFocus.Window->WID);
            if(Root)
            {
                SetWindowFocusByNode(Root->List);
                MoveCursorToCenterOfFocusedWindow();
            }
        }
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
            tree_node *Node = GetTreeNodeFromWindowID(Root, WindowID);
            if(Node)
                GiveFocusToScreen(Screen->ID, Node, false, true);
        }
    }
}

void MoveCursorToCenterOfWindow(window_info *Window)
{
    Assert(Window);
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
    {
        CGPoint WindowPos = AXLibGetWindowPosition(WindowRef);
        CGSize WindowSize = AXLibGetWindowSize(WindowRef);
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
    std::memset(&KWMScreen.MarkedWindow, 0, sizeof(window_info));
    ClearBorder(&MarkedBorder);
}

void MarkWindowContainer(window_info *Window)
{
    if(Window)
    {
        if(KWMScreen.MarkedWindow.WID == Window->WID)
        {
            DEBUG("MarkWindowContainer() Unmarked " << Window->Name);
            ClearMarkedWindow();
        }
        else
        {
            DEBUG("MarkWindowContainer() Marked " << Window->Name);
            KWMScreen.MarkedWindow = *Window;
            UpdateBorder("marked");
        }
    }
}

void MarkFocusedWindowContainer()
{
    MarkWindowContainer(KWMFocus.Window);
}

void SetKwmFocus(AXUIElementRef WindowRef)
{
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

    UpdateBorder("focused");
    if(KWMToggles.EnableTilingMode)
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        Space->FocusedWindowID = KWMFocus.Window->WID;
    }

    if(KWMMode.Focus != FocusModeDisabled &&
       KWMToggles.StandbyOnFloat)
        KWMMode.Focus = IsFocusedWindowFloating() ? FocusModeStandby : FocusModeAutoraise;
}

void SetWindowRefFocus(AXUIElementRef WindowRef)
{
    KWMFocus.Cache = GetWindowByRef(WindowRef);
    if(WindowsAreEqual(&KWMFocus.Cache, &KWMFocus.NULLWindowInfo))
    {
        KWMFocus.Window = NULL;
        ClearBorder(&FocusedBorder);
        return;
    }

    KWMFocus.Window = &KWMFocus.Cache;
    KWMFocus.InsertionPoint = KWMFocus.Cache;

    ProcessSerialNumber NewPSN;
    GetProcessForPID(KWMFocus.Window->PID, &NewPSN);
    KWMFocus.PSN = NewPSN;

    AXUIElementSetAttributeValue(WindowRef, kAXMainAttribute, kCFBooleanTrue);
    AXUIElementSetAttributeValue(WindowRef, kAXFocusedAttribute, kCFBooleanTrue);
    AXUIElementPerformAction(WindowRef, kAXRaiseAction);

    if(KWMMode.Focus != FocusModeStandby)
        SetFrontProcessWithOptions(&KWMFocus.PSN, kSetFrontProcessFrontWindowOnly);

    UpdateBorder("focused");
    if(KWMToggles.EnableTilingMode)
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        Space->FocusedWindowID = KWMFocus.Window->WID;
    }

    DEBUG("SetWindowRefFocus() Focused Window: " << KWMFocus.Window->Name << " " << KWMFocus.Window->X << "," << KWMFocus.Window->Y);
    if(KWMMode.Focus != FocusModeDisabled &&
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
            DEBUG("SetWindowFocusByNode()");
            SetWindowFocus(Window);
        }
    }
}

void SetWindowFocusByNode(link_node *Link)
{
    if(Link)
    {
        window_info *Window = GetWindowByID(Link->WindowID);
        if(Window)
        {
            DEBUG("SetWindowFocusByNode()");
            SetWindowFocus(Window);
        }
    }
}

void CenterWindowInsideNodeContainer(AXUIElementRef WindowRef, int *Xptr, int *Yptr, int *Wptr, int *Hptr)
{
    CGPoint WindowOrigin = AXLibGetWindowPosition(WindowRef);
    CGSize WindowOGSize = AXLibGetWindowSize(WindowRef);

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

        AXLibSetWindowPosition(WindowRef, X, Y);
        AXLibSetWindowSize(WindowRef, Width, Height);
    }
}

void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height)
{
    AXLibSetWindowPosition(WindowRef, X, Y);
    AXLibSetWindowSize(WindowRef, Width, Height);
    CenterWindowInsideNodeContainer(WindowRef, &X, &Y, &Width, &Height);
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
    if(!KWMFocus.Window || !IsWindowFloating(KWMFocus.Window->WID, NULL))
        return;

    AXUIElementRef WindowRef;
    if(GetWindowRef(KWMFocus.Window, &WindowRef))
    {
        CGPoint WindowPos = AXLibGetWindowPosition(WindowRef);
        WindowPos.x += X;
        WindowPos.y += Y;
        AXLibSetWindowPosition(WindowRef, WindowPos.x, WindowPos.y);
    }
}

bool IsWindowTilable(window_info *Window)
{
    bool Result = true;
    if(KWMTiling.FloatNonResizable)
    {
        AXUIElementRef WindowRef;
        if(GetWindowRef(Window, &WindowRef))
            Result = IsWindowTilable(WindowRef);

        if(!Result && !IsWindowFloating(Window->WID, NULL))
            KWMTiling.FloatingWindowLst.push_back(Window->WID);
    }

    return Result;
}

bool IsWindowTilable(AXUIElementRef WindowRef)
{

    return AXLibIsWindowResizable(WindowRef) && AXLibIsWindowMovable(WindowRef);
}

CGPoint GetWindowPos(window_info *Window)
{
    CGPoint Result = {};
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
        Result = AXLibGetWindowPosition(WindowRef);

    return Result;
}

window_info GetWindowByRef(AXUIElementRef WindowRef)
{
    Assert(WindowRef);
    window_info *Window = GetWindowByID((int)AXLibGetWindowID(WindowRef));
    return Window ? *Window : KWMFocus.NULLWindowInfo;
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

ax_window *GetWindowByID(unsigned int WindowID)
{
    std::map<pid_t, ax_application>::iterator It;
    for(It = AXState.Applications.begin();
        It != AXState.Applications.end();
        ++It)
    {
        ax_application *Application = &It->second;
        ax_window *Window = AXLibFindApplicationWindow(Application, WindowID);
        if(Window)
            return Window;
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
            AXLibGetWindowRole(WindowRef, Role);
            AXLibGetWindowSubrole(WindowRef, SubRole);
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
        DEBUG("GetWindowRef() Failed to get App for: " << Window->Name);
        return false;
    }

    CFArrayRef AppWindowLst;
    AXUIElementCopyAttributeValue(App, kAXWindowsAttribute, (CFTypeRef*)&AppWindowLst);
    if(!AppWindowLst)
    {
        DEBUG("GetWindowRef() Could not get AppWindowLst");
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
                if(AXLibGetWindowID(AppWindowRef) == Window->WID)
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
