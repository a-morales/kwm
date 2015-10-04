#include "kwm.h"

static CGWindowListOption osx_window_list_option = kCGWindowListOptionOnScreenOnly | 
                                                   kCGWindowListExcludeDesktopElements;

extern std::vector<window_info> window_lst;
extern ProcessSerialNumber focused_psn;
extern AXUIElementRef focused_window_ref;
extern window_info focused_window;
extern bool enable_auto_raise;

bool is_window_below_cursor(window_info *window)
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

void detect_window_below_cursor()
{
    window_lst.clear();

    CFArrayRef osx_window_list = CGWindowListCopyWindowInfo(osx_window_list_option, kCGNullWindowID);
    if(osx_window_list)
    {
        CFIndex osx_window_count = CFArrayGetCount(osx_window_list);
        for(CFIndex osx_window_index = 0; osx_window_index < osx_window_count; ++osx_window_index)
        {
            CFDictionaryRef elem = (CFDictionaryRef)CFArrayGetValueAtIndex(osx_window_list, osx_window_index);
            window_lst.push_back(window_info());
            CFDictionaryApplyFunction(elem, get_window_info, NULL);
        }
        CFRelease(osx_window_list);

        for(int i = 0; i < window_lst.size(); ++i)
        {
            if(is_window_below_cursor(&window_lst[i]))
            {
                if(!windows_are_equal(&focused_window, &window_lst[i]))
                {
                    ProcessSerialNumber newpsn;
                    GetProcessForPID(window_lst[i].pid, &newpsn);

                    focused_psn = newpsn;
                    focused_window = window_lst[i];
                    get_layout_of_window(&focused_window);

                    AXUIElementRef window_ref;
                    if(get_window_ref(&focused_window, &window_ref))
                    {
                        AXUIElementSetAttributeValue(window_ref, kAXMainAttribute, kCFBooleanTrue);
                        AXUIElementSetAttributeValue(window_ref, kAXFocusedAttribute, kCFBooleanTrue);
                        AXUIElementPerformAction (window_ref, kAXRaiseAction);

                        if(focused_window_ref != NULL)
                            CFRelease(focused_window_ref);

                        focused_window_ref = window_ref;
                        std::cout << get_space_of_window(&focused_window) << std::endl;
                    }

                    if(enable_auto_raise)
                        SetFrontProcessWithOptions(&focused_psn, kSetFrontProcessFrontWindowOnly);

                    std::cout << "Keyboard focus: " << focused_window.pid << std::endl;
                    if(focused_window.layout)
                        std::cout << focused_window.layout->name << std::endl;
                }
                break;
            }
        }
    }
}

bool get_expression_from_shift_direction(window_info *window, const std::string &direction)
{
    get_layout_of_window(window);
    if(!window->layout)
        return false;

    int shift = 0;
    if(direction == "prev")
        shift = -1;
    else if(direction == "next")
        shift = 1;

    return (window->layout_index == focused_window.layout_index + shift);
}

void shift_window_focus(const std::string &direction)
{
    window_layout *focused_window_layout = get_layout_of_window(&focused_window);
    if(!focused_window_layout)
        return;

    int screen_index = get_display_of_window(&focused_window)->id;
    std::vector<window_info*> screen_window_lst = get_all_windows_on_display(screen_index);
    for(int window_index = 0; window_index < screen_window_lst.size(); ++window_index)
    {
        window_info *window = screen_window_lst[window_index];
        if(get_expression_from_shift_direction(window, direction))
        {
            AXUIElementRef window_ref;
            if(get_window_ref(window, &window_ref))
            {
                ProcessSerialNumber newpsn;
                GetProcessForPID(window->pid, &newpsn);

                AXUIElementSetAttributeValue(window_ref, kAXMainAttribute, kCFBooleanTrue);
                AXUIElementSetAttributeValue(window_ref, kAXFocusedAttribute, kCFBooleanTrue);
                AXUIElementPerformAction (window_ref, kAXRaiseAction);

                SetFrontProcessWithOptions(&newpsn, kSetFrontProcessFrontWindowOnly);

                if(focused_window_ref != NULL)
                    CFRelease(focused_window_ref);

                focused_window_ref = window_ref;
                focused_window = *window;
                focused_psn = newpsn;
            }
            break;
        }
    }
}

