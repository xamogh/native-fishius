#include "aquarium/hud_care.hpp"
#include "aquarium/hud_placement.hpp"
#include "fixtures.hpp"
#include <cmath>
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
 void press(SDL_FPoint p,bool touch=false,bool blocked=false){if(touch)finger(SDL_EVENT_FINGER_DOWN,p,1,blocked);else mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,p,blocked);}
 void move(SDL_FPoint p,bool touch=false,bool blocked=false){if(touch)finger(SDL_EVENT_FINGER_MOTION,p,1,blocked);else mouse(SDL_EVENT_MOUSE_MOTION,p,blocked);}
 void release(SDL_FPoint p,bool touch=false,bool blocked=false){if(touch)finger(SDL_EVENT_FINGER_UP,p,1,blocked);else mouse(SDL_EVENT_MOUSE_BUTTON_UP,p,blocked);}
 void tap(SDL_FPoint p,bool touch=false,bool blocked=false){press(p,touch,blocked);release(p,touch,blocked);}
 void drag(SDL_FPoint from,SDL_FPoint to,bool touch=false){press(from,touch);move(to,touch);release(to,touch);}
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
void viewport(const Content& content,const std::filesystem::path& assets,int width,int height,const std::filesystem::path& captures,Insets safe={},int pixelRatio=1){
 Canvas canvas(assets,width,height,true);Session session(content,"/tmp/hud-care-unused.json",1000,true);HudCare care(canvas,session);Input input{canvas,care,width,height,{}};
 canvas.previewViewport(PreviewViewport{width,height,pixelRatio,safe});canvas.begin();
 const auto hud=layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
 const auto detail=[&](){return care.detailsLayout();};
 const auto water=canvas.toScreen({544,320});const auto food=hud[HudPart::Food],net=hud[HudPart::Rehome];
 const State base=fishState(content);const auto id=base.fish.front().id;
 auto reset=[&](State state){care.reset();input.pointer={};session.domain().install(std::move(state));care.advance(3);};
 auto open=[&](bool touch=false){input.tap(water,touch);check(care.detailsOpen()&&care.selected()==id,"Fish tap did not open its details");};
 auto capture=[&](std::string name){
  if(captures.empty())return;
  // Capture settled popovers so the following click uses the visible controls.
  if(care.detailsOpen())care.advance(.5);
  canvas.begin();canvas.scene(session.domain(),1,0,care.tool(),care.held(),false,{},nullptr,care.selectedDecor());const auto rewards=care.rewards().display(session.domain());paintHud(canvas,session.domain(),hud,{},{},&rewards);care.paint();care.rewards().paint(canvas,hud);
  const auto folder=captures/(std::to_string(width)+"x"+std::to_string(height)+(pixelRatio>1?"@"+std::to_string(pixelRatio)+"x":"")+(safe.left||safe.top||safe.right||safe.bottom?"-safe":""));std::filesystem::create_directories(folder);
  check(canvas.capture(folder/(name+".png")),"Care screenshot failed");
 };
 // Decoration selection is presentation only, with the same tap rules for mouse and touch.
 auto decorated=base;decorated.decor={{501,"CP-02",{1},{300,480}},{502,"CD-01",{1},{700,500}}};
 // Selection fixtures stay in the visible crop, including portrait windows.
 for(auto& item:decorated.decor){const auto projection=canvas.decorProjection();
  item.position=projection.place(*content.findDecor(item.kind),projection.toWorld(canvas.toScreen(item.position)));
 }
 decorated.nextDecorId=503;decorated.decorOwned={"CP-02","CD-01"};
 const auto plantPoint=center(canvas.decorRect(*content.findDecor("CP-02"),decorated.decor[0].position));
 const auto rockPoint=center(canvas.decorRect(*content.findDecor("CD-01"),decorated.decor[1].position));
 const auto emptyPoint=canvas.toScreen({544,450});
 for(bool touch:{false,true}){
  reset(base);const auto unchanged=encode(session.domain().state());
  input.tap(emptyPoint,touch);
  check(session.domain().ripples().size()==1&&session.domain().fish(id)->motion.fleeRemaining>0,"Empty water does not create a ripple and startle nearby fish");
  check(encode(session.domain().state())==unchanged,"Water tap changes saved state before swimming");
  const auto at=canvas.toScreen(session.domain().ripples().front().position);
  check(std::hypot(at.x-emptyPoint.x,at.y-emptyPoint.y)<.01,"Ripple is not centered on the tap");
  for(int i=0;i<12;++i)session.domain().stepMovement(.02,Tool::Select);
  capture(touch?"water-ripple-touch":"water-ripple");
  check((session.domain().fish(id)->position.x-base.fish.front().position.x)*base.fish.front().motion.direction>0&&session.domain().fish(id)->motion.direction==base.fish.front().motion.direction,"Water tap reverses the fish's heading");
  reset(base);input.tap(water,touch);check(session.domain().ripples().empty()&&care.detailsOpen(),"Fish selection creates a ripple");
  if(!care.detailsLayout().dialog.frame.has(emptyPoint.x,emptyPoint.y)){
   input.tap(emptyPoint,touch);check(!care.detailsOpen()&&session.domain().ripples().size()==1,"Empty water cannot dismiss fish details and ripple");
  }
  reset(decorated);input.tap(plantPoint,touch);check(session.domain().ripples().empty(),"Decor selection creates a ripple");
  for(auto part:{HudPart::Food,HudPart::Rehome,HudPart::Shop,HudPart::Settings}){
   reset(base);input.click(hud[part],touch);check(session.domain().ripples().empty(),"HUD button creates a ripple");
   if(part==HudPart::Food||part==HudPart::Rehome){input.tap(emptyPoint,touch);check(session.domain().ripples().empty(),"A tool tap creates a ripple");}
  }
  reset(base);input.tap(emptyPoint,touch,true);check(session.domain().ripples().empty(),"A menu allows water ripples underneath");
 }
 reset(base);
 input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,emptyPoint);check(session.domain().ripples().empty(),"Unmatched release creates a ripple");
 input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,emptyPoint);check(session.domain().ripples().empty(),"Water ripples before a tap finishes");
 input.mouse(SDL_EVENT_MOUSE_MOTION,water);input.mouse(SDL_EVENT_MOUSE_MOTION,emptyPoint);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,emptyPoint);
 check(session.domain().ripples().empty(),"Dragging away and back creates a ripple");
 input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,emptyPoint,false,SDL_BUTTON_RIGHT);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,emptyPoint,false,SDL_BUTTON_RIGHT);
 input.finger(SDL_EVENT_FINGER_DOWN,emptyPoint,7);input.finger(SDL_EVENT_FINGER_CANCELED,emptyPoint,7);input.finger(SDL_EVENT_FINGER_UP,emptyPoint,7);
 check(session.domain().ripples().empty(),"Right click or cancelled touch creates a ripple");
 input.finger(SDL_EVENT_FINGER_DOWN,emptyPoint,7);input.finger(SDL_EVENT_FINGER_DOWN,water,8);input.finger(SDL_EVENT_FINGER_UP,water,8);
 input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,emptyPoint,false,SDL_BUTTON_LEFT,SDL_TOUCH_MOUSEID);
 input.finger(SDL_EVENT_FINGER_UP,emptyPoint,7);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,emptyPoint,false,SDL_BUTTON_LEFT,SDL_TOUCH_MOUSEID);
 check(session.domain().ripples().size()==1,"Touch compatibility events or a second finger duplicate a ripple");
 std::cout<<"PASS empty-water ripples, fish response and input ownership "<<width<<'x'<<height<<'\n';
 for(bool touch:{false,true}){
  reset(decorated);const auto unchanged=encode(session.domain().state());
  input.tap(plantPoint,touch);check(care.selectedDecor()==501&&!care.detailsOpen(),"Tapping a plant does not select it");capture("plant-selected");
  input.tap(rockPoint,touch);check(care.selectedDecor()==502&&!care.detailsOpen(),"Tapping a decoration does not replace the selected plant");capture("decoration-selected");
  input.tap(rockPoint,touch);check(care.selectedDecor()==502,"Tapping the selected decoration unexpectedly clears it");
  const auto clearSelection=canvas.toScreen({544,200});
  input.tap(clearSelection,touch);check(!care.selectedDecor(),"Tapping empty water leaves a decoration selected");capture("decor-selection-cleared");
  input.tap(plantPoint,touch);input.key(SDLK_ESCAPE);check(!care.selectedDecor(),"Escape leaves a decoration selected");
  input.tap(plantPoint,touch);input.tap(water,touch);check(care.detailsOpen()&&!care.selectedDecor(),"Selecting a fish leaves the decoration glowing");
  // A visible plant outside the fish popover can be selected with one tap.
  if(!care.detailsLayout().dialog.frame.has(plantPoint.x,plantPoint.y)){
   input.tap(plantPoint,touch);check(!care.detailsOpen()&&care.selectedDecor()==501,"A plant tap cannot switch from fish details to decor selection");
  }
  check(encode(session.domain().state())==unchanged,"Decor selection changes saved state or charges currency");
  care.reset();input.tap(plantPoint,touch);input.click(food,touch);check(!care.selectedDecor()&&care.tool()==Tool::Food,"Food mode leaves decor selected");
  input.tap(plantPoint,touch);check(!care.selectedDecor(),"Feeding also selects a plant");
  const auto afterFeeding=encode(session.domain().state());
  input.key(SDLK_ESCAPE);input.tap(plantPoint,touch);input.key(SDLK_S);check(!care.selectedDecor(),"Rehome mode leaves decor selected");
  input.tap(plantPoint,touch);check(!care.selectedDecor(),"The net selects a plant");input.key(SDLK_ESCAPE);
  input.tap(plantPoint,touch);input.click(hud[HudPart::Shop],touch);check(!care.selectedDecor(),"Opening a menu leaves decor selected");
  input.tap(plantPoint,touch);input.tap(rockPoint,touch,true);check(!care.selectedDecor(),"Blocked input selects decor behind a menu");
  check(encode(session.domain().state())==afterFeeding,"Changing or clearing decor selection changes saved state");
 }
 reset(decorated);
 input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,plantPoint);check(!care.selectedDecor(),"An unmatched release selects decor");
 input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,plantPoint);check(!care.selectedDecor(),"Decor is selected before a tap finishes");
 input.mouse(SDL_EVENT_MOUSE_MOTION,emptyPoint);input.mouse(SDL_EVENT_MOUSE_MOTION,plantPoint);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,plantPoint);
 check(!care.selectedDecor(),"Dragging away and back selects decor");
 input.finger(SDL_EVENT_FINGER_DOWN,plantPoint,7);input.finger(SDL_EVENT_FINGER_CANCELED,plantPoint,7);input.finger(SDL_EVENT_FINGER_UP,plantPoint,7);
 check(!care.selectedDecor(),"Cancelled touch selects decor");
 input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,plantPoint,false,SDL_BUTTON_RIGHT);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,plantPoint,false,SDL_BUTTON_RIGHT);
 check(!care.selectedDecor(),"Right click selects decor");
 auto overlap=decorated;overlap.fish.front().position=canvas.toWorld(plantPoint.x,plantPoint.y);overlap.fish.front().motion={};
 reset(overlap);input.tap(plantPoint);check(care.selected()==id&&!care.selectedDecor(),"Decor intercepts a fish drawn in front of it");
 overlap=decorated;overlap.decor[1]=overlap.decor[0];overlap.decor[1].id=502;
 reset(overlap);input.tap(plantPoint);check(care.selectedDecor()==502,"Equal-depth decor does not select the last drawn copy");
 const auto overlappingSave=encode(session.domain().state());
 input.tap(plantPoint);check(care.selectedDecor()==501,"Repeated overlap tap cannot select the covered copy");
 input.tap(plantPoint);check(care.selectedDecor()==502&&encode(session.domain().state())==overlappingSave,"Overlap cycling changes the save or cannot return to the front copy");
 overlap.decor[0].position.y+=4;reset(overlap);input.tap(plantPoint);check(care.selectedDecor()==501,"Overlapping decor ignores anchor depth");
 const auto background=std::find_if(content.decorations.begin(),content.decorations.end(),[](const auto& def){return def.layer=="Background";});
 check(background!=content.decorations.end(),"Missing background decor fixture");
 overlap.decor[1].kind=background->id;overlap.decor[1].position.y+=8;reset(overlap);
 input.tap(plantPoint);check(care.selectedDecor()==502,"Catalog layers override the item's live perspective depth");
 for(int change=0;change<4;++change){
  reset(decorated);input.tap(plantPoint);auto changed=session.domain().state();
  if(change==0)changed.decor[0].stored=true;
  if(change==1)changed.decor.erase(changed.decor.begin());
  if(change==2){changed.tanks.push_back({TankId{2},10});changed.decor[0].tank={2};}
  if(change==3){changed.tanks.push_back({TankId{2},10});changed.activeTank={2};}
  session.domain().install(changed);care.advance(0);check(!care.selectedDecor(),"Hidden or removed decor retains its selection");
  if(change<3){input.tap(plantPoint);check(!care.selectedDecor(),"Stored, removed or transferred decor can still be selected");}
 }
 auto scaled=decorated;scaled.decor[0].flipped=true;scaled.decor[0].sizeMul=1.7;reset(scaled);
 input.tap(center(canvas.decorRect(*content.findDecor("CP-02"),scaled.decor[0].position,scaled.decor[0].sizeMul)),true);
 check(care.selectedDecor()==501,"A resized or flipped plant cannot be selected");
 std::cout<<"PASS plant and decor selection, clearing, overlap and input cancellation "<<width<<'x'<<height<<'\n';
 for(bool touch:{false,true}){
  reset(base);const auto walletBefore=session.domain().state().wallet;
  input.click(food,touch);check(care.tool()==Tool::Food&&session.domain().pellets().empty(),"Food activation drops a pellet or fails to toggle");
  input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water);check(session.domain().pellets().empty(),"An unmatched release drops food");
  input.tap(water,touch);input.drag(water,{water.x+canvas.minimumTouchSize()*3/44,water.y},touch);
  check(session.domain().pellets().empty()&&!care.detailsOpen(),"A tap or small finger movement drops food or selects a fish");
  input.press(emptyPoint,touch);care.advance(.2);check(session.domain().pellets().empty(),"Holding without dragging drops food");
  input.move(water,touch);const auto firstTrail=session.domain().pellets().size();
  check(firstTrail>1,"A quick swipe does not leave several pellets before release");
  for(const auto& pellet:session.domain().pellets())check(std::abs(pellet.position.x-544)<.01&&pellet.position.y>=319.99&&pellet.position.y<=450.01,"Food misses the drag path");
  const auto next=canvas.toScreen({300,320});input.move(next,touch);
  check(session.domain().pellets().size()>firstTrail,"Continuing a drag does not keep dropping food");
  const auto beforeHold=session.domain().pellets().size();input.move(next,touch);input.move(next,touch);
  check(session.domain().pellets().size()==beforeHold,"Repeated motion events duplicate food");
  for(int i=0;i<20;++i)care.advance(.05);
  check(session.domain().pellets().size()>=beforeHold+12,"A slow drag or pause does not pour a steady stream");
  capture(touch?"feeding-drag-touch":"feeding-drag");
  const auto poured=session.domain().pellets().size();
  input.release(water,touch);
  check(session.domain().pellets().size()==poured&&session.domain().state().wallet.coins==walletBefore.coins&&session.domain().state().wallet.pearls==walletBefore.pearls&&session.domain().state().xp==base.xp,"Releasing adds food or feeding charges money");
  check(!care.detailsOpen()&&session.domain().ripples().empty(),"Pouring food activates the tank underneath");
  care.advance(.1);capture(touch?"feeding-touch":"feeding");
  input.release(water,touch);input.move(emptyPoint,touch);
  input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,emptyPoint,false,SDL_BUTTON_RIGHT);input.mouse(SDL_EVENT_MOUSE_MOTION,water);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water,false,SDL_BUTTON_RIGHT);
  care.advance(.2);check(session.domain().pellets().size()==poured,"Release, hover or right drag keeps pouring food");
  const SDL_FPoint done{canvas.width()*.5f,canvas.height()-canvas.safeInsets().bottom-64*hud.unit};
  for(const auto invalid:{center(food),center(hud[HudPart::Shop]),center(hud[HudPart::Profile]),done,SDL_FPoint{-10,water.y}}){
   input.press(water,touch);input.move(invalid,touch);care.advance(.2);input.release(invalid,touch);
   check(session.domain().pellets().size()==poured&&care.tool()==Tool::Food,"Dragging over a control or outside water pours food");
  }
  input.press(center(food),touch);input.move(water,touch);const auto beforeReturn=session.domain().pellets().size();
  input.move(center(food),touch);care.advance(.2);input.release(center(food),touch);
  check(session.domain().pellets().size()==beforeReturn&&care.tool()==Tool::Food,"Returning to Food keeps pouring or toggles the tool");
  input.click(food,touch);check(care.tool()==Tool::Select,"Food does not toggle off");
  input.key(SDLK_F);input.key(SDLK_F,true);check(care.tool()==Tool::Food,"Key repeat toggles feeding");input.key(SDLK_ESCAPE);check(care.tool()==Tool::Select,"Escape does not stop feeding");
  for(auto part:{HudPart::Shop,HudPart::Tank,HudPart::Bag,HudPart::Projects,HudPart::Rewards,HudPart::Settings,HudPart::CoinPlus,HudPart::PearlPlus}){
   input.click(food,touch);input.click(hud[part],touch);check(care.tool()==Tool::Select,"A menu keeps Food active");
   input.click(net,touch);input.click(hud[part],touch);check(care.tool()==Tool::Select,"A menu keeps Rehome active");
  }
  input.click(food,touch);input.tap(water,touch,true);check(care.tool()==Tool::Select&&session.domain().pellets().size()==beforeReturn,"Modal input feeds a fish underneath");
  reset(base);input.drag(emptyPoint,water,touch);input.drag(center(net),water,touch);
  check(session.domain().pellets().empty()&&care.tool()==Tool::Select,"A drag outside Food mode starts feeding");
  input.press(center(food),touch);input.move(water,touch);
  check(care.tool()==Tool::Food&&session.domain().pellets().size()==1,"Dragging Food does not start pouring immediately");
  for(int i=0;i<6;++i){input.move(canvas.toScreen({544+i*24.,320}),touch);care.advance(.08);}
  check(session.domain().pellets().size()>=7,"Dragging from the Food button does not pour multiple pellets");
  capture(touch?"feeding-from-button-touch":"feeding-from-button");
  const auto beforeRelease=session.domain().pellets().size();input.release(water,touch);care.advance(.2);
  check(session.domain().pellets().size()==beforeRelease,"Releasing Food does not stop the stream");
  input.drag(center(food),emptyPoint,touch);check(session.domain().pellets().size()==beforeRelease+1,"A second Food drag does not start a new stream");
  input.click(net,touch);input.drag(center(food),water,touch);
  check(care.tool()==Tool::Food&&!care.selected().value&&session.domain().pellets().size()==beforeRelease+2,"Dragging Food does not leave the Sell tool");
  for(const auto type:{SDL_EVENT_WINDOW_FOCUS_LOST,SDL_EVENT_WILL_ENTER_BACKGROUND,SDL_EVENT_WINDOW_RESIZED,SDL_EVENT_RENDER_DEVICE_RESET,SDL_EVENT_RENDER_TARGETS_RESET}){
   input.press(center(food),touch);input.move(water,touch);const auto beforeCancel=session.domain().pellets().size();
   SDL_Event interrupted{};interrupted.type=type;input.send(interrupted);care.advance(.2);input.move(emptyPoint,touch);input.release(water,touch);
   check(session.domain().pellets().size()==beforeCancel,"An interrupted drag keeps pouring food");
  }
  input.press(center(food),touch);input.move(water,touch);const auto beforeEscape=session.domain().pellets().size();
  input.key(SDLK_ESCAPE);care.advance(.2);input.release(water,touch);
  check(care.tool()==Tool::Select&&session.domain().pellets().size()==beforeEscape,"Escape does not stop pouring food");
  input.press(center(food),touch);input.move(water,touch,true);care.advance(.2);input.release(water,touch);
  check(care.tool()==Tool::Select&&session.domain().pellets().size()==beforeEscape,"A menu opening during a drag keeps pouring food");
  reset(base);input.press(center(food),touch);input.move(water,touch);input.move(center(hud[HudPart::Shop]),touch);
  for(int i=0;i<20;++i)care.advance(.05);
  check(session.domain().pellets().size()==1,"Holding a drag over a menu keeps pouring food");
  input.move(emptyPoint,touch);check(session.domain().pellets().size()==2,"Returning to water releases a backlog of food");input.release(emptyPoint,touch);
  std::size_t steadyCount=0;
  for(int fps:{30,60,120}){
   reset(base);input.press(center(food),touch);input.move(water,touch);
   for(int i=0;i<fps;++i)care.advance(1./fps);
   const auto count=session.domain().pellets().size();input.release(water,touch);
   check(count>=12&&count<=14&&(!steadyCount||count==steadyCount),"Food stream depends on frame rate");steadyCount=count;
  }
  reset(base);input.press(center(food),touch);input.move(water,touch);care.advance(10);
  check(session.domain().pellets().size()<=5,"A stalled frame releases a burst of food");input.release(water,touch);
  reset(base);input.press(center(food),touch);input.move(water,touch);
  for(int i=0;i<100;++i)care.advance(.08);
  check(session.domain().pellets().size()==48&&!care.notice().empty(),"A long pour exceeds the food limit or hides its feedback");input.release(water,touch);
  // Before Junior, neither the net nor the details action can settle a fish.
  for(bool egg:{false,true}){
   auto young=base;young.fish.front().egg=egg;reset(young);const auto unchanged=encode(session.domain().state());
   input.click(net,touch);capture(egg?"egg-growing":"baby-growing");input.tap(water,touch);
   check(care.tool()==Tool::Sell&&!care.detailsOpen()&&encode(session.domain().state())==unchanged,"Net sold an egg or Baby");
   input.key(SDLK_ESCAPE);open(touch);input.click(detail().action,touch);input.click(detail().action,touch);
   check(care.detailsOpen()&&encode(session.domain().state())==unchanged,"Details sold an egg or Baby");
   capture(egg?"egg-popover":"baby-popover");
  }
  auto junior=base;testing::stage(junior.fish.front(),1);reset(junior);
  input.click(net,touch);input.tap(water,touch);check(!session.domain().fish(id)&&care.tool()==Tool::Sell,"Junior cannot be sold with one tap");
  auto fed=junior;fed.fish.front().lastFedAt=fed.simNow;reset(fed);open(touch);capture("growing-fish");
  reset(junior);
  // Both modal close gestures and selling buttons require a complete, stationary tap.
  open(touch);const auto before=encode(session.domain().state());capture("hungry-fish");
  input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,center(detail().action));input.mouse(SDL_EVENT_MOUSE_MOTION,water);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,center(detail().action));
  check(care.detailsOpen()&&encode(session.domain().state())==before,"Dragging the Sell button sells a fish");
  input.finger(SDL_EVENT_FINGER_DOWN,center(detail().action));input.finger(SDL_EVENT_FINGER_CANCELED,center(detail().action));input.finger(SDL_EVENT_FINGER_UP,center(detail().action));
  check(care.detailsOpen()&&encode(session.domain().state())==before,"Cancelled Sell touch sells a fish");
  input.key(SDLK_ESCAPE);check(!care.detailsOpen()&&encode(session.domain().state())==before,"Escape sells a fish");
  open(touch);input.click(detail().favorite,touch);check(session.domain().fish(id)->favorite,"Favorite is not saved");
  capture("favorite-popover");
  input.click(detail().action,touch);check(care.detailsOpen()&&session.domain().fish(id),"Favorite can be sold");
  check(care.toast().visible(),"Blocked favourite sale in details has no toast");
  const auto protectedState=encode(session.domain().state());
  const auto protectedLayout=detail().dialog.frame;
  care.advance(.5);input.mouse(SDL_EVENT_MOUSE_MOTION,{0,0});capture("favorite-toast-popover");
  input.click(care.toastLayout().close,touch);
  check(!care.toast().visible()&&care.detailsOpen()&&encode(session.domain().state())==protectedState,"Toast dismissal changes the favourite fish or closes its details");
  check(detail().dialog.frame.h==protectedLayout.h,"Toast changes the fish popover size");
  input.click(detail().favorite,touch);check(care.detailsOpen()&&!session.domain().fish(id)->favorite,"Unfavorite did not restore selling");
  const auto saleButton=detail().action;const auto saleReward=fishReward(*session.domain().fish(id));
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
  const auto settled=encode(session.domain().state());input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,center(detail().action));
  check(encode(session.domain().state())==settled,"Repeated release pays twice");
  care.advance(.1);check(care.rewards().active()&&care.rewards().display(session.domain()).coins==balance,"Quick rehome did not start a reward flight");capture("rehome-reward");decodeAndValidate(settled,content);
  auto favorite=adult;favorite.fish.front().favorite=true;reset(favorite);capture("favorite-idle");input.click(net,touch);capture("favorite-locked");input.tap(water,touch);
  check(session.domain().fish(id)&&!care.detailsOpen(),"Quick sale removes a favorite or opens details");
  const auto favoriteState=encode(session.domain().state());
  check(care.toast().visible()&&care.notice()=="Unfavourite it before selling.","Favourite sale does not show the approved toast message");
  if(width==667&&!touch&&!captures.empty()){
   capture("toast-enter-000");care.advance(.12);capture("toast-enter-120");
   care.advance(.18);capture("toast-enter-300");care.advance(.2);
  }else care.advance(.5);
  input.mouse(SDL_EVENT_MOUSE_MOTION,{0,0});capture(touch?"favorite-toast-touch":"favorite-toast");
  const auto closeToast=care.toastLayout().close;
  input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,center(closeToast));
  check(care.toast().visible(),"Unmatched release dismisses the toast");
  input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,center(closeToast));input.mouse(SDL_EVENT_MOUSE_MOTION,water);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,center(closeToast));
  check(care.toast().visible()&&encode(session.domain().state())==favoriteState,"Dragging the toast close button changes game state or dismisses it");
  input.finger(SDL_EVENT_FINGER_DOWN,center(closeToast));input.finger(SDL_EVENT_FINGER_CANCELED,center(closeToast));input.finger(SDL_EVENT_FINGER_UP,center(closeToast));
  check(care.toast().visible(),"Cancelled touch dismisses the toast");
  input.click(care.toastLayout().title,touch);
  check(care.toast().visible()&&encode(session.domain().state())==favoriteState,"Toast body tap passes through to the aquarium");
  input.click(closeToast,touch);
  check(!care.toast().visible()&&care.tool()==Tool::Sell&&encode(session.domain().state())==favoriteState,"Dismissing toast changes the fish, wallet or active tool");
  input.tap(water,touch);care.advance(2.8);input.tap(water,touch);care.advance(3.2);
  check(care.toast().visible(),"Repeated blocked sale does not restart the toast timeout");
  input.key(SDLK_ESCAPE);
  check(!care.toast().visible()&&care.tool()==Tool::Sell,"Escape fails to dismiss toast independently of the sell tool");
  input.tap(water,touch);care.advance(.5);input.click(care.toastLayout().title,true);care.advance(3.01);
  check(!care.toast().visible()&&encode(session.domain().state())==favoriteState,"Touch leaves a permanent hover or toast changes protected fish state");
  input.tap(water,touch);care.advance(.5);input.mouse(SDL_EVENT_MOUSE_MOTION,center(care.toastLayout().title));care.advance(2.9);
  check(care.toast().visible(),"Toast disappears before its short display time ends");
  if(!touch)capture("toast-exit-3400");
  care.advance(.11);
  check(!care.toast().visible()&&encode(session.domain().state())==favoriteState,"Toast stays past 3.5 seconds while hovered or changes the favourite fish");
  if(!touch)capture("toast-gone-3510");
  input.tap(water,touch);input.click(hud[HudPart::Shop],touch);
  check(!care.toast().visible(),"Opening a menu leaves a stale toast");
  reset(adult);open(touch);capture("adult-fish");input.click(detail().favorite,touch);capture("favorite-adult");
  const auto retained=encode(session.domain().state());input.click(detail().action,touch);
  check(care.detailsOpen()&&session.domain().fish(id)->favorite&&encode(session.domain().state())==retained,"Favorite adult was sold or converted");
  check(session.domain().state().wallet.coins==adult.wallet.coins&&session.domain().state().xp==adult.xp&&session.domain().living({1})==1,"Favoriting paid a reward or freed tank space");
  care.advance(0);check(!care.rewards().active(),"Favoriting started a reward flight");
  reset(decodeAndValidate(retained,content));open(touch);capture("favorite-restored");input.click(detail().dialog.close,touch);
  input.click(net,touch);capture("favorite-retained");input.tap(water,touch);
  check(encode(session.domain().state())==retained,"Favorite adult can be sold after reload");
  input.key(SDLK_ESCAPE);input.key(SDLK_ESCAPE);open(touch);input.click(detail().favorite,touch);input.click(detail().action,touch);
  check(!session.domain().fish(id)&&session.domain().state().wallet.coins==adult.wallet.coins+expected.coins(),"Unfavorite adult lost its sale value");
 }
 for(const auto* species:{"bubbleEyeGoldfish","koi"})for(bool touch:{false,true})for(int stage:{1,4}){
  auto pearl=base;pearl.xp=content.levels[4];pearl.highestRewardedLevel=5;testing::openingBalances(pearl);
  auto& fish=pearl.fish.front();fish.species=species;fish.purchase=purchaseQuote(content,*content.find(species),5);testing::stage(fish,stage);fish.lastFedAt=pearl.simNow;
  const auto expected=fishReward(fish);reset(pearl);open(touch);capture(std::string(species)+(stage==4?"-adult":"-junior"));
  check(expected.coins()>0&&expected.xp>0,"Pearl fish has no displayed sale reward");
  input.click(detail().action,touch);
  check(!session.domain().fish(id)&&!care.detailsOpen()&&care.tool()==Tool::Select,"Pearl fish details do not sell the fish");
  check(session.domain().state().wallet.coins==pearl.wallet.coins+expected.coins()&&session.domain().state().xp==pearl.xp+expected.xp&&session.domain().state().wallet.pearls==pearl.wallet.pearls,"Pearl fish details pay the wrong reward");
  decodeAndValidate(encode(session.domain().state()),content);
 }
 if(!captures.empty()){
  auto marker=base;testing::stage(marker.fish.front(),4);marker.fish.front().favorite=true;marker.fish.front().lastFedAt=marker.simNow;
  marker.fish.front().motion.direction=1;marker.fish.front().motion.turnRemaining=0;marker.fish.front().motion.pitch=.15;
  reset(marker);input.click(net);capture("favorite-head-left");
  input.click(net);capture("favorite-after-sell");input.click(food);capture("favorite-feeding");
  marker.fish.front().motion.direction=-1;reset(marker);input.click(net);capture("favorite-head-right");
  marker.fish.front().favorite=false;reset(marker);input.click(net);capture("favorite-head-off");
 }
 // Reduced motion keeps the same notice and protected state without animation.
 auto reducedFavorite=base;testing::stage(reducedFavorite.fish.front(),4);reducedFavorite.fish.front().favorite=true;reducedFavorite.settings.reducedMotion=true;
 reset(reducedFavorite);input.click(net);input.tap(water);care.advance(0);input.mouse(SDL_EVENT_MOUSE_MOTION,{0,0});capture("favorite-toast-reduced-motion");
 check(care.toast().visible()&&encode(session.domain().state())==encode(reducedFavorite),"Reduced motion changes the blocked favourite sale");
 input.click(food);check(!care.toast().visible(),"Changing tools leaves a stale toast");
 // Reaching Junior while the net is active unlocks a sale without reselecting it.
 auto threshold=base;auto& young=threshold.fish.front();young.growthMs=young.purchase.durationMs/10000*young.purchase.stages[1]-1;young.lastFedAt=threshold.simNow;
 reset(threshold);input.click(net);input.tap(water);check(session.domain().fish(id)&&!care.detailsOpen(),"Baby sold just before Junior");
 session.domain().advanceCare(1);check(session.domain().fish(id)->age==1&&nextStageProgress(*session.domain().fish(id))==0.,"Stage meter did not reset at Junior");
 input.tap(water);check(!session.domain().fish(id)&&care.tool()==Tool::Sell,"Junior needs the net to be reselected");
 // Selling uses the live reward if the fish grows while its details are open.
 auto growing=base;testing::stage(growing.fish.front(),3);reset(growing);open();
 auto grown=session.domain().state();testing::stage(grown.fish.front(),4);session.domain().install(grown);
 const auto liveReward=fishReward(grown.fish.front());input.click(detail().action);
 check(!care.detailsOpen()&&!session.domain().fish(id)&&session.domain().state().wallet.coins==grown.wallet.coins+liveReward.coins()&&session.domain().state().xp==grown.xp+liveReward.xp,"Sell did not settle the current reward in one tap");
 // Eggs cannot be fed or refunded through the selling controls.
 auto egg=base;egg.fish.front().egg=true;egg.fish.front().hatchAt=egg.simNow+content.hatchMs;egg.fish.front().growthMs=0;reset(egg);open();
 const auto eggBefore=encode(session.domain().state());input.click(detail().action);input.click(detail().action);check(care.detailsOpen()&&encode(session.domain().state())==eggBefore,"Egg sale changed its state");
 // The first finger owns the action. SDL's compatibility mouse events are ignored.
 reset(base);input.finger(SDL_EVENT_FINGER_DOWN,center(food),7);input.finger(SDL_EVENT_FINGER_MOTION,water,7);input.finger(SDL_EVENT_FINGER_DOWN,water,8);input.finger(SDL_EVENT_FINGER_MOTION,emptyPoint,8);input.finger(SDL_EVENT_FINGER_UP,emptyPoint,8);
 check(session.domain().pellets().size()==1,"A second finger adds food");care.advance(.16);
 check(session.domain().pellets().size()==3,"A second finger stops the first finger's stream");
 input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,water,false,SDL_BUTTON_LEFT,SDL_TOUCH_MOUSEID);input.finger(SDL_EVENT_FINGER_UP,water,7);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water,false,SDL_BUTTON_LEFT,SDL_TOUCH_MOUSEID);
 care.advance(.2);check(session.domain().pellets().size()==3,"Touch compatibility events duplicate feeding or keep the stream running");
 input.finger(SDL_EVENT_FINGER_DOWN,center(food),9);input.finger(SDL_EVENT_FINGER_MOTION,water,9);input.finger(SDL_EVENT_FINGER_CANCELED,water,9);input.finger(SDL_EVENT_FINGER_UP,water,9);
 care.advance(.2);check(session.domain().pellets().size()==4,"Cancelled touch keeps pouring food");
 input.mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,center(food));input.mouse(SDL_EVENT_MOUSE_MOTION,water);SDL_Event focus{};focus.type=SDL_EVENT_WILL_ENTER_BACKGROUND;input.send(focus);input.mouse(SDL_EVENT_MOUSE_BUTTON_UP,water);
 care.advance(.2);check(session.domain().pellets().size()==5,"Backgrounded gesture keeps pouring food");
 // Eat a real pellet, with the normal movement simulation and no direct Feed shortcut.
 reset(base);input.drag(center(food),water);const auto wallet=session.domain().state().wallet;
 for(int i=0;i<100&&session.domain().fish(id)->lastFedAt<base.simNow;++i)session.domain().stepMovement(.02,care.tool(),care.held());
 check(session.domain().fish(id)->lastFedAt==base.simNow&&session.domain().pellets().empty()&&session.domain().state().wallet.coins==wallet.coins,"A hungry fish cannot eat free food");
 session.domain().advanceCare(1000);check(session.domain().fish(id)->growthMs==1000,"Feeding does not resume growth");
 // Full tanks do not block care, and cancelling placement does not charge for an egg.
 FishPlacement placement;reset(base);check(bool(startFishPlacement(session.domain(),placement,"neonTetra")),"Placement setup failed");const auto beforeCancel=encode(session.domain().state());cancelFishPlacement(placement);
 check(!placement.active()&&encode(session.domain().state())==beforeCancel,"Switching away from placement costs coins");
 auto full=base;testing::stage(full.fish.front(),1);full.tanks.front().slots=int(full.fish.size());reset(full);input.drag(center(food),water);check(session.domain().pellets().size()==1,"Full tank blocks feeding");
 for(int i=1;i<48;++i)session.domain().execute({.action=Action::DropFood,.point={500,320}});
 input.drag(emptyPoint,water);check(session.domain().pellets().size()==48&&!care.notice().empty(),"Pellet limit has no feedback");
 input.key(SDLK_ESCAPE);open();input.click(detail().action);check(session.domain().living({1})==0,"Full tank blocks selling");
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
void toastArtwork(const std::filesystem::path& assets){
 const auto path=assets/"hud-icons/error-toast-bottle-v1.png";
 std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> art(IMG_Load(path.string().c_str()),SDL_DestroySurface);
 check(bool(art),"Cannot load bottle toast artwork");
 Uint8 alpha{};
 for(const auto [x,y]:{std::pair{0,0},std::pair{art->w-1,0},std::pair{0,art->h-1},std::pair{art->w-1,art->h-1}}){
  check(SDL_ReadSurfacePixel(art.get(),x,y,nullptr,nullptr,nullptr,&alpha)&&alpha==0,"Bottle toast has an opaque backdrop");
 }
 check(SDL_ReadSurfacePixel(art.get(),art->w/2,art->h/2,nullptr,nullptr,nullptr,&alpha)&&alpha==255,"Bottle toast lost its parchment surface");
 std::cout<<"PASS transparent bottle toast artwork\n";
}
void toastGeometry(){
 for(const auto [w,h]:{std::pair{1672.f,941.f},std::pair{1088.f,635.f},std::pair{667.f,375.f},std::pair{390.f,844.f},std::pair{320.f,568.f},std::pair{852.f,393.f}}){
  const Insets safe=w==852?Insets{59,0,59,21}:Insets{};
  const auto l=layoutHudToast(w,h,safe);
  check(l.frame.x>=safe.left&&l.frame.y>=safe.top&&l.frame.x+l.frame.w<=w-safe.right&&l.frame.y+l.frame.h<=h-safe.bottom,"Toast leaves the safe viewport");
  check(l.close.w>=44&&l.close.h>=44,"Toast close target is too small for touch");
  check(l.frame.w<=288.01f&&l.frame.h<=68,"Toast becomes a large banner instead of a compact notice");
  if(w>=600)check(l.frame.w<=(w-safe.left-safe.right)*.5f,"Toast takes more than half a landscape viewport");
  check(l.frame.has(l.close.x,l.close.y)&&l.frame.has(l.close.x+l.close.w,l.close.y+l.close.h),"Toast close target leaves its frame");
  check(l.title.w>0&&l.title.x+l.title.w<l.close.x&&l.body.x+l.body.w<l.close.x,"Toast message overlaps the close target");
  const auto hud=layoutHud(w,h,safe,44);
  const auto settings=hud[HudPart::Settings];
  check(std::abs(l.frame.x+l.frame.w-std::min(settings.x+settings.w,w-safe.right-12))<.01f,"Toast is not anchored to the right below Settings");
  check(std::abs(l.frame.y-settings.y-settings.h-8)<.01f,"Toast is not directly below Settings");
  for(const auto part:{HudPart::Profile,HudPart::Coins,HudPart::Pearls,HudPart::Settings}){
   const auto box=hud[part];check(l.frame.y>=box.y+box.h,"Toast hides the top HUD");
  }
 }
 std::cout<<"PASS toast safe edges, readable text area and touch targets\n";
}
void toastMotion(){
 const auto base=layoutHudToast(667,375,{});HudToast toast;
 toast.show("This fish is a favourite","Unfavourite it before selling.");
 const auto start=toast.animatedLayout(base);
 check(start.frame.y+start.frame.h<=0,"Toast does not enter from above the window");
 toast.advance(.12);const auto entering=toast.animatedLayout(base);
 check(entering.frame.y>start.frame.y&&entering.frame.y<base.frame.y,"Toast does not drop toward its resting position");
 check(std::abs((entering.close.y-base.close.y)-(entering.frame.y-base.frame.y))<.001f,"Toast close target does not follow its animation");
 toast.advance(.18);const auto bounce=toast.animatedLayout(base);
 check(bounce.frame.y>base.frame.y&&bounce.frame.y-base.frame.y<base.frame.h*.25f,"Toast is missing its small landing bounce");
 toast.advance(.2);check(std::abs(toast.animatedLayout(base).frame.y-base.frame.y)<.001f,"Toast fails to settle after its entry");
 toast.advance(2.9);check(toast.visible()&&toast.animatedLayout(base).frame.y<base.frame.y,"Toast does not rise during its exit");
 toast.advance(.11);check(!toast.visible(),"Toast does not expire after 3.5 seconds");
 toast.show("Title","Message");toast.advance(0,true);
 check(toast.animatedLayout(base).frame.y==base.frame.y,"Reduced motion still moves the toast");
 toast.advance(3.51,true);check(!toast.visible(),"Reduced motion changes the toast lifetime");
 toast.show("Title","Message");toast.advance(.12,false);
 SDL_Event e{};e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.button=SDL_BUTTON_LEFT;
 const auto moving=toast.animatedLayout(base);
 check(toast.event(e,center(moving.close),moving,44),"Animated toast close cannot be pressed");
 e.type=SDL_EVENT_MOUSE_BUTTON_UP;
 check(toast.event(e,center(moving.close),moving,44)&&!toast.visible(),"Animated toast close cannot be dismissed");
 std::cout<<"PASS toast drop, bounce, exit, short lifetime and reduced motion\n";
}
void popoverGeometry(){
 for(const auto [w,h]:{std::pair{1642.f,958.f},std::pair{667.f,375.f},std::pair{390.f,844.f},std::pair{852.f,393.f}}){
  const Insets safe=w==852?Insets{59,0,59,21}:Insets{};
  for(const auto [x,y]:{std::pair{.12f,.18f},std::pair{.88f,.18f},std::pair{.12f,.85f},std::pair{.88f,.85f},std::pair{.5f,.5f}}){
   const Rect fish{w*x-24,h*y-14,48,28};
   for(bool extended:{false,true}){
    const auto l=layoutFishCare(w,h,safe,fish,44,extended);const auto r=l.dialog.frame;
    check(r.x>=safe.left&&r.y>=safe.top&&r.x+r.w<=w-safe.right+.01f&&r.y+r.h<=h-safe.bottom+.01f,"Popover clips a safe edge");
    for(const auto control:{l.favorite,l.dialog.close,l.action}){
     check(r.has(control.x,control.y)&&r.has(control.x+control.w,control.y+control.h),"Popover control leaves its frame");
     check(control.w>=43.9f&&control.h>=43.9f,"Popover control is too small for touch");
    }
    if(extended)check(l.breakdown.y+l.breakdown.h<=l.note.y&&l.note.y+l.note.h<l.action.y,"Growth badge overlaps rewards or actions");
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
 input.key(SDLK_F);input.drag(canvas.toScreen({544,450}),point);for(int i=0;i<100;++i)session.update(20,1000,care.tool());
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
 fail.click(blockedCare.detailsLayout().action);
 check(blockedCare.detailsOpen()&&!blockedCare.notice().empty()&&encode(blocked.domain().state())==before,"Failed popover sale loses a fish or closes its retry controls");
 blockedCare.advance(0);check(!blockedCare.rewards().active(),"A failed save animated an uncommitted reward");
 std::filesystem::remove_all(root);
 std::cout<<"PASS feeding persistence, rehome replay and save failure rollback\n";
}
}
int main(int argc,char** argv){try{
 const auto assets=std::filesystem::absolute(argc>1?argv[1]:"assets");std::ifstream file(assets/"content.json");const auto content=Content::fromJson(Json::parse(file));
 const std::filesystem::path captures=argc>2?argv[2]:"";
 popoverGeometry();toastGeometry();toastMotion();toastArtwork(assets);
 for(const auto [w,h]:{std::pair{667,375},std::pair{1024,768},std::pair{390,844}})viewport(content,assets,w,h,captures);
 viewport(content,assets,852,393,captures,{59,0,59,21},2);
 if(!captures.empty()){
  viewport(content,assets,1672,941,captures);
  viewport(content,assets,617,316,captures,{},2);
 }
 storage(content,assets);popoverCapture(content,assets,captures);return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
