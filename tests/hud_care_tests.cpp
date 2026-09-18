#include "aquarium/hud_care.hpp"
#include "aquarium/hud_placement.hpp"
#include "fixtures.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
struct Input {
 Canvas& canvas;HudCare& care;int width,height;HudPointer pointer;
 bool send(SDL_Event e,bool blocked=false){return normalizeHudPointer(pointer,e,float(width),float(height))&&care.event(e,pointer.touch,blocked);}
 void mouse(Uint32 type,SDL_FPoint p,bool blocked=false,Uint8 button=SDL_BUTTON_LEFT,SDL_MouseID which=0){
  SDL_Event e{};e.type=type;
  if(type==SDL_EVENT_MOUSE_MOTION){e.motion.x=p.x/canvas.width()*width;e.motion.y=p.y/canvas.height()*height;e.motion.which=which;}
  else{e.button.x=p.x/canvas.width()*width;e.button.y=p.y/canvas.height()*height;e.button.button=button;e.button.which=which;}
  send(e,blocked);
 }
 void finger(Uint32 type,SDL_FPoint p,SDL_FingerID id=1,bool blocked=false){SDL_Event e{};e.type=type;e.tfinger.fingerID=id;e.tfinger.x=p.x/canvas.width();e.tfinger.y=p.y/canvas.height();send(e,blocked);}
 void tap(SDL_FPoint p,bool touch=false,bool blocked=false){if(touch){finger(SDL_EVENT_FINGER_DOWN,p,1,blocked);finger(SDL_EVENT_FINGER_UP,p,1,blocked);}else{mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,p,blocked);mouse(SDL_EVENT_MOUSE_BUTTON_UP,p,blocked);}}
 void click(Rect r,bool touch=false){tap(center(r),touch);}
 void key(SDL_Keycode key,bool repeat=false){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=key;e.key.repeat=repeat;send(e);}
};
State fishState(const Content& content,int age=0){
 Domain domain(content,1000);
 const auto bought=domain.execute({.action=Action::Buy,.key="neonTetra",.point={544,320}});
 check(bool(bought),"Test fish purchase failed");
 auto state=domain.state();const Fish fish=*domain.fish(bought.fish);state.fish={fish};
 testing::stage(state.fish.front(),age);state.fish.front().lastFedAt=state.simNow-state.fish.front().purchase.feedMs;
 return state;
}
void viewport(const Content& content,const std::filesystem::path& assets,int width,int height,const std::filesystem::path& captures){
 Canvas canvas(assets,width,height,true);Session session(content,"/tmp/hud-care-unused.json",1000,true);HudCare care(canvas,session);Input input{canvas,care,width,height,{}};
 const auto hud=layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
 const auto detail=[&](){return care.detailsLayout();};
 const auto water=canvas.toScreen({544,320});const auto food=hud[HudPart::Food],net=hud[HudPart::Rehome];
 const State base=fishState(content);const auto id=base.fish.front().id;
 auto reset=[&](State state){care.reset();input.pointer={};session.domain().install(std::move(state));care.advance(3);};
 auto open=[&](bool touch=false){input.tap(water,touch);check(care.detailsOpen()&&care.selected()==id,"Fish tap did not open its details");};
 auto capture=[&](std::string name){
  if(captures.empty())return;
  canvas.begin();canvas.scene(session.domain(),1,0,care.tool(),care.held(),false);const auto rewards=care.rewards().display(session.domain());paintHud(canvas,session.domain(),hud,{},{},&rewards);care.paint();care.rewards().paint(canvas,hud);
  const auto folder=captures/(std::to_string(width)+"x"+std::to_string(height));std::filesystem::create_directories(folder);
  check(canvas.capture(folder/(name+".png")),"Care screenshot failed");
 };
 for(bool touch:{false,true}){
  reset(base);const auto walletBefore=session.domain().state().wallet;
  input.click(food,touch);check(care.tool()==Tool::Food&&session.domain().pellets().empty(),"Food activation drops a pellet or fails to toggle");
  input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water);check(session.domain().pellets().empty(),"An unmatched release drops food");
  input.tap(water,touch);check(session.domain().pellets().size()==1&&session.domain().state().wallet.coins==walletBefore.coins&&session.domain().state().wallet.pearls==walletBefore.pearls&&session.domain().state().xp==base.xp,"Feeding charges money or drops more than one pellet");
  care.advance(.1);capture(touch?"feeding-touch":"feeding");
  input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,water,false,SDL_BUTTON_RIGHT);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water,false,SDL_BUTTON_RIGHT);
  check(session.domain().pellets().size()==1,"Right click drops food");
  input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,water);input.mouse(SDL_EVENT_MOUSE_MOTION,{water.x+100,water.y});input.mouse(SDL_EVENT_MOUSE_MOTION,water);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water);
  check(session.domain().pellets().size()==1,"Dragging away and back drops food");
  input.click(food,touch);check(care.tool()==Tool::Select,"Food does not toggle off");
  input.key(SDLK_F);input.key(SDLK_F,true);check(care.tool()==Tool::Food,"Key repeat toggles feeding");input.key(SDLK_ESCAPE);check(care.tool()==Tool::Select,"Escape does not stop feeding");
  for(auto part:{HudPart::Shop,HudPart::Tank,HudPart::Bag,HudPart::Projects,HudPart::Rewards,HudPart::Settings,HudPart::CoinPlus,HudPart::PearlPlus}){
   input.click(food,touch);input.click(hud[part],touch);check(care.tool()==Tool::Select,"A menu keeps Food active");
   input.click(net,touch);input.click(hud[part],touch);check(care.tool()==Tool::Select,"A menu keeps Rehome active");
  }
  input.click(food,touch);input.tap(water,touch,true);check(care.tool()==Tool::Select&&session.domain().pellets().size()==1,"Modal input feeds a fish underneath");
  // Before Junior, neither the net nor the details action can settle a fish.
  for(bool egg:{false,true}){
   auto young=base;young.fish.front().egg=egg;reset(young);const auto unchanged=encode(session.domain().state());
   input.click(net,touch);capture(egg?"egg-growing":"baby-growing");input.tap(water,touch);
   check(care.tool()==Tool::Sell&&!care.detailsOpen()&&encode(session.domain().state())==unchanged,"Net sold an egg or Baby");
   input.key(SDLK_ESCAPE);open(touch);input.click(detail().secondary,touch);input.click(detail().secondary,touch);
   check(care.detailsOpen()&&encode(session.domain().state())==unchanged,"Details sold an egg or Baby");
   capture(egg?"egg-popover":"baby-popover");
  }
  auto junior=base;testing::stage(junior.fish.front(),1);reset(junior);
  input.click(net,touch);input.tap(water,touch);check(!session.domain().fish(id)&&care.tool()==Tool::Sell,"Junior cannot be sold with one tap");
  auto fed=junior;fed.fish.front().lastFedAt=fed.simNow;reset(fed);open(touch);capture("growing-fish");
  reset(junior);
  // Both modal close gestures and selling buttons require a complete, stationary tap.
  open(touch);const auto before=encode(session.domain().state());capture("hungry-fish");
  input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,center(detail().secondary));input.mouse(SDL_EVENT_MOUSE_MOTION,water);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,center(detail().secondary));
  check(care.detailsOpen()&&encode(session.domain().state())==before,"Dragging the Sell button sells a fish");
  input.finger(SDL_EVENT_FINGER_DOWN,center(detail().secondary));input.finger(SDL_EVENT_FINGER_CANCELED,center(detail().secondary));input.finger(SDL_EVENT_FINGER_UP,center(detail().secondary));
  check(care.detailsOpen()&&encode(session.domain().state())==before,"Cancelled Sell touch sells a fish");
  input.key(SDLK_ESCAPE);check(!care.detailsOpen()&&encode(session.domain().state())==before,"Escape sells a fish");
  open(touch);input.click(detail().favorite,touch);check(session.domain().fish(id)->favorite,"Favorite is not saved");
  capture("favorite-popover");
  input.click(detail().secondary,touch);check(care.detailsOpen()&&session.domain().fish(id),"Favorite can be sold");
  input.click(detail().favorite,touch);input.click(detail().primary,touch);check(care.detailsOpen()&&care.tool()==Tool::Select,"Keep must stay disabled before adulthood");
  const auto saleButton=detail().secondary;const auto saleReward=fishReward(*session.domain().fish(id));
  const auto saleBalance=session.domain().state().wallet.coins,saleXp=session.domain().state().xp;
  input.click(saleButton,touch);
  check(!care.detailsOpen()&&!session.domain().fish(id)&&session.domain().state().wallet.coins==saleBalance+saleReward.coins()&&session.domain().state().xp==saleXp+saleReward.xp,"Sell button does not settle the displayed reward in one tap");
  const auto saleState=encode(session.domain().state());input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,center(saleButton));
  check(encode(session.domain().state())==saleState,"Repeated Sell release pays twice");
  care.advance(.1);check(care.rewards().active()&&care.rewards().display(session.domain()).coins==saleBalance,"Sell button did not start a reward flight");capture("sell-reward");
  reset(base);open(touch);input.tap({0,0},touch);check(!care.detailsOpen()&&session.domain().fish(id),"Backdrop does not close safely");
  auto adult=base;testing::stage(adult.fish.front(),4);reset(adult);
  input.click(net,touch);check(care.tool()==Tool::Sell,"Net does not activate");capture(touch?"quick-sell-touch":"quick-sell");
  input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,water);input.mouse(SDL_EVENT_MOUSE_MOTION,{water.x+100,water.y});input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water);
  check(session.domain().fish(id),"Dragging sells a fish");
  const auto expected=fishReward(adult.fish.front());
  const auto balance=session.domain().state().wallet.coins;input.tap(water,touch);
  check(care.tool()==Tool::Sell,"Quick sale exits the sell tool");
  check(!session.domain().fish(id)&&!care.detailsOpen()&&session.domain().state().wallet.coins==balance+expected.coins()&&session.domain().state().xp==expected.xp,"Confirmed rehome pays the wrong reward");
  const auto settled=encode(session.domain().state());input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,center(detail().secondary));
  check(encode(session.domain().state())==settled,"Repeated release pays twice");
  care.advance(.1);check(care.rewards().active()&&care.rewards().display(session.domain()).coins==balance,"Quick rehome did not start a reward flight");capture("rehome-reward");decodeAndValidate(settled,content);
  auto favorite=adult;favorite.fish.front().favorite=true;reset(favorite);input.click(net,touch);capture("favorite-locked");input.tap(water,touch);
  check(session.domain().fish(id)&&!care.detailsOpen(),"Quick sale removes a favorite or opens details");
  reset(adult);open(touch);capture("adult-fish");input.click(detail().favorite,touch);capture("favorite-adult");input.click(detail().primary,touch);
  check(!session.domain().fish(id)&&session.domain().companion(id)&&session.domain().companion(id)->favorite,"Keep does not retain the favorite adult");
  care.advance(0);check(care.rewards().active(),"Keep did not start a reward flight");
  open(touch);capture("kept-fish");const auto kept=encode(session.domain().state());input.click(detail().secondary,touch);
  check(!care.detailsOpen()&&encode(session.domain().state())==kept,"Display fish can be sold twice");
  input.click(net,touch);capture("kept-locked");input.tap(water,touch);
  check(encode(session.domain().state())==kept,"Locked kept fish can be sold with the net");
  decodeAndValidate(kept,content);
 }
 // Reaching Junior while the net is active unlocks a sale without reselecting it.
 auto threshold=base;auto& young=threshold.fish.front();young.growthMs=young.purchase.durationMs/10000*young.purchase.stages[1]-1;young.lastFedAt=threshold.simNow;
 reset(threshold);input.click(net);input.tap(water);check(session.domain().fish(id)&&!care.detailsOpen(),"Baby sold just before Junior");
 session.domain().advanceCare(1);check(session.domain().fish(id)->age==1&&nextStageProgress(*session.domain().fish(id))==0.,"Stage meter did not reset at Junior");
 input.tap(water);check(!session.domain().fish(id)&&care.tool()==Tool::Sell,"Junior needs the net to be reselected");
 // Selling uses the live reward if the fish grows while its details are open.
 auto growing=base;testing::stage(growing.fish.front(),3);reset(growing);open();
 auto grown=session.domain().state();testing::stage(grown.fish.front(),4);session.domain().install(grown);
 const auto liveReward=fishReward(grown.fish.front());input.click(detail().secondary);
 check(!care.detailsOpen()&&!session.domain().fish(id)&&session.domain().state().wallet.coins==grown.wallet.coins+liveReward.coins()&&session.domain().state().xp==grown.xp+liveReward.xp,"Sell did not settle the current reward in one tap");
 // Eggs cannot be fed or refunded through the selling controls.
 auto egg=base;egg.fish.front().egg=true;egg.fish.front().hatchAt=egg.simNow+content.hatchMs;egg.fish.front().growthMs=0;reset(egg);open();
 input.click(detail().primary);check(care.detailsOpen()&&care.tool()==Tool::Select,"An egg can be kept");
 const auto eggBefore=encode(session.domain().state());input.click(detail().secondary);input.click(detail().secondary);check(care.detailsOpen()&&encode(session.domain().state())==eggBefore,"Egg sale changed its state");
 // The first finger owns the action. SDL's compatibility mouse events are ignored.
 reset(base);input.click(food,true);input.finger(SDL_EVENT_FINGER_DOWN,water,7);input.finger(SDL_EVENT_FINGER_DOWN,water,8);input.finger(SDL_EVENT_FINGER_UP,water,8);
 check(session.domain().pellets().empty(),"A second finger completes the first gesture");
 input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,water,false,SDL_BUTTON_LEFT,SDL_TOUCH_MOUSEID);input.finger(SDL_EVENT_FINGER_UP,water,7);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water,false,SDL_BUTTON_LEFT,SDL_TOUCH_MOUSEID);
 check(session.domain().pellets().size()==1,"Touch compatibility events duplicate feeding");
 input.finger(SDL_EVENT_FINGER_DOWN,water,9);input.finger(SDL_EVENT_FINGER_CANCELED,water,9);input.finger(SDL_EVENT_FINGER_UP,water,9);
 check(session.domain().pellets().size()==1,"Cancelled touch drops food");
 input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,water);SDL_Event focus{};focus.type=SDL_EVENT_WILL_ENTER_BACKGROUND;input.send(focus);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water);
 check(session.domain().pellets().size()==1,"Backgrounded gesture drops food");
 // Eat a real pellet, with the normal movement simulation and no direct Feed shortcut.
 reset(base);input.click(food);input.tap(water);const auto wallet=session.domain().state().wallet;
 for(int i=0;i<100&&session.domain().fish(id)->lastFedAt<base.simNow;++i)session.domain().stepMovement(.02,care.tool(),care.held());
 check(session.domain().fish(id)->lastFedAt==base.simNow&&session.domain().pellets().empty()&&session.domain().state().wallet.coins==wallet.coins,"A hungry fish cannot eat free food");
 session.domain().advanceCare(1000);check(session.domain().fish(id)->growthMs==1000,"Feeding does not resume growth");
 // Full tanks do not block care, and cancelling placement does not charge for an egg.
 FishPlacement placement;reset(base);check(bool(startFishPlacement(session.domain(),placement,"neonTetra")),"Placement setup failed");const auto beforeCancel=encode(session.domain().state());cancelFishPlacement(placement);
 check(!placement.active()&&encode(session.domain().state())==beforeCancel,"Switching away from placement costs coins");
 auto full=base;testing::stage(full.fish.front(),1);full.tanks.front().slots=int(full.fish.size());reset(full);input.click(food);input.tap(water);check(session.domain().pellets().size()==1,"Full tank blocks feeding");
 for(int i=1;i<48;++i)session.domain().execute({.action=Action::DropFood,.point={500,320}});
 input.tap(water);check(session.domain().pellets().size()==48&&!care.notice().empty(),"Pellet limit has no feedback");
 input.key(SDLK_ESCAPE);open();input.click(detail().secondary);check(session.domain().living({1})==0,"Full tank blocks selling");
 reset(base);open();auto switched=base;switched.tanks.push_back({TankId{2},10});switched.activeTank={2};session.domain().install(switched);care.advance(.02);
 check(!care.detailsOpen(),"Changing tanks leaves stale fish controls open");
 // A popover can switch fish directly without feeding or settling either one.
 auto pair=base;auto other=pair.fish.front();other.id={pair.nextFishId++};other.position={820,450};other.motion.hasPrevious=false;pair.fish.push_back(other);
 reset(pair);open();const auto pairBefore=encode(session.domain().state());input.tap(canvas.toScreen(other.position));
 check(care.detailsOpen()&&care.selected()==other.id&&encode(session.domain().state())==pairBefore,"Tapping another fish did not safely switch the popover");
 if(!captures.empty()){
  auto live=base;auto& fast=live.fish.front();fast.growthMs=fast.purchase.durationMs*999/10000;fast.lastFedAt=live.simNow;
  reset(live);open();capture("live-before");
  for(int frame=0;frame<6;++frame){session.update(20,session.domain().state().simNow+20,care.tool(),care.held());care.advance(.02);}
  capture("live-after");
  auto paused=session.domain().state();paused.fish.front().lastFedAt=paused.simNow-paused.fish.front().purchase.feedMs;
  reset(paused);open();capture("paused-before");
  session.update(120,paused.simNow+120,care.tool(),care.held());care.advance(.12);capture("paused-after");
  auto finishing=base;auto& almost=finishing.fish.front();almost.age=3;almost.growthMs=almost.purchase.durationMs-1;almost.lastFedAt=finishing.simNow;
  reset(finishing);open();capture("almost-adult");
  session.update(1,finishing.simNow+1,care.tool(),care.held());care.advance(.001);capture("new-adult");
  auto molly=base;auto& baby=molly.fish.front();baby.species="molly";
  baby.purchase=purchaseQuote(content,*content.find(baby.species),1,false);testing::stage(baby,0);
  baby.growthMs=baby.purchase.durationMs*17/100;baby.lastFedAt=molly.simNow;baby.favorite=true;
  reset(molly);open();capture("molly-growing");
  for(int stage=0;stage<=4;++stage){
   auto sample=base;auto& f=sample.fish.front();testing::stage(f,stage);
   if(stage<4)f.growthMs+=f.purchase.durationMs/10000*(f.purchase.stages[stage+1]-f.purchase.stages[stage])*3/5;
   reset(sample);input.key(SDLK_S);capture("stage-"+std::to_string(stage));
  }
  care.reset();session.domain().fixture("aquarium");care.advance(3);care.advance(3);input.key(SDLK_S);capture("ribbon-tank");
 }
 std::cout<<"PASS Clay feeding and rehoming, mouse and touch "<<width<<'x'<<height<<'\n';
}
void popoverGeometry(){
 for(const auto [w,h]:{std::pair{1642.f,958.f},std::pair{667.f,375.f},std::pair{390.f,844.f},std::pair{852.f,393.f}}){
  const Insets safe=w==852?Insets{59,0,59,21}:Insets{};
  for(const auto [x,y]:{std::pair{.12f,.18f},std::pair{.88f,.18f},std::pair{.12f,.85f},std::pair{.88f,.85f},std::pair{.5f,.5f}}){
   const Rect fish{w*x-24,h*y-14,48,28};
   for(bool extended:{false,true}){
    const auto l=layoutFishCare(w,h,safe,fish,44,extended);const auto r=l.dialog.frame;
    check(r.x>=safe.left&&r.y>=safe.top&&r.x+r.w<=w-safe.right+.01f&&r.y+r.h<=h-safe.bottom+.01f,"Popover clips a safe edge");
    for(const auto control:{l.favorite,l.dialog.close,l.secondary,l.primary}){
     check(r.has(control.x,control.y)&&r.has(control.x+control.w,control.y+control.h),"Popover control leaves its frame");
     check(control.w>=43.9f&&control.h>=43.9f,"Popover control is too small for touch");
    }
    check(l.primary.x+l.primary.w<l.secondary.x,"Popover actions overlap");
    if(extended)check(l.breakdown.y+l.breakdown.h<=l.note.y&&l.note.y+l.note.h<l.primary.y,"Growth badge overlaps rewards or actions");
    check(r.x+r.w<=fish.x||r.x>=fish.x+fish.w||r.y+r.h<=fish.y||r.y>=fish.y+fish.h,"Popover hides the selected fish");
    const auto point=l.tail[2];
    check(point.x>=fish.x-8*l.dialog.unit-.1f&&point.x<=fish.x+fish.w+8*l.dialog.unit+.1f&&point.y>=fish.y-8*l.dialog.unit-.1f&&point.y<=fish.y+fish.h+8*l.dialog.unit+.1f,"Popover pointer misses its fish");
   }
  }
 }
 std::cout<<"PASS popover safe edges, touch targets and fish anchors\n";
}
void popoverCapture(const Content& content,const std::filesystem::path& assets,const std::filesystem::path& output){
 if(output.empty())return;
 Canvas canvas(assets,1088,635,true);Session session(content,"/tmp/popover-preview-unused.json",1000,true);HudCare care(canvas,session);
 auto state=fishState(content,4);auto& fish=state.fish.front();
 fish.species="longhornCowfish";fish.purchase=purchaseQuote(content,*content.find(fish.species),30,false);testing::stage(fish,4);
 fish.position={619,360};fish.motion={};fish.motion.direction=1;
 state.wallet.coins=1157;state.xp=46;testing::openingBalances(state);session.domain().install(state);
 for(const auto [w,h]:{std::pair{1642,958},std::pair{667,375},std::pair{1024,768},std::pair{390,844}}){
  canvas.previewViewport(PreviewViewport{w,h,1,{}});canvas.begin();care.reset();
  Input input{canvas,care,w,h,{}};input.tap(canvas.toScreen(fish.position));check(care.detailsOpen(),"Cowfish preview did not open");
  canvas.begin();canvas.scene(session.domain(),1,0,Tool::Select,care.held(),false);
  paintHud(canvas,session.domain(),layoutHud(canvas.width(),canvas.height(),{},canvas.minimumTouchSize()));care.paint();
  std::filesystem::create_directories(output);
  check(canvas.capture(output/("cowfish-"+std::to_string(w)+"x"+std::to_string(h)+".png")),"Cowfish capture failed");
  const auto l=care.detailsLayout();const float sx=float(w)/canvas.width(),sy=float(h)/canvas.height();
  std::ofstream metadata(output/("cowfish-"+std::to_string(w)+"x"+std::to_string(h)+".json"));
  metadata<<Json{{"frame",{l.dialog.frame.x*sx,l.dialog.frame.y*sy,l.dialog.frame.w*sx,l.dialog.frame.h*sy}},{"tail_tip",{l.tail[2].x*sx,l.tail[2].y*sy}},{"viewport",{w,h}}}.dump(2);
 }
}
void storage(const Content& content,const std::filesystem::path& assets){
 const auto root=std::filesystem::temp_directory_path()/("fishius-hud-care-"+std::to_string(SDL_GetTicksNS()));std::filesystem::create_directories(root);
 Canvas canvas(assets,1088,635,true);
 Session session(content,root/"save.json",1000);session.domain().install(fishState(content));HudCare care(canvas,session);Input input{canvas,care,1088,635,{}};
 const auto point=canvas.toScreen({544,320});const auto id=session.domain().state().fish.front().id;
 input.key(SDLK_F);input.tap(point);for(int i=0;i<100;++i)session.update(20,1000,care.tool());
 check(session.checkpoint(1000),"Cannot save feeding");
 Session loaded(content,root/"save.json",1000);check(loaded.domain().fish(id)->lastFedAt>=0,"Feeding was lost on reload");
 auto adult=loaded.domain().state();testing::stage(adult.fish.front(),4);loaded.domain().install(adult);
 HudCare loadedCare(canvas,loaded);Input rehome{canvas,loadedCare,1088,635,{}};
 rehome.key(SDLK_S);rehome.tap(canvas.toScreen(loaded.domain().fish(id)->position));
 check(!loaded.domain().fish(id),"Saved fish was not rehomed");Session settled(content,root/"save.json",1000);
 const auto saved=settled.domain().state();check(settled.command({.action=Action::Sell,.fish=id}).replayed&&settled.domain().state().wallet.coins==saved.wallet.coins&&settled.domain().state().xp==saved.xp&&settled.domain().state().settlements==saved.settlements,"Reload allows a second reward");
 {std::ofstream file(root/"blocked");file<<"not a directory";}
 Session blocked(content,root/"blocked"/"save.json",1000);blocked.domain().install(fishState(content,4));HudCare blockedCare(canvas,blocked);Input fail{canvas,blockedCare,1088,635,{}};
 const auto before=encode(blocked.domain().state());fail.key(SDLK_S);fail.tap(point);
 check(blockedCare.tool()==Tool::Sell&&!blockedCare.detailsOpen()&&!blockedCare.notice().empty()&&encode(blocked.domain().state())==before,"Save failure loses a fish or has no retry feedback");
 fail.key(SDLK_ESCAPE);fail.tap(point);check(blockedCare.detailsOpen(),"Cannot open details after a failed save");
 fail.click(blockedCare.detailsLayout().secondary);
 check(blockedCare.detailsOpen()&&!blockedCare.notice().empty()&&encode(blocked.domain().state())==before,"Failed popover sale loses a fish or closes its retry controls");
 blockedCare.advance(0);check(!blockedCare.rewards().active(),"A failed save animated an uncommitted reward");
 std::filesystem::remove_all(root);
 std::cout<<"PASS feeding persistence, rehome replay and save failure rollback\n";
}
}
int main(int argc,char** argv){try{
 const auto assets=std::filesystem::absolute(argc>1?argv[1]:"assets");std::ifstream file(assets/"content.json");const auto content=Content::fromJson(Json::parse(file));
 const std::filesystem::path captures=argc>2?argv[2]:"";
 popoverGeometry();
 for(const auto [w,h]:{std::pair{667,375},std::pair{1024,768},std::pair{390,844}})viewport(content,assets,w,h,captures);
 if(!captures.empty())viewport(content,assets,1672,941,captures);
 storage(content,assets);popoverCapture(content,assets,captures);return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
