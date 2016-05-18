#include "application.h"
#include "window.h"
#include "axlib/element.h"

extern kwm_tiling KWMTiling;
extern kwm_cache KWMCache;

void AllowRoleForWindow(window_info *Window, std::string Role)
{
    std::map<int, CFTypeRef>::iterator It = KWMTiling.AllowedWindowRoles.find(Window->WID);
    if(It == KWMTiling.AllowedWindowRoles.end())
    {
        CFStringRef RoleRef = CFStringCreateWithCString(NULL, Role.c_str(), kCFStringEncodingMacRoman);
        KWMTiling.AllowedWindowRoles[Window->WID] = RoleRef;
    }
}

bool IsWindowSpecificRole(window_info *Window, CFTypeRef Role, CFTypeRef SubRole)
{
    std::map<int, CFTypeRef>::iterator It = KWMTiling.AllowedWindowRoles.find(Window->WID);
    if(It != KWMTiling.AllowedWindowRoles.end())
    {
        if(CFEqual(Role, It->second) || CFEqual(SubRole, It->second))
            return true;
    }

    return false;
}

bool IsApplicationInCache(int PID, std::vector<AXUIElementRef> *Elements)
{
    bool Result = false;
    std::map<int, std::vector<AXUIElementRef> >::iterator It = KWMCache.WindowRefs.find(PID);

    if(It != KWMCache.WindowRefs.end())
    {
        *Elements = It->second;
        Result = true;
    }

    return Result;
}

bool GetWindowRefFromCache(window_info *Window, AXUIElementRef *WindowRef)
{
    std::vector<AXUIElementRef> Elements;
    bool IsCached = IsApplicationInCache(Window->PID, &Elements);

    if(IsCached)
    {
        for(std::size_t ElementIndex = 0; ElementIndex < Elements.size(); ++ElementIndex)
        {
            if(AXLibGetWindowID(Elements[ElementIndex]) == Window->WID)
            {
                *WindowRef = Elements[ElementIndex];
                return true;
            }
        }
    }

    if(!IsCached)
        KWMCache.WindowRefs[Window->PID] = std::vector<AXUIElementRef>();

    return false;
}

void FreeWindowRefCache(int PID)
{
    std::map<int, std::vector<AXUIElementRef> >::iterator It = KWMCache.WindowRefs.find(PID);

    if(It != KWMCache.WindowRefs.end())
    {
        int NumElements = KWMCache.WindowRefs[PID].size();
        for(int RefIndex = 0; RefIndex < NumElements; ++RefIndex)
            CFRelease(KWMCache.WindowRefs[PID][RefIndex]);

        KWMCache.WindowRefs[PID].clear();
    }
}
