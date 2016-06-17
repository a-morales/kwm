#include "element.h"

#define internal static
#define local_persist static

CFTypeRef AXLibGetWindowProperty(AXUIElementRef WindowRef, CFStringRef Property)
{
    CFTypeRef TypeRef;
    AXError Error = AXUIElementCopyAttributeValue(WindowRef, Property, &TypeRef);
    return (Error == kAXErrorSuccess) ? TypeRef : NULL;
}

AXError AXLibSetWindowProperty(AXUIElementRef WindowRef, CFStringRef Property, CFTypeRef Value)
{
    return AXUIElementSetAttributeValue(WindowRef, Property, Value);
}

bool AXLibIsWindowMinimized(AXUIElementRef WindowRef)
{
    bool Result = true;

    CFBooleanRef Value = (CFBooleanRef) AXLibGetWindowProperty(WindowRef, kAXMinimizedAttribute);
    if(Value)
    {
        Result = CFBooleanGetValue(Value);
        CFRelease(Value);
    }

    return Result;
}

bool AXLibIsWindowMovable(AXUIElementRef WindowRef)
{
    bool Result;

    AXError Error = AXUIElementIsAttributeSettable(WindowRef, kAXPositionAttribute, (Boolean*)&Result);
    if(Error != kAXErrorSuccess)
        Result = false;

    return Result;
}

bool AXLibIsWindowResizable(AXUIElementRef WindowRef)
{
    bool Result;

    AXError Error = AXUIElementIsAttributeSettable(WindowRef, kAXSizeAttribute, (Boolean*)&Result);
    if(Error != kAXErrorSuccess)
        Result = false;

    return Result;
}

bool AXLibSetWindowPosition(AXUIElementRef WindowRef, int X, int Y)
{
    bool Result = false;

    CGPoint WindowPos = CGPointMake(X, Y);
    CFTypeRef WindowPosRef = (CFTypeRef)AXValueCreate(kAXValueCGPointType, (const void*)&WindowPos);
    if(WindowPosRef)
    {
        Result = AXLibSetWindowProperty(WindowRef, kAXPositionAttribute, WindowPosRef);
        CFRelease(WindowPosRef);
    }

    return Result;
}

bool AXLibSetWindowSize(AXUIElementRef WindowRef, int Width, int Height)
{
    bool Result = false;

    CGSize WindowSize = CGSizeMake(Width, Height);
    CFTypeRef WindowSizeRef = (CFTypeRef)AXValueCreate(kAXValueCGSizeType, (void*)&WindowSize);
    if(WindowSizeRef)
    {
        Result = AXLibSetWindowProperty(WindowRef, kAXSizeAttribute, WindowSizeRef);
        CFRelease(WindowSizeRef);
    }

    return Result;
}

/* NOTE(koekeishiya): If a window is minimized when we call this function, the WindowID returned is 0. */
uint32_t AXLibGetWindowID(AXUIElementRef WindowRef)
{
    uint32_t WindowID = kCGNullWindowID;
    _AXUIElementGetWindow(WindowRef, &WindowID);
    return WindowID;
}

std::string AXLibGetWindowTitle(AXUIElementRef WindowRef)
{
    CFStringRef WindowTitleRef = (CFStringRef) AXLibGetWindowProperty(WindowRef, kAXTitleAttribute);
    std::string WindowTitle;

    if(WindowTitleRef)
    {
        WindowTitle = GetUTF8String(WindowTitleRef);
        if(WindowTitle.empty())
            WindowTitle = CFStringGetCStringPtr(WindowTitleRef, kCFStringEncodingMacRoman);

        CFRelease(WindowTitleRef);
    }

    return WindowTitle;
}

CGPoint AXLibGetWindowPosition(AXUIElementRef WindowRef)
{
    CGPoint WindowPos = {};
    AXValueRef WindowPosRef = (AXValueRef) AXLibGetWindowProperty(WindowRef, kAXPositionAttribute);

    if(WindowPosRef)
    {
        AXValueGetValue(WindowPosRef, kAXValueCGPointType, &WindowPos);
        CFRelease(WindowPosRef);
    }

    return WindowPos;
}

CGSize AXLibGetWindowSize(AXUIElementRef WindowRef)
{
    CGSize WindowSize = {};
    AXValueRef WindowSizeRef = (AXValueRef) AXLibGetWindowProperty(WindowRef, kAXSizeAttribute);

    if(WindowSizeRef)
    {
        AXValueGetValue(WindowSizeRef, kAXValueCGSizeType, &WindowSize);
        CFRelease(WindowSizeRef);
    }

    return WindowSize;
}

bool AXLibGetWindowRole(AXUIElementRef WindowRef, CFTypeRef *Role)
{
    *Role = AXLibGetWindowProperty(WindowRef, kAXRoleAttribute);
    return *Role != NULL;
}

bool AXLibGetWindowSubrole(AXUIElementRef WindowRef, CFTypeRef *Subrole)
{
    *Subrole = AXLibGetWindowProperty(WindowRef, kAXSubroleAttribute);
    return *Subrole != NULL;
}

std::string
GetUTF8String(CFStringRef Temp)
{
    std::string Result;

    if(!CFStringGetCStringPtr(Temp, kCFStringEncodingUTF8))
    {
        CFIndex Length = CFStringGetLength(Temp);
        CFIndex Bytes = 4 * Length + 1;
        char *TempUTF8StringPtr = (char*) malloc(Bytes);

        CFStringGetCString(Temp, TempUTF8StringPtr, Bytes, kCFStringEncodingUTF8);
        if(TempUTF8StringPtr)
        {
            Result = TempUTF8StringPtr;
            free(TempUTF8StringPtr);
        }
    }

    return Result;
}
