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
 if(domain.level()<result.next->level)result.requirement="Unlocks at Level "+std::to_string(result.next->level);
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
  const int control=tankSwitcherControl(l,p);
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
   if(control==6){state.open=!state.open;state.notice.clear();state.shopTarget.reset();}
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
  const float inset=12*u,priceGap=32*l.unit,priceWidth=(r.w-2*inset-priceGap)*.5f,priceHeight=56*l.unit;
  l.coins[i]={r.x+inset,r.y+r.h-inset-priceHeight,priceWidth,priceHeight};
  l.pearls[i]={r.x+r.w-inset-priceWidth,l.coins[i].y,priceWidth,priceHeight};
  const float targetHeight=std::max(priceHeight,page.minimumTouch);
  l.coinTargets[i]={l.coins[i].x,l.coins[i].y+l.coins[i].h-targetHeight,priceWidth,targetHeight};
  l.pearlTargets[i]={l.pearls[i].x,l.coinTargets[i].y,priceWidth,targetHeight};
 }
 l.scrollTrack=page.scrollTrack;const float trackWidth=l.scrollTrack.w;
 const float thumbWidth=std::min(trackWidth,std::max(40*u,trackWidth*availableWidth/contentWidth));
 l.scrollThumb={l.scrollTrack.x+(l.maxScroll>0?l.scroll/l.maxScroll:0)*(trackWidth-thumbWidth),l.scrollTrack.y,thumbWidth,l.scrollTrack.h};
 l.scrollHitArea={l.scrollTrack.x,l.scrollTrack.y-8*u,trackWidth,20*u};
 return l;
}
namespace {
void cancelTankShopPress(TankShopState& state){state.pressed=-1;state.pointerDown=state.scrollbarDrag=false;}
}
void focusTankShop(TankShopState& state,const ShopLayout& page,TankId id){
 if(id.value<1||id.value>6)return;
 cancelTankShopPress(state);state.focusedTank=id;
 const auto l=layoutTankShop(page,state.scroll);const auto card=l.cards[id.value-1];
 state.scroll=l.scroll;
 if(card.x<l.viewport.x)state.scroll-=(l.viewport.x-card.x)/l.step;
 else if(card.x+card.w>l.viewport.x+l.viewport.w)state.scroll+=(card.x+card.w-l.viewport.x-l.viewport.w)/l.step;
 state.scroll=std::clamp(state.scroll,0.f,l.maxScroll);
}
int tankShopControl(const TankShopLayout& l,SDL_FPoint p){
 if(!l.viewport.has(p.x,p.y))return -1;
 for(int i=0;i<5;++i){if(l.coinTargets[i].has(p.x,p.y))return i*2;if(l.pearlTargets[i].has(p.x,p.y))return i*2+1;}
 return -1;
}
bool tankShopEvent(Session& session,TankShopState& state,const ShopLayout& page,const SDL_Event& e,SDL_FPoint p){
 if(cancelled(e)||(e.type==SDL_EVENT_KEY_DOWN&&e.key.key==SDLK_ESCAPE)){cancelTankShopPress(state);return false;}
 const auto l=layoutTankShop(page,state.scroll);state.scroll=l.scroll;
 const bool inside=l.viewport.has(p.x,p.y),overTrack=l.maxScroll>0&&l.scrollHitArea.has(p.x,p.y);
 if(e.type==SDL_EVENT_MOUSE_WHEEL&&(inside||overTrack)){
  float delta=e.wheel.x!=0?e.wheel.x:-e.wheel.y;if(e.wheel.direction==SDL_MOUSEWHEEL_FLIPPED)delta=-delta;
  cancelTankShopPress(state);state.scroll=std::clamp(state.scroll+delta*.25f,0.f,l.maxScroll);return true;
 }
 if(e.type==SDL_EVENT_KEY_DOWN){
  float target=state.scroll;
  switch(e.key.key){
   case SDLK_LEFT:target-=1;break;case SDLK_RIGHT:target+=1;break;
   case SDLK_HOME:target=0;break;case SDLK_END:target=l.maxScroll;break;
   default:return false;
  }
  cancelTankShopPress(state);state.scroll=std::clamp(target,0.f,l.maxScroll);return true;
 }
 auto drag=[&]{
  if(state.scrollbarDrag){
   const float travel=l.scrollTrack.w-l.scrollThumb.w;
   if(travel>0)state.scroll=std::clamp(state.pressScroll+(p.x-state.pressPoint.x)*l.maxScroll/travel,0.f,l.maxScroll);
  }else{
   const float threshold=std::max(8*page.unit,page.minimumTouch*6/44);
   if(std::hypot(p.x-state.pressPoint.x,p.y-state.pressPoint.y)>threshold)state.dragged=true;
   if(state.dragged)state.scroll=std::clamp(state.pressScroll+(state.pressPoint.x-p.x)/l.step,0.f,l.maxScroll);
  }
 };
 if(e.type==SDL_EVENT_MOUSE_MOTION&&state.pointerDown){drag();return true;}
 if(leftButton(e)){
  if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){
   if(!inside&&!overTrack)return false;
   state.pointerDown=true;state.scrollbarDrag=overTrack;state.dragged=overTrack;
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
  cancelTankShopPress(state);
  if(activate){
   const TankId id{control/2+1};const auto offer=tankOffer(session.domain(),id);
   if(offer.available()){
    const bool pearl=control%2;const bool owned=session.domain().tank(id)!=nullptr;
    const auto result=session.command({.action=owned?Action::ExpandTank:Action::UnlockTank,.tank=id,.currency=pearl?Currency::Pearls:Currency::Coins});
    state.noticeTank=id;
    state.shortfall=result.error==Error::Funds?std::optional{result.shortfall}:std::nullopt;
    state.notice=result?(owned?"Tank upgraded!":"Tank unlocked!"):result.error==Error::Funds?"":(result.message.empty()?errorText(result.error):result.message);
   }
  }
  return true;
 }
 return false;
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
  text(future?"Soon":tank?(current?"Current":"Owned"):(locked?"Locked":"New"),{card.x+card.w-108*u,card.y+24*u,96*u,32*u},18);
  const std::string capacity=future?"More room to explore":(tank?std::to_string(tank->slots)+(offer.next?" > "+std::to_string(offer.next->slots):""):std::to_string(offer.next?offer.next->slots:0))+" growing slots";
  text(capacity,{card.x+12*u,card.y+60*u,card.w-24*u,28*u},20,locked?Color{235,229,194,255}:shopTheme::white);
  const Rect note{card.x+12*u,l.coins[i].y-48*u,card.w-24*u,32*u};
  const float artTop=card.y+96*u;
  const Rect art{card.x+24*u,artTop,card.w-48*u,std::max(0.f,note.y-artTop-16*u)};
  const Rect source=tank?Rect{48,87,1678,731}:Rect{47,86,1680,732};
  const float scale=std::min(art.w/source.w,art.h/source.h);
  canvas.image(tank?"tank-grid/active.png":"tank-grid/empty.png",{art.x+(art.w-source.w*scale)*.5f-source.x*scale,art.y+(art.h-source.h*scale)*.5f-source.y*scale,1774*scale,887*scale},0,{.5f,.5f},locked?.6f:1.f);
  if(locked&&!future){
   const float size=std::min(76*u,art.h*.6f);
   shopTheme::lockIcon(canvas,{art.x+(art.w-size)*.5f,art.y+(art.h-size)*.5f,size,size});
  }
  if(state.noticeTank==id&&!state.notice.empty())text(state.notice,note,24);
  else if(offer.available())text(tank?"Upgrade":"Unlock",note,24);
  if(!future&&offer.available()){
   for(int currency=0;currency<2;++currency){
    const auto r=currency?l.pearls[i]:l.coins[i];const auto value=currency?offer.next->cost.pearls:offer.next->cost.coins;
    shopTheme::pricePanel(canvas,r,u,state.pressed==i*2+currency&&!state.dragged);
    const std::string amount=compact(value);const float size=28*u,textWidth=canvas.textWidth(amount,size,true,false,false,false,true),total=textWidth+40*u;
    const float fit=std::min(1.f,(r.w-16*u)/total),left=r.x+(r.w-total*fit)*.5f;
    canvas.text(amount,left,r.y+(r.h-size*fit)*.5f,size*fit,shopTheme::white,false,textWidth*fit,true,true);
    canvas.icon(currency?"hud-icons/pearl-v4.png":"hud-icons/coin-v4.png",{left+(textWidth+8*u)*fit,r.y+(r.h-32*u*fit)*.5f,32*u*fit,32*u*fit});
   }
   text("or",{l.coins[i].x+l.coins[i].w,l.coins[i].y,32*u,l.coins[i].h},20);
  }else{
   const Rect r{l.coins[i].x,l.coins[i].y,l.pearls[i].x+l.pearls[i].w-l.coins[i].x,l.coins[i].h};
   shopTheme::pricePanel(canvas,r,u);
   const std::string requirement=offer.next&&domain.level()<offer.next->level?"Level "+std::to_string(offer.next->level):offer.requirement;
   text(future?"Coming soon":offer.next?requirement:"Fully upgraded",r,28);
  }
 }
 canvas.clearClip();
 if(l.maxScroll>0)shopTheme::scrollbar(canvas,l.scrollTrack,l.scrollThumb,u);
}
}
