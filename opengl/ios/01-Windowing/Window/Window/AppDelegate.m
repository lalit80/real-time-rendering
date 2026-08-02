#import "AppDelegate.h"
#import "ViewController.h"
#import "MyView.h"

@implementation AppDelegate
{
    UIWindow *window;
    ViewController *viewController;
    MyView *view;
}

-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    CGRect screenRect = [[UIScreen mainScreen] bounds];

    window = [[UIWindow alloc] initWithFrame:screenRect];

    viewController = [[ViewController alloc] init];

    [window setRootViewController:viewController];

    view = [[MyView alloc] initWithFrame:screenRect];

    [viewController setView:view];

    [view release];

    [window makeKeyAndVisible];
    
    return YES;
}

-(void)applicationWillResignActive:(UIApplication *)application {
}

-(void)applicationDidEnterBackground:(UIApplication *)application {
}

-(void)applicationWillEnterForeground:(UIApplication *)application {
}

-(void)applicationDidBecomeActive:(UIApplication *)application {
}

-(void)applicationWillTerminate:(UIApplication *)application {
}

-(void)dealloc {
    [view release];
    [viewController release];
    [window release];
    [super dealloc];
}

@end
