#include "window.h"
#include "container.h"
#include "node.h"
#include "display.h"
#include "space.h"
#include "tree.h"
#include "border.h"
#include "helpers.h"
#include "rules.h"
#include "serializer.h"
#include "cursor.h"
#include "scratchpad.h"
#include "axlib/axlib.h"

#include <cmath>

#define internal static
#define local_persist static

/* TODO(koekeishiya): Every event should pass a valid identifier that can be used to lookup the appropriate
                      context, rather than passing a pointer to the context itself. This is necessary to be
                      able to guarantee that we do not access an invalid context, as it may have been invalidated
                      between event creation and event processing. */

extern std::map<const char *, space_info> WindowTree;
extern ax_state AXState;
extern ax_display *FocusedDisplay;
extern ax_application *FocusedApplication;
extern ax_window *MarkedWindow;

extern kwm_settings KWMSettings;
extern kwm_path KWMPath;
extern kwm_thread KWMThread;
extern kwm_border MarkedBorder;
extern kwm_border FocusedBorder;

internal void
DrawFocusedBorder(ax_display *Display)
{
    if((Display) &&
       (Display->Space->Type == kCGSSpaceUser))
    {
        UpdateBorder("focused");
    }
}

internal void
FloatNonResizable(ax_window *Window)
{
    if(Window)
    {
        if((KWMSettings.FloatNonResizable) &&
           (!AXLibHasFlags(Window, AXWindow_Resizable)))
        {
            AXLibAddFlags(Window, AXWindow_Floating);
        }
    }
}

internal void
StandbyOnFloat(ax_window *Window)
{
    if(Window)
    {
        if((KWMSettings.StandbyOnFloat) &&
           (KWMSettings.Focus != FocusModeDisabled))
        {
            if(AXLibHasFlags(Window, AXWindow_Floating))
                KWMSettings.Focus = FocusModeStandby;
            else
                KWMSettings.Focus = FocusModeAutoraise;
        }
    }
}

internal void
TileWindow(ax_display *Display, ax_window *Window)
{
    if(Window)
    {
        if((!AXLibHasFlags(Window, AXWindow_Minimized)) &&
           (AXLibIsWindowStandard(Window) || AXLibIsWindowCustom(Window)) &&
           (!AXLibHasFlags(Window, AXWindow_Floating)))
        {
            AddWindowToNodeTree(Display, Window->ID);
        }
    }
}

/* TODO(koekeishiya): Event context is a pointer to the new display. */
EVENT_CALLBACK(Callback_AXEvent_DisplayAdded)
{
    DEBUG("AXEvent_DisplayAdded");
}

EVENT_CALLBACK(Callback_AXEvent_DisplayRemoved)
{
    DEBUG("AXEvent_DisplayRemoved");
}

/* NOTE(koekeishiya): Event context is a pointer to the resized display. */
EVENT_CALLBACK(Callback_AXEvent_DisplayResized)
{
    ax_display *Display = (ax_display *) Event->Context;
    DEBUG("AXEvent_DisplayResized");

    std::map<CGSSpaceID, ax_space>::iterator It;
    for(It = Display->Spaces.begin(); It != Display->Spaces.end(); ++It)
    {
        ax_space *Space = &It->second;
        space_info *SpaceInfo = &WindowTree[Space->Identifier];
        if(Space == Display->Space)
            UpdateSpaceOfDisplay(Display, SpaceInfo);
        else
            SpaceInfo->NeedsUpdate = true;
    }
}

/* NOTE(koekeishiya): Event context is a pointer to the moved display. */
EVENT_CALLBACK(Callback_AXEvent_DisplayMoved)
{
    ax_display *Display = (ax_display *) Event->Context;
    DEBUG("AXEvent_DisplayMoved");

    std::map<CGSSpaceID, ax_space>::iterator It;
    for(It = Display->Spaces.begin(); It != Display->Spaces.end(); ++It)
    {
        ax_space *Space = &It->second;
        space_info *SpaceInfo = &WindowTree[Space->Identifier];
        if(Space == Display->Space)
            UpdateSpaceOfDisplay(Display, SpaceInfo);
        else
            SpaceInfo->NeedsUpdate = true;
    }
}

/* NOTE(koekeishiya): Event context is NULL. */
EVENT_CALLBACK(Callback_AXEvent_DisplayChanged)
{
    FocusedDisplay = AXLibMainDisplay();
    printf("%d: AXEvent_DisplayChanged\n", FocusedDisplay->ArrangementID);

    AXLibRunningApplications();
    CreateWindowNodeTree(FocusedDisplay);
    RebalanceNodeTree(FocusedDisplay);
}

internal void
AddMinimizedWindowsToTree(ax_display *Display)
{
    std::vector<ax_window *> Windows = AXLibGetAllVisibleWindows();
    for(std::size_t Index = 0; Index < Windows.size(); ++Index)
    {
        ax_window *Window = Windows[Index];
        if(AXLibHasFlags(Window, AXWindow_Minimized))
        {
            AXLibClearFlags(Window, AXWindow_Minimized);
            if((AXLibIsWindowStandard(Window) || AXLibIsWindowCustom(Window)) &&
               (!AXLibHasFlags(Window, AXWindow_Floating)))
            {
                AddWindowToNodeTree(Display, Window->ID);
            }
        }
    }
}

