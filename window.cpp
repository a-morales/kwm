#include "kwm.h"

static CGWindowListOption OsxWindowListOption = kCGWindowListOptionOnScreenOnly | 
                                                   kCGWindowListExcludeDesktopElements;

extern std::vector<window_info> WindowLst;
extern ProcessSerialNumber FocusedPSN;
extern AXUIElementRef FocusedWindowRef;
extern window_info FocusedWindow;
extern bool EnableAutoraise;

bool WindowsAreEqual(window_info *window, window_info *match)
{
    if(window->x == match->x &&
        window->y == match->y &&
        window->width == match->width &&
        window->height == match->height &&
        window->name == match->name)
            return true;

    return false;
}

bool IsWindowBelowCursor(window_info *window)
{
    CGEventRef event = CGEventCreate(NULL);
    CGPoint cursor = CGEventGetLocation(event);
    CFRelease(event);
    if(cursor.x >= window->x && 
            cursor.x <= window->x + window->width && 
            cursor.y >= window->y && 
            cursor.y <= window->y + window->height)
        return true;
        
    return false;
}

void DetectWindowBelowCursor()
{
    WindowLst.clear();

    CFArrayRef osx_window_list = CGWindowListCopyWindowInfo(OsxWindowListOption, kCGNullWindowID);
    if(osx_window_list)
    {
        CFIndex osx_window_count = CFArrayGetCount(osx_window_list);
        for(CFIndex osx_window_index = 0; osx_window_index < osx_window_count; ++osx_window_index)
        {
            CFDictionaryRef elem = (CFDictionaryRef)CFArrayGetValueAtIndex(osx_window_list, osx_window_index);
            WindowLst.push_back(window_info());
            CFDictionaryApplyFunction(elem, GetWindowInfo, NULL);
        }
        CFRelease(osx_window_list);

        for(int i = 0; i < WindowLst.size(); ++i)
        {
            if(IsWindowBelowCursor(&WindowLst[i]))
            {
                if(!WindowsAreEqual(&FocusedWindow, &WindowLst[i]))
                {
                    ProcessSerialNumber newpsn;
                    GetProcessForPID(WindowLst[i].pid, &newpsn);

                    FocusedPSN = newpsn;
                    FocusedWindow = WindowLst[i];

                    AXUIElementRef window_ref;
                    if(GetWindowRef(&FocusedWindow, &window_ref))
                    {
                        AXUIElementSetAttributeValue(window_ref, kAXMainAttribute, kCFBooleanTrue);
                        AXUIElementSetAttributeValue(window_ref, kAXFocusedAttribute, kCFBooleanTrue);
                        AXUIElementPerformAction (window_ref, kAXRaiseAction);

                        if(FocusedWindowRef != NULL)
                            CFRelease(FocusedWindowRef);

                        FocusedWindowRef = window_ref;
                        std::cout << "current space: " << GetSpaceOfWindow(&FocusedWindow) << std::endl;
                    }

                    if(EnableAutoraise)
                        SetFrontProcessWithOptions(&FocusedPSN, kSetFrontProcessFrontWindowOnly);

                    std::cout << "Keyboard focus: " << FocusedWindow.pid << std::endl;
                    if(FocusedWindow.layout)
                        std::cout << FocusedWindow.layout->name << std::endl;
                }
                break;
            }
        }
    }
}

bool GetExpressionFromShiftDirection(window_info *window, const std::string &direction)
{
    GetLayoutOfWindow(window);
    if(!window->layout)
        return false;

    int shift = 0;
    if(direction == "prev")
        shift = -1;
    else if(direction == "next")
        shift = 1;

    return (window->layout_index == FocusedWindow.layout_index + shift);
}