void set_window_dimensions(AXUIElementRef app_window, window_info *window, int x, int y, int width, int height)
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

    get_layout_of_window(window);

    CFRelease(new_window_pos);
    CFRelease(new_window_size);
}

bool get_window_ref(window_info *window, AXUIElementRef *window_ref)
{
    AXUIElementRef app = AXUIElementCreateApplication(window->pid);
    bool result = false;
    CFArrayRef app_window_lst;
    
    if(app)
    {
        AXError error = AXUIElementCopyAttributeValue(app, kAXWindowsAttribute, (CFTypeRef*)&app_window_lst);
        if(error == kAXErrorSuccess)
        {
            CFIndex do_not_free = -1;
            CFIndex app_window_count = CFArrayGetCount(app_window_lst);
            for(CFIndex window_index = 0; window_index < app_window_count; ++window_index)
            {
                AXUIElementRef app_window_ref = (AXUIElementRef)CFArrayGetValueAtIndex(app_window_lst, window_index);
                if(!result)
                {
                    AXValueRef temp;

                    CFStringRef app_window_title;
                    AXUIElementCopyAttributeValue(app_window_ref, kAXTitleAttribute, (CFTypeRef*)&app_window_title);

                    std::string app_window_name;
                    if(CFStringGetCStringPtr(app_window_title, kCFStringEncodingMacRoman))
                        app_window_name = CFStringGetCStringPtr(app_window_title, kCFStringEncodingMacRoman);
                    CFRelease(app_window_title);

                    CGSize app_window_size;
                    AXUIElementCopyAttributeValue(app_window_ref, kAXSizeAttribute, (CFTypeRef*)&temp);
                    AXValueGetValue(temp, kAXValueCGSizeType, &app_window_size);
                    CFRelease(temp);

                    CGPoint app_window_pos;
                    AXUIElementCopyAttributeValue(app_window_ref, kAXPositionAttribute, (CFTypeRef*)&temp);
                    AXValueGetValue(temp, kAXValueCGPointType, &app_window_pos);
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
        CFRelease(app);
    }

    return result;
}

window_info get_window_info_from_ref(AXUIElementRef window_ref)
{
    window_info info;

    CFStringRef window_title;
    AXValueRef temp;
    CGSize window_size;
    CGPoint window_pos;

    AXUIElementCopyAttributeValue(window_ref, kAXTitleAttribute, (CFTypeRef*)&window_title);
    if(CFStringGetCStringPtr(window_title, kCFStringEncodingMacRoman))
        info.name = CFStringGetCStringPtr(window_title, kCFStringEncodingMacRoman);
    CFRelease(window_title);

    AXUIElementCopyAttributeValue(window_ref, kAXSizeAttribute, (CFTypeRef*)&temp);
    AXValueGetValue(temp, kAXValueCGSizeType, &window_size);
    CFRelease(temp);

    AXUIElementCopyAttributeValue(window_ref, kAXPositionAttribute, (CFTypeRef*)&temp);
    AXValueGetValue(temp, kAXValueCGPointType, &window_pos);
    CFRelease(temp);

    info.width = window_size.width;
    info.height = window_size.height;

    return info;
}

void get_window_info(const void *key, const void *value, void *context)
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
                window_lst[window_lst.size()-1].name = value_str;
        }
    }
    else if(id == CFNumberGetTypeID())
    {
        int myint;
        CFNumberRef v = (CFNumberRef)value;
        CFNumberGetValue(v, kCFNumberSInt64Type, &myint);
        if(key_str == "kCGWindowNumber")
            window_lst[window_lst.size()-1].wid = myint;
        else if(key_str == "kCGWindowOwnerPID")
            window_lst[window_lst.size()-1].pid = myint;
        else if(key_str == "X")
            window_lst[window_lst.size()-1].x = myint;
        else if(key_str == "Y")
            window_lst[window_lst.size()-1].y = myint;
        else if(key_str == "Width")
            window_lst[window_lst.size()-1].width = myint;
        else if(key_str == "Height")
            window_lst[window_lst.size()-1].height = myint;
    }
    else if(id == CFDictionaryGetTypeID())
    {
        CFDictionaryRef elem = (CFDictionaryRef)value;
        CFDictionaryApplyFunction(elem, get_window_info, NULL);
    } 
}