/* NOTE(koekeishiya): Event context is a pointer to the display whos space was changed. */
EVENT_CALLBACK(Callback_AXEvent_SpaceChanged)
{
    ax_display *Display = (ax_display *) Event->Context;
    DEBUG("AXEvent_SpaceChanged");

    ClearBorder(&FocusedBorder);
    ClearMarkedWindow();

    FocusedDisplay = Display;
    ax_space *PrevSpace = Display->PrevSpace;
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];

    AXLibRunningApplications();
    RebalanceNodeTree(Display);
    if(SpaceInfo->NeedsUpdate)
        UpdateSpaceOfDisplay(Display, SpaceInfo);

    /* NOTE(koekeishiya): This space transition was invoked through deminiaturizing a window.
                          We have no way of passing the actual window in question, to this callback,
                          so we have marked the window through AXWindow_Minimized. We iterate through
                          all visible windows to locate the window we need. */
    if(AXLibHasFlags(PrevSpace, AXSpace_DeminimizedTransition))
    {
        AXLibClearFlags(PrevSpace, AXSpace_DeminimizedTransition);
        AddMinimizedWindowsToTree(Display);
    }

    /* NOTE(koekeishiya): This space transition was invoked after a window was moved to this space.
                          We have marked the window through AXWindow_Minimized. We iterate through
                          all visible windows to locate the window we need. */
    if(SpaceInfo->Initialized &&
       AXLibHasFlags(Display->Space, AXSpace_NeedsUpdate))
    {
        AXLibClearFlags(Display->Space, AXSpace_NeedsUpdate);
        AddMinimizedWindowsToTree(Display);
    }

    CreateWindowNodeTree(Display);

    /* NOTE(koekeishiya): If we trigger a space changed event through cmd+tab, we receive the 'didApplicationActivate'
                          notification before the 'didActiveSpaceChange' notification. If a space has not been visited
                          before, this will cause us to end up on that space with an unsynchronized focused application state.

                          Always update state of focused application and its window after a space transition. */
    FocusedApplication = AXLibGetFocusedApplication();
    if(FocusedApplication)
    {
        FocusedApplication->Focus = AXLibGetFocusedWindow(FocusedApplication);
        if((FocusedApplication->Focus) &&
           (AXLibSpaceHasWindow(FocusedApplication->Focus, Display->Space->ID)))
        {
            DrawFocusedBorder(Display);
            MoveCursorToCenterOfWindow(FocusedApplication->Focus);
            Display->Space->FocusedWindow = FocusedApplication->Focus->ID;
        }
    }

    /* NOTE(koekeishiya): This space transition was triggered through AXLibSpaceTransition(..) and OSX does not
                          update our focus in this case. We manually try to activate the appropriate window. */
    if(AXLibHasFlags(Display->Space, AXSpace_FastTransition))
    {
        AXLibClearFlags(Display->Space, AXSpace_FastTransition);
        ax_window *Window = GetWindowByID(Display->Space->FocusedWindow);
        if(Window)
        {
            AXLibSetFocusedWindow(Window);
            DrawFocusedBorder(Display);
            MoveCursorToCenterOfWindow(Window);
        }
    }

    if(Display->Space->Type != kCGSSpaceUser)
        ClearBorder(&FocusedBorder);
}

/* TODO(koekeishiya): Is this interesting (?)
EVENT_CALLBACK(Callback_AXEvent_SpaceCreated)
{
    ax_space *Space = (ax_space *) Event->Context;
}
*/

/* NOTE(koekeishiya): Event context is a pointer to the launched application. */
EVENT_CALLBACK(Callback_AXEvent_ApplicationLaunched)
{
    ax_application *Application = (ax_application *) Event->Context;
    DEBUG("AXEvent_ApplicationLaunched: " << Application->Name);

    std::map<uint32_t, ax_window *>::iterator It;
    for(It = Application->Windows.begin(); It != Application->Windows.end(); ++It)
    {
        ax_window *Window = It->second;
        if(ApplyWindowRules(Window))
            continue;

        ax_display *Display = AXLibCursorDisplay();
        if(!Display)
            Display = AXLibWindowDisplay(Window);

        FloatNonResizable(Window);
        TileWindow(Display, Window);
    }
}

/* NOTE(koekeishiya): Event context is NULL */
EVENT_CALLBACK(Callback_AXEvent_ApplicationTerminated)
{
    DEBUG("AXEvent_ApplicationTerminated");

    /* TODO(koekeishiya): We probably want to flag every display for an update, as the application
                          in question could have had windows on several displays and spaces. */
    ax_display *Display = AXLibMainDisplay();
    Assert(Display != NULL);
    RebalanceNodeTree(Display);
}

/* NOTE(koekeishiya): Event context is a pointer to the activated application. */
EVENT_CALLBACK(Callback_AXEvent_ApplicationActivated)
{
    ax_application *Application = (ax_application *) Event->Context;
    DEBUG("AXEvent_ApplicationActivated: " << Application->Name);

    FocusedApplication = Application;
    if(Application->Focus)
    {
        ax_display *Display = AXLibWindowDisplay(Application->Focus);
        if(AXLibSpaceHasWindow(Application->Focus, Display->Space->ID))
        {
            StandbyOnFloat(Application->Focus);
            Display->Space->FocusedWindow = Application->Focus->ID;
            DrawFocusedBorder(Display);
        }
    }
}

