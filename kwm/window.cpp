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
        RebalanceNodeTree(FocusedDisplay);
    }
    else
    {
        printf("What the fuck.. Display was null (?)");
    }
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
        ApplyWindowRules(Application->Focus);
        ax_display *Display = AXLibWindowDisplay(Application->Focus);
        if(Display)
        {
            if((!AXLibHasFlags(Application->Focus, AXWindow_Minimized)) &&
               (AXLibIsWindowStandard(Application->Focus) || AXLibIsWindowCustom(Application->Focus)) &&
               (!AXLibHasFlags(Application->Focus, AXWindow_Floating)))
            {
                AddWindowToNodeTree(Display, Application->Focus->ID);
            }
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
        RebalanceNodeTree(Display);

    ClearBorder(&FocusedBorder);
}

EVENT_CALLBACK(Callback_AXEvent_ApplicationActivated)
{
    ax_application *Application = (ax_application *) Event->Context;
    ax_application *OldFocusedApplication = FocusedApplication;

    FocusedApplication = Application;
    FocusedApplication->Focus = AXLibGetFocusedWindow(FocusedApplication);

    /* NOTE(koekeishiya): When an application that is already running, but has no open windows, is activated,
                          or a window is deminimized, we receive 'didApplicationActivate' notification first.
                          We have to preserve our insertion point and flag this application for activation at a later point in time. */

    if((!FocusedApplication->Focus) ||
       (AXLibHasFlags(FocusedApplication->Focus, AXWindow_Minimized)))
    {
        FocusedApplication = OldFocusedApplication;
        AXLibAddFlags(Application, AXApplication_Activate);
    }
    else
    {
        DEBUG("AXEvent_ApplicationActivated: " << Application->Name);
        UpdateBorder("focused");
    }
}

EVENT_CALLBACK(Callback_AXEvent_WindowCreated)
{
    ax_window *Window = (ax_window *) Event->Context;
    if(Window && AXLibFindApplicationWindow(Window->Application, Window->ID))
    {
        DEBUG("AXEvent_WindowCreated: " << Window->Application->Name << " - " << Window->Name);
        ApplyWindowRules(Window);
        ax_display *Display = AXLibWindowDisplay(Window);
        if(Display)
        {
            if((!AXLibHasFlags(Window, AXWindow_Minimized)) &&
               (AXLibIsWindowStandard(Window) || AXLibIsWindowCustom(Window)) &&
               (!AXLibHasFlags(Window, AXWindow_Floating)))
            {
                AddWindowToNodeTree(Display, Window->ID);
            }
        }
    }
}

EVENT_CALLBACK(Callback_AXEvent_WindowDestroyed)
{
    ax_window *Window = (ax_window *) Event->Context;
    DEBUG("AXEvent_WindowDestroyed: " << Window->Application->Name << " - " << Window->Name);
    ax_display *Display = AXLibWindowDisplay(Window);
    if(Display)
        RemoveWindowFromNodeTree(Display, Window->ID);

    UpdateBorder("focused");
    AXLibDestroyWindow(Window);
}

EVENT_CALLBACK(Callback_AXEvent_WindowMinimized)
{
    ax_window *Window = (ax_window *) Event->Context;
    DEBUG("AXEvent_WindowMinimized: " << Window->Application->Name << " - " << Window->Name);
    AXLibAddFlags(Window, AXWindow_Minimized);

    ax_display *Display = AXLibWindowDisplay(Window);
    if(Display)
        RemoveWindowFromNodeTree(Display, Window->ID);

    ClearBorder(&FocusedBorder);
}

EVENT_CALLBACK(Callback_AXEvent_WindowDeminimized)
{
    ax_window *Window = (ax_window *) Event->Context;
    DEBUG("AXEvent_WindowDeminimized: " << Window->Application->Name << " - " << Window->Name);

    ax_display *Display = AXLibWindowDisplay(Window);
    if(Display)
    {
        if((!AXLibHasFlags(Window, AXWindow_Minimized)) &&
           (AXLibIsWindowStandard(Window) || AXLibIsWindowCustom(Window)) &&
           (!AXLibHasFlags(Window, AXWindow_Floating)))
        {
            AddWindowToNodeTree(Display, Window->ID);
        }
    }

    /* NOTE(koekeishiya): If a window was minimized, it should now be the focused window for its application,
                          and the corresponding application will have input focus. */
    if(AXLibHasFlags(Window, AXWindow_Minimized))
    {
        AXLibClearFlags(Window, AXWindow_Minimized);
        FocusedApplication = Window->Application;
        FocusedApplication->Focus = Window;
        UpdateBorder("focused");
    }
}

