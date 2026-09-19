#include "aquarium/platform_feedback.hpp"
#import <UIKit/UIKit.h>
namespace aq {
std::optional<bool> systemReducedMotion(){return bool(UIAccessibilityIsReduceMotionEnabled());}
void playHaptic(){
 @autoreleasepool {
  UIImpactFeedbackGenerator* feedback=[[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
  [feedback impactOccurred];
  #if !__has_feature(objc_arc)
  [feedback release];
  #endif
 }
}
}
