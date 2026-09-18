#include "aquarium/hud_care.hpp"
#include "aquarium/fish_details.hpp"
#include "aquarium/shop_theme.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace aq {
bool normalizeHudPointer(HudPointer& state,SDL_Event& e,float width,float height){
 const bool mouseButton=e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP;
 if(mouseButton||e.type==SDL_EVENT_MOUSE_MOTION){
  const auto which=mouseButton?e.button.which:e.motion.which;
  if(which==SDL_TOUCH_MOUSEID||state.finger)return false;
  state.touch=false;return true;
 }
 if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND||e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET){state.finger.reset();return true;}
 const auto type=e.type;
 if(type!=SDL_EVENT_FINGER_DOWN&&type!=SDL_EVENT_FINGER_UP&&type!=SDL_EVENT_FINGER_MOTION&&type!=SDL_EVENT_FINGER_CANCELED)return true;
 if(type==SDL_EVENT_FINGER_DOWN){if(state.finger)return false;state.finger=e.tfinger.fingerID;}
 if(state.finger!=e.tfinger.fingerID)return false;
 const float x=e.tfinger.x*width,y=e.tfinger.y*height;
 const auto window=e.tfinger.windowID;state.touch=true;
 e={};
 if(type==SDL_EVENT_FINGER_CANCELED){state.finger.reset();e.type=SDL_EVENT_WINDOW_FOCUS_LOST;return true;}
 if(type==SDL_EVENT_FINGER_MOTION){e.type=SDL_EVENT_MOUSE_MOTION;e.motion.windowID=window;e.motion.which=SDL_TOUCH_MOUSEID;e.motion.x=x;e.motion.y=y;}
 else{e.type=type==SDL_EVENT_FINGER_DOWN?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;e.button.windowID=window;e.button.which=SDL_TOUCH_MOUSEID;e.button.button=SDL_BUTTON_LEFT;e.button.down=type==SDL_EVENT_FINGER_DOWN;e.button.x=x;e.button.y=y;}
 if(type==SDL_EVENT_FINGER_UP)state.finger.reset();
 return true;
}

FishCareLayout layoutFishCare(float width,float height,Insets safe,Rect fish,float minimumTouch,bool extended){
 const auto hud=layoutHud(width,height,safe,minimumTouch);
 const float margin=12*hud.unit;
 const Rect available{safe.left+margin,safe.top+margin,width-safe.left-safe.right-2*margin,height-safe.top-safe.bottom-2*margin};
 // Match the approved 470 by 314 pixel frame at its 1642 by 958 viewport.
 // Phone controls gain touch space without changing the two-column layout.
 float u=std::max(hud.unit,minimumTouch/80.f);
 u=std::min({u,available.w/460.f,available.h/(extended?430.f:390.f)});
 const float header=std::max(52*u,std::min(minimumTouch,available.h*.15f));
 const float button=std::max(56*u,std::min(minimumTouch,available.h*.15f));
 const float w=460*u,h=header+(extended?252:200)*u+button,tail=36*u,gap=8*u;
 const float cx=fish.x+fish.w*.5f,cy=fish.y+fish.h*.5f;
 const float tipX=cx-fish.w*.2f;
 const std::array<Rect,4> candidates{{
  {tipX-w*.56f,fish.y-gap-tail-h,w,h},
  {tipX-w*.56f,fish.y+fish.h+gap+tail,w,h},
  {fish.x-gap-tail-w,cy-h*.5f,w,h},
  {fish.x+fish.w+gap+tail,cy-h*.5f,w,h}
 }};
 auto overlap=[](Rect a,Rect b){return std::max(0.f,std::min(a.x+a.w,b.x+b.w)-std::max(a.x,b.x))*std::max(0.f,std::min(a.y+a.h,b.y+b.h)-std::max(a.y,b.y));};
 Rect frame{};int side=0;float best=std::numeric_limits<float>::max();
 for(int i=0;i<4;++i){
  auto r=candidates[i];
  r.x=std::clamp(r.x,available.x,available.x+std::max(0.f,available.w-w));
  r.y=std::clamp(r.y,available.y,available.y+std::max(0.f,available.h-h));
  float score=overlap(r,fish)*1000+std::hypot(r.x-candidates[i].x,r.y-candidates[i].y)+float(i)*u;
  for(const auto part:hudControls())score+=overlap(r,hud[part])*10;
  for(const auto part:{HudPart::Profile,HudPart::Coins,HudPart::CoinIcon,HudPart::Pearls,HudPart::PearlIcon})score+=overlap(r,hud[part])*10;
  if(score<best){best=score;frame=r;side=i;}
 }
 auto box=[&](float x,float y,float bw,float bh){return Rect{frame.x+x*u,frame.y+y*u,bw*u,bh*u};};
 const float top=header/u,actionY=top+(extended?242:190);
 const float control=std::max(44*u,std::min(minimumTouch,header));
 const Rect close{frame.x+w-control-8*u,frame.y+(header-control)*.5f,control,control};
 const Rect favorite{close.x-control-4*u,close.y,control,control};
 FishCareLayout out;
 out.dialog={{0,0,width,height},frame,box(4,4,452,top-4),
  {frame.x+16*u,frame.y, favorite.x-frame.x-24*u,header},close,
  box(4,top,452,h/u-top-4),box(16,top+8,428,h/u-top-16),u};
 out.favorite=favorite;
 out.status=box(20,top+10,420,28);out.progress=box(24,top+44,412,16);
 out.stages=box(20,top+70,420,22);out.rewards=box(24,top+104,412,56);
 out.breakdown=box(20,top+168,420,extended?28:22);out.note=box(20,top+204,420,28);
 out.primary=box(16,actionY,208,button/u);out.secondary=box(236,actionY,208,button/u);
 if(side<2){
  const float rootX=std::clamp(tipX,frame.x+60*u,frame.x+w-28*u);
  const float rootY=side==0?frame.y+h-6*u:frame.y+6*u;
  const float pointY=side==0?fish.y-gap:fish.y+fish.h+gap;
  out.tail={SDL_FPoint{rootX-36*u,rootY},SDL_FPoint{rootX-4*u,rootY},SDL_FPoint{tipX,pointY}};
  out.tailAtTop=side==1;
 }else{
  const float rootY=std::clamp(cy,frame.y+header+24*u,frame.y+h-28*u);
  const float rootX=side==2?frame.x+w-6*u:frame.x+6*u;
  const float pointX=side==2?fish.x-gap:fish.x+fish.w+gap;
  out.tail={SDL_FPoint{rootX,rootY-16*u},SDL_FPoint{rootX,rootY+16*u},SDL_FPoint{pointX,cy}};
 }
 return out;
}