EVENT_CALLBACK(Callback_AXEvent_WindowFocused)
{
    ax_window *Window = (ax_window *) Event->Context;

    DEBUG("AXEvent_WindowFocused: " << Window->Application->Name << " - " << Window->Name);

    /* NOTE(koekeishiya): When a window is deminimized, we receive a FocusedWindowChanged notification before the
                          window is visible. Only set the focused window for the corresponding application when
                          we know that we can interact with the window in question, so we can preserve our insertion point. */
    if(!AXLibHasFlags(Window, AXWindow_Minimized))
        Window->Application->Focus = Window;

    /* NOTE(koekeishiya): If the application corresponding to this window is flagged for activation and
                          the window is visible to the user, this should be our focused application. */
    if(AXLibHasFlags(Window->Application, AXApplication_Activate))
    {
        AXLibClearFlags(Window->Application, AXApplication_Activate);
        if(!AXLibHasFlags(Window, AXWindow_Minimized))
            FocusedApplication = Window->Application;
    }

    if(FocusedApplication == Window->Application)
        UpdateBorder("focused");
}


EVENT_CALLBACK(Callback_AXEvent_WindowMoved)
{
    ax_window *Window = (ax_window *) Event->Context;
    if(Window && AXLibFindApplicationWindow(Window->Application, Window->ID))
    {
        DEBUG("AXEvent_WindowMoved: " << Window->Application->Name << " - " << Window->Name);
        UpdateBorder("focused");
    }
}

EVENT_CALLBACK(Callback_AXEvent_WindowResized)
{
    ax_window *Window = (ax_window *) Event->Context;
    if(Window && AXLibFindApplicationWindow(Window->Application, Window->ID))
    {
        DEBUG("AXEvent_WindowResized: " << Window->Application->Name << " - " << Window->Name);
        UpdateBorder("focused");
    }
}

bool WindowsAreEqual(window_info *Window, window_info *Match) { }

std::vector<window_info> FilterWindowListAllDisplays() { }

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

std::vector<int> GetAllWindowIDsToRemoveFromTree(std::vector<int> &WindowIDsInTree) { }

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
/* TODO(koekeishiya): Fix how these settings are stored. */
internal void
LoadSpaceSettings(ax_display *Display, space_info *SpaceInfo)
{
    int DesktopID = AXLibDesktopIDFromCGSSpaceID(Display, Display->Space->ID);

    /* NOTE(koekeishiya): Load global default display settings. */
    SpaceInfo->Settings.Offset = KWMScreen.DefaultOffset;
    SpaceInfo->Settings.Mode = SpaceModeDefault;
    SpaceInfo->Settings.Layout = "";
    SpaceInfo->Settings.Name = "";

    /* NOTE(koekeishiya): The space in question may have overloaded settings. */
    space_settings *SpaceSettings = NULL;
    if((SpaceSettings = GetSpaceSettingsForDesktopID(Display->ArrangementID, DesktopID)))
        SpaceInfo->Settings = *SpaceSettings;
    else if((SpaceSettings = GetSpaceSettingsForDisplay(Display->ArrangementID)))
        SpaceInfo->Settings = *SpaceSettings;

    /* TODO(koekeishiya): Is SpaceModeDefault necessary (?) */
    if(SpaceInfo->Settings.Mode == SpaceModeDefault)
        SpaceInfo->Settings.Mode = KWMMode.Space;
}

