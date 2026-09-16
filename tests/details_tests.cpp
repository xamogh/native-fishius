#include "aquarium/view.hpp"
#include "fixtures.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace aq {
struct ViewTestAccess {
 static Panel panel(const View& v){return v.panel_;}
 static FishId selected(const View& v){return v.selected_;}
 static Rect bounds(const View& v){return v.panelRect_;}
 static bool has(const View& v,std::string_view id){return std::any_of(v.buttons_.begin(),v.buttons_.end(),[&](const auto& b){return b.id==id;});}
 static Rect button(const View& v,std::string_view id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing button: "+std::string(id));}
};
}
namespace {
using namespace aq;
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void run(const std::filesystem::path& assets,int width,int height){
 std::ifstream file(assets/"content.json");Session session(Content::fromJson(Json::parse(file)),"/tmp/aquarium-details-unused.json",0,true);
 auto& d=session.domain();const auto bought=d.execute({.action=Action::Buy,.key="neonTetra",.point={490,330}});check(bool(bought),"Cannot create purchased fish");const auto id=bought.fish;
 auto state=d.state();std::erase_if(state.fish,[&](const auto& f){return f.id!=id;});state.simNow=60000;testing::stage(state.fish.front(),1);state.fish.front().lastFedAt=state.simNow-state.fish.front().purchase.feedMs;state.settings.reducedMotion=true;d.install(state);
 const auto& species=*d.content().find("neonTetra");const auto info=fishDetailsContent(species,*d.fish(id),state.simNow);
 check(info.stage==1&&info.progress==.25f&&info.care=="Hungry · Paused"&&info.reward.coins()==4&&info.reward.xp==0&&info.adultReward.coins()==11&&info.adultReward.xp==2,"Growth and reward details differ from the snapshot");
 Canvas canvas(assets,width,height,true);View view(canvas,session);double time=1;
 const auto render=[&]{view.render(time+=.7);};render();
 const auto tap=[&](SDL_FPoint p){for(auto type:{SDL_EVENT_FINGER_DOWN,SDL_EVENT_FINGER_UP}){SDL_Event e{};e.type=type;e.tfinger.touchID=1;e.tfinger.fingerID=1;e.tfinger.x=p.x/canvas.width();e.tfinger.y=p.y/canvas.height();view.event(e,time);}render();};
 const auto click=[&](std::string_view key){const auto r=ViewTestAccess::button(view,key);tap({r.x+r.w/2,r.y+r.h/2});};
 const auto out=std::filesystem::absolute(assets).parent_path()/"evidence/fish-popover-match";std::filesystem::create_directories(out);
 const auto capture=[&](std::string name){check(canvas.capture(out/(std::to_string(width)+"x"+std::to_string(height)+"-"+name+".png"),true),"Cannot capture popover");};
 const auto initial=encode(d.state());tap(canvas.toScreen(d.fish(id)->position));
 check(ViewTestAccess::panel(view)==Panel::Details&&ViewTestAccess::selected(view)==id&&encode(d.state())==initial,"Selecting fish changed its state");
 const auto bounds=ViewTestAccess::bounds(view);check(bounds.x>=0&&bounds.y>=0&&bounds.x+bounds.w<=canvas.width()&&bounds.y+bounds.h<=canvas.height(),"Popover overflows viewport");
 check(!bounds.has(canvas.toScreen(d.fish(id)->position).x,canvas.toScreen(d.fish(id)->position).y),"Popover covers its fish");
 check(!ViewTestAccess::has(view,"one-feed")&&!ViewTestAccess::has(view,"one-revive")&&ViewTestAccess::has(view,"tool-food"),"Popover retained removed care actions");
 capture("hungry");
 for(const auto position:{WorldPoint{80,90},WorldPoint{980,90},WorldPoint{205,500},WorldPoint{870,500}}){
  click("details-close");auto moved=state;moved.fish.front().position=position;moved.fish.front().motion.previous=position;d.install(moved);render();const auto at=canvas.toScreen(position);tap(at);
  check(ViewTestAccess::panel(view)==Panel::Details&&ViewTestAccess::selected(view)==id,"Edge fish cannot open its popover");
  const auto p=ViewTestAccess::bounds(view);check(p.x>=0&&p.y>=0&&p.x+p.w<=canvas.width()&&p.y+p.h<=canvas.height()&&!p.has(at.x,at.y),"Edge popover overlaps its fish or screen boundary");
  capture("edge-"+std::to_string(int(position.x))+"-"+std::to_string(int(position.y)));
 }
 click("details-close");d.install(state);render();tap(canvas.toScreen(d.fish(id)->position));
 click("fish-rehome");check(d.fish(id)&&ViewTestAccess::has(view,"rehome-confirm")&&ViewTestAccess::has(view,"rehome-back"),"Rehome needs an inline confirmation");capture("confirm");click("rehome-back");check(encode(d.state())==initial,"Cancelling rehome changed state");
 check(bool(d.execute({.action=Action::Feed,.fish=id})),"Cannot feed");render();check(fishDetailsContent(species,*d.fish(id),d.state().simNow).care=="Fed · Growing","Popover did not refresh after feeding");capture("fed");
 click("fish-favorite");capture("favorite");click("fish-rehome");check(!ViewTestAccess::has(view,"rehome-confirm"),"Favorite protection failed");click("fish-favorite");
 // If growth changes the amount while confirming, the first new tap only
 // refreshes the quote. A second deliberate tap confirms that displayed value.
 click("fish-rehome");auto grown=d.state();testing::stage(grown.fish.front(),4);d.install(grown);render();capture("adult-confirm");click("rehome-confirm");check(d.fish(id),"A changed reward was confirmed without review");click("rehome-back");capture("adult");
 const auto before=d.state().wallet.coins;click("fish-keep");check(!d.fish(id)&&d.companion(id)&&d.state().wallet.coins==before+11&&d.state().xp==2,"Keep did not pay exactly once");
 check(ViewTestAccess::panel(view)==Panel::Details&&ViewTestAccess::has(view,"fish-store")&&!ViewTestAccess::has(view,"fish-rehome"),"Keep did not switch to display details");capture("display");
 check(encode(decodeAndValidate(encode(d.state()),d.content()))==encode(d.state()),"Kept fish save is invalid");click("fish-store");check(d.companion(id)->stored&&ViewTestAccess::panel(view)==Panel::None,"Store did not close display details");
 // Rehome itself removes the growing fish and pays the same adult amount.
 const auto another=d.execute({.action=Action::Buy,.key="neonTetra",.point={490,330}});check(bool(another),"Cannot create rehome sample");grown=d.state();testing::stage(grown.fish.back(),4);d.install(grown);render();tap(canvas.toScreen(d.fish(another.fish)->position));const auto old=d.state().wallet.coins;click("fish-rehome");click("rehome-confirm");check(!d.fish(another.fish)&&d.state().wallet.coins==old+11,"Rehome payout differs from Keep");
 std::cout<<"PASS compact popover, live growth, quote review, favorite, Keep, Rehome and display storage "<<width<<'x'<<height<<'\n';
}
}
int main(int argc,char** argv){try{if(argc!=2)return 2;for(auto [w,h]:{std::pair{667,375},std::pair{852,393},std::pair{1024,768},std::pair{1210,834},std::pair{1338,1002}})run(argv[1],w,h);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
