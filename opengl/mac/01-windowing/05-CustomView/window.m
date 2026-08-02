#import <Cocoa/Cocoa.h>

@interface MyView : NSView
@end

@implementation MyView
-(void)drawRect:(NSRect)dirtyRect {
    [[NSColor blackColor] setFill];
    NSRectFill(dirtyRect);
    
    NSString *text = @"Hello World !!!";
    NSDictionary *attributes = @{
        NSFontAttributeName: [NSFont fontWithName:@"Helvetica" size:32],
        NSForegroundColorAttributeName: [NSColor greenColor]
    };
    NSSize textSize = [text sizeWithAttributes:attributes];
    NSPoint point = NSMakePoint((dirtyRect.size.width - textSize.width) / 2, 
                                (dirtyRect.size.height - textSize.height) / 2);
    [text drawAtPoint:point withAttributes:attributes];
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation AppDelegate {
    NSWindow *window;
    MyView *view;
}

-(void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSRect winRect = NSMakeRect(0.0, 0.0, 800.0, 600.0);
    window = [[NSWindow alloc] initWithContentRect:winRect
                                         styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    view = [[MyView alloc] initWithFrame:winRect];
    [window setContentView:view];
    [window setTitle:@"LRC: Custom View"];
    [window center];
    [window setDelegate:self];
    [window makeKeyAndOrderFront:self];
}

-(void)windowWillClose:(NSNotification*)notification {
    [NSApp terminate:self];
}

-(void)dealloc {
    [view release];
    [window release];
    [super dealloc];
}
@end

int main(int argc, char* argv[]) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSApp = [NSApplication sharedApplication];
    [NSApp setDelegate:[[AppDelegate alloc] init]];
    [NSApp run];
    [pool release];
    return 0;
}