void CreateWindowNodeTree(ax_display *Display)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(!SpaceInfo->Initialized && !SpaceInfo->RootNode)
    {
        SpaceInfo->Initialized = true;
        LoadSpaceSettings(Display, SpaceInfo);
        if(SpaceInfo->Settings.Mode == SpaceModeFloating)
            return;

        std::vector<uint32_t> Windows = GetAllWindowIDSOnDisplay(Display);
        if(SpaceInfo->Settings.Mode == SpaceModeBSP && !SpaceInfo->Settings.Layout.empty())
        {
            LoadBSPTreeFromFile(Display, SpaceInfo, SpaceInfo->Settings.Layout);
            FillDeserializedTree(SpaceInfo->RootNode, &Windows);
        }
        else
        {
            SpaceInfo->RootNode = CreateTreeFromWindowIDList(Display, &Windows);
        }
    }
    else if(SpaceInfo->Initialized && !SpaceInfo->RootNode)
    {
        if(SpaceInfo->Settings.Mode == SpaceModeFloating)
            return;

        /* NOTE(koekeishiya): If a space has been initialized, but the node-tree was destroyed. */
        std::vector<uint32_t> Windows = GetAllWindowIDSOnDisplay(Display);
        if(SpaceInfo->Settings.Mode == SpaceModeBSP && !SpaceInfo->Settings.Layout.empty())
        {
            LoadBSPTreeFromFile(Display, SpaceInfo, SpaceInfo->Settings.Layout);
            FillDeserializedTree(SpaceInfo->RootNode, &Windows);
        }
        else
        {
            SpaceInfo->RootNode = CreateTreeFromWindowIDList(Display, &Windows);
        }

        /* NOTE(koekeishiya): Is this something that we really need to do (?) */
        if(SpaceInfo->RootNode)
        {
            if(SpaceInfo->Settings.Mode == SpaceModeBSP)
            {
                SetRootNodeContainer(Display, SpaceInfo->RootNode);
                CreateNodeContainers(Display, SpaceInfo->RootNode, true);
            }
            else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
            {
                SetRootNodeContainer(Display, SpaceInfo->RootNode);
                link_node *Link = SpaceInfo->RootNode->List;
                while(Link)
                {
                    SetLinkNodeContainer(Display, Link);
                    Link = Link->Next;
                }
            }
        }
    }

    if(SpaceInfo->RootNode)
        ApplyTreeNodeContainer(SpaceInfo->RootNode);
}

void RebalanceNodeTree(ax_display *Display)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->Settings.Mode == SpaceModeBSP)
        RebalanceBSPTree(Display);
    else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
        RebalanceMonocleTree(Display);
}

void RebalanceBSPTree(ax_display *Display)
{
    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(Space->RootNode)
    {
        std::vector<int> WindowIDsInTree = GetAllWindowIDsInTree(Space);
        std::vector<uint32_t> WindowsToRemove = GetAllAXWindowIDsToRemoveFromTree(WindowIDsInTree);

        for(std::size_t WindowIndex = 0; WindowIndex < WindowsToRemove.size(); ++WindowIndex)
        {
            DEBUG("RebalanceBSPTree() Remove Window " << WindowsToRemove[WindowIndex]);
            RemoveWindowFromBSPTree(Display, WindowsToRemove[WindowIndex]);
        }
    }
}

void RebalanceMonocleTree(ax_display *Display)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->RootNode && SpaceInfo->RootNode->List)
    {
        std::vector<int> WindowIDsInTree = GetAllWindowIDsInTree(SpaceInfo);
        std::vector<uint32_t> WindowsToRemove = GetAllAXWindowIDsToRemoveFromTree(WindowIDsInTree);

        for(std::size_t WindowIndex = 0; WindowIndex < WindowsToRemove.size(); ++WindowIndex)
        {
            DEBUG("RebalanceMonocleTree() Remove Window " << WindowsToRemove[WindowIndex]);
            RemoveWindowFromMonocleTree(Display, WindowsToRemove[WindowIndex]);
        }
    }
}

void AddWindowToNodeTree(ax_display *Display, unsigned int WindowID)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(!SpaceInfo->RootNode)
        CreateWindowNodeTree(Display);
    else if(SpaceInfo->Settings.Mode == SpaceModeBSP)
        AddWindowToBSPTree(Display, SpaceInfo, WindowID);
    else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
        AddWindowToMonocleTree(Display, SpaceInfo, WindowID);
}

void RemoveWindowFromNodeTree(ax_display *Display, unsigned int WindowID)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->Settings.Mode == SpaceModeBSP)
        RemoveWindowFromBSPTree(Display, WindowID);
    else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
        RemoveWindowFromMonocleTree(Display, WindowID);
}

