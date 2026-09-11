#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
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
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void run(const std::filesystem::path& assets,int width,int height){
 using namespace aq;
 std::ifstream file(assets/"content.json");Session session(Content::fromJson(Json::parse(file)),"/tmp/aquarium-details-test-unused.json",0,true);
 auto& d=session.domain();const auto* species=d.content().find("molly");check(species,"Molly missing");
 auto state=d.state();auto original=std::find_if(state.fish.begin(),state.fish.end(),[](const auto& f){return f.species=="molly";});const auto id=original->id;
 state.simNow=60000;d.install(state);const auto initial=encode(d.state());
 const auto first=fishDetailsContent(*species,*d.fish(id),d.state().simNow);
 check(first.title=="BABY FISH"&&first.stage==0&&first.progress==0,"Baby stage presentation is wrong");
 check(first.care=="HUNGRY - FEED WITHIN 11H 59M","Molly countdown does not use its live workbook interval");
 check(first.growth=="GROWTH PAUSED - FEED TO RESUME"&&first.sale=="FIRST SALE UNLOCKS AT JUNIOR","Baby guidance is wrong");
 Canvas canvas(assets,width,height,false);View view(canvas,session);double time=1;
 // These tests check settled controls. Animation timing has its own suite.
 auto render=[&]{view.render(time+=.7);canvas.present();};render();
 auto finger=[&](Uint32 type,float x,float y){int w{},h{};SDL_GetWindowSize(canvas.window(),&w,&h);const float fit=std::min(float(w)/canvas.width(),float(h)/canvas.height());SDL_Event event{};event.type=type;event.tfinger.touchID=1;event.tfinger.fingerID=1;event.tfinger.x=((w-canvas.width()*fit)*.5f+x*fit)/w;event.tfinger.y=((h-canvas.height()*fit)*.5f+y*fit)/h;view.event(event,time);};
 auto tap=[&](float x,float y){finger(SDL_EVENT_FINGER_DOWN,x,y);finger(SDL_EVENT_FINGER_UP,x,y);render();};
 const auto originalWidth=canvas.width();const auto at=canvas.toScreen(d.fish(id)->position);tap(at.x,at.y);
 check(ViewTestAccess::panel(view)==Panel::Details&&ViewTestAccess::selected(view)==id,"Selecting a fish did not open its details");
 check(encode(d.state())==initial,"Selecting a fish mutated its care or position");
 const auto p=ViewTestAccess::bounds(view);
 check(canvas.width()==originalWidth,"Opening a popover changed the aquarium viewport");
 check(p.w<=580&&p.h<=467&&!p.has(at.x,at.y),"Details is not a compact popover beside the real fish");
 check(p.x>=0&&p.y>=0&&p.x+p.w<=canvas.width()+1&&p.y+p.h<=canvas.height()+1,"Details frame overflows viewport");
 for(const char* button:{"nav0","nav1","tool-select","tool-food","nav3","tool-sell","details-close"})check(ViewTestAccess::has(view,button),"Aquarium controls are hidden by the popover");
 tap(p.x+p.w*.65f,p.y+p.h*.34f);
 check(ViewTestAccess::panel(view)==Panel::Details&&encode(d.state())==initial,"Card text triggers an unintended action");
 const auto out=std::filesystem::absolute(assets).parent_path()/"evidence/fish-popover-reference";std::filesystem::create_directories(out);
 auto capture=[&](std::string name){view.render(time+=.02);check(canvas.capture(out/(std::to_string(width)+"x"+std::to_string(height)+"-"+name+".png"),true),"Cannot capture details");canvas.present();};
 capture("baby");
 check(bool(d.execute({Action::Feed,id})),"Cannot feed test fish");render();
 check(fishDetailsContent(*species,*d.fish(id),d.state().simNow).condition==Care::Fed,"Open card did not reflect feeding");
 for(const std::string name:{"junior","adult","sick","egg","dead"}){
  auto change=d.state();auto& fish=*std::find_if(change.fish.begin(),change.fish.end(),[&](const auto& f){return f.id==id;});fish.egg=false;fish.dead=false;fish.age=0;fish.growthMs=0;fish.lastFedAt=change.simNow;
  if(name=="junior"){fish.age=1;fish.growthMs=species->stageMs/2;}
  if(name=="adult")fish.age=4;
  if(name=="sick")fish.lastFedAt=change.simNow-species->feedMs;
  if(name=="egg"){fish.egg=true;fish.hatchAt=change.simNow+6000;}
  if(name=="dead"){fish.dead=true;change.wallet.pearls=1;}
  d.install(std::move(change));render();const auto content=fishDetailsContent(*species,*d.fish(id),d.state().simNow);
  if(name=="junior")check(content.stage==1&&std::abs(content.progress-.5f)<.001f&&content.sale.find("SELL VALUE:")==0,"Junior stage or live sale value is wrong");
  if(name=="adult")check(content.stage==4&&content.progress==1&&content.growth=="FULLY GROWN","Adult details are wrong");
  if(name=="sick")check(content.care=="SICK - FEED TO RECOVER","Sick details are wrong");
  if(name=="egg")check(content.title=="FISH EGG"&&content.stage==-1&&content.care=="HATCHES IN 6S","Egg details are wrong");
  if(name=="dead")check(ViewTestAccess::has(view,"one-revive")&&ViewTestAccess::has(view,"one-remove"),"Dead fish lost recovery controls");
  capture(name);
 }
 auto revive=ViewTestAccess::button(view,"one-revive");tap(revive.x+revive.w/2,revive.y+revive.h/2);
 check(!d.fish(id)->dead&&d.state().wallet.pearls==0&&!ViewTestAccess::has(view,"one-revive"),"Revival does not refresh the card");
 const auto unchanged=encode(d.state());tap(canvas.width()*.5f,canvas.height()-10);
 check(ViewTestAccess::panel(view)==Panel::None&&encode(d.state())==unchanged,"Backdrop did not dismiss without a second action");
 check(view.tool()==Tool::Select,"Backdrop changed the active tool");
 // The card must re-anchor at the edges without covering its selected fish.
 for(auto point:{WorldPoint{80,90},WorldPoint{980,90},WorldPoint{80,500},WorldPoint{980,500},WorldPoint{544,320}}){
  auto change=d.state();auto& fish=*std::find_if(change.fish.begin(),change.fish.end(),[&](const auto& f){return f.id==id;});fish.position=point;fish.motion.previous=point;d.install(std::move(change));render();
  const auto position=canvas.toScreen(point);tap(position.x,position.y);
  const auto bounds=ViewTestAccess::bounds(view);
  check(ViewTestAccess::panel(view)==Panel::Details&&ViewTestAccess::selected(view)==id,"Edge fish did not open its popover");
  check(bounds.x>=0&&bounds.y>=0&&bounds.x+bounds.w<=canvas.width()&&bounds.y+bounds.h<=canvas.height(),"Edge popover is clipped");
  check(!bounds.has(position.x,position.y),"Popover covers the selected fish");
  capture("edge-"+std::to_string(int(point.x))+"-"+std::to_string(int(point.y)));
  tap(canvas.width()*.5f,canvas.height()-10);
 }
 auto current=canvas.toScreen(d.fish(id)->position);tap(current.x,current.y);
 auto change=d.state();auto& other=*std::find_if(change.fish.begin(),change.fish.end(),[&](const auto& f){return f.id!=id;});const auto otherId=other.id;other.position={940,145};other.motion.previous=other.position;d.install(std::move(change));render();
 const auto otherAt=canvas.toScreen(d.fish(otherId)->position);tap(otherAt.x,otherAt.y);
 check(ViewTestAccess::selected(view)==otherId&&ViewTestAccess::panel(view)==Panel::Details,"A single tap did not switch to another fish");
 const auto food=ViewTestAccess::button(view,"tool-food");tap(food.x+food.w*.5f,food.y+food.h*.5f);
 check(view.tool()==Tool::Food&&ViewTestAccess::panel(view)==Panel::None,"Popover blocks aquarium tools");
 const auto select=ViewTestAccess::button(view,"tool-select");tap(select.x+select.w*.5f,select.y+select.h*.5f);
 current=canvas.toScreen(d.fish(id)->position);tap(current.x,current.y);const auto close=ViewTestAccess::button(view,"details-close");tap(close.x+close.w*.5f,close.y+close.h*.5f);
 check(ViewTestAccess::panel(view)==Panel::None,"Popover close button failed");
 std::cout<<"PASS "<<width<<'x'<<height<<": anchored popover, live states, screen edges, switching fish, tools, close and outside tap\n";
}
}
int main(int argc,char** argv){try{if(argc!=2)throw std::runtime_error("Usage: aquarium_details_tests ASSETS");for(auto [w,h]:{std::pair{669,506},std::pair{804,415},std::pair{852,393},std::pair{667,375},std::pair{1024,768},std::pair{1210,834}})run(argv[1],w,h);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
