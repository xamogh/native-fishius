#include "aquarium/hud_tanks.hpp"
#include "aquarium/shop_theme.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
namespace {
constexpr Color numberInk{12,48,63,255};
bool cancelled(const SDL_Event& e){return e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND||e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET;}
bool leftButton(const SDL_Event& e){return (e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)&&e.button.button==SDL_BUTTON_LEFT;}
void label(Canvas& canvas,std::string_view text,Rect r,float size,Color color=numberInk){canvas.text(text,r.x+r.w*.5f,r.y+(r.h-size)*.5f,size,color,true,r.w,true,true);}
void tankIllustration(Canvas& canvas,Rect art,bool owned,float alpha=1){
 const Rect source=owned?Rect{48,87,1678,731}:Rect{47,86,1680,732};
 const float scale=std::min(art.w/source.w,art.h/source.h);
 canvas.image(owned?"tank-grid/active.png":"tank-grid/empty.png",{art.x+(art.w-source.w*scale)*.5f-source.x*scale,art.y+(art.h-source.h*scale)*.5f-source.y*scale,1774*scale,887*scale},0,{.5f,.5f},alpha);
}
void porthole(Canvas& canvas,Rect r,int index,bool selected,bool owned){
 static constexpr std::array<std::string_view,6> art{
  "tank-switcher/tank-1-goldfish.png","tank-switcher/tank-2-clownfish.png","tank-switcher/tank-3-blue-tang.png",
  "tank-switcher/tank-4-pink-fish.png","tank-switcher/tank-5-yellow-fish.png","tank-switcher/tank-6-purple-fish.png"};
 // Fit the artwork's visible alpha bounds, rather than its transparent canvas.
 static constexpr std::array<Rect,6> bounds{{{154,129,943,969},{62,25,1130,1147},{118,95,1014,1038},{153,123,945,975},{122,100,1005,1032},{110,83,1031,1062}}};
 const float scale=std::min(r.w/943.f,r.h/969.f);
 const Rect slot{r.x+(r.w-943*scale)*.5f,r.y+(r.h-969*scale)*.5f,943*scale,969*scale};
 auto fit=[&](std::string_view asset,Rect visible,float alpha=1.f,Color tint={255,255,255,255}){
  const float sx=slot.w/visible.w,sy=slot.h/visible.h;
  canvas.image(asset,{slot.x-visible.x*sx,slot.y-visible.y*sy,1254*sx,1254*sy},0,{.5f,.5f},alpha,false,tint);
 };
 const auto assetIndex=std::clamp(index,0,5);
 fit(art[assetIndex],bounds[assetIndex],1,owned?Color{255,255,255,255}:Color{108,151,163,255});
 fit("tank-switcher/normal-rim.png",{120,87,1012,1046});
 if(selected)fit("tank-switcher/tank-selected-overlay.png",{153,127,945,970},.78f);
 if(!owned)shopTheme::lockIcon(canvas,{r.x+r.w*.35f,r.y+r.h*.24f,r.w*.30f,r.h*.33f});
 if(selected||!owned){
  const Rect badge{r.x+r.w*.14f,r.y+r.h*.60f,r.w*.72f,r.h*.16f};
  const float u=r.h/154.f;
  canvas.gradient({badge.x,badge.y+2*u,badge.w,badge.h},{7,42,51,110},{7,42,51,110},10*u);
  canvas.gradient(badge,selected?Color{255,238,150,255}:Color{30,86,99,255},selected?Color{248,195,54,255}:Color{16,61,77,255},10*u);
  canvas.outline(badge,selected?Color{145,91,20,255}:Color{161,212,209,255},10*u,1.5f*u);
  label(canvas,selected?"Current":index==5?"Coming soon":"Locked",badge,(index==5?16:20)*u,selected?numberInk:shopTheme::white);
 }
 // Lilita One keeps the live digits upright, like the approved number tabs.
 canvas.text(std::to_string(index+1),r.x+r.w*.5f,r.y+r.h*.780f,r.h*.17f,numberInk,true,r.w*.32f,true);
}
}