/* NOTE(koekeishiya): Event context is a pointer to the new window. */
EVENT_CALLBACK(Callback_AXEvent_WindowCreated)
{
    ax_window *Window = (ax_window *) Event->Context;
    DEBUG("AXEvent_WindowCreated: " << Window->Application->Name << " - " << Window->Name);

    if(ApplyWindowRules(Window))
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(Display)
    {
        FloatNonResizable(Window);
        TileWindow(Display, Window);
    }
}

/* NOTE(koekeishiya): Event context is a pointer to the closed window. Must call AXLibDestroyWindow() */
EVENT_CALLBACK(Callback_AXEvent_WindowDestroyed)
{
    ax_window *Window = (ax_window *) Event->Context;
    DEBUG("AXEvent_WindowDestroyed: " << Window->Application->Name << " - " << Window->Name);

    ax_display *Display = AXLibWindowDisplay(Window);
    if(Display)
    {
        RemoveWindowFromScratchpad(Window);
        RemoveWindowFromNodeTree(Display, Window->ID);
    }

    if(FocusedApplication == Window->Application)
    {
        if(FocusedApplication->Focus == Window)
            Display->Space->FocusedWindow = 0;

        StandbyOnFloat(FocusedApplication->Focus);
        DrawFocusedBorder(Display);
    }

    if(MarkedWindow == Window)
        ClearMarkedWindow();

    AXLibDestroyWindow(Window);
}

/* NOTE(koekeishiya): Event context is a pointer to the minimized window. */
EVENT_CALLBACK(Callback_AXEvent_WindowMinimized)
{
    ax_window *Window = (ax_window *) Event->Context;
    DEBUG("AXEvent_WindowMinimized: " << Window->Application->Name << " - " << Window->Name);

    ax_display *Display = AXLibWindowDisplay(Window);
    Assert(Display != NULL);
    RemoveWindowFromNodeTree(Display, Window->ID);

    ClearBorder(&FocusedBorder);
    if(MarkedWindow == Window)
        ClearMarkedWindow();
}

/* NOTE(koekeishiya): Event context is a pointer to the deminimized window. */
EVENT_CALLBACK(Callback_AXEvent_WindowDeminimized)
{
    ax_window *Window = (ax_window *) Event->Context;
    DEBUG("AXEvent_WindowDeminimized: " << Window->Application->Name << " - " << Window->Name);

    ax_display *Display = AXLibWindowDisplay(Window);
    Assert(Display != NULL);

    if((AXLibIsWindowStandard(Window) || AXLibIsWindowCustom(Window)) &&
       (!AXLibHasFlags(Window, AXWindow_Floating)))
    {
        AddWindowToNodeTree(Display, Window->ID);
    }

    Window->Application->Focus = Window;
    AXLibClearFlags(Window, AXWindow_Minimized);
    AXLibConstructEvent(AXEvent_ApplicationActivated, Window->Application, false);
}

/* NOTE(koekeishiya): Event context is a pointer to the focused window. */
EVENT_CALLBACK(Callback_AXEvent_WindowFocused)
{
    ax_window *Window = (ax_window *) Event->Context;
    DEBUG("AXEvent_WindowFocused: " << Window->Application->Name << " - " << Window->Name);

    if((AXLibIsWindowStandard(Window) || AXLibIsWindowCustom(Window)))
    {
        Window->Application->Focus = Window;
        if(FocusedApplication == Window->Application)
        {
            ax_display *Display = AXLibWindowDisplay(Window);
            StandbyOnFloat(Window);
            DrawFocusedBorder(Display);
            Display->Space->FocusedWindow = Window->ID;
        }
    }
}


/* NOTE(koekeishiya): Event context is a pointer to the moved window. */
EVENT_CALLBACK(Callback_AXEvent_WindowMoved)
{
    ax_window *Window = (ax_window *) Event->Context;
    ax_display *Display = AXLibWindowDisplay(Window);
    DEBUG("AXEvent_WindowMoved: " << Window->Application->Name << " - " << Window->Name);

    if(!Event->Intrinsic && KWMSettings.LockToContainer)
        LockWindowToContainerSize(Window);

    if(FocusedApplication == Window->Application)
        DrawFocusedBorder(Display);

    if(MarkedWindow == Window)
        UpdateBorder("marked");
}

/* NOTE(koekeishiya): Event context is a pointer to the resized window. */
EVENT_CALLBACK(Callback_AXEvent_WindowResized)
{
    ax_window *Window = (ax_window *) Event->Context;
    ax_display *Display = AXLibWindowDisplay(Window);
    DEBUG("AXEvent_WindowResized: " << Window->Application->Name << " - " << Window->Name);

    if(!Event->Intrinsic && KWMSettings.LockToContainer)
        LockWindowToContainerSize(Window);

    if(FocusedApplication == Window->Application)
        DrawFocusedBorder(Display);

    if(MarkedWindow == Window)
        UpdateBorder("marked");
}

