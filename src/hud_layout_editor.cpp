#include "aquarium/hud_layout_editor.hpp"
#include "aquarium/hud_tokens.hpp"
#include "aquarium/shop_theme.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
namespace {
const DecorDef& definition(const Domain& domain,const Decoration& item){
 if(const auto* def=domain.content().findDecor(item.kind))return *def;
 static const DecorDef legacy=[](){DecorDef def;def.width=1.1;def.height=.95;return def;}();return legacy;
}
bool pointerInput(Uint32 type){return type==SDL_EVENT_MOUSE_BUTTON_DOWN||type==SDL_EVENT_MOUSE_BUTTON_UP||type==SDL_EVENT_MOUSE_MOTION||type==SDL_EVENT_MOUSE_WHEEL;}
bool input(Uint32 type){return pointerInput(type)||type==SDL_EVENT_KEY_DOWN||type==SDL_EVENT_KEY_UP||type==SDL_EVENT_TEXT_INPUT;}
bool interrupted(Uint32 type){return type==SDL_EVENT_WINDOW_FOCUS_LOST||type==SDL_EVENT_WILL_ENTER_BACKGROUND||type==SDL_EVENT_WINDOW_RESIZED||type==SDL_EVENT_RENDER_DEVICE_RESET||type==SDL_EVENT_RENDER_TARGETS_RESET;}
}