void ShiftWindowFocus(const std::string &direction)
{
    window_layout *FocusedWindow_layout = GetLayoutOfWindow(&FocusedWindow);
    if(!FocusedWindow_layout)
        return;

    int screen_index = GetDisplayOfWindow(&FocusedWindow)->id;
    std::vector<window_info*> screen_WindowLst = GetAllWindowsOnDisplay(screen_index);
    for(int window_index = 0; window_index < screen_WindowLst.size(); ++window_index)
    {
        window_info *window = screen_WindowLst[window_index];
        if(GetExpressionFromShiftDirection(window, direction))
        {
            AXUIElementRef window_ref;
            if(GetWindowRef(window, &window_ref))
            {
                ProcessSerialNumber newpsn;
                GetProcessForPID(window->pid, &newpsn);

                AXUIElementSetAttributeValue(window_ref, kAXMainAttribute, kCFBooleanTrue);
                AXUIElementSetAttributeValue(window_ref, kAXFocusedAttribute, kCFBooleanTrue);
                AXUIElementPerformAction (window_ref, kAXRaiseAction);

                SetFrontProcessWithOptions(&newpsn, kSetFrontProcessFrontWindowOnly);

                if(FocusedWindowRef != NULL)
                    CFRelease(FocusedWindowRef);

                FocusedWindowRef = window_ref;
                FocusedWindow = *window;
                FocusedPSN = newpsn;
            }
            break;
        }
    }
}

void SetWindowDimensions(AXUIElementRef app_window, window_info *window, int x, int y, int width, int height)
{
    if(app_window != NULL)
    {
        CGPoint window_pos = CGPointMake(x, y);
        CFTypeRef new_window_pos = (CFTypeRef)AXValueCreate(kAXValueCGPointType, (const void*)&window_pos);

        CGSize window_size = CGSizeMake(width, height);
        CFTypeRef new_window_size = (CFTypeRef)AXValueCreate(kAXValueCGSizeType, (void*)&window_size);

        if(AXUIElementSetAttributeValue(app_window, kAXPositionAttribute, new_window_pos) != kAXErrorSuccess)
            std::cout << "failed to set new window position" << std::endl;
        if(AXUIElementSetAttributeValue(app_window, kAXSizeAttribute, new_window_size) != kAXErrorSuccess)
            std::cout << "failed to set new window size" << std::endl;

        window->x = window_pos.x;
        window->y = window_pos.y;
        window->width = window_size.width;
        window->height = window_size.height;

        GetLayoutOfWindow(window);

        if(new_window_pos != NULL)
            CFRelease(new_window_pos);
        if(new_window_size != NULL)
            CFRelease(new_window_size);
    }
}

bool GetWindowRef(window_info *window, AXUIElementRef *window_ref)
{
    AXUIElementRef app = AXUIElementCreateApplication(window->pid);
    CFArrayRef app_WindowLst;
    bool result = false;
    
    if(app)
    {
        AXError error = AXUIElementCopyAttributeValue(app, kAXWindowsAttribute, (CFTypeRef*)&app_WindowLst);
        if(error == kAXErrorSuccess)
        {
            CFIndex do_not_free = -1;
            CFIndex app_window_count = CFArrayGetCount(app_WindowLst);
            for(CFIndex window_index = 0; window_index < app_window_count; ++window_index)
            {
                AXUIElementRef app_window_ref = (AXUIElementRef)CFArrayGetValueAtIndex(app_WindowLst, window_index);
                if(app_window_ref != NULL)
                {
                    if(!result)
                    {
                        AXValueRef temp;

                        CFStringRef app_window_title;
                        AXUIElementCopyAttributeValue(app_window_ref, kAXTitleAttribute, (CFTypeRef*)&app_window_title);

                        std::string app_window_name;
                        if(CFStringGetCStringPtr(app_window_title, kCFStringEncodingMacRoman))
                            app_window_name = CFStringGetCStringPtr(app_window_title, kCFStringEncodingMacRoman);
                        if(app_window_title != NULL)
                            CFRelease(app_window_title);

                        CGSize app_window_size;
                        AXUIElementCopyAttributeValue(app_window_ref, kAXSizeAttribute, (CFTypeRef*)&temp);
                        AXValueGetValue(temp, kAXValueCGSizeType, &app_window_size);
                        if(temp != NULL)
                            CFRelease(temp);

                        CGPoint app_window_pos;
                        AXUIElementCopyAttributeValue(app_window_ref, kAXPositionAttribute, (CFTypeRef*)&temp);
                        AXValueGetValue(temp, kAXValueCGPointType, &app_window_pos);
                        if(temp != NULL)
                            CFRelease(temp);

                        if(window->x == app_window_pos.x && 
                                window->y == app_window_pos.y &&
                                window->width == app_window_size.width && 
                                window->height == app_window_size.height &&
                                window->name == app_window_name)
                        {
                            *window_ref = app_window_ref;
                            do_not_free = window_index;
                            result = true;
                        }

                    }

                    if(window_index != do_not_free)
                        CFRelease(app_window_ref);
                }
            }
        }
        CFRelease(app);
    }

    return result;
}

