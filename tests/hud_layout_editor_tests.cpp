#include "aquarium/hud_layout_editor.hpp"
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
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
bool same(WorldPoint a,WorldPoint b){return std::abs(a.x-b.x)<.001&&std::abs(a.y-b.y)<.001;}
bool inside(Rect a,Rect b){return a.x>=b.x-.1f&&a.y>=b.y-.1f&&a.x+a.w<=b.x+b.w+.1f&&a.y+a.h<=b.y+b.h+.1f;}
std::size_t stackCount(const HudLayoutEditor& editor,std::string_view kind){
 for(const auto& stack:editor.storedStacks())if(stack.kind==kind)return stack.count;
 return 0;
}
State owned(const Content& content){
 Domain domain(content,1000);auto s=domain.state();
 s.wallet={1000000,1000};s.xp=content.levels.back();s.highestRewardedLevel=40;
 s.tanks.push_back({{2},10});
 s.decor={{501,"CP-02",{1},{330,430},false,true,1.2},{502,"CD-01",{1},{740,510}},{503,"PP-01",{2},{500,440},true,true,1.4}};
 s.decorOwned={"CP-02","CD-01","PP-01"};s.nextDecorId=504;testing::openingBalances(s);return s;
}
struct Input {
 Canvas& canvas;HudLayoutEditor& editor;HudCare& care;HudPointer pointer;
 void send(Uint32 type,SDL_FPoint point={},bool touch=false,Uint8 button=SDL_BUTTON_LEFT,Uint64 timestamp=1000000000){
  int w{},h{};SDL_GetWindowSize(canvas.window(),&w,&h);SDL_Event e{};e.type=type;e.common.timestamp=timestamp;
  if(touch){e.tfinger.fingerID=1;e.tfinger.x=point.x/canvas.width();e.tfinger.y=point.y/canvas.height();}
  else if(type==SDL_EVENT_MOUSE_MOTION){e.motion.x=point.x/canvas.width()*w;e.motion.y=point.y/canvas.height()*h;}
  else if(type==SDL_EVENT_MOUSE_BUTTON_DOWN||type==SDL_EVENT_MOUSE_BUTTON_UP){e.button.button=button;e.button.x=point.x/canvas.width()*w;e.button.y=point.y/canvas.height()*h;}
  if(!normalizeHudPointer(pointer,e,float(w),float(h)))return;
  const bool wasOpen=editor.active(),handled=editor.event(e);
  if(!wasOpen&&editor.active())care.reset();
  if(!handled)care.event(e,pointer.touch);
 }
 void tap(SDL_FPoint p,bool touch=false){send(touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,p,touch);send(touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP,p,touch);}
 void click(Rect r,bool touch=false){tap(center(r),touch);}
 void drag(SDL_FPoint from,SDL_FPoint to,bool touch=false){send(touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,from,touch);send(touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION,to,touch);send(touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP,to,touch);}
 Rect card(std::size_t index=0)const{return shopCardBounds(editor.inventoryLayout(),editor.inventoryState(),index);}
 void bag(bool touch=false){
  if(editor.active())click(editor.inventoryOpen()?editor.inventoryLayout().close:editor.layout().exit,touch);
  click(layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize())[HudPart::Bag],touch);
 }
 void key(SDL_Keycode key,bool repeat=false){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=key;e.key.repeat=repeat;if(!editor.event(e))care.event(e,false);}
};
void viewport(const Content& content,const std::filesystem::path& assets,const std::filesystem::path& captures,int w,int h,Insets safe={}){
 Canvas canvas(assets,w,h,true);canvas.previewViewport(PreviewViewport{w,h,1,safe});canvas.begin();
 Session session(content,"/tmp/layout-editor-unused.json",1000,true);HudLayoutEditor editor(canvas,session);HudCare care(canvas,session);Input input{canvas,editor,care,{}};
 auto base=owned(content);const auto projection=canvas.decorProjection();
 // Keep interaction fixtures within this device's crop. Rendering a saved
 // layout across devices is checked separately without moving its anchors.
 for(auto& item:base.decor)if(const auto* def=content.findDecor(item.kind))item.position=projection.place(*def,item.position,item.sizeMul);
 const auto hud=layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
 const auto entry=hud[HudPart::Layout],sell=hud[HudPart::Rehome];
 check(std::abs(entry.w-sell.w)<.01f&&std::abs(entry.h-sell.h)<.01f&&entry.y+entry.h<sell.y&&std::abs(entry.x-sell.x)<.01f,"Layout button does not match Sell or sit directly above it");
 const auto capture=[&](const std::string& name){
  if(captures.empty())return;const auto folder=captures/(std::to_string(w)+"x"+std::to_string(h));std::filesystem::create_directories(folder);
  canvas.begin();canvas.scene(session.domain(),1,0,Tool::Select,{},false,{},editor.preview());
  if(!editor.active()||editor.inventoryOpen())paintHud(canvas,session.domain(),hud);
  if(editor.active())editor.paint();
  check(canvas.capture(folder/(name+".png")),"Cannot capture layout editor");
 };
 session.domain().install(base);capture("button");
 input.send(SDL_EVENT_MOUSE_BUTTON_UP,center(entry));check(!editor.active(),"An unmatched release opens the editor");
 input.send(SDL_EVENT_MOUSE_BUTTON_DOWN,center(entry),false,SDL_BUTTON_RIGHT);input.send(SDL_EVENT_MOUSE_BUTTON_UP,center(entry),false,SDL_BUTTON_RIGHT);
 check(!editor.active(),"Right click opens the editor");
 input.send(SDL_EVENT_MOUSE_BUTTON_DOWN,center(entry));input.send(SDL_EVENT_MOUSE_MOTION,center(hud[HudPart::Water]));input.send(SDL_EVENT_MOUSE_BUTTON_UP,center(entry));
 check(!editor.active(),"Dragging the layout button opens it");
 for(bool touch:{false,true}){
  editor.close();care.reset();input.pointer={};session.domain().install(base);
  input.click(entry,touch);check(editor.active()&&care.tool()==Tool::Select,"Layout button does not open the editing mode");
  {
   const auto l=layoutEditor(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
   const auto in=canvas.safeInsets();const Rect safeArea{in.left,in.top,canvas.width()-in.left-in.right,canvas.height()-in.top-in.bottom};
   for(const auto r:{l.exit,l.exitHit,l.header,l.title})check(inside(r,safeArea),"Editor controls cross the safe area");
   check(l.exitHit.h>=canvas.minimumTouchSize()&&l.exitHit.w>=canvas.minimumTouchSize(),"Exit has lost its touch target");
   check(inside(l.exit,l.exitHit)&&inside(l.title,l.header)&&l.exitHit.x+l.exitHit.w<l.header.x,"The compact Exit button overlaps the inventory count");
   check(std::abs(center(l.exit).x-center(safeArea).x)<.01f&&l.exit.y<safeArea.y+canvas.minimumTouchSize(),"Exit is not centered at the top");
   const auto inventory=editor.inventoryLayout();
   check(inside(inventory.dialog,safeArea)&&inventory.dialog.w<safeArea.w&&inventory.dialog.h<safeArea.h,"Inventory is not a dialog inside the safe area");
   check(inside(inventory.closeHit,inventory.dialog)&&inventory.closeHit.w>=canvas.minimumTouchSize()&&inventory.closeHit.h>=canvas.minimumTouchSize(),"Inventory close loses its safe touch target");
   for(int i=0;i<6;++i)check((inventory.tabs[i].w>0)==(i==int(ShopCategory::Plants)||i==int(ShopCategory::Decorations)),"Inventory does not have exactly Plants and Decorations tabs");
   for(int i=0;i<inventory.visibleCards;++i)check(inside(inventory.cards[i],inventory.body)&&inventory.cards[i].h>224*inventory.unit,"Inventory cards overflow or have no room for their artwork");
  }
  capture("open");
  const auto original=*session.domain().decoration(501);const auto* def=content.findDecor(original.kind);
  const auto bounds=canvas.decorRect(*def,original.position,original.sizeMul);const SDL_FPoint grab{bounds.x+bounds.w*.35f,bounds.y+bounds.h*.45f};
  auto overlap=base;overlap.fish.resize(1);overlap.fish.front().position=canvas.toWorld(grab.x,grab.y);overlap.fish.front().motion={};testing::stage(overlap.fish.front(),4);session.domain().install(overlap);
  const auto before=encode(session.domain().state());
  const SDL_FPoint destination{grab.x+canvas.width()*.13f,grab.y-canvas.height()*.08f};
  const auto from=projection.toWorld(grab),to=projection.toWorld(destination);
  const auto expected=projection.place(*def,{original.position.x+to.x-from.x,original.position.y+to.y-from.y},original.sizeMul);
  const auto down=touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,motion=touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION,up=touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP;
  input.send(down,grab,touch);input.send(motion,destination,touch);
  check(encode(session.domain().state())==before,"An unfinished drag saves its position");
  input.send(up,destination,touch);
  check(editor.placement().moving&&editor.placement().preview.id==501&&same(editor.placement().preview.position,expected),"Direct drag jumps, selects a fish, or fails to move the decoration");
  const auto actions=editor.placementLayout();
  check(actions.cancelHit.x+actions.cancelHit.w<actions.placeHit.x,"Remove and tick have overlapping targets");
  const auto normal=layoutDecorPlacement(canvas,session.domain(),editor.placement());
  check(actions.cancel.w==normal.cancel.w&&actions.cancel.h==normal.cancel.h&&actions.place.w==normal.place.w&&actions.place.h==normal.place.h,"Move actions differ from normal placement size");
  for(const auto [icon,target]:{std::pair{actions.cancel,actions.cancelHit},std::pair{actions.place,actions.placeHit}})
   check(inside(target,editor.layout().water)&&inside(icon,target)&&target.h>=canvas.minimumTouchSize(),"A selection action loses its touch area");
  check(same(session.domain().decoration(501)->position,expected)&&!care.detailsOpen(),"A completed drag does not save or opens fish details");
  const auto dropped=encode(session.domain().state());
  input.key(SDLK_F);input.key(SDLK_S);check(editor.active()&&care.tool()==Tool::Select&&encode(session.domain().state())==dropped,"Editing activates feeding or selling");
  capture("moving");
  const SDL_FPoint outside{editor.layout().water.x+4,canvas.height()*.3f};
  input.tap(outside,touch);
  check(editor.active()&&!editor.preview()&&encode(session.domain().state())==dropped,"An outside tap loses the drop position or moves the item to the tap");
  // Interrupting a later drag preserves the last completed drop.
  const auto movedGrab=center(canvas.decorRect(*def,expected,original.sizeMul));
  const SDL_FPoint next{movedGrab.x+canvas.width()*.1f,movedGrab.y};
  input.send(down,movedGrab,touch);input.send(motion,next,touch);input.send(touch?SDL_EVENT_FINGER_CANCELED:SDL_EVENT_WINDOW_FOCUS_LOST,next,touch);input.send(up,next,touch);
  check(editor.preview()&&same(editor.preview()->position,expected)&&encode(session.domain().state())==dropped,"An interrupted drag rolls back the earlier saved drop");
  input.key(SDLK_ESCAPE);check(editor.active()&&!editor.preview()&&encode(session.domain().state())==dropped,"Escape rolls back a saved move");
  input.tap(movedGrab,touch);input.click(editor.placementLayout().place,touch);
  check(editor.active()&&!editor.preview()&&encode(session.domain().state())==dropped,"The tick changes the saved position instead of just leaving editing");
  input.tap(movedGrab,touch);
  const auto remove=center(editor.placementLayout().cancel);
  input.send(down,remove,touch);input.send(motion,grab,touch);input.send(up,remove,touch);
  check(editor.preview()&&!session.domain().decoration(501)->stored,"Dragging Remove stores an item");
  capture("remove");input.click(editor.placementLayout().cancel,touch);
  check(session.domain().decoration(501)->stored&&!editor.preview()&&editor.active(),"Remove does not retain the same owned copy in inventory");
  input.bag(touch);check(editor.inventoryOpen()&&editor.storedStacks().size()==2,"Bag does not include removed plants and copies from another tank");capture("stash");
  input.click(input.card(),touch);
  check(editor.preview()&&editor.placement().moving&&!editor.inventoryOpen()&&!session.domain().decoration(501)->stored,"Place does not restore and select the stored copy immediately");
  const auto placed=encode(session.domain().state());
  input.click(editor.layout().exit,touch);check(encode(session.domain().state())==placed&&!editor.active(),"Done removes or changes the restored copy");
  input.bag(touch);
  const auto stacks=editor.storedStacks();const auto at=std::find_if(stacks.begin(),stacks.end(),[](const auto& stack){return stack.firstId==503;})-stacks.begin();
  input.click(input.card(std::size_t(at)),touch);
  check(editor.preview()&&editor.preview()->id==503&&editor.preview()->flipped&&editor.preview()->sizeMul==1.4&&editor.preview()->tank==TankId{1},"Restoration loses its saved identity, flip, size or destination");
  capture("restore");const auto restoredSave=encode(session.domain().state());input.click(editor.placementLayout().place,touch);
  check(!editor.active()&&encode(session.domain().state())==restoredSave&&!session.domain().decoration(503)->stored,"The tick changes an item placed from inventory");
  const auto& after=session.domain().state();
  check(after.decor.size()==base.decor.size()&&after.nextDecorId==base.nextDecorId&&after.decorOwned==base.decorOwned&&after.wallet.coins==base.wallet.coins&&after.wallet.pearls==base.wallet.pearls&&after.xp==base.xp,"Editing changes money, XP or ownership");
  decodeAndValidate(encode(after),content);
  // The modal filters owned copies and consumes complete gestures before placement.
  auto mixed=base;mixed.decor.push_back({504,"CD-01",{2},{540,460},true,true,1.25});mixed.nextDecorId=505;
  session.domain().install(mixed);const auto inventoryBefore=encode(mixed);input.bag(touch);
  const auto inventory=editor.inventoryLayout();
  input.click(inventory.tabs[int(ShopCategory::Decorations)],touch);
  check(editor.inventoryOpen()&&editor.inventoryState().category==ShopCategory::Decorations&&editor.storedStacks().size()==1&&editor.storedStacks().front().firstId==504,"Decorations tab shows unowned or plant copies");
  capture("inventory-decorations");
  input.send(down,center(input.card()),touch);input.send(touch?SDL_EVENT_FINGER_CANCELED:SDL_EVENT_WINDOW_FOCUS_LOST,center(input.card()),touch);input.send(up,center(input.card()),touch);
  check(editor.inventoryOpen()&&!editor.preview()&&encode(session.domain().state())==inventoryBefore,"An interrupted inventory press restores an item");
  input.send(SDL_EVENT_MOUSE_BUTTON_DOWN,center(input.card()),false,SDL_BUTTON_RIGHT);input.send(SDL_EVENT_MOUSE_BUTTON_UP,center(input.card()),false,SDL_BUTTON_RIGHT);
  check(editor.inventoryOpen()&&!editor.preview(),"Right click restores an inventory item");
  input.click(input.card(),touch);
  check(editor.preview()&&editor.preview()->id==504&&editor.preview()->flipped&&editor.preview()->sizeMul==1.25,"A filtered card loses its owned copy or saved appearance");
  const auto filteredSave=encode(session.domain().state());
  input.click(editor.layout().exit,touch);input.bag(touch);
  input.click(inventory.tabs[int(ShopCategory::Decorations)],touch);input.click(inventory.tabs[int(ShopCategory::Plants)],touch);
  check(editor.storedStacks().size()==1&&editor.storedStacks().front().firstId==503,"Plants tab does not restore its own list");
  input.send(down,center(inventory.close),touch);input.send(motion,center(inventory.body),touch);input.send(up,center(inventory.close),touch);
  check(editor.inventoryOpen(),"Dragging the inventory close button dismisses it");
  input.click(inventory.close,touch);check(!editor.active()&&encode(session.domain().state())==filteredSave,"Inventory close changes ownership or remains in editing mode");
  input.bag(touch);const SDL_FPoint backdrop{inventory.dialog.x-4*canvas.minimumTouchSize()/44,inventory.dialog.y+inventory.dialog.h*.5f};
  input.tap(backdrop,touch);check(!editor.active()&&!care.detailsOpen()&&encode(session.domain().state())==filteredSave,"Inventory backdrop clicks reach the aquarium or change progress");
  input.bag(touch);input.key(SDLK_ESCAPE);check(!editor.active(),"Escape does not dismiss the inventory dialog");
 }
 // Place selects one live copy. Ending editing never takes the next copy.
 for(bool touch:{false,true})for(const auto category:{ShopCategory::Plants,ShopCategory::Decorations}){
  auto stacked=base;stacked.decor.front().stored=true;stacked.wallet={0,0};
  stacked.decor.push_back({504,"CP-02",{2},{420,440},true,false,.8});
  stacked.decor.push_back({505,"CP-02",{1},{680,440},true,true,1.35});
  stacked.decor.push_back({506,"CD-01",{2},{420,440},true,false,.85});
  stacked.decor.push_back({507,"CD-01",{1},{540,440},true,true,1.4});
  stacked.decor.push_back({508,"CD-01",{2},{680,440},true,false,1.1});
  stacked.nextDecorId=509;testing::openingBalances(stacked);
  const std::array<std::uint64_t,3> ids=category==ShopCategory::Plants?std::array<std::uint64_t,3>{501,504,505}:std::array<std::uint64_t,3>{506,507,508};
  const auto* def=content.findDecor(category==ShopCategory::Plants?"CP-02":"CD-01");
  const auto chooseCopy=[&](){
   input.bag(touch);input.click(editor.inventoryLayout().tabs[int(category)],touch);
   const auto stacks=editor.storedStacks();const auto found=std::find_if(stacks.begin(),stacks.end(),[&](const auto& stack){return stack.kind==def->id;});
   check(found!=stacks.end(),"The remaining stack is missing from inventory");input.click(input.card(std::size_t(found-stacks.begin())),touch);
  };
  const auto openStack=[&](){
   editor.close();care.reset();input.pointer={};session.domain().install(stacked);input.bag(touch);
   input.click(editor.inventoryLayout().tabs[int(category)],touch);
   const auto stacks=editor.storedStacks();
   check(stacks.size()==(category==ShopCategory::Plants?2u:1u)&&stacks.front().firstId==ids.front()&&stacks.front().count==3,"Inventory splits duplicates or counts placed copies");
   capture(category==ShopCategory::Plants?"inventory-plant-stack":"inventory-decor-stack");
   input.click(input.card(),touch);
  };
  const auto down=touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,motion=touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION,up=touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP;
  const SDL_FPoint outside{editor.layout().water.x+4,canvas.height()*.3f};
  openStack();const auto before=encode(session.domain().state());
  check(editor.preview()&&editor.placement().moving&&editor.preview()->id==ids.front()&&!session.domain().decoration(ids.front())->stored&&stackCount(editor,def->id)==2,"Place does not restore exactly one stack copy before editing");
  const auto actions=editor.placementLayout();const auto normal=layoutDecorPlacement(canvas,session.domain(),editor.placement());
  check(actions.cancel.w==normal.cancel.w&&actions.cancel.h==normal.cancel.h&&actions.place.w==normal.place.w&&actions.place.h==normal.place.h,"Inventory editing does not use the normal remove and tick controls");
  for(const auto [icon,target]:{std::pair{actions.cancel,actions.cancelHit},std::pair{actions.place,actions.placeHit}})
   check(inside(target,editor.layout().water)&&inside(icon,target)&&target.w>=canvas.minimumTouchSize()&&target.h>=canvas.minimumTouchSize(),"An inventory editing action loses its touch area");
  input.send(up,outside,touch);check(editor.preview()&&encode(session.domain().state())==before,"The opening release changes the placed copy");
  const auto original=editor.preview()->position;
  if(!touch){input.send(SDL_EVENT_MOUSE_MOTION,outside);check(same(editor.preview()->position,original),"Hovering moves the inventory item");}
  const auto bounds=actions.item;
  const SDL_FPoint grab{bounds.x+bounds.w*.35f,bounds.y+bounds.h*.45f},dragTo{grab.x+canvas.width()*.13f,grab.y-canvas.height()*.08f};
  const auto from=projection.toWorld(grab),to=projection.toWorld(dragTo);
  const auto expected=projection.place(*def,{original.x+to.x-from.x,original.y+to.y-from.y},editor.preview()->sizeMul);
  input.drag(grab,dragTo,touch);
  check(editor.preview()&&same(editor.preview()->position,expected)&&same(session.domain().decoration(ids.front())->position,expected)&&stackCount(editor,def->id)==2,"A drop jumps from its grab point, fails to save or takes another copy");
  const auto dropped=encode(session.domain().state());capture("stack-two-left");
  for(const bool finish:{false,true}){
   const auto button=center(finish?editor.placementLayout().place:editor.placementLayout().cancel);
   input.send(down,button,touch);input.send(motion,{button.x+canvas.width()*.2f,button.y},touch);input.send(up,button,touch);
   check(editor.preview()&&encode(session.domain().state())==dropped,"Dragging an action removes the item or ends editing");
  }
  input.send(down,center(editor.placementLayout().place),touch);input.send(SDL_EVENT_WINDOW_FOCUS_LOST);input.send(up,center(editor.placementLayout().place),touch);
  check(editor.preview()&&encode(session.domain().state())==dropped,"An interrupted tick ends editing or changes the copy");
  const auto nextGrab=center(editor.placementLayout().item);const SDL_FPoint nextTarget{nextGrab.x+canvas.width()*.1f,nextGrab.y};
  for(const auto interruption:{SDL_EVENT_WINDOW_FOCUS_LOST,SDL_EVENT_WINDOW_RESIZED,SDL_EVENT_WILL_ENTER_BACKGROUND}){
   input.send(down,nextGrab,touch);input.send(motion,nextTarget,touch);input.send(interruption);input.send(up,nextTarget,touch);
   check(editor.preview()&&same(editor.preview()->position,expected)&&encode(session.domain().state())==dropped,"An interrupted drag loses the earlier saved position");
  }
  if(touch){input.send(down,nextGrab,true);input.send(motion,nextTarget,true);input.send(SDL_EVENT_FINGER_CANCELED,nextTarget,true);input.send(up,nextTarget,true);}
  input.send(down,nextGrab,touch);input.send(up,{-10,-10},touch);
  input.send(SDL_EVENT_MOUSE_BUTTON_DOWN,outside,false,SDL_BUTTON_RIGHT);input.send(SDL_EVENT_MOUSE_BUTTON_UP,outside,false,SDL_BUTTON_RIGHT);
  input.click(editor.layout().header,touch);
  input.send(down,center(editor.layout().exit),touch);input.send(motion,outside,touch);input.send(up,center(editor.layout().exit),touch);
  check(editor.preview()&&same(editor.preview()->position,expected)&&encode(session.domain().state())==dropped,"A cancelled gesture, header tap or right click changes the saved drop");
  input.tap(outside,touch);
  check(!editor.active()&&!editor.preview()&&encode(session.domain().state())==dropped,"An outside tap removes or moves the item instead of ending editing");
  for(std::size_t i=1;i<ids.size();++i){
   const auto appearance=*session.domain().decoration(ids[i]);chooseCopy();
   check(editor.preview()&&editor.preview()->id==ids[i]&&stackCount(editor,def->id)==ids.size()-i-1,"Selecting the next stored copy loses its identity or stack count");
   const auto saved=encode(session.domain().state());
   const auto* placed=session.domain().decoration(ids[i]);
   check(placed&&!placed->stored&&placed->tank==TankId{1}&&placed->flipped==appearance.flipped&&placed->sizeMul==appearance.sizeMul,"Place changes the owned copy's saved appearance");
   if(i==1)capture("stack-one-left");
   if(i==1){input.key(SDLK_RETURN,true);check(editor.preview(),"Repeated Enter ends editing");input.key(SDLK_RETURN);}
   else input.click(editor.placementLayout().place,touch);
   check(!editor.active()&&!editor.preview()&&encode(session.domain().state())==saved,"The tick or Enter places an extra copy or changes the saved item");
   input.send(up,outside,touch);check(encode(session.domain().state())==saved,"A repeated release takes another copy");
  }
  const auto& after=session.domain().state();
  check(after.decor.size()==stacked.decor.size()&&after.nextDecorId==stacked.nextDecorId&&after.decorOwned==stacked.decorOwned&&after.wallet.coins==0&&after.wallet.pearls==0&&after.xp==stacked.xp,"Inventory editing changes currency, XP or ownership");
  check(session.domain().decoration(503)->stored&&editor.placement().receipts.empty(),"Inventory editing restores another kind or displays a purchase receipt");
  decodeAndValidate(encode(after),content);
  input.bag(touch);input.click(editor.inventoryLayout().tabs[int(category)],touch);
  check(stackCount(editor,def->id)==0,"An exhausted stack remains in inventory");
  openStack();input.click(editor.placementLayout().cancel,touch);
  check(!editor.active()&&session.domain().decoration(ids.front())->stored&&stackCount(editor,def->id)==3&&session.domain().state().decor.size()==stacked.decor.size(),"Remove deletes ownership or fails to restore the inventory count");
  for(const std::string_view finish:{"tick","done","escape","outside"}){
   openStack();const auto placed=encode(session.domain().state());
   if(finish=="escape")input.key(SDLK_ESCAPE);
   else if(finish=="outside")input.tap(outside,touch);
   else input.click(finish=="tick"?editor.placementLayout().place:editor.layout().exit,touch);
   check(!editor.active()&&encode(session.domain().state())==placed&&!session.domain().decoration(ids.front())->stored&&session.domain().decoration(ids[1])->stored,"Ending editing changes the placed copy or takes another one");
  }
  // The final slot can be edited freely; further copies stay in inventory.
  auto nearlyFull=stacked;
  for(int i=1;i<content.decorTuning.placedLimit-1;++i)nearlyFull.decor.push_back({nearlyFull.nextDecorId++,"CD-01",{1},{500,440}});
  session.domain().install(nearlyFull);editor.open(true);input.click(editor.inventoryLayout().tabs[int(category)],touch);input.click(input.card(),touch);
  check(editor.preview()&&stackCount(editor,def->id)==2&&session.domain().placedDecor({1})==std::size_t(content.decorTuning.placedLimit),"Placing into the last slot loses an extra copy");
  input.click(editor.placementLayout().place,touch);input.bag(touch);input.click(editor.inventoryLayout().tabs[int(category)],touch);
  const auto fullSave=encode(session.domain().state());input.click(input.card(),touch);
  check(editor.inventoryOpen()&&!editor.preview()&&encode(session.domain().state())==fullSave,"A full tank consumes another stack copy");capture("stack-full-tank");
 }
 // Open water beside Exit and at the bottom stays interactive.
 for(const auto anchor:{WorldPoint{80,0},WorldPoint{544,tankHeight}}){
  auto edge=base;const auto* def=content.findDecor(edge.decor[0].kind);
  edge.decor[0].position=projection.place(*def,anchor,edge.decor[0].sizeMul);session.domain().install(edge);editor.open();
  input.tap(center(canvas.decorRect(*def,edge.decor[0].position,edge.decor[0].sizeMul)));
  check(editor.preview()&&editor.preview()->id==501,"Header or footer reserves an empty strip that makes edge decorations unreachable");
 }
 // Pagination and swipes must never restore the wrong copy.
 auto many=base;std::uint64_t nextId=600;
 for(const auto& def:content.decorations)if(def.category=="Plant"){
  many.decor.push_back({nextId++,def.id,{1},{500,440},true});many.decor.push_back({nextId++,def.id,{2},{500,440},true});
 }
 many.nextDecorId=nextId;
 session.domain().install(many);editor.open(true);const auto firstId=editor.storedStacks().front().firstId;
 const auto firstPoint=center(input.card());const auto stride=input.card(1).x-input.card().x;const SDL_FPoint swipeEnd{firstPoint.x-stride,firstPoint.y};
 input.send(SDL_EVENT_FINGER_DOWN,firstPoint,true);
 input.send(SDL_EVENT_FINGER_MOTION,swipeEnd,true);input.send(SDL_EVENT_FINGER_UP,swipeEnd,true);
 check(editor.inventoryOpen()&&!editor.preview()&&session.domain().decoration(firstId)->stored,"An inventory swipe places an item");
 input.click(input.card(1));check(editor.preview()&&editor.preview()->id!=firstId,"An inventory swipe does not scroll to another card");
 input.key(SDLK_ESCAPE);check(!editor.active(),"Escape does not leave inventory placement");
 editor.open(true);
 const auto flickStart=center(input.card(1));const SDL_FPoint flickEnd{flickStart.x-stride*.5f,flickStart.y};
 input.send(SDL_EVENT_FINGER_DOWN,flickStart,true,SDL_BUTTON_LEFT,2000000000);
 input.send(SDL_EVENT_FINGER_MOTION,flickEnd,true,SDL_BUTTON_LEFT,2100000000);
 input.send(SDL_EVENT_FINGER_UP,flickEnd,true,SDL_BUTTON_LEFT,2105000000);
 const float released=editor.inventoryState().scroll;editor.advance(.1);
 check(editor.inventoryState().scroll>released+.1f&&!editor.preview(),"Inventory swipe does not glide after releasing the finger");
 input.click(input.card(2));const float stopped=editor.inventoryState().scroll;editor.advance(.1);
 check(editor.inventoryState().scroll==stopped&&editor.inventoryOpen()&&!editor.preview(),"Stopping inventory momentum places an item");
 input.key(SDLK_ESCAPE);
 auto full=base;
 const auto placed=std::count_if(full.decor.begin(),full.decor.end(),[&](const auto& item){return item.tank==full.activeTank&&!item.stored;});
 for(auto i=placed;i<content.decorTuning.placedLimit;++i)full.decor.push_back({full.nextDecorId++,"CP-02",full.activeTank,{500,440}});
 session.domain().install(full);editor.open(true);input.click(input.card());
 check(editor.inventoryOpen()&&!editor.preview()&&session.domain().decoration(503)->stored,"A full tank accepts a restored item");capture("full-tank");
 session.domain().install(base);editor.open(true);input.click(input.card());
 auto switched=base;switched.activeTank={2};session.domain().install(switched);input.send(SDL_EVENT_MOUSE_MOTION,{10,10});
 check(!editor.active()&&session.domain().decoration(503)->stored,"Changing tanks leaves a stale restore preview active");
 auto empty=base;for(auto& item:empty.decor)item.stored=false;session.domain().install(empty);editor.open(true);
 check(editor.storedStacks().empty(),"An empty inventory shows placed items");capture("inventory-empty");editor.close();
 std::cout<<"PASS layout editor mouse/touch move, stash, restore, cancellation, paging, blockers and ownership "<<w<<'x'<<h<<'\n';
}
void layering(const Content& original,const std::filesystem::path& assets,const std::filesystem::path& captures,int w,int h){
 auto content=original;
 for(auto& def:content.decorations)if(def.id=="CP-02"||def.id=="CD-01"){
  def.width=2.2;def.height=2.2;def.layer=def.id=="CP-02"?"Foreground":"Background";
 }
 Canvas canvas(assets,w,h,true);canvas.previewViewport(PreviewViewport{w,h,1,{}});canvas.begin();
 Session session(content,"/tmp/layout-layering-unused.json",1000,true);HudLayoutEditor editor(canvas,session);HudCare care(canvas,session);Input input{canvas,editor,care,{}};
 auto state=owned(content);state.decor={{502,"CD-01",{1},{544,475},false,false,1.3},
  {501,"CP-02",{1},{544,425}},{503,"CP-02",{1},{544,450},false,true,1.1},
  {504,"CD-01",{1},{544,475},true},{505,"CD-01",{2},{544,475}}};
 state.nextDecorId=506;const auto& def=*content.findDecor("CP-02");
 const auto projection=canvas.decorProjection();
 const auto grab=center(canvas.decorRect(def,state.decor[1].position));
 state.fish.resize(1);state.fish.front().position=canvas.toWorld(grab.x,grab.y);state.fish.front().motion={};testing::stage(state.fish.front(),4);
 for(bool touch:{false,true}){
  session.domain().install(state);editor.open();care.reset();input.pointer={};const auto saved=encode(session.domain().state());
  const auto down=touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,motion=touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION,up=touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP;
  for(auto id:{502,503,501,502,503,501}){
   input.tap(grab,touch);check(editor.preview()&&editor.preview()->id==std::uint64_t(id),"Overlap taps cannot cycle through covered items in depth order");
   check(encode(session.domain().state())==saved,"Selecting an overlapping item saves or moves it");
  }
  if(!captures.empty()&&!touch){
   const auto folder=captures/(std::to_string(w)+"x"+std::to_string(h));std::filesystem::create_directories(folder);
   canvas.begin();canvas.scene(session.domain(),1,0,Tool::Select,{},false,{},editor.preview());editor.paint();
   check(canvas.capture(folder/"layered-selection.png"),"Cannot capture covered selection");
  }
  input.send(down,grab,touch);input.send(SDL_EVENT_WINDOW_FOCUS_LOST);input.send(up,grab,touch);
  check(editor.preview()&&editor.preview()->id==501&&encode(session.domain().state())==saved,"An interrupted tap cycles to a different copy");
  // Slight finger movement remains a tap and never saves an accidental move.
  input.send(down,grab,touch);input.send(motion,{grab.x+editor.layout().dragThreshold*.25f,grab.y},touch);input.send(up,grab,touch);
  check(editor.preview()&&editor.preview()->id==502&&encode(session.domain().state())==saved,"Tap jitter moves an item or prevents cycling");
  input.tap(grab,touch);input.tap(grab,touch);
  const auto originalPosition=editor.preview()->position;
  const SDL_FPoint back{grab.x,grab.y-canvas.height()*.1f};
  input.send(down,grab,touch);input.send(motion,back,touch);
  check(editor.preview()&&editor.preview()->id==501&&decorScale(editor.preview()->position)<decorScale(originalPosition)&&encode(session.domain().state())==saved,"A foreground item steals the rear item's drag or depth does not shrink it");
  input.send(up,back,touch);
  check(editor.preview()&&editor.preview()->id==501&&same(session.domain().decoration(501)->position,editor.preview()->position)&&!same(originalPosition,editor.preview()->position),"Dropping a covered item fails to save its position");
  check(same(session.domain().decoration(502)->position,state.decor[0].position)&&same(session.domain().decoration(503)->position,state.decor[2].position),"Dragging a rear item moves an occluder");
  // Pull the same item back in front, then verify the hit stack agrees.
  const auto current=center(editor.placementLayout().item);const auto currentWorld=projection.toWorld(current);
  const auto target=projection.toScreen({currentWorld.x,currentWorld.y+520-editor.preview()->position.y});
  input.drag(current,target,touch);
  check(editor.preview()&&editor.preview()->id==501&&editor.preview()->position.y>475,"Moving forward loses the selected copy");
  const auto hits=canvas.decorHits(session.domain(),center(editor.placementLayout().item));
  check(!hits.empty()&&hits.front()->id==501,"Moving forward changes size without changing selection order");
  const auto dropped=encode(session.domain().state());input.click(editor.placementLayout().place,touch);
  check(!editor.preview()&&encode(session.domain().state())==dropped,"Ending a layered move changes saved positions");
  auto reloaded=decodeAndValidate(dropped,content);session.domain().install(reloaded);
  check(encode(session.domain().state())==dropped,"Layered positions do not survive reload");
 }
 std::cout<<"PASS covered-item cycling, touch drag ownership, depth changes and reload "<<w<<'x'<<h<<'\n';
}
void persistence(const Content& content,const std::filesystem::path& assets){
 const auto temp=std::filesystem::temp_directory_path()/("fishius-layout-save-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(temp);const auto parent=temp/"blocked",save=parent/"save.json";
 Canvas canvas(assets,1088,635,true);Session session(content,save,1000);auto copies=owned(content);
 copies.decor.push_back({504,"CP-02",{2},{420,440},true,false,.8});copies.decor.push_back({505,"CP-02",{1},{680,440},true,true,1.35});copies.nextDecorId=506;session.domain().install(copies);
 HudCare care(canvas,session);HudLayoutEditor editor(canvas,session);Input input{canvas,editor,care,{}};
 const auto item=*session.domain().decoration(501);const auto point=center(canvas.decorRect(*content.findDecor(item.kind),item.position,item.sizeMul));
 {std::ofstream blocked(parent);blocked<<"prevents saves";}
 const auto before=encode(session.domain().state());editor.open();input.tap(point);input.click(editor.placementLayout().cancel);
 check(editor.preview()&&!editor.placement().error.empty()&&session.saveFailed()&&encode(session.domain().state())==before,"A failed removal loses the selection or the saved item");
 std::filesystem::remove(parent);input.click(editor.placementLayout().cancel);check(!editor.preview()&&session.domain().decoration(501)->stored,"Removal cannot be retried after a save failure");
 auto loaded=Storage(save).load(content);check(loaded.state&&encode(*loaded.state)==encode(session.domain().state()),"The removed copy does not remain in inventory after reload");
 input.bag();
 std::filesystem::rename(parent,temp/"previous-save");{std::ofstream blocked(parent);blocked<<"prevents saves";}
 const auto stored=encode(session.domain().state());input.click(input.card());
 check(editor.inventoryOpen()&&!editor.preview()&&session.saveFailed()&&stackCount(editor,"CP-02")==3&&encode(session.domain().state())==stored,"A failed Place takes a stored copy or leaves a false selection");
 std::filesystem::remove(parent);input.click(input.card());
 check(editor.preview()&&editor.preview()->id==501&&!session.domain().decoration(501)->stored&&stackCount(editor,"CP-02")==2,"Retrying Place loses or duplicates a stack copy");
 loaded=Storage(save).load(content);check(loaded.state&&encode(*loaded.state)==encode(session.domain().state()),"Place requires a tick before it survives reload");
 const auto grab=center(editor.placementLayout().item);const SDL_FPoint target{grab.x+canvas.width()*.1f,grab.y-canvas.height()*.06f};
 input.drag(grab,target);const auto dropped=encode(session.domain().state());
 loaded=Storage(save).load(content);check(loaded.state&&encode(*loaded.state)==dropped,"Dropping the inventory item is not saved before ticking");
 // Neither the tick nor an outside tap should try to write an already saved item.
 std::filesystem::rename(parent,temp/"saved-drop");{std::ofstream blocked(parent);blocked<<"prevents another write";}
 input.click(editor.placementLayout().place);
 check(!editor.active()&&!session.saveFailed()&&encode(session.domain().state())==dropped,"The tick writes or changes an already saved drop");
 editor.open();const auto moved=*session.domain().decoration(501);
 const auto movedGrab=center(canvas.decorRect(*content.findDecor(moved.kind),moved.position,moved.sizeMul));
 input.tap(movedGrab);input.tap({5,canvas.height()*.3f});
 check(!editor.preview()&&!session.saveFailed()&&encode(session.domain().state())==dropped,"An outside tap writes or changes an already saved drop");
 // Failed drops snap back to the last saved anchor, and a new drag can retry.
 const SDL_FPoint next{movedGrab.x+canvas.width()*.1f,movedGrab.y};input.drag(movedGrab,next);
 check(editor.preview()&&!editor.placement().error.empty()&&session.saveFailed()&&same(editor.preview()->position,moved.position)&&encode(session.domain().state())==dropped,"A failed drop leaves an unsaved position or rolls back an earlier drop");
 std::filesystem::remove(parent);input.drag(movedGrab,next);
 check(editor.preview()&&editor.placement().error.empty()&&!session.saveFailed()&&!same(editor.preview()->position,moved.position),"A failed drop cannot be retried by dragging again");
 loaded=Storage(save).load(content);check(loaded.state&&encode(*loaded.state)==encode(session.domain().state()),"A retried drop does not survive reload");
 std::filesystem::remove_all(temp);std::cout<<"PASS remove, Place and drop save rollback, retry, immediate reload and write-free deselection\n";
}

}
int main(int argc,char** argv){try{
 const std::filesystem::path assets=argc>1?argv[1]:"assets",captures=argc>2?argv[2]:"";
 std::ifstream file(assets/"content.json");const auto content=aq::Content::fromJson(aq::Json::parse(file));
 for(const auto [w,h]:{std::pair{1608,908},std::pair{667,375},std::pair{390,844},std::pair{617,316},std::pair{320,240}})viewport(content,assets,captures,w,h);
 viewport(content,assets,captures,852,393,{59,0,59,21});
 for(const auto [w,h]:{std::pair{1608,908},std::pair{852,393},std::pair{390,844}})layering(content,assets,captures,w,h);
 persistence(content,assets);return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
