#import "SceneDelegate.h"
#import "GLESView.h"

@implementation SceneDelegate {
    GLESView* view;
}

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    // code
    view = (GLESView*)[[[self window]rootViewController]view];
    if([view isKindOfClass:[GLESView class]]) [view startDisplayLink];
}


- (void)sceneDidDisconnect:(UIScene *)scene {
    // code
    if([view isKindOfClass:[GLESView class]]) [view stopDisplayLink];
}


- (void)sceneDidBecomeActive:(UIScene *)scene {
    // code
    if([view isKindOfClass:[GLESView class]]) [view startDisplayLink];
}


- (void)sceneWillResignActive:(UIScene *)scene {
    // code
    if([view isKindOfClass:[GLESView class]]) [view stopDisplayLink];
}


- (void)sceneWillEnterForeground:(UIScene *)scene {
    // code
}


- (void)sceneDidEnterBackground:(UIScene *)scene {
    // code
}

-(void) dealloc {
    [super dealloc];
}


@end
