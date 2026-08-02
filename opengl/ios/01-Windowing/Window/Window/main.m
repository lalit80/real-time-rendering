#import <UIKit/UIKit.h>
#import "AppDelegate.h"

int main(int argc, char * argv[]) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSString *delegateClassName = NSStringFromClass([AppDelegate class]);
    int result = UIApplicationMain(argc, argv, nil, delegateClassName);

    [pool release];

    return result;
}
