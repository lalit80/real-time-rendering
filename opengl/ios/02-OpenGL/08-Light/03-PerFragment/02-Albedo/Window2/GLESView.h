#import <UIKit/UIKit.h>


@interface GLESView : UIView <UIGestureRecognizerDelegate>
-(void) startDisplayLink;
-(void) stopDisplayLink;
@end

