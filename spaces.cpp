#include "kwm.h"

extern std::vector<spaces_info> SpacesLst;

void GetActiveSpaces()
{
    CFStringRef SpacesPlist = CFSTR("com.apple.spaces");
    CFStringRef SpacesKey = CFSTR("SpacesDisplayConfiguration");

    CFArrayRef SpacesPropList = (CFArrayRef) CFPreferencesCopyAppValue(SpacesKey, SpacesPlist); 
    if(SpacesPropList)
    {
        SpacesLst.clear();

        CFDictionaryApplyFunction((CFDictionaryRef)SpacesPropList, GetSpacesInfo, NULL);
        CFRelease(SpacesPropList);

        for(int SpaceIndex = 0; SpaceIndex < SpacesLst.size(); ++SpaceIndex)
        {
            SpacesLst[SpaceIndex].RootNode = CreateTreeFromWindowIDList(&SpacesLst[SpaceIndex].Windows);
            SpacesLst[SpaceIndex].NextLayoutIndex = 0;
        }
    }

    CFRelease(SpacesKey);
    CFRelease(SpacesPlist);
}

int GetSpaceOfWindow(window_info *Window)
{
    for(int SpaceIndex = 0; SpaceIndex < SpacesLst.size(); ++SpaceIndex)
    {
        for(int WindowIndex = 0; WindowIndex < SpacesLst[SpaceIndex].Windows.size(); ++WindowIndex)
        {
            if(Window->WID == SpacesLst[SpaceIndex].Windows[WindowIndex])
                return SpaceIndex;
        }
    }

    return -1;
}

void GetSpacesInfo(const void *Key, const void *Value, void *Context)
{
    CFStringRef K = (CFStringRef)Key;
    std::string KeyStr = CFStringGetCStringPtr(K, kCFStringEncodingMacRoman);

    if(KeyStr == "Space Properties" || KeyStr == "name" || KeyStr == "windows") 
    {
        CFTypeID ID = CFGetTypeID(Value);
        if(ID == CFArrayGetTypeID())
        {
            CFArrayRef V = (CFArrayRef)Value;
            if(V)
            {
                CFIndex Count = CFArrayGetCount(V);
                for(CFIndex Index = 0; Index < Count; ++Index)
                {
                    if(KeyStr == "windows")
                    {
                        CFNumberRef Num = (CFNumberRef)CFArrayGetValueAtIndex(V, Index);
                        int MyInt;
                        CFNumberGetValue(Num, kCFNumberSInt64Type, &MyInt);
                        SpacesLst[SpacesLst.size()-1].Windows.push_back(MyInt);
                    }
                    else
                    {
                        SpacesLst.push_back(spaces_info());
                        CFDictionaryRef Elem = (CFDictionaryRef)CFArrayGetValueAtIndex(V, Index);
                        CFDictionaryApplyFunction(Elem, GetSpacesInfo, NULL);
                    }
                }
            }
        }
    }
}