internal std::vector<uint32_t>
GetAllWindowIDsInTree(space_info *Space)
{
    std::vector<uint32_t> Windows;
    if(Space->Settings.Mode == SpaceModeBSP)
    {
        tree_node *CurrentNode = NULL;
        GetFirstLeafNode(Space->RootNode, (void**)&CurrentNode);
        while(CurrentNode)
        {
            if(CurrentNode->WindowID != 0)
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

internal std::vector<uint32_t>
GetAllAXWindowIDsToRemoveFromTree(std::vector<uint32_t> &WindowIDsInTree)
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

internal std::vector<uint32_t>
GetAllWindowIDSOnDisplay(ax_display *Display)
{
    std::vector<uint32_t> Windows;
    std::vector<ax_window*> VisibleWindows = AXLibGetAllVisibleWindows();
    for(int Index = 0; Index < VisibleWindows.size(); ++Index)
    {
        ax_window *Window = VisibleWindows[Index];
        ax_display *DisplayOfWindow = AXLibWindowDisplay(Window);
        if(DisplayOfWindow)
        {
            if(DisplayOfWindow != Display)
            {
                space_info *SpaceOfWindow = &WindowTree[DisplayOfWindow->Space->Identifier];
                if(!SpaceOfWindow->Initialized ||
                   SpaceOfWindow->Settings.Mode == SpaceModeFloating ||
                   GetTreeNodeFromWindowID(SpaceOfWindow->RootNode, Window->ID) ||
                   GetLinkNodeFromWindowID(SpaceOfWindow->RootNode, Window->ID))
                    continue;
            }

            Windows.push_back(Window->ID);
        }
    }

    return Windows;
}

/* TODO(koekeishiya): Fix how these settings are stored. */
internal void
LoadSpaceSettings(ax_display *Display, space_info *SpaceInfo)
{
    int DesktopID = AXLibDesktopIDFromCGSSpaceID(Display, Display->Space->ID);

    /* NOTE(koekeishiya): Load global default display settings. */
    SpaceInfo->Settings.Offset = KWMSettings.DefaultOffset;
    SpaceInfo->Settings.Mode = SpaceModeDefault;
    SpaceInfo->Settings.Layout = "";
    SpaceInfo->Settings.Name = "";

    /* NOTE(koekeishiya): The space in question may have overloaded settings. */
    space_settings *SpaceSettings = NULL;
    if((SpaceSettings = GetSpaceSettingsForDesktopID(Display->ArrangementID, DesktopID)))
        SpaceInfo->Settings = *SpaceSettings;
    else if((SpaceSettings = GetSpaceSettingsForDisplay(Display->ArrangementID)))
        SpaceInfo->Settings = *SpaceSettings;

    if(SpaceInfo->Settings.Mode == SpaceModeDefault)
        SpaceInfo->Settings.Mode = KWMSettings.Space;
}

internal void
AddWindowToBSPTree(ax_display *Display, space_info *SpaceInfo, uint32_t WindowID)
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
        ax_application *Application = FocusedApplication ? FocusedApplication : AXLibGetFocusedApplication();
        ax_window *Window = Application ? Application->Focus : NULL;
        if(MarkedWindow && MarkedWindow->ID != WindowID)
            CurrentNode = GetTreeNodeFromWindowIDOrLinkNode(RootNode, MarkedWindow->ID);

        if(!CurrentNode && Window && Window->ID != WindowID)
            CurrentNode = GetTreeNodeFromWindowIDOrLinkNode(RootNode, Window->ID);

        if(!CurrentNode)
            GetFirstLeafNode(RootNode, (void**)&CurrentNode);

        if(CurrentNode)
        {
            if(CurrentNode->Type == NodeTypeTree)
            {
                split_type SplitMode = KWMSettings.SplitMode == SPLIT_OPTIMAL ? GetOptimalSplitMode(CurrentNode) : KWMSettings.SplitMode;
                CreateLeafNodePair(Display, CurrentNode, CurrentNode->WindowID, WindowID, SplitMode);
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
}

internal void
RemoveWindowFromBSPTree(ax_display *Display, uint32_t WindowID)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    tree_node *WindowNode = GetTreeNodeFromWindowID(SpaceInfo->RootNode, WindowID);
    if(!WindowNode)
    {
        link_node *Link = GetLinkNodeFromWindowID(SpaceInfo->RootNode, WindowID);
        tree_node *Root = GetTreeNodeFromLink(SpaceInfo->RootNode, Link);
        if(Link)
        {
            link_node *Prev = Link->Prev;
            link_node *Next = Link->Next;

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

            free(Link);
        }
    }
    else
    {
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
}

internal void
AddWindowToMonocleTree(ax_display *Display, space_info *SpaceInfo, uint32_t WindowID)
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

internal void
RemoveWindowFromMonocleTree(ax_display *Display, uint32_t WindowID)
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

internal void
CreateAndInitializeSpaceInfoWithWindowTree(ax_display *Display, space_info *SpaceInfo, std::vector<uint32_t> *Windows)
{
    SpaceInfo->Initialized = true;
    LoadSpaceSettings(Display, SpaceInfo);
    if((SpaceInfo->Settings.Mode == SpaceModeFloating) ||
       (Display->Space->Type != kCGSSpaceUser))
        return;

    if(SpaceInfo->Settings.Mode == SpaceModeBSP && !SpaceInfo->Settings.Layout.empty())
    {
        LoadBSPTreeFromFile(Display, SpaceInfo, SpaceInfo->Settings.Layout);
        FillDeserializedTree(SpaceInfo->RootNode, Display, Windows);
    }
    else
    {
        SpaceInfo->RootNode = CreateTreeFromWindowIDList(Display, Windows);
    }

    if(SpaceInfo->RootNode)
        ApplyTreeNodeContainer(SpaceInfo->RootNode);
}

internal void
CreateSpaceInfoWithWindowTree(ax_display *Display, space_info *SpaceInfo, std::vector<uint32_t> *Windows)
{
    if((SpaceInfo->Settings.Mode == SpaceModeFloating) ||
       (Display->Space->Type != kCGSSpaceUser))
        return;

    /* NOTE(koekeishiya): If a space has been initialized, but the node-tree was destroyed. */
    if(SpaceInfo->Settings.Mode == SpaceModeBSP && !SpaceInfo->Settings.Layout.empty())
    {
        LoadBSPTreeFromFile(Display, SpaceInfo, SpaceInfo->Settings.Layout);
        FillDeserializedTree(SpaceInfo->RootNode, Display, Windows);
    }
    else
    {
        SpaceInfo->RootNode = CreateTreeFromWindowIDList(Display, Windows);
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

        ApplyTreeNodeContainer(SpaceInfo->RootNode);
    }
}

void CreateWindowNodeTree(ax_display *Display)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(!SpaceInfo->Initialized && !SpaceInfo->RootNode)
    {
        std::vector<ax_window *> KnownWindows = AXLibGetAllKnownWindows();
        for(std::size_t Index = 0; Index < KnownWindows.size(); ++Index)
        {
            ax_window *Window = KnownWindows[Index];
            ApplyWindowRules(Window);
        }

        std::vector<uint32_t> Windows = GetAllWindowIDSOnDisplay(Display);
        CreateAndInitializeSpaceInfoWithWindowTree(Display, SpaceInfo, &Windows);
    }
    else if(SpaceInfo->Initialized && !SpaceInfo->RootNode)
    {
        std::vector<uint32_t> Windows = GetAllWindowIDSOnDisplay(Display);
        CreateSpaceInfoWithWindowTree(Display, SpaceInfo, &Windows);
    }
}

void LoadWindowNodeTree(ax_display *Display, std::string Layout)
{
    if(Display)
    {
        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        if(SpaceInfo->Settings.Mode == SpaceModeBSP)
        {
            std::vector<uint32_t> Windows = GetAllWindowIDSOnDisplay(Display);
            LoadBSPTreeFromFile(Display, SpaceInfo, Layout);
            FillDeserializedTree(SpaceInfo->RootNode, Display, &Windows);
            ApplyTreeNodeContainer(SpaceInfo->RootNode);
        }
    }
}

void ResetWindowNodeTree(ax_display *Display, space_tiling_option Mode)
{
    if(Display)
    {
        if(AXLibIsSpaceTransitionInProgress())
            return;

        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        if(SpaceInfo->Settings.Mode == Mode)
            return;

        DestroyNodeTree(SpaceInfo->RootNode);
        SpaceInfo->RootNode = NULL;
        SpaceInfo->Initialized = true;
        SpaceInfo->Settings.Mode = Mode;
        CreateWindowNodeTree(Display);
    }
}

void AddWindowToNodeTree(ax_display *Display, uint32_t WindowID)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(!SpaceInfo->RootNode)
        CreateWindowNodeTree(Display);
    else if(SpaceInfo->Settings.Mode == SpaceModeBSP)
        AddWindowToBSPTree(Display, SpaceInfo, WindowID);
    else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
        AddWindowToMonocleTree(Display, SpaceInfo, WindowID);
}

void RemoveWindowFromNodeTree(ax_display *Display, uint32_t WindowID)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->Settings.Mode == SpaceModeBSP)
        RemoveWindowFromBSPTree(Display, WindowID);
    else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
        RemoveWindowFromMonocleTree(Display, WindowID);
}