void AddWindowToBSPTree(ax_display *Display, space_info *SpaceInfo, unsigned int WindowID)
{
    tree_node *RootNode = SpaceInfo->RootNode;
    if(RootNode)
    {
        tree_node *Insert = GetFirstPseudoLeafNode(SpaceInfo->RootNode);
        if(Insert && (Insert->WindowID = WindowID))
        {
            ApplyTreeNodeContainer(Insert);
            return;
        }

        tree_node *CurrentNode = NULL;
        ax_window *Window = FocusedApplication->Focus;
        if(Window && Window->ID != WindowID)
        {
            DEBUG("INSERT AT FOCUSED WINDOW");
            CurrentNode = GetTreeNodeFromWindowIDOrLinkNode(RootNode, Window->ID);
        }

        if(!CurrentNode)
        {
            DEBUG("INSERT AT LEFT-MOST WINDOW");
            GetFirstLeafNode(RootNode, (void**)&CurrentNode);
        }

        if(CurrentNode)
        {
            split_type SplitMode = KWMScreen.SplitMode == SPLIT_OPTIMAL ? GetOptimalSplitMode(CurrentNode) : KWMScreen.SplitMode;
            CreateLeafNodePair(Display, CurrentNode, CurrentNode->WindowID, WindowID, SplitMode);
            ApplyTreeNodeContainer(CurrentNode);
        }
    }
}

void RemoveWindowFromBSPTree(ax_display *Display, unsigned int WindowID)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    tree_node *WindowNode = GetTreeNodeFromWindowID(SpaceInfo->RootNode, WindowID);
    if(!WindowNode)
        return;

    tree_node *Parent = WindowNode->Parent;
    if(Parent && Parent->LeftChild && Parent->RightChild)
    {
        tree_node *AccessChild = IsRightChild(WindowNode) ? Parent->LeftChild : Parent->RightChild;
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
        }

        ResizeLinkNodeContainers(Parent);
        ApplyTreeNodeContainer(Parent);
        free(AccessChild);
        free(WindowNode);
    }
    else if(!Parent)
    {
        free(SpaceInfo->RootNode);
        SpaceInfo->RootNode = NULL;
    }
}

void AddWindowToMonocleTree(ax_display *Display, space_info *SpaceInfo, unsigned int WindowID)
{
    if(SpaceInfo->RootNode)
    {
        link_node *Link = SpaceInfo->RootNode->List;
        while(Link->Next)
            Link = Link->Next;

        link_node *NewLink = CreateLinkNode();
        SetLinkNodeContainer(Display, NewLink);

        NewLink->WindowID = WindowID;
        Link->Next = NewLink;
        NewLink->Prev = Link;

        ResizeWindowToContainerSize(NewLink);
    }
}

void RemoveWindowFromMonocleTree(ax_display *Display, unsigned int WindowID)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->RootNode && SpaceInfo->RootNode->List)
    {
        link_node *Link = GetLinkNodeFromTree(SpaceInfo->RootNode, WindowID);
        if(Link)
        {
            link_node *Prev = Link->Prev;
            link_node *Next = Link->Next;

            if(Prev)
                Prev->Next = Next;

            if(Next)
                Next->Prev = Prev;

            if(Link == SpaceInfo->RootNode->List)
            {
                SpaceInfo->RootNode->List = Next;

                if(!SpaceInfo->RootNode->List)
                {
                    free(SpaceInfo->RootNode);
                    SpaceInfo->RootNode = NULL;
                }
            }
            free(Link);
        }
    }
}

/* NOTE(koekeishiya): Old tiling code. */
void CreateWindowNodeTree(screen_info *Screen, std::vector<window_info*> *Windows) { }

void ShouldWindowNodeTreeUpdate(screen_info *Screen) { }

void ShouldBSPTreeUpdate(screen_info *Screen, space_info *Space) { }

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

void ShouldMonocleTreeUpdate(screen_info *Screen, space_info *Space) {}

void AddWindowToMonocleTree(screen_info *Screen, int WindowID) { }

void RemoveWindowFromMonocleTree(screen_info *Screen, int WindowID, bool Center, bool UpdateFocus) { }

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

void ToggleWindowFloating(unsigned int WindowID, bool Center)
{
    ax_window *Window = GetWindowByID(WindowID);
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    /* TODO(koekeishiya): Do we handle standby-on-float here (?)
    if(KWMMode.Focus != FocusModeDisabled && KWMToggles.StandbyOnFloat)
        KWMMode.Focus = FocusModeAutoraise;
    if(KWMMode.Focus != FocusModeDisabled && KWMToggles.StandbyOnFloat)
        KWMMode.Focus = FocusModeStandby;
    */

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(AXLibHasFlags(Window, AXWindow_Floating))
    {
        AXLibClearFlags(Window, AXWindow_Floating);
        AddWindowToNodeTree(Display, Window->ID);
    }
    else
    {
        AXLibAddFlags(Window, AXWindow_Floating);
        RemoveWindowFromNodeTree(Display, Window->ID);
    }
}

