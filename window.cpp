#include "kwm.h"

static CGWindowListOption OsxWindowListOption = kCGWindowListOptionOnScreenOnly | 
                                                   kCGWindowListExcludeDesktopElements;

extern std::vector<window_info> WindowLst;
extern ProcessSerialNumber FocusedPSN;
extern AXUIElementRef FocusedWindowRef;
extern window_info *FocusedWindow;
extern bool EnableAutoraise;

bool WindowsAreEqual(window_info *Window, window_info *Match)
{
    bool Result = Window == Match;

    if(Window && Match)
    {
        if(Window->X == Match->X &&
                Window->Y == Match->Y &&
                Window->Width == Match->Width &&
                Window->Height == Match->Height &&
                Window->Name == Match->Name)
                    Result = true;
    }

    return Result;
}

bool IsWindowBelowCursor(window_info *Window)
{
    bool Result = Window == NULL;

    if(Window)
    {
        CGEventRef Event = CGEventCreate(NULL);
        CGPoint Cursor = CGEventGetLocation(Event);
        CFRelease(Event);
        if(Cursor.x >= Window->X && 
                Cursor.x <= Window->X + Window->Width && 
                Cursor.y >= Window->Y && 
                Cursor.y <= Window->Y + Window->Height)
                    Result = true;
    }
        
    return Result;
}

void DetectWindowBelowCursor()
{
    WindowLst.clear();
    CFArrayRef OsxWindowLst = CGWindowListCopyWindowInfo(OsxWindowListOption, kCGNullWindowID);
    if(OsxWindowLst)
    {
        CFIndex OsxWindowCount = CFArrayGetCount(OsxWindowLst);
        for(CFIndex WindowIndex = 0; WindowIndex < OsxWindowCount; ++WindowIndex)
        {
            CFDictionaryRef Elem = (CFDictionaryRef)CFArrayGetValueAtIndex(OsxWindowLst, WindowIndex);
            WindowLst.push_back(window_info());
            CFDictionaryApplyFunction(Elem, GetWindowInfo, NULL);
        }
        CFRelease(OsxWindowLst);

        for(int i = 0; i < WindowLst.size(); ++i)
        {
            if(IsWindowBelowCursor(&WindowLst[i]))
            {
                if(!WindowsAreEqual(FocusedWindow, &WindowLst[i]))
                {
                    SetWindowFocus(&WindowLst[i]);
                }
                break;
            }
        }
    }
}

bool GetExpressionFromShiftDirection(window_info *Window, const std::string &Direction)
{
    int Shift = 0;
    if(Direction == "prev")
        Shift = -1;
    else if(Direction == "next")
        Shift = 1;

    return (GetLayoutIndexOfWindow(Window) == GetLayoutIndexOfWindow(FocusedWindow) + Shift);
}

void ShiftWindowFocus(const std::string &Direction)
{
    int ScreenIndex = GetDisplayOfWindow(FocusedWindow)->ID;
    std::vector<window_info*> ScreenWindowLst = GetAllWindowsOnDisplay(ScreenIndex);
    for(int WindowIndex = 0; WindowIndex < ScreenWindowLst.size(); ++WindowIndex)
    {
        window_info *Window = ScreenWindowLst[WindowIndex];
        if(GetExpressionFromShiftDirection(Window, Direction))
        {
            SetWindowFocus(Window);
            break;
        }
    }
}

void CloseFocusedWindow()
{
    DEBUG("Closing window: " << FocusedWindow->Name)
    if(FocusedWindow)
    {
        CloseWindowByRef(FocusedWindowRef);
        CFRelease(FocusedWindowRef);
        FocusedWindowRef = NULL;
        FocusedWindow = NULL;
        DetectWindowBelowCursor();
    }
}

void CloseWindowByRef(AXUIElementRef WindowRef)
{
    AXUIElementRef ActionClose;
    AXUIElementCopyAttributeValue(WindowRef, kAXCloseButtonAttribute, (CFTypeRef*)&ActionClose);
    AXUIElementPerformAction(ActionClose, kAXPressAction);
}

void CloseWindow(window_info *Window)
{
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
    {
        CloseWindowByRef(WindowRef);
    }
}

void SetWindowRefFocus(AXUIElementRef WindowRef, window_info *Window)
{
    ProcessSerialNumber NewPSN;
    GetProcessForPID(Window->PID, &NewPSN);

    FocusedPSN = NewPSN;
    FocusedWindow = Window;

    AXUIElementSetAttributeValue(WindowRef, kAXMainAttribute, kCFBooleanTrue);
    AXUIElementSetAttributeValue(WindowRef, kAXFocusedAttribute, kCFBooleanTrue);
    AXUIElementPerformAction (WindowRef, kAXRaiseAction);

    if(FocusedWindowRef != NULL)
        CFRelease(FocusedWindowRef);

    FocusedWindowRef = WindowRef;
    DEBUG("Focused Window: " << FocusedWindow->Name << "| Workspace: " << GetSpaceOfWindow(FocusedWindow))

    if(EnableAutoraise)
        SetFrontProcessWithOptions(&FocusedPSN, kSetFrontProcessFrontWindowOnly);
}

void SetWindowFocus(window_info *Window)
{
    AXUIElementRef WindowRef;
    if(GetWindowRef(Window, &WindowRef))
    {
        SetWindowRefFocus(WindowRef, Window);
    }
}