internal void
RebalanceBSPTree(ax_display *Display)
{
    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(Space->RootNode)
    {
        std::vector<uint32_t> WindowIDsInTree = GetAllWindowIDsInTree(Space);
        std::vector<uint32_t> WindowsToRemove = GetAllAXWindowIDsToRemoveFromTree(WindowIDsInTree);

        for(std::size_t WindowIndex = 0; WindowIndex < WindowsToRemove.size(); ++WindowIndex)
        {
            DEBUG("RebalanceBSPTree() Remove Window " << WindowsToRemove[WindowIndex]);
            RemoveWindowFromBSPTree(Display, WindowsToRemove[WindowIndex]);
        }
    }
}

internal void
RebalanceMonocleTree(ax_display *Display)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->RootNode && SpaceInfo->RootNode->List)
    {
        std::vector<uint32_t> WindowIDsInTree = GetAllWindowIDsInTree(SpaceInfo);
        std::vector<uint32_t> WindowsToRemove = GetAllAXWindowIDsToRemoveFromTree(WindowIDsInTree);

        for(std::size_t WindowIndex = 0; WindowIndex < WindowsToRemove.size(); ++WindowIndex)
        {
            DEBUG("RebalanceMonocleTree() Remove Window " << WindowsToRemove[WindowIndex]);
            RemoveWindowFromMonocleTree(Display, WindowsToRemove[WindowIndex]);
        }
    }
}

