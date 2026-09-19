#include "aquarium/hud_level_up.hpp"
#include "aquarium/shop_theme.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
namespace {
constexpr Color levelInk{4,60,78,255};
constexpr double rewardDelay=.5,flightTime=.78,countTime=.16,pulseTime=.38;
double smooth(double value){const double t=std::clamp(value,0.,1.);return t*t*(3-2*t);}
int tokenCount(Amount value){return int(std::clamp<Amount>(value,0,7));}
double departure(int index){return rewardDelay+index*.06;}
double arrival(int index){return departure(index)+flightTime;}
float rewardPulse(Amount value,double age,bool reduced){
 float pulse=0;
 for(int i=0;i<tokenCount(value);++i){
  const double since=age-(reduced?rewardDelay:arrival(i));
  if(since>=0&&since<pulseTime)pulse=std::max(pulse,float(1-smooth(since/pulseTime)));
 }
 return pulse;
}
long double pendingReward(Amount value,double age){
 const int count=tokenCount(value);long double pending=0;
 for(int i=0;i<count;++i)
  pending+=(value/count+(i<value%count?1:0))*(1-static_cast<long double>(smooth((age-arrival(i))/countTime)));
 return pending;
}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
SDL_FPoint tokenPosition(SDL_FPoint from,SDL_FPoint to,int index,int count,double age,float unit){
 const float progress=float(std::clamp((age-departure(index))/flightTime,0.,1.));
 const float t=progress*progress,a=1-t,fan=float(index)-float(count-1)*.5f;
 const SDL_FPoint c1{from.x+fan*26*unit,from.y-(100+std::abs(fan)*12)*unit};
 const SDL_FPoint c2{to.x+fan*12*unit,to.y+90*unit};
 return {a*a*a*from.x+3*a*a*t*c1.x+3*a*t*t*c2.x+t*t*t*to.x,
         a*a*a*from.y+3*a*a*t*c1.y+3*a*t*t*c2.y+t*t*t*to.y};
}
void sparkle(Canvas& c,SDL_FPoint at,float radius,Color color){
 c.triangle({SDL_FPoint{at.x,at.y-radius},{at.x+radius*.45f,at.y},{at.x,at.y+radius}},color);
 c.triangle({SDL_FPoint{at.x,at.y-radius},{at.x-radius*.45f,at.y},{at.x,at.y+radius}},color);
}
std::string amount(Amount value){
 auto result=std::to_string(value);
 for(int i=int(result.size())-3;i>0;i-=3)result.insert(std::size_t(i),",");
 return result;
}
struct RewardSlot {Amount value{};Rect icon,number;float numberSize{};bool pearl{};};
std::array<RewardSlot,2> rewardSlots(Canvas& c,const LevelUpLayout& l,Money reward){
 std::array<RewardSlot,2> slots{};
 if(l.sideReceipt){
  const float gap=6*l.unit,row=(l.rewards.h-gap)*.5f,icon=row+8*l.unit;
  const int count=(reward.coins>0)+(reward.pearls>0);int index=0;
  for(int i=0;i<2;++i){
   auto& s=slots[i];s.pearl=i==1;s.value=s.pearl?reward.pearls:reward.coins;if(s.value<=0)continue;
   const float y=l.rewards.y+(count==1?(l.rewards.h-row)*.5f:float(index++)*(row+gap));
   s.icon={l.rewards.x,y+(row-icon)*.5f,icon,icon};
   s.number={s.icon.x+icon+10*l.unit,y,l.rewards.w-icon-10*l.unit,row};
   s.numberSize=31*l.textUnit;
  }
  return slots;
 }
 const float groupW=l.rewards.w/((reward.coins>0&&reward.pearls>0)?2:1);
 const float iconSize=std::min(l.rewards.h,groupW*.32f),font=l.compact?l.rewards.h*.8f:31*l.textUnit,gap=8*l.unit;
 int group=0;
 for(int i=0;i<2;++i){
  auto& s=slots[i];s.pearl=i==1;s.value=s.pearl?reward.pearls:reward.coins;if(s.value<=0)continue;
  const float numberW=std::min(groupW-iconSize-gap,c.textWidth(amount(s.value),font,true));
  const float left=l.rewards.x+groupW*float(group++)+(groupW-iconSize-gap-numberW)*.5f;
  s.icon={left,l.rewards.y,iconSize,iconSize};s.number={left+iconSize+gap,l.rewards.y,numberW,iconSize};
  s.numberSize=font;
 }
 return slots;
}
void text(Canvas& c,std::string_view value,Rect r,float size,Color color=levelInk,bool reference=false){
 c.text(value,r.x+r.w*.5f,r.y+(r.h-size*1.12f)*.5f,size,color,true,r.w,true,reference);
}
void leftText(Canvas& c,std::string_view value,Rect r,float size,Color color=levelInk){
 c.text(value,r.x,r.y+(r.h-size*1.12f)*.5f,size,color,false,r.w,true);
}
void itemName(Canvas& c,std::string_view name,Rect r,float size,bool centered=false){
 auto lineText=[&](std::string_view value,Rect line){if(centered)text(c,value,line,size);else leftText(c,value,line,size);};
 if(c.textWidth(name,size,true)<=r.w){lineText(name,r);return;}
 std::size_t split=std::string_view::npos;float best=1e9f;
 for(std::size_t i=0;i<name.size();++i)if(name[i]==' '){
  const float a=c.textWidth(name.substr(0,i),size,true),b=c.textWidth(name.substr(i+1),size,true);
  const float score=std::max(a,b)+std::abs(a-b)*.1f;
  if(score<best){best=score;split=i;}
 }
 if(split==std::string_view::npos){lineText(name,r);return;}
 const float line=std::min(size*1.08f,r.h*.5f),top=r.y+(r.h-2*line)*.5f;
 lineText(name.substr(0,split),{r.x,top,r.w,line});
 lineText(name.substr(split+1),{r.x,top+line,r.w,line});
}

}