void ToggleFocusedWindowFloating()
{
    if(FocusedApplication && FocusedApplication->Focus)
        ToggleWindowFloating(FocusedApplication->Focus->ID, true);
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
    /* NOTE(koekeishiya): The following code works and was added due to frustration of using a pre-alpha
                          window manager during development.  */

    /* TODO(koekeishiya): This function should be able to assume that the focused window is valid. */

    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(!Space->RootNode)
        return;

    tree_node *Node = NULL;
    if(Space->RootNode->WindowID == -1)
    {
        Node = GetTreeNodeFromWindowID(Space->RootNode, Window->ID);
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
        Node = GetTreeNodeFromWindowID(Space->RootNode, Window->ID);
        if(Node)
        {
            ResizeWindowToContainerSize(Node);
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

void DetachAndReinsertWindow(unsigned int WindowID, int Degrees)
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
           WindowID == 0)
            return;

        ax_window *ClosestWindow = NULL;
        window_info InsertWindow = {};
        if(FindClosestWindow(Degrees, &ClosestWindow, false))
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
    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(!Space)
        return;

    if(Space->Settings.Mode == SpaceModeMonocle)
    {
        link_node *Link = GetLinkNodeFromTree(Space->RootNode, Window->ID);
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
                // MoveCursorToCenterOfFocusedWindow();
            }
        }
    }
    else if(Space->Settings.Mode == SpaceModeBSP)
    {
        tree_node *TreeNode = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, Window->ID);
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
                // MoveCursorToCenterOfFocusedWindow();
            }
        }
    }
}

void SwapFocusedWindowDirected(int Degrees)
{
    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(!Space)
        return;

    if(Space->Settings.Mode == SpaceModeBSP)
    {
        tree_node *TreeNode = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, Window->ID);
        if(TreeNode)
        {
            tree_node *NewFocusNode = NULL;
            ax_window *ClosestWindow = NULL;
            if(FindClosestWindow(Degrees, &ClosestWindow, KWMMode.Cycle == CycleModeScreen))
                NewFocusNode = GetTreeNodeFromWindowID(Space->RootNode, ClosestWindow->ID);

            if(NewFocusNode)
            {
                SwapNodeWindowIDs(TreeNode, NewFocusNode);
            }
        }
    }
    else if(Space->Settings.Mode == SpaceModeMonocle)
    {
        if(Degrees == 90)
            SwapFocusedWindowWithNearest(1);
        else if(Degrees == 270)
            SwapFocusedWindowWithNearest(-1);
    }
}

bool WindowIsInDirection(ax_window *WindowA, ax_window *WindowB, int Degrees)
{
    ax_display *Display = AXLibWindowDisplay(WindowA);
    space_info *Space = &WindowTree[Display->Space->Identifier];
    tree_node *NodeA = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, WindowA->ID);
    tree_node *NodeB = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, WindowB->ID);

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

bool WindowIsInDirection(window_info *WindowA, window_info *WindowB, int Degrees) { }

void GetCenterOfWindow(ax_window *Window, int *X, int *Y)
{
    ax_display *Display = AXLibWindowDisplay(Window);
    space_info *Space = &WindowTree[Display->Space->Identifier];
    tree_node *Node = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, Window->ID);
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

void GetCenterOfWindow(window_info *Window, int *X, int *Y) { }