void RebalanceNodeTree(ax_display *Display)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(!SpaceInfo->Initialized)
        return;

    if(SpaceInfo->Settings.Mode == SpaceModeBSP)
        RebalanceBSPTree(Display);
    else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
        RebalanceMonocleTree(Display);
}


void CreateInactiveWindowNodeTree(ax_display *Display, std::vector<uint32_t> *Windows)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(!SpaceInfo->Initialized && !SpaceInfo->RootNode)
    {
        std::vector<ax_window *> KnownWindows = AXLibGetAllKnownWindows();
        for(std::size_t Index = 0; Index < KnownWindows.size(); ++Index)
        {
            ax_window *Window = KnownWindows[Index];
            ApplyWindowRules(Window);
        }
        CreateAndInitializeSpaceInfoWithWindowTree(Display, SpaceInfo, Windows);
    }
    else if(SpaceInfo->Initialized && !SpaceInfo->RootNode)
    {
        CreateSpaceInfoWithWindowTree(Display, SpaceInfo, Windows);
    }
}

void AddWindowToInactiveNodeTree(ax_display *Display, uint32_t WindowID)
{
    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(!SpaceInfo->RootNode)
    {
        std::vector<uint32_t> Windows;
        Windows.push_back(WindowID);
        CreateInactiveWindowNodeTree(Display, &Windows);
    }
    else if(SpaceInfo->Settings.Mode == SpaceModeBSP)
    {
        DEBUG("AddWindowToInactiveNodeTree() BSP Space");
        tree_node *CurrentNode = NULL;
        GetFirstLeafNode(SpaceInfo->RootNode, (void**)&CurrentNode);
        split_type SplitMode = KWMSettings.SplitMode == SPLIT_OPTIMAL ? GetOptimalSplitMode(CurrentNode) : KWMSettings.SplitMode;

        CreateLeafNodePair(Display, CurrentNode, CurrentNode->WindowID, WindowID, SplitMode);
        ApplyTreeNodeContainer(CurrentNode);
    }
    else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
    {
        DEBUG("AddWindowToInactiveNodeTree() Monocle Space");
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

void ToggleWindowFloating(uint32_t WindowID, bool Center)
{
    ax_window *Window = GetWindowByID(WindowID);
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    if(AXLibHasFlags(Window, AXWindow_Floating))
    {
        AXLibClearFlags(Window, AXWindow_Floating);
        TileWindow(Display, Window);
        if((KWMSettings.StandbyOnFloat) &&
           (KWMSettings.Focus != FocusModeDisabled))
            KWMSettings.Focus = FocusModeAutoraise;
    }
    else
    {
        AXLibAddFlags(Window, AXWindow_Floating);
        RemoveWindowFromNodeTree(Display, Window->ID);
        if((KWMSettings.StandbyOnFloat) &&
           (KWMSettings.Focus != FocusModeDisabled))
            KWMSettings.Focus = FocusModeStandby;
    }
}

void ToggleFocusedWindowFloating()
{
    if(FocusedApplication && FocusedApplication->Focus)
        ToggleWindowFloating(FocusedApplication->Focus->ID, true);
}

void ToggleFocusedWindowParentContainer()
{
    /* TODO(koekeishiya): Should this function be able to assume that the focused window is valid (?) */

    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(!Space->RootNode)
        return;

    if(Space->Settings.Mode != SpaceModeBSP)
        return;

    tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, Window->ID);
    if(Node && Node->Parent)
    {
        if(IsLeafNode(Node) && Node->Parent->WindowID == 0)
        {
            DEBUG("ToggleFocusedWindowParentContainer() Set Parent Container");
            Node->Parent->WindowID = Node->WindowID;
            ResizeWindowToContainerSize(Node->Parent);
        }
        else
        {
            DEBUG("ToggleFocusedWindowParentContainer() Restore Window Container");
            Node->Parent->WindowID = 0;
            ResizeWindowToContainerSize(Node);
        }
    }
}

void ToggleFocusedWindowFullscreen()
{
    /* NOTE(koekeishiya): The following code works and was added due to frustration of using a pre-alpha
                          window manager during development.  */

    /* TODO(koekeishiya): Should this function be able to assume that the focused window is valid (?) */

    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(!Space->RootNode)
        return;

    if(Space->Settings.Mode != SpaceModeBSP)
        return;

    tree_node *Node = NULL;
    if(Space->RootNode->WindowID == 0)
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
        Space->RootNode->WindowID = 0;
        Node = GetTreeNodeFromWindowID(Space->RootNode, Window->ID);
        if(Node)
        {
            ResizeWindowToContainerSize(Node);
        }
    }
}

