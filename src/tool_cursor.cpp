#include "aquarium/canvas.hpp"
#include <algorithm>

namespace aq {
namespace {
// FishX's CSS ease-out curve, applied separately to each half of the gesture.
double easeOut(double x){
 double lo=0,hi=1,t=x;
 for(int i=0;i<20;++i){const double at=3*(1-t)*t*t*.58+t*t*t;if(at<x)lo=t;else hi=t;t=(lo+hi)*.5;}
 return 3*(1-t)*t*t+t*t*t;
}
}
ToolCursorPose toolCursorPose(Tool tool,double elapsed,bool reduced){
 const double duration=tool==Tool::Food?.2:.25;
 double pulse=0;
 if(!reduced&&elapsed>=0&&elapsed<duration){
  const double t=elapsed/duration;
  pulse=t<=.5?easeOut(t*2):1-easeOut((t-.5)*2);
 }
 return tool==Tool::Food?ToolCursorPose{-30-40*pulse,float(1-.04*pulse)}:ToolCursorPose{-18*pulse,float(1+.08*pulse)};
}
void Canvas::cursor(CursorKind kind){
 if(kind==cursorKind_)return;
 cursorKind_=kind;
 if(kind==CursorKind::Hidden){SDL_HideCursor();return;}
 SDL_ShowCursor();SDL_SetCursor(SDL_GetDefaultCursor());
}
void Canvas::toolCursor(Tool tool,SDL_FPoint point,double elapsed,bool reduced){
 const bool food=tool==Tool::Food;const auto pose=toolCursorPose(tool,elapsed,reduced);
 const auto name=food?"lagoon/food.png":"tools/sell-net.png";const auto* art=texture(name);
 const float w=(food?52.f:72.f)*worldScale()*pose.scale,h=w*art->height/art->width;
 const SDL_FPoint pivot=food?SDL_FPoint{.4f,.08f}:SDL_FPoint{.31f,.45f};
 const auto bottom=toScreen({waterWidth,waterHeight});clip({0,0,bottom.x,bottom.y});
 image(name,{point.x-pivot.x*w,point.y-pivot.y*h,w,h},pose.angle,pivot);clearClip();
}
}
