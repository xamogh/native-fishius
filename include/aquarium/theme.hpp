#pragma once
#include "aquarium/view.hpp"
#include <algorithm>

namespace aq::theme {
inline constexpr Color ink{7,54,79,255};
inline constexpr Color body{24,76,122,255};
inline constexpr Color white{250,254,255,255};
inline constexpr Color cyan{137,249,255,255};
inline constexpr Color cream{255,251,240,255};
inline constexpr Color teal{13,112,137,255};
// Both reference screens share this canvas. One transform drives drawing and input.
struct Layout {
 float u{},x{},y{};
 explicit Layout(const Canvas& c) {
  const auto safe=c.safeInsets();
  u=std::min({c.width()/1672.f,c.height()/941.f,
              (c.width()-safe.left-safe.right)/1532.f});
  x=std::clamp((c.width()-1672*u)*.5f,safe.left-70*u,
               c.width()-safe.right-1602*u);
  y=(c.height()-941*u)*.5f;
 }
 Rect rect(float px,float py,float w,float h)const{return {x+px*u,y+py*u,w*u,h*u};}
};
// Keep the aquarium controls in the same places when a menu opens.
struct HudLayout : Layout {
 float left{},right{},top{},floor{};
 explicit HudLayout(const Canvas& c):Layout(c) {
  const auto safe=c.safeInsets();
  left=std::max(18*u,safe.left+8*u);
  right=c.width()-std::max(22*u,safe.right+8*u);
  top=std::max(25*u,safe.top+8*u);
  floor=c.height()-std::max(28*u,safe.bottom+8*u);
 }
 Rect xp()const{return {left+68*u,top+17*u,316*u,50*u};}
 Rect tank()const{return {left,floor-140*u,265*u,138*u};}
 Rect shop()const{return {right-203*u,floor-187*u,200*u,187*u};}
 Rect menuSpace()const {
  const float x=tank().x+tank().w+12*u,y=top+95*u;
  return {x,y,shop().x-12*u-x,floor-y};
 }
};
// Tank and Shop dialogs share the same frame size and safe-area scaling.
struct DialogLayout {
 float u{},x{},y{};
 explicit DialogLayout(const Canvas& c) {
  const auto safe=c.safeInsets();
  const float w=c.width()-safe.left-safe.right,h=c.height()-safe.top-safe.bottom;
  u=std::min(w/1784.f,h/1004.f);
  x=safe.left+(w-1784*u)*.5f;y=safe.top+(h-1004*u)*.5f;
 }
 Rect rect(float px,float py,float w,float h)const{return {x+px*u,y+py*u,w*u,h*u};}
 Rect frame()const{return rect(286,120,1220,850);}
};
// Expand the Shop's vertical spacing to the shared dialog height. Type and
// icons keep a uniform scale, and the frame leaves the aquarium controls clear.
struct ShopLayout {
 float u{},v{},x{},y{};
 explicit ShopLayout(const Canvas& c) {
  auto frame=DialogLayout(c).frame();
  const auto space=HudLayout(c).menuSpace();
  frame.w=std::min(frame.w,space.w);
  frame.x=std::clamp(frame.x,space.x,space.x+space.w-frame.w);
  u=frame.w/1496.f;v=frame.h/824.f;
  x=frame.x-62*u;y=frame.y-88*v;
 }
 Rect rect(float px,float py,float w,float h)const{return {x+px*u,y+py*v,w*u,h*v};}
 Rect art(float px,float py,float w,float h)const{return {x+px*u,y+py*v,w*u,h*u};}
};
}