std::vector<LevelUpItem> levelUpItems(const Domain& domain,int level){
 std::vector<LevelUpItem> result;
 const auto calendar=calendarAt(domain.state().calendarNow);
 for(const auto& s:domain.content().species){
  // Gifts have their own claim rules; this gallery describes shop purchases.
  if(s.level==level&&s.artReady&&s.releaseGate=="Launch"&&eventOpen(s,calendar))
   result.push_back({s.id,s.name,s.asset});
 }
 for(const auto& d:domain.content().decorations)
  if(d.level==level&&d.artReady&&d.releaseGate=="Launch"&&domain.decorVisible(d))result.push_back({d.id,d.name,d.asset});
 for(const auto& t:domain.content().tankEntitlements)if(t.slots==10&&t.level==level){
  const auto name="Tank "+std::to_string(t.tank.value);
  result.push_back({t.id,name,"tank-grid/active.png"});
 }
 return result;
}

LevelUpLayout layoutLevelUp(float width,float height,Insets safe,float minimumTouch){
 const float base=std::min(width/1608.f,height/908.f);
 const float availableW=width-safe.left-safe.right-48*base,availableH=height-safe.top-safe.bottom-48*base;
 const float scale=std::max(base,minimumTouch/64.f);
 const bool tall=availableW<availableH*.9f;
 const float w=std::min(1280*scale,availableW);
 const float h=std::min(availableH,tall?std::max(768*scale,availableH*.9f):std::max(w*.455f,minimumTouch*6.5f));
 LevelUpLayout l;
 l.dialog=layoutDialog(width,height,safe,{"Level up!",DialogSize::Custom,w/base,h/base});
 auto& d=l.dialog;
 // Keep the shared frame and touch targets, and leave both HUD destinations visible.
 const auto hud=layoutHud(width,height,safe,minimumTouch);
 const auto coin=hud[HudPart::CoinIcon],pearl=hud[HudPart::PearlIcon];
 const float walletBottom=std::max(coin.y+coin.h,pearl.y+pearl.h)+8*base;
 if(d.frame.y<walletBottom){
  d.frame.y=walletBottom;
  d.frame.h=std::min(d.frame.h,height-safe.bottom-24*base-d.frame.y);
 }
 d.unit=std::max(std::min(d.frame.w/1280.f,d.frame.h/768.f),minimumTouch/64.f);
 const float u=d.unit,close=64*u,header=80*u;
 d.header={d.frame.x+8*u,d.frame.y+8*u,d.frame.w-16*u,header};
 d.close={d.header.x+d.header.w-close-8*u,d.header.y+8*u,close,close};
 d.title={d.header.x+close+16*u,d.header.y,d.header.w-2*close-32*u,header};
 d.body={d.frame.x+8*u,d.header.y+header,d.frame.w-16*u,d.frame.h-header-16*u};
 d.content={d.body.x+24*u,d.body.y+24*u,d.body.w-48*u,d.body.h-48*u};
 const auto b=d.body;const float cx=b.x+b.w*.5f,p=minimumTouch/44.f;
 l.portrait=tall;l.compact=!tall&&b.w<480*p;l.sideReceipt=!tall&&!l.compact;
 l.unit=l.sideReceipt?std::min(b.w/704.f,b.h/264.f):std::min(b.w/360.f,b.h/650.f);
 l.textUnit=std::max(l.unit,p*.94f);
 if(l.sideReceipt){
  // Mobile reference: a fixed receipt takes 30% of the body. The gallery
  // keeps square cards, including a partial next card as the swipe cue.
  const float s=l.unit,top=b.y+(b.h-264*s)*.5f,divide=b.x+b.w*.305f;
  const float left=b.x+12*s,receiptW=divide-left-12*s;
  const float buttonH=std::max(44*s,minimumTouch);
  l.continueButton={left,b.y+b.h-8*s-buttonH,receiptW,buttonH};
  l.rewards={left+8*s,l.continueButton.y-7*s-86*s,receiptW-16*s,86*s};
  l.received={left,l.rewards.y-7*s-22*s,receiptW,22*s};
  const float badgeH=std::min(94*s,l.received.y-top-6*s);
  l.badge={left+(receiptW-badgeH*2)*.5f,top+2*s,badgeH*2,badgeH};
  l.divider={divide,top+10*s,std::max(1.f,s),b.y+b.h-8*s-top-10*s};
  const float galleryX=divide+15*s;
  l.unlocks={galleryX,top+8*s,b.x+b.w-galleryX-12*s,28*s};
  l.gallery={galleryX,top+44*s,b.x+b.w-galleryX-4*s,184*s};
  l.cardWidth=l.gallery.h;l.gap=12*s;
  l.progress={galleryX,l.gallery.y+l.gallery.h+14*s,l.gallery.w,12*s};
 }else if(tall){
  const float s=b.w/360.f;l.unit=s;l.textUnit=std::max(s,p*.94f);
  const float inset=16*s,buttonH=std::max(48*s,minimumTouch);
  l.badge={cx-98*s,b.y+6*s,196*s,98*s};
  l.received={b.x+inset,b.y+110*s,b.w-2*inset,26*s};
  l.rewards={b.x+inset,l.received.y+l.received.h+8*s,b.w-2*inset,48*s};
  l.unlocks={b.x+inset,l.rewards.y+l.rewards.h+22*s,b.w-2*inset,30*s};
  l.continueButton={b.x+inset,b.y+b.h-buttonH-14*s,b.w-2*inset,buttonH};
  const float cardY=l.unlocks.y+l.unlocks.h+12*s;
  l.cardWidth=std::min(b.w*.84f,l.continueButton.y-cardY-44*s);
  l.gallery={b.x+inset,cardY,b.w-inset-4*s,l.cardWidth};l.gap=12*s;
  l.progress={l.gallery.x,l.gallery.y+l.gallery.h+16*s,l.gallery.w,12*s};
 }else{
  // A small-window fallback keeps both actions reachable below 480 points.
  l.unit=p;l.textUnit=p;
  l.badge={b.x+4*p,b.y+2*p,76*p,38*p};
  l.received={b.x+86*p,b.y+2*p,b.w-92*p,13*p};
  l.rewards={b.x+84*p,b.y+20*p,b.w-90*p,22*p};
  l.continueButton={cx-b.w*.3f,b.y+b.h-minimumTouch-3*p,b.w*.6f,minimumTouch};
  l.unlocks={b.x+8*p,b.y+43*p,b.w-16*p,15*p};
  const float cardY=l.unlocks.y+l.unlocks.h+3*p;
  l.gallery={b.x+8*p,cardY,b.w-12*p,l.continueButton.y-cardY-6*p};
  l.cardWidth=l.gallery.w*.8f;l.gap=8*p;
 }
 l.grid=l.gallery;l.cardHeight=l.grid.h;
 return l;
}
Rect levelUpCardBounds(const LevelUpLayout& l,std::size_t index,float scroll){
 return {l.grid.x+float(index)*(l.cardWidth+l.gap)-scroll,l.grid.y,l.cardWidth,l.cardHeight};
}
float levelUpScrollLimit(const LevelUpLayout& l,std::size_t count){
 if(!count)return 0;
 return std::max(0.f,float(count)*l.cardWidth+float(count-1)*l.gap-l.grid.w);
}

