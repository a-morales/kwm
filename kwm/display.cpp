#include "display.h"
#include "space.h"
#include "container.h"
#include "tree.h"
#include "window.h"
#include "helpers.h"
#include "cursor.h"

extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_tiling KWMTiling;
extern kwm_thread KWMThread;
extern kwm_toggles KWMToggles;

unsigned int GetDisplayIDFromIdentifier(CFStringRef Identifier) { }

void UpdateDisplayIDForDisplay(int OldDisplayIndex, int NewDisplayIndex) { }

void DisplayReconfigurationCallBack(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void *UserInfo) { }

screen_info CreateDefaultScreenInfo(int DisplayIndex, int ScreenIndex) { }

void UpdateExistingScreenInfo(screen_info *Screen, int DisplayIndex, int ScreenIndex) { }

CFStringRef GetDisplayIdentifier(int DisplayID) { }

void GetActiveDisplays() { }

void RefreshActiveDisplays(bool ShouldFocusScreen) { }

screen_info *GetDisplayFromScreenID(unsigned int ID) { }

screen_info *GetDisplayOfMousePointer() { }

screen_info *GetDisplayOfWindow(window_info *Window) { }

std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex) { }

void UpdateSpaceOfScreen(space_info *Space, screen_info *Screen)
{
    /*
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

        ApplyTreeNodeContainer(Space->RootNode);
        Space->NeedsUpdate = false;
    }
    */
}

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
    screen_info *Screen = GetDisplayOfMousePointer();
    space_info *Space = GetActiveSpaceOfScreen(Screen);

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

    UpdateSpaceOfScreen(Space, Screen);
}

void ChangeGapOfDisplay(const std::string &Side, int Offset)
{
    screen_info *Screen = GetDisplayOfMousePointer();
    space_info *Space = GetActiveSpaceOfScreen(Screen);

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

    UpdateSpaceOfScreen(Space, Screen);
}

int GetIndexOfNextScreen()
{
    return KWMScreen.Current->ID + 1 >= KWMScreen.ActiveCount ? 0 : KWMScreen.Current->ID + 1;
}

int GetIndexOfPrevScreen()
{
    return KWMScreen.Current->ID == 0 ? KWMScreen.ActiveCount - 1 : KWMScreen.Current->ID - 1;
}

void GiveFocusToScreen(unsigned int ScreenIndex, tree_node *FocusNode, bool Mouse, bool UpdateFocus)
{
    /*
    screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
    if(Screen && Screen != KWMScreen.Current)
    {
        KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
        KWMScreen.Current = Screen;

        Screen->ActiveSpace = GetActiveSpaceOfDisplay(Screen);
        ShouldActiveSpaceBeManaged();
        space_info *Space = GetActiveSpaceOfScreen(Screen);

        DEBUG("GiveFocusToScreen() " << ScreenIndex << \
              ": Space transition ended " << KWMScreen.PrevSpace << \
              " -> " << Screen->ActiveSpace);

        if(UpdateFocus)
        {
            if(Space->Initialized && FocusNode)
            {
                DEBUG("Populated Screen 'Window -f Focus'");

                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);
                SetWindowFocusByNode(FocusNode);
                MoveCursorToCenterOfFocusedWindow();
            }
            else if(Space->Initialized && Space->RootNode)
            {
                DEBUG("Populated Screen Key/Mouse Focus");

                UpdateActiveWindowList(Screen);
                FilterWindowList(Screen);

                bool WindowBelowCursor = IsAnyWindowBelowCursor();
                if(Mouse && !WindowBelowCursor)
                    ClearFocusedWindow();
                else if(Mouse && WindowBelowCursor)
                    FocusWindowBelowCursor();

                if(!Mouse)
                {
                    if(Space->FocusedWindowID == -1)
                    {
                        if(Space->Settings.Mode == SpaceModeBSP)
                        {
                            void *FocusNode = NULL;
                            GetFirstLeafNode(Space->RootNode, (void**)&FocusNode);
                            Space->FocusedWindowID = ((tree_node*)FocusNode)->WindowID;
                        }
                        else if(Space->Settings.Mode == SpaceModeMonocle)
                        {
                            if(Space->RootNode->List)
                                Space->FocusedWindowID = Space->RootNode->List->WindowID;
                        }
                    }

                    FocusWindowByID(Space->FocusedWindowID);
                    MoveCursorToCenterOfFocusedWindow();
                }
            }
            else
            {
                if(!Space->Initialized ||
                   Space->Settings.Mode == SpaceModeFloating ||
                   !Space->RootNode)
                {
                    DEBUG("Uninitialized Screen");
                    ClearFocusedWindow();

                    if(!Mouse)
                        CGWarpMouseCursorPosition(CGPointMake(Screen->X + (Screen->Width / 2), Screen->Y + (Screen->Height / 2)));

                    if(Space->Settings.Mode != SpaceModeFloating && !Space->RootNode)
                    {
                        CGPoint ClickPos = GetCursorPos();
                        CGEventRef ClickEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, ClickPos, kCGMouseButtonLeft);
                        CGEventSetFlags(ClickEvent, 0);
                        CGEventPost(kCGHIDEventTap, ClickEvent);
                        CFRelease(ClickEvent);

                        ClickEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, ClickPos, kCGMouseButtonLeft);
                        CGEventSetFlags(ClickEvent, 0);
                        CGEventPost(kCGHIDEventTap, ClickEvent);
                        CFRelease(ClickEvent);
                    }
                }
            }
        }
    }
*/
}

void MoveWindowToDisplay(window_info *Window, int Shift, bool Relative)
{
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
}

container_offset CreateDefaultScreenOffset()
{
    container_offset Offset = { 40, 20, 20, 20, 10, 10 };
    return Offset;
}

void UpdateActiveScreen()
{
    screen_info *Screen = GetDisplayOfMousePointer();
    if(Screen && KWMScreen.Current && KWMScreen.Current != Screen)
    {
        DEBUG("UpdateActiveScreen() Active Display Changed");

        ClearMarkedWindow();
        GiveFocusToScreen(Screen->ID, NULL, true, true);
    }
}

space_settings *GetSpaceSettingsForDisplay(unsigned int ScreenID)
{
    std::map<unsigned int, space_settings>::iterator It = KWMTiling.DisplaySettings.find(ScreenID);
    if(It != KWMTiling.DisplaySettings.end())
        return &It->second;
    else
        return NULL;
}