bool IsWindowFullscreen(ax_window *Window)
{
    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return false;

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    return SpaceInfo->RootNode && SpaceInfo->RootNode->WindowID == Window->ID;
}

bool IsWindowParentContainer(ax_window *Window)
{
    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return false;

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    tree_node *Node = GetTreeNodeFromWindowID(SpaceInfo->RootNode, Window->ID);
    return Node && Node->Parent && Node->Parent->WindowID == Window->ID;
}

void LockWindowToContainerSize(ax_window *Window)
{
    if(Window)
    {
        ax_display *Display = AXLibWindowDisplay(Window);
        if(!Display)
            return;

        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        tree_node *Node = GetTreeNodeFromWindowID(SpaceInfo->RootNode, Window->ID);
        if(Node)
        {
            if(IsWindowFullscreen(Window))
                ResizeWindowToContainerSize(SpaceInfo->RootNode);
            else if(IsWindowParentContainer(Window))
                ResizeWindowToContainerSize(Node->Parent);
            else
                ResizeWindowToContainerSize(Node);
        }
        else
        {
            link_node *Link = GetLinkNodeFromTree(SpaceInfo->RootNode, Window->ID);
            if(Link)
            {
                ResizeWindowToContainerSize(Link);
            }
        }
    }
}

void DetachAndReinsertWindow(unsigned int WindowID, int Degrees)
{
    if(MarkedWindow && MarkedWindow->ID == WindowID)
    {
        ax_window *FocusedWindow = FocusedApplication->Focus;
        if(MarkedWindow == FocusedWindow)
            return;

        ToggleWindowFloating(WindowID, false);
        ClearMarkedWindow();
        ToggleWindowFloating(WindowID, false);
        MoveCursorToCenterOfFocusedWindow();
    }
    else
    {
        if(WindowID == 0)
            return;

        if(MarkedWindow && MarkedWindow->ID == WindowID)
            return;

        ax_window *ClosestWindow = NULL;
        if(FindClosestWindow(Degrees, &ClosestWindow, false))
        {
            ToggleWindowFloating(WindowID, false);
            ax_window *PrevMarkedWindow = MarkedWindow;
            MarkedWindow = ClosestWindow;
            ToggleWindowFloating(WindowID, false);
            MoveCursorToCenterOfFocusedWindow();
            MarkedWindow = PrevMarkedWindow;
        }
    }
}

