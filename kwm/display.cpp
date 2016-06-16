#include "display.h"
#include "space.h"
#include "container.h"
#include "tree.h"
#include "window.h"
#include "helpers.h"
#include "cursor.h"

extern std::map<CFStringRef, space_info> WindowTree;
extern kwm_screen KWMScreen;
extern kwm_tiling KWMTiling;

void SetDefaultPaddingOfDisplay(container_offset Offset)
{
    KWMScreen.DefaultOffset.PaddingTop = Offset.PaddingTop;
    KWMScreen.DefaultOffset.PaddingBottom = Offset.PaddingBottom;
    KWMScreen.DefaultOffset.PaddingLeft = Offset.PaddingLeft;
    KWMScreen.DefaultOffset.PaddingRight = Offset.PaddingRight;
}

void SetDefaultGapOfDisplay(container_offset Offset)
{
    KWMScreen.DefaultOffset.VerticalGap = Offset.VerticalGap;
    KWMScreen.DefaultOffset.HorizontalGap = Offset.HorizontalGap;
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

/* TODO(koekeishiya): Make this work for ax_window */
void MoveWindowToDisplay(window_info *Window, int Shift, bool Relative)
{
    /*
    int NewScreenIndex = -1;

    if(Relative)
        NewScreenIndex = Shift == 1 ? GetIndexOfNextScreen() : GetIndexOfPrevScreen();
    else
        NewScreenIndex = Shift;

    screen_info *NewScreen = GetDisplayFromScreenID(NewScreenIndex);
    if(NewScreen && NewScreen != KWMScreen.Current)
    {
        space_info *SpaceOfWindow = GetActiveSpaceOfScreen(KWMScreen.Current);
        SpaceOfWindow->FocusedWindowID = -1;

        if(IsWindowFloating(Window->WID, NULL))
            CenterWindow(NewScreen, Window);
        else
            AddWindowToTreeOfUnfocusedMonitor(NewScreen, Window);
    }
    */
}

container_offset CreateDefaultScreenOffset()
{
    container_offset Offset = { 40, 20, 20, 20, 10, 10 };
    return Offset;
}

space_settings *GetSpaceSettingsForDisplay(unsigned int ScreenID)
{
    std::map<unsigned int, space_settings>::iterator It = KWMTiling.DisplaySettings.find(ScreenID);
    if(It != KWMTiling.DisplaySettings.end())
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

