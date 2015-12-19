#include "kwm.h"

extern uint32_t MaxDisplayCount;
extern uint32_t ActiveDisplaysCount;
extern CGDirectDisplayID ActiveDisplays[];

extern screen_info *Screen;
extern int CurrentSpace;

extern std::map<unsigned int, screen_info> DisplayMap;
extern std::vector<window_info> WindowLst;
extern std::vector<window_info> FloatingAppLst;
extern window_info *FocusedWindow;

extern int DefaultPaddingLeft, DefaultPaddingRight;
extern int DefaultPaddingTop, DefaultPaddingBottom;
extern int DefaultGapVertical, DefaultGapHorizontal;

extern pthread_mutex_t BackgroundLock;

void DisplayReconfigurationCallBack(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void *UserInfo)
{
    pthread_mutex_lock(&BackgroundLock);

    if (Flags & kCGDisplayAddFlag)
    {
        // Display has been added
        DEBUG("New display detected! DisplayID: " << Display << " Index: " << ActiveDisplaysCount)

        DisplayMap[Display] = CreateDefaultScreenInfo(Display, ActiveDisplaysCount++);
    }
    else if (Flags & kCGDisplayRemoveFlag)
    {
        // Display has been removed
        DEBUG("Display has been removed! DisplayID: " << Display)

        std::map<int, space_info>::iterator It;
        for(It = DisplayMap[Display].Space.begin(); It != DisplayMap[Display].Space.end(); ++It)
            DestroyNodeTree(It->second.RootNode);

        DisplayMap.erase(Display);
        RefreshActiveDisplays();
    }

    pthread_mutex_unlock(&BackgroundLock);
}

screen_info CreateDefaultScreenInfo(int DisplayIndex, int ScreenIndex)
{
    CGRect DisplayRect = CGDisplayBounds(DisplayIndex);
    screen_info Screen;

    Screen.ID = ScreenIndex;
    Screen.ForceContainerUpdate = false;
    Screen.ActiveSpace = -1;
    Screen.OldWindowListCount = -1;

    Screen.X = DisplayRect.origin.x;
    Screen.Y = DisplayRect.origin.y;
    Screen.Width = DisplayRect.size.width;
    Screen.Height = DisplayRect.size.height;

    Screen.PaddingTop = DefaultPaddingTop;
    Screen.PaddingLeft = DefaultPaddingLeft;
    Screen.PaddingRight = DefaultPaddingRight;
    Screen.PaddingBottom = DefaultPaddingBottom;

    Screen.VerticalGap = DefaultGapVertical;
    Screen.HorizontalGap = DefaultGapHorizontal;

    return Screen;
}

void GetActiveDisplays()
{
    CGGetActiveDisplayList(MaxDisplayCount, (CGDirectDisplayID*)&ActiveDisplays, &ActiveDisplaysCount);
    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        unsigned int DisplayID = ActiveDisplays[DisplayIndex];
        DisplayMap[DisplayID] = CreateDefaultScreenInfo(DisplayID, DisplayIndex);;

        DEBUG("DisplayID " << DisplayID << " has index " << DisplayIndex)
    }

    Screen = GetDisplayOfMousePointer();
    CGDisplayRegisterReconfigurationCallback(DisplayReconfigurationCallBack, NULL);
}

void RefreshActiveDisplays()
{
    CGGetActiveDisplayList(MaxDisplayCount, (CGDirectDisplayID*)&ActiveDisplays, &ActiveDisplaysCount);
    for(int DisplayIndex = 0; DisplayIndex < ActiveDisplaysCount; ++DisplayIndex)
    {
        unsigned int DisplayID = ActiveDisplays[DisplayIndex];
        std::map<unsigned int, screen_info>::iterator It;
        if(It != DisplayMap.end())
            DisplayMap[DisplayID].ID = DisplayIndex;
        else
            DisplayMap[DisplayID] = CreateDefaultScreenInfo(DisplayID, DisplayIndex);

        DEBUG("DisplayID " << DisplayID << " has index " << DisplayIndex)
    }

    Screen = GetDisplayOfMousePointer();
}

screen_info *GetDisplayFromScreenID(int ID)
{
    std::map<unsigned int, screen_info>::iterator It;
    for(It = DisplayMap.begin(); It != DisplayMap.end(); ++It)
    {
        screen_info *Screen = &It->second;
        if(Screen->ID == ID)
            return Screen;
    }

    return NULL;
}

screen_info *GetDisplayOfMousePointer()
{
    CGEventRef Event = CGEventCreate(NULL);
    CGPoint Cursor = CGEventGetLocation(Event);
    CFRelease(Event);

    std::map<unsigned int, screen_info>::iterator It;
    for(It = DisplayMap.begin(); It != DisplayMap.end(); ++It)
    {
        screen_info *Screen = &It->second;
        if(Cursor.x >= Screen->X && Cursor.x <= Screen->X + Screen->Width &&
           Cursor.y >= Screen->Y && Cursor.y <= Screen->Y + Screen->Height)
               return Screen;
    }

    return NULL;
}