TankOffer tankOffer(const Domain& domain,TankId id){
 TankOffer result{domain.nextTankEntitlement(id),{}};
 if(!result.next)return result;
 if(!domain.tank(id)&&domain.level()<result.next->level)result.requirement="Unlocks at Level "+std::to_string(result.next->level);
 if(!result.next->prerequisite.empty()){
  const auto& entries=domain.content().tankEntitlements;
  const auto prior=std::find_if(entries.begin(),entries.end(),[&](const auto& e){return e.id==result.next->prerequisite;});
  if(prior!=entries.end()){
   const auto* tank=domain.tank(prior->tank);
   if(!tank||tank->slots<prior->slots){
    if(result.requirement.empty())result.requirement="Not available yet";
   }
  }
 }
 return result;
}

TankSwitcherLayout layoutTankSwitcher(const Domain& domain,const HudLayout& hud){
 TankSwitcherLayout l{};l.toggle=hud[HudPart::Tank];l.unit=l.toggle.w/144.f;
 l.count=int(l.ids.size());
 const float u=l.unit;const int columns=std::min(3,std::max(1,l.count)),rows=(l.count+2)/3;
 const float width=std::max(242.f,float(columns)*158-2)*u;
 const float left=l.toggle.x-8*u,bottom=l.toggle.y-12*u;
 const float top=bottom-float(std::max(0,rows-1)*156+154)*u;
 l.header={left+(width-242*u)*.5f,top-42*u,242*u,40*u};
 for(int i=0;i<l.count;++i){
  l.ids[i]={i+1};if(domain.tank(l.ids[i]))++l.ownedCount;
  l.tanks[i]={left+float(i%3)*158*u,top+float(i/3)*156*u,156*u,154*u};
 }
 return l;
}
int tankSwitcherControl(const TankSwitcherLayout& l,SDL_FPoint p){
 if(l.toggle.has(p.x,p.y))return 6;
 for(int i=0;i<l.count;++i){
  const auto r=l.tanks[i];const float x=(p.x-r.x-r.w*.5f)/(r.w*.5f),y=(p.y-r.y-r.h*.465f)/(r.h*.465f);
  if(x*x+y*y<=1||Rect{r.x+r.w*.32f,r.y+r.h*.78f,r.w*.36f,r.h*.22f}.has(p.x,p.y))return i;
 }
 return 7; // Aquarium backdrop, including the non-interactive title plate.
}
bool tankSwitcherEvent(Session& session,TankSwitcherState& state,const HudLayout& hud,const SDL_Event& e,SDL_FPoint p){
 if(cancelled(e)){state.pressed=-1;state.open=false;state.shopTarget.reset();return false;}
 const auto l=layoutTankSwitcher(session.domain(),hud);
 auto choose=[&](int index){
  if(!session.domain().tank(l.ids[index])){state.shopTarget=l.ids[index];state.open=false;state.pressed=-1;state.notice.clear();return;}
  if(l.ids[index]==session.domain().state().activeTank){state.open=false;state.notice.clear();return;}
  const auto result=session.command({.action=Action::SwitchTank,.tank=l.ids[index]});
  if(result){state.open=false;state.notice.clear();}
  else state.notice=result.message.empty()?errorText(result.error):result.message;
 };
 if(state.open&&e.type==SDL_EVENT_KEY_DOWN){
  if(e.key.key==SDLK_ESCAPE){state.open=false;state.pressed=-1;return true;}
  if(e.key.key>=SDLK_1&&e.key.key<=SDLK_6){
   if(!e.key.repeat)for(int i=0;i<l.count;++i)if(l.ids[i].value==int(e.key.key-SDLK_1)+1){choose(i);break;}
   return true;
  }
 }
 if(leftButton(e)){
  const auto point=state.open?state.motion.inputPoint(p):p;
  const int animatedControl=tankSwitcherControl(l,point);
  const int control=l.toggle.has(p.x,p.y)?6:animatedControl==6?7:animatedControl;
  if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){
   if(!state.open&&control!=6)return false;
   if(state.open&&control==7){
    // Other HUD controls remain one click away when the switcher is open.
    if(const auto hit=hudHit(hud,p);hit&&*hit!=HudPart::Tank){state.open=false;state.pressed=-1;return false;}
   }
   state.pressed=control;state.pressPoint=p;state.dragged=false;return true;
  }
  const int pressed=state.pressed;state.pressed=-1;
  if(pressed<0)return state.open;
  if(!state.dragged&&control==pressed&&std::hypot(p.x-state.pressPoint.x,p.y-state.pressPoint.y)<=12*l.unit){
   if(control==6){state.open=!state.open;state.motion={};state.notice.clear();state.shopTarget.reset();}
   else if(state.open&&control<l.count)choose(control);
   else if(state.open&&control==7)state.open=false;
  }
  return true;
 }
 if(e.type==SDL_EVENT_MOUSE_MOTION){
  if(state.pressed>=0&&std::hypot(p.x-state.pressPoint.x,p.y-state.pressPoint.y)>12*l.unit)state.dragged=true;
  return state.open||state.pressed>=0;
 }
 return state.open&&(e.type==SDL_EVENT_MOUSE_WHEEL||e.type==SDL_EVENT_KEY_DOWN||e.type==SDL_EVENT_KEY_UP);
}
void paintTankSwitcher(Canvas& canvas,const Domain& domain,const HudLayout& hud,const TankSwitcherState& state,SDL_FPoint pointer){
 if(!state.open)return;
 const auto l=layoutTankSwitcher(domain,hud);const float u=l.unit;const auto h=l.header;
 Rect frame=h;
 for(int i=0;i<l.count;++i){const auto r=l.tanks[i];const float right=std::max(frame.x+frame.w,r.x+r.w),bottom=std::max(frame.y+frame.h,r.y+r.h);frame.x=std::min(frame.x,r.x);frame.y=std::min(frame.y,r.y);frame.w=right-frame.x;frame.h=bottom-frame.y;}
 const DialogPaint animation(canvas,state.motion,frame,domain.state().settings.reducedMotion,false);
 pointer=state.motion.inputPoint(pointer);
 const float sx=h.w/1747.f,sy=h.h/265.f;
 canvas.image("tank-switcher/header.png",{h.x-98*sx,h.y-265*sy,1944*sx,809*sy});
 label(canvas,"MY TANKS",{h.x+12*u,h.y+3*u,h.w-80*u,h.h},27*u);
 label(canvas,std::to_string(l.ownedCount)+"/6",{h.x+h.w*.75f,h.y+3*u,h.w*.25f,h.h},24*u);
 const int hover=tankSwitcherControl(l,pointer);
 for(int i=0;i<l.count;++i){
  auto r=l.tanks[i];const bool active=l.ids[i]==domain.state().activeTank;
  if(state.pressed==i&&!state.dragged){r.x+=2*u;r.y+=2*u;r.w-=4*u;r.h-=4*u;}
  porthole(canvas,r,l.ids[i].value-1,active,domain.tank(l.ids[i])!=nullptr);
  if(hover==i&&!active)canvas.outline({r.x+2*u,r.y+2*u,r.w-4*u,r.h*.92f-4*u},{255,251,205,145},r.w*.5f,2*u);
 }
 if(!state.notice.empty()){
  const Rect message{h.x,h.y-44*u,std::max(h.w,320*u),36*u};
  canvas.gradient(message,{255,248,220},{255,233,189},10*u);label(canvas,state.notice,message,20*u,{126,44,28,255});
 }
}

