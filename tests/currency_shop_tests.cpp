#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static Panel panel(const View& v){return v.panel_;}
 static Currency currency(const View& v){return v.currencyCategory_;}
 static const auto& funds(const View& v){return v.fundsDialog_;}
 static const auto& fundsCurrency(const View& v){return v.fundsCurrency_;}
 static Rect fundsBounds(const View& v){return v.fundsBounds_;}
 static Rect shopBounds(const View& v){return v.panelDisplayRect_;}
 static void shortfall(View& v,CurrencyShortfall missing){v.showFunds(missing);}
 static FishId selected(const View& v){return v.selected_;}
 static bool has(const View& v,std::string_view id){return std::any_of(v.buttons_.begin(),v.buttons_.end(),[&](const auto& b){return b.id==id;});}
 static bool textDrawn(const Canvas& canvas,std::string_view text){for(const auto& [key,entry]:canvas.textCache_)if(entry.used==canvas.frame_&&key.ends_with(":"+std::string(text)))return true;return false;}
 static Rect button(const View& v,std::string_view id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing "+std::string(id));}
 static SDL_FPoint windowPoint(const Canvas& canvas,SDL_FPoint p){return {canvas.displayRect_.x+p.x*canvas.displayRect_.w/canvas.width_,canvas.displayRect_.y+p.y*canvas.displayRect_.h/canvas.height_};}
};
}
namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool within(Rect inner,Rect outer){return inner.x>=outer.x-.01f&&inner.y>=outer.y-.01f&&inner.x+inner.w<=outer.x+outer.w+.01f&&inner.y+inner.h<=outer.y+outer.h+.01f;}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
void run(const std::filesystem::path& assets,int width,int height,bool software){
 std::ifstream input(assets/"content.json");Session session(Content::fromJson(Json::parse(input)),"/tmp/currency-test-unused.json",1000,true);
 auto& d=session.domain();auto state=d.state();state.fish.resize(1);auto& fish=state.fish.front();fish.position={440,300};fish.motion.previous=fish.position;const auto id=fish.id;d.install(state);
 Canvas canvas(assets,width,height,software);View view(canvas,session);double time=1;
 const auto render=[&]{view.render(time+=.7);};render();
 const auto send=[&](Uint32 type,SDL_FPoint point,bool mouse=false){
  SDL_Event event{};event.type=type;const auto p=ViewTestAccess::windowPoint(canvas,point);
  if(mouse){event.button.button=SDL_BUTTON_LEFT;event.button.x=p.x;event.button.y=p.y;}
  else{int w{},h{};SDL_GetWindowSize(canvas.window(),&w,&h);event.tfinger.touchID=1;event.tfinger.fingerID=1;event.tfinger.x=p.x/w;event.tfinger.y=p.y/h;}
  view.event(event,time);
 };
 const auto queuedTap=[&](SDL_FPoint p,bool mouse=false){send(mouse?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_FINGER_DOWN,p,mouse);send(mouse?SDL_EVENT_MOUSE_BUTTON_UP:SDL_EVENT_FINGER_UP,p,mouse);};
 const auto tap=[&](SDL_FPoint p,bool mouse=false){queuedTap(p,mouse);render();};
 const auto click=[&](std::string_view id,bool mouse=false){tap(center(ViewTestAccess::button(view,id)),mouse);};
 const auto key=[&](SDL_Keycode key){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=key;view.event(e,time);render();};
 const auto viewport=std::to_string(width)+"x"+std::to_string(height);
 const auto out=assets.parent_path()/"evidence/currency-shop-cleanup"/(software?"software":"native")/viewport,fundsOut=out/"funds";
 std::filesystem::create_directories(out);std::filesystem::create_directories(fundsOut);Json frames=Json::array();
 const auto capture=[&](const char* name,bool modal=false){
  const auto target=(modal?fundsOut:out)/name;check(canvas.capture(target),"Cannot capture currency UI");
  if(modal){
   const auto bounds=ViewTestAccess::fundsBounds(view);int pixelWidth{},pixelHeight{};SDL_GetWindowSizeInPixels(canvas.window(),&pixelWidth,&pixelHeight);
   Json frame{{"image",name},{"canvas",{canvas.width(),canvas.height()}},{"pixels",{pixelWidth,pixelHeight}},{"dialog",{bounds.x*pixelWidth/canvas.width(),bounds.y*pixelHeight/canvas.height(),bounds.w*pixelWidth/canvas.width(),bounds.h*pixelHeight/canvas.height()}}};
   if(const auto& missing=ViewTestAccess::funds(view))frame["shortfall"]={{"coins",missing->coins},{"pearls",missing->pearls}};
   if(const auto currency=ViewTestAccess::fundsCurrency(view))frame["direct_currency"]=*currency==Currency::Coins?"coins":"pearls";
   frames.push_back(std::move(frame));std::ofstream(fundsOut/"frames.json")<<frames.dump(2)<<'\n';
  }
 };
 const auto modalTargets=[&]{
  const auto bounds=ViewTestAccess::fundsBounds(view);check(bounds.w>0&&bounds.h>0&&within(bounds,{0,0,canvas.width(),canvas.height()}),"Funds artwork extends beyond the window");
  const auto safe=canvas.safeInsets();
  check(bounds.w<=620.f*canvas.minimumTouchSize()/44.f+.01f&&bounds.w<=(canvas.width()-safe.left-safe.right)*.92f+.01f&&bounds.h<=(canvas.height()-safe.top-safe.bottom)*.92f+.01f,"Message dialog extends beyond its safe limits");
  check(!ViewTestAccess::has(view,"funds-cancel"),"Message dialog retained the removed Cancel button");
  for(const auto control:{"funds-close","funds-shop"}){
   const auto r=ViewTestAccess::button(view,control);check(within(r,{0,0,canvas.width(),canvas.height()}),"Funds control extends beyond the window");
   check(r.w+.01f>=canvas.minimumTouchSize()&&r.h+.01f>=canvas.minimumTouchSize(),"Funds control is smaller than a 44-point touch target");
  }
 };
 const auto shopTargets=[&]{
  const auto safe=canvas.safeInsets();
  const Rect usable{safe.left,safe.top,canvas.width()-safe.left-safe.right,canvas.height()-safe.top-safe.bottom};
  const auto bounds=ViewTestAccess::shopBounds(view);
  check(within(bounds,usable),"Currency shop artwork extends beyond the safe window");
  check(std::abs(bounds.w/bounds.h-1124.f/729.f)<.001f,"Currency shop does not preserve the shared reference frame");
  std::vector<Rect> cards;
  const std::array<Rect,7> measured{{{326,257,244,239},{584,257,244,239},{842,257,244,239},
                                  {326,518,244,239},{584,518,244,239},{842,518,244,239},{1100,257,274,500}}};
  for(int i=0;i<7;++i){
   const auto r=ViewTestAccess::button(view,i==6?"currency-bundle":"currency-pack"+std::to_string(i));
   check(within(r,bounds),"An offer extends beyond the shop frame");
   check(r.w>=canvas.minimumTouchSize()&&r.h>=canvas.minimumTouchSize(),"An offer is smaller than a 44-point touch target");
   check(std::abs(r.w/r.h-measured[i].w/measured[i].h)<.001f,"A source card is stretched");
   const float scale=bounds.w/1124.f;
   check(std::abs(r.x-(bounds.x+(measured[i].x-278)*scale))<.05f&&
         std::abs(r.y-(bounds.y+(measured[i].y-168)*scale))<.05f,"Offer placement differs from the reference");
   for(const auto other:cards)check(r.x+r.w<=other.x||other.x+other.w<=r.x||r.y+r.h<=other.y||other.y+other.h<=r.y,"Offer cards overlap");
   cards.push_back(r);
  }
  check(within(ViewTestAccess::button(view,"panel-close"),usable),"Currency shop close button is clipped");
  for(const auto id:{"currency-coins","currency-pearls","currency-bundles"})
   check(!ViewTestAccess::has(view,id),"A removed currency tab still has a touch target");
  check(within(ViewTestAccess::button(view,"currency-earn"),bounds),"Earn Coins is clipped");
  for(const auto text:{"Small Coin Pack","Medium Coin Pack","Large Coin Pack","Small Pearl Pack","Medium Pearl Pack","Large Pearl Pack","Starter Bundle",
                       "2,500","12,000","35,000","15","40","90","25,000","30",
                       "$0.99","$1.99","$3.99","$4.99","$9.99"})
   check(ViewTestAccess::textDrawn(canvas,text),"An offer label is missing from the native text layer");
  check(cards[0].y==cards[1].y&&cards[1].y==cards[2].y,"The three coin offers do not share a row");
  check(cards[3].y==cards[4].y&&cards[4].y==cards[5].y,"The three pearl offers do not share a row");
  check(cards[6].y==cards[0].y&&std::abs(cards[6].y+cards[6].h-cards[3].y-cards[3].h)<.1f,"Starter bundle does not span both rows");

 };
 tap(canvas.toScreen(fish.position));check(ViewTestAccess::panel(view)==Panel::Details,"Fish details did not open");
 const auto before=encode(d.state());
 click("pearl-more");check(ViewTestAccess::panel(view)==Panel::CurrencyShop&&ViewTestAccess::currency(view)==Currency::Pearls,"Pearl balance did not open currency shop");shopTargets();capture("pearls.png");
 click("currency-pack0");check(ViewTestAccess::funds(view)&&!ViewTestAccess::fundsCurrency(view)&&ViewTestAccess::funds(view)->pearls==0&&encode(d.state())==before,"Unconnected pack purchase changes saved state or opens a shortage variant");capture("purchase-unavailable.png");
 click("funds-shop");check(!ViewTestAccess::funds(view)&&ViewTestAccess::panel(view)==Panel::CurrencyShop,"Unavailable purchase does not return to the shop");
 for(int i=1;i<6;++i){
  click("currency-pack"+std::to_string(i),i%2==0);
  check(ViewTestAccess::funds(view)&&encode(d.state())==before,"A Pearl offer changes the wallet before checkout is connected");
  key(SDLK_ESCAPE);check(ViewTestAccess::panel(view)==Panel::CurrencyShop&&!ViewTestAccess::funds(view),"Escape did not dismiss only the purchase message");
 }
 shopTargets();capture("combined.png");
 for(int i=0;i<7;++i){
  const auto card=center(ViewTestAccess::button(view,i==6?"currency-bundle":"currency-pack"+std::to_string(i)));
  send(SDL_EVENT_MOUSE_BUTTON_DOWN,card,true);view.render(time+=.1);
  check(!ViewTestAccess::funds(view)&&encode(d.state())==before,"Pressing an offer fires before release");
  if(i==0||i==6)capture(i==0?"coin-pressed.png":"bundle-pressed.png");
  send(SDL_EVENT_MOUSE_BUTTON_UP,{1,canvas.height()-1},true);view.render(time+=.05);
  if(i==0||i==6)capture(i==0?"coin-release.png":"bundle-release.png");
  render();check(!ViewTestAccess::funds(view)&&encode(d.state())==before,"Releasing outside an offer activates it");
  shopTargets();
 }
 const std::array<const char*,7> offerDetails{{"2,500 Coins • $0.99","12,000 Coins • $3.99","35,000 Coins • $9.99",
  "15 Pearls • $1.99","40 Pearls • $4.99","90 Pearls • $9.99","25,000 Coins + 30 Pearls • $4.99"}};
 for(int i=0;i<7;++i){
  click(i==6?"currency-bundle":"currency-pack"+std::to_string(i),i%2==0);
  check(ViewTestAccess::funds(view)&&ViewTestAccess::textDrawn(canvas,offerDetails[i])&&encode(d.state())==before,"An offer previews the wrong price or amount, or changes progress");
  key(SDLK_ESCAPE);
 }

 click("currency-bundle");key(SDLK_ESCAPE);check(ViewTestAccess::panel(view)==Panel::CurrencyShop&&!ViewTestAccess::funds(view)&&encode(d.state())==before,"Starter bundle preview changes the wallet or loses the shop");
 shopTargets();
 click("panel-close");check(ViewTestAccess::panel(view)==Panel::Details&&ViewTestAccess::selected(view)==id&&encode(d.state())==before,"Closing currency shop did not restore the selected fish");
 key(SDLK_ESCAPE);
 view.fixture("currency-shop");render();click("currency-earn",true);
 check(ViewTestAccess::panel(view)==Panel::Quests&&encode(d.state())==before,"Earn Coins does not open goals without changing the wallet");
 capture("earn-goals.png");key(SDLK_ESCAPE);
 for(const auto& [control,currency]:{std::pair{"coin-balance",Currency::Coins},{"pearl-balance",Currency::Pearls},{"coin-more",Currency::Coins},{"pearl-more",Currency::Pearls}}){
  const bool mouse=currency==Currency::Coins;
  const auto trigger=center(ViewTestAccess::button(view,control));
  const auto otherCurrency=center(ViewTestAccess::button(view,currency==Currency::Coins?"pearl-balance":"coin-balance"));
  const auto underlyingNavigation=center(ViewTestAccess::button(view,"nav1"));
  queuedTap(trigger,mouse);queuedTap(otherCurrency,mouse);queuedTap(underlyingNavigation,mouse);
  check(ViewTestAccess::panel(view)==Panel::CurrencyShop&&ViewTestAccess::currency(view)==currency&&!ViewTestAccess::funds(view)&&encode(d.state())==before,"Queued taps replace or close the Shop opened by a currency bar");
  render();check(ViewTestAccess::panel(view)==Panel::CurrencyShop&&ViewTestAccess::currency(view)==currency&&!ViewTestAccess::funds(view),"Currency bar does not open the matching Shop directly");
  shopTargets();capture((std::string(control)+"-shop.png").c_str());
  click("panel-close",mouse);check(ViewTestAccess::panel(view)==Panel::None&&encode(d.state())==before,"Closing the Shop opened by a currency bar does not preserve the aquarium and wallet");
 }
 auto poor=d.state();poor.xp=d.content().levels[2];poor.highestRewardedLevel=3;poor.wallet.coins=60;poor.wallet.pearls=3;d.install(poor);render();const auto coinBefore=encode(d.state());
 click("nav0");click("tank-buy");auto missing=ViewTestAccess::funds(view);
 check(missing&&missing->coins==240&&!missing->pearls&&encode(d.state())==coinBefore,"Tank expansion does not preserve the exact reference shortfall of 240 coins");
 check(!ViewTestAccess::fundsCurrency(view),"An actual purchase retained the previous forced currency");modalTargets();capture("coin-240.png",true);
 check(ViewTestAccess::textDrawn(canvas,"Not Enough Coins")&&ViewTestAccess::textDrawn(canvas,"You need ")&&ViewTestAccess::textDrawn(canvas,"240")&&ViewTestAccess::textDrawn(canvas," more coins"),"Coin dialog does not render its exact live sentence");
 for(const auto control:{"funds-shop","funds-close"}){
  send(SDL_EVENT_MOUSE_BUTTON_DOWN,center(ViewTestAccess::button(view,control)),true);view.render(time+=.1);
  capture(std::string_view(control)=="funds-shop"?"coin-shop-pressed.png":"coin-close-pressed.png",true);
  check(ViewTestAccess::funds(view)&&encode(d.state())==coinBefore,"Pressing a dialog control fires before release");
  send(SDL_EVENT_MOUSE_BUTTON_UP,{1,canvas.height()-1},true);render();
  check(ViewTestAccess::funds(view)&&encode(d.state())==coinBefore,"Releasing outside a dialog control activates it");
 }
 const auto referenceBounds=ViewTestAccess::fundsBounds(view);
 const auto matchingArtworkFrame=[&]{
  modalTargets();const auto bounds=ViewTestAccess::fundsBounds(view);
  check(std::abs(bounds.w/bounds.h-888.f/732.f)<.001f&&std::abs(bounds.w-referenceBounds.w)<.01f&&std::abs(bounds.h-referenceBounds.h)<.01f,"A shortage does not preserve the supplied dialog proportions");
 };
 click("funds-shop",true);check(ViewTestAccess::currency(view)==Currency::Coins,"240-coin shortfall did not open Coins");click("panel-close");check(ViewTestAccess::panel(view)==Panel::Tanks,"Shop does not return to Tanks");
 poor=d.state();poor.wallet.coins=299;poor.wallet.pearls=2;d.install(poor);render();const auto tankBefore=encode(d.state());
 for(auto currency:{Currency::Coins,Currency::Pearls}){
  click(currency==Currency::Coins?"tank-buy":"tank-pearl1");missing=ViewTestAccess::funds(view);
  check(missing&&missing->coins==int(currency==Currency::Coins)&&missing->pearls==int(currency==Currency::Pearls)&&encode(d.state())==tankBefore,"Tank dialog must show only the selected currency shortfall and leave the wallet unchanged");
  matchingArtworkFrame();check(ViewTestAccess::textDrawn(canvas,"1")&&ViewTestAccess::textDrawn(canvas,currency==Currency::Coins?" more coin":" more pearl")&&!ViewTestAccess::textDrawn(canvas,"Also need 1 pearl"),"Tank dialog includes the unselected currency");capture(currency==Currency::Coins?"tank-coin.png":"tank-pearl.png",true);
  click("funds-shop");check(ViewTestAccess::currency(view)==currency,"Tank shortfall did not open the selected currency shop");click("panel-close");
 }
 // Keep generic mixed-shortfall rendering covered independently of tank prices.
 struct MixedCase {CurrencyShortfall missing;Currency shop;const char* amount;const char* suffix;const char* extra;const char* image;};
 for(const auto& item:std::array{
  MixedCase{{2,3},Currency::Coins,"2"," more coins","Also need 3 pearls","coin-and-pearls.png"}
 }){
  ViewTestAccess::shortfall(view,item.missing);render();const auto actual=*ViewTestAccess::funds(view);
  check(actual.coins==item.missing.coins&&actual.pearls==item.missing.pearls&&encode(d.state())==tankBefore,"Mixed dialog drops a currency amount or changes the wallet");
  matchingArtworkFrame();check(ViewTestAccess::textDrawn(canvas,item.amount)&&ViewTestAccess::textDrawn(canvas,item.suffix)&&ViewTestAccess::textDrawn(canvas,item.extra),"Mixed artwork dialog does not show all required currencies");capture(item.image,true);
  click("funds-shop");check(ViewTestAccess::panel(view)==Panel::CurrencyShop&&ViewTestAccess::currency(view)==item.shop&&!ViewTestAccess::funds(view),"Mixed artwork dialog opens the wrong Shop tab");
  click("panel-close");check(ViewTestAccess::panel(view)==Panel::Tanks&&encode(d.state())==tankBefore,"Mixed dialog Shop return changes the panel or wallet");
 }
 // Large values must remain exact in the dialog model and leave the wallet alone.
 // Captures let visual review check the fitted amount within the reference row.
 for(const auto currency:{Currency::Coins,Currency::Pearls}){
  constexpr Amount large=123456789;ViewTestAccess::shortfall(view,currency==Currency::Coins?CurrencyShortfall{large,0}:CurrencyShortfall{0,large});render();
  const auto value=*ViewTestAccess::funds(view);check((currency==Currency::Coins?value.coins:value.pearls)==large&&encode(d.state())==tankBefore,"Large shortage was rounded or changed the wallet");
  modalTargets();capture(currency==Currency::Coins?"coin-large.png":"pearl-large.png",true);key(SDLK_ESCAPE);check(!ViewTestAccess::funds(view)&&ViewTestAccess::panel(view)==Panel::Tanks,"Escape does not preserve the underlying panel");
 }
 // Choosing Pearls with no Pearl balance must open the Pearl shop,
 // even when the wallet could afford the alternative coin price.
 poor=d.state();poor.wallet.coins=300;poor.wallet.pearls=0;d.install(poor);render();const auto pearlBefore=encode(d.state());click("tank-pearl1");missing=ViewTestAccess::funds(view);
 check(missing&&!missing->coins&&missing->pearls==3&&encode(d.state())==pearlBefore,"Pearl-only tank failure invents a Coin shortfall");
 check(ViewTestAccess::textDrawn(canvas,"Not Enough Pearls")&&ViewTestAccess::textDrawn(canvas,"Get more pearls from the shop!"),"Pearl dialog retained coin copy");
 check(ViewTestAccess::textDrawn(canvas,"3")&&ViewTestAccess::textDrawn(canvas," more pearls"),"Pearl-only tank failure is mislabeled");capture("pearl-only.png",true);
 click("funds-shop");check(!ViewTestAccess::funds(view)&&ViewTestAccess::panel(view)==Panel::CurrencyShop&&ViewTestAccess::currency(view)==Currency::Pearls&&encode(d.state())==pearlBefore,"Pearl-only tank failure opens the wrong shop or changes the wallet");
 click("panel-close");check(ViewTestAccess::panel(view)==Panel::Tanks,"Pearl shop does not return to Tanks");
 std::cout<<"PASS currency dialog variants, exact amounts, 44-point controls, mouse/touch modal input, shop routing and unchanged wallet "<<width<<'x'<<height<<'\n';
}
}
int main(int argc,char** argv){try{if(argc<2||argc>3||(argc==3&&std::string_view(argv[2])!="--native"))return 2;for(const auto [w,h]:{std::pair{667,375},std::pair{852,393},std::pair{1024,768},std::pair{1472,744},std::pair{1472,950},std::pair{600,800},std::pair{1674,930}})run(std::filesystem::absolute(argv[1]),w,h,argc==2);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
