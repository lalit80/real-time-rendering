#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

@interface MyView : NSView {
    @private
    NSString *text;
}
@end

@implementation MyView

-(id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        text = @"Hello World !!!";
        [self setWantsLayer:YES];
        [[self layer] setBackgroundColor:[[NSColor blackColor] CGColor]];
    }
    return self;
}

-(void)drawRect:(NSRect)dirtyRect {
    NSFont *textFont = [NSFont fontWithName:@"Helvetica" size:32];
    NSColor *textColor = [NSColor greenColor];
    NSDictionary *dictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                textFont, NSFontAttributeName,
                                textColor, NSForegroundColorAttributeName, nil];
    
    NSSize textSize = [text sizeWithAttributes:dictionary];
    NSPoint point;
    point.x = (dirtyRect.size.width / 2) - (textSize.width / 2);
    point.y = (dirtyRect.size.height / 2) - (textSize.height / 2);
    
    [text drawAtPoint:point withAttributes:dictionary];
}

-(BOOL)acceptsFirstResponder {
    return YES;
}

-(void)mouseDown:(NSEvent*)event {
    text = @"Left Mouse Button Is Clicked";
    [self setNeedsDisplay:YES];
}

-(void)keyDown:(NSEvent*)event {
    int key = (int)[[event characters] characterAtIndex:0];
    if (key == 27) { // ESC key
        [NSApp terminate:self];
    }
}

-(void)dealloc {
    [super dealloc];
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation AppDelegate {
    @private
    NSWindow *window;
    MyView *view;
}

-(void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSRect winRect = NSMakeRect(0.0, 0.0, 800.0, 600.0);
    window = [[NSWindow alloc] initWithContentRect:winRect
                                         styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    [window setTitle:@"LRC: Event Handling"];
    [window center];

    view = [[MyView alloc] initWithFrame:winRect];
    [window setContentView:view];
    [window setDelegate:self];
    [window makeKeyAndOrderFront:self];
    [window makeFirstResponder:view];
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