double GetWindowDistance(ax_window *A, ax_window *B, int Degrees, bool Wrap)
{
    ax_display *Display = AXLibWindowDisplay(A);
    double Rank = INT_MAX;

    int X1, Y1, X2, Y2;
    GetCenterOfWindow(A, &X1, &Y1);
    GetCenterOfWindow(B, &X2, &Y2);

    if(Wrap)
    {
        if(Degrees == 0 && Y1 < Y2)
            Y2 -= Display->Frame.size.height;
        else if(Degrees == 180 && Y1 > Y2)
            Y2 += Display->Frame.size.height;
        else if(Degrees == 90 && X1 > X2)
            X2 += Display->Frame.size.width;
        else if(Degrees == 270 && X1 < X2)
            X2 -= Display->Frame.size.width;
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

double GetWindowDistance(window_info *A, window_info *B, int Degrees, bool Wrap) { }

bool FindClosestWindow(int Degrees, window_info *Target, bool Wrap) { }

bool FindClosestWindow(int Degrees, ax_window **ClosestWindow, bool Wrap)
{
    ax_window *Match = FocusedApplication->Focus;
    std::vector<ax_window*> Windows = AXLibGetAllVisibleWindows();

    int MatchX, MatchY;
    GetCenterOfWindow(Match, &MatchX, &MatchY);

    double MinDist = INT_MAX;
    for(int Index = 0; Index < Windows.size(); ++Index)
    {
        ax_window *Window = Windows[Index];
        if(Match->ID != Window->ID &&
           WindowIsInDirection(Match, Window, Degrees))
        {
            double Dist = GetWindowDistance(Match, Window, Degrees, Wrap);
            if(Dist < MinDist)
            {
                MinDist = Dist;
                *ClosestWindow = Window;
            }
        }
    }

    return MinDist != INT_MAX;
}

void ShiftWindowFocusDirected(int Degrees)
{
    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->Settings.Mode == SpaceModeBSP)
    {
        ax_window *ClosestWindow = NULL;
        if((KWMMode.Cycle == CycleModeDisabled &&
            FindClosestWindow(Degrees, &ClosestWindow, false)) ||
           (KWMMode.Cycle == CycleModeScreen &&
            FindClosestWindow(Degrees, &ClosestWindow, true)))
        {
            AXLibSetFocusedWindow(ClosestWindow);
            // MoveCursorToCenterOfFocusedWindow();
        }
    }
    else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
    {
        if(Degrees == 90)
            ShiftWindowFocus(1);
        else if(Degrees == 270)
            ShiftWindowFocus(-1);
    }
}

void ShiftWindowFocus(int Shift)
{
    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
    {
        link_node *Link = GetLinkNodeFromTree(SpaceInfo->RootNode, Window->ID);
        if(Link)
        {
            link_node *FocusNode = Shift == 1 ? Link->Next : Link->Prev;
            if(KWMMode.Cycle == CycleModeScreen && !FocusNode)
            {
                SpaceInfo->RootNode->Type = NodeTypeLink;
                if(Shift == 1)
                    GetFirstLeafNode(SpaceInfo->RootNode, (void**)&FocusNode);
                else
                    GetLastLeafNode(SpaceInfo->RootNode, (void**)&FocusNode);
                SpaceInfo->RootNode->Type = NodeTypeTree;
            }

            if(FocusNode)
            {
                SetWindowFocusByNode(FocusNode);
                // MoveCursorToCenterOfFocusedWindow();
            }
        }
    }
    else if(SpaceInfo->Settings.Mode == SpaceModeBSP)
    {
        tree_node *TreeNode = GetTreeNodeFromWindowID(SpaceInfo->RootNode, Window->ID);
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
                    GetFirstLeafNode(SpaceInfo->RootNode, (void**)&FocusNode);
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
                    GetLastLeafNode(SpaceInfo->RootNode, (void**)&FocusNode);
                    while(IsPseudoNode(FocusNode))
                        FocusNode = GetNearestTreeNodeToTheLeft(FocusNode);
                }
            }

            SetWindowFocusByNode(FocusNode);
            // MoveCursorToCenterOfFocusedWindow();
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
    ax_window *Window = GetWindowByID((unsigned int)WindowID);
    if(Window)
    {
        AXLibSetFocusedWindow(Window);
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

void SetKwmFocus(AXUIElementRef WindowRef) { }

void SetWindowRefFocus(AXUIElementRef WindowRef) { }

void SetWindowFocus(window_info *Window) { }

void SetWindowFocusByNode(tree_node *Node)
{
    if(Node)
    {
        ax_window *Window = GetWindowByID((unsigned int)Node->WindowID);
        if(Window)
        {
            DEBUG("SetWindowFocusByNode()");
            AXLibSetFocusedWindow(Window);
        }
    }
}

void SetWindowFocusByNode(link_node *Link)
{
    if(Link)
    {
        ax_window *Window = GetWindowByID((unsigned int)Link->WindowID);
        if(Window)
        {
            DEBUG("SetWindowFocusByNode()");
            AXLibSetFocusedWindow(Window);
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

CGPoint GetWindowPos(window_info *Window) { }
window_info GetWindowByRef(AXUIElementRef WindowRef) { }

window_info *GetWindowByID(int WindowID) { }

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

bool GetWindowRole(window_info *Window, CFTypeRef *Role, CFTypeRef *SubRole) { }

bool GetWindowRef(window_info *Window, AXUIElementRef *WindowRef) { }

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
