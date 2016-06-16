#include "scratchpad.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "axlib/axlib.h"

extern kwm_tiling KWMTiling;
extern kwm_screen KWMScreen;
extern kwm_mode KWMMode;
extern kwm_path KWMPath;
extern scratchpad Scratchpad;

bool IsScratchpadSlotValid(int Index)
{
    std::map<int, ax_window*>::iterator It = Scratchpad.Windows.find(Index);
    return It != Scratchpad.Windows.end();
}

int GetScratchpadSlotOfWindow(ax_window *Window)
{
    int Slot = -1;
    std::map<int, ax_window*>::iterator It;

    for(It = Scratchpad.Windows.begin(); It != Scratchpad.Windows.end(); ++It)
    {
        if(It->second->ID == Window->ID)
        {
            Slot = It->first;
            break;
        }
    }

    DEBUG("GetScratchpadSlotOfWindow() " << Window->Name << " " << Slot);
    return Slot;
}

bool IsWindowOnScratchpad(ax_window *Window)
{
    return GetScratchpadSlotOfWindow(Window) != -1;
}

int GetFirstAvailableScratchpadSlot()
{
    int Slot = 0;

    if(!Scratchpad.Windows.empty())
    {
        std::map<int, ax_window*>::iterator It = Scratchpad.Windows.find(Slot);
        while(It != Scratchpad.Windows.end())
            It = Scratchpad.Windows.find(++Slot);
    }

    return Slot;
}

void AddWindowToScratchpad(ax_window *Window)
{
    if(!AXLibIsSpaceTransitionInProgress() &&
       !IsWindowOnScratchpad(Window))
    {
        int Slot = GetFirstAvailableScratchpadSlot();
        Scratchpad.Windows[Slot] = Window;
        DEBUG("AddWindowToScratchpad() " << Slot);
    }
}

void RemoveWindowFromScratchpad(ax_window *Window)
{
    /*
    if(!IsSpaceTransitionInProgress() &&
       IsWindowOnScratchpad(Window))
    {
        int Index = -1;
        if(IsWindowFloating(Window->WID, &Index))
             KWMTiling.FloatingWindowLst.erase(KWMTiling.FloatingWindowLst.begin() + Index);

        if(!IsWindowOnActiveSpace(Window->WID))
            AddWindowToSpace(KWMScreen.Current->ActiveSpace, Window->WID);

        int Slot = GetScratchpadSlotOfWindow(Window);
        Scratchpad.Windows.erase(Slot);
        DEBUG("RemoveWindowFromScratchpad() " << Slot);
    }
    */
}

void ToggleScratchpadWindow(int Index)
{
    /*
    if(!IsSpaceTransitionInProgress() &&
       IsScratchpadSlotValid(Index))
    {
        ax_window* *Window = &Scratchpad.Windows[Index];
        if(IsWindowOnActiveSpace(Window->WID))
            HideScratchpadWindow(Index);
        else
            ShowScratchpadWindow(Index);
    }
    */
}

void HideScratchpadWindow(int Index)
{
    /*
    if(!IsSpaceTransitionInProgress() &&
       IsScratchpadSlotValid(Index))
    {
        ax_window* *Window = &Scratchpad.Windows[Index];
        screen_info *Screen = GetDisplayOfWindow(Window);
        if(Screen)
        {
            if(!IsWindowFloating(Window->WID, NULL))
                KWMTiling.FloatingWindowLst.push_back(Window->WID);

            RemoveWindowFromSpace(Screen->ActiveSpace, Window->WID);
            ClearFocusedWindow();
            if(KWMMode.Focus == FocusModeStandby)
                KWMMode.Focus = FocusModeAutoraise;

            if(Scratchpad.LastFocus != -1)
                FocusWindowByID(Scratchpad.LastFocus);
        }
    }
    */
}

void ShowScratchpadWindow(int Index)
{
    /*
    if(!IsSpaceTransitionInProgress() &&
       IsScratchpadSlotValid(Index))
    {
        if(KWMFocus.Window)
            Scratchpad.LastFocus = KWMFocus.Window->WID;

        ax_window* *Window = &Scratchpad.Windows[Index];
        AddWindowToSpace(KWMScreen.Current->ActiveSpace, Window->WID);
        ResizeScratchpadWindow(KWMScreen.Current, Window);
        UpdateActiveWindowList(KWMScreen.Current);
        FocusWindowByID(Window->WID);
    }
    */
}

void ResizeScratchpadWindow(ax_display *Screen, ax_window *Window)
{
    /*
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
    {
        int NewX = Screen->X + Screen->Width * 0.125;
        int NewY = Screen->Y + Screen->Height * 0.125;
        int NewWidth = Screen->Width * 0.75;
        int NewHeight = Screen->Height * 0.75;
        SetWindowDimensions(WindowRef, Window, NewX, NewY, NewWidth, NewHeight);
    }
    */
}

void ShowAllScratchpadWindows()
{
    std::map<int, ax_window*>::iterator It;
    for(It = Scratchpad.Windows.begin(); It != Scratchpad.Windows.end(); ++It)
        ShowScratchpadWindow(It->first);
}

std::string GetWindowsOnScratchpad()
{
    std::string Result;

    int Index = 0;
    std::map<int, ax_window*>::iterator It;
    for(It = Scratchpad.Windows.begin(); It != Scratchpad.Windows.end(); ++It)
    {
        Result += std::to_string(It->first) + ": " +
                  std::to_string(It->second->ID) + ", " +
                  It->second->Application->Name + ", " +
                  It->second->Name;

        if(Index++ < Scratchpad.Windows.size() - 1)
            Result += "\n";
    }

    return Result;
}
