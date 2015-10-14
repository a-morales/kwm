#include "kwm.h"

static CGWindowListOption OsxWindowListOption = kCGWindowListOptionOnScreenOnly | 
                                                   kCGWindowListExcludeDesktopElements;

extern std::vector<window_info> WindowLst;
extern ProcessSerialNumber FocusedPSN;
extern AXUIElementRef FocusedWindowRef;
extern window_info FocusedWindow;
extern bool EnableAutoraise;

bool WindowsAreEqual(window_info *Window, window_info *Match)
{
    if(Window->X == Match->X &&
        Window->Y == Match->Y &&
        Window->Width == Match->Width &&
        Window->Height == Match->Height &&
        Window->Name == Match->Name)
            return true;

    return false;
}

bool IsWindowBelowCursor(window_info *Window)
{
    CGEventRef Event = CGEventCreate(NULL);
    CGPoint Cursor = CGEventGetLocation(Event);
    CFRelease(Event);
    if(Cursor.x >= Window->X && 
            Cursor.x <= Window->X + Window->Width && 
            Cursor.y >= Window->Y && 
            Cursor.y <= Window->Y + Window->Height)
        return true;
        
    return false;
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
                if(!WindowsAreEqual(&FocusedWindow, &WindowLst[i]))
                {
                    ProcessSerialNumber NewPSN;
                    GetProcessForPID(WindowLst[i].PID, &NewPSN);

                    FocusedPSN = NewPSN;
                    FocusedWindow = WindowLst[i];

                    AXUIElementRef WindowRef;
                    if(GetWindowRef(&FocusedWindow, &WindowRef))
                    {
                        AXUIElementSetAttributeValue(WindowRef, kAXMainAttribute, kCFBooleanTrue);
                        AXUIElementSetAttributeValue(WindowRef, kAXFocusedAttribute, kCFBooleanTrue);
                        AXUIElementPerformAction (WindowRef, kAXRaiseAction);

                        if(FocusedWindowRef != NULL)
                            CFRelease(FocusedWindowRef);

                        FocusedWindowRef = WindowRef;
                        std::cout << "current space: " << GetSpaceOfWindow(&FocusedWindow) << std::endl;
                    }

                    if(EnableAutoraise)
                        SetFrontProcessWithOptions(&FocusedPSN, kSetFrontProcessFrontWindowOnly);

                    std::cout << "Keyboard focus: " << FocusedWindow.PID << std::endl;
                    if(FocusedWindow.Layout)
                        std::cout << FocusedWindow.Layout->Name << std::endl;
                }
                break;
            }
        }
    }
}

bool GetExpressionFromShiftDirection(window_info *Window, const std::string &Direction)
{
    GetLayoutOfWindow(Window);
    if(!Window->Layout)
        return false;

    int Shift = 0;
    if(Direction == "prev")
        Shift = -1;
    else if(Direction == "next")
        Shift = 1;

    return (Window->LayoutIndex == FocusedWindow.LayoutIndex + Shift);
}

void ShiftWindowFocus(const std::string &Direction)
{
    window_layout *FocusedWindowLayout = GetLayoutOfWindow(&FocusedWindow);
    if(!FocusedWindowLayout)
        return;

    int ScreenIndex = GetDisplayOfWindow(&FocusedWindow)->ID;
    std::vector<window_info*> ScreenWindowLst = GetAllWindowsOnDisplay(ScreenIndex);
    for(int WindowIndex = 0; WindowIndex < ScreenWindowLst.size(); ++WindowIndex)
    {
        window_info *Window = ScreenWindowLst[WindowIndex];
        if(GetExpressionFromShiftDirection(Window, Direction))
        {
            AXUIElementRef WindowRef;
            if(GetWindowRef(Window, &WindowRef))
            {
                ProcessSerialNumber NewPSN;
                GetProcessForPID(Window->PID, &NewPSN);

                AXUIElementSetAttributeValue(WindowRef, kAXMainAttribute, kCFBooleanTrue);
                AXUIElementSetAttributeValue(WindowRef, kAXFocusedAttribute, kCFBooleanTrue);
                AXUIElementPerformAction (WindowRef, kAXRaiseAction);

                SetFrontProcessWithOptions(&NewPSN, kSetFrontProcessFrontWindowOnly);

                if(FocusedWindowRef != NULL)
                    CFRelease(FocusedWindowRef);

                FocusedWindowRef = WindowRef;
                FocusedWindow = *Window;
                FocusedPSN = NewPSN;
            }
            break;
        }
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

        GetLayoutOfWindow(Window);
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

// Currently not in use
window_info GetWindowInfoFromRef(AXUIElementRef WindowRef)
{
    window_info Info;
    Info.Name = "invalid";
    if(WindowRef != NULL)
    {

        CFStringRef WindowTitle;
        AXValueRef Temp;
        CGSize WindowSize;
        CGPoint WindowPos;

        AXUIElementCopyAttributeValue(WindowRef, kAXTitleAttribute, (CFTypeRef*)&WindowTitle);
        if(CFStringGetCStringPtr(WindowTitle, kCFStringEncodingMacRoman))
            Info.Name = CFStringGetCStringPtr(WindowTitle, kCFStringEncodingMacRoman);
        
        if(WindowTitle != NULL)
            CFRelease(WindowTitle);

        AXUIElementCopyAttributeValue(WindowRef, kAXSizeAttribute, (CFTypeRef*)&Temp);
        AXValueGetValue(Temp, kAXValueCGSizeType, &WindowSize);
        if(Temp != NULL)
            CFRelease(Temp);

        AXUIElementCopyAttributeValue(WindowRef, kAXPositionAttribute, (CFTypeRef*)&Temp);
        AXValueGetValue(Temp, kAXValueCGPointType, &WindowPos);
        if(Temp !=NULL)
            CFRelease(Temp);

        Info.X = WindowPos.x;
        Info.Y = WindowPos.y;
        Info.Width = WindowSize.width;
        Info.Height = WindowSize.height;

        return Info;
    }
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
