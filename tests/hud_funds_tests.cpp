#include "aquarium/hud_funds.hpp"
#include "aquarium/hud_care.hpp"
#include "aquarium/hud_placement.hpp"
#include "aquarium/hud_tanks.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
SDL_Event button(Uint32 type){SDL_Event e{};e.type=type;e.button.button=SDL_BUTTON_LEFT;return e;}
bool inside(Rect a,Rect b){return a.x>=b.x-.1f&&a.y>=b.y-.1f&&a.x+a.w<=b.x+b.w+.1f&&a.y+a.h<=b.y+b.h+.1f;}
}
int main(int argc,char** argv){try{
 using namespace aq;
 const std::filesystem::path assets=argc>1?argv[1]:"assets";
 std::ifstream in(assets/"content.json");const auto content=Content::fromJson(Json::parse(in));
 Session session(content,"/tmp/fishius-funds-unused.json",0,true);auto& domain=session.domain();
 auto poor=domain.state();poor.xp=content.levels.back();poor.wallet={0,0};domain.install(poor);
 FundsDialogState funds;const auto dialog=layoutFundsDialog(1608,940,{});
 auto event=[&](Uint32 type,Rect r){return fundsDialogEvent(funds,dialog,button(type),center(r));};
 auto click=[&](Rect r){event(SDL_EVENT_MOUSE_BUTTON_DOWN,r);return event(SDL_EVENT_MOUSE_BUTTON_UP,r);};
 auto verify=[&](const Result& result,Currency currency,Amount missing){
  check(showFundsDialog(funds,result)&&funds.open(),"Purchase shortage did not open the shared dialog");
  check(fundsCurrency(funds)==currency,"Shortage chose the other currency");
  check(fundsDialogMessage(funds).find(currency==Currency::Coins?"coin":"pearl")!=std::string::npos,"Shortage message names the wrong currency");
  check((currency==Currency::Coins?funds.shortfall.coins:funds.shortfall.pearls)==missing,"Dialog lost the exact shortfall");
  bool shopOpen=true;ShopState shop{ShopCategory::Fish,1,2.5f};FundsShopReturn navigation;
  check(click(dialog.shop)==FundsDialogEvent::OpenShop&&!funds.open(),"Open Shop button failed");
  navigation.open(funds,shopOpen,shop);
  check(shopOpen&&shop.category==ShopCategory::Treasure&&shop.subtab==(currency==Currency::Pearls?1:0)&&shop.scroll==0,"Open Shop did not select the matching currency packs");
  navigation.close(shopOpen,shop);
  check(shopOpen&&shop.category==ShopCategory::Fish&&shop.subtab==1&&shop.scroll==2.5f,"Closing Treasure lost the catalog or scroll position");
  navigation.close(shopOpen,shop);check(!shopOpen,"Closing the restored catalog kept the shop open");
 };
 // Both balances are empty. Each purchase must identify only its own cost.
 const auto before=encode(domain.state());
 for(const auto currency:{Currency::Coins,Currency::Pearls}){
  const auto species=std::find_if(content.species.begin(),content.species.end(),[&](const auto& s){return s.currency==currency&&domain.blocker(s).error==Error::Funds;});
  check(species!=content.species.end(),"Missing purchasable fish for currency test");
  FishPlacement placement;const auto result=startFishPlacement(domain,placement,species->id);
  verify(result,currency,currency==Currency::Coins?domain.quote(*species).principal:species->price);
  check(!placement.active()&&encode(domain.state())==before,"Blocked fish selection charged or armed placement");
  check(result.message==(currency==Currency::Coins?"Not enough coins.":"Not enough pearls."),"Domain fallback lacks the specific currency");
 }
 const auto background=session.command({.action=Action::PurchaseEnvironment,.tank={1},.key="coral-garden"});
 verify(background,Currency::Coins,findEnvironment("coral-garden")->price);
 check(encode(domain.state())==before,"Blocked background purchase changed the save");
 const auto shopPage=layoutShop(1608,940,{});const auto tankGrid=layoutTankShop(shopPage);
 for(int tank=1;tank<=2;++tank)for(bool pearls:{false,true}){
  auto state=poor;if(tank==2)state.tanks.front().slots=20;domain.install(state);
  const auto snapshot=encode(domain.state());const auto cost=domain.nextTankEntitlement({tank})->cost;
  TankShopState tankShop;const auto point=center(pearls?tankGrid.pearls[tank-1]:tankGrid.coins[tank-1]);
  tankShopEvent(session,tankShop,shopPage,button(SDL_EVENT_MOUSE_BUTTON_DOWN),point);
  tankShopEvent(session,tankShop,shopPage,button(SDL_EVENT_MOUSE_BUTTON_UP),point);
  check(tankShop.shortfall.has_value()&&tankShop.notice.empty(),"Tank purchase only shows an inline error");
  verify({.error=Error::Funds,.shortfall=*tankShop.shortfall},pearls?Currency::Pearls:Currency::Coins,pearls?cost.pearls:cost.coins);
  check((pearls?tankShop.shortfall->coins:tankShop.shortfall->pearls)==0&&encode(domain.state())==snapshot,"Tank shortage includes the other currency or changes progress");
 }
 // The first egg spends the last coins. The next tap must exit placement and surface the same prompt.
 auto state=poor;const auto price=domain.quote(*content.find("neonTetra")).principal;state.wallet.coins=price;domain.install(state);
 FishPlacement placement;check(bool(startFishPlacement(domain,placement,"neonTetra")),"Could not begin last-coin placement");
 const auto placementLayout=layoutFishPlacement(1608,940,{});const SDL_FPoint water{650,400};
 auto place=[&]{fishPlacementEvent(session,placement,placementLayout,button(SDL_EVENT_MOUSE_BUTTON_DOWN),water);return fishPlacementEvent(session,placement,placementLayout,button(SDL_EVENT_MOUSE_BUTTON_UP),water);};
 check(place()==PlacementEvent::Placed&&domain.state().wallet.coins==0,"First egg did not spend the remaining coins");
 const auto lastEgg=encode(domain.state());
 check(place()==PlacementEvent::None&&!placement.active()&&placement.shortfall&&placement.error.empty(),"Repeated placement keeps a generic error banner");
 verify({.error=Error::Funds,.shortfall=*placement.shortfall},Currency::Coins,price);
 check(encode(domain.state())==lastEgg&&placement.receipts.size()==1,"Failed repeat placement changes the save or loses its receipt");

 const Result coin{.error=Error::Funds,.shortfall={240,0}},pearl{.error=Error::Funds,.shortfall={0,12}};
 showFundsDialog(funds,coin);check(fundsDialogMessage(funds)=="240 coins needed","Coin amount sentence is wrong");
 check(!showFundsDialog(funds,{.error=Error::Level}),"Level gate was replaced with a funds dialog");
 check(event(SDL_EVENT_MOUSE_BUTTON_UP,dialog.shop)==FundsDialogEvent::Handled&&funds.open(),"Unmatched release opened the shop");
 event(SDL_EVENT_MOUSE_BUTTON_DOWN,dialog.shop);event(SDL_EVENT_MOUSE_MOTION,dialog.illustration);event(SDL_EVENT_MOUSE_BUTTON_UP,dialog.shop);check(funds.open(),"Dragging away and back opened the shop");
 for(const auto type:{SDL_EVENT_WINDOW_FOCUS_LOST,SDL_EVENT_WILL_ENTER_BACKGROUND,SDL_EVENT_RENDER_DEVICE_RESET,SDL_EVENT_RENDER_TARGETS_RESET}){
  event(SDL_EVENT_MOUSE_BUTTON_DOWN,dialog.shop);SDL_Event cancelled{};cancelled.type=type;fundsDialogEvent(funds,dialog,cancelled,{});event(SDL_EVENT_MOUSE_BUTTON_UP,dialog.shop);check(funds.open(),"Interrupted gesture opened the shop");
 }
 check(click(dialog.dialog.close)==FundsDialogEvent::Handled&&!funds.open(),"X did not dismiss and consume input");
 showFundsDialog(funds,pearl);SDL_Event key{};key.type=SDL_EVENT_KEY_DOWN;key.key.key=SDLK_ESCAPE;check(fundsDialogEvent(funds,dialog,key,{})==FundsDialogEvent::Handled&&!funds.open(),"Escape did not dismiss the prompt");
 showFundsDialog(funds,coin);const Rect outside{0,0,1,1};check(click(outside)==FundsDialogEvent::Handled&&!funds.open(),"Backdrop dismissal did not consume the click");
 showFundsDialog(funds,pearl);key.key.key=SDLK_RETURN;key.key.repeat=true;fundsDialogEvent(funds,dialog,key,{});check(funds.open(),"Held Return activated the shop");key.key.repeat=false;check(fundsDialogEvent(funds,dialog,key,{})==FundsDialogEvent::OpenShop&&!funds.open(),"Return did not open the shop");
 bool shopOpen=false;ShopState shop;FundsShopReturn navigation;navigation.open(funds,shopOpen,shop);navigation.close(shopOpen,shop);check(!shopOpen,"Aquarium shortage did not return to the aquarium");
 shopOpen=true;navigation.open(funds,shopOpen,shop);shopOpen=false;navigation.close(shopOpen,shop);check(!shopOpen&&!navigation.previous,"Escape in the aquarium reopened a stale shop return");
 showFundsDialog(funds,pearl);HudPointer touch;
 for(const auto type:{SDL_EVENT_FINGER_DOWN,SDL_EVENT_FINGER_UP}){
  const auto p=center(dialog.shop);SDL_Event e{};e.type=type;e.tfinger.fingerID=7;e.tfinger.x=p.x/1608;e.tfinger.y=p.y/940;
  check(normalizeHudPointer(touch,e,1608,940),"Touch did not normalize");const auto result=fundsDialogEvent(funds,dialog,e,{e.button.x,e.button.y});
  check(result==(type==SDL_EVENT_FINGER_UP?FundsDialogEvent::OpenShop:FundsDialogEvent::Handled),"Touch action differs from mouse");
 }
 for(const auto missing:{CurrencyShortfall{1,0},CurrencyShortfall{0,1},CurrencyShortfall{123456789,0},CurrencyShortfall{0,123456789},CurrencyShortfall{2,3}}){
  showFundsDialog(funds,{.error=Error::Funds,.shortfall=missing});
  if(missing.coins==1||missing.pearls==1)check(fundsDialogMessage(funds)==(missing.coins?"1 coin needed":"1 pearl needed"),"Singular shortfall is wrong");
  if(missing.coins==123456789||missing.pearls==123456789)check(fundsDialogMessage(funds).starts_with("123,456,789"),"Large shortfall is rounded or ungrouped");
 }
 Canvas canvas(assets,1088,635,true);
 for(const auto size:std::array{SDL_FPoint{1088,635},SDL_FPoint{667,375},SDL_FPoint{1024,768},SDL_FPoint{390,844},SDL_FPoint{320,240},SDL_FPoint{844,390}}){
  const Insets safe=size.x==844?Insets{59,0,59,21}:Insets{};
  canvas.previewViewport(PreviewViewport{int(size.x),int(size.y),1,safe});canvas.begin();
  const auto margins=canvas.safeInsets();const Rect safeArea{margins.left,margins.top,canvas.width()-margins.left-margins.right,canvas.height()-margins.top-margins.bottom};
  const auto l=layoutFundsDialog(canvas.width(),canvas.height(),margins,canvas.minimumTouchSize());
  check(inside(l.dialog.frame,safeArea),"Funds dialog crosses the safe area");
  check(inside(l.scene,l.dialog.body),"Funds scene crosses the dialog frame");
  for(const auto r:{l.message,l.illustration,l.shop})check(inside(r,l.dialog.content),"Funds content crosses its bounds");
  check(l.message.y+l.message.h<=l.illustration.y&&l.illustration.y+l.illustration.h<=l.shop.y,"Funds amount, artwork or shop button overlap");
  check(l.dialog.close.h>=canvas.minimumTouchSize()-.1f&&l.shop.h>=canvas.minimumTouchSize()-.1f,"Funds controls are smaller than a touch target");
  for(bool pearls:{false,true}){
   showFundsDialog(funds,pearls?pearl:coin);canvas.begin();paintShop(canvas,domain,layoutShop(canvas.width(),canvas.height(),margins,canvas.minimumTouchSize()),{ShopCategory::Fish});paintFundsDialog(canvas,l,funds);
   if(argc>2){
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
    check(canvas.capture(output/(std::string(pearls?"pearls-":"coins-")+std::to_string(int(size.x))+"x"+std::to_string(int(size.y))+".png")),"Cannot capture funds dialog");
   }
  }
 }
 std::cout<<"PASS currency-specific fish, tank, background and repeated-placement prompts, exact shortfalls, shop return, input cancellation and responsive layout\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
