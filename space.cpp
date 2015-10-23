#include "kwm.h"

extern std::vector<window_info> WindowLst;
extern std::map<int, std::vector<int> > SpacesOfWindow;
static int SpaceCounter;

void GetActiveSpaces()
{
    CFStringRef SpacesPlist = CFSTR("com.apple.spaces");
    CFStringRef SpacesKey = CFSTR("SpacesDisplayConfiguration");

    CFArrayRef SpacesPropList = (CFArrayRef) CFPreferencesCopyAppValue(SpacesKey, SpacesPlist); 
    if(SpacesPropList)
    {
        SpaceCounter = 0;
        SpacesOfWindow.clear();

        CFDictionaryApplyFunction((CFDictionaryRef)SpacesPropList, GetSpaceInfo, NULL);
        CFRelease(SpacesPropList);
    }

    CFRelease(SpacesKey);
    CFRelease(SpacesPlist);
}

void AddWindowToSpace(int WindowID, int SpaceIndex)
{
    auto Identifier = SpacesOfWindow.find(WindowID);
    if(Identifier != SpacesOfWindow.end())
    {
        Identifier->second.push_back(SpaceIndex);
    }
    else
    {
        SpacesOfWindow.insert(std::pair<int, std::vector<int> >(WindowID, std::vector<int>()));
        Identifier = SpacesOfWindow.find(WindowID);
        Identifier->second.push_back(SpaceIndex);
    }
}

int NumberOfSpaces()
{
    return SpaceCounter;
}

int GetSpaceOfWindow(window_info *Window)
{
    auto Identifier = SpacesOfWindow.find(Window->WID);
    if(Identifier != SpacesOfWindow.end())
    {
        if(Identifier->second.size() == 1)
        {
            return Identifier->second[0];
        }
    }

    return -1;
}

void GetSpaceInfo(const void *Key, const void *Value, void *Context)
{
    CFStringRef KeyRef = (CFStringRef)Key;
    std::string KeyStr = CFStringGetCStringPtr(KeyRef, kCFStringEncodingMacRoman);

    if(KeyStr == "Space Properties" || KeyStr == "name" || KeyStr == "windows") 
    {
        CFTypeID ID = CFGetTypeID(Value);
        if(ID == CFArrayGetTypeID())
        {
            CFArrayRef ValueRef = (CFArrayRef)Value;
            if(ValueRef)
            {
                CFIndex Count = CFArrayGetCount(ValueRef);
                for(CFIndex Index = 0; Index < Count; ++Index)
                {
                    if(KeyStr == "windows")
                    {
                        CFNumberRef Num = (CFNumberRef)CFArrayGetValueAtIndex(ValueRef, Index);
                        int MyInt;
                        CFNumberGetValue(Num, kCFNumberSInt64Type, &MyInt);

                        auto Identifier = SpacesOfWindow.find(MyInt);
                        if(Identifier == SpacesOfWindow.end())
                        {
                            SpacesOfWindow.insert(std::pair<int, std::vector<int> >(MyInt, std::vector<int>()));
                            Identifier = SpacesOfWindow.find(MyInt);
                        }

                        if(Identifier != SpacesOfWindow.end())
                        {
                            Identifier->second.push_back(SpaceCounter-1);
                        }
                    }
                    else
                    {
                        SpaceCounter++;
                        CFDictionaryRef Elem = (CFDictionaryRef)CFArrayGetValueAtIndex(ValueRef, Index);
                        CFDictionaryApplyFunction(Elem, GetSpaceInfo, NULL);
                    }
                }
            }
        }
    }
}
