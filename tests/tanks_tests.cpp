#include "aquarium/view.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static Panel panel(const View& view){return view.panel_;}
 static int selection(const View& view){return view.selectedTank_;}
 static const auto& buttons(const View& view){return view.buttons_;}
 static const auto& funds(const View& view){return view.fundsDialog_;}
 static Rect bounds(const View& view){return view.panelRect_;}
 static Rect button(const View& view,std::string_view id){
  for(const auto& button:view.buttons_)if(button.id==id)return button.area;
  throw std::runtime_error("Missing tank control: "+std::string(id));
 }
 static bool has(const View& view,std::string_view id){return std::any_of(view.buttons_.begin(),view.buttons_.end(),[&](const auto& b){return b.id==id;});}
};
}
namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void run(const std::filesystem::path& assets,int width,int height,bool native){
 std::ifstream input(assets/"content.json");Session session(Content::fromJson(Json::parse(input)),"/tmp/tank-menu-test-unused.json",0,true);
 auto& domain=session.domain();auto fresh=domain.state();fresh.settings.reducedMotion=true;fresh.tutorialStep=11;fresh.wallet={1000000,1000};
 domain.install(fresh);Canvas canvas(assets,width,height,!native);View view(canvas,session);double now=1;
 const auto render=[&]{view.render(now+=.7);};
 const auto finger=[&](Uint32 type,SDL_FPoint point){SDL_Event event{};event.type=type;event.tfinger.touchID=1;event.tfinger.fingerID=1;event.tfinger.x=point.x/canvas.width();event.tfinger.y=point.y/canvas.height();view.event(event,now);};
 const auto click=[&](std::string_view id){const auto r=ViewTestAccess::button(view,id);const SDL_FPoint p{r.x+r.w*.5f,r.y+r.h*.5f};finger(SDL_EVENT_FINGER_DOWN,p);finger(SDL_EVENT_FINGER_UP,p);render();};
 const auto output=assets.parent_path()/"evidence/tank-menu-reference"/(std::string(native?"native-":"")+std::to_string(width)+"x"+std::to_string(height));
 std::filesystem::create_directories(output);
 const auto capture=[&](const char* name){check(canvas.capture(output/(std::string(name)+".png")),"Cannot capture tank state");};
 view.setPanel(Panel::Tanks);render();
 check(ViewTestAccess::selection(view)==1,"Opening Tanks must select the active aquarium");
 click("tank-heading");check(ViewTestAccess::panel(view)==Panel::Tanks,"The title sign is part of the dialog, not a dismissing backdrop");
 const auto bounds=ViewTestAccess::bounds(view);
 check(bounds.x>=0&&bounds.y>=0&&bounds.x+bounds.w<=canvas.width()&&bounds.y+bounds.h<=canvas.height(),"Tank frame is outside the viewport");
 std::set<std::string> ids;
 for(const auto& button:ViewTestAccess::buttons(view)){
  check(ids.insert(button.id).second,"Duplicate control IDs can release a purchase onto the wrong button");
  if(!button.id.starts_with("tank-")&&button.id!="panel-close")continue;
  check(button.area.x>=0&&button.area.y>=0&&button.area.x+button.area.w<=canvas.width()+1&&button.area.y+button.area.h<=canvas.height()+1,"Tank control extends beyond the viewport");
 }
 const auto list=ViewTestAccess::button(view,"tank-select1"),buy=ViewTestAccess::button(view,"tank-buy"),pearl=ViewTestAccess::button(view,"tank-pearl1");
 check(list.x+list.w<buy.x&&buy.x+buy.w<pearl.x,"List and upgrade controls must occupy separate columns");
 capture("current");
 const auto before=encode(domain.state());
 click("tank-select3");
 check(ViewTestAccess::selection(view)==3&&ViewTestAccess::panel(view)==Panel::Tanks&&encode(domain.state())==before,"Selecting a locked row must only update the preview");
 click("tank-detail-buy");check(encode(domain.state())==before,"Detail purchase bypassed the level or preceding-tank gate");
 check(ViewTestAccess::has(view,"notice-ok"),"Level-locked tank purchase did not explain its requirement");
 capture("locked-notice");click("notice-close");
 capture("locked");
 auto rich=fresh;rich.xp=domain.content().levels.back();rich.highestRewardedLevel=40;domain.install(rich);render();
 const auto missingPrevious=encode(domain.state());click("tank-detail-buy");
 check(ViewTestAccess::has(view,"notice-ok")&&encode(domain.state())==missingPrevious,"Missing previous tank silently ignores Buy or changes progress");
 click("notice-ok");
 click("tank-select2");click("tank-detail-pearl");
 check(domain.tank({2})&&domain.state().activeTank.value==2&&ViewTestAccess::selection(view)==2,"Detail unlock did not select and activate the purchased tank");
 check(domain.state().wallet.coins==rich.wallet.coins&&domain.state().wallet.pearls==rich.wallet.pearls-domain.content().tankPearlCosts[1][0],"Pearl unlock charged the wrong currency");
 auto two=domain.state();click("tank-select1");
 check(encode(domain.state())==encode(two),"Previewing an owned tank switched it early");
 capture("owned");click("tank-use");
 check(domain.state().activeTank.value==1&&ViewTestAccess::panel(view)==Panel::None,"Use Tank did not activate the selected owned aquarium");
 view.setPanel(Panel::Tanks);render();
 const auto coinBefore=domain.state().wallet.coins,pearlBefore=domain.state().wallet.pearls;
 click("tank-buy");check(domain.tank({1})->slots==15&&domain.state().wallet.coins==coinBefore-domain.content().tankCosts[0][1]&&domain.state().wallet.pearls==pearlBefore,"Coin capacity upgrade did not charge once");
 capture("upgraded");
 auto poor=domain.state();poor.wallet={0,0};domain.install(poor);render();click("tank-select2");click("tank-pearl2");
 check(bool(ViewTestAccess::funds(view)),"Insufficient funds did not open the existing funds dialog");
 click("funds-shop");click("panel-close");
 check(ViewTestAccess::panel(view)==Panel::Tanks&&ViewTestAccess::selection(view)==2,"Currency shop return lost the selected tank");
 auto maximum=two;maximum.tanks[1].slots=20;domain.install(maximum);render();
 check(!ViewTestAccess::has(view,"tank-buy")&&!ViewTestAccess::has(view,"tank-pearl2"),"Maximum capacity still offers an upgrade");
 capture("maximum");
 click("panel-close");check(ViewTestAccess::panel(view)==Panel::None,"Close did not dismiss the tank menu");
 view.setPanel(Panel::Tanks);render();
 SDL_Event escape{};escape.type=SDL_EVENT_KEY_DOWN;escape.key.key=SDLK_ESCAPE;view.event(escape,now);render();
 check(ViewTestAccess::panel(view)==Panel::None,"Escape did not close Tanks");
 // An empty tank has no growing occupants.
 auto empty=fresh;empty.fish.clear();domain.install(empty);view.setPanel(Panel::Tanks);render();capture("empty");
 std::cout<<"PASS tank selection, safe layout, level gates, currency purchases, capacity limits and return state at "<<width<<'x'<<height<<'\n';
}
}
int main(int argc,char** argv){try{
 const std::filesystem::path assets=argc>1?argv[1]:"assets";const bool native=std::getenv("AQ_TEST_NATIVE")!=nullptr;
 if(native){run(assets,892,502,true);run(assets,667,375,true);}
 else for(const auto [width,height]:{std::pair{1784,1004},std::pair{667,375},std::pair{852,393},std::pair{1024,768},std::pair{600,800}})run(assets,width,height,false);
 return 0;
}catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}}