TankShopLayout layoutTankShop(const ShopLayout& page,float scroll){
 TankShopLayout l{};const float u=page.unit;
 l.portrait=page.page.h>page.page.w*1.2f;
 l.viewport=page.cardViewport;
 const float left=l.viewport.x,availableWidth=l.viewport.w;
 const float width=l.portrait?availableWidth*.84f:std::max(432*u,page.minimumTouch*432/172);
 l.unit=l.portrait?width/432:u;
 const float gap=page.cards[1].x-page.cards[0].x-page.cards[0].w;
 const float top=page.cards[0].y,height=page.cards[0].h;
 l.step=width+gap;
 const float contentWidth=6*width+5*gap;
 l.maxScroll=std::max(0.f,(contentWidth-availableWidth)/l.step);
 l.scroll=std::clamp(std::isfinite(scroll)?scroll:0.f,0.f,l.maxScroll);
 for(int i=0;i<6;++i){
  const Rect r{left+(float(i)-l.scroll)*l.step,top,width,height};l.cards[i]=r;
  const float inset=12*u,actionHeight=56*l.unit;
  l.actions[i]={r.x+inset,r.y+r.h-inset-actionHeight,r.w-2*inset,actionHeight};
 }
 l.scrollTrack=page.scrollTrack;const float trackWidth=l.scrollTrack.w;
 const float thumbWidth=std::min(trackWidth,std::max(40*u,trackWidth*availableWidth/contentWidth));
 l.scrollThumb={l.scrollTrack.x+(l.maxScroll>0?l.scroll/l.maxScroll:0)*(trackWidth-thumbWidth),l.scrollTrack.y,thumbWidth,l.scrollTrack.h};
 l.scrollHitArea={l.scrollTrack.x,l.scrollTrack.y-8*u,trackWidth,20*u};
 return l;
}
TankPurchaseLayout layoutTankPurchase(const ShopLayout& page){
 const float base=page.unit,scale=std::max(base,page.minimumTouch/64.f);
 const Insets safe{page.header.x,page.header.y,page.page.w-page.header.x-page.header.w,page.page.h-page.footer.y-page.footer.h};
 auto dialog=layoutDialog(page.page.w,page.page.h,safe,{"",DialogSize::Custom,800*scale/base,584*scale/base});
 const float u=dialog.frame.w/800.f,close=std::max(64*u,page.minimumTouch),header=close+16*u;
 dialog.unit=u;
 dialog.header={dialog.frame.x+8*u,dialog.frame.y+8*u,dialog.frame.w-16*u,header};
 dialog.close={dialog.header.x+dialog.header.w-close-8*u,dialog.header.y+8*u,close,close};
 dialog.title={dialog.header.x+16*u,dialog.header.y,dialog.header.w-close-40*u,header};
 dialog.body={dialog.frame.x+8*u,dialog.header.y+header,dialog.frame.w-16*u,dialog.frame.h-header-16*u};
 dialog.content={dialog.body.x+24*u,dialog.body.y+20*u,dialog.body.w-48*u,dialog.body.h-40*u};
 const auto c=dialog.content;const float height=std::max(88*u,page.minimumTouch),gap=40*u,buttonWidth=(c.w-gap)*.5f;
 const Rect coins{c.x,c.y+c.h-height,buttonWidth,height},pearls{c.x+c.w-buttonWidth,coins.y,buttonWidth,height};
 const Rect message{c.x,coins.y-48*u,c.w,32*u};
 const Rect capacity{c.x,message.y-56*u,c.w,40*u};
 const Rect art{c.x+40*u,c.y,c.w-80*u,std::max(0.f,capacity.y-c.y-16*u)};
 return {dialog,art,capacity,message,coins,pearls,{coins.x+coins.w,coins.y,gap,height}};
}
namespace {
void cancelTankShopPress(TankShopState& state){state.pressed=-1;state.pointerDown=state.scrollbarDrag=false;}
bool tankPurchaseEvent(Session& session,TankShopState& state,const ShopLayout& page,const SDL_Event& e,SDL_FPoint p){
 auto& purchase=state.purchase;
 const auto screenPoint=p;p=purchase.dialog.motion.inputPoint(p);
 if(cancelled(e)){
  purchase.pressed=-1;purchase.dragged=false;purchase.dialog.closePressed=purchase.dialog.backdropPressed=false;
  cancelTankShopPress(state);return false;
 }
 const auto l=layoutTankPurchase(page);
 const int control=l.coins.has(p.x,p.y)?0:l.pearls.has(p.x,p.y)?1:-1;
 const float threshold=std::max(8*page.unit,page.minimumTouch*6/44);
 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT){purchase.pressed=control;purchase.pressPoint=screenPoint;purchase.dragged=false;}
 if(e.type==SDL_EVENT_MOUSE_MOTION&&std::hypot(screenPoint.x-purchase.pressPoint.x,screenPoint.y-purchase.pressPoint.y)>threshold){
  purchase.dragged=true;purchase.dialog.closePressed=purchase.dialog.backdropPressed=false;
 }
 if(e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.button==SDL_BUTTON_LEFT){
  const bool activate=control>=0&&control==purchase.pressed&&!purchase.dragged&&std::hypot(screenPoint.x-purchase.pressPoint.x,screenPoint.y-purchase.pressPoint.y)<=threshold;
  purchase.pressed=-1;
  if(activate){
   const auto id=purchase.offer.tank;const auto offer=tankOffer(session.domain(),id);
   // Confirm only the capacity and price that the player reviewed.
   if(!offer.available()||offer.next->id!=purchase.offer.id){
    state.noticeTank=id;state.notice=errorText(Error::Conflict);purchase={};return true;
   }
   const auto result=session.command({.action=purchase.upgrade?Action::ExpandTank:Action::UnlockTank,.tank=id,.currency=control?Currency::Pearls:Currency::Coins});
   state.shortfall=result.error==Error::Funds?std::optional{result.shortfall}:std::nullopt;
   if(result){state.noticeTank=id;state.notice=purchase.upgrade?"Tank upgraded!":"Tank unlocked!";purchase={};}
   else if(result.error==Error::Funds){state.notice.clear();purchase={};}
   else purchase.error=result.message.empty()?errorText(result.error):result.message;
   return true;
  }
 }
 const bool handled=dialogEvent(purchase.dialog,l.dialog,e,screenPoint);
 if(!purchase.dialog.open)purchase={};
 return handled;
}
}
void focusTankShop(TankShopState& state,const ShopLayout& page,TankId id){
 if(id.value<1||id.value>6)return;
 cancelTankShopPress(state);state.motion.stop();state.focusedTank=id;
 const auto l=layoutTankShop(page,state.scroll);const auto card=l.cards[id.value-1];
 state.scroll=l.scroll;
 if(card.x<l.viewport.x)state.scroll-=(l.viewport.x-card.x)/l.step;
 else if(card.x+card.w>l.viewport.x+l.viewport.w)state.scroll+=(card.x+card.w-l.viewport.x-l.viewport.w)/l.step;
 state.scroll=std::clamp(state.scroll,0.f,l.maxScroll);
}
int tankShopControl(const TankShopLayout& l,SDL_FPoint p){
 if(!l.viewport.has(p.x,p.y))return -1;
 for(int i=0;i<6;++i)if(l.cards[i].has(p.x,p.y))return i;
 return -1;
}
bool tankShopEvent(Session& session,TankShopState& state,const ShopLayout& page,const SDL_Event& e,SDL_FPoint p){
 if(state.purchase.dialog.open){state.motion.stop();return tankPurchaseEvent(session,state,page,e,p);}
 if(cancelled(e)||(e.type==SDL_EVENT_KEY_DOWN&&e.key.key==SDLK_ESCAPE)){cancelTankShopPress(state);state.motion.stop();return false;}
 const auto l=layoutTankShop(page,state.scroll);state.scroll=l.scroll;
 const bool reduced=session.domain().state().settings.reducedMotion;
 const bool inside=l.viewport.has(p.x,p.y),overTrack=l.maxScroll>0&&l.scrollHitArea.has(p.x,p.y);
 if(e.type==SDL_EVENT_MOUSE_WHEEL&&(inside||overTrack)){
  cancelTankShopPress(state);state.motion.wheel(state.scroll,horizontalWheel(e)*.25f,l.maxScroll,reduced);return true;
 }
 if(e.type==SDL_EVENT_KEY_DOWN){
  float target=state.motion.destination(state.scroll);
  switch(e.key.key){
   case SDLK_LEFT:target-=1;break;case SDLK_RIGHT:target+=1;break;
   case SDLK_HOME:target=0;break;case SDLK_END:target=l.maxScroll;break;
   default:return false;
  }
  cancelTankShopPress(state);state.motion.stop();state.scroll=std::clamp(target,0.f,l.maxScroll);return true;
 }
 auto drag=[&]{
  if(state.scrollbarDrag){
   const float travel=l.scrollTrack.w-l.scrollThumb.w;
   if(travel>0)state.scroll=std::clamp(state.pressScroll+(p.x-state.pressPoint.x)*l.maxScroll/travel,0.f,l.maxScroll);
  }else{
   const float threshold=std::max(8*page.unit,page.minimumTouch*6/44);
   if(std::hypot(p.x-state.pressPoint.x,p.y-state.pressPoint.y)>threshold)state.dragged=true;
   if(state.dragged)state.motion.drag(state.scroll,state.pressScroll+(state.pressPoint.x-p.x)/l.step,l.maxScroll,scrollEventTime(e));
  }
 };
 if(e.type==SDL_EVENT_MOUSE_MOTION&&state.pointerDown){drag();return true;}
 if(leftButton(e)){
  if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){
   if(!inside&&!overTrack)return false;
   state.pointerDown=true;state.scrollbarDrag=overTrack;state.dragged=overTrack||state.motion.moving();
   state.motion.begin(state.scroll,scrollEventTime(e));
   state.pressed=overTrack?-1:tankShopControl(l,p);state.pressPoint=p;
   if(overTrack&&!l.scrollThumb.has(p.x,l.scrollThumb.y+l.scrollThumb.h*.5f)){
    const float travel=l.scrollTrack.w-l.scrollThumb.w;
    if(travel>0)state.scroll=std::clamp((p.x-l.scrollTrack.x-l.scrollThumb.w*.5f)*l.maxScroll/travel,0.f,l.maxScroll);
   }
   state.pressScroll=state.scroll;return true;
  }
  if(!state.pointerDown)return false;
  drag();const int pressed=state.pressed,control=tankShopControl(l,p);
  const bool activate=!state.dragged&&!state.scrollbarDrag&&control>=0&&control==pressed;
  state.motion.release(scrollEventTime(e),reduced||state.scrollbarDrag);
  cancelTankShopPress(state);
  if(activate){
   const TankId id{control+1};const auto offer=tankOffer(session.domain(),id);
   if(offer.available()){
    state.purchase={};state.purchase.offer=*offer.next;state.purchase.upgrade=session.domain().tank(id)!=nullptr;state.purchase.dialog.open=true;
    state.shortfall.reset();state.notice.clear();
   }
  }
  return true;
 }
 return false;
}
void advanceTankShop(TankShopState& state,const ShopLayout& page,double seconds,bool reducedMotion){
 state.purchase.dialog.motion.advance(state.purchase.dialog.open,seconds,reducedMotion);
 if(state.purchase.dialog.open){state.motion.stop();return;}
 state.motion.advance(state.scroll,seconds,layoutTankShop(page).maxScroll,reducedMotion);
}
void paintTankShop(Canvas& canvas,const Domain& domain,const ShopLayout& page,const TankShopState& state){
 const auto l=layoutTankShop(page,state.scroll);const float u=l.unit;
 auto text=[&](std::string_view value,Rect r,float size,Color color=shopTheme::white){label(canvas,value,r,size*u,color);};
 canvas.clip(l.viewport);
 for(int i=0;i<6;++i){
  const TankId id{i+1};const auto card=l.cards[i];
  if(card.x+card.w<l.viewport.x||card.x>l.viewport.x+l.viewport.w)continue;
  const auto* tank=domain.tank(id);const auto offer=tankOffer(domain,id);
  const bool future=i==5,locked=!tank&&!offer.available(),current=id==domain.state().activeTank;
  shopTheme::card(canvas,card,page.unit,locked);
  if(current||state.focusedTank==id){
   canvas.outline({card.x+2*u,card.y+2*u,card.w-4*u,card.h-4*u},{255,234,123,255},10*u,2*u);
  }
  text("Tank "+std::to_string(i+1),{card.x+4*u,card.y+12*u,card.w-8*u,56*u},28);
  const Rect badge{card.x+card.w-110*u,card.y+24*u,98*u,32*u};
  if(current)shopTheme::panel(canvas,badge,u*.65f,shopTheme::Surface::Amber);
  text(future?"Soon":tank?(current?"Current":"Owned"):(locked?"Locked":"New"),badge,18,current?numberInk:shopTheme::white);
  const std::string capacity=future?"More room to explore":(tank?std::to_string(tank->slots)+(offer.next?" > "+std::to_string(offer.next->slots):""):std::to_string(offer.next?offer.next->slots:0))+" fish";
  text(capacity,{card.x+12*u,card.y+60*u,card.w-24*u,28*u},20,locked?Color{235,229,194,255}:shopTheme::white);
  const Rect note{card.x+12*u,l.actions[i].y-48*u,card.w-24*u,32*u};
  const float artTop=card.y+96*u;
  const Rect art{card.x+24*u,artTop,card.w-48*u,std::max(0.f,note.y-artTop-16*u)};
  tankIllustration(canvas,art,tank!=nullptr,locked?.6f:1.f);
  if(locked&&!future){
   const float size=std::min(76*u,art.h*.6f);
   shopTheme::lockIcon(canvas,{art.x+(art.w-size)*.5f,art.y+(art.h-size)*.5f,size,size});
  }
  if(state.noticeTank==id&&!state.notice.empty())text(state.notice,note,24);
  if(!future&&offer.available()){
   shopTheme::pricePanel(canvas,l.actions[i],u,state.pressed==i&&!state.dragged);
   text(tank?"Upgrade":"Unlock",l.actions[i],28);
  }else{
   const Rect r=l.actions[i];
   shopTheme::pricePanel(canvas,r,u);
   const std::string requirement=offer.next&&domain.level()<offer.next->level?"Level "+std::to_string(offer.next->level):offer.requirement;
   text(future?"Coming soon":offer.next?requirement:"Fully upgraded",r,28);
  }
 }
 canvas.clearClip();
 if(l.maxScroll>0)shopTheme::scrollbar(canvas,l.scrollTrack,l.scrollThumb,u);
}
void paintTankPurchase(Canvas& canvas,const ShopLayout& page,const TankShopState& state,bool reducedMotion){
 const auto& purchase=state.purchase;if(!purchase.dialog.open)return;
 const auto l=layoutTankPurchase(page);const float u=l.dialog.unit;
 const auto& offer=purchase.offer;
 const std::string title=(purchase.upgrade?"Upgrade Tank ":"Unlock Tank ")+std::to_string(offer.tank.value)+"?";
 const DialogPaint animation(canvas,purchase.dialog.motion,l.dialog.frame,reducedMotion);
 paintDialog(canvas,l.dialog,{title},false,purchase.dialog.closePressed,DialogPresentation::ModalContent);
 tankIllustration(canvas,l.illustration,purchase.upgrade);
 const std::string capacity=purchase.upgrade?std::to_string(offer.slots-offer.addedSlots)+" > "+std::to_string(offer.slots)+" fish":"Room for "+std::to_string(offer.slots)+" fish";
 label(canvas,capacity,l.capacity,32*u,shopTheme::ink);
 if(!purchase.error.empty())label(canvas,purchase.error,l.message,24*u,{157,48,38,255});
 for(int currency=0;currency<2;++currency){
  const auto r=currency?l.pearls:l.coins;const auto value=currency?offer.cost.pearls:offer.cost.coins;
  shopTheme::panel(canvas,r,u,currency?shopTheme::Surface::PearlBuy:shopTheme::Surface::Buy,purchase.pressed==currency&&!purchase.dragged);
  label(canvas,currency?"Pearls":"Coins",{r.x,r.y+8*u,r.w,24*u},20*u,shopTheme::white);
  const std::string amount=compact(value);const float size=32*u,textWidth=canvas.textWidth(amount,size,true,false,false,false,true),total=textWidth+44*u;
  const float fit=std::min(1.f,(r.w-24*u)/total),left=r.x+(r.w-total*fit)*.5f,centerY=r.y+32*u+(r.h-40*u)*.5f;
  canvas.text(amount,left,centerY-size*fit*.5f,size*fit,shopTheme::white,false,textWidth*fit,true,true);
  canvas.icon(currency?"hud-icons/pearl-v4.png":"hud-icons/coin-v4.png",{left+(textWidth+8*u)*fit,centerY-18*u*fit,36*u*fit,36*u*fit});
 }
 label(canvas,"or",l.choice,20*u,shopTheme::ink);
}
}
