#include "kwm.h"

extern std::vector<spaces_info> spaces_lst;

void get_active_spaces()
{
    CFStringRef spaces_plist = CFSTR("com.apple.spaces");
    CFStringRef spaces_key = CFSTR("SpacesDisplayConfiguration");

    CFArrayRef spaces_prop_list = (CFArrayRef) CFPreferencesCopyAppValue(spaces_key, spaces_plist); 
    if(spaces_prop_list)
    {
        spaces_lst.clear();

        CFDictionaryApplyFunction((CFDictionaryRef)spaces_prop_list, get_spaces_info, NULL);
        CFRelease(spaces_prop_list);

        for(int space_index = 0; space_index < spaces_lst.size(); ++space_index)
        {
            spaces_lst[space_index].active_layout_index = 0;
            spaces_lst[space_index].next_layout_index = 0;
        }
    }

    CFRelease(spaces_key);
    CFRelease(spaces_plist);
}

int get_space_of_window(window_info *window)
{
    for(int space_index = 0; space_index < spaces_lst.size(); ++space_index)
    {
        for(int window_index = 0; window_index < spaces_lst[space_index].windows.size(); ++window_index)
        {
            if(window->wid == spaces_lst[space_index].windows[window_index])
                return space_index;
        }
    }

    return -1;
}

void get_spaces_info(const void *key, const void *value, void *context)
{
    CFStringRef k = (CFStringRef)key;
    std::string key_str = CFStringGetCStringPtr(k, kCFStringEncodingMacRoman);

    if(key_str == "Space Properties" || key_str == "name" || key_str == "windows") 
    {
        CFTypeID id = CFGetTypeID(value);
        if(id == CFArrayGetTypeID())
        {
            CFArrayRef v = (CFArrayRef)value;
            if(v)
            {
                CFIndex count = CFArrayGetCount(v);
                for(CFIndex c = 0; c < count; ++c)
                {
                    if(key_str == "windows")
                    {
                        CFNumberRef num = (CFNumberRef)CFArrayGetValueAtIndex(v, c);
                        int myint;
                        CFNumberGetValue(num, kCFNumberSInt64Type, &myint);
                        spaces_lst[spaces_lst.size()-1].windows.push_back(myint);
                    }
                    else
                    {
                        spaces_lst.push_back(spaces_info());
                        CFDictionaryRef elem = (CFDictionaryRef)CFArrayGetValueAtIndex(v, c);
                        CFDictionaryApplyFunction(elem, get_spaces_info, NULL);
                    }
                }
            }
        }
    }
}
