#include "kwm.h"

extern std::vector<spaces_info> SpacesLst;

void GetActiveSpaces()
{
    CFStringRef spaces_plist = CFSTR("com.apple.spaces");
    CFStringRef spaces_key = CFSTR("SpacesDisplayConfiguration");

    CFArrayRef spaces_prop_list = (CFArrayRef) CFPreferencesCopyAppValue(spaces_key, spaces_plist); 
    if(spaces_prop_list)
    {
        SpacesLst.clear();

        CFDictionaryApplyFunction((CFDictionaryRef)spaces_prop_list, GetSpacesInfo, NULL);
        CFRelease(spaces_prop_list);

        for(int space_index = 0; space_index < SpacesLst.size(); ++space_index)
        {
            SpacesLst[space_index].active_layout_index = 0;
            SpacesLst[space_index].next_layout_index = 0;
        }
    }

    CFRelease(spaces_key);
    CFRelease(spaces_plist);
}

int GetSpaceOfWindow(window_info *window)
{
    for(int space_index = 0; space_index < SpacesLst.size(); ++space_index)
    {
        for(int window_index = 0; window_index < SpacesLst[space_index].windows.size(); ++window_index)
        {
            if(window->wid == SpacesLst[space_index].windows[window_index])
                return space_index;
        }
    }

    return -1;
}

void RefreshActiveSpacesInfo()
{
    std::vector<std::pair<int,int> > spaces_cache;
    for(int space_index = 0; space_index < SpacesLst.size(); ++space_index)
    {
        std::pair<int,int> info = std::make_pair(SpacesLst[space_index].active_layout_index,
                                                 SpacesLst[space_index].next_layout_index);
        spaces_cache.push_back(info);
    }

    GetActiveSpaces();
    for(int space_index = 0; space_index < SpacesLst.size(); ++space_index)
    {
        if(space_index < spaces_cache.size())
        {
            SpacesLst[space_index].active_layout_index = spaces_cache[space_index].first;
            SpacesLst[space_index].next_layout_index = spaces_cache[space_index].second;
        }
    }
}

void GetSpacesInfo(const void *key, const void *value, void *context)
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
                        SpacesLst[SpacesLst.size()-1].windows.push_back(myint);
                    }
                    else
                    {
                        SpacesLst.push_back(spaces_info());
                        CFDictionaryRef elem = (CFDictionaryRef)CFArrayGetValueAtIndex(v, c);
                        CFDictionaryApplyFunction(elem, GetSpacesInfo, NULL);
                    }
                }
            }
        }
    }
}
