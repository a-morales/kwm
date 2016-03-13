#import "Private.h"

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, CGWindowID *out) __attribute__((weak_import));

@implementation PrivateAPI

+ (CGWindowID)GetCGWindowIDFromAXElement:(AXUIElementRef) el {
    CGWindowID id = 0;
    _AXUIElementGetWindow(el, &id);
    return id;
}

@end