// Currently not in use
window_info GetWindowInfoFromRef(AXUIElementRef window_ref)
{
    window_info info;
    info.name = "invalid";
    if(window_ref != NULL)
    {

        CFStringRef window_title;
        AXValueRef temp;
        CGSize window_size;
        CGPoint window_pos;

        AXUIElementCopyAttributeValue(window_ref, kAXTitleAttribute, (CFTypeRef*)&window_title);
        if(CFStringGetCStringPtr(window_title, kCFStringEncodingMacRoman))
            info.name = CFStringGetCStringPtr(window_title, kCFStringEncodingMacRoman);
        
        if(window_title != NULL)
            CFRelease(window_title);

        AXUIElementCopyAttributeValue(window_ref, kAXSizeAttribute, (CFTypeRef*)&temp);
        AXValueGetValue(temp, kAXValueCGSizeType, &window_size);
        if(temp != NULL)
            CFRelease(temp);

        AXUIElementCopyAttributeValue(window_ref, kAXPositionAttribute, (CFTypeRef*)&temp);
        AXValueGetValue(temp, kAXValueCGPointType, &window_pos);
        if(temp !=NULL)
            CFRelease(temp);

        info.width = window_size.width;
        info.height = window_size.height;

        return info;
    }
}

void GetWindowInfo(const void *key, const void *value, void *context)
{
    CFStringRef k = (CFStringRef)key;
    std::string key_str = CFStringGetCStringPtr(k, kCFStringEncodingMacRoman);

    CFTypeID id = CFGetTypeID(value);
    if(id == CFStringGetTypeID())
    {
        CFStringRef v = (CFStringRef)value;
        if(CFStringGetCStringPtr(v, kCFStringEncodingMacRoman))
        {
            std::string value_str = CFStringGetCStringPtr(v, kCFStringEncodingMacRoman);
            if(key_str == "kCGWindowName")
                WindowLst[WindowLst.size()-1].name = value_str;
        }
    }
    else if(id == CFNumberGetTypeID())
    {
        int myint;
        CFNumberRef v = (CFNumberRef)value;
        CFNumberGetValue(v, kCFNumberSInt64Type, &myint);
        if(key_str == "kCGWindowNumber")
            WindowLst[WindowLst.size()-1].wid = myint;
        else if(key_str == "kCGWindowOwnerPID")
            WindowLst[WindowLst.size()-1].pid = myint;
        else if(key_str == "X")
            WindowLst[WindowLst.size()-1].x = myint;
        else if(key_str == "Y")
            WindowLst[WindowLst.size()-1].y = myint;
        else if(key_str == "Width")
            WindowLst[WindowLst.size()-1].width = myint;
        else if(key_str == "Height")
            WindowLst[WindowLst.size()-1].height = myint;
    }
    else if(id == CFDictionaryGetTypeID())
    {
        CFDictionaryRef elem = (CFDictionaryRef)value;
        CFDictionaryApplyFunction(elem, GetWindowInfo, NULL);
    } 
}