FishCareLayout HudCare::detailsLayout()const{
 const auto f=visual(selected_);
 SDL_FPoint point{canvas_.width()*.5f,canvas_.height()*.5f},size{};bool extended=false;
 if(f){
  point=canvas_.toScreen(fishPose(*f,session_.interpolation()).position);
  const float egg=std::max(20.f,30*canvas_.worldScale());
  size=f->egg?SDL_FPoint{egg,egg}:canvas_.fishSize(*session_.domain().content().find(f->species),*f);
  extended=!session_.domain().companion(f->id)&&(f->egg||f->age<4);
 }
 return layoutFishCare(canvas_.width(),canvas_.height(),canvas_.safeInsets(),{point.x-size.x*.5f,point.y-size.y*.5f,size.x,size.y},canvas_.minimumTouchSize(),extended||!notice_.empty());
}

std::optional<Fish> HudCare::visual(FishId id)const{
 const auto& d=session_.domain();
 if(const auto* f=d.fish(id);f&&!f->stashed&&f->tank==d.state().activeTank)return *f;
 if(const auto* c=d.companion(id);c&&!c->stored&&c->tank==d.state().activeTank)return companionVisual(*c);
 return {};
}
FishId HudCare::hitFish(SDL_FPoint point)const{
 const auto& d=session_.domain();FishId found{};double best=std::numeric_limits<double>::max();
 auto hit=[&](const Fish& f){
  if(f.stashed||f.tank!=d.state().activeTank)return;
  const auto p=canvas_.toScreen(fishPose(f,session_.interpolation()).position);
  const float egg=std::max(20.f,30*canvas_.worldScale());
  const auto size=f.egg?SDL_FPoint{egg,egg}:canvas_.fishSize(*d.content().find(f.species),f);
  const float minimum=canvas_.minimumTouchSize()*.5f,pad=tool_==Tool::Sell?16*canvas_.worldScale():0;
  const double rx=std::max(minimum,size.x*.5f+pad),ry=std::max(minimum,size.y*.5f+pad);
  const double dx=point.x-p.x,dy=point.y-p.y,distance=dx*dx+dy*dy;
  if(dx*dx/(rx*rx)+dy*dy/(ry*ry)<=1&&distance<best){found=f.id;best=distance;}
 };
 for(const auto& f:d.state().fish)hit(f);
 for(const auto& c:d.state().companions)hit(companionVisual(c));
 return found;
}
FishReward HudCare::reward(const Fish& fish)const{
 auto result=fishReward(fish);
 result.xp=std::min(result.xp,session_.domain().content().levels.back()-session_.domain().state().xp);
 return result;
}
Rect HudCare::doneBounds()const{
 const float u=layoutHud(canvas_.width(),canvas_.height(),canvas_.safeInsets(),canvas_.minimumTouchSize()).unit;
 return {(canvas_.width()-120*u)*.5f,canvas_.height()-canvas_.safeInsets().bottom-96*u,120*u,64*u};
}
bool HudCare::waterAt(SDL_FPoint point)const{
 const auto safe=canvas_.safeInsets();
 const Rect water{safe.left,safe.top,canvas_.width()-safe.left-safe.right,canvas_.height()-safe.top-safe.bottom};
 const auto hud=layoutHud(canvas_.width(),canvas_.height(),safe,canvas_.minimumTouchSize());
 if(!water.has(point.x,point.y)||hudHit(hud,point))return false;
 if(point.y<hud[HudPart::Profile].y+hud[HudPart::Profile].h+12*hud.unit)return false;
 if(tool_!=Tool::Select&&doneBounds().has(point.x,point.y))return false;
 return inPlacementWater(canvas_.toWorld(point.x,point.y));
}
void HudCare::cancelPress(){downActive_=dragged_=false;pressed_=-1;pressedFish_={};details_.closePressed=details_.backdropPressed=false;}
void HudCare::closeDetails(){selected_={};details_={false};cancelPress();}
void HudCare::reset(){tool_=Tool::Select;closeDetails();pointerSeen_=false;canvas_.cursor(CursorKind::Arrow);}
void HudCare::setTool(Tool tool){closeDetails();tool_=tool;notice_.clear();}
void HudCare::message(std::string text){notice_=std::move(text);noticeAge_=0;}
bool HudCare::command(const Command& c){
 const auto result=session_.command(c);
 if(!result){message(result.message.empty()?errorText(result.error):result.message);return false;}
 notice_.clear();return true;
}
int HudCare::control(SDL_FPoint point)const{
 if(details_.open){
  const auto l=detailsLayout();
  if(l.favorite.has(point.x,point.y))return 3;
  if(l.primary.has(point.x,point.y))return 4;
  if(l.secondary.has(point.x,point.y))return 5;
  return -1;
 }
 const auto hud=layoutHud(canvas_.width(),canvas_.height(),canvas_.safeInsets(),canvas_.minimumTouchSize());
 const auto part=hudHit(hud,point);
 if(part==HudPart::Food)return 0;
 if(part==HudPart::Rehome)return 1;
 if(tool_!=Tool::Select&&doneBounds().has(point.x,point.y))return 2;
 return -1;
}
void HudCare::activate(int target){
 if(target==0){setTool(tool_==Tool::Food?Tool::Select:Tool::Food);return;}
 if(target==1){setTool(tool_==Tool::Sell?Tool::Select:Tool::Sell);return;}
 if(target==2){setTool(Tool::Select);return;}
 const auto f=visual(selected_);if(!f){closeDetails();return;}
 const auto& d=session_.domain();const bool display=d.companion(f->id)!=nullptr;
 if(target==3){command({.action=Action::Favorite,.fish=f->id});return;}
 if(target==4){
  if(display){setTool(Tool::Food);return;}
  if(!f->egg&&f->age==4){if(command({.action=Action::Keep,.fish=f->id})){closeDetails();tool_=Tool::Select;}}
  return;
 }
 if(target!=5)return;
 if(display){closeDetails();return;}
 if(f->egg||f->age<1)return;
 if(f->favorite){message("Unfavorite this fish before selling it.");return;}
 if(command({.action=Action::Sell,.fish=f->id})){closeDetails();tool_=Tool::Select;}
}
bool HudCare::event(const SDL_Event& e,bool touch,bool blocked){
 if(blocked){reset();return false;}
 if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND||e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET||e.type==SDL_EVENT_WINDOW_RESIZED){cancelPress();pointerSeen_=false;canvas_.cursor(CursorKind::Arrow);return false;}
 if(details_.open&&!visual(selected_)){closeDetails();return true;}
 if(e.type==SDL_EVENT_KEY_DOWN&&!details_.open){
  if(e.key.key==SDLK_F||e.key.key==SDLK_S){if(!e.key.repeat)activate(e.key.key==SDLK_F?0:1);return true;}
  if(e.key.key==SDLK_ESCAPE&&tool_!=Tool::Select){setTool(Tool::Select);return true;}
 }
 const bool button=e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP;
 const bool pointer=button||e.type==SDL_EVENT_MOUSE_MOTION;
 if(pointer){
  pointer_=button?canvas_.inputPoint(e.button.x,e.button.y):canvas_.inputPoint(e.motion.x,e.motion.y);pointerSeen_=true;touch_=touch;
  if(downActive_&&std::hypot(pointer_.x-down_.x,pointer_.y-down_.y)>canvas_.minimumTouchSize()*8/44){dragged_=true;details_.closePressed=details_.backdropPressed=false;}
 }
 if(details_.open){
  const auto layout=detailsLayout();
  if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT){cancelPress();downActive_=true;down_=pointer_;pressed_=control(pointer_);}
  if(e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.button==SDL_BUTTON_LEFT){
   const int action=downActive_&&!dragged_&&pressed_==control(pointer_)?pressed_:-1;
   if(action>=0){cancelPress();activate(action);return true;}
   if(downActive_&&!dragged_&&details_.backdropPressed&&!layout.dialog.frame.has(pointer_.x,pointer_.y)){
    const auto other=hitFish(pointer_);
    if(other.value&&other!=selected_){selected_=other;details_={true};notice_.clear();cancelPress();return true;}
   }
  }
  const bool consumed=dialogEvent(details_,layout.dialog,e,pointer_);
  if(e.type==SDL_EVENT_MOUSE_BUTTON_UP)cancelPress();
  if(!details_.open)closeDetails();
  return consumed;
 }
 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT){
  cancelPress();down_=pointer_;pressed_=control(pointer_);
  if(pressed_>=0){downActive_=true;return true;}
  const auto hud=layoutHud(canvas_.width(),canvas_.height(),canvas_.safeInsets(),canvas_.minimumTouchSize());
  if(hudHit(hud,pointer_)){setTool(Tool::Select);return false;}
  if(waterAt(pointer_)){downActive_=true;if(tool_!=Tool::Food)pressedFish_=hitFish(pointer_);return true;}
 }
 if(e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.button==SDL_BUTTON_LEFT){
  if(!downActive_)return false;
  const int pressed=pressed_;const auto fish=pressedFish_;const bool tap=!dragged_;cancelPress();
  if(tap&&pressed>=0&&pressed==control(pointer_))activate(pressed);
  else if(tap&&pressed<0&&waterAt(pointer_)){
   if(tool_==Tool::Food){if(command({.action=Action::DropFood,.point=canvas_.toWorld(pointer_.x,pointer_.y)}))tapAge_=0;}
   else if(fish.value&&visual(fish)&&hitFish(pointer_)==fish){
    if(tool_==Tool::Sell){
     const auto f=visual(fish);
     if(session_.domain().companion(fish))message("Display fish stay in your aquarium.");
     else if(!f->egg&&f->age>=1){
      if(f->favorite)message("Unfavorite this fish before selling it.");
      else command({.action=Action::Sell,.fish=fish});
     }
    }else{selected_=fish;details_={true};notice_.clear();}
   }
  }
  return true;
 }
 return e.type==SDL_EVENT_MOUSE_MOTION&&downActive_;
}
void HudCare::advance(double seconds,bool rewardsVisible){
 tapAge_+=seconds;noticeAge_+=seconds;
 const bool reduced=session_.domain().state().settings.reducedMotion;
 rewards_.advance(seconds,reduced);
 if(noticeAge_>6&&!details_.open)notice_.clear();
 if(selected_.value&&!visual(selected_))closeDetails();
 for(auto& item:feedback_)item.age+=seconds;
 std::erase_if(feedback_,[](const auto& item){return item.event.kind=="feed"&&item.age>=2;});
 for(auto& e:session_.domain().takeEvents()){
  if(rewardsVisible)rewards_.collect(e,reduced);
  if(e.kind=="feed"||e.kind=="level")feedback_.push_back({std::move(e),0});
 }
 // Shop replaces the destinations. Show the saved totals there immediately.
 if(!rewardsVisible)rewards_.finish();
 const int shownLevel=levelFor(session_.domain().content(),Amount(rewards_.display(session_.domain()).xp));
 std::erase_if(feedback_,[&](const auto& item){
  const auto& e=item.event;
  if(e.kind!="level"||e.reachedLevel>shownLevel)return false;
  message(e.text+(e.pearls?"  +"+compact(e.pearls)+" Pearls":""));return true;
 });
 if(feedback_.size()>24)feedback_.erase(feedback_.begin(),feedback_.end()-24);
}
void HudCare::paintDetails(){
 const auto f=visual(selected_);if(!f)return;
 const auto& d=session_.domain();const auto& species=*d.content().find(f->species);
 const bool display=d.companion(f->id)!=nullptr,adult=!display&&!f->egg&&f->age==4;
 const auto l=detailsLayout();const auto frame=l.dialog.frame;const float u=l.dialog.unit;
 const auto content=fishDetailsContent(species,*f,d.state().simNow);
 canvas_.triangle(l.tail,shopTheme::dialogEdge);
 paintDialog(canvas_,l.dialog,{species.name},l.dialog.close.has(pointer_.x,pointer_.y),details_.closePressed,DialogPresentation::Popover);
 auto inner=l.tail;
 const SDL_FPoint middle{(inner[0].x+inner[1].x)*.5f,(inner[0].y+inner[1].y)*.5f};
 for(int i=0;i<2;++i){inner[i].x+=(middle.x-inner[i].x)*.22f;inner[i].y+=(middle.y-inner[i].y)*.22f;}
 const float distance=std::max(1.f,std::hypot(inner[2].x-middle.x,inner[2].y-middle.y));
 inner[2].x+=(middle.x-inner[2].x)*std::min(1.f,5*u/distance);inner[2].y+=(middle.y-inner[2].y)*std::min(1.f,5*u/distance);
 canvas_.triangle(inner,l.tailAtTop?Color{22,78,121,255}:shopTheme::dialogPaper);
 auto text=[&](std::string_view value,Rect r,float size,Color color=shopTheme::ink,bool center=false){
  canvas_.text(value,r.x+(center?r.w*.5f:0),r.y,size*u,color,center,r.w,true,true);
 };
 auto badge=[&](std::string_view label,Rect bounds,shopTheme::Surface style,Color ink,bool clock=false){
  const float font=18*u,art=clock?20*u:0,gap=art?8*u:0;
  const float textWidth=canvas_.textWidth(label,font,true,false,false,false,true);
  const float width=std::min(bounds.w,textWidth+art+gap+28*u);
  const Rect r{bounds.x+(bounds.w-width)*.5f,bounds.y,width,bounds.h};
  shopTheme::panel(canvas_,r,u*.65f,style);
  if(clock)shopTheme::clockIcon(canvas_,{r.x+14*u,r.y+(r.h-art)*.5f,art,art},ink);
  canvas_.text(label,r.x+14*u+art+gap,r.y+(r.h-font)*.5f,font,ink,false,r.w-28*u-art-gap,true,true);
 };
 auto line=[&](Rect r,float y){r.y+=y*u;return r;};
 auto icon=[&](std::string_view path,Rect r,Color color=shopTheme::white){canvas_.icon(path,r,1,false,color);};
 // Hearts use the current body font; the popover adds no icon or font assets.
 auto heart=[&](Rect r,Color color=shopTheme::white){
  canvas_.text("♥",r.x+r.w*.5f,r.y-r.h*.15f,r.h,color,true,r.w);
 };
 const float heartSize=38*u;
 heart({l.favorite.x+(l.favorite.w-heartSize)*.5f,l.favorite.y+(l.favorite.h-heartSize)*.5f,heartSize,heartSize},f->favorite?Color{255,207,74,255}:shopTheme::white);
 const std::string status=display?"Display fish":adult?content.care:f->egg?"Egg":stageName(*f)+(content.condition==Care::Fed?" · Growing":" · Hungry");
 const float percentWidth=116*u;
 auto statusRect=l.status;statusRect.w-=percentWidth;text(status,statusRect,24);
 // Use saved milliseconds so the display never rounds up to 100% early.
 const auto duration=f->purchase.durationMs;
 const auto hundredths=duration>0?std::clamp(f->growthMs,Millis{0},duration)*10000/duration:10000;
 const auto fraction=hundredths%100;
 const std::string percent=std::to_string(hundredths/100)+"."+(fraction<10?"0":"")+std::to_string(fraction)+"%";
 text(display?"Kept":percent,{l.status.x+l.status.w-percentWidth,l.status.y,percentWidth,l.status.h},24,shopTheme::ink,true);
 shopTheme::panel(canvas_,l.progress,u*.6f,shopTheme::Surface::Well);
 // Equal stage spacing follows the mock. The fill advances within each saved
 // stage interval; the percentage above remains the total timed growth.
 const float progress=display?1:f->egg?0:std::clamp((float(f->age)+float(nextStageProgress(*f)))/4.f,0.f,1.f);
 if(progress>0)canvas_.gradient({l.progress.x+3*u,l.progress.y+3*u,(l.progress.w-6*u)*progress,l.progress.h-6*u},{111,222,142},{42,167,112},5*u);
 constexpr std::array<std::string_view,5> names{"Baby","Junior","Young","Mature","Adult"};
 for(int i=0;i<5;++i){
  const float x=l.progress.x+8*u+(l.progress.w-16*u)*float(i)*.25f;
  const bool reached=display||(!f->egg&&i<=f->age);
  canvas_.gradient({x-7*u,l.progress.y+u,14*u,14*u},reached?Color{255,227,135,255}:Color{197,216,206,255},reached?Color{250,201,74,255}:Color{147,177,176,255},7*u);
  canvas_.outline({x-7*u,l.progress.y+u,14*u,14*u},shopTheme::fishInk,7*u,u);
  const float labelWidth=std::min(88*u,canvas_.textWidth(names[i],18*u,true,false,false,false,true));
  const float left=std::clamp(x-labelWidth*.5f,frame.x+16*u,frame.x+frame.w-16*u-labelWidth);
  text(names[i],{left,l.stages.y,labelWidth,l.stages.h},18,i==f->age?shopTheme::fishInk:shopTheme::ink,true);
 }
 canvas_.fill({frame.x+36*u,l.rewards.y-12*u,388*u,u},{126,149,134,100});
 if(display){
  text(d.companion(f->id)->origin.value?"Reward collected":"Permanent companion",line(l.rewards,4),26,shopTheme::ink,true);
  text("No more coin or XP rewards",line(l.rewards,38),18,shopTheme::ink,true);
  text("This fish stays in your aquarium.",l.breakdown,18,shopTheme::ink,true);
 }else{
  auto grown=*f;grown.egg=false;grown.age=4;const auto atAdult=reward(grown);
  const bool locked=f->egg||f->age<1;
  const auto payout=locked?atAdult:reward(*f);
  const float y=l.rewards.y;
  canvas_.icon("hud-icons/coin-v4.png",{frame.x+32*u,y-4*u,60*u,60*u});
  text(compact(payout.coins()),{frame.x+104*u,y,120*u,36*u},32);
  text(locked?"Adult coins":"Coins",{frame.x+104*u,y+36*u,120*u,24*u},18);
  canvas_.fill({frame.x+230*u,y+4*u,u,48*u},{96,140,148,180});
  paintXpBadge(canvas_,{frame.x+268*u,y-2*u,56*u,52*u},u,1,"XP",24*u);
  text("+"+compact(payout.xp),{frame.x+336*u,y,108*u,36*u},32);
  text(locked?"Adult XP":"XP",{frame.x+336*u,y+36*u,108*u,24*u},18);
  if(locked)badge("Unlocks at Junior",l.breakdown,shopTheme::Surface::PearlBuy,{64,48,92,255});
  else if(f->favorite)text("Unfavorite to sell",l.breakdown,18,shopTheme::ink,true);
  else if(!adult)text("Adult: "+compact(atAdult.coins())+" coins + "+compact(atAdult.xp)+" XP",l.breakdown,18,shopTheme::ink,true);
 }
 const bool growthBadge=notice_.empty()&&!adult&&!display;
 const bool paused=!f->egg&&content.condition!=Care::Fed;
 const std::string note=!notice_.empty()?notice_:adult||display?"":paused?"Growth paused":content.growth;
 if(growthBadge){
  badge(note,l.note,paused?shopTheme::Surface::PausedBadge:shopTheme::Surface::Tab,paused?Color{100,65,21,255}:shopTheme::ink,!paused);
 }else if(!note.empty())text(note,l.note,18,notice_.empty()?shopTheme::ink:shopTheme::fishInk,true);
 auto button=[&](Rect r,int id,std::string_view label,shopTheme::Surface style,std::string_view asset,bool enabled=true,bool showHeart=false){
  shopTheme::panel(canvas_,r,u,enabled?style:shopTheme::Surface::DisabledButton,enabled&&pressed_==id&&downActive_&&!dragged_);
  const Color foreground=!enabled?Color{65,87,86,255}:style==shopTheme::Surface::Keep?Color{95,40,36,255}:style==shopTheme::Surface::Buy?Color{16,71,45,255}:shopTheme::white;
  const float font=28*u,art=enabled&&asset.empty()&&!showHeart?0:32*u,gap=art?12*u:0;
  const float width=canvas_.textWidth(label,font,true,false,false,false,true);
  const float scale=std::min(1.f,(r.w-24*u)/(width+art+gap));
  const float x=r.x+(r.w-(width+art+gap)*scale)*.5f;
  const Rect artBounds{x,r.y+(r.h-art*scale)*.5f,art*scale,art*scale};
  if(!enabled)shopTheme::lockIcon(canvas_,artBounds);
  else if(!asset.empty())icon(asset,artBounds,foreground);
  else if(showHeart)heart(artBounds,foreground);
  canvas_.text(label,x+(art+gap)*scale,r.y+(r.h-font*scale)*.5f,font*scale,foreground,false,width*scale,true,true);
 };
 button(l.primary,4,display?"Feed fish":"Keep",display?shopTheme::Surface::Buy:shopTheme::Surface::Keep,"",display||adult,!display);
 button(l.secondary,5,display?"Close":"Sell",display?shopTheme::Surface::Button:shopTheme::Surface::Buy,display?"":"mask:tools/sell-net.png",display||(!f->favorite&&!f->egg&&f->age>=1));
}
void HudCare::paintStageMeter(const Fish& fish,SDL_FPoint position,SDL_FPoint fishSize,bool locked){
 const auto safe=canvas_.safeInsets();
 const float density=canvas_.minimumTouchSize()/44.f;
 const float u=std::max(.9f*density,canvas_.worldScale());
 const float side=10*u,gap=2*u,stroke=.8f*u,height=3.5f*u;
 // Follow the fish's width. Only tiny phone fish need room for a readable digit.
 const float width=std::max(fishSize.x,(locked?36.f:26.f)*u);
 const float x=std::clamp(position.x-width*.5f,safe.left+2,canvas_.width()-safe.right-width-2);
 const float y=std::max(safe.top+2,position.y-fishSize.y*.5f-8*u-side);
 const Rect stage{x,y,side,side};
 const Rect bar{x+side-stroke,y+(side-height)*.5f,width-side+stroke-(locked?side+gap:0),height};
 constexpr Color edge{17,66,83,255};
 canvas_.gradient(bar,{13,104,129,255},{12,88,113,255},height*.5f);
 canvas_.outline(bar,edge,height*.5f,stroke);
 const float progress=float(nextStageProgress(fish));
 if(progress>0)canvas_.gradient({bar.x+stroke,bar.y+stroke,(bar.w-2*stroke)*progress,bar.h-2*stroke},{110,250,242,255},{36,218,225,255},height*.5f-stroke);
 canvas_.gradient(stage,{255,227,135,255},{250,201,74,255},2*u);
 canvas_.outline(stage,edge,2*u,stroke);
 if(fish.egg)canvas_.icon("ui/egg.png",{stage.x+stroke,stage.y+stroke,stage.w-2*stroke,stage.h-2*stroke});
 else{
  const float font=std::max(9.f,7*u);
  canvas_.text(std::to_string(std::clamp(fish.age,0,4)),stage.x+side*.5f,stage.y+(side-font*1.2f)*.5f,font,edge,true,side-2*stroke,true);
 }
 if(locked)shopTheme::lockIcon(canvas_,{x+width-side,y,side,side});
}
void HudCare::paint(){
 const auto hud=layoutHud(canvas_.width(),canvas_.height(),canvas_.safeInsets(),canvas_.minimumTouchSize());const float u=hud.unit;
 for(const auto& item:feedback_){
  if(item.event.kind!="feed")continue;
  const auto p=canvas_.toScreen(item.event.position);const auto& e=item.event;
  std::string value=e.kind=="feed"?"Yum!":"+"+compact(e.coins)+" coins";
  if(e.xp)value+="  +"+compact(e.xp)+" XP";
  if(e.pearls)value+="  +"+compact(e.pearls)+" Pearls";
  const Uint8 alpha=Uint8(255*std::clamp((2-item.age)/.5,0.,1.));
  const float y=p.y-40*u-float(item.age)*36*u,x=std::clamp(p.x,180*u,canvas_.width()-180*u);
  canvas_.text(value,x+u,y+u,28*u,{20,58,57,alpha},true,360*u,true,true);
  canvas_.text(value,x,y,28*u,{241,255,185,alpha},true,360*u,true,true);
 }
 if(tool_!=Tool::Select&&!details_.open){
  const auto active=hud[tool_==Tool::Food?HudPart::Food:HudPart::Rehome];
  canvas_.outline({active.x-4*u,active.y-4*u,active.w+8*u,active.h+8*u},{231,255,167,255},16*u,4*u);
  if(tool_==Tool::Sell){
   const auto& d=session_.domain();
   auto label=[&](const Fish& f,bool companion){
    if(f.stashed||f.tank!=d.state().activeTank)return;
    const auto p=canvas_.toScreen(fishPose(f,session_.interpolation()).position);
    const float egg=std::max(20.f,30*canvas_.worldScale());
    const auto& species=*d.content().find(f.species);
    const auto size=f.egg?SDL_FPoint{egg,egg}:canvas_.fishSize(species,f);
    const bool locked=companion||!sellable(species,f);
    paintStageMeter(f,p,size,locked);
    if(locked)return;
    // Uniformly halve the accepted ribbon, including its artwork and live text.
    constexpr float rewardScale=.5f;
    const float sourceScale=std::max(.85f*canvas_.minimumTouchSize()/44.f,1.385f*canvas_.worldScale());
    const float scale=sourceScale*rewardScale;
    const float w=104*scale,h=w*375.f/1838.f;
    const auto safe=canvas_.safeInsets();
    const float x=std::clamp(p.x-w*.5f,safe.left+2,canvas_.width()-safe.right-w-2);
    const float y=std::clamp(p.y+size.y*.5f+4*scale,safe.top+2,canvas_.height()-safe.bottom-h-2);
    const auto value=reward(f);
    // Fit the visible artwork, preserving the original generated alpha canvas.
    const float artX=w/1838.f,artY=h/375.f;
    canvas_.image("hud-icons/sell-reward-ribbon-v1.png",{x-50*artX,y-224*artY,1923*artX,818*artY});
    // Lilita One matches the upright, rounded lettering in the approved mockup.
    // Position the glyphs by their visible baseline rather than the old all-caps font.
    // Scale the rendered glyphs too, preserving proportions below the font cache's minimum size.
    auto text=[&](std::string_view label,float tx,float ty,float font,Color color,float width){
     canvas_.text(label,tx,ty,font*sourceScale,color,true,width*scale,true,false,false,false,0,1,false,false,rewardScale);
    };
    text(compact(value.coins()),x+42.5f*scale,y+2.5f*scale,14,{79,40,19,255},27);
    const std::string xp="+"+compact(value.xp)+" XP";
    const float xpX=x+78.5f*scale,xpY=y+3.5f*scale;
    for(const auto offset:std::array<SDL_FPoint,4>{{{-.6f,0},{.6f,0},{0,-.6f},{0,.8f}}})
     text(xp,xpX+offset.x*scale,xpY+offset.y*scale,12,{12,57,77,255},34);
    text(xp,xpX,xpY,12,{255,255,248,255},34);
   };
   for(const auto& f:d.state().fish)label(f,false);
   for(const auto& c:d.state().companions)if(!c.stored)label(companionVisual(c),true);
  }
  const auto done=doneBounds();
  shopTheme::panel(canvas_,done,u,shopTheme::Surface::Buy,pressed_==2&&downActive_);
  canvas_.text("Done",done.x+done.w*.5f,done.y+16*u,28*u,shopTheme::white,true,done.w,true,true);
 }
 if((!notice_.empty()||session_.saveFailed())&&!details_.open){
  Rect note{(canvas_.width()-880*u)*.5f,canvas_.safeInsets().top+(tool_==Tool::Sell?164:100)*u,880*u,52*u};
  canvas_.gradient(note,{18,68,82,240},{18,68,82,240},12*u);
  canvas_.text(session_.saveFailed()?"Progress is not saved. Keep the game open to retry.":notice_,note.x+note.w*.5f,note.y+12*u,26*u,shopTheme::white,true,note.w-24*u,true,true);
 }
 if(details_.open)paintDetails();
 const bool cursor=tool_!=Tool::Select&&!details_.open&&pointerSeen_&&waterAt(pointer_)&&(!touch_||downActive_||tapAge_<.2);
 canvas_.cursor(cursor&&!touch_?CursorKind::Hidden:CursorKind::Arrow);
 if(cursor)canvas_.toolCursor(tool_,pointer_,tapAge_,session_.domain().state().settings.reducedMotion);
}
}
