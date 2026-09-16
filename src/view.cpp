#include "aquarium/theme.hpp"
#include "aquarium/ui_renderer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <sstream>
namespace aq {
namespace {
constexpr Color aqua{159,225,216,255},green{161,217,144,255},cream{247,235,197,255},ink{32,77,96,255},purple{199,172,230,255};
}
View::View(Canvas& c,Session& s,std::filesystem::path project):canvas_(c),session_(s),dialogDocument_(c.assets()/"ui/general-dialog.json"){
 if(!project.empty())uiAck_=project.string()+".ack.json";
 uiDocument_=std::make_unique<UiDocument>(project.empty()?c.assets()/"ui/studio.json":project,c.assets());
 const auto& state=s.domain().state();
 if(!state.pendingDecor.empty()){tool_=Tool::Decor;decorId_=state.pendingDecor;decorPreview_=decorPreviewPoint({waterWidth*.5,decorReferenceHeight*(.3+.82*(.985-.3))});}
}
Result View::command(Command c){auto r=session_.command(c);if(!r){if(r.error==Error::Funds)showFunds(r.shortfall);}else if(!r.replayed&&session_.domain().state().settings.sound)canvas_.sound(600,float(session_.domain().state().settings.volume));return r;}
void View::cancelGesture(){if(decorGestureStart_)decorPreview_=decorGestureStart_;decorGestureStart_.reset();dragged_={};fishPreview_.reset();if(draggingDecor_){decorPreview_.reset();draggingDecor_=false;}selectionGesture_=selectionDragging_=false;placementGesture_=false;if(!pressed_.empty())press(pressed_,false);pointerDown_=false;touchOwned_=false;worldGesture_=false;pageGesture_=pageSwiping_=pageDragCancelled_=false;pageSettleFrom_=pageDragOffset_=0;pressed_.clear();SDL_CaptureMouse(false);}
void View::resetActionTool(){
 if(tool_!=Tool::Food&&tool_!=Tool::Sell&&tool_!=Tool::Buy&&tool_!=Tool::Restore)return;
 tool_=Tool::Select;selected_={};buySpecies_.clear();restore_={};armed_=false;jarAt_=netAt_=-100;
 canvas_.cursor(CursorKind::Arrow);
}
void View::open(Panel p){
 if(p!=Panel::None)resetActionTool();
 if(p!=Panel::None&&decorPreview_){cancelDecor();if(decorPreview_)return;}
 cancelGesture();
 shopHighlightedId_.clear();shopHighlightedCategory_=-1;highlightedTank_={0};
 if(p!=Panel::None&&p!=paintedPanel_)panelMotion_={};
 if(p==Panel::Tanks&&panel_!=Panel::Tanks&&panel_!=Panel::CurrencyShop)selectedTank_=session_.domain().state().activeTank.value;
 panelMotion_.show(p!=Panel::None,now_);panel_=p;page_=0;rehomeConfirm_=false;rehomeQuote_.reset();
 if(p!=Panel::Details){selected_={};selectedDecor_=0;}
}
void View::setPanel(Panel p){open(p);}
void View::setTool(Tool t){
 if(!session_.domain().state().pendingDecor.empty()&&!command({Action::CancelDecor}))return;
 open(Panel::None);draggingDecor_=false;tool_=t;restore_={};decorId_.clear();buySpecies_.clear();selectedDecor_=restoreDecor_=0;decorPreview_.reset();decorPlacementError_.clear();buttons_.clear();armed_=false;jarAt_=netAt_=-100;
}
void View::armBuy(std::string id){
 const auto* species=session_.domain().content().find(id);if(!species)return;
 const auto gate=session_.domain().blocker(*species);
 if(!gate){if(gate.error==Error::Funds)showFunds(gate.shortfall);return;}
 setTool(Tool::Buy);buySpecies_=std::move(id);buyOffer_=session_.domain().quote(*species);
 armX_=pointerX_;armY_=pointerY_;armed_=pointerSeen_;
}
void View::armRestore(FishId id){setTool(Tool::Restore);restore_=id;armX_=pointerX_;armY_=pointerY_;armed_=true;}
void View::armDecor(std::string id){
 const auto* def=session_.domain().content().findDecor(id);if(!def)return;
 const auto gate=session_.domain().blocker(*def);
 if(!gate){if(gate.error==Error::Funds)showFunds(gate.shortfall);return;}
 open(Panel::None);tool_=Tool::Decor;restore_={};selectedDecor_=restoreDecor_=0;draggingDecor_=false;armed_=false;
 decorPlacementError_.clear();buttons_.clear();
 decorId_=std::move(id);decorPreview_=decorPreviewPoint({waterWidth*.5,decorReferenceHeight*(.3+.82*(.985-.3))});
 receipts_.clear();
}
void View::button(std::string id,Rect rect,std::string label,std::function<void()> action,Color color,std::string icon){
 glassButton(std::move(id),rect,std::move(label),std::move(action),color.g>color.r?"green":"blue",std::clamp(rect.h*.40f,10.f,26.f),std::move(icon));
}
void View::render(double t){now_=t;uiLayout_.reset();
 if(!dialogOverride_&&SDL_GetTicks()>=nextDesignPoll_){dialogDocument_.poll();pollUi();nextDesignPoll_=SDL_GetTicks()+250;}
 if(tool_==Tool::Sell)selected_=pointerInTank()?hitFish(pointerX_,pointerY_,true):FishId{};
 buttons_.clear();auto& d=session_.domain();if(panel_==Panel::Details&&!d.fish(selected_)&&!d.companion(selected_))open(Panel::None);canvas_.begin();canvas_.origin(0);
 std::optional<Decoration> preview;const bool previewHighlight=decorPreview_.has_value();
 const bool placing=panel_==Panel::None&&previewHighlight;
 if(decorPreview_){
  const auto* item=d.decoration(restoreDecor_?restoreDecor_:selectedDecor_);
  // Use the future copy's identity so its animated pose also stays continuous.
  preview=item?*item:Decoration{d.state().nextDecorId,decorId_,d.state().activeTank,*decorPreview_};
  preview->position=*decorPreview_;preview->stored=false;preview->tank=d.state().activeTank;
 }
 canvas_.scene(d,session_.interpolation(),t,tool_,held(),!placing&&(panel_==Panel::None||panel_==Panel::Details),fishPreview_?dragged_:FishId{},preview?&*preview:nullptr,selectedDecor_,previewHighlight);
 if(fishPreview_&&(d.fish(dragged_)||d.companion(dragged_))){
  auto moving=d.fish(dragged_)?*d.fish(dragged_):companionVisual(*d.companion(dragged_));moving.position=*fishPreview_;moving.motion.previous=moving.position;
  const auto& species=*d.content().find(moving.species);
  canvas_.fish(species,moving,1,true,{183,255,249,255},true,reducedMotion(),careOf(species,moving,d.state().simNow),t);
 }
 if(!uiPreviewScreen_.empty()){renderUiPreview();return;}
 if(!placing)ui();else decorPlacementControls();
 panelButtonStart_=buttons_.size();
 if(panel_!=Panel::None)panel();
 else if(placing){panelLayer_.reset();paintedPanel_=Panel::None;}
 else if(panelLayer_){
  panelPose_=menuPose(panelMotion_,panelAnchor_);
  // A closing snapshot has no controls and never re-renders removed fish.
  if(panelPose_.alpha<=0||layerWidth_!=canvas_.width()||layerHeight_!=canvas_.height()){panelLayer_.reset();paintedPanel_=Panel::None;}
  else{
   panelDisplayRect_=panelPose_.apply(panelLayoutRect_);
   if(paintedPanel_!=Panel::Details&&paintedPanel_!=Panel::Shop&&paintedPanel_!=Panel::CurrencyShop&&paintedPanel_!=Panel::Tanks)canvas_.fill({0,0,canvas_.width(),canvas_.height()},{0,25,55,Uint8(55*panelPose_.alpha)});
   canvas_.menuLayer(*panelLayer_,panelPose_);
  }
 }
 // Keep the tank selector above its anchored popup, with matching hit order.
 if(panel_==Panel::Tanks)tankNavigation();
 if(!placing){selectionControls();if(panel_==Panel::None&&tutorialVisible_)tutorial();effects();}
 for(auto e:d.takeEvents()){
  if(e.kind=="level"&&e.reachedLevel>=2){if(levelUps_.empty()){cancelGesture();levelAnimation_.motion.show(true,now_);}levelUps_.push_back(e);}
  else if(!placing){if(receipts_.size()>=32)receipts_.pop_front();receipts_.push_back({e,t});}
  if(d.state().settings.sound)canvas_.sound(e.kind=="feed"?890:520,float(d.state().settings.volume));
 }
 toolPointer();
 dialogs();
 storageNotice();
 for(auto it=presses_.begin();it!=presses_.end();)if(!it->second.down&&t-it->second.start>1)it=presses_.erase(it);else ++it;
}
void View::storageNotice(){
 storageNoticeRect_={};const auto* notice=session_.notification();if(!notice)return;
 const bool failed=notice->kind==SessionNotificationKind::SaveFailure;
 const float width=std::min(940.f,canvas_.width()-40.f),height=std::max(110.f,canvas_.minimumTouchSize()+24.f);
 storageNoticeRect_={(canvas_.width()-width)*.5f,std::max(112.f,canvas_.safeInsets().top+12.f),width,height};
 const auto r=storageNoticeRect_;const float actionWidth=std::max(150.f,canvas_.minimumTouchSize());
 canvas_.round(r,{250,237,204,255},18,{141,99,54,255},2,true);
 touchTarget("save-notice-body",r,[]{});
 canvas_.text(failed?"Progress could not be saved":"Save recovery notice",r.x+18,r.y+12,26,ink,false,r.w-actionWidth-54,true);
 canvas_.text(failed?"Keep the game open and retry saving.":"Original save preserved. Progress uses a recovery file.",r.x+18,r.y+54,24,ink,false,r.w-actionWidth-54);
 glassButton(failed?"save-retry":"save-notice-dismiss",{r.x+r.w-actionWidth-12,r.y+12,actionWidth,r.h-24},failed?"Retry":"OK",[this,failed]{if(failed)session_.checkpoint(session_.domain().state().wallAnchor);else session_.dismissNotification();},"green",26);
 if(r.has(pointerX_,pointerY_))canvas_.cursor(CursorKind::Arrow);
}
std::string View::price(const Species& s)const{return s.currency==Currency::Gift?"FREE":compact(s.companion?s.price:session_.domain().quote(s).principal)+(s.currency==Currency::Pearls?" pearls":" coins");}
void View::inventory(){
 auto& d=session_.domain();std::vector<Fish> stored;for(const auto& f:d.state().companions)if(f.stored)stored.push_back(companionVisual(f));
 if(inventoryCategory_==1||stored.empty()){inventoryCategory_=1;decorInventory();return;}
 const auto ip=panelRect_;glassButton("inventory-decor",{ip.x+ip.w-230,ip.y+74,210,44},"Decorations",[this]{inventoryCategory_=1;page_=0;},"blue",23);
 const Rect p=panelRect_;
 int pages=std::max(1,(static_cast<int>(stored.size())+7)/8);page_=std::clamp(page_,0,pages-1);float gap=10,cw=(p.w-54)/4,ch=(p.h-202)/2;
 for(int i=0;i<8;++i){int ix=page_*8+i;if(ix>=static_cast<int>(stored.size()))break;const auto& f=stored[ix];const auto& s=*d.content().find(f.species);Rect r{p.x+12+(i%4)*(cw+gap),p.y+124+(i/4)*(ch+gap),cw,ch};canvas_.skin("card",r);canvas_.text(s.name,r.x+cw*.5f,r.y+7,20,ink,true,cw-12,true);canvas_.icon(f.egg?"ui/egg.png":s.asset,{r.x+cw*.15f,r.y+ch*.24f,cw*.7f,ch*(ch<140?.27f:.39f)});canvas_.text("Display companion",r.x+cw*.5f,r.y+ch-53,12,ink,true,cw-10);button("restore"+std::to_string(f.id.value),{r.x+8,r.y+ch-30,cw-16,25},"PLACE",[this,id=f.id]{armRestore(id);},green);}
 button("inv-prev",{p.x+14,p.y+p.h-33,48,23},"<",[this]{page_=std::max(0,page_-1);});button("inv-next",{p.x+p.w-62,p.y+p.h-33,48,23},">",[this,pages]{page_=std::min(pages-1,page_+1);});canvas_.text(std::to_string(stored.size())+" stored fish",p.x+p.w*.5f,p.y+p.h-33,13,ink,true);
}
void View::quests(){
 const auto& d=session_.domain();const Rect p=panelRect_;constexpr int per=4;
 if(d.questDefinitions().empty()){
  canvas_.label("Daily quests are coming soon",p.x+p.w*.5f,p.y+112,28,true,p.w-80);
  canvas_.text("Keep raising fish and shaping your aquarium.",p.x+p.w*.5f,p.y+163,22,theme::body,true,p.w-80);return;
 }
 const int pages=std::max(1,(int(d.questDefinitions().size())+per-1)/per);page_=std::clamp(page_,0,pages-1);
 const float rowH=(p.h-133)/per;
 for(int i=0;i<per;++i){const int index=page_*per+i;if(index>=int(d.questDefinitions().size()))break;
  const auto& q=d.questDefinitions()[index];const float y=p.y+69+i*rowH;
  canvas_.skin("card",{p.x+22,y,p.w-44,rowH-9},24);
  const auto it=d.state().quests.find(q.id);const auto progress=it==d.state().quests.end()?ObjectiveProgress{}:it->second;
  const bool locked=d.level()<q.level,ready=!locked&&q.configured&&progress.count>=q.target&&!progress.claimed;
  canvas_.label((q.weekly?"Weekly: ":"")+q.label,p.x+39,y+10,23,false,p.w-230);
  std::string detail;
  if(locked)detail="Unlocks at Level "+std::to_string(q.level);
  else if(!q.configured)detail="Rewards coming in a future update";
  else {detail=std::to_string(progress.count)+" / "+std::to_string(q.target);if(q.xp)detail+="   +"+compact(q.xp)+" XP";if(q.coins)detail+="   +"+compact(q.coins)+" coins";if(q.pearls)detail+="   +"+compact(q.pearls)+" pearls";}
  canvas_.text(detail,p.x+39,y+57,20,theme::body,false,p.w-230);
  const std::string label=progress.claimed?"Claimed":locked?"Level "+std::to_string(q.level):!q.configured?"Soon":ready?"Claim":"In progress";
  glassButton("claim"+q.id,{p.x+p.w-179,y+23,137,55},label,[this,id=q.id]{command({Action::ClaimQuest,{}, {},id});},ready?"green":"blue",22);
 }
 glassButton("quest-prev",{p.x+24,p.y+p.h-53,116,37},"Previous",[this]{page_=std::max(0,page_-1);},"blue",20);
 glassButton("quest-next",{p.x+p.w-140,p.y+p.h-53,116,37},"Next",[this,pages]{page_=std::min(pages-1,page_+1);},"blue",20);
 glassButton("go-gifts",{p.x+p.w*.5f-113,p.y+p.h-57,226,45},"Gifts",[this]{open(Panel::Gifts);},"blue",23);
}
void View::gifts(){
 const auto& d=session_.domain();const Rect p=panelRect_;
 canvas_.icon("theme/starter.png",{p.x+p.w*.5f-55,p.y+79,110,110});
 canvas_.label("Marina's Gifts",p.x+p.w*.5f,p.y+202,30,true,p.w-60);
 canvas_.text("Gifting and Daily Egg rewards are coming in a future update.",p.x+p.w*.5f,p.y+272,23,theme::body,true,p.w-65);
 const float bw=(p.w-88)*.5f;
 glassButton("send-gift",{p.x+30,p.y+p.h-112,bw,62},d.level()<5?"Gifting: Level 5":"Gifting: Coming soon",[this]{command({Action::SendGift});},"blue",25);
 glassButton("daily-egg",{p.x+58+bw,p.y+p.h-112,bw,62},d.level()<6?"Daily Egg: Level 6":"Daily Egg: Coming soon",[this]{command({Action::DailyEgg});},"blue",25);
}
void View::tutorial(){
 const auto& d=session_.domain();int step=d.state().tutorialStep;if(step>=11)return;float w=canvas_.width(),h=canvas_.height();constexpr std::array<const char*,11> tips{"Choose your aquarium look","Meet your fish: choose FOOD, then tap the water","Open SHOP and choose a fish","Tap the aquarium to place your egg","Try one free growth demonstration","Choose SELL and catch your Junior fish","Open SHOP > DECOR and decorate your aquarium","Earn 80 XP through ordinary purchases and sales","Complete and claim a quest","Visit Marina and send a gift","Your next visit: feed before the hungry timer closes"};
 float pw=std::min(w-100.f,560.f),ph=42;float x=(w-pw)*.5f,y=h-(h<450?112:142);canvas_.round({x,y,pw,ph},{245,233,191,237},16,{149,143,111,255},2,true);canvas_.text(tips[step],x+pw*.5f-35,y+8,12,ink,true,pw-110);
 button("tutorial-action",{x+pw-76,y+6,67,28},step==4?"GROW":"NEXT",[this,step]{if(step==2||step==3)open(Panel::Shop);else if(step==6){open(Panel::Shop);category_=1;}else if(step==8)open(Panel::Quests);else if(step==9)open(Panel::Gifts);else if(step==1)setTool(Tool::Food);else if(step==5)setTool(Tool::Sell);else command({Action::Tutorial});},green);
 if(step==0){float yy=y-40;for(int i=0;i<3;++i)button("look"+std::to_string(i),{x+float(i)*(pw/3)+3,yy,pw/3-6,30},i==0?"LAGOON":i==1?"SUNSET":"TWILIGHT",[this,i]{command({Action::SetLook,{}, {},"",{},Currency::Coins,double(i)});},i==0?aqua:i==1?cream:purple);}
}
void View::effects(){
 while(!receipts_.empty()&&now_-receipts_.front().start>.8)receipts_.pop_front();
 // World feedback stays behind the selected fish's information card.
 if(panel_==Panel::Details)return;
 for(auto& r:receipts_){double t=(now_-r.start)/.8;auto at=canvas_.toScreen(r.event.position);float alpha=float(std::min(t/.08,std::min(1.,(1-t)/.27)));std::string label=r.event.text;auto part=[](Amount x,const char* unit){return (x>=0?"+":"")+compact(x)+unit;};if(r.event.coins||r.event.xp||r.event.pearls){label="";if(r.event.coins)label+=part(r.event.coins,"c  ");if(r.event.xp)label+=part(r.event.xp," XP  ");if(r.event.pearls)label+=part(r.event.pearls,"p");}float y=at.y+6-float(t)*39;canvas_.text(label,at.x+1,y+1,16,{39,72,86,static_cast<Uint8>(255*alpha)},true,280,true);canvas_.text(label,at.x,y,16,{255,237,178,static_cast<Uint8>(255*alpha)},true,280,true);}
}
FishId View::hitFish(float x,float y,bool net)const{
 const auto& d=session_.domain();double nearest=std::numeric_limits<double>::max(),nearestAny=nearest;FishId best{},any{};
 const auto hit=[&](const Fish& f){if(f.stashed||f.tank!=d.state().activeTank)return;auto* s=d.content().find(f.species);auto p=canvas_.toScreen(f.position);auto size=canvas_.fishSize(*s,f);double w=size.x;double dx=x-p.x,dy=y-p.y;double rx=net?w*.5+20*canvas_.worldScale():std::max(24.,w*.5),ry=net?w*.28+20*canvas_.worldScale():std::max(22.,double(size.y)*.5);
  if(net?(std::abs(dx)>w*.5+20*canvas_.worldScale()||std::abs(dy)>double(size.y)*.5+20*canvas_.worldScale()):(dx*dx/(rx*rx)+dy*dy/(ry*ry)>1))return;double distance=dx*dx+dy*dy;if(net&&f.id==selected_){best=f.id;nearest=-1;return;}if(distance<nearestAny){nearestAny=distance;any=f.id;}if(distance<nearest){nearest=distance;best=f.id;}
 };
 for(const auto& fish:d.state().fish)hit(fish);
 if(!net)for(const auto& display:d.state().companions)hit(companionVisual(display));
 return best.value?best:any;
}
void View::aquariumPress(){
 auto p=canvas_.toWorld(pointerX_,pointerY_);
 if(tool_==Tool::Buy&&!buySpecies_.empty()){
  if(armed_&&std::hypot(pointerX_-armX_,pointerY_-armY_)<canvas_.minimumTouchSize()*24/44)return;
  armed_=false;
  p={std::clamp(p.x,0.,waterWidth),std::clamp(p.y,0.,waterHeight)};
  const auto* species=session_.domain().content().find(buySpecies_);
  const auto result=command({.action=Action::Buy,.key=buySpecies_,.point=p,.offer=buyOffer_});
  if(!result){
   if(result.error!=Error::Funds)receipts_.push_back({Event{.position=p,.text=errorText(result.error)},now_});
   setTool(Tool::Select);
  }else if(!species||species->companion||species->oneTime||species->annual)setTool(Tool::Select);
  return;
 }
 if(!inTank(p))return;
 if(decorPreview_){
  decorGestureStart_=decorPreview_;dragOffset_={};
  if(decorPreviewRect().has(pointerX_,pointerY_))dragOffset_={decorPreview_->x-p.x,decorPreview_->y-p.y};
  else decorPreview_=decorPreviewPoint(p);
  placementGesture_=true;SDL_CaptureMouse(true);return;
 }
 if(tool_==Tool::Select||tool_==Tool::Decor){selectItem(pointerX_,pointerY_);return;}
 if(!inPlacementWater(p)){
  return;
 }
 if(armed_){if(std::hypot(pointerX_-armX_,pointerY_-armY_)<24)return;armed_=false;}
 if(tool_==Tool::Food){if(command({Action::DropFood,{}, {},"",p}))jarAt_=now_;return;}
 if(tool_==Tool::Restore){if(command({Action::Restore,restore_,{},"",p}))setTool(Tool::Select);return;}
 auto id=hitFish(pointerX_,pointerY_,tool_==Tool::Sell);
 if(!id.value){selected_={};selectedDecor_=0;return;}
 selectedDecor_=0;
 if(tool_==Tool::Sell){open(Panel::Details);selected_=id;return;}

 selected_=id;open(Panel::Details);selected_=id;
}
void View::pointerDown(float x,float y){
 if(pointerDown_)return;pointerSeen_=true;pointerTouch_=touchOwned_;pointerDown_=true;pointerX_=x;pointerY_=y;pressStarted_=now_;pressed_.clear();worldGesture_=false;placementGesture_=false;
 pageGesture_=pageSwiping_=pageDragCancelled_=false;pageSettleFrom_=pageDragOffset_=0;
 if(session_.notification()&&storageNoticeRect_.has(x,y)){
  for(auto it=buttons_.rbegin();it!=buttons_.rend();++it)if(it->id.starts_with("save-")&&it->area.has(x,y)){press(it->id,true);pressed_=it->id;break;}
  return;
 }
 if(dialogBlocking()){
  for(const auto& b:buttons_)if(b.area.has(x,y)){press(b.id,true);pressed_=b.id;return;}
  // Wait for the dialog's first visible frame, and let closing layers keep
  // blocking input. Only the top dialog owns a backdrop press.
  if(buttons_.empty())return;
  const auto dismissOutside=[&](const DialogAnimation& animation,bool visible,void(View::*dismiss)()){
   if(visible&&animation.layer&&animation.pose.alpha>0&&!animation.pose.apply(animation.bounds).has(x,y))(this->*dismiss)();
  };
  if(!levelUps_.empty()||levelAnimation_.layer)dismissOutside(levelAnimation_,!levelUps_.empty(),&View::dismissLevelUp);
  else if(fundsDialog_||noticeDialog_||fundsAnimation_.layer)dismissOutside(fundsAnimation_,fundsDialog_||noticeDialog_,&View::dismissFunds);
  else dismissOutside(helpAnimation_,helpOpen_,&View::dismissHelp);
  return;
 }
 // Opening Currency Shop invalidates the previous frame's controls and bounds.
 // Consume queued taps until the new panel has supplied its own hit targets.
 if(panel_==Panel::CurrencyShop&&(buttons_.empty()||paintedPanel_!=panel_))return;
 if(panel_==Panel::None&&panelLayer_&&(paintedPanel_==Panel::Shop||paintedPanel_==Panel::CurrencyShop||panelDisplayRect_.has(x,y)))return;
 // Placement owns every primary press, including presses over aquarium controls.
 if(panel_==Panel::None&&tool_==Tool::Buy&&!buySpecies_.empty()){
  for(const auto& b:buttons_)if(b.id=="done"&&b.area.has(x,y)){press(b.id,true);pressed_=b.id;return;}
  aquariumPress();return;
 }
 std::size_t firstButton=0;
 if(panel_==Panel::Shop||panel_==Panel::CurrencyShop){
  Rect r=panelDisplayRect_;
  firstButton=panelButtonStart_;
  // The Shop close button straddles the frame. Its outer half must receive
  // a normal press and release before treating this tap as a backdrop tap.
  const bool overClose=std::any_of(buttons_.begin()+firstButton,buttons_.end(),[&](const Button& b){
   if(b.id!="panel-close")return false;
   const float ex=touchOwned_?std::max(0.f,(canvas_.minimumTouchSize()-b.area.w)*.5f):0;
   const float ey=touchOwned_?std::max(0.f,(canvas_.minimumTouchSize()-b.area.h)*.5f):0;
   return Rect{b.area.x-ex,b.area.y-ey,b.area.w+2*ex,b.area.h+2*ey}.has(x,y);
  });
  if(!r.has(x,y)&&!overClose){if(panel_==Panel::CurrencyShop)closeCurrencyShop();else open(Panel::None);return;}
  if(panel_==Panel::Shop&&category_<=2&&panelPose_.apply(pageArea_).has(x,y)){
   pageGesture_=true;pageStartX_=x;pageStartY_=y;
  }
 }
 if(panel_!=Panel::None&&panelDisplayRect_.has(x,y))firstButton=panelButtonStart_;
 for(std::size_t i=buttons_.size();i>firstButton;--i){const auto& b=buttons_[i-1];if(b.area.has(x,y)){press(b.id,true);pressed_=b.id;return;}}
 // Expanded touch target uses nearest center only after exact visual hits fail.
 if(touchOwned_){const Button* nearest=nullptr;float dist=1e9;for(std::size_t i=firstButton;i<buttons_.size();++i){const auto& b=buttons_[i];float ex=std::max(0.f,(canvas_.minimumTouchSize()-b.area.w)*.5f),ey=std::max(0.f,(canvas_.minimumTouchSize()-b.area.h)*.5f);if(Rect{b.area.x-ex,b.area.y-ey,b.area.w+2*ex,b.area.h+2*ey}.has(x,y)){float dd=std::hypot(x-b.area.x-b.area.w*.5f,y-b.area.y-b.area.h*.5f);if(dd<dist){dist=dd;nearest=&b;}}}if(nearest){press(nearest->id,true);pressed_=nearest->id;return;}}
 if(panel_==Panel::Details){
  if(panelDisplayRect_.has(x,y))return;
  if((tool_==Tool::Select||tool_==Tool::Decor)&&inTank(canvas_.toWorld(x,y))){worldGesture_=true;selectItem(x,y);}else open(Panel::None);
  return;
 }
 if(panel_!=Panel::None){if(!panelDisplayRect_.has(x,y))open(Panel::None);return;}worldGesture_=true;aquariumPress();
}
float View::pageOffset()const{
 if(reducedMotion())return 0;
 if(pageSwiping_)return pageDragOffset_;
 const float remaining=1-float(std::clamp((now_-pageSettleAt_)/.22,0.,1.));
 return pageSettleFrom_*remaining*remaining*remaining;
}
void View::pointerMove(float x,float y){
 pointerSeen_=true;pointerTouch_=touchOwned_;pointerX_=x;pointerY_=y;
 if(dialogBlocking())return;
 if(selectionGesture_){moveSelection(x,y);return;}
 if(pointerDown_&&pageGesture_){
  const float dx=x-pageStartX_,dy=y-pageStartY_,slop=canvas_.minimumTouchSize()*10/44;
  if(!pageSwiping_&&std::hypot(dx,dy)>slop){
   if(!pressed_.empty())press(pressed_,false);pressed_.clear();
   if(std::abs(dx)>std::abs(dy)*1.2f)pageSwiping_=true;
   else{pageDragCancelled_=true;pageGesture_=false;return;}
  }
  if(pageSwiping_){
   pageDragOffset_=std::clamp(dx,-pageArea_.w,pageArea_.w);
   if((page_==0&&dx>0)||(page_==pageCount_-1&&dx<0))pageDragOffset_*=.2f;
   return;
  }
 }
 if(armed_&&std::hypot(x-armX_,y-armY_)>=(tool_==Tool::Buy?canvas_.minimumTouchSize()*24/44:24))armed_=false;
 if(panel_==Panel::None&&decorPreview_&&pointerDown_&&worldGesture_&&placementGesture_){
  const auto point=canvas_.toWorld(x,y);
  const bool overControl=std::any_of(buttons_.begin(),buttons_.end(),[&](const auto& b){return b.area.has(x,y);});
  if(inTank(point)&&!overControl)decorPreview_=decorPreviewPoint({point.x+dragOffset_.x,point.y+dragOffset_.y});
 }
 if(tool_==Tool::Sell)selected_=pointerInTank()?hitFish(x,y,true):FishId{};
}
void View::pointerUp(float x,float y){
 pointerX_=x;pointerY_=y;if(!pointerDown_)return;
 if(selectionGesture_){moveSelection(x,y);finishSelection(x,y);pointerDown_=worldGesture_=false;return;}
 if(pageGesture_)pointerMove(x,y);
 if(pageSwiping_||pageDragCancelled_){
  const float dx=x-pageStartX_,unit=canvas_.minimumTouchSize()/44.f;
  const bool committed=pageSwiping_&&(std::abs(dx)>=36*unit||(std::abs(dx)>=18*unit&&now_-pressStarted_<.3));
  const int next=committed?std::clamp(page_+(dx<0?1:-1),0,pageCount_-1):page_;
  pageSettleFrom_=pageDragOffset_+float(next-page_)*pageArea_.w;pageSettleAt_=now_;page_=next;
  pageGesture_=pageSwiping_=pageDragCancelled_=pointerDown_=worldGesture_=false;pressed_.clear();return;
 }
 pageGesture_=false;
 std::string id=pressed_;if(!id.empty())press(id,false);pointerDown_=false;pressed_.clear();
 if(!id.empty()){
  auto i=std::find_if(buttons_.begin(),buttons_.end(),[&](auto& b){return b.id==id;});
  if(i!=buttons_.end()){
   Rect r=i->area;if(touchOwned_){float ex=std::max(8.f,(canvas_.minimumTouchSize()-r.w)*.5f),ey=std::max(8.f,(canvas_.minimumTouchSize()-r.h)*.5f);r.x-=ex;r.y-=ey;r.w+=2*ex;r.h+=2*ey;}
   if(r.has(x,y)){auto action=i->action;action();}
  }
 }
 if(worldGesture_&&placementGesture_&&id.empty()){
  const auto point=canvas_.toWorld(x,y);
  const bool overControl=std::any_of(buttons_.begin(),buttons_.end(),[&](const auto& b){return b.area.has(x,y);});
  if((decorPreview_?inTank(point):inPlacementWater(point))&&!overControl){
   if(decorPreview_)decorPreview_=decorPreviewPoint({point.x+dragOffset_.x,point.y+dragOffset_.y});
  }else if(decorGestureStart_)decorPreview_=decorGestureStart_;
 }
 decorGestureStart_.reset();SDL_CaptureMouse(false);
 placementGesture_=worldGesture_=false;
}
void View::event(const SDL_Event& original,double t){now_=t;SDL_Event e=original;
 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP){auto p=canvas_.inputPoint(e.button.x,e.button.y);e.button.x=p.x;e.button.y=p.y;}
 if(e.type==SDL_EVENT_MOUSE_MOTION){auto p=canvas_.inputPoint(e.motion.x,e.motion.y);e.motion.x=p.x;e.motion.y=p.y;}
 if(e.type==SDL_EVENT_FINGER_DOWN||e.type==SDL_EVENT_FINGER_UP||e.type==SDL_EVENT_FINGER_MOTION){auto p=canvas_.inputPoint(e.tfinger.x,e.tfinger.y,true);e.tfinger.x=p.x/canvas_.width();e.tfinger.y=p.y/canvas_.height();}

