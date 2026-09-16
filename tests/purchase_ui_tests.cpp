#include "aquarium/view.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static Panel panel(const View& v){return v.panel_;}
 static int page(const View& v){return v.page_;}
 static int category(const View& v){return v.category_;}
 static Currency currency(const View& v){return v.currencyCategory_;}
 static const auto& funds(const View& v){return v.fundsDialog_;}
 static bool textDrawn(const Canvas& canvas,std::string_view text){for(const auto& [key,entry]:canvas.textCache_)if(entry.used==canvas.frame_&&key.ends_with(":"+std::string(text)))return true;return false;}
 static SDL_FPoint windowPoint(const Canvas& canvas,SDL_FPoint p){return {canvas.displayRect_.x+p.x*canvas.displayRect_.w/canvas.width_,canvas.displayRect_.y+p.y*canvas.displayRect_.h/canvas.height_};}
 static FishId selected(const View& v){return v.selected_;}
 static bool has(const View& v,std::string_view id){return std::any_of(v.buttons_.begin(),v.buttons_.end(),[&](const auto& b){return b.id==id;});}
 static Rect pageArea(const View& v){return v.pageArea_;}
 static Rect button(const View& v,std::string_view id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing "+std::string(id));}
 static std::vector<std::string> items(const View& v,std::string_view prefix){
  std::vector<std::string> result;for(const auto& b:v.buttons_)if(b.id.starts_with(prefix))result.push_back(b.id.substr(prefix.size()));return result;
 }
};
}
namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int redPixels(Canvas& canvas,Rect area){
 using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
 Surface raw(SDL_RenderReadPixels(canvas.renderer(),nullptr),SDL_DestroySurface);check(bool(raw),"Cannot read prices");
 Surface rgba(SDL_ConvertSurface(raw.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);check(bool(rgba),"Cannot convert prices");
 const float sx=float(rgba->w)/canvas.width(),sy=float(rgba->h)/canvas.height();int count=0;
 for(int y=std::max(0,int(area.y*sy));y<std::min(rgba->h,int((area.y+area.h)*sy));++y)
  for(int x=std::max(0,int(area.x*sx));x<std::min(rgba->w,int((area.x+area.w)*sx));++x){
   const auto* p=static_cast<const Uint8*>(rgba->pixels)+y*rgba->pitch+x*4;
   count+=p[0]>200&&p[1]<120&&p[2]<120;
  }
 return count;
}
void run(const std::filesystem::path& assets,int width,int height,bool native){
 std::ifstream input(assets/"content.json");Session session(Content::fromJson(Json::parse(input)),"/tmp/purchase-ui-unused.json",1000,true);
 auto& domain=session.domain();auto fresh=domain.state();fresh.settings.reducedMotion=true;fresh.tutorialStep=11;
 auto rich=fresh;rich.xp=domain.content().levels.back();rich.highestRewardedLevel=40;rich.wallet={1000000,100000};
 domain.install(rich);Canvas canvas(assets,width,height,!native);View view(canvas,session);double time=1;
 const auto render=[&]{view.render(time+=.7);};render();
 const auto finger=[&](Uint32 type,SDL_FPoint point){const auto p=ViewTestAccess::windowPoint(canvas,point);int w{},h{};SDL_GetWindowSize(canvas.window(),&w,&h);SDL_Event e{};e.type=type;e.tfinger.touchID=1;e.tfinger.fingerID=1;e.tfinger.x=p.x/w;e.tfinger.y=p.y/h;view.event(e,time);};
 const auto click=[&](std::string_view id){const auto r=ViewTestAccess::button(view,id);const SDL_FPoint p{r.x+r.w*.5f,r.y+r.h*.5f};finger(SDL_EVENT_FINGER_DOWN,p);finger(SDL_EVENT_FINGER_UP,p);render();};
 const auto mouseClick=[&](std::string_view id){
  const auto r=ViewTestAccess::button(view,id);const auto p=ViewTestAccess::windowPoint(canvas,{r.x+r.w*.5f,r.y+r.h*.5f});
  SDL_Event e{};e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=SDL_BUTTON_LEFT;e.button.x=p.x;e.button.y=p.y;view.event(e,time);
  view.render(time+=.1);e.type=SDL_EVENT_MOUSE_BUTTON_UP;view.event(e,time);render();
 };
 const auto escape=[&]{SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=SDLK_ESCAPE;view.event(e,time);render();};
 const auto nextPage=[&]{const auto r=ViewTestAccess::pageArea(view);const SDL_FPoint start{r.x+r.w*.7f,r.y+r.h*.3f},end{start.x-canvas.minimumTouchSize()*2,start.y};
  finger(SDL_EVENT_FINGER_DOWN,start);finger(SDL_EVENT_FINGER_MOTION,end);finger(SDL_EVENT_FINGER_UP,end);render();
 };
 const auto out=assets.parent_path()/"evidence/shop-shortfall"/(std::string(native?"native-":"")+std::to_string(width)+"x"+std::to_string(height));std::filesystem::create_directories(out);
 // A fresh wallet cannot buy upgrades or unlocks. The domain checks levels
 // first, but each price button must still show its own currency shortfall.
 auto starter=fresh;starter.wallet={250,0};starter.settings.reducedMotion=false;domain.install(starter);render();mouseClick("nav0");
 const auto starterBefore=encode(domain.state());
 for(int id:{1,2}){
  mouseClick("tank-select"+std::to_string(id));const auto* next=domain.nextTankEntitlement({id});check(next,"Missing starter tank offer");
  for(const auto currency:{Currency::Coins,Currency::Pearls}){
   const std::string control=currency==Currency::Coins?(id==1?"tank-buy":"tank-detail-buy"):(id==1?"tank-pearl1":"tank-detail-pearl");
   mouseClick(control);const auto missing=ViewTestAccess::funds(view);
   check(missing&&missing->coins==(currency==Currency::Coins?next->cost.coins-starter.wallet.coins:0)&&missing->pearls==(currency==Currency::Pearls?next->cost.pearls:0),"Locked tank Buy silently ignores the selected currency shortage");
   check(ViewTestAccess::textDrawn(canvas,"Requires level "+std::to_string(next->level))&&encode(domain.state())==starterBefore,"Currency dialog hides the level requirement or changes progress");
   check(!ViewTestAccess::has(view,control),"Tank price stays active behind the currency dialog");
   check(canvas.capture(out/("starter-tank"+std::to_string(id)+(currency==Currency::Coins?"-coins-locked.png":"-pearls-locked.png"))),"Cannot capture locked tank shortfall");
   mouseClick("funds-shop");check(ViewTestAccess::panel(view)==Panel::CurrencyShop&&ViewTestAccess::currency(view)==currency,"Locked tank shortage opens the wrong shop");mouseClick("panel-close");
  }
 }
 mouseClick("tank-select1");
 auto fundedStarter=starter;fundedStarter.wallet=rich.wallet;domain.install(fundedStarter);render();
 for(const char* control:{"tank-buy","tank-pearl1"}){
  const auto before=encode(domain.state());mouseClick(control);
  check(ViewTestAccess::has(view,"notice-ok")&&ViewTestAccess::textDrawn(canvas,"Level 3 Needed"),"Funded but level-locked tank purchase silently ignores Buy");
  check(!ViewTestAccess::funds(view)&&encode(domain.state())==before,"Level gate offered currency or changed progress");
  check(canvas.capture(out/"starter-level-notice.png"),"Cannot capture starter purchase notice");
  if(std::string_view(control)=="tank-buy")mouseClick("notice-ok");else escape();
 }
 starter.xp=domain.content().levels[2];starter.highestRewardedLevel=3;domain.install(starter);render();
 const auto eligibleBefore=encode(domain.state());
 for(const auto currency:{Currency::Coins,Currency::Pearls}){
  mouseClick(currency==Currency::Coins?"tank-buy":"tank-pearl1");const auto missing=ViewTestAccess::funds(view);
  check(missing&&missing->coins==(currency==Currency::Coins?50:0)&&missing->pearls==(currency==Currency::Pearls?3:0),"Eligible starter upgrade does not show the coin or pearl shortage");
  check(ViewTestAccess::textDrawn(canvas,currency==Currency::Coins?"Not Enough Coins":"Not Enough Pearls")&&encode(domain.state())==eligibleBefore,"Shortage dialog is not rendered or changed progress");
  check(canvas.capture(out/(currency==Currency::Coins?"starter-coins.png":"starter-pearls.png")),"Cannot capture starter shortfall");
  mouseClick("funds-close");
 }
 mouseClick("panel-close");domain.install(rich);render();
 click("nav1");
 for(int category:{0,1,2})for(Currency currency:{Currency::Coins,Currency::Pearls}){
  domain.install(rich);render();click("shop-tab"+std::to_string(category));
  // Use a later page so returning from the currency shop must restore it.
  nextPage();const std::string prefix=category==0?"buy":"decor-buy";std::string id;Amount price{};
  for(int attempt=0;attempt<12&&id.empty();++attempt){
   for(const auto& candidate:ViewTestAccess::items(view,prefix)){
    const auto* fish=category==0?domain.content().find(candidate):nullptr;
    const auto* decor=category==0?nullptr:domain.content().findDecor(candidate);
    if((fish?fish->currency:decor->currency)==currency){id=candidate;price=fish?(fish->companion?fish->price:domain.quote(*fish).principal):decor->price;break;}
   }
   if(id.empty()){const auto previous=ViewTestAccess::page(view);nextPage();if(previous==ViewTestAccess::page(view))break;}
  }
  check(!id.empty()&&price>0,"Cannot find a paid shop item for each currency");
  const auto control=prefix+id;const int page=ViewTestAccess::page(view);
  const std::string stem=std::string(category==0?"fish":category==1?"plants":"decorations")+(currency==Currency::Coins?"-coins":"-pearls");
  auto exact=rich;if(currency==Currency::Coins)exact.wallet.coins=price;else exact.wallet.pearls=price;
  domain.install(exact);render();const int affordableRed=redPixels(canvas,ViewTestAccess::button(view,control));
  auto poor=exact;if(currency==Currency::Coins)--poor.wallet.coins;else --poor.wallet.pearls;
  domain.install(poor);render();const auto before=encode(domain.state());
  check(redPixels(canvas,ViewTestAccess::button(view,control))>affordableRed+5,"Unaffordable shop price did not turn red");
  check(canvas.capture(out/(stem+"-price.png")),"Cannot capture red shop price");
  click(control);const auto missing=ViewTestAccess::funds(view);
  check(missing&&missing->coins==(currency==Currency::Coins?1:0)&&missing->pearls==(currency==Currency::Pearls?1:0),"Shop menu does not show the exact missing currency");
  check(ViewTestAccess::panel(view)==Panel::Shop&&view.tool()==Tool::Select&&encode(domain.state())==before,"Shortfall changed ownership, wallet or placement");
  check(ViewTestAccess::items(view,prefix).empty(),"Shortfall menu leaves shop purchase controls active behind it");
  check(canvas.capture(out/(stem+"-menu.png")),"Cannot capture shop shortfall menu");
  click("funds-close");
  check(!ViewTestAccess::funds(view)&&ViewTestAccess::page(view)==page&&ViewTestAccess::category(view)==category&&encode(domain.state())==before,"Cancel changed the shop context or game state");
  click(control);escape();check(!ViewTestAccess::funds(view)&&ViewTestAccess::panel(view)==Panel::Shop,"Escape did not dismiss only the shortfall menu");
  click(control);click("funds-shop");
  check(ViewTestAccess::panel(view)==Panel::CurrencyShop&&ViewTestAccess::currency(view)==currency,"Shortfall opens the wrong currency shop tab");
  click("panel-close");
  check(ViewTestAccess::panel(view)==Panel::Shop&&ViewTestAccess::page(view)==page&&ViewTestAccess::category(view)==category&&encode(domain.state())==before,"Currency shop did not restore the previous shop page and category");
  domain.install(exact);render();
  check(redPixels(canvas,ViewTestAccess::button(view,control))==affordableRed,"Affordable shop price stays red after the balance changes");
 }
 std::cout<<"PASS red prices at the balance threshold, shortfall menus, cancellation and currency shop return for fish, plants and decorations "<<width<<'x'<<height<<'\n';
 click("panel-close");click("nav0");
 const auto checkTank=[&](int id,bool owned,bool active){
  auto exact=rich;exact.tanks.clear();
  for(int preceding=1;preceding<=id-(owned?0:1);++preceding)exact.tanks.push_back({{preceding},10});
  exact.activeTank={active?id:1};
  const int step=owned?1:0;
  exact.wallet.coins=domain.content().tankCosts[id-1][step];exact.wallet.pearls=domain.content().tankPearlCosts[id-1][step];
  check(exact.wallet.coins>0&&exact.wallet.pearls>0,"Tank fixture requires a price for each currency");
  domain.install(exact);render();click("tank-select"+std::to_string(id));
  const std::string coin=active?"tank-buy":"tank-buy"+std::to_string(id),pearl="tank-pearl"+std::to_string(id);
  const int affordableCoinRed=redPixels(canvas,ViewTestAccess::button(view,coin)),affordablePearlRed=redPixels(canvas,ViewTestAccess::button(view,pearl));
  for(int shortage:{1,2,3}){
   const bool coins=shortage&1,pearls=shortage&2;
   auto poor=exact;if(coins)--poor.wallet.coins;if(pearls)--poor.wallet.pearls;
   domain.install(poor);render();const auto before=encode(domain.state());
   const int coinRed=redPixels(canvas,ViewTestAccess::button(view,coin)),pearlRed=redPixels(canvas,ViewTestAccess::button(view,pearl));
   check(coins?coinRed>affordableCoinRed+5:coinRed==affordableCoinRed,"Tank coin price does not match the coin balance");
   check(pearls?pearlRed>affordablePearlRed+5:pearlRed==affordablePearlRed,"Tank Pearl price does not match the Pearl balance");
   const std::string stem="tank-"+std::to_string(id)+(owned?"-upgrade":"-unlock");
   if(shortage==3)check(canvas.capture(out/(stem+"-price.png")),"Cannot capture unaffordable tank costs");
   for(auto currency:{Currency::Coins,Currency::Pearls}){
    if(currency==Currency::Coins?!coins:!pearls)continue;
    const auto& control=currency==Currency::Coins?coin:pearl;
    click(control);const auto missing=ViewTestAccess::funds(view);
    check(missing&&missing->coins==int(currency==Currency::Coins)&&missing->pearls==int(currency==Currency::Pearls),"Tank menu must show only the selected currency shortfall");
    check(ViewTestAccess::panel(view)==Panel::Tanks&&encode(domain.state())==before,"Unfunded tank action changes progress");
    check(!ViewTestAccess::has(view,coin)&&!ViewTestAccess::has(view,pearl),"Tank buttons stay active behind the shortfall menu");
    if(shortage==3)check(canvas.capture(out/(stem+(currency==Currency::Coins?"-coins-menu.png":"-pearls-menu.png"))),"Cannot capture tank shortfall menu");
    click("funds-close");
    check(!ViewTestAccess::funds(view)&&ViewTestAccess::panel(view)==Panel::Tanks&&encode(domain.state())==before,"Dismissing the tank menu changes progress");
    click(control);click("funds-shop");
    check(!ViewTestAccess::funds(view)&&ViewTestAccess::panel(view)==Panel::CurrencyShop&&ViewTestAccess::currency(view)==currency&&encode(domain.state())==before,"Tank shortfall opens the wrong currency tab or changes progress");
    click("panel-close");
    check(ViewTestAccess::panel(view)==Panel::Tanks&&encode(domain.state())==before,"Currency shop does not return to the unchanged Tanks menu");
   }
  }
  domain.install(exact);render();
  check(redPixels(canvas,ViewTestAccess::button(view,coin))==affordableCoinRed&&redPixels(canvas,ViewTestAccess::button(view,pearl))==affordablePearlRed,"Tank costs stay red after balances recover");
  for(auto currency:{Currency::Coins,Currency::Pearls})for(bool emptyOther:{false,true}){
   auto funded=exact;
   if(emptyOther)(currency==Currency::Coins?funded.wallet.pearls:funded.wallet.coins)=0;
   domain.install(funded);render();click(currency==Currency::Coins?coin:pearl);
   const auto* purchased=domain.tank({id});
   check(!ViewTestAccess::funds(view)&&purchased&&purchased->slots==(owned?15:10),"Either currency must buy the tank without requiring the other balance");
   check(domain.state().wallet.coins==(currency==Currency::Coins?0:funded.wallet.coins)&&domain.state().wallet.pearls==(currency==Currency::Pearls?0:funded.wallet.pearls),"Tank purchase must charge only the selected currency exactly once");
  }
 };
 for(int id=1;id<=5;++id)checkTank(id,id==1,id==1);
 checkTank(2,true,false);checkTank(5,true,true);
 auto maximum=domain.state();maximum.tanks.back().slots=20;domain.install(maximum);render();
 check(!ViewTestAccess::has(view,"tank-buy")&&!ViewTestAccess::has(view,"tank-pearl5"),"Maximum tank still offers an upgrade");
 click("panel-close");
 std::cout<<"PASS independent tank costs, all tank unlocks, active and inactive upgrades, maximum capacity and shortfalls "<<width<<'x'<<height<<'\n';
}
}
int main(int argc,char** argv){try{if(argc<2||argc>3)return 2;const bool native=argc==3&&std::string(argv[2])=="--native";for(const auto [w,h]:{std::pair{667,375},std::pair{852,393},std::pair{1024,768},std::pair{1472,950}})run(std::filesystem::absolute(argv[1]),w,h,native);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
