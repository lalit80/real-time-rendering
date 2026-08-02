#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@interface MyView : NSView
@end

int main(int argc, char* argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc]init];

    NSApp = [NSApplication sharedApplication];

    [NSApp setDelegate:[[AppDelegate alloc]init]];
    
    [NSApp run];

    [pool release];

    return 0;
}

@implementation AppDelegate
{
    @private
    NSWindow *window;
    MyView *view;
}

-(void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSRect winRect = NSMakeRect(0.0, 0.0, 800.0, 600.0);
    window = [[NSWindow alloc]initWithContentRect:winRect
                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                    backing:NSBackingStoreBuffered
                                    defer:NO];
    [window setTitle:@"LRC: macOS Window"];
    [window center];

    view = [[MyView alloc]initWithFrame:winRect];

    [window setContentView:view];
    [window setDelegate:self];
    [window makeKeyAndOrderFront:self];
}

-(void)applicationWillTerminate:(NSNotification*)notification {
     
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


@implementation MyView {
    @private
    NSString *text;
}

-(id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [[self window]setContentView:self];
        text = @"Hello World !!!";
        [self setWantsLayer:YES];
        NSColor *backGroundColor = [NSColor blackColor];
        struct CGColor *bgColor = [backGroundColor CGColor];
        [[self layer]setBackgroundColor:bgColor];
    }

    return self;
}

-(void)drawRect:(NSRect)dirtyRect {
    NSFont *textFont = [NSFont fontWithName:@"Helvetica" size:32];
    NSColor *textColor = [NSColor colorWithDeviceRed:0.0 green:1.0 blue:0.0 alpha:1.0];
    NSDictionary *dictionary = [NSDictionary dictionaryWithObjectsAndKeys:textFont, NSFontAttributeName,
                                                                        textColor, NSForegroundColorAttributeName,
                                                                        nil];
    NSSize textSize = [text sizeWithAttributes:dictionary];
    NSPoint point;
    point.x = (dirtyRect.size.width / 2) - (textSize.width / 2);
    point.y = (dirtyRect.size.height / 2) - (textSize.height / 2) + 12;
    [text drawAtPoint:point withAttributes:dictionary];
}

-(BOOL)acceptsFirstResponder {
    [[self window]makeFirstResponder:self];
    return YES;
}

-(void)keyDown:(NSEvent*)event {
    int key = (int)[[event characters]characterAtIndex:0];

    switch(key) {
        case 27:                                
            [self release];
            [NSApp terminate:self];
            break;

        case 'f':
        case 'F':
            text = @"'F' or 'f' Key is pressed";
            [[self window]toggleFullScreen:self];
            break;

        default:
            break;
    }
}

-(void)mouseDown:(NSEvent*)event {
    text = @"Left Mouse Button Is Clicked";
    [self setNeedsDisplay:YES];
}

-(void)dealloc {
    [super dealloc];
}

@end
