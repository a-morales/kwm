#include "element.h"

#define internal static
#define local_persist static
#define global_variable static

CFTypeRef AXLibGetWindowProperty(AXUIElementRef WindowRef, CFStringRef Property)
{
    CFTypeRef TypeRef;
    AXUIElementCopyAttributeValue(WindowRef, Property, &TypeRef);
    return TypeRef;
}

AXError AXLibSetWindowProperty(AXUIElementRef WindowRef, CFStringRef Property, CFTypeRef Value)
{
    return AXUIElementSetAttributeValue(WindowRef, Property, Value);
}

bool AXLibIsWindowMovable(AXUIElementRef WindowRef)
{
    bool Result = false;

    AXError Error = AXUIElementIsAttributeSettable(WindowRef, kAXPositionAttribute, (Boolean*)&Result);
    if(Error != kAXErrorSuccess)
        Result = false;

    return Result;
}

bool AXLibIsWindowResizable(AXUIElementRef WindowRef)
{
    bool Result = false;

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

int AXLibGetWindowID(AXUIElementRef WindowRef)
{
    int WindowID = -1;
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
    bool Result = false;
    *Role = AXLibGetWindowProperty(WindowRef, kAXRoleAttribute);
    return Result;
}

bool AXLibGetWindowSubrole(AXUIElementRef WindowRef, CFTypeRef *Subrole)
{
    bool Result = false;
    *Subrole = AXLibGetWindowProperty(WindowRef, kAXSubroleAttribute);
    return Result;
}

/*
void AXLibParseWindowInfo(const void *Key, const void *Value, void *Context)
{
    CFStringRef K = (CFStringRef)Key;
    std::string KeyStr = CFStringGetCStringPtr(K, kCFStringEncodingMacRoman);
    CFTypeID ID = CFGetTypeID(Value);
    if(ID == CFStringGetTypeID())
    {
        CFStringRef V = (CFStringRef)Value;
        std::string ValueStr = GetUTF8String(V);
        if(ValueStr.empty() && CFStringGetCStringPtr(V, kCFStringEncodingMacRoman))
            ValueStr = CFStringGetCStringPtr(V, kCFStringEncodingMacRoman);

        if(KeyStr == "kCGWindowName")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Name = ValueStr;
        else if(KeyStr == "kCGWindowOwnerName")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Owner = ValueStr;
    }
    else if(ID == CFNumberGetTypeID())
    {
        int MyInt;
        CFNumberRef V = (CFNumberRef)Value;
        CFNumberGetValue(V, kCFNumberSInt64Type, &MyInt);
        if(KeyStr == "kCGWindowNumber")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].WID = MyInt;
        else if(KeyStr == "kCGWindowOwnerPID")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].PID = MyInt;
        else if(KeyStr == "kCGWindowLayer")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Layer = MyInt;
        else if(KeyStr == "X")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].X = MyInt;
        else if(KeyStr == "Y")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Y = MyInt;
        else if(KeyStr == "Width")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Width = MyInt;
        else if(KeyStr == "Height")
            KWMTiling.WindowLst[KWMTiling.WindowLst.size()-1].Height = MyInt;
    }
    else if(ID == CFDictionaryGetTypeID())
    {
        CFDictionaryRef Elem = (CFDictionaryRef)Value;
        CFDictionaryApplyFunction(Elem, GetWindowInfo, NULL);
    }
}
*/

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
