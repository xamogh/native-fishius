#include "aquarium/hud_decor_placement.hpp"
#include "aquarium/hud_tokens.hpp"
#include "aquarium/shop_theme.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
namespace {
const DecorDef& placementDef(const Domain& domain,std::string_view kind){
 if(const auto* def=domain.content().findDecor(kind))return *def;
 static const DecorDef legacy=[](){DecorDef def;def.width=1.1;def.height=.95;return def;}();
 return legacy;
}
bool available(const Domain& domain,const DecorPlacement& state){
 if(state.preview.tank!=domain.state().activeTank)return false;
 if(!state.moving)return true;
 const auto* item=domain.decoration(state.preview.id);
 return item&&!item->stored&&item->tank==state.preview.tank&&item->kind==state.preview.kind;
}
}
Rect decorDragBounds(Rect item,float minimumTouch){
 if(item.w<=0||item.h<=0)return item;
 const float width=std::max(item.w,minimumTouch),height=std::max(item.h,minimumTouch);
 return {item.x+(item.w-width)*.5f,item.y+(item.h-height)*.5f,width,height};
}
DecorPlacementLayout layoutDecorPlacement(float w,float h,Insets safe,float minimumTouch,Rect item){
 const float u=std::min(w/hudTokens::canvasWidth,h/hudTokens::canvasHeight),margin=hudTokens::margin*u;
 const Rect water{safe.left,safe.top,w-safe.left-safe.right,h-safe.top-safe.bottom};
 // Keep the tiles together, with extra touch padding on the outer edges.
 const float gap=hudTokens::measure(8)*u;
 // Match normal placement icon size and spacing.
 const float size=hudTokens::food*u;
 const float hitSize=std::max(size,minimumTouch),pad=(hitSize-size)*.5f;
 const float groupWidth=2*hitSize+gap;
 float x=water.x+(water.w-groupWidth)*.5f,y=water.y+water.h-margin-hitSize,iconOffsetY=hitSize-size;
 const float labelUnit=std::min(std::max(u,minimumTouch/64.f),(water.w-2*margin)/400.f);
 if(item.w>0&&item.h>0){
  const float top=water.y+margin,bottom=water.y+water.h-margin;
  const float left=water.x+margin,right=water.x+water.w-margin;
  struct Candidate {float x,y,iconOffsetY;};
  const std::array<Candidate,4> candidates{{
   {item.x+(item.w-groupWidth)*.5f,item.y+item.h+gap,0},
   {item.x+(item.w-groupWidth)*.5f,item.y-gap-hitSize,hitSize-size},
   {item.x+item.w+gap,item.y+(item.h-hitSize)*.5f,pad},
   {item.x-gap-groupWidth,item.y+(item.h-hitSize)*.5f,pad}
  }};
  const float maxX=std::max(left,right-groupWidth),maxY=std::max(top,bottom-hitSize);
  // Keep the visible row centered below or above the item. A side placement
  // puts the outer touch padding between the item and its first icon, so use
  // it only when neither vertical position fits. Clamp at the screen edges.
  x=std::clamp(candidates.front().x,left,maxX);y=std::clamp(candidates.front().y,top,maxY);iconOffsetY=candidates.front().iconOffsetY;
  for(const auto p:candidates)if(p.y>=top&&p.y+hitSize<=bottom){
   const float candidateX=std::clamp(p.x,left,maxX);
   const bool overlaps=candidateX<item.x+item.w&&candidateX+groupWidth>item.x&&p.y<item.y+item.h&&p.y+hitSize>item.y;
   if(!overlaps){x=candidateX;y=p.y;iconOffsetY=p.iconOffsetY;break;}
  }
 }
 return {{0,0,w,h},water,{x+hitSize-size,y+iconOffsetY,size,size},{x+hitSize+gap,y+iconOffsetY,size,size},
  {water.x+margin,water.y+margin,water.w-2*margin,64*labelUnit},
  {x,y,hitSize,hitSize},{x+hitSize+gap,y,hitSize,hitSize},labelUnit,minimumTouch*8/44,item,sceneProjection({0,0,w,h}),decorDragBounds(item,minimumTouch)};
}
DecorPlacementLayout layoutDecorPlacement(const Canvas& canvas,const Domain& domain,const DecorPlacement& state){
 return layoutDecorPlacement(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize(),
  state.active()?canvas.decorRect(placementDef(domain,state.preview.kind),state.preview.position,state.preview.sizeMul):Rect{});
}
namespace {
DecorPlacementControl control(const DecorPlacementLayout& l,SDL_FPoint p,bool moving){
 if(l.cancelHit.has(p.x,p.y))return moving?DecorPlacementControl::Remove:DecorPlacementControl::Cancel;
 if(l.placeHit.has(p.x,p.y))return DecorPlacementControl::Place;
 if(l.water.has(p.x,p.y))return moving&&!l.itemHit.has(p.x,p.y)?DecorPlacementControl::Outside:DecorPlacementControl::Water;
 return DecorPlacementControl::None;
}
void clearPress(DecorPlacement& state){state.pressed=DecorPlacementControl::None;state.dragged=false;}
void position(const Domain& domain,DecorPlacement& state,const DecorPlacementLayout& l,SDL_FPoint point){
 const auto world=l.scene.toWorld(point);
 state.preview.position=l.scene.place(placementDef(domain,state.preview.kind),
  {world.x+state.grabOffset.x,world.y+state.grabOffset.y},state.preview.sizeMul);
}
void placementIcon(Canvas& canvas,Rect r,bool confirm,bool pressed,bool remove=false){
 const float u=r.w/hudTokens::food;
 // Include the panel's shadow inside the tile bounds.
 shopTheme::panel(canvas,{r.x,r.y,r.w,r.h-4*u},u,confirm?shopTheme::Surface::Buy:shopTheme::Surface::Close,pressed);
 auto stroke=[&](SDL_FPoint a,SDL_FPoint b,float width,Color color){
  a={r.x+a.x*u,r.y+a.y*u};b={r.x+b.x*u,r.y+b.y*u};width*=u;
  const float length=std::hypot(b.x-a.x,b.y-a.y),dx=(b.y-a.y)/length*width*.5f,dy=(a.x-b.x)/length*width*.5f;
  canvas.triangle({SDL_FPoint{a.x+dx,a.y+dy},{a.x-dx,a.y-dy},{b.x+dx,b.y+dy}},color);
  canvas.triangle({SDL_FPoint{b.x+dx,b.y+dy},{a.x-dx,a.y-dy},{b.x-dx,b.y-dy}},color);
  for(const auto p:{a,b})canvas.gradient({p.x-width*.5f,p.y-width*.5f,width,width},color,color,width*.5f);
 };
 auto symbol=[&](float width,Color color){
  if(confirm){stroke({17,30},{28,41},width,color);stroke({28,41},{48,19},width,color);}
  else if(remove){
   stroke({18,21},{46,21},width,color);stroke({27,14},{37,14},width,color);
   stroke({22,27},{24,45},width,color);stroke({24,45},{40,45},width,color);stroke({40,45},{42,27},width,color);
   stroke({30,29},{30,38},width,color);stroke({36,29},{36,38},width,color);
  }else{stroke({21,19},{43,41},width,color);stroke({43,19},{21,41},width,color);}
 };
 symbol(remove?8:12,confirm?Color{30,108,53,255}:Color{135,47,37,255});
 symbol(remove?4:8,shopTheme::white);
}
}
void cancelDecorPlacement(DecorPlacement& state){auto receipts=std::move(state.receipts);state={};state.receipts=std::move(receipts);}
Result startDecorPlacement(const Domain& domain,DecorPlacement& state,std::string_view id){
 const auto* def=domain.content().findDecor(id);
 if(!def)return {.error=Error::Unknown,.message=errorText(Error::Unknown)};
 if(auto gate=domain.blocker(*def);!gate)return gate;
 cancelDecorPlacement(state);state.preview={domain.state().nextDecorId,def->id,domain.state().activeTank,decorPlacementPoint(*def,{544,441.1904})};
 return {};
}
Result startDecorMove(const Domain& domain,DecorPlacement& state,std::uint64_t id){
 const auto* item=domain.decoration(id);
 if(!item)return {.error=Error::Unknown,.message=errorText(Error::Unknown)};
 if(item->stored||item->tank!=domain.state().activeTank)return {.error=Error::NotReady,.message=errorText(Error::NotReady)};
 cancelDecorPlacement(state);state.preview=*item;state.moving=true;return {};
}
Result startDecorRestore(Session& session,DecorPlacement& state,std::uint64_t id){
 const auto& domain=session.domain();
 const auto* item=domain.decoration(id);
 if(!item)return {.error=Error::Unknown,.message=errorText(Error::Unknown)};
 if(!item->stored)return {.error=Error::NotReady,.message=errorText(Error::NotReady)};
 if(domain.placedDecor(domain.state().activeTank)>=std::size_t(domain.content().decorTuning.placedLimit))
  return {.error=Error::Maximum,.message="Stash an item first. This tank has "+std::to_string(domain.content().decorTuning.placedLimit)+" placed items."};
 const auto point=decorPlacementPoint(placementDef(domain,item->kind),{544,441.1904},item->sizeMul);
 const auto result=session.command({.action=Action::RestoreDecor,.tank=domain.state().activeTank,.point=point,.decor=id});
 return result?startDecorMove(session.domain(),state,id):result;
}
Result saveDecorMove(Session& session,DecorPlacement& state){
 if(!state.active()||!state.moving||!available(session.domain(),state))return {.error=Error::NotReady,.message=errorText(Error::NotReady)};
 const auto* saved=session.domain().decoration(state.preview.id);
 if(std::hypot(saved->position.x-state.preview.position.x,saved->position.y-state.preview.position.y)<.000001){state.preview=*saved;return {};}
 const auto result=session.command({.action=Action::MoveDecor,.tank=state.preview.tank,.point=state.preview.position,.decor=state.preview.id});
 // A failed drop returns to the last saved position. Earlier drops stay saved.
 state.preview=*session.domain().decoration(state.preview.id);
 state.error=result?std::string{}:result.message.empty()?errorText(result.error):result.message;
 return result;
}
Result removeDecorPlacement(Session& session,DecorPlacement& state){
 if(!state.active()||!state.moving||!available(session.domain(),state))return {.error=Error::NotReady,.message=errorText(Error::NotReady)};
 const auto result=session.command({.action=Action::StoreDecor,.decor=state.preview.id});
 if(result)cancelDecorPlacement(state);
 else state.error=result.message.empty()?errorText(result.error):result.message;
 return result;
}
Result confirmDecorPlacement(Session& session,DecorPlacement& state){
 if(!state.active())return {.error=Error::NotReady,.message=errorText(Error::NotReady)};
 if(!available(session.domain(),state)){cancelDecorPlacement(state);return {.error=Error::NotReady,.message=errorText(Error::NotReady)};}
 // Owned items are saved on each drop. The tick only clears their selection.
 if(state.moving){cancelDecorPlacement(state);return {};}
 const auto result=session.command({.action=Action::BuyDecor,.key=state.preview.kind,.point=state.preview.position});
 if(result){
  state.receipts.push_back({state.preview.position,result.pearls<0?-result.pearls:-result.coins,result.xp,result.pearls<0,0,true});
  if(state.receipts.size()>24)state.receipts.erase(state.receipts.begin());
  cancelDecorPlacement(state);
 }
 else if(result.error==Error::Funds){cancelDecorPlacement(state);state.shortfall=result.shortfall;}
 else state.error=result.message.empty()?errorText(result.error):result.message;
 return result;
}
PlacementEvent decorPlacementEvent(Session& session,DecorPlacement& state,const DecorPlacementLayout& l,const SDL_Event& e,SDL_FPoint point){
 if(!state.active())return PlacementEvent::None;
 if(!available(session.domain(),state)){cancelDecorPlacement(state);return PlacementEvent::Cancelled;}
 if(e.type==SDL_EVENT_KEY_DOWN&&!e.key.repeat){
  if(e.key.key==SDLK_ESCAPE){cancelDecorPlacement(state);return PlacementEvent::Cancelled;}
  if(e.key.key==SDLK_RETURN||e.key.key==SDLK_KP_ENTER){clearPress(state);return confirmDecorPlacement(session,state)?PlacementEvent::Placed:PlacementEvent::None;}
 }
 if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND||e.type==SDL_EVENT_WINDOW_RESIZED||e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET){
  if(state.pressed==DecorPlacementControl::Water)state.preview.position=state.original;
  clearPress(state);return PlacementEvent::None;
 }
 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT){
  clearPress(state);state.pressed=control(l,point,state.moving);state.down=point;state.original=state.preview.position;
  state.grabOffset={};
  if(state.pressed==DecorPlacementControl::Water&&l.itemHit.has(point.x,point.y)){
   const auto world=l.scene.toWorld(point);
   state.grabOffset={state.original.x-world.x,state.original.y-world.y};
  }
 }
 if(e.type==SDL_EVENT_MOUSE_MOTION||e.type==SDL_EVENT_MOUSE_BUTTON_UP){
  if(state.pressed!=DecorPlacementControl::None&&std::hypot(point.x-state.down.x,point.y-state.down.y)>l.dragThreshold)state.dragged=true;
 }
 // A water drag keeps ownership while crossing the following action buttons.
 if(e.type==SDL_EVENT_MOUSE_MOTION&&l.water.has(point.x,point.y)&&state.pressed==DecorPlacementControl::Water&&(!state.moving||state.dragged))
  position(session.domain(),state,l,point);
 if(e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.button==SDL_BUTTON_LEFT){
  const auto pressed=state.pressed;const bool tap=!state.dragged;clearPress(state);
  const auto released=control(l,point,state.moving);
  if(pressed==DecorPlacementControl::Water){
   if(state.moving&&tap)state.preview.position=state.original;
   else if(released==DecorPlacementControl::Water||released==DecorPlacementControl::Outside){
    position(session.domain(),state,l,point);
    state.error.clear();
    if(state.moving)saveDecorMove(session,state);
   }
   else state.preview.position=state.original;
  }else if(tap&&pressed==released){
   if(pressed==DecorPlacementControl::Cancel){cancelDecorPlacement(state);return PlacementEvent::Cancelled;}
   if(pressed==DecorPlacementControl::Remove&&removeDecorPlacement(session,state))return PlacementEvent::Cancelled;
   if((pressed==DecorPlacementControl::Place||pressed==DecorPlacementControl::Outside)&&confirmDecorPlacement(session,state))return PlacementEvent::Placed;
  }
 }
 return PlacementEvent::None;
}
void paintDecorPlacement(Canvas& canvas,const Session& session,const DecorPlacement& state,const DecorPlacementLayout& l,bool showNotice){
 if(!state.active())return;
 const float u=l.unit;
 placementIcon(canvas,l.cancel,false,(state.pressed==DecorPlacementControl::Cancel||state.pressed==DecorPlacementControl::Remove)&&!state.dragged,state.moving);
 placementIcon(canvas,l.place,true,state.pressed==DecorPlacementControl::Place&&!state.dragged);
 const std::string notice=session.saveFailed()?"Progress is not saved. Keep the game open to retry.":state.error;
 if(showNotice&&!notice.empty()){
  canvas.gradient(l.notice,{104,43,34,235},{104,43,34,235},12*u);
  canvas.text(notice,l.notice.x+l.notice.w*.5f,l.notice.y+20*u,24*u,shopTheme::white,true,l.notice.w-24*u,true,true);
 }
}
}
