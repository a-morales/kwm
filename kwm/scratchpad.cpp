#include "scratchpad.h"
#include "display.h"
#include "space.h"
#include "window.h"
#include "axlib/axlib.h"

extern kwm_settings KWMSettings;
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
    if(!AXLibIsSpaceTransitionInProgress() &&
       IsWindowOnScratchpad(Window))
    {
        if(AXLibHasFlags(Window, AXWindow_Floating))
            AXLibClearFlags(Window, AXWindow_Floating);

        ax_display *Display = AXLibWindowDisplay(Window);
        if(Display)
        {
            if(!AXLibSpaceHasWindow(Window, Display->Space->ID))
                AXLibSpaceAddWindow(Display->Space->ID, Window->ID);

            AddWindowToNodeTree(Display, Window->ID);
            if(KWMSettings.Focus == FocusModeStandby)
                KWMSettings.Focus = FocusModeAutoraise;
        }

        int Slot = GetScratchpadSlotOfWindow(Window);
        Scratchpad.Windows.erase(Slot);
        DEBUG("RemoveWindowFromScratchpad() " << Slot);
    }
}

void ToggleScratchpadWindow(int Index)
{
    if(!AXLibIsSpaceTransitionInProgress() &&
       IsScratchpadSlotValid(Index))
    {
        ax_window *Window = Scratchpad.Windows[Index];
        ax_display *Display = AXLibWindowDisplay(Window);
        if(Display)
        {
            if(AXLibSpaceHasWindow(Window, Display->Space->ID))
                HideScratchpadWindow(Index);
            else
                ShowScratchpadWindow(Index);
        }
    }
}

void HideScratchpadWindow(int Index)
{
    if(!AXLibIsSpaceTransitionInProgress() &&
       IsScratchpadSlotValid(Index))
    {
        ax_window *Window = Scratchpad.Windows[Index];
        ax_display *Display = AXLibWindowDisplay(Window);
        if(Display)
        {
            if(!AXLibHasFlags(Window, AXWindow_Floating))
                AXLibAddFlags(Window, AXWindow_Floating);

            RemoveWindowFromNodeTree(Display, Window->ID);
            AXLibSpaceRemoveWindow(Display->Space->ID, Window->ID);
            if(KWMSettings.Focus == FocusModeStandby)
                KWMSettings.Focus = FocusModeAutoraise;

            if(Scratchpad.LastFocus != -1)
                FocusWindowByID(Scratchpad.LastFocus);
        }
    }
}

void ShowScratchpadWindow(int Index)
{
    if(!AXLibIsSpaceTransitionInProgress() &&
       IsScratchpadSlotValid(Index))
    {
        ax_application *Application = AXLibGetFocusedApplication();
        if(!Application)
            return;

        ax_window *FocusedWindow = Application->Focus;
        if(FocusedWindow)
            Scratchpad.LastFocus = FocusedWindow->ID;

        ax_window *Window = Scratchpad.Windows[Index];
        ax_display *Display = AXLibWindowDisplay(Window);
        if(Display)
        {
            AXLibSpaceAddWindow(Display->Space->ID, Window->ID);
            ResizeScratchpadWindow(Display, Window);
            FocusWindowByID(Window->ID);
        }
    }
}

void ResizeScratchpadWindow(ax_display *Display, ax_window *Window)
{
    int NewX = Display->Frame.origin.x + Display->Frame.size.width * 0.125;
    int NewY = Display->Frame.origin.y + Display->Frame.size.height * 0.125;
    int NewWidth = Display->Frame.size.width * 0.75;
    int NewHeight = Display->Frame.size.height * 0.75;
    SetWindowDimensions(Window->Ref, NewX, NewY, NewWidth, NewHeight);
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
