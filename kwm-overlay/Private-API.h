#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

@interface PrivateAPI : NSObject
+ (CGWindowID)GetCGWindowIDFromAXElement:(AXUIElementRef) el;
@end