void HudLevelUp::collect(const Domain& domain,const Event& e){
 if(e.kind!="level"||e.reachedLevel<=latestLevel_||e.reachedLevel>int(domain.content().levels.size()))return;
 latestLevel_=e.reachedLevel;
 notices_.push_back({e.reachedLevel,{e.coins,e.pearls},levelUpItems(domain,e.reachedLevel)});
}
void HudLevelUp::reveal(int displayedLevel){
 displayedLevel_=displayedLevel;
 if(!open()&&!notices_.empty()&&notices_.front().level<=displayedLevel_){dialog_={true};scroll_=0;scrollMotion_.stop();age_=0;cancelPress();}
}
void HudLevelUp::advance(double seconds,bool reducedMotion){
 dialog_.motion.advance(open(),seconds,reducedMotion);
 // Once travel is skipped, changing settings must not restart that reward.
 reducedMotion_=open()?(reducedMotion_||reducedMotion):reducedMotion;
 if(open()&&std::isfinite(seconds))age_=std::min(4.,age_+std::max(0.,seconds));
 if(open())scrollMotion_.advance(scroll_,seconds,scrollLimit_,reducedMotion_,scrollUnit_);
}
HudRewardDisplay HudLevelUp::display(HudRewardDisplay result)const{
 const bool reduced=reducedMotion_||result.reducedMotion;
 long double coinsPending=0,pearlsPending=0;
 for(std::size_t i=0;i<notices_.size();++i){
  const auto reward=notices_[i].reward;
  if(!reduced){
   // Queued levels wait for their own receipt before contributing to the HUD.
   coinsPending+=pendingReward(reward.coins,i==0&&open()?age_:0);
   pearlsPending+=pendingReward(reward.pearls,i==0&&open()?age_:0);
  }
  if(i==0&&open()){
   result.coinPulse=std::max(result.coinPulse,rewardPulse(reward.coins,age_,reduced));
   result.pearlPulse=std::max(result.pearlPulse,rewardPulse(reward.pearls,age_,reduced));
  }
 }
 result.coins-=static_cast<Amount>(std::ceil(std::clamp(coinsPending,0.L,static_cast<long double>(result.coins))));
 result.pearls-=static_cast<Amount>(std::ceil(std::clamp(pearlsPending,0.L,static_cast<long double>(result.pearls))));
 result.reducedMotion=reduced;
 return result;
}
void HudLevelUp::cancelPress(){pressed_=-1;dragged_=false;dialog_.closePressed=dialog_.backdropPressed=false;}
void HudLevelUp::dismiss(){
 if(!notices_.empty())notices_.pop_front();
 dialog_.open=false;scroll_=0;scrollMotion_.stop();cancelPress();reveal(displayedLevel_);
}
int HudLevelUp::control(const LevelUpLayout& l,SDL_FPoint p)const{
 if(l.continueButton.has(p.x,p.y))return 0;
 if(l.grid.has(p.x,p.y))return 1;
 return -1;
}
bool HudLevelUp::event(const LevelUpLayout& l,const SDL_Event& e,SDL_FPoint p){
 if(!open())return false;
 const auto screenPoint=p;p=dialog_.motion.inputPoint(p);
 const float limit=levelUpScrollLimit(l,current()->items.size());scroll_=std::clamp(scroll_,0.f,limit);
 const float step=l.cardWidth+l.gap;
 scrollLimit_=limit;scrollUnit_=step;
 if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND||e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET){scrollMotion_.stop();cancelPress();return false;}
 if(e.type==SDL_EVENT_KEY_DOWN){
  scrollMotion_.stop();cancelPress();
  if(e.key.key==SDLK_RETURN||e.key.key==SDLK_SPACE||e.key.key==SDLK_ESCAPE){if(!e.key.repeat)dismiss();return true;}
  if(e.key.key==SDLK_UP||e.key.key==SDLK_LEFT)scroll_-=step;
  if(e.key.key==SDLK_DOWN||e.key.key==SDLK_RIGHT)scroll_+=step;
  if(e.key.key==SDLK_PAGEUP)scroll_-=l.grid.w*.9f;
  if(e.key.key==SDLK_PAGEDOWN)scroll_+=l.grid.w*.9f;
  if(e.key.key==SDLK_HOME)scroll_=0;
  if(e.key.key==SDLK_END)scroll_=limit;
  scroll_=std::clamp(scroll_,0.f,limit);return true;
 }
 if(e.type==SDL_EVENT_MOUSE_WHEEL){
  if(l.grid.has(p.x,p.y)){
   scrollMotion_.wheel(scroll_,horizontalWheel(e)*step*.25f,limit,reducedMotion_);cancelPress();
  }
  return true;
 }
 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT){
  pressed_=control(l,p);down_=screenPoint;dragged_=pressed_==1&&scrollMotion_.moving();scrollStart_=scroll_;
  if(pressed_==1)scrollMotion_.begin(scroll_,scrollEventTime(e));else scrollMotion_.stop();
  if(pressed_>0)return true;
 }
 if(e.type==SDL_EVENT_MOUSE_MOTION&&std::hypot(screenPoint.x-down_.x,screenPoint.y-down_.y)>std::max(6.f,12*l.dialog.unit)){
  dragged_=true;dialog_.closePressed=dialog_.backdropPressed=false;
 }
 if(e.type==SDL_EVENT_MOUSE_MOTION){
  if(pressed_==1&&dragged_)scrollMotion_.drag(scroll_,scrollStart_+down_.x-screenPoint.x,limit,scrollEventTime(e),step);
  if(pressed_>0)return true;
 }
 if(e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.button==SDL_BUTTON_LEFT){
  if(pressed_==1)scrollMotion_.release(scrollEventTime(e),reducedMotion_);
  const int action=pressed_;pressed_=-1;
  const bool clicked=!dragged_&&action>=0&&action==control(l,p)&&std::hypot(screenPoint.x-down_.x,screenPoint.y-down_.y)<=std::max(6.f,12*l.dialog.unit);
  if(clicked&&action==0){dismiss();cancelPress();return true;}
  if(action>0){cancelPress();return true;}
 }
 const bool handled=dialogEvent(dialog_,l.dialog,e,screenPoint);
 if(!dialog_.open)dismiss();
 return handled;
}

