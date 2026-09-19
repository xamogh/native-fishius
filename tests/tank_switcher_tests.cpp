#include "aquarium/hud_tanks.hpp"
#include "aquarium/hud_care.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
SDL_Event pointerEvent(Uint32 type){SDL_Event e{};e.type=type;e.common.timestamp=1000000000;e.button.button=SDL_BUTTON_LEFT;return e;}
}
int main(int argc,char** argv){try{
 using namespace aq;
 const auto assets=std::filesystem::path(argc>1?argv[1]:"assets");std::ifstream in(assets/"content.json");const auto content=Content::fromJson(Json::parse(in));
 Session session(content,"/tmp/fishius-switcher-review-unused.json",0,true);auto& domain=session.domain();
 TankSwitcherState switcher;const auto hud=layoutHud(1608,940,{},44);const auto page=layoutShop(1608,940,{});
 auto swEvent=[&](Uint32 type,SDL_FPoint p){return tankSwitcherEvent(session,switcher,hud,pointerEvent(type),p);};
 auto swClick=[&](Rect r){swEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,center(r));swEvent(SDL_EVENT_MOUSE_BUTTON_UP,center(r));};
 const auto initialState=domain.state();const auto initial=encode(initialState);const auto starter=layoutTankSwitcher(domain,hud);
 check(starter.count==6&&starter.ownedCount==1,"Switcher must show all six tanks and count only owned tanks");
 for(int i=0;i<6;++i)check(starter.ids[i].value==i+1,"Tank order changes with ownership");
 swClick(hud[HudPart::Tank]);check(switcher.open&&encode(domain.state())==initial,"Tank button failed to open without changing the save");
 swClick(hud[HudPart::Tank]);check(!switcher.open,"Tank button did not collapse switcher");
 for(int i=1;i<6;++i){
  swClick(hud[HudPart::Tank]);swClick(starter.tanks[i]);
  check(!switcher.open&&switcher.shopTarget==TankId{i+1}&&encode(domain.state())==initial,"Locked tank must request its Shop card without buying or switching");
 }
 swClick(hud[HudPart::Tank]);check(!switcher.shopTarget,"Reopening retained a previous Shop request");
 swEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,center(starter.tanks[1]));swEvent(SDL_EVENT_MOUSE_MOTION,center(starter.tanks[2]));swEvent(SDL_EVENT_MOUSE_BUTTON_UP,center(starter.tanks[1]));
 check(switcher.open&&!switcher.shopTarget,"Dragging a locked tank opened Shop");
 swEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,center(starter.tanks[1]));swEvent(SDL_EVENT_MOUSE_BUTTON_UP,center(starter.tanks[2]));
 check(switcher.open&&!switcher.shopTarget,"Release on another locked tank opened Shop");
 swClick(starter.tanks[0]);check(!switcher.open&&!switcher.shopTarget&&encode(domain.state())==initial,"Current tank selection must only collapse the switcher");
 auto ready=domain.state();ready.wallet={1000000,1000};ready.xp=content.levels.back();ready.tanks={{{1},20},{{2},10},{{3},10}};domain.install(ready);
 auto layout=layoutTankSwitcher(domain,hud);swClick(hud[HudPart::Tank]);
 check(layout.count==6&&layout.ownedCount==3,"Owned count did not update");
 for(int i=0;i<6;++i)check(layout.tanks[i].x==starter.tanks[i].x&&layout.tanks[i].y==starter.tanks[i].y,"Purchasing a tank moved the switcher positions");
 swEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,center(layout.tanks[1]));swEvent(SDL_EVENT_MOUSE_MOTION,center(layout.tanks[2]));swEvent(SDL_EVENT_MOUSE_BUTTON_UP,center(layout.tanks[1]));
 check(domain.state().activeTank.value==1&&switcher.open,"Dragging away and back switched tanks");
 swEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,center(layout.tanks[1]));SDL_Event cancel{};cancel.type=SDL_EVENT_WINDOW_FOCUS_LOST;tankSwitcherEvent(session,switcher,hud,cancel,{});swEvent(SDL_EVENT_MOUSE_BUTTON_UP,center(layout.tanks[1]));
 check(domain.state().activeTank.value==1&&!switcher.open,"Focus loss retained a switch gesture");
 swClick(hud[HudPart::Tank]);swClick(layout.tanks[1]);check(domain.state().activeTank.value==2&&!switcher.open&&!switcher.shopTarget&&domain.state().wallet.coins==ready.wallet.coins&&domain.state().wallet.pearls==ready.wallet.pearls,"Owned tank switch failed, opened Shop or charged currency");
 swClick(hud[HudPart::Tank]);swEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,{700,400});check(switcher.open,"Backdrop closes before release");swEvent(SDL_EVENT_MOUSE_BUTTON_UP,{700,400});check(!switcher.open,"Backdrop failed to close");
 swClick(hud[HudPart::Tank]);SDL_Event key{};key.type=SDL_EVENT_KEY_DOWN;key.key.key=SDLK_6;const auto beforeShop=encode(domain.state());
 key.key.repeat=true;tankSwitcherEvent(session,switcher,hud,key,{});check(switcher.open&&!switcher.shopTarget,"Repeated key opened Shop");
 key.key.repeat=false;tankSwitcherEvent(session,switcher,hud,key,{});check(!switcher.open&&switcher.shopTarget==TankId{6}&&encode(domain.state())==beforeShop,"Coming soon shortcut must lead to its Shop card without changing the save");
 swClick(hud[HudPart::Tank]);key.key.key=SDLK_3;tankSwitcherEvent(session,switcher,hud,key,{});check(!switcher.open&&!switcher.shopTarget&&domain.state().activeTank.value==3,"Owned keyboard shortcut failed");
 swClick(hud[HudPart::Tank]);key.key.key=SDLK_ESCAPE;tankSwitcherEvent(session,switcher,hud,key,{});check(!switcher.open,"Escape failed to collapse switcher");
 swClick(hud[HudPart::Tank]);check(!swEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,center(hud[HudPart::Shop]))&&!switcher.open,"Switcher blocked another HUD button");
 // A normalized touch uses the same release and cancellation rules.
 HudPointer touch;
 auto finger=[&](Uint32 type,Rect target,SDL_FingerID id=7){
  const auto p=center(target);SDL_Event e{};e.type=type;e.tfinger.fingerID=id;e.tfinger.x=p.x/1608;e.tfinger.y=p.y/940;
  if(normalizeHudPointer(touch,e,1608,940))tankSwitcherEvent(session,switcher,hud,e,{e.button.x,e.button.y});
 };
 finger(SDL_EVENT_FINGER_DOWN,hud[HudPart::Tank]);finger(SDL_EVENT_FINGER_UP,hud[HudPart::Tank]);check(switcher.open,"Touch failed to open switcher");
 finger(SDL_EVENT_FINGER_DOWN,layout.tanks[0]);finger(SDL_EVENT_FINGER_DOWN,layout.tanks[1],8);finger(SDL_EVENT_FINGER_UP,layout.tanks[1],8);finger(SDL_EVENT_FINGER_UP,layout.tanks[0]);
 check(domain.state().activeTank.value==1&&!switcher.open,"Second finger redirected the active tank selection");
 finger(SDL_EVENT_FINGER_DOWN,hud[HudPart::Tank]);finger(SDL_EVENT_FINGER_UP,hud[HudPart::Tank]);
 finger(SDL_EVENT_FINGER_DOWN,layout.tanks[4]);finger(SDL_EVENT_FINGER_UP,layout.tanks[4]);
 check(!switcher.open&&switcher.shopTarget==TankId{5}&&domain.state().activeTank.value==1,"Locked touch selection failed to request the matching Shop card");
 swClick(hud[HudPart::Tank]);swEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,center(layout.tanks[4]));tankSwitcherEvent(session,switcher,hud,cancel,{});swEvent(SDL_EVENT_MOUSE_BUTTON_UP,center(layout.tanks[4]));
 check(!switcher.open&&!switcher.shopTarget,"Focus loss retained a locked tank request");

 TankShopState shop;const auto grid=layoutTankShop(page);const auto confirmation=layoutTankPurchase(page);
 auto shopEvent=[&](Uint32 type,Rect r){return tankShopEvent(session,shop,page,pointerEvent(type),center(r));};
 auto click=[&](Rect r){shopEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,r);shopEvent(SDL_EVENT_MOUSE_BUTTON_UP,r);};
 auto buy=[&](int tank,bool pearls){
  const auto before=encode(domain.state());click(grid.cards[tank-1]);
  check(encode(domain.state())==before,"Opening a tank card changed progress");
  if(shop.purchase.dialog.open)click(pearls?confirmation.pearls:confirmation.coins);
 };
 domain.install(initialState);auto poor=domain.state();poor.wallet={0,0};domain.install(poor);
 check(tankOffer(domain,{1}).available()&&tankOffer(domain,{1}).requirement.empty(),"Owned starter tank has a level lock");
 check(!tankOffer(domain,{2}).available()&&tankOffer(domain,{2}).requirement=="Unlocks at Level 7","New tank lost its level lock");
 buy(1,false);check(shop.shortfall&&shop.shortfall->coins==domain.nextTankEntitlement({1})->cost.coins&&!shop.shortfall->pearls&&shop.notice.empty()&&domain.tank({1})->slots==10&&!shop.purchase.dialog.open,"Insufficient funds upgraded a tank or lost its dialog shortfall");
 ready=domain.state();ready.wallet={1000000,1000};domain.install(ready);const auto cost=domain.nextTankEntitlement({1})->cost;
 click(grid.cards[0]);check(shop.purchase.dialog.open&&shop.purchase.offer.cost.coins==cost.coins&&shop.purchase.offer.slots==15&&shop.purchase.upgrade&&encode(domain.state())==encode(ready),"Card did not open the next upgrade without charging");
 shopEvent(SDL_EVENT_MOUSE_BUTTON_UP,confirmation.coins);check(domain.tank({1})->slots==10,"Unmatched release confirmed a purchase");
 shopEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,confirmation.coins);shopEvent(SDL_EVENT_MOUSE_BUTTON_UP,confirmation.pearls);check(domain.tank({1})->slots==10,"Release on another currency purchased an upgrade");
 shopEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,confirmation.coins);shopEvent(SDL_EVENT_MOUSE_MOTION,confirmation.pearls);shopEvent(SDL_EVENT_MOUSE_BUTTON_UP,confirmation.coins);check(domain.tank({1})->slots==10,"Dialog drag purchased an upgrade");
 for(const auto type:{SDL_EVENT_WINDOW_FOCUS_LOST,SDL_EVENT_WILL_ENTER_BACKGROUND,SDL_EVENT_RENDER_DEVICE_RESET,SDL_EVENT_RENDER_TARGETS_RESET}){
  shopEvent(SDL_EVENT_MOUSE_BUTTON_DOWN,confirmation.coins);SDL_Event interrupted{};interrupted.type=type;tankShopEvent(session,shop,page,interrupted,{});shopEvent(SDL_EVENT_MOUSE_BUTTON_UP,confirmation.coins);
  check(domain.tank({1})->slots==10&&shop.purchase.dialog.open,"Interrupted dialog press spent currency");
 }
 SDL_Event confirmationKey{};confirmationKey.type=SDL_EVENT_KEY_DOWN;confirmationKey.key.key=SDLK_END;tankShopEvent(session,shop,page,confirmationKey,{});
 check(shop.scroll==0&&shop.purchase.dialog.open,"Dialog allowed keyboard input to reach the cards");
 confirmationKey.key.key=SDLK_RETURN;tankShopEvent(session,shop,page,confirmationKey,{});check(domain.tank({1})->slots==10,"Return chose a currency without a payment click");
 SDL_Event confirmationWheel{};confirmationWheel.type=SDL_EVENT_MOUSE_WHEEL;confirmationWheel.wheel.y=-100;tankShopEvent(session,shop,page,confirmationWheel,center(grid.viewport));check(shop.scroll==0,"Dialog allowed the cards to scroll");
 click(confirmation.dialog.close);check(!shop.purchase.dialog.open&&encode(domain.state())==encode(ready),"Close spent currency or left the dialog open");
 click(grid.actions[0]);check(shop.purchase.dialog.open,"Card footer did not open confirmation");
 confirmationKey.key.key=SDLK_ESCAPE;check(tankShopEvent(session,shop,page,confirmationKey,{})&&!shop.purchase.dialog.open&&encode(domain.state())==encode(ready),"Escape did not dismiss only the confirmation");
 click(grid.cards[0]);click({0,0,1,1});check(!shop.purchase.dialog.open&&encode(domain.state())==encode(ready),"Backdrop dismissal spent currency");
 buy(1,false);check(domain.tank({1})->slots==15&&domain.state().wallet.coins==ready.wallet.coins-cost.coins&&domain.state().wallet.pearls==ready.wallet.pearls&&!shop.purchase.dialog.open,"Starter coin upgrade failed");
 shopEvent(SDL_EVENT_MOUSE_BUTTON_UP,confirmation.coins);check(domain.tank({1})->slots==15,"Repeated release bought the next upgrade");
 buy(1,true);check(domain.tank({1})->slots==20&&domain.level()==1,"Owned tank upgrade still requires a level");
 const auto upgraded=encode(domain.state());buy(2,true);buy(2,false);check(encode(domain.state())==upgraded&&!domain.tank({2})&&!shop.purchase.dialog.open,"New tank bypassed level gate");
 ready=domain.state();ready.xp=content.levels.back();domain.install(ready);buy(3,true);check(!domain.tank({3})&&!shop.purchase.dialog.open,"Tank unlock bypassed prerequisite");
 const auto price=domain.nextTankEntitlement({2})->cost;buy(2,true);check(domain.tank({2})&&domain.state().wallet.coins==ready.wallet.coins&&domain.state().wallet.pearls==ready.wallet.pearls-price.pearls,"Pearl unlock charged wrong currency");
 for(int capacity:{25,30,35,40}){buy(1,true);check(domain.tank({1})->slots==capacity,"Larger tank upgrade is missing");}
 const auto maxed=encode(domain.state());buy(1,false);buy(1,true);check(encode(domain.state())==maxed&&!tankOffer(domain,{1}).next&&!shop.purchase.dialog.open,"Fully upgraded tank charged currency");
 buy(6,false);buy(6,true);check(encode(domain.state())==maxed&&!domain.tank({6})&&!shop.purchase.dialog.open,"Coming soon tank is purchasable");
 const auto all=shopSubtabBounds(page,ShopCategory::Tanks,0);
 check(all.y==page.subtabs[0].y&&all.w==page.subtabs[0].w&&all.h==page.subtabs[0].h&&center(all).x==center(page.title).x,"Tank All tab differs from the shared filter row");
 check(shopControl(page,center(all),ShopCategory::Tanks)==4,"Tank All tab is not clickable");
 check(shopControl(page,{page.subtabs[1].x+page.subtabs[1].w-4,center(all).y},ShopCategory::Tanks)==-1,"Hidden tank filter receives clicks");
 ShopState allTanks{ShopCategory::Tanks,0,2};activateShopControl(allTanks,4);
 check(allTanks.category==ShopCategory::Tanks&&allTanks.subtab==0&&allTanks.scroll==0,"All tab does not reset tank browsing");
 ShopState category;activateShopControl(category,shopControl(page,center(page.tabs[5])));check(category.category==ShopCategory::Tanks&&shopItems(domain,category).empty(),"Tanks tab did not navigate to tank inventory");
 // Scrolling must reveal the remaining tanks without turning a drag into a purchase.
 {
  Session scrolling(content,"/tmp/fishius-tank-scroll-review-unused.json",0,true);auto& inventory=scrolling.domain();
  auto rich=inventory.state();rich.wallet={1000000,1000};rich.xp=content.levels.back();rich.tanks={{{1},20},{{2},20},{{3},20},{{4},20}};inventory.install(rich);
  TankShopState strip;const auto start=layoutTankShop(page);const auto untouched=encode(inventory.state());
  const float right=start.viewport.x+start.viewport.w;
  check(start.cards[2].x+start.cards[2].w<right&&start.cards[3].x<right&&start.cards[3].x+start.cards[3].w>right,"Desktop must show three full cards and part of the next card");
  auto event=[&](Uint32 type,SDL_FPoint point){return tankShopEvent(scrolling,strip,page,pointerEvent(type),point);};
  event(SDL_EVENT_MOUSE_BUTTON_DOWN,center(start.actions[4]));event(SDL_EVENT_MOUSE_BUTTON_UP,center(start.actions[4]));
  check(encode(inventory.state())==untouched&&tankShopControl(start,center(start.actions[4]))==-1,"Off-screen tank can be purchased");
  const auto from=center(start.actions[1]);const SDL_FPoint to{from.x-start.step*2,from.y};
  event(SDL_EVENT_MOUSE_BUTTON_DOWN,from);event(SDL_EVENT_MOUSE_MOTION,to);event(SDL_EVENT_MOUSE_BUTTON_UP,to);
  check(strip.scroll>1.9f&&encode(inventory.state())==untouched&&!strip.pointerDown&&!strip.purchase.dialog.open,"Dragging a card failed to scroll or opened confirmation");
  const auto moved=layoutTankShop(page,strip.scroll);
  check(tankShopControl(moved,center(moved.actions[0]))==-1,"Clipped card still has an active hit target");
  SDL_Event wheel{};wheel.type=SDL_EVENT_MOUSE_WHEEL;wheel.wheel.y=-100;
  auto settle=[&]{for(int i=0;i<60;++i)advanceTankShop(strip,page,1./60,false);};
  const auto beforeWheel=strip.scroll;tankShopEvent(scrolling,strip,page,wheel,center(start.viewport));
  check(strip.scroll==beforeWheel,"Wheel jumps to its destination without easing");settle();check(strip.scroll==start.maxScroll,"Wheel cannot reach the last tank");
  wheel.wheel.direction=SDL_MOUSEWHEEL_FLIPPED;tankShopEvent(scrolling,strip,page,wheel,center(start.viewport));settle();check(strip.scroll==0,"Reversed wheel failed to return to the start");
  wheel.wheel.direction=SDL_MOUSEWHEEL_NORMAL;wheel.wheel.y=-4;
  check(!tankShopEvent(scrolling,strip,page,wheel,center(page.close))&&strip.scroll==0,"Wheel outside the card row scrolls tanks");
  event(SDL_EVENT_MOUSE_BUTTON_DOWN,center(start.actions[1]));tankShopEvent(scrolling,strip,page,wheel,center(start.viewport));event(SDL_EVENT_MOUSE_BUTTON_UP,center(start.actions[1]));settle();
  check(strip.scroll>0&&encode(inventory.state())==untouched,"Wheeling during a press purchased a tank");
  focusTankShop(strip,page,{6});const auto end=layoutTankShop(page,strip.scroll);
  check(end.cards[5].x>=end.viewport.x&&end.cards[5].x+end.cards[5].w<=end.viewport.x+end.viewport.w,"Switcher focus did not reveal Tank 6");
  event(SDL_EVENT_MOUSE_BUTTON_DOWN,center(end.actions[5]));event(SDL_EVENT_MOUSE_BUTTON_UP,center(end.actions[5]));
  check(encode(inventory.state())==untouched&&!inventory.tank({6}),"Visible Coming soon card can be purchased");
  const auto tankFiveCost=inventory.nextTankEntitlement({5})->cost;
  event(SDL_EVENT_MOUSE_BUTTON_DOWN,center(end.actions[4]));event(SDL_EVENT_MOUSE_BUTTON_UP,center(end.actions[4]));
  check(strip.purchase.dialog.open&&strip.purchase.offer.tank==TankId{5}&&!strip.purchase.upgrade&&encode(inventory.state())==untouched,"Scrolled card did not confirm the matching tank");
  event(SDL_EVENT_MOUSE_BUTTON_DOWN,center(confirmation.coins));event(SDL_EVENT_MOUSE_BUTTON_UP,center(confirmation.coins));
  check(inventory.tank({5})&&inventory.state().wallet.coins==rich.wallet.coins-tankFiveCost.coins&&inventory.state().wallet.pearls==rich.wallet.pearls,"Confirmed unlock did not buy the matching tank with coins");
  SDL_Event navigation{};navigation.type=SDL_EVENT_KEY_DOWN;navigation.key.key=SDLK_HOME;
  tankShopEvent(scrolling,strip,page,navigation,{});check(strip.scroll==0,"Home failed to scroll to the first tank");
  event(SDL_EVENT_MOUSE_BUTTON_DOWN,center(start.scrollThumb));
  const SDL_FPoint trackEnd{start.scrollTrack.x+start.scrollTrack.w,start.scrollTrack.y+start.scrollTrack.h*.5f};
  event(SDL_EVENT_MOUSE_MOTION,trackEnd);event(SDL_EVENT_MOUSE_BUTTON_UP,trackEnd);
  check(strip.scroll==start.maxScroll&&!strip.pointerDown,"Scrollbar drag failed to reach the end");
  navigation.key.key=SDLK_HOME;tankShopEvent(scrolling,strip,page,navigation,{});
  event(SDL_EVENT_MOUSE_BUTTON_DOWN,from);tankShopEvent(scrolling,strip,page,cancel,{});event(SDL_EVENT_MOUSE_MOTION,to);event(SDL_EVENT_MOUSE_BUTTON_UP,to);
  check(strip.scroll==0&&!strip.pointerDown,"Cancelled drag continued to scroll");
  const auto beforeFlick=encode(inventory.state());
  auto timed=[&](Uint32 type,SDL_FPoint point,Uint64 time){auto e=pointerEvent(type);e.common.timestamp=time;tankShopEvent(scrolling,strip,page,e,point);};
  timed(SDL_EVENT_MOUSE_BUTTON_DOWN,from,1000000000);
  const SDL_FPoint flickEnd{from.x-start.step*.65f,from.y};
  timed(SDL_EVENT_MOUSE_MOTION,flickEnd,1100000000);timed(SDL_EVENT_MOUSE_BUTTON_UP,flickEnd,1105000000);
  const float released=strip.scroll;advanceTankShop(strip,page,.1,false);
  check(strip.scroll>released+.1f&&!strip.purchase.dialog.open,"Tank swipe stops at release instead of gliding");
  const auto movingCard=center(layoutTankShop(page,strip.scroll).cards[2]);
  timed(SDL_EVENT_MOUSE_BUTTON_DOWN,movingCard,1200000000);timed(SDL_EVENT_MOUSE_BUTTON_UP,movingCard,1210000000);
  const float stopped=strip.scroll;advanceTankShop(strip,page,.1,false);
  check(strip.scroll==stopped&&!strip.purchase.dialog.open&&encode(inventory.state())==beforeFlick,"Stopping a moving tank row opens a purchase or keeps gliding");
  navigation.key.key=SDLK_HOME;tankShopEvent(scrolling,strip,page,navigation,{});
  // Normalized touch owns the full swipe, even when a second finger arrives.
  HudPointer swipe;
  auto fingerEvent=[&](Uint32 type,SDL_FPoint point,SDL_FingerID id=21){
   SDL_Event e{};e.type=type;e.common.timestamp=2000000000;e.tfinger.fingerID=id;e.tfinger.x=point.x/1608;e.tfinger.y=point.y/940;
   if(normalizeHudPointer(swipe,e,1608,940)){
    const SDL_FPoint p=e.type==SDL_EVENT_MOUSE_MOTION?SDL_FPoint{e.motion.x,e.motion.y}:SDL_FPoint{e.button.x,e.button.y};
    tankShopEvent(scrolling,strip,page,e,p);
   }
  };
  const auto beforeSwipe=encode(inventory.state());
  fingerEvent(SDL_EVENT_FINGER_DOWN,from);fingerEvent(SDL_EVENT_FINGER_DOWN,center(start.actions[2]),22);
  fingerEvent(SDL_EVENT_FINGER_MOTION,to);fingerEvent(SDL_EVENT_FINGER_UP,center(start.actions[2]),22);fingerEvent(SDL_EVENT_FINGER_UP,to);
  check(strip.scroll>1.9f&&!strip.pointerDown&&!strip.purchase.dialog.open&&encode(inventory.state())==beforeSwipe,"Touch swipe opened confirmation or was redirected by another finger");
  navigation.key.key=SDLK_HOME;tankShopEvent(scrolling,strip,page,navigation,{});
  fingerEvent(SDL_EVENT_FINGER_DOWN,center(start.cards[0]));fingerEvent(SDL_EVENT_FINGER_UP,center(start.cards[0]));
  check(strip.purchase.dialog.open&&encode(inventory.state())==beforeSwipe,"Touch card did not open a free confirmation");
  const auto pearlCost=strip.purchase.offer.cost.pearls;const auto wallet=inventory.state().wallet;
  fingerEvent(SDL_EVENT_FINGER_DOWN,center(confirmation.pearls));fingerEvent(SDL_EVENT_FINGER_UP,center(confirmation.pearls));
  check(!strip.purchase.dialog.open&&inventory.tank({1})->slots==25&&inventory.state().wallet.pearls==wallet.pearls-pearlCost&&inventory.state().wallet.coins==wallet.coins,"Touch confirmation did not charge only pearls");
  navigation.key.key=SDLK_END;tankShopEvent(scrolling,strip,page,navigation,{});check(strip.scroll==start.maxScroll,"End failed to reveal the last tank");
 }
 for(const auto size:std::array{SDL_FPoint{1641,959},SDL_FPoint{667,375},SDL_FPoint{1024,768},SDL_FPoint{390,844},SDL_FPoint{320,240}}){
  const float width=std::max(1608.f,830.f*size.x/size.y),height=width*size.y/size.x;
  const auto h=layoutHud(width,height,{},44*width/size.x);const auto l=layoutTankSwitcher(domain,h);const auto p=layoutShop(width,height,{},44*width/size.x);const auto g=layoutTankShop(p);
  check(l.header.y>h[HudPart::Xp].y+h[HudPart::Xp].h&&l.header.x>=0,"Switcher overlaps the top HUD or crosses the viewport");
  for(int i=0;i<l.count;++i)check(l.tanks[i].y+l.tanks[i].h<h[HudPart::Tank].y&&tankSwitcherControl(l,center(l.tanks[i]))==i,"Porthole hit target or anchor is wrong");
  for(int i=3;i<l.count;++i)check(l.tanks[i-3].y+l.tanks[i-3].h<l.tanks[i].y,"Porthole rows share a hit target");
  for(int i=0;i<6;++i)check(g.cards[i].h>0&&g.cards[i].y+g.cards[i].h<p.footer.y&&g.actions[i].y>g.cards[i].y+g.cards[i].h*.6f,"Tank Shop card crosses its content area");
  for(const auto otherCategory:{ShopCategory::Fish,ShopCategory::Plants,ShopCategory::Decorations,ShopCategory::Treasure}){
   const auto other=shopCardBounds(p,ShopState{otherCategory},0);
   check(g.cards[0].y==other.y&&g.cards[0].h==other.h,"Tank card height or vertical position differs from the other shop tabs");
  }
  check(g.cards[0].x==p.cards[0].x&&std::abs(g.cards[1].x-g.cards[0].x-g.cards[0].w-(p.cards[1].x-p.cards[0].x-p.cards[0].w))<.01f,"Tank cards do not use the shared left inset and gap");
  check(g.viewport.x==p.cardViewport.x&&g.viewport.w==p.cardViewport.w&&g.viewport.x>p.body.x&&g.viewport.x+g.viewport.w<p.body.x+p.body.w,"Scrolling tank cards can cover the shop frame");
  check(g.actions[0].h==56*g.unit&&g.actions[0].y+g.actions[0].h==g.cards[0].y+g.cards[0].h-12*p.unit,"Tank action row differs from the shared size and bottom inset");
  check(tankShopControl(g,center(g.cards[0]))==0&&tankShopControl(g,center(g.actions[0]))==0,"Whole card is not clickable");
  const auto purchase=layoutTankPurchase(p);
  auto inside=[](Rect a,Rect b){return a.x>=b.x-.01f&&a.y>=b.y-.01f&&a.x+a.w<=b.x+b.w+.01f&&a.y+a.h<=b.y+b.h+.01f;};
  for(const auto r:{purchase.coins,purchase.pearls,purchase.dialog.close}){
   check(r.h*size.x/width>=43.99f&&r.w*size.x/width>=43.99f,"Confirmation button is below the touch target size");
   check(inside(r,purchase.dialog.frame),"Confirmation button crosses the frame");
  }
  check(inside(purchase.illustration,purchase.dialog.content)&&purchase.illustration.h>0&&inside(purchase.capacity,purchase.dialog.content)&&inside(purchase.message,purchase.dialog.content),"Confirmation content crosses the frame");
  check(purchase.illustration.y+purchase.illustration.h<=purchase.capacity.y&&purchase.capacity.y+purchase.capacity.h<=purchase.message.y&&purchase.message.y+purchase.message.h<=purchase.coins.y&&purchase.coins.x+purchase.coins.w<purchase.pearls.x,"Confirmation content overlaps");
  check(inside(purchase.dialog.frame,p.page),"Confirmation crosses the screen");
  check(g.scrollTrack.x==p.scrollTrack.x&&g.scrollTrack.y==p.scrollTrack.y&&g.scrollTrack.w==p.scrollTrack.w&&g.scrollTrack.h==p.scrollTrack.h,"Tank scrollbar does not match the shared bottom track");
  check(g.scrollHitArea.y+g.scrollHitArea.h<=p.footer.y,"Tank scrollbar crosses the wallet footer");
  check(g.scrollHitArea.y>=g.actions[0].y+g.actions[0].h,"Tank scrollbar overlaps card actions");
  for(int i=0;i<6;++i){
   TankShopState focused;focusTankShop(focused,p,{i+1});const auto f=layoutTankShop(p,focused.scroll);
   check(f.cards[i].x>=f.viewport.x-.01f&&f.cards[i].x+f.cards[i].w<=f.viewport.x+f.viewport.w+.01f,"Focused tank is clipped at this viewport size");
  }
  for(int i=0;i<6;++i)check(p.tabs[i].x+p.tabs[i].w<p.close.x,"Shop tab overlaps close button");
 }
 const auto temporary=std::filesystem::temp_directory_path()/("fishius-switcher-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(temporary);
 {
  Session saved(content,temporary/"save.json",0);saved.domain().fixture("performance");const auto durable=saved.domain().state();TankSwitcherState state;state.open=true;
  const auto l=layoutTankSwitcher(saved.domain(),hud);const auto p=center(l.tanks[1]);tankSwitcherEvent(saved,state,hud,pointerEvent(SDL_EVENT_MOUSE_BUTTON_DOWN),p);tankSwitcherEvent(saved,state,hud,pointerEvent(SDL_EVENT_MOUSE_BUTTON_UP),p);
  Session restored(content,temporary/"save.json",0);check(restored.domain().state().activeTank.value==2&&restored.domain().state().tanks.size()==2,"Tank switch did not survive save/reload");
  std::ofstream(temporary/"blocked")<<"not a directory";Session blocked(content,temporary/"blocked"/"save.json",0);blocked.domain().install(durable);const auto before=encode(blocked.domain().state());state={};state.open=true;
  tankSwitcherEvent(blocked,state,hud,pointerEvent(SDL_EVENT_MOUSE_BUTTON_DOWN),p);tankSwitcherEvent(blocked,state,hud,pointerEvent(SDL_EVENT_MOUSE_BUTTON_UP),p);
  check(encode(blocked.domain().state())==before&&state.open&&!state.notice.empty(),"Save failure changed active tank or hid feedback");
  auto fundedContent=content;fundedContent.startingWallet={1000000,1000};
  auto purchaseClick=[&](Session& target,TankShopState& store,Rect bounds){
   tankShopEvent(target,store,page,pointerEvent(SDL_EVENT_MOUSE_BUTTON_DOWN),center(bounds));
   tankShopEvent(target,store,page,pointerEvent(SDL_EVENT_MOUSE_BUTTON_UP),center(bounds));
  };
  Session purchaseSaved(fundedContent,temporary/"purchase.json",0);const auto funded=purchaseSaved.domain().state();TankShopState store;
  purchaseClick(purchaseSaved,store,grid.cards[0]);purchaseClick(purchaseSaved,store,confirmation.coins);
  Session purchaseRestored(fundedContent,temporary/"purchase.json",0);
  check(!purchaseRestored.notification(),purchaseRestored.status().c_str());
  check(purchaseRestored.domain().tank({1})->slots==15&&purchaseRestored.domain().state().wallet.coins==funded.wallet.coins-cost.coins,"Confirmed tank purchase did not survive save/reload");
  blocked.domain().install(funded);store={};purchaseClick(blocked,store,grid.cards[0]);
  for(const auto button:{confirmation.coins,confirmation.pearls}){
   purchaseClick(blocked,store,button);
   check(encode(blocked.domain().state())==encode(funded)&&store.purchase.dialog.open&&!store.purchase.error.empty(),"Failed save charged currency or lost dialog feedback");
  }
  Session changed(content,temporary/"unused.json",0,true);changed.domain().install(funded);store={};purchaseClick(changed,store,grid.cards[0]);
  check(bool(changed.command({.action=Action::ExpandTank,.tank={1}})),"Could not simulate a changed tank offer");
  const auto changedState=encode(changed.domain().state());purchaseClick(changed,store,confirmation.coins);
  check(encode(changed.domain().state())==changedState&&!store.purchase.dialog.open&&!store.notice.empty(),"A stale confirmation bought the next capacity step");
 }
 std::filesystem::remove_all(temporary);
 if(argc>2){
  const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
  domain.fixture("shop");auto visual=domain.state();visual.wallet={1157,0};visual.xp=46;visual.tanks={{{1},10},{{2},10},{{3},10},{{4},10},{{5},10}};visual.activeTank={1};
  const std::array<SDL_FPoint,8> fishPositions{{{830,432},{1030,430},{1185,431},{1430,501},{1588,433},{1591,538},{1416,730},{244,665}}};
  const auto* tetra=content.find("neonTetra");
  for(std::size_t i=0;i<visual.fish.size();++i){
   auto& fish=visual.fish[i];fish.species="neonTetra";fish.purchase=domain.quote(*tetra);fish.position={fishPositions[i].x*waterWidth/1641.,fishPositions[i].y*tankHeight/959.};fish.motion.previous=fish.position;fish.motion.hasPrevious=false;fish.motion.pitch=0;fish.motion.phase=0;fish.motion.direction=i%2?1:-1;fish.age=4;fish.growthMs=fish.purchase.durationMs;fish.lastFedAt=visual.simNow-fish.purchase.feedMs;
  }
  domain.install(visual);
  Canvas canvas(assets,1088,635,true);
  auto capture=[&](std::string_view name,int width,int height,bool open,bool store,ShopCategory category=ShopCategory::Tanks){
   canvas.previewViewport(PreviewViewport{width,height,1,{}});canvas.begin();canvas.scene(domain,0,0,Tool::Select,{},true,{},nullptr,0,false,true);
   const auto h=layoutHud(canvas.width(),canvas.height(),{},canvas.minimumTouchSize());
   if(store){
    const auto page=layoutShop(canvas.width(),canvas.height(),{},canvas.minimumTouchSize());auto visible=shop;
    if(visible.focusedTank)focusTankShop(visible,page,*visible.focusedTank);
    paintShop(canvas,domain,page,ShopState{category},-1,-1,&visible);
   }
   else{paintHud(canvas,domain,h);TankSwitcherState state;state.open=open;paintTankSwitcher(canvas,domain,h,state,{-1,-1});}
   check(canvas.capture(output/(std::string(name)+".png")),"Native capture failed");
  };
  capture("expanded-five",1641,959,true,false);capture("collapsed",1641,959,false,false);
  visual.activeTank={3};domain.install(visual);capture("selected-three",1641,959,true,false);visual.activeTank={1};domain.install(visual);
  // Six owned tanks reproduce the future design solely in this ephemeral renderer fixture.
  visual.tanks.push_back({{6},10});domain.install(visual);capture("reference-six",1641,959,true,false);
  visual.tanks={{{1},10}};domain.install(visual);shop={};capture("expanded-starter",1641,959,true,false);capture("shop-tanks",1641,959,false,true);
  shop.focusedTank=TankId{2};capture("shop-focused-two",1641,959,false,true);
  shop.focusedTank=TankId{6};capture("shop-coming-soon",1641,959,false,true);
  capture("phone-locked",667,375,true,false);capture("portrait-locked",390,844,true,false);
  shop.focusedTank=TankId{2};capture("portrait-shop-focused",390,844,false,true);shop={};
  visual.xp=content.levels.back();visual.wallet={1000000,1000};domain.install(visual);capture("shop-ready",1641,959,false,true);
  visual.tanks={{{1},20},{{2},10},{{3},10},{{4},10},{{5},10}};domain.install(visual);
  capture("phone-expanded",667,375,true,false);capture("tablet-expanded",1024,768,true,false);capture("portrait-expanded",390,844,true,false);capture("phone-shop",667,375,false,true);capture("portrait-shop",390,844,false,true);
  visual.tanks={{{1},10}};visual.xp=content.levels[6];visual.wallet={12650,38};visual.activeTank={1};domain.install(visual);shop={};
  capture("cards-approved",1643,957,false,true);capture("cards-phone",667,375,false,true);capture("cards-tablet",1024,768,false,true);capture("cards-portrait",390,844,false,true);capture("cards-small",320,240,false,true);
  capture("height-fish",1643,957,false,true,ShopCategory::Fish);capture("height-decorations",1643,957,false,true,ShopCategory::Decorations);
  capture("height-fish-phone",667,375,false,true,ShopCategory::Fish);
  shop.scroll=100;capture("cards-end",1643,957,false,true);
  shop={};shop.focusedTank=TankId{6};capture("cards-focused-six",390,844,false,true);
  visual.xp=0;visual.wallet={45,0};domain.install(visual);shop={};
  capture("fish-reference",1088,635,false,true,ShopCategory::Fish);capture("tanks-matched",1088,635,false,true);
  visual.tanks={{{1},35}};domain.install(visual);capture("level-one-upgrade-to-40",1088,635,false,true);
  visual.tanks={{{1},40}};domain.install(visual);capture("level-one-maxed-40",1088,635,false,true);
  visual.tanks={{{1},15}};visual.xp=content.levels[6];visual.wallet={20000,38};domain.install(visual);shop={};
  capture("confirmation-cards",1088,635,false,true);click(grid.cards[0]);
  capture("confirm-upgrade-desktop",1088,635,false,true);capture("confirm-upgrade-phone",667,375,false,true);
  capture("confirm-upgrade-portrait",390,844,false,true);capture("confirm-upgrade-small",320,240,false,true);capture("confirm-upgrade-tablet",1024,768,false,true);
  click(confirmation.dialog.close);click(grid.cards[1]);capture("confirm-unlock-desktop",1088,635,false,true);
 }
 std::cout<<"PASS all six tanks, Shop focus, owned switching, scrolling, wheel, scrollbar, keyboard, touch, cancellation, purchases, gates, save recovery and responsive layout\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
