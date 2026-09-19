#include "aquarium/scroll.hpp"
#include "aquarium/hud_care.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
float wheelAt(int hz){
 ScrollMotion motion;float position=0;motion.wheel(position,3,10);
 check(position==0,"Wheel input jumps before a frame is rendered");
 for(int i=0;i<hz/4;++i){const auto before=position;motion.advance(position,1./hz,10);check(position>=before&&position<=3,"Wheel easing overshoots or reverses");}
 return position;
}
float flingAt(int hz){
 ScrollMotion motion;float position=0;motion.begin(position,1);
 for(int i=1;i<=6;++i)motion.drag(position,float(i)/6,20,1.+i/60.);
 check(position==1,"Dragging lags behind the pointer");motion.release(1.1);
 for(int i=0;i<hz/2;++i)motion.advance(position,1./hz,20);
 check(position>1.5f,"Swipe has no momentum");return position;
}
}
int main(){try{
 using namespace aq;
 check(std::abs(wheelAt(60)-wheelAt(120))<.001f&&std::abs(wheelAt(60)-wheelAt(144))<.001f,"Wheel speed depends on display refresh rate");
 check(std::abs(flingAt(60)-flingAt(120))<.001f&&std::abs(flingAt(60)-flingAt(30))<.001f,"Fling distance depends on frame rate");
 ScrollMotion motion;float position=0;
 for(int i=0;i<12;++i)motion.wheel(position,.025f,10);
 for(int i=0;i<60;++i)motion.advance(position,1./60,10);
 check(std::abs(position-.3f)<.001f&&!motion.moving(),"Fractional wheel movement was lost or never settled");
 motion.wheel(position,4,10);motion.advance(position,1./60,10);const float before=position;
 motion.wheel(position,-.25f,10);motion.advance(position,1./60,10);
 check(position<before,"Changing direction continues queued forward motion");
 motion.wheel(position,100,10);for(int i=0;i<60;++i)motion.advance(position,1./60,10);
 check(position==10,"Wheel cannot reach the final item");
 motion.wheel(position,-100,10);for(int i=0;i<60;++i)motion.advance(position,1./60,10);
 check(position==0,"Wheel scrolls before the first item");
 motion.begin(position,2);motion.drag(position,1,10,2.05);motion.release(2.3);motion.advance(position,1./60,10);
 check(position==1&&!motion.moving(),"A held row flings on release");
 motion.begin(position,3);motion.drag(position,2,10,3.05);motion.release(3.05);motion.stop();motion.advance(position,.1,10);
 check(position==2,"Cancelled gesture keeps moving");
 motion.begin(position,4);motion.drag(position,3,10,4.05);motion.release(4.05,true);motion.advance(position,.1,10,true);
 check(position==3&&!motion.moving(),"Reduced motion retains a fling");
 motion.wheel(position,1,10,true);check(position==4&&!motion.moving(),"Reduced motion animates the wheel");
 motion.wheel(position,5,10);motion.advance(position,1./60,2);check(position<=2,"Resized content leaves the row beyond its bounds");
 motion.advance(position,1,2);check(!motion.moving(),"Suspended menu resumes stale momentum");
 SDL_Event wheel{};wheel.type=SDL_EVENT_MOUSE_WHEEL;wheel.wheel.x=.01f;wheel.wheel.y=-.5f;
 check(horizontalWheel(wheel)==.5f,"Trackpad cross-axis noise overrides the main movement");
 wheel.wheel.direction=SDL_MOUSEWHEEL_FLIPPED;check(horizontalWheel(wheel)==-.5f,"Natural scrolling is reversed");
 wheel.wheel.direction=SDL_MOUSEWHEEL_NORMAL;wheel.wheel.x=.7f;
 check(horizontalWheel(wheel)==.7f,"Horizontal wheel direction differs between menus");
 HudPointer pointer;SDL_Event finger{};finger.type=SDL_EVENT_FINGER_DOWN;finger.common.timestamp=1234567890;finger.tfinger.fingerID=7;
 check(normalizeHudPointer(pointer,finger,800,600)&&finger.common.timestamp==1234567890,"Touch normalization loses motion timing");
 std::cout<<"PASS smooth wheel input, swipe momentum, frame-rate independence, cancellation and touch timing\n";
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
