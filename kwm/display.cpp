#include "display.h"
#include "space.h"
#include "container.h"
#include "tree.h"
#include "window.h"
#include "helpers.h"
#include "cursor.h"

extern std::map<CFStringRef, space_info> WindowTree;
extern kwm_settings KWMSettings;

void SetDefaultPaddingOfDisplay(container_offset Offset)
{
    KWMSettings.DefaultOffset.PaddingTop = Offset.PaddingTop;
    KWMSettings.DefaultOffset.PaddingBottom = Offset.PaddingBottom;
    KWMSettings.DefaultOffset.PaddingLeft = Offset.PaddingLeft;
    KWMSettings.DefaultOffset.PaddingRight = Offset.PaddingRight;
}

void SetDefaultGapOfDisplay(container_offset Offset)
{
    KWMSettings.DefaultOffset.VerticalGap = Offset.VerticalGap;
    KWMSettings.DefaultOffset.HorizontalGap = Offset.HorizontalGap;
}

void ChangePaddingOfDisplay(const std::string &Side, int Offset)
{
    ax_display *Display = AXLibCursorDisplay();
    if(!Display)
        return;

    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(Side == "all")
    {
        if(Space->Settings.Offset.PaddingLeft + Offset >= 0)
            Space->Settings.Offset.PaddingLeft += Offset;

        if(Space->Settings.Offset.PaddingRight + Offset >= 0)
            Space->Settings.Offset.PaddingRight += Offset;

        if(Space->Settings.Offset.PaddingTop + Offset >= 0)
            Space->Settings.Offset.PaddingTop += Offset;

        if(Space->Settings.Offset.PaddingBottom + Offset >= 0)
            Space->Settings.Offset.PaddingBottom += Offset;
    }
    else if(Side == "left")
    {
        if(Space->Settings.Offset.PaddingLeft + Offset >= 0)
            Space->Settings.Offset.PaddingLeft += Offset;
    }
    else if(Side == "right")
    {
        if(Space->Settings.Offset.PaddingRight + Offset >= 0)
            Space->Settings.Offset.PaddingRight += Offset;
    }
    else if(Side == "top")
    {
        if(Space->Settings.Offset.PaddingTop + Offset >= 0)
            Space->Settings.Offset.PaddingTop += Offset;
    }
    else if(Side == "bottom")
    {
        if(Space->Settings.Offset.PaddingBottom + Offset >= 0)
            Space->Settings.Offset.PaddingBottom += Offset;
    }

    UpdateSpaceOfDisplay(Display, Space);
}

void ChangeGapOfDisplay(const std::string &Side, int Offset)
{
    ax_display *Display = AXLibCursorDisplay();
    if(!Display)
        return;

    space_info *Space = &WindowTree[Display->Space->Identifier];
    if(Side == "all")
    {
        if(Space->Settings.Offset.VerticalGap + Offset >= 0)
            Space->Settings.Offset.VerticalGap += Offset;

        if(Space->Settings.Offset.HorizontalGap + Offset >= 0)
            Space->Settings.Offset.HorizontalGap += Offset;
    }
    else if(Side == "vertical")
    {
        if(Space->Settings.Offset.VerticalGap + Offset >= 0)
            Space->Settings.Offset.VerticalGap += Offset;
    }
    else if(Side == "horizontal")
    {
        if(Space->Settings.Offset.HorizontalGap + Offset >= 0)
            Space->Settings.Offset.HorizontalGap += Offset;
    }

    UpdateSpaceOfDisplay(Display, Space);
}

void MoveWindowToDisplay(ax_window *Window, int Shift, bool Relative)
{
    ax_display *Display = AXLibWindowDisplay(Window);
    ax_display *NewDisplay = NULL;
    if(Relative)
        NewDisplay = Shift == 1 ? AXLibNextDisplay(Display) : AXLibPreviousDisplay(Display);
    else
        NewDisplay = AXLibArrangementDisplay(Shift);

    if(NewDisplay && NewDisplay != Display)
    {
        if(AXLibHasFlags(Window, AXWindow_Floating))
        {
            CenterWindow(NewDisplay, Window);
        }
        else
        {
            RemoveWindowFromNodeTree(Display, Window->ID);
            AddWindowToInactiveNodeTree(NewDisplay, Window->ID);
        }
    }
}

container_offset CreateDefaultScreenOffset()
{
    container_offset Offset = { 40, 20, 20, 20, 10, 10 };
    return Offset;
}

space_settings *GetSpaceSettingsForDisplay(unsigned int ScreenID)
{
    std::map<unsigned int, space_settings>::iterator It = KWMSettings.DisplaySettings.find(ScreenID);
    if(It != KWMSettings.DisplaySettings.end())
        return &It->second;
    else
        return NULL;
}

void UpdateSpaceOfDisplay(ax_display *Display, space_info *Space)
{
    if(Space->RootNode)
    {
        if(Space->Settings.Mode == SpaceModeBSP)
        {
            SetRootNodeContainer(Display, Space->RootNode);
            CreateNodeContainers(Display, Space->RootNode, true);
        }
        else if(Space->Settings.Mode == SpaceModeMonocle)
        {
            link_node *Link = Space->RootNode->List;
            while(Link)
            {
                SetLinkNodeContainer(Display, Link);
                Link = Link->Next;
            }
        }

        ApplyTreeNodeContainer(Space->RootNode);
        Space->NeedsUpdate = false;
    }
}

void FocusDisplay(ax_display *Display)
{
    if(Display)
    {
        space_info *SpaceInfo = &WindowTree[Display->Space->Identifier];
        if(SpaceInfo->RootNode)
        {
            if(SpaceInfo->Settings.Mode == SpaceModeBSP)
            {
                tree_node *Node = NULL;
                GetFirstLeafNode(SpaceInfo->RootNode, (void**)&Node);
                if(Node)
                {
                    FocusWindowByID(Node->WindowID);
                    int X  = Node->Container.X + (Node->Container.Width / 2);
                    int Y  = Node->Container.Y + (Node->Container.Height / 2);
                    CGWarpMouseCursorPosition(CGPointMake(X, Y));
                }
            }
            else if(SpaceInfo->Settings.Mode == SpaceModeMonocle)
            {
                int X  = Display->Frame.origin.x + (Display->Frame.size.width / 2);
                int Y  = Display->Frame.origin.y + (Display->Frame.size.height / 2);
                CGWarpMouseCursorPosition(CGPointMake(X, Y));
                FocusWindowBelowCursor();
            }
        }
        else
        {
            int X  = Display->Frame.origin.x + (Display->Frame.size.width / 2);
            int Y  = Display->Frame.origin.y + (Display->Frame.size.height / 2);
            CGWarpMouseCursorPosition(CGPointMake(X, Y));
        }
    }
}
