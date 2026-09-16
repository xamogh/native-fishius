#include "fixtures.hpp"
#include "aquarium/view.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static const auto& rewards(const View& v){return v.levelUps_;}
 static Panel panel(const View& v){return v.panel_;}
 static int page(const View& v){return v.levelUnlockPage_;}
 static int shopPage(const View& v){return v.page_;}
 static int category(const View& v){return v.category_;}
 static bool highlights(const View& v,const LevelUnlock& item){return item.tank.value?v.highlightedTank_==item.tank:v.shopHighlightedId_==item.id&&v.shopHighlightedCategory_==item.category;}
 static Rect button(const View& v,std::string_view id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing "+std::string(id));}
 static bool has(const View& v,std::string_view id){return std::any_of(v.buttons_.begin(),v.buttons_.end(),[&](const auto& b){return b.id==id;});}
};
}
namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void run(const std::filesystem::path& assets,int width,int height){
 std::ifstream in(assets/"content.json");auto content=Content::fromJson(Json::parse(in));
 Session session(content,"/tmp/level-up-unused.json",0,true);auto& domain=session.domain();
 auto state=domain.state();state.xp=79;state.settings.reducedMotion=true;state.tutorialStep=11;testing::stage(state.fish.front(),4);testing::openingBalances(state);domain.install(state);
 Canvas canvas(assets,width,height,true);View view(canvas,session);double time=1;
 const auto render=[&]{view.render(time+=.8);};
 const auto tap=[&](SDL_FPoint point){
  int ww{},wh{};SDL_GetWindowSize(canvas.window(),&ww,&wh);
  for(auto type:{SDL_EVENT_MOUSE_BUTTON_DOWN,SDL_EVENT_MOUSE_BUTTON_UP}){
   SDL_Event e{};e.type=type;e.button.button=SDL_BUTTON_LEFT;e.button.x=point.x/canvas.width()*ww;e.button.y=point.y/canvas.height()*wh;view.event(e,time);
  }
 };
 const auto click=[&](std::string_view id){const auto r=ViewTestAccess::button(view,id);tap({r.x+r.w*.5f,r.y+r.h*.5f});render();};
 const auto touch=[&](std::string_view id){
  const auto r=ViewTestAccess::button(view,id);
  for(auto type:{SDL_EVENT_FINGER_DOWN,SDL_EVENT_FINGER_UP}){
   SDL_Event e{};e.type=type;e.tfinger.fingerID=1;e.tfinger.x=(r.x+r.w*.5f)/canvas.width();e.tfinger.y=(r.y+r.h*.5f)/canvas.height();view.event(e,time);
  }
  render();
 };
 const auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=code;view.event(e,time);};
 const auto out=assets.parent_path()/"evidence/level-up"/(std::to_string(width)+"x"+std::to_string(height));std::filesystem::create_directories(out);
 check(bool(session.command({.action=Action::Keep,.fish=domain.state().fish.front().id})),"Cannot trigger real level-up");render();
 check(ViewTestAccess::rewards(view).size()==1&&ViewTestAccess::rewards(view).front().reachedLevel==2,"Level-up did not open");
 const auto items=levelUnlocks(domain,2);
 check(items.size()==4&&items[0].name=="Zebra Danio"&&items[1].name=="Bubble Eye Goldfish"&&items[2].name=="Dwarf Hairgrass Patch"&&items[3].name=="Spiral Shell","Level 2 does not show actual unlocks");
 check(canvas.capture(out/"level-2.png",true),"Cannot capture level 2");
 const auto paid=encode(domain.state());
 for(auto code:{SDLK_F,SDLK_B,SDLK_S,SDLK_I})key(code);
 check(encode(domain.state())==paid&&view.tool()==Tool::Select&&ViewTestAccess::panel(view)==Panel::None,"Modal allows underlying input");
 check(!ViewTestAccess::has(view,"level-shop"),"Removed shop button is still active");
 for(const auto id:{"level-close","level-continue"}){
  const auto r=ViewTestAccess::button(view,id);
  check(r.w+.1f>=canvas.minimumTouchSize()&&r.h+.1f>=canvas.minimumTouchSize(),"Modal target is too small");
  check(r.x>=0&&r.y>=0&&r.x+r.w<=canvas.width()&&r.y+r.h<=canvas.height(),"Modal target outside viewport");
 }
 click("level-continue");
 check(ViewTestAccess::rewards(view).empty()&&encode(domain.state())==paid&&domain.state().fish.size()==state.fish.size()-1,"Continue loses a placed egg or pays twice");
 render();check(ViewTestAccess::rewards(view).empty(),"Modal reopens after Continue");
 domain.fixture("level-up");render();const auto backdropState=encode(domain.state());
 tap({10,10});tap({canvas.width()-10,canvas.height()-10});render();
 check(ViewTestAccess::rewards(view).empty()&&encode(domain.state())==backdropState&&view.tool()==Tool::Select&&ViewTestAccess::panel(view)==Panel::None,"Backdrop dismissal changes rewards or reaches underlying controls");
 domain.fixture("level-up");render();click("level-close");check(ViewTestAccess::rewards(view).empty(),"Close fails");
 domain.fixture("level-up");render();key(SDLK_ESCAPE);render();check(ViewTestAccess::rewards(view).empty(),"Escape fails");
 domain.fixture("level-up");domain.fixture("level-up-10");render();
 check(ViewTestAccess::rewards(view).size()==2,"Queued level-ups were lost");
 click("level-continue");check(ViewTestAccess::rewards(view).size()==1&&ViewTestAccess::panel(view)==Panel::None,"Continue skips queued reward screens");
 click("level-continue");check(ViewTestAccess::rewards(view).empty(),"Continue does not finish queued screens");
 // Every level-2 card goes straight to its matching shop entry. Alternate
 // mouse and touch input and confirm that browsing cannot spend currency.
 for(std::size_t i=0;i<items.size();++i){
  view.setPanel(Panel::None);domain.fixture("level-up");domain.fixture("level-up-10");render();
  const auto before=encode(domain.state());const auto& item=items[i];
  const auto target=ViewTestAccess::button(view,"level-item-"+item.id);
  check(target.w>=canvas.minimumTouchSize()&&target.h>=canvas.minimumTouchSize(),"Item link target is too small");
  if(i%2)touch("level-item-"+item.id);else click("level-item-"+item.id);
  check(ViewTestAccess::rewards(view).empty()&&ViewTestAccess::panel(view)==Panel::Shop,"Item does not open shop immediately");
  check(ViewTestAccess::category(view)==item.category&&ViewTestAccess::highlights(view,item),"Wrong category or missing highlight");
  check(ViewTestAccess::has(view,(item.category==0?"buy":"decor-buy")+item.id),"Matching shop item is not visible");
  check(encode(domain.state())==before,"Item link purchases or changes saved state");
  check(canvas.capture(out/("shop-"+item.id+".png"),true),"Cannot capture shop link");
  click("panel-close");check(!ViewTestAccess::highlights(view,item),"Highlight survives closing shop");
 }
 view.setPanel(Panel::None);domain.fixture("level-up-40");render();
 check(ViewTestAccess::has(view,"level-next"),"Overflow unlocks cannot be paged");
 check(canvas.capture(out/"level-40-page-1.png",true),"Cannot capture level 40");
 const auto beforePage=encode(domain.state());click("level-next");
 check(ViewTestAccess::page(view)==1&&ViewTestAccess::has(view,"level-prev")&&!ViewTestAccess::has(view,"level-next"),"Last unlock page is wrong");
 check(canvas.capture(out/"level-40-page-2.png",true),"Cannot capture unlock overflow");
 click("level-prev");check(ViewTestAccess::page(view)==0&&encode(domain.state())==beforePage,"Paging pays rewards or changes state");
 const auto lateItems=levelUnlocks(domain,40);
 click("level-next");const auto late=lateItems.back();click("level-item-"+late.id);
 check(ViewTestAccess::panel(view)==Panel::Shop&&ViewTestAccess::shopPage(view)>0&&ViewTestAccess::highlights(view,late),"Late unlock does not scroll to its shop page");
 check(ViewTestAccess::has(view,(late.category==0?"buy":"decor-buy")+late.id),"Late unlock page does not contain matching item");
 check(encode(domain.state())==beforePage,"Late unlock link changes save");
 check(canvas.capture(out/"shop-late-unlock.png",true),"Cannot capture late unlock");
 click("shop-tab0");check(!ViewTestAccess::highlights(view,late),"Switching categories keeps stale highlight");
 view.setPanel(Panel::None);
 const int tankLevel=content.tankLevels[1];
 auto beforeUnlock=domain.state();beforeUnlock.xp=content.levels[tankLevel-1]-1;beforeUnlock.highestRewardedLevel=tankLevel-1;testing::stage(beforeUnlock.fish.front(),4);testing::openingBalances(beforeUnlock);domain.install(beforeUnlock);
 check(bool(session.command({.action=Action::Keep,.fish=domain.state().fish.front().id})),"Cannot trigger tank level-up");render();
 const auto tankItems=levelUnlocks(domain,tankLevel);
 const auto tank=std::find_if(tankItems.begin(),tankItems.end(),[](const auto& item){return item.tank.value!=0;});
 check(tank!=tankItems.end(),"Tank unlock fixture is missing");
 while(!ViewTestAccess::has(view,"level-item-"+tank->id))click("level-next");
 const auto beforeTank=encode(domain.state());touch("level-item-"+tank->id);
 check(ViewTestAccess::panel(view)==Panel::Tanks&&ViewTestAccess::highlights(view,*tank),"Tank card does not focus its purchase row");
 check(encode(domain.state())==beforeTank,"Tank link purchases a tank");
 check(canvas.capture(out/"tank-unlock.png",true),"Cannot capture tank unlock");
 std::cout<<"PASS level-up controls, catalog, queue, paging and input isolation "<<width<<'x'<<height<<'\n';
}
}
int main(int argc,char** argv){try{if(argc!=2)return 2;for(const auto [w,h]:{std::pair{667,375},std::pair{852,393},std::pair{1024,768},std::pair{1296,784}})run(std::filesystem::absolute(argv[1]),w,h);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
