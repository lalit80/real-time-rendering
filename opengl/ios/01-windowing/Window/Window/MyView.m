#import "MyView.h"

@implementation MyView

-(id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.backgroundColor = [UIColor redColor];
    }
    return self;
}

-(void)dealloc {
    [super dealloc];
}

@end
