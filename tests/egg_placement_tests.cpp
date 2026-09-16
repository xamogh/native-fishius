#include "fixtures.hpp"
#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static Rect button(const View& v,std::string_view id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing button: "+std::string(id));}
 static bool has(const View& v,std::string_view id){return std::any_of(v.buttons_.begin(),v.buttons_.end(),[&](const auto& b){return b.id==id;});}
 static void arm(View& v,std::string id){v.pointerMove(0,0);v.armBuy(std::move(id));}
 static Panel panel(const View& v){return v.panel_;}
 static CursorKind cursor(const Canvas& c){return c.cursorKind_;}
 static bool funds(const View& v){return v.fundsDialog_.has_value();}
};
}
namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool near(double a,double b){return std::abs(a-b)<.01;}
void run(const Content& content,const std::filesystem::path& assets,int width,int height){
 Canvas canvas(assets,width,height,true);Session session(content,"/tmp/egg-placement-unused.json",1000,true);auto& domain=session.domain();
 auto state=domain.state();state.xp=content.levels.back();state.highestRewardedLevel=40;state.tutorialStep=11;state.settings.reducedMotion=true;testing::openingBalances(state);
 domain.install(state);View view(canvas,session);double time=1;
 const auto render=[&]{view.render(time+=.8);};
 const auto mouse=[&](Uint32 type,SDL_FPoint point,Uint8 button=SDL_BUTTON_LEFT){
  int w{},h{};SDL_GetWindowSize(canvas.window(),&w,&h);const float scale=std::min(w/canvas.width(),h/canvas.height());
  SDL_Event e{};e.type=type;
  if(type==SDL_EVENT_MOUSE_MOTION){e.motion.x=(w-canvas.width()*scale)*.5f+point.x*scale;e.motion.y=(h-canvas.height()*scale)*.5f+point.y*scale;}
  else{e.button.button=button;e.button.x=(w-canvas.width()*scale)*.5f+point.x*scale;e.button.y=(h-canvas.height()*scale)*.5f+point.y*scale;}
  view.event(e,time);
 };
 const auto tap=[&](SDL_FPoint p){mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,p);mouse(SDL_EVENT_MOUSE_BUTTON_UP,p);};
 const auto center=[&](std::string_view id){const auto r=ViewTestAccess::button(view,id);return SDL_FPoint{r.x+r.w*.5f,r.y+r.h*.5f};};
 const auto click=[&](std::string_view id){tap(center(id));render();};
 const auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=code;view.event(e,time);render();};
 const auto out=assets.parent_path()/"evidence/egg-placement"/(std::to_string(width)+"x"+std::to_string(height));std::filesystem::create_directories(out);
 render();click("nav1");const auto armPoint=center("buyguppy");const auto before=encode(domain.state());
 tap(armPoint);render();
 check(view.tool()==Tool::Buy&&ViewTestAccess::panel(view)==Panel::None&&encode(domain.state())==before,"Shop Buy must arm placement without changing the save");
 tap(armPoint);render();check(encode(domain.state())==before,"Double-clicking Buy purchased an egg at the arming point");
 const SDL_FPoint jitter{armPoint.x+canvas.minimumTouchSize()*12/44,armPoint.y};mouse(SDL_EVENT_MOUSE_MOTION,jitter);tap(jitter);render();
 check(encode(domain.state())==before,"Pointer jitter within the arming dead zone purchased an egg");
 const auto water=canvas.toScreen({540,360});mouse(SDL_EVENT_MOUSE_MOTION,water);render();
 check(ViewTestAccess::cursor(canvas)==CursorKind::Hidden,"Egg ghost did not replace the mouse cursor");
 check(!ViewTestAccess::has(view,"resume-eggs"),"The queued egg button still exists");
 check(canvas.capture(out/"ready.png",true),"Cannot capture egg placement");
 mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,water,SDL_BUTTON_RIGHT);mouse(SDL_EVENT_MOUSE_BUTTON_UP,water,SDL_BUTTON_RIGHT);
 check(encode(domain.state())==before,"A right-click purchased an egg");
 mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,water);
 check(domain.state().fish.size()==state.fish.size()+1&&domain.state().wallet.coins==state.wallet.coins-purchaseQuote(content,*content.find("guppy"),40).principal,"The primary press did not buy exactly one egg");
 const auto first=domain.state().fish.back();check(first.egg&&near(first.position.x,540)&&near(first.position.y,360)&&first.hatchAt==state.simNow+6000,"Egg was not placed at the tap with a fresh hatch timer");
 mouse(SDL_EVENT_MOUSE_MOTION,canvas.toScreen({600,300}));mouse(SDL_EVENT_MOUSE_BUTTON_UP,water);render();
 check(domain.state().fish.size()==state.fish.size()+1&&view.tool()==Tool::Buy,"Dragging or releasing bought another egg or ended placement");
 tap(water);render();check(domain.state().fish.size()==state.fish.size()+2&&domain.state().wallet.coins==state.wallet.coins-2*purchaseQuote(content,*content.find("guppy"),40).principal,"Repeated taps did not buy one egg each");
 check(canvas.capture(out/"placed.png",true),"Cannot capture repeated placement");
 // Placement accepts the lower tank, including the navigation area it owns.
 const auto floorTap=canvas.toScreen({544,590});const auto floorPoint=canvas.toWorld(floorTap.x,floorTap.y);
 tap(floorTap);render();check(ViewTestAccess::panel(view)==Panel::None&&view.tool()==Tool::Buy&&domain.state().fish.size()==state.fish.size()+3,"Shop button leaked through the placement layer");
 check(floorPoint.y>512&&near(domain.state().fish.back().position.y,floorPoint.y),"A lower tap moved the egg away from the pointer");
 const auto paid=encode(domain.state());mouse(SDL_EVENT_MOUSE_MOTION,center("done"));render();
 check(ViewTestAccess::cursor(canvas)==CursorKind::Arrow,"Done must show the normal cursor");
 const auto done=ViewTestAccess::button(view,"done");check(done.w>=canvas.minimumTouchSize()&&done.h>=canvas.minimumTouchSize(),"Done is too small for touch");
 click("done");check(view.tool()==Tool::Select&&encode(domain.state())==paid&&!ViewTestAccess::has(view,"resume-eggs"),"Done spent money or retained a queue");
 check(canvas.capture(out/"done.png",true),"Cannot capture finished placement");
 check(encode(decodeAndValidate(paid,content))==paid&&!paid.contains("pendingEggs"),"Placed eggs do not round-trip without a queue");
 domain.advanceCare(5999);check(domain.fish(first.id)->egg,"Egg hatched too early");domain.advanceCare(1);check(!domain.fish(first.id)->egg,"Egg did not hatch after six seconds");

 domain.install(state);ViewTestAccess::arm(view,"guppy");render();key(SDLK_ESCAPE);check(encode(domain.state())==before&&view.tool()==Tool::Select,"Cancelling before a drop changed progress");
 ViewTestAccess::arm(view,"guppy");render();key(SDLK_F);check(view.tool()==Tool::Food&&encode(domain.state())==before,"Switching tools spent money");
 key(SDLK_B);check(view.tool()==Tool::Select,"Opening Shop left placement active");view.setPanel(Panel::None);render();
 auto full=state;while(full.fish.size()<10){auto f=full.fish.front();f.id={full.nextFishId++};full.fish.push_back(f);}domain.install(full);
 ViewTestAccess::arm(view,"guppy");check(view.tool()==Tool::Select&&encode(domain.state())==encode(full),"Full tank allowed placement to start");
 auto lastSlot=full;lastSlot.fish.pop_back();domain.install(lastSlot);ViewTestAccess::arm(view,"guppy");render();
 mouse(SDL_EVENT_MOUSE_MOTION,water);tap(water);render();const auto filled=encode(domain.state());tap(water);render();
 check(view.tool()==Tool::Select&&encode(domain.state())==filled,"A failed full-tank purchase charged or kept placing");
 auto poor=state;poor.wallet.coins=purchaseQuote(content,*content.find("guppy"),40).principal;domain.install(poor);ViewTestAccess::arm(view,"guppy");render();mouse(SDL_EVENT_MOUSE_MOTION,water);tap(water);render();
 const auto spent=encode(domain.state());tap(water);render();check(ViewTestAccess::funds(view)&&view.tool()==Tool::Select&&encode(domain.state())==spent,"Running out of coins did not stop safely");key(SDLK_ESCAPE);
 auto premium=state;domain.install(premium);ViewTestAccess::arm(view,"bubbleEyeGoldfish");render();mouse(SDL_EVENT_MOUSE_MOTION,water);tap(water);render();
 check(domain.state().wallet.pearls==premium.wallet.pearls&&domain.state().wallet.coins==premium.wallet.coins&&domain.state().companions.size()==1&&view.tool()==Tool::Select,"Free Bubble Eye did not claim one permanent companion");key(SDLK_ESCAPE);

 // Only the first finger may spend. Synthetic mouse events must not duplicate it.
 domain.install(state);ViewTestAccess::arm(view,"guppy");render();mouse(SDL_EVENT_MOUSE_MOTION,water);
 const auto finger=[&](Uint32 type,SDL_FingerID id){SDL_Event e{};e.type=type;e.tfinger.touchID=1;e.tfinger.fingerID=id;e.tfinger.x=water.x/canvas.width();e.tfinger.y=water.y/canvas.height();view.event(e,time);};
 finger(SDL_EVENT_FINGER_DOWN,1);finger(SDL_EVENT_FINGER_DOWN,2);finger(SDL_EVENT_FINGER_UP,2);
 SDL_Event synthetic{};synthetic.type=SDL_EVENT_MOUSE_BUTTON_DOWN;synthetic.button.which=SDL_TOUCH_MOUSEID;synthetic.button.button=SDL_BUTTON_LEFT;view.event(synthetic,time);
 finger(SDL_EVENT_FINGER_UP,1);render();
 check(domain.state().fish.size()==state.fish.size()+1,"Multitouch or synthetic mouse events duplicated a purchase");
 finger(SDL_EVENT_FINGER_DOWN,1);finger(SDL_EVENT_FINGER_UP,1);render();check(domain.state().fish.size()==state.fish.size()+2,"Repeated touch taps did not place eggs");key(SDLK_ESCAPE);
 auto grown=domain.state();const auto recovered=grown.fish.back().id;testing::stage(grown.fish.back(),4);domain.install(grown);check(bool(domain.execute({.action=Action::Keep,.fish=recovered})),"Cannot keep test adult");check(bool(domain.execute({.action=Action::Stash,.fish=recovered})),"Cannot store companion");const auto stored=domain.state().wallet;
 click("inventory");click("inventory-fish");click("restore"+std::to_string(recovered.value));mouse(SDL_EVENT_MOUSE_MOTION,water);tap(water);render();
 check(!domain.companion(recovered)->stored&&domain.state().wallet.coins==stored.coins&&view.tool()==Tool::Select,"Bag did not restore a companion for free");
 std::cout<<"PASS egg placement, input ownership, cancellation, funds and capacity "<<width<<'x'<<height<<'\n';
}

}
int main(int argc,char** argv){try{if(argc!=2)return 2;const auto assets=std::filesystem::absolute(argv[1]);std::ifstream in(assets/"content.json");const auto content=aq::Content::fromJson(aq::Json::parse(in));for(auto [w,h]:{std::pair{667,375},std::pair{852,393},std::pair{1024,768}})run(content,assets,w,h);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