void SwapFocusedWindowWithMarked()
{
    ax_window *FocusedWindow = FocusedApplication->Focus;
    if(!FocusedWindow || !MarkedWindow || (FocusedWindow == MarkedWindow))
        return;

    ax_display *Display = AXLibWindowDisplay(FocusedWindow);
    if(!Display)
        return;

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    tree_node *TreeNode = GetTreeNodeFromWindowIDOrLinkNode(SpaceInfo->RootNode, FocusedWindow->ID);
    if(TreeNode)
    {
        tree_node *NewFocusNode = GetTreeNodeFromWindowID(SpaceInfo->RootNode, MarkedWindow->ID);
        if(NewFocusNode)
        {
            SwapNodeWindowIDs(TreeNode, NewFocusNode);
            MoveCursorToCenterOfFocusedWindow();
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
            if(KWMSettings.Cycle == CycleModeScreen && !ShiftNode)
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
                MoveCursorToCenterOfWindow(Window);
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
                MoveCursorToCenterOfWindow(Window);
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
            if(FindClosestWindow(Degrees, &ClosestWindow, KWMSettings.Cycle == CycleModeScreen))
                NewFocusNode = GetTreeNodeFromWindowID(Space->RootNode, ClosestWindow->ID);

            if(NewFocusNode)
            {
                SwapNodeWindowIDs(TreeNode, NewFocusNode);
                Window->Position = AXLibGetWindowPosition(Window->Ref);
                Window->Size = AXLibGetWindowSize(Window->Ref);
                MoveCursorToCenterOfWindow(Window);
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
    ax_application *Application = AXLibGetFocusedApplication();
    ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
    if(!Application || !Window)
    {
        if(Degrees == 90)
            FocusLastLeafNode(AXLibCursorDisplay());
        else if(Degrees == 270)
            FocusFirstLeafNode(AXLibCursorDisplay());

        return;
    }

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->Settings.Mode == SpaceModeBSP)
    {
        ax_window *ClosestWindow = NULL;
        if((KWMSettings.Cycle == CycleModeDisabled &&
            FindClosestWindow(Degrees, &ClosestWindow, false)) ||
           (KWMSettings.Cycle == CycleModeScreen &&
            FindClosestWindow(Degrees, &ClosestWindow, true)))
        {
            AXLibSetFocusedWindow(ClosestWindow);
            MoveCursorToCenterOfWindow(ClosestWindow);
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
    ax_application *Application = AXLibGetFocusedApplication();
    ax_window *Window = FocusedApplication ? FocusedApplication->Focus : NULL;
    if(!Application || !Window)
    {
        if(Shift == 1)
            FocusLastLeafNode(AXLibCursorDisplay());
        else if(Shift == -1)
            FocusFirstLeafNode(AXLibCursorDisplay());

        return;
    }

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
            if(KWMSettings.Cycle == CycleModeScreen && !FocusNode)
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
                MoveCursorToCenterOfFocusedWindow();
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

                if(KWMSettings.Cycle == CycleModeScreen && !FocusNode)
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

                if(KWMSettings.Cycle == CycleModeScreen && !FocusNode)
                {
                    GetLastLeafNode(SpaceInfo->RootNode, (void**)&FocusNode);
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
    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    ax_display *Display = AXLibWindowDisplay(Window);
    if(!Display)
        return;

    space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
    if(SpaceInfo->Settings.Mode == SpaceModeBSP)
    {
        link_node *Link = GetLinkNodeFromWindowID(SpaceInfo->RootNode, Window->ID);
        tree_node *Root = GetTreeNodeFromLink(SpaceInfo->RootNode, Link);
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
            tree_node *Root = GetTreeNodeFromWindowID(SpaceInfo->RootNode, Window->ID);
            if(Root)
            {
                SetWindowFocusByNode(Root->List);
                MoveCursorToCenterOfFocusedWindow();
            }
        }
    }
}

void FocusFirstLeafNode(ax_display *Display)
{
    if(Display)
    {
        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        if(SpaceInfo->Settings.Mode == SpaceModeBSP)
        {
            tree_node *Node = NULL;
            GetFirstLeafNode(SpaceInfo->RootNode, (void**)&Node);
            SetWindowFocusByNode(Node);
        }
        else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
        {
            link_node *Node = SpaceInfo->RootNode->List;
            SetWindowFocusByNode(Node);
        }
    }
}

void FocusLastLeafNode(ax_display *Display)
{
    if(Display)
    {
        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        if(SpaceInfo->Settings.Mode == SpaceModeBSP)
        {
            tree_node *Node = NULL;
            GetLastLeafNode(SpaceInfo->RootNode, (void **)&Node);
            SetWindowFocusByNode(Node);
        }
        else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
        {
            link_node *Node = SpaceInfo->RootNode->List;
            while(Node && Node->Next)
                Node = Node->Next;

            SetWindowFocusByNode(Node);
        }
    }
}

void FocusWindowByID(uint32_t WindowID)
{
    ax_window *Window = GetWindowByID(WindowID);
    if(Window)
    {
        AXLibSetFocusedWindow(Window);
    }
}

void ClearMarkedWindow()
{
    MarkedWindow = NULL;
    ClearBorder(&MarkedBorder);
}

void MarkWindowContainer(ax_window *Window)
{
    if(Window)
    {
        if(MarkedWindow && MarkedWindow->ID == Window->ID)
        {
            DEBUG("MarkWindowContainer() Unmarked " << Window->Name);
            ClearMarkedWindow();
        }
        else
        {
            DEBUG("MarkWindowContainer() Marked " << Window->Name);
            MarkedWindow = Window;
            UpdateBorder("marked");
        }
    }
}

void MarkFocusedWindowContainer()
{
    MarkWindowContainer(FocusedApplication->Focus);
}

void SetWindowFocusByNode(tree_node *Node)
{
    if(Node)
    {
        ax_window *Window = GetWindowByID(Node->WindowID);
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
        ax_window *Window = GetWindowByID(Link->WindowID);
        if(Window)
        {
            DEBUG("SetWindowFocusByNode()");
            AXLibSetFocusedWindow(Window);
        }
    }
}

void CenterWindowInsideNodeContainer(ax_window *Window, int *Xptr, int *Yptr, int *Wptr, int *Hptr)
{
    CGPoint WindowOrigin = AXLibGetWindowPosition(Window->Ref);
    CGSize WindowOGSize = AXLibGetWindowSize(Window->Ref);

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

        AXLibAddFlags(Window, AXWindow_MoveIntrinsic);
        AXLibSetWindowPosition(Window->Ref, X, Y);

        AXLibAddFlags(Window, AXWindow_SizeIntrinsic);
        AXLibSetWindowSize(Window->Ref, Width, Height);
    }
}

void SetWindowDimensions(ax_window *Window, int X, int Y, int Width, int Height)
{
    AXLibAddFlags(Window, AXWindow_MoveIntrinsic);
    AXLibSetWindowPosition(Window->Ref, X, Y);

    AXLibAddFlags(Window, AXWindow_SizeIntrinsic);
    AXLibSetWindowSize(Window->Ref, Width, Height);

    CenterWindowInsideNodeContainer(Window, &X, &Y, &Width, &Height);
}

void CenterWindow(ax_display *Display, ax_window *Window)
{
    int NewX = Display->Frame.origin.x + Display->Frame.size.width / 4;
    int NewY = Display->Frame.origin.y + Display->Frame.size.height / 4;
    int NewWidth = Display->Frame.size.width / 2;
    int NewHeight = Display->Frame.size.height / 2;
    SetWindowDimensions(Window, NewX, NewY, NewWidth, NewHeight);
}

ax_window *GetWindowByID(uint32_t WindowID)
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

void MoveFloatingWindow(int X, int Y)
{
    ax_window *Window = FocusedApplication->Focus;
    if(!Window)
        return;

    if(AXLibHasFlags(Window, AXWindow_Floating))
    {
        AXLibSetWindowPosition(Window->Ref,
                               Window->Position.x + X,
                               Window->Position.y + Y);
    }
}

