#include "aquarium/view.hpp"
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
 SDL_ShowCursor();SDL_Cursor* selected=SDL_GetDefaultCursor();
 if(kind==CursorKind::Move){
  if(!moveCursor_){
   if(auto* image=IMG_Load((assets_/"controls/move.png").string().c_str())){
    if(auto* sized=SDL_ScaleSurface(image,40,40,SDL_SCALEMODE_LINEAR)){
     moveCursor_=SDL_CreateColorCursor(sized,20,20);SDL_DestroySurface(sized);
    }
    SDL_DestroySurface(image);
   }
   if(!moveCursor_)moveCursor_=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
  }
  if(moveCursor_)selected=moveCursor_;
 }
 if(kind==CursorKind::Stash){if(!stashCursor_)stashCursor_=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);if(stashCursor_)selected=stashCursor_;}
 SDL_SetCursor(selected);
}
void Canvas::toolCursor(Tool tool,SDL_FPoint point,double elapsed,bool reduced){
 const bool food=tool==Tool::Food;const auto pose=toolCursorPose(tool,elapsed,reduced);
 const auto name=food?"lagoon/food.png":"tools/sell-net.png";const auto* art=texture(name);
 const float w=(food?52.f:72.f)*worldScale()*pose.scale,h=w*art->height/art->width;
 const SDL_FPoint pivot=food?SDL_FPoint{.4f,.08f}:SDL_FPoint{.31f,.45f};
 const auto bottom=toScreen({waterWidth,waterHeight});clip({0,0,bottom.x,bottom.y});
 image(name,{point.x-pivot.x*w,point.y-pivot.y*h,w,h},pose.angle,pivot);clearClip();
}
bool View::pointerInTank()const{
 if((session_.notification()&&storageNoticeRect_.has(pointerX_,pointerY_))||fundsDialog_||!pointerSeen_||panel_!=Panel::None||panelLayer_)return false;
 const auto point=canvas_.toWorld(pointerX_,pointerY_);
 const bool decorArea=decorPreview_||tool_==Tool::Select||tool_==Tool::Move||tool_==Tool::Stash||tool_==Tool::Decor;
 if(!(decorArea?inTank(point):inPlacementWater(point)))return false;
 return std::none_of(buttons_.begin(),buttons_.end(),[&](const auto& button){return button.area.has(pointerX_,pointerY_);});
}
void View::toolPointer(){
 if(tool_==Tool::Buy&&!buySpecies_.empty()&&panel_==Panel::None&&!panelLayer_&&pointerSeen_&&!fundsDialog_&&levelUps_.empty()){
  const bool overControl=std::any_of(buttons_.begin(),buttons_.end(),[&](const auto& b){return (b.id=="done"||b.id.starts_with("save-"))&&b.area.has(pointerX_,pointerY_);});
  const bool visible=!overControl&&(!pointerTouch_||pointerDown_);
  canvas_.cursor(visible&&!pointerTouch_?CursorKind::Hidden:CursorKind::Arrow);
  if(visible){
   const auto* species=session_.domain().content().find(buySpecies_);const bool companion=species&&species->companion;
   const float size=std::max(20.f,canvas_.worldScale()*(companion?64.f:30.f));
   canvas_.icon(companion?fishArt(species->id):"ui/egg.png",{pointerX_-size*.5f,pointerY_-size*.5f,size,size});
  }
  return;
 }
 const auto movePointer=[&]{
  canvas_.cursor(pointerTouch_?CursorKind::Arrow:CursorKind::Move);
  if(!pointerTouch_||!selectionDragging_)return;
  const float size=72.f,offset=canvas_.minimumTouchSize()*.85f;
  canvas_.icon("controls/move.png",{pointerX_-size*.5f,pointerY_-size*.5f-offset,size,size});
 };
 if(selectionDragging_){movePointer();return;}
 if(pointerSeen_&&(tool_==Tool::Select||tool_==Tool::Decor)&&panel_==Panel::Details&&selected_.value&&!pointerTouch_&&hitFish(pointerX_,pointerY_)==selected_&&!panelDisplayRect_.has(pointerX_,pointerY_)){
  movePointer();return;
 }
 if(!pointerInTank()){canvas_.cursor(CursorKind::Arrow);return;}
 if(decorPreview_||((tool_==Tool::Select||tool_==Tool::Decor)&&((selected_.value&&hitFish(pointerX_,pointerY_)==selected_)||(selectedDecor_&&hitDecor(pointerX_,pointerY_)==selectedDecor_)))){movePointer();return;}
 if(tool_==Tool::Stash){canvas_.cursor(pointerTouch_?CursorKind::Arrow:CursorKind::Stash);return;}
 if(tool_==Tool::Food||tool_==Tool::Sell){
  const double elapsed=now_-(tool_==Tool::Food?jarAt_:netAt_);
  // Touch has no hover. Keep the can/net visible through its tap animation.
  const bool visible=!pointerTouch_||pointerDown_||(elapsed>=0&&elapsed<(tool_==Tool::Food?.2:.25));
  canvas_.cursor(visible&&!pointerTouch_?CursorKind::Hidden:CursorKind::Arrow);
  if(visible)canvas_.toolCursor(tool_,{pointerX_,pointerY_},elapsed,reducedMotion());
  return;
 }
 canvas_.cursor(CursorKind::Arrow);
}
}
