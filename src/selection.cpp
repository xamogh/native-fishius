#include "aquarium/view.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace aq {
void View::selectItem(float x,float y){
 auto& domain=session_.domain();
 const auto fish=hitFish(x,y);
 const auto decor=fish.value?0:hitDecor(x,y);
 if(!fish.value&&!decor){open(Panel::None);return;}
 selected_=fish;selectedDecor_=decor;if(fish.value)tool_=Tool::Select;
 if(decor&&panel_==Panel::Details){panel_=Panel::None;panelMotion_.show(false,now_);panelLayer_.reset();}
 dragOriginal_=fish.value?(domain.fish(fish)?domain.fish(fish)->position:domain.companion(fish)->position):domain.decoration(decor)->position;
 const auto point=canvas_.toWorld(x,y);
 dragOffset_={dragOriginal_.x-point.x,dragOriginal_.y-point.y};
 selectionDown_={x,y};selectionGesture_=true;selectionDragging_=false;
}

void View::moveSelection(float x,float y){
 if(!selectionGesture_)return;
 // A small touch wobble is still a tap. Keep the point grabbed by the user
 // under their finger instead of snapping the object's centre to it.
 if(!selectionDragging_){
  if(std::hypot(x-selectionDown_.x,y-selectionDown_.y)<canvas_.minimumTouchSize()*8/44)return;
  selectionDragging_=true;
  panel_=Panel::None;panelMotion_.show(false,now_);panelLayer_.reset();
  buttons_.resize(std::min(buttons_.size(),panelButtonStart_));
  if(selectedDecor_)beginMoveDecor(selectedDecor_);else{dragged_=selected_;fishPreview_=dragOriginal_;}
  SDL_CaptureMouse(true);
 }
 auto point=canvas_.toWorld(x,y);
 if(!inTank(point))return;
 point.x+=dragOffset_.x;point.y+=dragOffset_.y;
 if(draggingDecor_)decorPreview_=decorPreviewPoint(point);
 else if(dragged_.value){
  point.x=std::clamp(point.x,0.,waterWidth);point.y=std::clamp(point.y,56.,waterHeight);
  fishPreview_=point;
 }
}

void View::finishSelection(float x,float y){
 const bool moved=selectionDragging_;
 const auto point=canvas_.toWorld(x,y);
 const bool overControl=std::any_of(buttons_.begin(),buttons_.end(),[&](const auto& button){
  return button.id!="selection-stash"&&button.area.has(x,y);
 });
 const bool valid=(selectedDecor_?inTank(point):inPlacementWater(point))&&!overControl;
 if(moved){
  if(draggingDecor_&&!valid){decorPreview_.reset();draggingDecor_=false;}
  else if(dragged_.value){
   if(valid&&fishPreview_&&command({.action=Action::Move,.fish=dragged_,.point=*fishPreview_}))session_.checkpoint(session_.domain().state().wallAnchor);
   dragged_={};fishPreview_.reset();
  }
 }
 selectionGesture_=selectionDragging_=false;SDL_CaptureMouse(false);
 if(!moved&&valid&&selected_.value)open(Panel::Details);
}

void View::stashSelection(){
 if(!selectedDecor_)return;
 inventoryCategory_=1;
 if(command({.action=Action::StoreDecor,.decor=selectedDecor_}))setTool(Tool::Decor);
}

void View::selectionControls(){
 if(!selectedDecor_||tool_!=Tool::Decor||selectionDragging_||decorPreview_||panel_!=Panel::None)return;
 const auto& domain=session_.domain();Rect item{};std::string name;
 if(selectedDecor_){
  const auto* decor=domain.decoration(selectedDecor_);
  if(!decor||decor->stored||decor->tank!=domain.state().activeTank)return;
  const auto* source=domain.content().findDecor(decor->kind);
  DecorDef legacy;legacy.width=1.1;legacy.height=.95;
  item=canvas_.decorRect(source?*source:legacy,decor->position,decor->sizeMul);
  name=source?source->name:"Decoration";
 }
 const auto safe=canvas_.safeInsets();
 const float buttonSize=std::max(116.f,canvas_.minimumTouchSize()),width=std::max(250.f,buttonSize),height=buttonSize+74;
 const Rect bounds{std::max(20.f,safe.left+12),std::max(108.f,safe.top+100),
                   canvas_.width()-std::max(20.f,safe.left+12)-std::max(154.f,safe.right+12),
                   canvas_.height()-std::max(108.f,safe.top+100)-std::max(110.f,safe.bottom+12)};
 const float centreX=item.x+item.w*.5f,centreY=item.y+item.h*.5f;
 const std::array<Rect,4> choices{{{centreX-width*.5f,item.y+item.h+14,width,height},
                                 {centreX-width*.5f,item.y-height-14,width,height},
                                 {item.x+item.w+14,centreY-height*.5f,width,height},
                                 {item.x-width-14,centreY-height*.5f,width,height}}};
 auto overlap=[](Rect a,Rect b){return std::max(0.f,std::min(a.x+a.w,b.x+b.w)-std::max(a.x,b.x))*std::max(0.f,std::min(a.y+a.h,b.y+b.h)-std::max(a.y,b.y));};
 Rect chosen{};float best=std::numeric_limits<float>::max();
 for(auto candidate:choices){
  candidate.x=std::clamp(candidate.x,bounds.x,bounds.x+bounds.w-width);
  candidate.y=std::clamp(candidate.y,bounds.y,bounds.y+bounds.h-height);
  float score=overlap(candidate,item);
  if(panel_==Panel::Details)score+=overlap(candidate,panelDisplayRect_);
  for(const auto& control:buttons_)score+=overlap(candidate,control.area);
  if(score<best){chosen=candidate;best=score;}
 }
 const float cx=chosen.x+width*.5f;
 canvas_.label(name,cx,chosen.y,27,true,width-8,{255,248,217,255},1.5f);
 canvas_.icon("controls/move.png",{cx-89,chosen.y+35,28,28});
 canvas_.label("Drag to move",cx+16,chosen.y+37,21,true,160,{232,255,236,255},1);
 const Rect button{cx-buttonSize*.5f,chosen.y+70,buttonSize,buttonSize};
 const auto visual=buttonVisual("selection-stash",button);
 canvas_.icon("controls/stash.png",visual);
 touchTarget("selection-stash",button,[this]{stashSelection();});
}
}