LayoutEditorLayout layoutEditor(float w,float h,Insets safe,float minimumTouch){
 LayoutEditorLayout l;
 const float u=std::max(std::min(w/hudTokens::canvasWidth,h/hudTokens::canvasHeight),minimumTouch/64.f);
 const float density=minimumTouch/44.f;
 const float margin=12*u,left=safe.left+margin,right=w-safe.right-margin;
 const float top=safe.top+8*density,bottom=h-safe.bottom-margin;
 l.page={0,0,w,h};l.unit=u;l.dragThreshold=minimumTouch*8/44;
 const float exitWidth=hudTokens::measure(56)*density,exitHeight=hudTokens::measure(24)*density;
 // Keep the visible control compact while retaining the standard touch target.
 l.exitHit={(left+right-exitWidth)*.5f,top,exitWidth,minimumTouch};
 l.exit={l.exitHit.x,top+(minimumTouch-exitHeight)*.5f,exitWidth,exitHeight};
 // Inventory editing shows how many other copies remain stored.
 l.header={l.exit.x+l.exit.w+8*density,l.exit.y,80*density,exitHeight};
 l.title={l.header.x+8*density,l.header.y+5*density,l.header.w-16*density,16*density};
 const float waterTop=l.exitHit.y+l.exitHit.h+8*u;
 const float noticeWidth=std::min(right-left,460*density);
 l.notice={left+(right-left-noticeWidth)*.5f,waterTop,noticeWidth,28*density};
 l.toolbarArea={safe.left,waterTop,w-safe.left-safe.right,std::max(1.f,bottom-waterTop)};
 l.water={safe.left,safe.top,w-safe.left-safe.right,h-safe.top-safe.bottom};
 return l;
}
LayoutEditorLayout HudLayoutEditor::layout()const{return layoutEditor(canvas_.width(),canvas_.height(),canvas_.safeInsets(),canvas_.minimumTouchSize());}
ShopLayout HudLayoutEditor::inventoryLayout()const{return layoutInventory(canvas_.width(),canvas_.height(),canvas_.safeInsets(),canvas_.minimumTouchSize());}
DecorPlacementLayout HudLayoutEditor::placementLayout()const{
 const auto editor=layout();const auto safe=canvas_.safeInsets();
 const auto item=preview()?canvas_.decorRect(definition(session_.domain(),*preview()),preview()->position,preview()->sizeMul):Rect{};
 const bool notice=!error_.empty()||!placement_.error.empty()||session_.saveFailed();
 const float top=notice?editor.notice.y+editor.notice.h+8*editor.unit:editor.toolbarArea.y;
 auto l=layoutDecorPlacement(canvas_.width(),canvas_.height(),{safe.left,top,safe.right,canvas_.height()-editor.toolbarArea.y-editor.toolbarArea.h},canvas_.minimumTouchSize(),item);
 l.water=editor.water;
 l.notice=editor.notice;return l;
}
std::vector<StoredDecorStack> HudLayoutEditor::storedStacks()const{
 std::vector<StoredDecorStack> stacks;
 for(const auto& item:session_.domain().state().decor){
  const bool plant=definition(session_.domain(),item).category=="Plant";
  if(!item.stored||plant!=(inventory_.category==ShopCategory::Plants))continue;
  const auto stack=std::find_if(stacks.begin(),stacks.end(),[&](const auto& s){return s.kind==item.kind;});
  if(stack==stacks.end())stacks.push_back({item.kind,item.id,1});
  else ++stack->count;
 }
 return stacks;
}
std::vector<ShopItem> HudLayoutEditor::inventoryOffers()const{
 std::vector<ShopItem> offers;
 for(const auto& stack:storedStacks()){
  const auto* item=session_.domain().decoration(stack.firstId);if(!item)continue;
  const auto& def=definition(session_.domain(),*item);
  offers.push_back({.name=def.name.empty()?item->kind:def.name,.asset=def.asset.empty()?"decor/"+item->kind+".png":def.asset,.price="Place",.decorId=item->kind,.flipped=item->flipped,.quantity=stack.count});
 }
 return offers;
}
void HudLayoutEditor::clearPress(){pressed_=Control::None;pressedItem_=cycleItem_=0;dragged_=false;inventoryControl_=-1;}
void HudLayoutEditor::open(bool stored){
 close();active_=true;inventoryOpen_=stored;inventoryMotion_={};tank_=session_.domain().state().activeTank;inventory_={ShopCategory::Plants};
 if(stored&&storedStacks().empty()){
  inventory_.category=ShopCategory::Decorations;
  if(storedStacks().empty())inventory_.category=ShopCategory::Plants;
 }
}
void HudLayoutEditor::close(){active_=inventoryOpen_=inventoryEdit_=false;inventory_.motion.stop();cancelDecorPlacement(placement_);error_.clear();clearPress();}
void HudLayoutEditor::advance(double seconds){
 inventoryMotion_.advance(inventoryOpen_,seconds,session_.domain().state().settings.reducedMotion);
 if(!inventoryOpen_){inventory_.motion.stop();return;}
 inventory_.motion.advance(inventory_.scroll,seconds,shopScrollLimit(inventory_,storedStacks().size(),inventoryLayout().visibleCards),session_.domain().state().settings.reducedMotion);
}
HudLayoutEditor::Control HudLayoutEditor::control(const LayoutEditorLayout& l,SDL_FPoint p)const{
 if(l.exitHit.has(p.x,p.y))return Control::Exit;
 return Control::None;
}
std::uint64_t HudLayoutEditor::cardAt(const ShopLayout& l,SDL_FPoint p)const{
 const auto stacks=storedStacks();
 if(const auto i=shopCardAt(l,inventory_,stacks.size(),p))return stacks[*i].firstId;
 return 0;
}
bool HudLayoutEditor::inventoryEvent(const SDL_Event& e,SDL_FPoint point){
 const auto screenPoint=point;point=inventoryMotion_.inputPoint(point);
 if(e.type==SDL_EVENT_KEY_DOWN&&!e.key.repeat&&e.key.key==SDLK_ESCAPE){close();return true;}
 const auto l=inventoryLayout();const auto count=storedStacks().size();
 const float limit=shopScrollLimit(inventory_,count,l.visibleCards);
 scrollShop(inventory_,0,count,l.visibleCards);
 if(e.type==SDL_EVENT_MOUSE_WHEEL&&l.body.has(point.x,point.y)){
  inventory_.motion.wheel(inventory_.scroll,horizontalWheel(e)*.25f,limit,session_.domain().state().settings.reducedMotion);clearPress();return true;
 }
 const bool down=e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT;
 const bool up=e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.button==SDL_BUTTON_LEFT;
 if(down){
  clearPress();down_=screenPoint;scrollStart_=inventory_.scroll;inventoryControl_=shopControl(l,point,inventory_.category);
  dragged_=l.body.has(point.x,point.y)&&inventory_.motion.moving();inventory_.motion.begin(inventory_.scroll,scrollEventTime(e));
  pressedItem_=cardAt(l,point);
  pressed_=!l.dialog.has(point.x,point.y)?Control::Backdrop:inventoryControl_>=0?Control::Inventory:pressedItem_?Control::Card:l.body.has(point.x,point.y)?Control::Content:Control::Inventory;
 }
 if(pressed_!=Control::None&&(e.type==SDL_EVENT_MOUSE_MOTION||up)){
  if(std::hypot(screenPoint.x-down_.x,screenPoint.y-down_.y)>canvas_.minimumTouchSize()*8/44)dragged_=true;
  if(dragged_&&(pressed_==Control::Card||pressed_==Control::Content)){
   inventory_.motion.drag(inventory_.scroll,scrollStart_+(down_.x-screenPoint.x)/(l.cards[1].x-l.cards[0].x),limit,scrollEventTime(e));
  }
 }
 if(up){
  inventory_.motion.release(scrollEventTime(e),session_.domain().state().settings.reducedMotion);
  const auto target=pressed_;const auto id=pressedItem_;const auto controlId=inventoryControl_;const bool tap=!dragged_;clearPress();
  if(!tap)return true;
  if(target==Control::Backdrop&&!l.dialog.has(point.x,point.y)){close();return true;}
  if(target==Control::Inventory&&controlId==shopControl(l,point,inventory_.category)){
   if(controlId==6)close();
   else if(controlId==int(ShopCategory::Plants)||controlId==int(ShopCategory::Decorations)||controlId==4){activateShopControl(inventory_,controlId);error_.clear();}
  }
  if(target==Control::Card&&id==cardAt(l,point)){
   const auto result=startDecorRestore(session_,placement_,id);
   if(result){inventoryOpen_=false;inventoryEdit_=true;error_.clear();}
   else error_=result.message.empty()?errorText(result.error):result.message;
  }
 }
 return input(e.type);
}
std::uint64_t HudLayoutEditor::hitDecor(SDL_FPoint p,std::uint64_t after)const{
 const auto hits=canvas_.decorHits(session_.domain(),p);if(hits.empty())return 0;
 const auto current=std::find_if(hits.begin(),hits.end(),[&](const auto* item){return item->id==after;});
 return current==hits.end()||std::next(current)==hits.end()?hits.front()->id:(*std::next(current))->id;
}
bool HudLayoutEditor::event(const SDL_Event& e,bool blocked){
 if(blocked){close();return false;}
 if(active_&&tank_!=session_.domain().state().activeTank){close();return input(e.type);}
 SDL_FPoint point{};
 if(e.type==SDL_EVENT_MOUSE_MOTION)point=canvas_.inputPoint(e.motion.x,e.motion.y);
 else if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)point=canvas_.inputPoint(e.button.x,e.button.y);
 else if(e.type==SDL_EVENT_MOUSE_WHEEL)point=canvas_.inputPoint(e.wheel.mouse_x,e.wheel.mouse_y);
 if(interrupted(e.type)){
  clearPress();inventory_.motion.stop();if(active_&&placement_.active())decorPlacementEvent(session_,placement_,placementLayout(),e,point);
  return false;
 }
 const bool down=e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT;
 const bool up=e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.button==SDL_BUTTON_LEFT;
 if(!active_){
  const auto hud=layoutHud(canvas_.width(),canvas_.height(),canvas_.safeInsets(),canvas_.minimumTouchSize());
  const auto target=hud[HudPart::Layout].has(point.x,point.y)?Control::Launch:hud[HudPart::Bag].has(point.x,point.y)?Control::Bag:Control::None;
  if(down&&target!=Control::None){clearPress();pressed_=target;down_=point;return true;}
  if(pressed_!=Control::Launch&&pressed_!=Control::Bag)return false;
  if((e.type==SDL_EVENT_MOUSE_MOTION||up)&&std::hypot(point.x-down_.x,point.y-down_.y)>canvas_.minimumTouchSize()*8/44)dragged_=true;
  if(up){const bool activate=target==pressed_&&!dragged_;clearPress();if(activate)open(target==Control::Bag);}
  return pointerInput(e.type);
 }
 if(inventoryOpen_)return inventoryEvent(e,point);
 // Validate the same owned copy before rendering or handling an edit.
 if(placement_.active()){
  SDL_Event check{};
  if(decorPlacementEvent(session_,placement_,placementLayout(),check,{})==PlacementEvent::Cancelled&&inventoryEdit_){close();return input(e.type);}
 }
 if(e.type==SDL_EVENT_KEY_DOWN&&!e.key.repeat&&e.key.key==SDLK_ESCAPE){
  if(inventoryEdit_)close();
  else if(placement_.active())cancelDecorPlacement(placement_);
  else close();
  clearPress();error_.clear();return true;
 }
 const auto l=layout();
 if(down){
  clearPress();const auto target=control(l,point);
  if(target!=Control::None){pressed_=target;down_=point;return true;}
 }
 if(pressed_!=Control::None){
  if((e.type==SDL_EVENT_MOUSE_MOTION||up)&&std::hypot(point.x-down_.x,point.y-down_.y)>l.dragThreshold)dragged_=true;
  if(up){
   const auto target=pressed_;const bool tap=!dragged_;clearPress();
   if(!tap)return true;
   if(target!=control(l,point))return true;
   if(target==Control::Exit)close();
  }
  return input(e.type);
 }
 const bool notice=!error_.empty()||!placement_.error.empty()||session_.saveFailed();
 const bool chrome=(inventoryEdit_&&l.header.has(point.x,point.y))||(notice&&l.notice.has(point.x,point.y))||control(l,point)!=Control::None;
 if(pointerInput(e.type)&&chrome){
  if(up&&placement_.active())decorPlacementEvent(session_,placement_,placementLayout(),e,{-1,-1});
  return true;
 }
 if(down&&!inventoryEdit_&&l.water.has(point.x,point.y)){
  const auto actions=placementLayout();
  if(placement_.active()&&actions.itemHit.has(point.x,point.y)&&!actions.cancelHit.has(point.x,point.y)&&!actions.placeHit.has(point.x,point.y)){
   // Wait for a completed tap before cycling. A drag retains the current copy.
   const auto next=hitDecor(point,placement_.preview.id);
   if(next!=placement_.preview.id)cycleItem_=next;
  }else if(!placement_.active()||(!actions.cancelHit.has(point.x,point.y)&&!actions.placeHit.has(point.x,point.y))){
   const auto id=hitDecor(point);
   if(id){const auto result=startDecorMove(session_.domain(),placement_,id);if(!result)error_=result.message;else error_.clear();}
  }
 }
 if(up&&cycleItem_){
  const auto next=cycleItem_;cycleItem_=0;
  if(placement_.pressed==DecorPlacementControl::Water&&!placement_.dragged&&
     std::hypot(point.x-placement_.down.x,point.y-placement_.down.y)<=l.dragThreshold&&
     hitDecor(point,placement_.preview.id)==next){
   const auto result=startDecorMove(session_.domain(),placement_,next);
   if(!result)error_=result.message;else error_.clear();
   return true;
  }
 }
 if(placement_.active()){
  decorPlacementEvent(session_,placement_,placementLayout(),e,point);
  if(inventoryEdit_&&!placement_.active())close();
 }
 return input(e.type);
}
void HudLayoutEditor::paint(){
 if(!active_)return;
 if(tank_!=session_.domain().state().activeTank){close();return;}
 if(inventoryOpen_){
  const auto l=inventoryLayout();const auto items=inventoryOffers();scrollShop(inventory_,0,items.size(),l.visibleCards);
  const DialogPaint animation(canvas_,inventoryMotion_,l.dialog,session_.domain().state().settings.reducedMotion);
  float x{},y{};SDL_GetMouseState(&x,&y);const auto point=inventoryMotion_.inputPoint(canvas_.inputPoint(x,y));
  paintInventory(canvas_,session_.domain(),l,inventory_,items,shopControl(l,point,inventory_.category),pressed_==Control::Inventory&&!dragged_?inventoryControl_:-1,false);
  if(!error_.empty()||session_.saveFailed()){
   canvas_.gradient(l.footer,{104,43,34,255},{104,43,34,255},8*l.unit);
   canvas_.text(session_.saveFailed()?"Progress is not saved. Keep the game open to retry.":error_,l.footer.x+l.footer.w*.5f,l.footer.y+16*l.unit,20*l.unit,shopTheme::white,true,l.footer.w-24*l.unit,true,true);
  }
  canvas_.cursor(CursorKind::Arrow);return;
 }
 const auto l=layout();const float u=l.unit;
 const float density=canvas_.minimumTouchSize()/44.f;
 if(inventoryEdit_){
  const auto& decor=session_.domain().state().decor;
  const auto count=std::count_if(decor.begin(),decor.end(),[&](const auto& item){return item.stored&&item.kind==placement_.preview.kind;});
  canvas_.gradient(l.header,{18,68,82,235},{13,58,73,235},6*density);
  canvas_.text("×"+std::to_string(count)+" left",l.title.x+l.title.w*.5f,l.title.y,12*density,shopTheme::white,true,l.title.w,true,true);
 }
 shopTheme::panel(canvas_,{l.exit.x,l.exit.y,l.exit.w,l.exit.h-2*density},density*.5f,inventoryEdit_?shopTheme::Surface::Button:shopTheme::Surface::Close,pressed_==Control::Exit&&!dragged_);
 canvas_.text(inventoryEdit_?"Done":"Exit",l.exit.x+l.exit.w*.5f,l.exit.y+5*density,12*density,shopTheme::white,true,l.exit.w-16*density,true,true);
 if(placement_.active())paintDecorPlacement(canvas_,session_,placement_,placementLayout(),false);
 const auto notice=!error_.empty()?error_:placement_.error;
 if(!notice.empty()||session_.saveFailed()){
  canvas_.gradient(l.notice,{104,43,34,240},{104,43,34,240},8*u);
  canvas_.text(session_.saveFailed()?"Progress is not saved. Keep the game open to retry.":notice,l.notice.x+l.notice.w*.5f,l.notice.y+6*density,12*density,shopTheme::white,true,l.notice.w-16*u,true);
 }else if(!inventoryEdit_){
  canvas_.text("Drag to move. Tap an overlap again to select behind.",l.notice.x+l.notice.w*.5f,l.notice.y+6*density,12*density,shopTheme::white,true,l.notice.w-16*u,true);
 }
 canvas_.cursor(CursorKind::Arrow);
}
}