void HudLevelUp::paint(Canvas& c,const LevelUpLayout& l,SDL_FPoint pointer)const{
 if(!open()||!current())return;
 const auto& notice=*current();const float u=l.textUnit,s=l.unit;
 const DialogPaint animation(c,dialog_.motion,l.dialog.frame,reducedMotion_);
 pointer=dialog_.motion.inputPoint(pointer);
 paintDialog(c,l.dialog,{"Level up!"},l.dialog.close.has(pointer.x,pointer.y),dialog_.closePressed,DialogPresentation::ModalContent);
 c.roundedImage("dialogs/level-up-mobile-background-v1.png",l.dialog.body,12*l.dialog.unit);
 c.roundedImage("dialogs/level-up-mobile-badge-v1.png",l.badge,8*s,8*s);
 const Rect face{l.badge.x+l.badge.w*.365f,l.badge.y+l.badge.h*.325f,l.badge.w*.26f,l.badge.h*.50f};
 text(c,std::to_string(notice.level),face,face.h*.84f,{94,61,12,255});
 text(c,"You received",l.received,(l.compact?11:20)*u);
 for(const auto& slot:rewardSlots(c,l,notice.reward)){
  if(slot.value<=0)continue;
  c.icon(slot.pearl?"hud-icons/pearl-v4.png":"hud-icons/coin-v4.png",slot.icon);
  const auto number=amount(slot.value);
  if(l.sideReceipt)leftText(c,number,slot.number,slot.numberSize);
  else text(c,number,slot.number,slot.numberSize);
 }
 if(l.sideReceipt)c.fill(l.divider,{107,192,192,255});
 leftText(c,notice.items.empty()?"Keep growing your aquarium!":"New in the Shop",l.unlocks,(l.compact?13:l.portrait?22:26)*u);
 if(!notice.items.empty()){
  const float limit=levelUpScrollLimit(l,notice.items.size()),offset=std::clamp(scroll_,0.f,limit);
  c.clip(l.grid);
  for(std::size_t i=0;i<notice.items.size();++i){
   const auto card=levelUpCardBounds(l,i,offset);
   if(card.x+card.w<l.grid.x||card.x>l.grid.x+l.grid.w)continue;
   const auto& item=notice.items[i];
   const float radius=std::min(12*s,card.h*.15f);
   c.roundedImage("dialogs/level-up-mobile-card-v1.png",card,radius);
   c.outline(card,{65,189,204,255},radius,std::max(1.f,s));
   const float nameH=40*s;
   Rect art{card.x+14*s,card.y+12*s,card.w-28*s,card.h-nameH-20*s};
   Rect name{card.x+8*s,card.y+card.h-nameH-2*s,card.w-16*s,nameH};
   float font=18*u;
   if(l.compact){
    art={card.x+4*s,card.y+2*s,card.w*.44f-8*s,card.h-4*s};
    name={card.x+card.w*.44f,card.y+2*s,card.w*.56f-6*s,card.h-4*s};
    font=std::min(13*u,name.h*.4f);
   }
   c.icon(item.asset,art);itemName(c,item.name,name,font,!l.compact);
  }
  c.clearClip();
  if(limit>0&&!l.compact){
   const int visible=std::max(1,int((l.grid.w+l.gap)/(l.cardWidth+l.gap)));
   const int pages=std::min(5,int((notice.items.size()+std::size_t(visible)-1)/std::size_t(visible)));
   const int active=int(std::round(offset/limit*float(pages-1)));
   const float dot=7*s,pill=28*s,gap=12*s,total=pill+float(pages-1)*(dot+gap);
   float x=l.progress.x+(l.progress.w-total)*.5f;
   for(int i=0;i<pages;++i){
    const float w=i==active?pill:dot;
    c.round({x,l.progress.y+(l.progress.h-dot)*.5f,w,dot},i==active?Color{44,186,207,255}:Color{221,247,244,255},dot*.5f,{76,201,218,255},s,false,false);
    x+=w+gap;
   }
  }
 }
 const auto r=l.continueButton;
 shopTheme::panel(c,r,l.dialog.unit,shopTheme::Surface::Buy,pressed_==0&&!dragged_);
 if(r.has(pointer.x,pointer.y))c.outline(r,shopTheme::white,12*l.dialog.unit,2*l.dialog.unit);
 text(c,"Continue",{r.x+6*u,r.y,r.w-12*u,r.h},(l.compact?22:25)*u,shopTheme::white,true);
}

