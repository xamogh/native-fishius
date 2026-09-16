#include "aquarium/hud_placement.hpp"
#include "aquarium/shop_theme.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
FishPlacementLayout layoutFishPlacement(float w,float h,Insets safe,float minimumTouch){
 const float u=std::min(w/1608.f,h/908.f);
 Rect banner{(w-480*u)*.5f,h-safe.bottom-96*u,480*u,64*u};
 return {{0,0,w,h},{safe.left+24*u,safe.top+96*u,w-safe.left-safe.right-48*u,h-safe.top-safe.bottom-120*u},banner,{banner.x+banner.w-120*u,banner.y,120*u,64*u},u,layoutHud(w,h,safe,minimumTouch)};
}
namespace {
bool canPlace(const FishPlacementLayout& l,SDL_FPoint p){
 if(!l.water.has(p.x,p.y)||l.banner.has(p.x,p.y)||hudHit(l.hud,p))return false;
 return true;
}
void finish(FishPlacement& state){state.species.clear();state.error.clear();state.pointerDown=state.cancelPressed=false;}
}

Result startFishPlacement(const Domain& domain,FishPlacement& state,std::string_view id){
 const auto* species=domain.content().find(id);
 if(!species)return {.error=Error::Unknown,.message=errorText(Error::Unknown)};
 if(auto gate=domain.blocker(*species);!gate)return gate;
 state={};state.species=species->id;state.offer=domain.quote(*species);return {};
}
Result confirmFishPlacement(Session& session,FishPlacement& state,WorldPoint point){
 if(!state.active())return {.error=Error::Unknown,.message=errorText(Error::Unknown)};
 auto result=session.command({.action=Action::Buy,.key=state.species,.point=point,.offer=state.offer});
 if(result){
  state.error.clear();state.point=point;
  state.receipts.push_back({point,result.pearls<0?-result.pearls:-result.coins,result.xp,result.pearls<0,0});
  if(state.receipts.size()>24)state.receipts.erase(state.receipts.begin());
  if(const auto* species=session.domain().content().find(state.species))state.offer=session.domain().quote(*species);
 }else state.error=result.message.empty()?errorText(result.error):result.message;
 return result;
}
PlacementEvent fishPlacementEvent(Session& session,FishPlacement& state,const FishPlacementLayout& l,const SDL_Event& event,SDL_FPoint point){
 if(!state.active())return PlacementEvent::None;
 if(event.type==SDL_EVENT_KEY_DOWN&&event.key.key==SDLK_ESCAPE){finish(state);return PlacementEvent::Cancelled;}
 if((event.type==SDL_EVENT_WINDOW_FOCUS_LOST||event.type==SDL_EVENT_WILL_ENTER_BACKGROUND)){state.pointerDown=state.cancelPressed=false;return PlacementEvent::None;}
 if(event.type==SDL_EVENT_MOUSE_MOTION&&canPlace(l,point))state.point={point.x/l.page.w*waterWidth,point.y/l.page.h*waterHeight};
 if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT){state.down=point;state.cancelPressed=l.cancel.has(point.x,point.y);state.pointerDown=canPlace(l,point);}
 if(event.type==SDL_EVENT_MOUSE_BUTTON_UP&&event.button.button==SDL_BUTTON_LEFT){
  const bool cancel=state.cancelPressed&&l.cancel.has(point.x,point.y);
  const bool place=state.pointerDown&&canPlace(l,point)&&std::hypot(point.x-state.down.x,point.y-state.down.y)<24*l.unit;
  state.pointerDown=state.cancelPressed=false;
  if(cancel){finish(state);return PlacementEvent::Cancelled;}
  if(place&&confirmFishPlacement(session,state,{point.x/l.page.w*waterWidth,point.y/l.page.h*waterHeight}))return PlacementEvent::Placed;
 }
 return PlacementEvent::None;
}
void advanceFishPlacement(FishPlacement& state,double seconds){
 for(auto& receipt:state.receipts)receipt.age+=seconds;
 std::erase_if(state.receipts,[](const auto& receipt){return receipt.age>=1.6;});
}
void paintPlacementReceipts(Canvas& canvas,const FishPlacement& state,float u){
 for(const auto& receipt:state.receipts){
  const auto p=canvas.toScreen(receipt.point);const float rise=float(receipt.age)*48*u;
  const Uint8 alpha=Uint8(255*std::clamp((1.6-receipt.age)/.4,0.,1.));
  const std::string cost="-"+compact(receipt.cost),xp="+"+compact(receipt.xp);
  const float size=28*u,icon=28*u;
  const float costWidth=canvas.textWidth(cost,size,true,false,false,false,true),xpWidth=canvas.textWidth(xp,size,true,false,false,false,true);
  const float width=costWidth+xpWidth+2*icon+20*u;
  const float x=std::clamp(p.x-width*.5f,8*u,canvas.width()-width-8*u),y=std::max(100*u,p.y-80*u-rise);
  auto amount=[&](std::string_view text,float left,Color color){
   canvas.text(text,left+u,y+u,size,{24,59,68,Uint8(alpha*.65f)},false,0,true,true);
   canvas.text(text,left,y,size,color,false,0,true,true);
  };
  amount(cost,x,{255,91,85,alpha});
  canvas.icon(receipt.pearl?"hud-icons/pearl-v4.png":"hud-icons/coin-v4.png",{x+costWidth+4*u,y,icon,icon},float(alpha)/255);
  const float xpX=x+costWidth+icon+16*u;
  amount(xp,xpX,{112,255,112,alpha});
  const Rect badge{xpX+xpWidth+4*u,y,icon,icon};
  canvas.gradient(badge,{60,178,230,alpha},{23,108,179,alpha},6*u);
  canvas.text("XP",badge.x+icon*.5f,y+6*u,16*u,{255,255,255,alpha},true,icon-4*u,true,true);
 }
}
void paintFishPlacement(Canvas& canvas,const Domain& domain,const FishPlacement& state,const FishPlacementLayout& l){
 const auto* species=domain.content().find(state.species);if(!species)return;
 const float u=l.unit;const auto p=canvas.toScreen(state.point);
 canvas.outline({p.x-56*u,p.y-56*u,112*u,112*u},{212,255,176,230},56*u,4*u);
 canvas.icon(species->companion?species->asset:"ui/egg.png",{p.x-44*u,p.y-44*u,88*u,88*u},.85f);
 shopTheme::panel(canvas,l.banner,u,shopTheme::Surface::Button);
 const std::string instruction="Place "+species->name+(species->companion?"":" eggs");
 canvas.text(instruction,l.banner.x+24*u,l.banner.y+20*u,24*u,shopTheme::white,false,l.banner.w-160*u,true,true);
 shopTheme::panel(canvas,l.cancel,u,shopTheme::Surface::Buy,state.cancelPressed);
 canvas.text("Done",l.cancel.x+l.cancel.w*.5f,l.cancel.y+16*u,28*u,shopTheme::white,true,l.cancel.w,true,true);
 if(!state.error.empty()){
  Rect note{l.banner.x-80*u,l.banner.y-64*u,l.banner.w+160*u,48*u};
  canvas.gradient(note,{104,43,34,235},{104,43,34,235},8*u);
  canvas.text(state.error,note.x+note.w*.5f,note.y+12*u,24*u,shopTheme::white,true,note.w-24*u,true,true);
 }
}
}