screen_info *GetDisplayOfWindow(window_info *Window)
{
    if(Window)
    {
        std::map<unsigned int, screen_info>::iterator It;
        for(It = DisplayMap.begin(); It != DisplayMap.end(); ++It)
        {
            screen_info *Screen = &It->second;
            if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
                return Screen;
        }
    }

    return NULL;
}

std::vector<window_info*> GetAllWindowsOnDisplay(int ScreenIndex)
{
    screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
    std::vector<window_info*> ScreenWindowLst;
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        window_info *Window = &WindowLst[WindowIndex];
        if(!IsApplicationFloating(&WindowLst[WindowIndex]))
        {
            if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
                ScreenWindowLst.push_back(Window);
        }
    }

    return ScreenWindowLst;
}

std::vector<int> GetAllWindowIDsOnDisplay(int ScreenIndex)
{
    screen_info *Screen = GetDisplayFromScreenID(ScreenIndex);
    std::vector<int> ScreenWindowIDLst;
    for(int WindowIndex = 0; WindowIndex < WindowLst.size(); ++WindowIndex)
    {
        window_info *Window = &WindowLst[WindowIndex];
        if(!IsApplicationFloating(&WindowLst[WindowIndex]))
        {
            if(Window->X >= Screen->X && Window->X <= Screen->X + Screen->Width)
                ScreenWindowIDLst.push_back(Window->WID);
        }
    }

    return ScreenWindowIDLst;
}

void SetDefaultPaddingOfDisplay(const std::string &Side, int Offset)
{
    if(Side == "left")
        DefaultPaddingLeft = Offset;
    else if(Side == "right")
        DefaultPaddingRight = Offset;
    else if(Side == "top")
        DefaultPaddingTop = Offset;
    else if(Side == "bottom")
        DefaultPaddingBottom = Offset;
}

void SetDefaultGapOfDisplay(const std::string &Side, int Offset)
{
    if(Side == "vertical")
        DefaultGapVertical = Offset;
    else if(Side == "horizontal")
        DefaultGapHorizontal = Offset;
}

void ChangePaddingOfDisplay(const std::string &Side, int Offset)
{
    screen_info *Screen = GetDisplayOfMousePointer();
    if(Side == "left")
    {
        if(Screen->Space[CurrentSpace].PaddingLeft + Offset >= 0)
            Screen->Space[CurrentSpace].PaddingLeft += Offset;
    }
    else if(Side == "right")
    {
        if(Screen->Space[CurrentSpace].PaddingRight + Offset >= 0)
            Screen->Space[CurrentSpace].PaddingRight += Offset;
    }
    else if(Side == "top")
    {
        if(Screen->Space[CurrentSpace].PaddingTop + Offset >= 0)
            Screen->Space[CurrentSpace].PaddingTop += Offset;
    }
    else if(Side == "bottom")
    {
        if(Screen->Space[CurrentSpace].PaddingBottom + Offset >= 0)
            Screen->Space[CurrentSpace].PaddingBottom += Offset;
    }

    SetRootNodeContainer(Screen, Screen->Space[CurrentSpace].RootNode);
    CreateNodeContainers(Screen, Screen->Space[CurrentSpace].RootNode, true);
    ApplyNodeContainer(Screen->Space[CurrentSpace].RootNode);
}

void ChangeGapOfDisplay(const std::string &Side, int Offset)
{
    screen_info *Screen = GetDisplayOfMousePointer();
    if(Side == "vertical")
    {
        if(Screen->Space[CurrentSpace].VerticalGap + Offset >= 0)
            Screen->Space[CurrentSpace].VerticalGap += Offset;
    }
    else if(Side == "horizontal")
    {
        if(Screen->Space[CurrentSpace].HorizontalGap + Offset >= 0)
            Screen->Space[CurrentSpace].HorizontalGap += Offset;
    }

    CreateNodeContainers(Screen, Screen->Space[CurrentSpace].RootNode, true);
    ApplyNodeContainer(Screen->Space[CurrentSpace].RootNode);
}

void CycleFocusedWindowDisplay(int Shift)
{
    screen_info *Screen = GetDisplayOfWindow(FocusedWindow);
    int NewScreenIndex = -1;

    if(Shift == 1)
    {
        NewScreenIndex = (Screen->ID + 1 >= ActiveDisplaysCount) ? 0 : Screen->ID + 1;
    }
    else if(Shift == -1)
    {
        NewScreenIndex = (Screen->ID - 1 < 0) ? ActiveDisplaysCount - 1 : Screen->ID - 1;
    }

    if(NewScreenIndex != Screen->ID)
    {
        screen_info *NewScreen = GetDisplayFromScreenID(NewScreenIndex);
        AddWindowToTreeOfUnfocusedMonitor(NewScreen);
    }
}
