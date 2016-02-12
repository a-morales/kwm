#import "Cocoa/Cocoa.h"

extern void UpdateActiveSpace();

@interface MDWorkspaceWatcher : NSObject {
}
- (id)init;
@end

@implementation MDWorkspaceWatcher
- (id)init {
    if ((self = [super init])) {
       [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                selector:@selector(activeSpaceDidChange:)
                name:NSWorkspaceActiveSpaceDidChangeNotification
                object:nil];
    }
    return self;
}

- (void)dealloc {
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
    [super dealloc];
}
- (void)activeSpaceDidChange:(NSNotification *)notification {
    UpdateActiveSpace();
}
@end

void CreateWorkspaceWatcher(void *Watcher)
{
    MDWorkspaceWatcher *MDWatcher = [[MDWorkspaceWatcher alloc] init];
    Watcher = (void*)MDWatcher;
}