 if(!levelUps_.empty()&&e.type==SDL_EVENT_KEY_DOWN){if(!e.key.repeat&&(e.key.key==SDLK_ESCAPE||e.key.key==SDLK_RETURN||e.key.key==SDLK_SPACE))dismissLevelUp();return;}
 if(!levelUps_.empty()&&e.type==SDL_EVENT_MOUSE_WHEEL)return;
 if((fundsDialog_||noticeDialog_)&&e.type==SDL_EVENT_KEY_DOWN){if(!e.key.repeat&&(e.key.key==SDLK_ESCAPE||(noticeDialog_&&(e.key.key==SDLK_RETURN||e.key.key==SDLK_SPACE))))dismissFunds();return;}
 if((fundsDialog_||noticeDialog_)&&e.type==SDL_EVENT_MOUSE_WHEEL)return;
 if(helpOpen_&&e.type==SDL_EVENT_KEY_DOWN){if(e.key.key==SDLK_ESCAPE)dismissHelp();return;}
 if(dialogBlocking()&&(e.type==SDL_EVENT_KEY_DOWN||e.type==SDL_EVENT_MOUSE_WHEEL))return;
 if(panel_==Panel::CurrencyShop&&e.type==SDL_EVENT_KEY_DOWN){if(e.key.key==SDLK_ESCAPE)closeCurrencyShop();return;}
 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.which!=SDL_TOUCH_MOUSEID&&e.button.button==SDL_BUTTON_LEFT&&!touchOwned_)pointerDown(e.button.x,e.button.y);
 else if(e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.which!=SDL_TOUCH_MOUSEID&&e.button.button==SDL_BUTTON_LEFT&&!touchOwned_)pointerUp(e.button.x,e.button.y);
 else if(e.type==SDL_EVENT_MOUSE_MOTION&&e.motion.which!=SDL_TOUCH_MOUSEID&&!touchOwned_)pointerMove(e.motion.x,e.motion.y);
 else if(e.type==SDL_EVENT_FINGER_DOWN){if(touchOwned_||pointerDown_)return;touchOwned_=true;finger_=e.tfinger.fingerID;pointerDown(e.tfinger.x*canvas_.width(),e.tfinger.y*canvas_.height());}
 else if(e.type==SDL_EVENT_FINGER_MOTION&&touchOwned_&&e.tfinger.fingerID==finger_)pointerMove(e.tfinger.x*canvas_.width(),e.tfinger.y*canvas_.height());
 else if(e.type==SDL_EVENT_FINGER_UP&&touchOwned_&&e.tfinger.fingerID==finger_){pointerUp(e.tfinger.x*canvas_.width(),e.tfinger.y*canvas_.height());touchOwned_=false;}
 else if(e.type==SDL_EVENT_KEY_DOWN&&!e.key.repeat){if(e.key.key==SDLK_ESCAPE){if(panel_!=Panel::None)open(Panel::None);else setTool(Tool::Select);}else if(e.key.key==SDLK_F)setTool(Tool::Food);else if(e.key.key==SDLK_S)setTool(Tool::Sell);else if(e.key.key==SDLK_B)open(Panel::Shop);else if(e.key.key==SDLK_I){inventoryCategory_=1;setTool(Tool::Select);open(Panel::Inventory);}}
 else if(e.type==SDL_EVENT_MOUSE_WHEEL&&panel_!=Panel::None){page_=std::max(0,page_+(e.wheel.y<0?1:-1));}
 else if(e.type==SDL_EVENT_WINDOW_MOUSE_LEAVE){pointerSeen_=false;canvas_.cursor(CursorKind::Arrow);}
 else if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WINDOW_RESIZED||e.type==SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND){pointerSeen_=false;canvas_.cursor(CursorKind::Arrow);cancelGesture();}
}
void View::fixture(std::string_view name){
 if(name=="quests"||name=="gifts"){
  open(name=="quests"?Panel::Quests:Panel::Gifts);panelMotion_.settle();return;
 }
 if(name=="environment"||name=="food-shop"){
  open(Panel::Shop);category_=name=="environment"?3:4;panelMotion_.settle();return;
 }
 if(name=="help"){open(Panel::None);showHelp();return;}
 if(name.starts_with("level-up")){open(Panel::None);return;}
 if(name=="not-enough-coins"||name=="not-enough-pearls"){
  showFunds(name=="not-enough-coins"?CurrencyShortfall{240,0}:CurrencyShortfall{0,12});return;
 }
 if(name=="coin-balance"||name=="pearl-balance"){openCurrencyShop(name=="pearl-balance"?Currency::Pearls:Currency::Coins);panelMotion_.settle();return;}
 if(name=="currency-shop"||name=="currency-pearls"){openCurrencyShop(name=="currency-pearls"?Currency::Pearls:Currency::Coins);panelMotion_.settle();return;}
 if(name=="plants"||name=="decor-intro"||name=="decorations"){open(Panel::Shop);category_=name=="decorations"?2:1;panelMotion_.settle();return;}
 if(name=="decor-scene"){setTool(Tool::Decor);return;}
 if(name.starts_with("details")){
  const bool reference=name=="details-reference";
  auto& d=session_.domain();Domain sample(d.content(),d.state().calendarNow);const auto bought=sample.execute({.action=Action::Buy,.key=reference?"platy":"neonTetra",.point={490,330}});auto state=sample.state();std::erase_if(state.fish,[&](const Fish& fish){return fish.id!=bought.fish;});state.simNow=60000;
  auto& fish=state.fish.front();fish.egg=false;fish.age=1;fish.growthMs=fish.purchase.durationMs*4/10;fish.lastFedAt=state.simNow-fish.purchase.feedMs;
  if(name=="details-fed")fish.lastFedAt=state.simNow;
  if(name=="details-baby"||reference){fish.age=0;fish.growthMs=6000;}
  if(reference){
   const float u=std::min(canvas_.width()/1338.f,canvas_.height()/1002.f);const auto size=canvas_.fishSize(*d.content().find(fish.species),fish);
   fish.position=canvas_.toWorld(936*u+size.x*.5f,461.5f*u);fish.motion.previous=fish.position;
  }
  if(name=="details-adult"||name=="details-display"){fish.age=4;fish.growthMs=fish.purchase.durationMs;fish.lastFedAt=state.simNow;}
  if(name=="details-egg"){fish.egg=true;fish.age=0;fish.growthMs=0;fish.boughtAt=state.simNow;fish.hatchAt=state.simNow+6000;}
  const auto id=fish.id;d.install(std::move(state));if(name=="details-display")d.execute({.action=Action::Keep,.fish=id});open(Panel::Details);selected_=id;panelMotion_.settle();return;
 }
 referencePreview_=name=="shop"||name=="tanks"||name=="collection"||name=="settings";if(name=="aquarium")open(Panel::None);else if(name=="shop")open(Panel::Shop);else if(name=="tanks")open(Panel::Tanks);else if(name=="collection")open(Panel::Collection);else if(name=="settings")open(Panel::Settings);else if(name=="inventory")open(Panel::Inventory);else if(name=="care")setTool(Tool::Food);panelMotion_.settle();}
}