void HudLevelUp::paintRewards(Canvas& c,const LevelUpLayout& l,const HudLayout& hud)const{
 if(!open()||!current()||age_<rewardDelay||age_>=2.2)return;
 const float u=std::max(hud.unit,c.minimumTouchSize()/88.f);
 for(const auto& slot:rewardSlots(c,l,current()->reward)){
  if(slot.value<=0)continue;
  const auto destination=hud[slot.pearl?HudPart::PearlIcon:HudPart::CoinIcon];
  const auto from=center(slot.icon),to=center(destination);
  const Color tint=slot.pearl?Color{229,211,255,255}:Color{255,233,151,255};
  const float pulse=rewardPulse(slot.value,age_,reducedMotion_);
  if(pulse>0){
   const auto bar=hud[slot.pearl?HudPart::Pearls:HudPart::Coins];
   c.outline({bar.x-2*u,bar.y-2*u,bar.w+4*u,bar.h+4*u},{tint.r,tint.g,tint.b,Uint8(210*pulse)},14*u,3*u);
   if(!reducedMotion_){
    for(float side:{-1.f,1.f})sparkle(c,{to.x+side*(destination.w*.45f+8*u*(1-pulse)),to.y-side*destination.h*.35f},6*u*pulse,{255,255,242,Uint8(230*pulse)});
   }
  }
  if(reducedMotion_)continue;
  const int count=tokenCount(slot.value);
  for(int i=0;i<count;++i){
   const double local=age_-departure(i);if(local<=0||age_>=arrival(i))continue;
   const auto at=tokenPosition(from,to,i,count,age_,u);
   const float progress=float(local/flightTime),pop=float(smooth(local/.09));
   const float absorbed=float(smooth((progress-.82)/.18)),alpha=pop*(1-.65f*absorbed);
   const float size=std::min(slot.icon.w*.72f,48*u)*(.6f+.4f*pop)*(1-.7f*absorbed);
   for(int trail=2;trail>0;--trail){
    const auto p=tokenPosition(from,to,i,count,std::max(departure(i),age_-trail*.035),u);
    const float r=(4-trail)*2*u;
    c.round({p.x-r,p.y-r,2*r,2*r},{tint.r,tint.g,tint.b,Uint8((58-trail*17)*alpha)},r,{},0,false,false);
   }
   c.round({at.x-size*.6f,at.y-size*.6f,size*1.2f,size*1.2f},{tint.r,tint.g,tint.b,Uint8(30*alpha)},size*.6f,{},0,false,false);
   c.image(slot.pearl?"hud-icons/pearl-v4.png":"hud-icons/coin-v4.png",{at.x-size*.5f,at.y-size*.5f,size,size},std::sin(float(local)*8+i)*10,{.5f,.5f},alpha);
   sparkle(c,{at.x-size*.22f,at.y-size*.24f},size*.10f,{255,255,242,Uint8(220*alpha)});
  }
 }
}
}
