#include "aquarium/hud_decor_placement.hpp"
#include "aquarium/hud_care.hpp"
#include "fixtures.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
bool same(WorldPoint a,WorldPoint b){return std::abs(a.x-b.x)<.001&&std::abs(a.y-b.y)<.001;}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
bool inside(Rect a,Rect b){return a.x>=b.x-.01&&a.y>=b.y-.01&&a.x+a.w<=b.x+b.w+.01&&a.y+a.h<=b.y+b.h+.01;}
bool overlaps(Rect a,Rect b){return a.x<b.x+b.w&&a.x+a.w>b.x&&a.y<b.y+b.h&&a.y+a.h>b.y;}
void anchoredControls(const Content& content,const std::filesystem::path& assets,const std::filesystem::path& captures){
 // Match the small Retina window in the reported pebble placement screenshot.
 Session session(content,"/tmp/fishius-decor-anchor-unused.json",1000,true);DecorPlacement state;
 Canvas canvas(assets,424,221,true);canvas.previewViewport(PreviewViewport{424,221,2,{}});canvas.begin();
 check(bool(startDecorPlacement(session.domain(),state,"CD-01")),"Cannot prepare the control anchor preview");
 state.preview.position=canvas.decorProjection().toWorld({canvas.width()*.2555f,canvas.height()*.9f});
 const auto actual=layoutDecorPlacement(canvas,session.domain(),state);
 if(!captures.empty()){
  canvas.scene(session.domain(),1,0,Tool::Select,{},false,{},&state.preview,0,true);
  paintDecorPlacement(canvas,session,state,actual);std::filesystem::create_directories(captures);
  check(canvas.capture(captures/"CD-01-small-window-anchor.png"),"Cannot capture the control anchor preview");
 }
 check(actual.cancel.y+actual.cancel.h<=actual.item.y,"Near the floor, the controls drift sideways instead of sitting above the item");
 check(std::abs((actual.cancel.x+actual.place.x+actual.place.w)*.5f-center(actual.item).x)<.01f,"The visible control pair is not centered on its item");
 for(const auto item:{Rect{94,50,35,25},Rect{94,150,35,25},Rect{2,190,35,25},Rect{387,190,35,25}}){
  const auto l=layoutDecorPlacement(424,221,{},44,item);
  const bool above=l.cancel.y+l.cancel.h<=item.y;
  const bool below=l.cancel.y>=item.y+item.h;
  check(above||below,"The action row moves sideways when there is vertical room");
  check((above?item.y-l.cancel.y-l.cancel.h:l.cancel.y-item.y-item.h)<=4.01f,"Invisible padding separates the controls from the item");
  const auto last=l.place;
  if(item.x>40&&item.x+item.w<384)
   check(std::abs((l.cancel.x+last.x+last.w)*.5f-center(item).x)<.01f,"The visible action row drifts off the item center");
  for(const auto [icon,target]:{std::pair{l.cancel,l.cancelHit},std::pair{l.place,l.placeHit}})
   check(inside(target,l.water)&&inside(icon,target)&&target.h>=44&&!overlaps(target,item),"A nearby action loses its touch area or covers the item");
  check(!overlaps(l.cancelHit,l.placeHit),"Nearby action targets overlap");
 }
 // Tall items still need the side fallback when neither vertical position fits.
 const Rect tall{180,6,50,209};const auto side=layoutDecorPlacement(424,221,{},44,tall);
 check(inside(side.cancelHit,side.water)&&inside(side.placeHit,side.water)&&!overlaps(side.cancelHit,tall)&&!overlaps(side.placeHit,tall),"A tall item loses its usable side controls");
 std::cout<<"PASS controls stay centered near small items and remain reachable at tank edges\n";
}
struct Input {
 Session& session;DecorPlacement& state;Canvas& canvas;HudPointer pointer;HudCare* care{};
 DecorPlacementLayout layout()const{return layoutDecorPlacement(canvas,session.domain(),state);}
 PlacementEvent send(Uint32 type,SDL_FPoint p={},bool touch=false,Uint8 button=SDL_BUTTON_LEFT){
  const auto current=layout();
  int width{},height{};SDL_GetWindowSize(canvas.window(),&width,&height);
  SDL_Event e{};e.type=type;
  if(touch){e.tfinger.fingerID=1;e.tfinger.x=p.x/current.page.w;e.tfinger.y=p.y/current.page.h;}
  else if(type==SDL_EVENT_MOUSE_MOTION){e.motion.x=p.x/current.page.w*width;e.motion.y=p.y/current.page.h*height;}
  else if(type==SDL_EVENT_MOUSE_BUTTON_DOWN||type==SDL_EVENT_MOUSE_BUTTON_UP){e.button.button=button;e.button.x=p.x/current.page.w*width;e.button.y=p.y/current.page.h*height;}
  if(!normalizeHudPointer(pointer,e,float(width),float(height)))return PlacementEvent::None;
  if(state.active())return decorPlacementEvent(session,state,current,e,p);
  if(care){care->event(e,pointer.touch);care->beginDecorMove(state);}
  return PlacementEvent::None;
 }
 PlacementEvent tap(SDL_FPoint p,bool touch=false){send(touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,p,touch);return send(touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP,p,touch);}
 PlacementEvent key(SDL_Keycode key,bool repeat=false){
  SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=key;e.key.repeat=repeat;
  if(state.active()&&!repeat&&(key==SDLK_F||key==SDLK_S))cancelDecorPlacement(state);
  if(state.active())return decorPlacementEvent(session,state,layout(),e,{});
  if(care)care->event(e,false);return PlacementEvent::None;
 }
};
void viewport(const Content& content,const std::filesystem::path& assets,const std::filesystem::path& captures,int w,int h,Insets safe={}){
 Session session(content,"/tmp/fishius-decor-ui-unused.json",1000,true);DecorPlacement state;
 Canvas canvas(assets,w,h,true);canvas.previewViewport(PreviewViewport{w,h,1,safe});canvas.begin();
 const auto layout=layoutDecorPlacement(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());Input input{session,state,canvas,{}};
 check(inside(layout.cancel,layout.water)&&inside(layout.place,layout.water),"Placement controls cross the safe area");
 const auto hud=layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
 for(const auto tile:{layout.cancel,layout.place})for(const auto part:{HudPart::Food,HudPart::Rehome})
  check(std::abs(tile.w-hud[part].w)<.01&&std::abs(tile.h-hud[part].h)<.01,"Placement icon size differs from Food or Rehome");
 check(inside(layout.cancelHit,layout.water)&&inside(layout.placeHit,layout.water)&&layout.cancelHit.h>=canvas.minimumTouchSize()-.01&&layout.placeHit.h>=canvas.minimumTouchSize()-.01&&layout.cancelHit.x+layout.cancelHit.w<layout.placeHit.x,"Placement targets cross the safe area, overlap or are too small");
 const auto base=session.domain().state();
 for(bool touch:{false,true})for(const auto* id:{"CP-01","CD-01"}){
  session.domain().install(base);input.pointer={};const auto before=encode(base);
  const auto receiptCount=state.receipts.size();
  const auto* def=content.findDecor(id);
  check(bool(startDecorPlacement(session.domain(),state,id)),"Cannot select a plant or decoration");
  check(state.preview.id==base.nextDecorId&&state.preview.tank==base.activeTank&&encode(session.domain().state())==before,"Selecting decor changes progress or uses the wrong preview identity");
  input.send(SDL_EVENT_MOUSE_BUTTON_UP,center(input.layout().place));
  check(state.active()&&encode(session.domain().state())==before,"Opening Shop release buys a decoration");
  const SDL_FPoint top{layout.water.x+layout.water.w*.5f,layout.water.y+12};
  input.tap(top,touch);
  check(same(state.preview.position,layout.scene.place(*def,layout.scene.toWorld(top))),"The removed heading still blocks placement at the top of the tank");
  const SDL_FPoint water{layout.page.w*.3f,layout.page.h*.7f},destination{layout.page.w*.7f,layout.page.h*.8f};
  input.tap(water,touch);
  check(state.active()&&same(state.preview.position,layout.scene.place(*def,layout.scene.toWorld(water)))&&encode(session.domain().state())==before,"Positioning buys decor or uses the wrong coordinates");
  const auto down=touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN;
  const auto motion=touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION;
  const auto up=touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP;
  input.send(down,water,touch);input.send(motion,destination,touch);input.send(up,destination,touch);
  const auto preview=state.preview;
  check(same(preview.position,layout.scene.place(*def,layout.scene.toWorld(destination)))&&encode(session.domain().state())==before,"Dragging buys decor or fails to move its preview");
  // An interrupted drag, a release outside the tank and a release over Place keep the old preview.
  for(int interrupted=0;interrupted<3;++interrupted){
   input.send(down,destination,touch);input.send(motion,water,touch);
   if(interrupted==0){input.send(touch?SDL_EVENT_FINGER_CANCELED:SDL_EVENT_WINDOW_FOCUS_LOST,water,touch);input.send(up,water,touch);}
   else input.send(up,interrupted==1?SDL_FPoint{-10,-10}:center(input.layout().place),touch);
   check(same(state.preview.position,preview.position)&&encode(session.domain().state())==before,"Interrupted movement changes its anchor or buys decor");
  }
  // A button press dragged away and back is not confirmation.
  input.send(down,center(input.layout().place),touch);input.send(motion,water,touch);input.send(up,center(input.layout().place),touch);
  check(state.active()&&encode(session.domain().state())==before,"Dragging Place purchases an item");
  input.send(down,center(input.layout().place),touch);input.send(SDL_EVENT_WINDOW_RESIZED);input.send(up,center(input.layout().place),touch);
  check(state.active()&&encode(session.domain().state())==before,"Resize retains a stale confirmation press");
  input.send(SDL_EVENT_MOUSE_BUTTON_DOWN,center(input.layout().place),false,SDL_BUTTON_RIGHT);input.send(SDL_EVENT_MOUSE_BUTTON_UP,center(input.layout().place),false,SDL_BUTTON_RIGHT);
  check(state.active()&&encode(session.domain().state())==before,"Right click purchases decor");
  check(input.tap(center(input.layout().place),touch)==PlacementEvent::Placed&&!state.active(),"Place does not complete the purchase");
  const auto& bought=session.domain().state();const auto* placed=session.domain().decoration(preview.id);
  check(placed&&placed->kind==id&&same(placed->position,preview.position)&&placed->sizeMul==preview.sizeMul,"Confirmed item differs from the preview");
  check(bought.decor.size()==base.decor.size()+1&&bought.wallet.coins==base.wallet.coins-def->price&&bought.wallet.pearls==base.wallet.pearls&&bought.xp==base.xp+def->buyXp,"Purchase does not charge once and grant the first-copy XP");
  check(state.receipts.size()==receiptCount+1&&state.receipts.back().cost==def->price&&!state.receipts.back().pearl&&state.receipts.back().xp==def->buyXp&&same(state.receipts.back().point,preview.position),"Confirmed decor loses its floating purchase receipt or shows the wrong amount");
  check(encode(decodeAndValidate(encode(bought),content))==encode(bought),"Placed decor does not survive save validation");
  input.send(up,center(input.layout().place),touch);
  check(session.domain().state().decor.size()==base.decor.size()+1,"Duplicate release buys twice");
  check(state.receipts.size()==receiptCount+1,"Duplicate release shows a second purchase receipt");
  const auto after=encode(session.domain().state());
  check(bool(startDecorPlacement(session.domain(),state,id)),"Cannot start another preview");
  check(input.tap(center(input.layout().cancel),touch)==PlacementEvent::Cancelled&&!state.active()&&encode(session.domain().state())==after,"Cancel changes currency or ownership");
  startDecorPlacement(session.domain(),state,id);input.key(SDLK_ESCAPE);
  check(!state.active()&&encode(session.domain().state())==after,"Escape changes currency or ownership");
  check(state.receipts.size()==receiptCount+1,"Cancelling a preview creates or drops a purchase receipt");
  startDecorPlacement(session.domain(),state,id);
  const auto original=state.preview.position;const auto item=input.layout().item;const auto grab=center(item);
  const SDL_FPoint target{grab.x+layout.page.w*.1f,grab.y};
  input.send(down,grab,touch);input.send(motion,target,touch);input.send(up,target,touch);
  const auto from=layout.scene.toWorld(grab),to=layout.scene.toWorld(target);
  check(same(state.preview.position,layout.scene.place(*def,{original.x+to.x-from.x,original.y+to.y-from.y})),"Grabbing a new preview jumps to its bottom anchor");
  cancelDecorPlacement(state);
 }
 advancePlacementReceipts(state.receipts,1);
 check(!state.receipts.empty()&&state.receipts.front().age==1,"Decor purchase feedback does not animate after placement ends");
 advancePlacementReceipts(state.receipts,.61);check(state.receipts.empty(),"Decor purchase feedback never expires");
 HudCare care(canvas,session);input.care=&care;
 for(bool touch:{false,true})for(const auto* kind:{"CP-01","CD-01"}){
  auto empty=base;empty.fish.clear();session.domain().install(empty);
  check(bool(session.command({.action=Action::BuyDecor,.key=kind,.point=layout.scene.place(*content.findDecor(kind),{340,430},1.24)})),"Cannot prepare a placed item to move");
  const auto id=session.domain().state().nextDecorId-1;
  session.command({.action=Action::FlipDecor,.decor=id});session.command({.action=Action::ResizeDecor,.value=.24,.decor=id});
  const auto owned=session.domain().state();const auto original=*session.domain().decoration(id);const auto before=encode(owned);
  const auto* def=content.findDecor(kind);const auto bounds=canvas.decorRect(*def,original.position,original.sizeMul);
  const SDL_FPoint grab{bounds.x+bounds.w*.35f,bounds.y+bounds.h*.45f};
  const SDL_FPoint destination{grab.x+canvas.width()*.15f,grab.y+canvas.height()*.1f};
  const auto from=layout.scene.toWorld(grab),to=layout.scene.toWorld(destination);
  const auto expected=layout.scene.place(*def,{original.position.x+to.x-from.x,original.position.y+to.y-from.y},original.sizeMul);
  const auto down=touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN;
  const auto motion=touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION;
  const auto up=touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP;
  const auto reset=[&](){care.reset();cancelDecorPlacement(state);input.pointer={};session.domain().install(owned);};
  const auto drag=[&](){input.send(down,grab,touch);input.send(motion,destination,touch);input.send(up,destination,touch);};
  const auto select=[&](){input.tap(grab,touch);check(care.selectedDecor()==id&&!state.active(),"A tap does not leave a placed item selected");};
  reset();drag();check(!state.active()&&encode(session.domain().state())==before,"Dragging an unselected decoration starts a move");
  select();input.send(down,grab,touch);input.send(motion,{grab.x+layout.dragThreshold*.25f,grab.y},touch);input.send(up,grab,touch);
  check(care.selectedDecor()==id&&!state.active(),"A slight tap movement opens the move controls");
  drag();
  check(state.active()&&state.moving&&state.preview.id==id&&!care.selectedDecor(),"Dragging a selected item does not open its move preview");
  check(same(state.preview.position,expected)&&state.preview.flipped==original.flipped&&state.preview.sizeMul==original.sizeMul,"Dragging jumps to the bottom anchor or changes the item's size or flip");
  check(same(session.domain().decoration(id)->position,expected),"Dropping an owned item does not save its position immediately");
  check(inside(input.layout().cancelHit,input.layout().water)&&inside(input.layout().placeHit,input.layout().water),"Move controls leave the safe area");
  if(!captures.empty()&&!touch){
   std::filesystem::create_directories(captures);canvas.begin();canvas.scene(session.domain(),1,0,Tool::Select,{},false,{},&state.preview);paintDecorPlacement(canvas,session,state,input.layout());
   check(canvas.capture(captures/(std::string("move-")+kind+"-"+std::to_string(w)+"x"+std::to_string(h)+".png")),"Cannot capture the move preview");
  }
  const auto dropped=encode(session.domain().state());
  check(input.tap(center(input.layout().place),touch)==PlacementEvent::Placed&&!state.active()&&encode(session.domain().state())==dropped,"The tick changes a saved move instead of just ending editing");
  const auto& moved=session.domain().state();const auto* item=session.domain().decoration(id);
  check(item&&same(item->position,expected)&&item->flipped==original.flipped&&item->sizeMul==original.sizeMul&&item->tank==original.tank,"Saved move changes the item's identity, appearance or tank");
  check(moved.decor.size()==owned.decor.size()&&moved.nextDecorId==owned.nextDecorId&&moved.decorOwned==owned.decorOwned&&moved.wallet.coins==owned.wallet.coins&&moved.wallet.pearls==owned.wallet.pearls&&moved.xp==owned.xp,"Moving buys a duplicate or changes currency or XP");
  check(state.receipts.empty(),"Moving a decoration shows a purchase charge");
  const auto saved=encode(moved);input.send(up,center(input.layout().place),touch);check(encode(session.domain().state())==saved,"A repeated release saves the move again");
  for(int cancel=0;cancel<4;++cancel){
   reset();select();drag();
   const auto beforeExit=encode(session.domain().state());
   if(cancel==0)input.tap(center(input.layout().cancel),touch);
   else input.key(cancel==1?SDLK_ESCAPE:cancel==2?SDLK_F:SDLK_S);
   check(!state.active()&&same(session.domain().decoration(id)->position,expected),"Leaving editing loses the saved drop position");
   if(cancel==0)check(session.domain().decoration(id)->stored&&session.domain().state().decor.size()==owned.decor.size(),"Remove does not return the same owned copy to inventory");
   else check(encode(session.domain().state())==beforeExit,"A tool shortcut or Escape rolls back the saved move");
  }
  for(int interrupted=0;interrupted<4;++interrupted){
   reset();select();input.send(down,grab,touch);input.send(motion,destination,touch);
   if(interrupted==0){input.send(touch?SDL_EVENT_FINGER_CANCELED:SDL_EVENT_WINDOW_FOCUS_LOST,destination,touch);input.send(up,destination,touch);}
   if(interrupted==1)input.send(up,{-10,-10},touch);
   if(interrupted==2)input.send(up,center(input.layout().place),touch);
   if(interrupted==3){input.send(SDL_EVENT_WINDOW_RESIZED);input.send(up,destination,touch);}
   check(state.active()&&same(state.preview.position,original.position)&&encode(session.domain().state())==before,"An interrupted drag saves or keeps the wrong move position");
  }
  reset();select();drag();const auto outsideSaved=encode(session.domain().state());
  input.tap({layout.water.x+2,layout.water.y+layout.water.h-2},touch);
  check(!state.active()&&encode(session.domain().state())==outsideSaved&&same(session.domain().decoration(id)->position,expected),"An outside tap repositions or rolls back an owned item");
  reset();select();drag();const auto edgeGrab=center(input.layout().item);const SDL_FPoint edgeTarget{layout.water.x+2,layout.water.y+layout.water.h-2};
  const auto edgeFrom=layout.scene.toWorld(edgeGrab),edgeTo=layout.scene.toWorld(edgeTarget);
  const auto edge=layout.scene.place(*def,{expected.x+edgeTo.x-edgeFrom.x,expected.y+edgeTo.y-edgeFrom.y},original.sizeMul);
  input.send(down,edgeGrab,touch);input.send(motion,edgeTarget,touch);input.send(up,edgeTarget,touch);
  check(state.active()&&same(session.domain().decoration(id)->position,edge),"Dragging to a tank edge does not clamp and save the footprint");
  input.key(SDLK_RETURN);check(!state.active()&&same(session.domain().decoration(id)->position,edge),"Enter changes the saved move");
  reset();
 }
 // Normal selection can also pick a covered copy, then drag through its occluder.
 for(bool touch:{false,true}){
  auto layered=base;layered.fish.clear();const auto& def=*content.findDecor("CP-02");
  const auto anchor=canvas.decorProjection().place(def,{544,460});
  layered.decor={{501,"CP-02",layered.activeTank,anchor},{502,"CP-02",layered.activeTank,anchor,false,false,1.6}};
  layered.decorOwned={"CP-02"};layered.nextDecorId=503;
  session.domain().install(layered);cancelDecorPlacement(state);care.reset();input.pointer={};
  const auto grab=center(canvas.decorRect(def,anchor));const auto saved=encode(session.domain().state());
  input.tap(grab,touch);check(care.selectedDecor()==502,"Normal selection does not pick the front copy");
  input.tap(grab,touch);check(care.selectedDecor()==501&&encode(session.domain().state())==saved,"Normal selection cannot cycle to a fully covered copy");
  const SDL_FPoint target{grab.x+canvas.width()*.1f,grab.y};
  input.send(touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,grab,touch);
  input.send(touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION,target,touch);
  check(state.active()&&state.preview.id==501&&encode(session.domain().state())==saved,"Normal selection gives a covered item's drag to the foreground copy");
  input.send(touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP,target,touch);
  check(state.active()&&state.preview.id==501&&!same(state.preview.position,anchor)&&same(session.domain().decoration(502)->position,anchor),"A covered move fails to save or moves the foreground item");
  cancelDecorPlacement(state);care.reset();
  auto small=layered;small.decor.resize(1);small.decor[0].sizeMul=.6;
  small.decor[0].position=canvas.decorProjection().place(def,{544,230},.6);
  session.domain().install(small);const auto smallBounds=canvas.decorRect(def,small.decor[0].position,.6);
  input.tap(center(smallBounds),touch);check(care.selectedDecor()==501,"Cannot select a small distant item");
  const auto touchBounds=decorDragBounds(smallBounds,canvas.minimumTouchSize());
  check(touchBounds.w>=canvas.minimumTouchSize()&&touchBounds.h>=canvas.minimumTouchSize(),"A small item's drag target shrinks with perspective");
  const SDL_FPoint paddedGrab{touchBounds.x+touchBounds.w*.5f,touchBounds.y+2},smallTarget{paddedGrab.x+canvas.width()*.1f,paddedGrab.y};
  input.send(touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,paddedGrab,touch);
  input.send(touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION,smallTarget,touch);
  input.send(touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP,smallTarget,touch);
  check(state.active()&&state.preview.id==501&&!same(state.preview.position,small.decor[0].position),"A small item cannot be dragged by its padded touch target");
  const auto activeBounds=input.layout();const SDL_FPoint regrab{activeBounds.itemHit.x+activeBounds.itemHit.w*.5f,activeBounds.itemHit.y+2};
  const auto beforeRegrab=state.preview.position;
  input.send(touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,regrab,touch);
  input.send(touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION,{regrab.x+canvas.width()*.06f,regrab.y},touch);
  input.send(touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP,{regrab.x+canvas.width()*.06f,regrab.y},touch);
  check(state.active()&&!same(state.preview.position,beforeRegrab),"An active distant preview loses its padded drag target");
  cancelDecorPlacement(state);care.reset();
 }
 input.care=nullptr;
 if(!captures.empty()){
  for(const auto* id:{"CP-01","CD-01"}){
   session.domain().install(base);startDecorPlacement(session.domain(),state,id);
   canvas.begin();canvas.scene(session.domain(),1,0,Tool::Select,{},false,{},&state.preview);paintDecorPlacement(canvas,session,state,input.layout());
   std::filesystem::create_directories(captures);
   check(canvas.capture(captures/(std::string(id)+"-"+std::to_string(w)+"x"+std::to_string(h)+".png")),"Cannot capture decor preview");
   check(bool(confirmDecorPlacement(session,state)),"Cannot confirm decor for its receipt capture");
   canvas.begin();canvas.scene(session.domain(),1,0,Tool::Select,{},false);paintHud(canvas,session.domain(),hud);paintPlacementReceipts(canvas,state.receipts,hud.unit);
   check(canvas.capture(captures/(std::string(id)+"-receipt-"+std::to_string(w)+"x"+std::to_string(h)+".png")),"Cannot capture decor purchase feedback");
   advancePlacementReceipts(state.receipts,2);
  }
 }
 std::cout<<"PASS "<<w<<"x"<<h<<" plant/decor mouse and touch purchase, selection, dragging, cancellation and preview bounds\n";
}
}
int main(int argc,char** argv){try{
 using namespace aq;
 const std::filesystem::path assets=argc>1?argv[1]:"assets",captures=argc>2?argv[2]:"";
 std::ifstream in(assets/"content.json");const auto content=Content::fromJson(Json::parse(in));
 anchoredControls(content,assets,captures);
 Session session(content,"/tmp/fishius-decor-unused.json",1000,true);auto& domain=session.domain();DecorPlacement state;
 for(auto category:{ShopCategory::Plants,ShopCategory::Decorations}){
  const auto items=shopItems(domain,{category});check(!items.empty(),"Plant or decoration catalog is empty");
  for(const auto& item:items){
   const auto* def=content.findDecor(item.decorId);
   check(def&&def->name==item.name&&def->asset==item.asset&&(def->category=="Plant")== (category==ShopCategory::Plants),"Catalog card does not retain its decor identity");
   check(item.locked==(def->level>domain.level())&&item.price==compact(def->price)+(def->currency==Currency::Pearls?" Pearls":" Coins"),"Decor card price or lock differs from the domain");
   check(item.purchaseReward=="First buy: +"+std::to_string(def->buyXp)+" XP","Shop does not show the first-copy XP before purchase");
  }
 }
 const auto before=encode(domain.state());
 const auto fresh=domain.state();
 check(bool(startDecorPlacement(domain,state,"CP-01"))&&bool(confirmDecorPlacement(session,state)),"Cannot check an owned item's Shop reward");
 check(shopItems(domain,{ShopCategory::Plants}).front().purchaseReward=="Owned: no bonus XP","Owned Shop item still advertises a new XP reward");
 const auto earned=domain.state().xp;
 check(bool(startDecorPlacement(domain,state,"CP-01"))&&bool(confirmDecorPlacement(session,state)),"Cannot check a repeat purchase");
 check(domain.state().xp==earned&&state.receipts.back().xp==0,"Repeat purchase displays or grants extra XP");
 domain.install(fresh);state.receipts.clear();
 check(startDecorPlacement(domain,state,"missing").error==Error::Unknown&&!state.active(),"Unknown item starts placement");
 check(startDecorPlacement(domain,state,"CP-02").error==Error::Level&&!state.active(),"Locked item starts placement");
 check(encode(domain.state())==before,"Blocked placement changes progress");
 auto high=domain.state();high.wallet={1000000,1000};high.xp=content.levels.back();high.highestRewardedLevel=40;testing::openingBalances(high);domain.install(high);
 check(shopItems(domain,{ShopCategory::Plants}).front().purchaseReward=="Max level","Capped Shop item advertises unavailable account XP");
 auto nearCap=high;nearCap.xp=content.levels.back()-1;nearCap.highestRewardedLevel=39;testing::openingBalances(nearCap);domain.install(nearCap);
 check(shopItems(domain,{ShopCategory::Plants}).front().purchaseReward=="First buy: +1 XP","Shop reward ignores the remaining account XP");
 check(bool(startDecorPlacement(domain,state,"CP-01"))&&bool(confirmDecorPlacement(session,state))&&state.receipts.back().xp==1,"Capped purchase receipt overstates earned XP");
 domain.install(high);
 for(const auto* id:{"PP-01","PD-01"}){
  const auto* def=content.findDecor(id);const auto balance=domain.state().wallet;
  check(bool(startDecorPlacement(domain,state,id))&&bool(confirmDecorPlacement(session,state)),"Premium decor cannot be placed");
  check(domain.state().wallet.coins==balance.coins&&domain.state().wallet.pearls==balance.pearls-def->price,"Premium decor charges the wrong currency");
  check(!state.receipts.empty()&&state.receipts.back().pearl&&state.receipts.back().cost==def->price&&state.receipts.back().xp==0,"Premium decor receipt uses the wrong currency or XP");
 }
 auto poor=high;poor.wallet={0,0};testing::openingBalances(poor);domain.install(poor);
 for(const auto* id:{"CP-01","PP-01"}){
  const auto result=startDecorPlacement(domain,state,id);const auto* def=content.findDecor(id);
  check(result.error==Error::Funds&&!state.active(),"Unaffordable decor does not report a currency shortage");
  check((def->currency==Currency::Coins?result.shortfall.coins:result.shortfall.pearls)==def->price,"Decor shortage loses the exact price");
 }
 domain.install(high);startDecorPlacement(domain,state,"CP-01");domain.install(poor);
 check(confirmDecorPlacement(session,state).error==Error::Funds&&!state.active()&&state.shortfall&&state.shortfall->coins==content.findDecor("CP-01")->price,"Confirmation shortage fails to exit placement and report missing coins");
 domain.install(high);cancelDecorPlacement(state);
 check(startDecorPlacement(domain,state,"PL-11").error==Error::EventClosed,"Closed event decor can be placed");
 for(int i=0;i<content.decorTuning.placedLimit;++i)check(bool(session.command({.action=Action::BuyDecor,.key="CP-01",.point={544,500}})),"Cannot fill the decor limit");
 const auto fullPlacement=startDecorPlacement(domain,state,"CD-01");
 check(fullPlacement.error==Error::Maximum&&!state.active(),"Full tank permits decor placement");
 check(fullPlacement.message=="Store a decoration first. This tank has 64 placed items.","Full tank message uses an outdated decor limit");
 auto full=domain.state();full.wallet={0,0};testing::openingBalances(full);domain.install(full);
 const auto existing=full.decor.front().id;
 check(bool(startDecorMove(domain,state,existing))&&state.moving,"An empty wallet or full tank blocks moving an owned decoration");
 state.preview.position={350,400};check(bool(saveDecorMove(session,state))&&bool(confirmDecorPlacement(session,state))&&domain.state().decor.size()==full.decor.size()&&domain.state().wallet.coins==0&&domain.state().wallet.pearls==0,"A free move changes currency or creates a copy");
 auto closed=high;closed.decor={{901,"PL-11",closed.activeTank,{544,500}}};closed.decorOwned={"PL-11"};closed.nextDecorId=902;domain.install(closed);
 check(bool(startDecorMove(domain,state,901)),"A closed event blocks moving an owned event item");state.preview.position={350,400};
 check(bool(saveDecorMove(session,state))&&bool(confirmDecorPlacement(session,state))&&domain.state().decor.size()==1,"An owned event decoration cannot be moved");
 check(!startDecorMove(domain,state,9999)&&!state.active(),"An unknown decoration can be moved");
 for(int hiddenItem=0;hiddenItem<4;++hiddenItem){
  domain.install(closed);check(bool(startDecorMove(domain,state,901)),"Cannot prepare an owned item move");auto changed=closed;
  if(hiddenItem==0)changed.decor.front().stored=true;
  if(hiddenItem==1)changed.decor.clear();
  if(hiddenItem==2){changed.tanks.push_back({{2},10});changed.decor.front().tank={2};}
  if(hiddenItem==3){changed.tanks.push_back({{2},10});changed.activeTank={2};}
  domain.install(changed);const auto unchanged=encode(domain.state());
  check(!confirmDecorPlacement(session,state)&&!state.active()&&encode(domain.state())==unchanged,"A stale move recreates or transfers a hidden item");
  check(!startDecorMove(domain,state,901)&&!state.active(),"A hidden or removed item starts a new move");
 }
 domain.install(high);startDecorPlacement(domain,state,"CP-01");
 auto switched=high;switched.tanks.push_back({{2},10});switched.activeTank={2};domain.install(switched);
 check(!confirmDecorPlacement(session,state)&&!state.active()&&domain.state().decor.size()==high.decor.size(),"Preview purchases into a different tank");
 auto noArt=content;noArt.decorations.front().artReady=false;Domain hidden(noArt);
 check(startDecorPlacement(hidden,state,noArt.decorations.front().id).error==Error::NoArt,"Missing decor art permits placement");
 std::cout<<"PASS catalog identity, price, currency, level, art, event and tank limits\n";
 // A real save failure must roll the purchase back while retaining its preview for retry.
 const auto temp=std::filesystem::temp_directory_path()/("fishius-decor-placement-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(temp);const auto parent=temp/"blocked",save=parent/"save.json";
 Session durable(content,save,1000);{std::ofstream blocked(parent);blocked<<"blocks the save directory";}
 const auto durableBefore=encode(durable.domain().state());startDecorPlacement(durable.domain(),state,"CP-01");
 const auto receiptCount=state.receipts.size();
 check(confirmDecorPlacement(durable,state).error==Error::SaveFailure&&state.active()&&!state.error.empty()&&durable.saveFailed()&&encode(durable.domain().state())==durableBefore,"Save failure loses the preview or charges currency");
 check(state.receipts.size()==receiptCount,"A failed save shows a purchase receipt");
 std::filesystem::remove(parent);check(bool(confirmDecorPlacement(durable,state))&&!state.active()&&!durable.saveFailed(),"Failed purchase cannot be retried");
 check(state.receipts.size()==receiptCount+1,"Retrying a purchase loses or duplicates its receipt");
 Storage storage(save);const auto loaded=storage.load(content);check(loaded.state&&encode(*loaded.state)==encode(durable.domain().state()),"Confirmed decor was not saved");
 // A failed move keeps the owned copy at its saved anchor and can be retried.
 const auto moveParent=temp/"blocked-move",moveSave=moveParent/"save.json";
 Session durableMove(content,moveSave,1000);durableMove.domain().install(durable.domain().state());
 {std::ofstream blocked(moveParent);blocked<<"blocks the save directory";}
 const auto ownedBefore=encode(durableMove.domain().state());const auto moveId=durableMove.domain().state().decor.back().id;
 check(bool(startDecorMove(durableMove.domain(),state,moveId)),"Cannot prepare a durable move");state.preview.position={350,450};
 const auto movedPreview=state.preview;
 check(saveDecorMove(durableMove,state).error==Error::SaveFailure&&state.active()&&state.moving&&!state.error.empty()&&same(state.preview.position,durableMove.domain().decoration(moveId)->position)&&encode(durableMove.domain().state())==ownedBefore,"A failed drop does not return to the saved position");
 std::filesystem::remove(moveParent);state.preview.position=movedPreview.position;
 check(bool(saveDecorMove(durableMove,state))&&state.active(),"A failed move cannot be retried with another drop");
 const auto movedSave=Storage(moveSave).load(content);
 check(movedSave.state&&encode(*movedSave.state)==encode(durableMove.domain().state())&&same(durableMove.domain().decoration(moveId)->position,movedPreview.position),"The confirmed move does not survive reload");
 // Once the drop is saved, the tick must not need another write.
 std::filesystem::rename(moveParent,temp/"previous-move");{std::ofstream blocked(moveParent);blocked<<"blocks another write";}
 const auto savedDrop=encode(durableMove.domain().state());
 check(bool(confirmDecorPlacement(durableMove,state))&&!state.active()&&!durableMove.saveFailed()&&encode(durableMove.domain().state())==savedDrop,"The tick writes or rolls back an already saved drop");
 std::filesystem::remove_all(temp);
 std::cout<<"PASS purchase and move save rollback, retry, free ownership and persisted positions\n";
 viewport(content,assets,captures,1608,908);viewport(content,assets,captures,667,375);viewport(content,assets,captures,852,393,{44,0,44,21});viewport(content,assets,captures,1024,768);viewport(content,assets,captures,390,844);
 return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
