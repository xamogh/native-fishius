#include "aquarium/hud_care.hpp"
#include "fixtures.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
double distance(WorldPoint a,WorldPoint b){return std::hypot(a.x-b.x,a.y-b.y);}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
struct Input {
 Canvas& canvas;HudCare& care;int width,height;HudPointer pointer;
 void send(Uint32 type,SDL_FPoint p={},bool touch=false){
  SDL_Event e{};e.type=type;
  if(touch){e.tfinger.fingerID=1;e.tfinger.x=p.x/canvas.width();e.tfinger.y=p.y/canvas.height();}
  else{e.button.button=SDL_BUTTON_LEFT;e.button.x=p.x/canvas.width()*width;e.button.y=p.y/canvas.height()*height;}
  if(normalizeHudPointer(pointer,e,float(width),float(height)))care.event(e,pointer.touch);
 }
 void tap(SDL_FPoint p,bool touch=false){send(touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,p,touch);send(touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP,p,touch);}
 void key(SDL_Keycode key){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=key;care.event(e,false);}
};
void localMovement(const Content& content,const std::filesystem::path& assets,int width,int height){
 Canvas canvas(assets,width,height,false);canvas.previewViewport(PreviewViewport{width,height,1,{}});canvas.begin();
 Session session(content,"/tmp/sell-local-unused.json",1000,true);HudCare care(canvas,session);Input input{canvas,care,width,height,{}};
 auto state=session.domain().state();const auto seed=state.fish.front();state.fish.clear();
 const std::array positions{WorldPoint{390,310},WorldPoint{420,320},WorldPoint{690,440}};
 for(std::size_t i=0;i<positions.size();++i){auto f=seed;f.id={i+1};testing::stage(f,4);f.position=positions[i];f.lastFedAt=state.simNow;state.fish.push_back(f);}
 state.nextFishId=4;testing::openingBalances(state);session.domain().install(state);
 input.key(SDLK_S);
 for(const auto& f:session.domain().state().fish){
  const auto a=canvas.toScreen(f.position),b=canvas.toScreen(*f.motion.sellTarget);
  check(std::hypot(a.x-b.x,a.y-b.y)<canvas.minimumTouchSize()*tankArtScale+.1,"Sell sends a fish too far from its original place");
 }
 const auto& untouched=session.domain().state().fish.back();
 check(distance(untouched.position,*untouched.motion.sellTarget)<.001,"Sell moves a fish that already has enough space");
 const auto& a=session.domain().state().fish[0];const auto& b=session.domain().state().fish[1];
 check(a.motion.sellTarget->x<b.motion.sellTarget->x&&a.motion.sellTarget->y<b.motion.sellTarget->y,"Nearby fish swap places during Sell spacing");
 check(distance(*a.motion.sellTarget,*b.motion.sellTarget)>distance(a.position,b.position)+10,"Nearby fish do not make more room");
 const auto start=a.position,target=*a.motion.sellTarget;const auto id=a.id;double previousStep=0;
 for(int i=0;i<300;++i){
  const auto before=session.domain().fish(id)->position;
  session.domain().stepMovement(.02,care.tool(),care.held());care.advance(.02);
  const auto after=session.domain().fish(id)->position;const double moved=distance(before,after);
  check(moved<=1.61,"Sell movement exceeds a gentle swimming speed");
  check(distance(after,target)<=distance(before,target)+.0001,"A settling fish overshoots its destination");
  if(i==0)check(moved<.3,"Sell movement starts with a sudden burst");
  if(i>0&&i<5)check(moved>previousStep,"Sell movement does not ease in");
  previousStep=moved;
 }
 check(distance(session.domain().fish(id)->position,target)<.001&&distance(start,target)>1,"Fish do not settle after a local adjustment");
 check(distance(session.domain().state().fish.back().position,positions.back())<.001,"An isolated fish drifts during Sell");
 input.key(SDLK_ESCAPE);input.key(SDLK_S);
 for(const auto& f:session.domain().state().fish)check(distance(f.position,*f.motion.sellTarget)<5,"Reopening Sell keeps rearranging settled fish");
 std::cout<<"PASS local Sell adjustments, stable neighbors and gentle easing "<<width<<'x'<<height<<'\n';
}
void viewport(const Content& content,const std::filesystem::path& assets,const std::filesystem::path& captures,int width,int height,int count,bool clustered,Insets safe={},bool reduced=false){
 Canvas canvas(assets,width,height,false);canvas.previewViewport(PreviewViewport{width,height,1,safe});canvas.begin();
 Session session(content,"/tmp/sell-spacing-unused.json",1000,true);HudCare care(canvas,session);Input input{canvas,care,width,height,{}};
 auto base=session.domain().state();const auto seed=base.fish.front();base.fish.clear();
 base.tanks={{{1},20},{{2},10}};base.nextFishId=1;base.xp=content.levels.back();base.highestRewardedLevel=40;base.settings.reducedMotion=reduced;
 const std::array corners{WorldPoint{0,0},WorldPoint{waterWidth,0},WorldPoint{0,tankHeight},WorldPoint{waterWidth,tankHeight}};
 for(int i=0;i<count;++i){
  auto f=seed;f.id={base.nextFishId++};testing::stage(f,i==1?0:4);f.favorite=i==2;
  f.position=clustered?WorldPoint{544,320}:corners[i%corners.size()];f.motion.hasPrevious=false;f.lastFedAt=base.simNow;base.fish.push_back(f);
 }
 auto hidden=seed;hidden.id={base.nextFishId++};hidden.tank={2};base.fish.push_back(hidden);
 const auto hiddenId=hidden.id;
 auto egg=seed;egg.id={base.nextFishId++};egg.egg=true;egg.position={400,tankHeight-12};egg.hatchAt=base.simNow+6000;base.fish.push_back(egg);
 const auto eggId=egg.id;
 const FishId pearlId{base.nextFishId++},storedId{base.nextFishId++};
 auto pearl=seed;pearl.id=pearlId;pearl.species="heartfinTetra";pearl.purchase=purchaseQuote(content,*content.find(pearl.species),40);testing::stage(pearl,4);pearl.position=clustered?WorldPoint{544,320}:corners[0];pearl.lastFedAt=base.simNow;base.fish.push_back(pearl);
 auto stored=pearl;stored.id=storedId;stored.position={100,100};stored.stashed=true;base.fish.push_back(stored);
 testing::openingBalances(base);session.domain().install(base);
 const auto hud=layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
 auto capture=[&](std::string name){
  if(captures.empty())return;
  const auto folder=captures/(std::to_string(width)+"x"+std::to_string(height)+"-"+std::to_string(count)+(clustered?"-cluster":"-corners")+(reduced?"-reduced":""));
  std::filesystem::create_directories(folder);canvas.begin();canvas.scene(session.domain(),1,0,care.tool(),care.held(),false);
  paintHud(canvas,session.domain(),hud);care.paint();check(canvas.capture(folder/(name+".png")),"Cannot capture Sell spacing");
 };
 const auto initial=encode(session.domain().state());capture("before");
 input.tap(center(hud[HudPart::Rehome]),true);
 check(care.tool()==Tool::Sell,"Sell button does not activate spacing");
 check(encode(session.domain().state())==initial,"Entering Sell teleports fish or changes saved progress");
 for(int i=0;i<count;++i)check(bool(session.domain().state().fish[i].motion.sellTarget),"A visible fish has no Sell destination");
 check(session.domain().fish(pearlId)->motion.sellTarget.has_value(),"Pearl fish do not join the spacing");
 check(!session.domain().fish(hiddenId)->motion.sellTarget&&!session.domain().fish(eggId)->motion.sellTarget&&!session.domain().fish(storedId)->motion.sellTarget,"Sell spacing moves hidden fish or eggs");
 auto step=[&](int frames){for(int i=0;i<frames;++i){session.domain().stepMovement(.02,care.tool(),care.held());care.advance(.02);}};
 step(1);
 for(int i=0;i<count;++i)check(distance(session.domain().state().fish[i].position,base.fish[i].position)<=1.61,"Sell entry jumps instead of swimming smoothly");
 // Corner fixtures need time to swim out from under the HUD before a tap.
 step(149);capture("moving");
 // Freeze a moving fish for the whole press, then cancel without selling it.
 const auto heldId=base.fish.front().id;
 const auto pressPoint=canvas.toScreen(fishPose(*session.domain().fish(heldId),0).position);
 input.send(SDL_EVENT_FINGER_DOWN,pressPoint,true);check(care.held()==heldId,"A moving fish cannot be targeted");
 const auto heldPosition=session.domain().fish(heldId)->position;step(20);
 check(distance(heldPosition,session.domain().fish(heldId)->position)<.001,"A pressed fish moves away from the pointer");
 input.send(SDL_EVENT_FINGER_CANCELED,pressPoint,true);step(600);capture("settled");

 std::vector<Fish> visible;
 for(const auto& f:session.domain().state().fish)if(!f.egg&&!f.stashed&&f.tank==TankId{1})visible.push_back(f);
 const auto in=canvas.safeInsets();const float density=canvas.minimumTouchSize()/44;
 for(std::size_t i=0;i<visible.size();++i){
  const auto& f=visible[i];const auto point=canvas.toScreen(f.position);const auto size=canvas.fishSize(*content.find(f.species),f);
  check(distance(f.position,*f.motion.sellTarget)<.01,"Fish do not settle at their Sell destinations");
  check(point.x-size.x*.5f>in.left+20*density&&point.x+size.x*.5f<canvas.width()-in.right-20*density&&point.y-size.y*.5f>in.top+20*density&&point.y+size.y*.5f<canvas.height()-in.bottom-20*density,"Fish still sit against the screen edges");
  check(!hudHit(hud,point),"A Sell destination is underneath a HUD control");
  if(clustered){
   const auto origin=canvas.toScreen({544,320});
   check(std::hypot(point.x-origin.x,point.y-origin.y)<canvas.minimumTouchSize()*tankArtScale+.1,"A crowded tank triggers a full rearrangement");
  }
 }
 if(clustered){
  double separation=0;
  for(std::size_t i=0;i<visible.size();++i)for(std::size_t j=0;j<i;++j){
   const auto a=canvas.toScreen(visible[i].position),b=canvas.toScreen(visible[j].position);
   separation+=std::hypot(a.x-b.x,a.y-b.y);
  }
  check(separation/(visible.size()*(visible.size()-1)*.5)>canvas.minimumTouchSize()*.5,"Crowded fish do not spread within their local area");
 }
 const auto settled=session.domain().state();step(50);
 for(const auto& f:visible)check(distance(f.position,session.domain().fish(f.id)->position)<.001,"Settled fish drift while Sell is active");
 check(distance(session.domain().fish(hiddenId)->position,hidden.position)<.001&&distance(session.domain().fish(storedId)->position,{100,100})<.001,"Spacing changes fish outside the active tank");
 check(session.domain().state().wallet.coins==base.wallet.coins&&session.domain().state().xp==base.xp&&session.domain().state().fish[1].age==0&&session.domain().state().fish[2].favorite,"Spacing changes rewards, growth or favorites");
 const auto reloaded=decodeAndValidate(encode(session.domain().state()),content);
 for(const auto& f:reloaded.fish)check(!f.motion.sellTarget&&f.motion.sellSpeed==0,"Sell movement leaks into the save");

 // Selling one fish leaves every other destination and position stable.
 input.tap(canvas.toScreen(session.domain().fish(heldId)->position));
 check(!session.domain().fish(heldId)&&care.tool()==Tool::Sell,"A spaced fish cannot be sold with one tap");step(50);
 for(const auto& f:settled.fish)if(f.id!=heldId&&!f.egg&&f.tank==TankId{1})check(distance(session.domain().fish(f.id)->position,f.position)<.001,"Selling one fish reshuffles the others");
 input.key(SDLK_ESCAPE);const auto rest=session.domain().state().fish.front().position;step(30);
 check(care.tool()==Tool::Select&&!session.domain().state().fish.front().motion.sellTarget&&distance(rest,session.domain().state().fish.front().position)>1,"Leaving Sell does not restore normal swimming");
 input.key(SDLK_S);check(session.domain().state().fish.front().motion.sellTarget.has_value(),"The keyboard shortcut does not arrange fish");
 session.domain().advanceCare(6001);care.advance(.02);
 check(!session.domain().fish(eggId)->egg&&session.domain().fish(eggId)->motion.sellTarget.has_value(),"A newly hatched fish does not join Sell spacing");
 canvas.previewViewport(PreviewViewport{height,width,1,{}});canvas.begin();care.advance(.02);step(600);
 for(const auto& f:session.domain().state().fish)if(!f.egg&&!f.stashed&&f.tank==TankId{1}){
  const auto p=canvas.toScreen(f.position);
  check(p.x>canvas.minimumTouchSize()&&p.x<canvas.width()-canvas.minimumTouchSize(),"Resizing leaves fish at the old screen edge");
 }
 care.reset();check(!session.domain().state().fish.front().motion.sellTarget,"Opening another mode leaves Sell destinations active");
 std::cout<<"PASS soft Sell spacing, safe targets, press hold, stable sales and normal swimming "<<width<<'x'<<height<<" fish="<<count<<(clustered?" cluster":" corners")<<'\n';
}
}
int main(int argc,char** argv){try{
 const std::filesystem::path assets=argc>1?argv[1]:"assets",captures=argc>2?argv[2]:"";
 std::ifstream file(assets/"content.json");const auto content=aq::Content::fromJson(aq::Json::parse(file));
 localMovement(content,assets,1088,635);
 localMovement(content,assets,667,375);
 localMovement(content,assets,390,844);
 viewport(content,assets,captures,1088,635,10,false);
 // A full tank has 18 swimming fish, one egg and one premium adult.
 viewport(content,assets,captures,667,375,18,true);
 viewport(content,assets,captures,390,844,18,false);
 viewport(content,assets,captures,852,393,18,true,{59,0,59,21});
 viewport(content,assets,captures,617,316,10,true);
 viewport(content,assets,captures,320,240,9,false);
 viewport(content,assets,captures,1088,635,10,true,{},true);
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
