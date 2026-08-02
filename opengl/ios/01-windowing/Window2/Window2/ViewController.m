#import "ViewController.h"
#import "MyView.h"

@implementation ViewController
{
    MyView *view;
}

-(void) loadView {
    // code
    view = [[MyView alloc]initWithFrame:CGRectZero];
    [self setView:view];	
}

- (void)viewDidLoad {
    // code
    [super viewDidLoad];
}

-(void) didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

-(void) dealloc {
    [super dealloc];
}

@end
