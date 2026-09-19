#include "aquarium/hud_toast.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
namespace {
constexpr double lifetime=3.5,entryDuration=.48,exitDuration=.24;
}
HudToastLayout layoutHudToast(float width,float height,Insets safe,float minimumTouch){
 const auto hud=layoutHud(width,height,safe,minimumTouch);
 const float density=minimumTouch/44.f;
 const float margin=12*density,available=std::max(1.f,width-safe.left-safe.right-2*margin);
 // Size the notice in window points, independently of the aquarium's canvas.
 // Keep it compact in landscape and cap its width on larger displays.
 const float scale=std::min(available,std::clamp(available*.36f,224*density,288*density))/712.f;
 const float w=712*scale,h=168*scale;
 const auto settings=hud[HudPart::Settings];
 const float top=settings.y+settings.h+8*density;
 const float right=std::min(settings.x+settings.w,width-safe.right-margin);
 const Rect frame{std::max(safe.left+margin,right-w),
  std::min(top,std::max(safe.top,height-safe.bottom-h-margin)),w,h};
 auto box=[&](float x,float y,float bw,float bh){return Rect{frame.x+x*scale,frame.y+y*scale,bw*scale,bh*scale};};
 const float side=std::max(minimumTouch,48*scale);
 const auto cross=box(652,63,0,0);
 const Rect close{std::clamp(cross.x-side*.5f,frame.x,frame.x+std::max(0.f,w-side)),
  std::clamp(cross.y-side*.5f,frame.y,frame.y+std::max(0.f,h-side)),side,side};
 auto title=box(178,51,428,42),body=box(183,91,423,34);
 title.w=std::min(title.w,close.x-8*density-title.x);
 body.w=std::min(body.w,close.x-8*density-body.x);
 return {frame,close,title,body,scale,
  std::max(40*scale,14*density),std::max(25*scale,11*density)};
}

void HudToast::show(std::string title,std::string message){
 title_=std::move(title);message_=std::move(message);elapsed_=0;remaining_=lifetime;
 pressed_=pressedClose_=dragged_=false;
}
void HudToast::clear(){title_.clear();message_.clear();pressed_=pressedClose_=dragged_=false;remaining_=0;}
void HudToast::advance(double seconds,bool reducedMotion){
 reduced_=reducedMotion;
 if(!visible())return;
 elapsed_+=std::max(0.,seconds);
 remaining_-=std::max(0.,seconds);
 if(remaining_<=0)clear();
}
HudToastLayout HudToast::animatedLayout(HudToastLayout layout)const{
 if(!visible()||reduced_)return layout;
 // Drop in from above the window, overshoot once, then settle. Transform the
 // hit targets with the artwork so the close control works during motion.
 const float t=float(std::clamp(elapsed_/entryDuration,0.,1.))-1;
 const float progress=1+2.35f*t*t*t+1.35f*t*t;
 const float exit=float(std::clamp(1-remaining_/exitDuration,0.,1.));
 const float offset=-(layout.frame.y+layout.frame.h*1.1f)*(1-progress)-layout.frame.h*.3f*exit*exit;
 for(auto* box:{&layout.frame,&layout.close,&layout.title,&layout.body})box->y+=offset;
 return layout;
}
bool HudToast::event(const SDL_Event& e,SDL_FPoint point,const HudToastLayout& layout,float minimumTouch){
 if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND||e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET||e.type==SDL_EVENT_WINDOW_RESIZED){
  pressed_=pressedClose_=dragged_=false;return false;
 }
 if(!visible())return false;
 if(e.type==SDL_EVENT_KEY_DOWN&&e.key.key==SDLK_ESCAPE){if(!e.key.repeat)clear();return true;}
 const bool over=layout.frame.has(point.x,point.y);
 if(e.type==SDL_EVENT_MOUSE_MOTION){
  if(pressed_&&std::hypot(point.x-down_.x,point.y-down_.y)>minimumTouch*8/44)dragged_=true;
  return pressed_||over;
 }
 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){
  if(!over)return false;
  if(e.button.button==SDL_BUTTON_LEFT){pressed_=true;down_=point;pressedClose_=layout.close.has(point.x,point.y);dragged_=false;}
  return true;
 }
 if(e.type==SDL_EVENT_MOUSE_BUTTON_UP){
  if(e.button.button!=SDL_BUTTON_LEFT)return over;
  const bool consumed=pressed_||over;
  const bool dismiss=pressed_&&pressedClose_&&!dragged_&&layout.close.has(point.x,point.y);
  pressed_=pressedClose_=dragged_=false;
  if(dismiss)clear();
  return consumed;
 }
 return false;
}
void HudToast::paint(Canvas& canvas,const HudToastLayout& l)const{
 if(!visible())return;
 const float opacity=reduced_?1.f:float(std::min({1.,elapsed_/.10,remaining_/exitDuration}));
 // Source bounds are recorded alongside the generated, text-free artwork.
 const float sx=l.frame.w/1957.f,sy=l.frame.h/447.f;
 const Rect art{l.frame.x-98*sx,l.frame.y-119*sy,2087*sx,754*sy};
 canvas.image("hud-icons/error-toast-bottle-v1.png",art,0,{.5f,.5f},opacity);
 if(pressed_&&pressedClose_&&!dragged_){
  canvas.clip(l.close);canvas.image("hud-icons/error-toast-bottle-v1.png",art,0,{.5f,.5f},opacity,false,{215,233,234,255});canvas.clearClip();
 }
 const auto alpha=Uint8(std::lround(255*opacity));
 canvas.text(title_,l.title.x,l.title.y,l.titleSize,{9,75,97,alpha},false,l.title.w,true,false,false,false,0,.98f);
 canvas.text(message_,l.body.x,l.body.y,l.bodySize,{39,126,133,alpha},false,l.body.w,true);
}
}
