#include "aquarium/view.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <sstream>
namespace aq {
namespace {
constexpr Color aqua{159,225,216,255},blue{117,197,215,255},green{161,217,144,255},cream{247,235,197,255},muted{177,195,197,255},ink{32,77,96,255},purple{199,172,230,255};
}
View::View(Canvas& c,Session& s):canvas_(c),session_(s){}
void View::toast(std::string t){if(t.empty())return;if(toasts_.size()>=3)toasts_.pop_front();toasts_.push_back({std::move(t),now_});}
Result View::command(Command c){auto r=session_.command(c);if(!r)toast(r.message.empty()?errorText(r.error):r.message);else if(session_.domain().state().settings.sound)canvas_.sound(600,float(session_.domain().state().settings.volume));return r;}
void View::cancelGesture(){if(dragged_.value){session_.command({Action::Move,dragged_,{},"",dragOriginal_});dragged_={};}if(!pressed_.empty())press(pressed_,false);pointerDown_=false;touchOwned_=false;worldGesture_=false;pressed_.clear();SDL_CaptureMouse(false);}
void View::open(Panel p){
 cancelGesture();closeSelectMenu();
 if(p!=Panel::None&&p!=paintedPanel_)panelMotion_={};
 panelMotion_.show(p!=Panel::None,now_);panel_=p;page_=0;
 if(p!=Panel::Details)selected_={};
}
void View::setPanel(Panel p){open(p);}
void View::setTool(Tool t){open(Panel::None);tool_=t;restore_={};buySpecies_.clear();decorId_.clear();armed_=false;}
void View::armBuy(std::string id){setTool(Tool::Buy);buySpecies_=std::move(id);armX_=pointerX_;armY_=pointerY_;armed_=true;toast("Tap inside the aquarium to place an egg. Done cancels.");}
void View::armRestore(FishId id){setTool(Tool::Restore);restore_=id;armX_=pointerX_;armY_=pointerY_;armed_=true;}
void View::armDecor(std::string id){setTool(Tool::Decor);decorId_=std::move(id);armX_=pointerX_;armY_=pointerY_;armed_=true;}
void View::button(std::string id,Rect rect,std::string label,std::function<void()> action,Color color,std::string icon){
 Rect display=buttonVisual(id,rect);canvas_.round(display,color,std::min(18.f,display.h*.4f),{43,100,119,255},2,true);
 float textY=display.y+display.h*.28f;if(!icon.empty()){float size=std::min(display.w*.65f,display.h*.59f);canvas_.image(icon,{display.x+(display.w-size)*.5f,display.y+2,size,size});textY=display.y+display.h*.69f;}
 if(!label.empty())canvas_.text(label,display.x+display.w*.5f,textY,icon.empty()?std::clamp(display.h*.31f,10.f,18.f):std::clamp(display.h*.18f,9.f,12.f),ink,true,display.w-9,true);
 rect.x+=canvas_.origin();rect.y+=canvas_.originY();buttons_.push_back({std::move(id),rect,std::move(action)});
}
void View::render(double t){now_=t;buttons_.clear();auto& d=session_.domain();if(panel_==Panel::Details&&!d.fish(selected_))open(Panel::None);canvas_.begin();canvas_.origin(0);canvas_.scene(d,session_.interpolation(),t,tool_,held(),panel_==Panel::None||panel_==Panel::Details);
 ui();panelButtonStart_=buttons_.size();
 if(panel_!=Panel::None)panel();
 else if(panelLayer_){
  panelPose_=menuPose(panelMotion_,panelAnchor_);
  // A closing snapshot has no controls and never re-renders removed fish.
  if(panelPose_.alpha<=0||layerWidth_!=canvas_.width()||layerHeight_!=canvas_.height()){panelLayer_.reset();paintedPanel_=Panel::None;}
  else{
   panelDisplayRect_=panelPose_.apply(panelLayoutRect_);
   if(paintedPanel_==Panel::Shop)canvas_.fill({0,0,canvas_.width(),canvas_.height()},{0,25,55,Uint8(120*panelPose_.alpha)});
   canvas_.menuLayer(*panelLayer_,panelPose_);
  }
 }
 if(panel_==Panel::None&&tutorialVisible_)tutorial();effects();
 for(auto e:d.takeEvents()){if(receipts_.size()>=32)receipts_.pop_front();receipts_.push_back({e,t});if(e.kind=="level")toast(e.text+(e.pearls?"  +1 pearl":""));if(d.state().settings.sound)canvas_.sound(e.kind=="feed"?890:520,float(d.state().settings.volume));}
 for(auto it=presses_.begin();it!=presses_.end();)if(!it->second.down&&t-it->second.start>1)it=presses_.erase(it);else ++it;
}
std::string View::price(const Species& s)const{return s.currency==Currency::Gift?"FREE":compact(s.price)+(s.currency==Currency::Pearls?" pearls":" coins");}
void View::inventory(){
 auto& d=session_.domain();Rect p=panelRect_;std::vector<const Fish*> stored;for(auto& f:d.state().fish)if(f.stashed)stored.push_back(&f);if(stored.empty()){canvas_.image("ui/inventory.png",{p.x+p.w*.5f-52,p.y+p.h*.35f-25,104,104});canvas_.text("A cozy place for a little break",p.x+p.w*.5f,p.y+p.h*.66f,18,ink,true,p.w-30,true);canvas_.text("Use STASH on a living fish or egg. Its care clock pauses here.",p.x+p.w*.5f,p.y+p.h*.79f,14,ink,true,p.w-40);return;}
 int pages=std::max(1,(static_cast<int>(stored.size())+7)/8);page_=std::clamp(page_,0,pages-1);float gap=10,cw=(p.w-54)/4,ch=(p.h-104)/2;
 for(int i=0;i<8;++i){int ix=page_*8+i;if(ix>=static_cast<int>(stored.size()))break;const auto& f=*stored[ix];const auto& s=*d.content().find(f.species);Rect r{p.x+12+(i%4)*(cw+gap),p.y+56+(i/4)*(ch+gap),cw,ch};canvas_.round(r,{232,245,224,255},13,{116,174,166,255},2,false);canvas_.text(s.name,r.x+cw*.5f,r.y+7,20,ink,true,cw-12,true);canvas_.icon(f.egg?"ui/egg.png":s.asset,{r.x+cw*.15f,r.y+ch*.24f,cw*.7f,ch*(ch<140?.27f:.39f)});canvas_.text(stageName(f)+" / PAUSED",r.x+cw*.5f,r.y+ch-53,12,ink,true,cw-10);button("restore"+std::to_string(f.id.value),{r.x+8,r.y+ch-30,cw-16,25},"PLACE",[this,id=f.id]{armRestore(id);},green);}
 button("inv-prev",{p.x+14,p.y+p.h-33,48,23},"<",[this]{page_=std::max(0,page_-1);});button("inv-next",{p.x+p.w-62,p.y+p.h-33,48,23},">",[this,pages]{page_=std::min(pages-1,page_+1);});canvas_.text(std::to_string(stored.size())+" stored fish",p.x+p.w*.5f,p.y+p.h-33,13,ink,true);
}
void View::quests(){
 const auto& d=session_.domain();const Rect p=panelRect_;constexpr int per=4;
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
  else {detail=std::to_string(progress.count)+" / "+std::to_string(q.target);if(q.xp)detail+="   +"+compact(q.xp)+" XP";if(q.coins)detail+="   +"+compact(q.coins)+" coins";if(q.tokens)detail+="   +"+compact(q.tokens)+" Gift Tokens";if(q.pearls)detail+="   +"+compact(q.pearls)+" pearls";}
  canvas_.text(detail,p.x+39,y+57,20,{225,248,255,255},false,p.w-230);
  const std::string label=progress.claimed?"Claimed":locked?"Level "+std::to_string(q.level):!q.configured?"Soon":ready?"Claim":"In progress";
  glassButton("claim"+q.id,{p.x+p.w-179,y+23,137,55},label,[this,id=q.id]{command({Action::ClaimQuest,{}, {},id});},ready?"green":"blue",22);
 }
 glassButton("quest-prev",{p.x+24,p.y+p.h-53,116,37},"Previous",[this]{page_=std::max(0,page_-1);},"blue",20);
 glassButton("quest-next",{p.x+p.w-140,p.y+p.h-53,116,37},"Next",[this,pages]{page_=std::min(pages-1,page_+1);},"blue",20);
 glassButton("go-gifts",{p.x+p.w*.5f-113,p.y+p.h-57,226,45},"Gifts",[this]{open(Panel::Gifts);},"blue",23);
}
void View::gifts(){
 const auto& d=session_.domain();const Rect p=panelRect_;
 canvas_.icon("ui/inventory.png",{p.x+p.w*.5f-55,p.y+79,110,110});
 canvas_.label("Gift Tokens: "+compact(d.state().giftTokens),p.x+p.w*.5f,p.y+202,30,true,p.w-60);
 canvas_.text("Use Gift Tokens with coins to expand your tanks.",p.x+p.w*.5f,p.y+257,25,{235,251,255,255},true,p.w-65);
 canvas_.text("Gifting and Daily Egg rewards are coming in a future update.",p.x+p.w*.5f,p.y+307,23,{235,251,255,255},true,p.w-65);
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
 while(!toasts_.empty()&&now_-toasts_.front().start>=2.4)toasts_.pop_front();float yy=canvas_.height()*.19f;for(auto& t:toasts_){double age=now_-t.start;float alpha=age<1.9?1.f:float((2.4-age)/.5);float w=std::min(canvas_.width()-40,610.f);canvas_.round({(canvas_.width()-w)*.5f,yy,w,35},{246,237,204,static_cast<Uint8>(245*alpha)},14,{92,130,132,static_cast<Uint8>(255*alpha)},1,true);canvas_.text(t.text,canvas_.width()*.5f,yy+8,12,{38,76,91,static_cast<Uint8>(255*alpha)},true,w-24);yy+=39;}
 while(!receipts_.empty()&&now_-receipts_.front().start>.8)receipts_.pop_front();
 // World feedback stays behind the selected fish's information card.
 if(panel_==Panel::Details)return;
 for(auto& r:receipts_){double t=(now_-r.start)/.8;auto at=canvas_.toScreen(r.event.position);float alpha=float(std::min(t/.08,std::min(1.,(1-t)/.27)));std::string label=r.event.text;auto part=[](Amount x,const char* unit){return (x>=0?"+":"")+compact(x)+unit;};if(r.event.coins||r.event.xp||r.event.pearls){label="";if(r.event.coins)label+=part(r.event.coins,"c  ");if(r.event.xp)label+=part(r.event.xp," XP  ");if(r.event.pearls)label+=part(r.event.pearls,"p");}float y=at.y+6-float(t)*39;canvas_.text(label,at.x+1,y+1,16,{39,72,86,static_cast<Uint8>(255*alpha)},true,280,true);canvas_.text(label,at.x,y,16,{255,237,178,static_cast<Uint8>(255*alpha)},true,280,true);}
}
FishId View::hitFish(float x,float y,bool net)const{
 const auto& d=session_.domain();double nearest=std::numeric_limits<double>::max(),nearestAny=nearest;FishId best{},any{};
 for(auto& f:d.state().fish){if(f.stashed||f.tank!=d.state().activeTank)continue;auto* s=d.content().find(f.species);auto p=canvas_.toScreen(f.position);auto size=canvas_.fishSize(*s,f);double w=size.x;double dx=x-p.x,dy=y-p.y;double rx=net?w*.5+20*canvas_.worldScale():std::max(24.,w*.5),ry=net?w*.28+20*canvas_.worldScale():std::max(22.,double(size.y)*.5);
  if(dx*dx/(rx*rx)+dy*dy/(ry*ry)>1)continue;double distance=dx*dx+dy*dy;if(net&&f.id==selected_&&sellable(*s,f))return f.id;if(distance<nearestAny){nearestAny=distance;any=f.id;}if((!net||sellable(*s,f))&&distance<nearest){nearest=distance;best=f.id;}
 }
 return best.value?best:any;
}
void View::aquariumPress(){
 auto p=canvas_.toWorld(pointerX_,pointerY_);if(p.x<0||p.x>1088||p.y<0||p.y>512)return;
 if(armed_){if(std::hypot(pointerX_-armX_,pointerY_-armY_)<24)return;armed_=false;}
 if(tool_==Tool::Food){if(command({Action::DropFood,{}, {},"",p}))jarAt_=now_;return;}
 if(tool_==Tool::Buy){auto r=command({Action::Buy,{}, {},buySpecies_,p});if(!r)setTool(Tool::Select);else if(auto* s=session_.domain().content().find(buySpecies_);s&&(s->oneTime||s->annual))setTool(Tool::Select);return;}
 if(tool_==Tool::Restore){if(command({Action::Restore,restore_,{},"",p}))setTool(Tool::Select);return;}
 if(tool_==Tool::Decor){if(!command({Action::BuyDecor,{}, {},decorId_,p}))setTool(Tool::Select);return;}
 auto id=hitFish(pointerX_,pointerY_,tool_==Tool::Sell);if(!id.value){selected_={};return;}
 if(tool_==Tool::Sell){if(command({Action::Sell,id})){netAt_=now_;selected_={};}return;}
 if(tool_==Tool::Medicine){const auto* f=session_.domain().fish(id);if(f&&f->dead){selected_=id;open(Panel::Details);selected_=id;}else if(f&&careOf(*session_.domain().content().find(f->species),*f,session_.domain().state().simNow)==Care::Fed)toast("This fish is healthy.");else command({Action::Feed,id});return;}
 if(tool_==Tool::Stash){command({Action::Stash,id});return;}
 if(tool_==Tool::Move){dragged_=id;dragOriginal_=session_.domain().fish(id)->position;SDL_CaptureMouse(true);return;}
 selected_=id;open(Panel::Details);selected_=id;
}
void View::pointerDown(float x,float y){
 if(pointerDown_)return;pointerDown_=true;pointerX_=x;pointerY_=y;pressStarted_=now_;pressed_.clear();worldGesture_=false;
 if(panel_==Panel::None&&panelLayer_&&(paintedPanel_==Panel::Shop||panelDisplayRect_.has(x,y)))return;
 std::size_t firstButton=0;
 if(panel_==Panel::Shop){
  Rect r=panelDisplayRect_;
  if(!r.has(x,y)){open(Panel::None);return;}
  firstButton=panelButtonStart_;
 }
 if(panel_==Panel::Details&&panelDisplayRect_.has(x,y))firstButton=panelButtonStart_;
 for(std::size_t i=buttons_.size();i>firstButton;--i){const auto& b=buttons_[i-1];if(b.area.has(x,y)){press(b.id,true);pressed_=b.id;return;}}
 // Expanded touch target uses nearest center only after exact visual hits fail.
 if(touchOwned_){const Button* nearest=nullptr;float dist=1e9;for(std::size_t i=firstButton;i<buttons_.size();++i){const auto& b=buttons_[i];float ex=std::max(0.f,(canvas_.minimumTouchSize()-b.area.w)*.5f),ey=std::max(0.f,(canvas_.minimumTouchSize()-b.area.h)*.5f);if(Rect{b.area.x-ex,b.area.y-ey,b.area.w+2*ex,b.area.h+2*ey}.has(x,y)){float dd=std::hypot(x-b.area.x-b.area.w*.5f,y-b.area.y-b.area.h*.5f);if(dd<dist){dist=dd;nearest=&b;}}}if(nearest){press(nearest->id,true);pressed_=nearest->id;return;}}
 if(panel_==Panel::Details){
  if(panelDisplayRect_.has(x,y))return;
  const auto id=hitFish(x,y);
  if(id.value){selected_=id;open(Panel::Details);}else open(Panel::None);
  return;
 }
 if(panel_!=Panel::None){if(!panelDisplayRect_.has(x,y))open(Panel::None);return;}worldGesture_=true;aquariumPress();
}
void View::pointerMove(float x,float y){pointerX_=x;pointerY_=y;if(armed_&&std::hypot(x-armX_,y-armY_)>=24)armed_=false;if(dragged_.value){auto p=canvas_.toWorld(x,y);p.x=std::clamp(p.x,0.,1088.);p.y=std::clamp(p.y,56.,512.);session_.command({Action::Move,dragged_,{},"",p});}else if(tool_==Tool::Sell&&panel_==Panel::None)selected_=hitFish(x,y,true);}
void View::pointerUp(float x,float y){pointerX_=x;pointerY_=y;if(!pointerDown_)return;std::string id=pressed_;if(!id.empty())press(id,false);pointerDown_=false;pressed_.clear();if(dragged_.value){dragged_={};session_.checkpoint(session_.domain().state().wallAnchor);SDL_CaptureMouse(false);}if(!id.empty()){auto i=std::find_if(buttons_.begin(),buttons_.end(),[&](auto& b){return b.id==id;});if(i!=buttons_.end()){Rect r=i->area;if(touchOwned_){float ex=std::max(8.f,(canvas_.minimumTouchSize()-r.w)*.5f),ey=std::max(8.f,(canvas_.minimumTouchSize()-r.h)*.5f);r.x-=ex;r.y-=ey;r.w+=2*ex;r.h+=2*ey;}if(r.has(x,y)){auto action=i->action;action();}}}worldGesture_=false;}
void View::event(const SDL_Event& original,double t){now_=t;SDL_Event e=original;
 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP){auto p=canvas_.inputPoint(e.button.x,e.button.y);e.button.x=p.x;e.button.y=p.y;}
 if(e.type==SDL_EVENT_MOUSE_MOTION){auto p=canvas_.inputPoint(e.motion.x,e.motion.y);e.motion.x=p.x;e.motion.y=p.y;}
 if(e.type==SDL_EVENT_FINGER_DOWN||e.type==SDL_EVENT_FINGER_UP||e.type==SDL_EVENT_FINGER_MOTION){auto p=canvas_.inputPoint(e.tfinger.x,e.tfinger.y,true);e.tfinger.x=p.x/canvas_.width();e.tfinger.y=p.y/canvas_.height();}

 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.which!=SDL_TOUCH_MOUSEID&&e.button.button==SDL_BUTTON_LEFT&&!touchOwned_)pointerDown(e.button.x,e.button.y);
 else if(e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.which!=SDL_TOUCH_MOUSEID&&e.button.button==SDL_BUTTON_LEFT&&!touchOwned_)pointerUp(e.button.x,e.button.y);
 else if(e.type==SDL_EVENT_MOUSE_MOTION&&e.motion.which!=SDL_TOUCH_MOUSEID&&!touchOwned_)pointerMove(e.motion.x,e.motion.y);
 else if(e.type==SDL_EVENT_FINGER_DOWN){if(touchOwned_||pointerDown_)return;touchOwned_=true;finger_=e.tfinger.fingerID;pointerDown(e.tfinger.x*canvas_.width(),e.tfinger.y*canvas_.height());}
 else if(e.type==SDL_EVENT_FINGER_MOTION&&touchOwned_&&e.tfinger.fingerID==finger_)pointerMove(e.tfinger.x*canvas_.width(),e.tfinger.y*canvas_.height());
 else if(e.type==SDL_EVENT_FINGER_UP&&touchOwned_&&e.tfinger.fingerID==finger_){pointerUp(e.tfinger.x*canvas_.width(),e.tfinger.y*canvas_.height());touchOwned_=false;}
 else if(e.type==SDL_EVENT_KEY_DOWN&&!e.key.repeat){if(e.key.key==SDLK_ESCAPE){if(panel_!=Panel::None)open(Panel::None);else setTool(Tool::Select);}else if(e.key.key==SDLK_F)setTool(Tool::Food);else if(e.key.key==SDLK_S)setTool(Tool::Sell);else if(e.key.key==SDLK_B)open(Panel::Shop);else if(e.key.key==SDLK_I)open(Panel::Inventory);else if(e.key.key==SDLK_M)setTool(Tool::Move);}
 else if(e.type==SDL_EVENT_MOUSE_WHEEL&&panel_!=Panel::None){page_=std::max(0,page_+(e.wheel.y<0?1:-1));}
 else if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WINDOW_RESIZED||e.type==SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND)cancelGesture();
}
void View::fixture(std::string_view name){
 if(name.starts_with("details")){
  auto& d=session_.domain();Domain sample(d.content(),d.state().calendarNow);auto state=sample.state();std::erase_if(state.fish,[](const Fish& fish){return fish.species!="molly"&&fish.species!="guppy";});state.xp=14;state.simNow=60000;
  for(auto& fish:state.fish){if(fish.species!="molly"){fish.position={940,156};fish.motion.previous=fish.position;fish.age=4;fish.lastFedAt=state.simNow;}else{
   if(name=="details-fed")fish.lastFedAt=state.simNow;
   if(name=="details-junior"){fish.age=1;fish.growthMs=d.content().find(fish.species)->stageMs/2;fish.lastFedAt=state.simNow;}
   if(name=="details-adult"){fish.age=4;fish.lastFedAt=state.simNow;}
   if(name=="details-sick")fish.lastFedAt=state.simNow-d.content().find(fish.species)->feedMs;
   if(name=="details-dead")fish.dead=true;
   if(name=="details-egg"){fish.egg=true;fish.hatchAt=state.simNow+6000;}
   selected_=fish.id;
  }}
  d.install(std::move(state));open(Panel::Details);panelMotion_.settle();return;
 }
 referencePreview_=name=="shop"||name=="tanks"||name=="collection"||name=="settings";if(name=="aquarium")open(Panel::None);else if(name=="shop")open(Panel::Shop);else if(name=="tanks")open(Panel::Tanks);else if(name=="collection")open(Panel::Collection);else if(name=="settings")open(Panel::Settings);else if(name=="inventory")open(Panel::Inventory);else if(name=="care")setTool(Tool::Food);panelMotion_.settle();}
}
