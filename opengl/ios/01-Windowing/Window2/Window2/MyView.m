#import "MyView.h"

@implementation MyView {
    NSString *text;
}

-(id)initWithFrame:(CGRect)frame {
    // code
    self = [super initWithFrame:frame];
    if(self) {
        [self setBackgroundColor:[UIColor blackColor]];
        text = @"Hello World !!!";
        
        // event handling
        UITapGestureRecognizer* singleTapGestureRecognizer = [[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(onSingleTap:)];
        [singleTapGestureRecognizer setNumberOfTapsRequired:1];
        [singleTapGestureRecognizer setNumberOfTouchesRequired:1];
        [singleTapGestureRecognizer setDelegate:self];
        [self addGestureRecognizer:singleTapGestureRecognizer];
        
        UITapGestureRecognizer* doubleTapGestureRecognizer = [[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(onDoubleTap:)];
        [doubleTapGestureRecognizer setNumberOfTapsRequired:2];
        [doubleTapGestureRecognizer setNumberOfTouchesRequired:1];
        [doubleTapGestureRecognizer setDelegate:self];
        [self addGestureRecognizer:doubleTapGestureRecognizer];
        
        [singleTapGestureRecognizer requireGestureRecognizerToFail:doubleTapGestureRecognizer];
        
        UISwipeGestureRecognizer* swipeGestureRecognizer = [[UISwipeGestureRecognizer alloc]initWithTarget:self action:@selector(onSwipe:)];
        [swipeGestureRecognizer setDelegate:self];
        [self addGestureRecognizer:swipeGestureRecognizer];
        
        UILongPressGestureRecognizer* longPressGestureRecognizer = [[UILongPressGestureRecognizer alloc]initWithTarget:self action:@selector(onLongPress:)];
        [longPressGestureRecognizer setDelegate:self];
        [self addGestureRecognizer:longPressGestureRecognizer];
    }
    
    return self;
}

- (void)drawRect:(CGRect)rect {
    // code
    UIFont *textFont = [UIFont fontWithName:@"Helvetica" size:32];
    UIColor *textColor = [UIColor colorWithRed:0.0 green:1.0 blue:0.0 alpha:1.0];
    NSDictionary *dictionary = [NSDictionary dictionaryWithObjectsAndKeys:textFont, NSFontAttributeName,textColor, NSForegroundColorAttributeName,nil];
    CGSize textSize = [text sizeWithAttributes:dictionary];
    CGPoint point;
    point.x = (rect.size.width / 2) - (textSize.width / 2);
    point.y = (rect.size.height / 2) - (textSize.height / 2) + 12;
    [text drawAtPoint:point withAttributes:dictionary];
}

-(BOOL) becomeFirstResponder {
    return YES;
}

-(void) onSingleTap:(UITapGestureRecognizer*) gestureRecognizer {
    // code
    text = @"Single Tap";
    [self setNeedsDisplay];
}

-(void) onDoubleTap:(UITapGestureRecognizer*) gestureRecognizer {
    // code
    text = @"Double Tap";
    [self setNeedsDisplay];
}

-(void) onSwipe:(UISwipeGestureRecognizer*) gestureRecognizer {
    // code
    [self release];
    exit(0);
}

-(void) onLongPress:(UILongPressGestureRecognizer*) gestureRecognizer {
    // code
    text = @"Long Press";
    [self setNeedsDisplay];
}

-(void) touchesBegan:(UITouch*)touches withEvent:(UIEvent *)event {}

-(void) dealloc {
    [super dealloc];
}

@end