void SetWindowDimensions(AXUIElementRef WindowRef, window_info *Window, int X, int Y, int Width, int Height)
{
    if(WindowRef != NULL)
    {
        CGPoint WindowPos = CGPointMake(X, Y);
        CFTypeRef NewWindowPos = (CFTypeRef)AXValueCreate(kAXValueCGPointType, (const void*)&WindowPos);

        CGSize WindowSize = CGSizeMake(Width, Height);
        CFTypeRef NewWindowSize = (CFTypeRef)AXValueCreate(kAXValueCGSizeType, (void*)&WindowSize);

        if(AXUIElementSetAttributeValue(WindowRef, kAXPositionAttribute, NewWindowPos) == kAXErrorSuccess)
        {
            Window->X = WindowPos.x;
            Window->Y = WindowPos.y;
        }

        if(AXUIElementSetAttributeValue(WindowRef, kAXSizeAttribute, NewWindowSize) == kAXErrorSuccess)
        {
            Window->Width = WindowSize.width;
            Window->Height = WindowSize.height;
        }

        if(NewWindowPos != NULL)
            CFRelease(NewWindowPos);
        if(NewWindowSize != NULL)
            CFRelease(NewWindowSize);
    }
}

bool GetWindowRef(window_info *Window, AXUIElementRef *WindowRef)
{
    AXUIElementRef App = AXUIElementCreateApplication(Window->PID);
    CFArrayRef AppWindowLst;
    bool Result = false;
    
    if(App)
    {
        AXError Error = AXUIElementCopyAttributeValue(App, kAXWindowsAttribute, (CFTypeRef*)&AppWindowLst);
        if(Error == kAXErrorSuccess)
        {
            CFIndex DoNotFree = -1;
            CFIndex AppWindowCount = CFArrayGetCount(AppWindowLst);
            for(CFIndex WindowIndex = 0; WindowIndex < AppWindowCount; ++WindowIndex)
            {
                AXUIElementRef AppWindowRef = (AXUIElementRef)CFArrayGetValueAtIndex(AppWindowLst, WindowIndex);
                if(AppWindowRef != NULL)
                {
                    if(!Result)
                    {
                        AXValueRef Temp;

                        CFStringRef AppWindowTitle;
                        AXUIElementCopyAttributeValue(AppWindowRef, kAXTitleAttribute, (CFTypeRef*)&AppWindowTitle);

                        std::string AppWindowName;
                        if(CFStringGetCStringPtr(AppWindowTitle, kCFStringEncodingMacRoman))
                            AppWindowName = CFStringGetCStringPtr(AppWindowTitle, kCFStringEncodingMacRoman);
                        if(AppWindowTitle != NULL)
                            CFRelease(AppWindowTitle);

                        CGSize AppWindowSize;
                        AXUIElementCopyAttributeValue(AppWindowRef, kAXSizeAttribute, (CFTypeRef*)&Temp);
                        AXValueGetValue(Temp, kAXValueCGSizeType, &AppWindowSize);
                        if(Temp != NULL)
                            CFRelease(Temp);

                        CGPoint AppWindowPos;
                        AXUIElementCopyAttributeValue(AppWindowRef, kAXPositionAttribute, (CFTypeRef*)&Temp);
                        AXValueGetValue(Temp, kAXValueCGPointType, &AppWindowPos);
                        if(Temp != NULL)
                            CFRelease(Temp);

                        if(Window->X == AppWindowPos.x && 
                                Window->Y == AppWindowPos.y &&
                                Window->Width == AppWindowSize.width && 
                                Window->Height == AppWindowSize.height &&
                                Window->Name == AppWindowName)
                        {
                            *WindowRef = AppWindowRef;
                            DoNotFree = WindowIndex;
                            Result = true;
                        }

                    }

                    if(WindowIndex != DoNotFree)
                        CFRelease(AppWindowRef);
                }
            }
        }
        CFRelease(App);
    }

    return Result;
}

void GetWindowInfo(const void *Key, const void *Value, void *Context)
{
    CFStringRef K = (CFStringRef)Key;
    std::string KeyStr = CFStringGetCStringPtr(K, kCFStringEncodingMacRoman);

    CFTypeID ID = CFGetTypeID(Value);
    if(ID == CFStringGetTypeID())
    {
        CFStringRef V = (CFStringRef)Value;
        if(CFStringGetCStringPtr(V, kCFStringEncodingMacRoman))
        {
            std::string ValueStr = CFStringGetCStringPtr(V, kCFStringEncodingMacRoman);
            if(KeyStr == "kCGWindowName")
                WindowLst[WindowLst.size()-1].Name = ValueStr;
        }
    }
    else if(ID == CFNumberGetTypeID())
    {
        int MyInt;
        CFNumberRef V = (CFNumberRef)Value;
        CFNumberGetValue(V, kCFNumberSInt64Type, &MyInt);
        if(KeyStr == "kCGWindowNumber")
            WindowLst[WindowLst.size()-1].WID = MyInt;
        else if(KeyStr == "kCGWindowOwnerPID")
            WindowLst[WindowLst.size()-1].PID = MyInt;
        else if(KeyStr == "X")
            WindowLst[WindowLst.size()-1].X = MyInt;
        else if(KeyStr == "Y")
            WindowLst[WindowLst.size()-1].Y = MyInt;
        else if(KeyStr == "Width")
            WindowLst[WindowLst.size()-1].Width = MyInt;
        else if(KeyStr == "Height")
            WindowLst[WindowLst.size()-1].Height = MyInt;
    }
    else if(ID == CFDictionaryGetTypeID())
    {
        CFDictionaryRef Elem = (CFDictionaryRef)Value;
        CFDictionaryApplyFunction(Elem, GetWindowInfo, NULL);
    } 
}
