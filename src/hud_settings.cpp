#include "aquarium/hud_settings.hpp"
#include "aquarium/platform_feedback.hpp"
#include "aquarium/shop_theme.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace aq {
namespace {
constexpr int sliderControl=4,aboutControl=5,closeControl=6,backControl=7,copySupportControl=8,backdropControl=9;
constexpr Color amberInk{100,65,21},mint{48,245,205};
Rect touchTarget(Rect r,float touch){return {r.x-std::max(0.f,touch-r.w)*.5f,r.y-std::max(0.f,touch-r.h)*.5f,std::max(r.w,touch),std::max(r.h,touch)};}
void bubble(Canvas& c,float x,float y,float size){
 c.gradient({x,y,size,size},{206,255,252,65},{71,194,215,15},size*.5f);
 c.outline({x,y,size,size},{215,255,252,95},size*.5f,size*.065f);
 c.gradient({x+size*.2f,y+size*.12f,size*.28f,size*.22f},{255,255,255,230},{245,255,255,25},size*.11f);
}
// Use the same menu font as Shop, fish details and shared dialog controls.
void label(Canvas& c,std::string_view text,float x,float y,float size,Color ink,float width,bool center=false){c.text(text,x,y,size,ink,center,width,true,true);}
float paragraph(Canvas& c,std::string_view text,Rect r,float size,Color ink){
 std::istringstream words{std::string(text)};std::string word,line;float y=r.y;
 while(words>>word){
  const auto next=line.empty()?word:line+" "+word;
  if(!line.empty()&&c.textWidth(next,size,true,false,false,false,true)>r.w){label(c,line,r.x,y,size,ink,r.w);y+=size*1.22f;line=word;}else line=next;
 }
 if(!line.empty()){label(c,line,r.x,y,size,ink,r.w);y+=size*1.22f;}
 return y;
}
}

SettingsLayout layoutSettings(float width,float height,Insets safe,float touch){
 SettingsLayout l{};l.touch=touch;const float points=touch/44;
 const Rect area{safe.left,safe.top,width-safe.left-safe.right,height-safe.top-safe.bottom};
 l.stacked=area.w<600*points&&area.h>area.w;
 const float designW=l.stacked?640.f:1280.f,designH=l.stacked?1260.f:752.f;
 l.unit=l.stacked?std::min({.68f*points,(area.w-32*points)/designW,(area.h-24*points)/designH}):std::min((area.w-32*points)/designW,area.h*.83f/designH);
 const float u=l.unit,w=designW*u,h=designH*u;
 auto& d=l.dialog;d.unit=u;d.backdrop={0,0,width,height};d.frame={area.x+(area.w-w)*.5f,area.y+(area.h-h)*.5f,w,h};
 d.header={d.frame.x+6*u,d.frame.y+6*u,w-12*u,78*u};
 d.title={d.frame.x+84*u,d.frame.y+6*u,w-168*u,78*u};
 d.close={d.frame.x+w-80*u,d.frame.y+14*u,60*u,60*u};
 l.closeHit=touchTarget(d.close,touch);
 // Keep the expanded close target inside the frame and clear of the cards.
 l.closeHit.x=std::clamp(l.closeHit.x,d.frame.x,d.frame.x+w-l.closeHit.w);
 l.closeHit.y=d.frame.y;
 d.body={d.frame.x+6*u,d.frame.y+84*u,w-12*u,h-90*u};
 const float pad=(l.stacked?32.f:44.f)*u;
 d.content={d.frame.x+pad,d.body.y+pad,w-2*pad,h-84*u-2*pad};
 const float gap=24*u,cardW=l.stacked?d.content.w:(d.content.w-gap)*.5f,cardH=(l.stacked?200.f:220.f)*u;
 for(int i=0;i<4;++i){
  const int row=l.stacked?i:i/2,col=l.stacked?0:i%2;
  auto& card=l.cards[i];card={d.content.x+col*(cardW+gap),d.content.y+row*(cardH+gap),cardW,cardH};
  l.toggles[i]={card.x+card.w-208*u,card.y+26*u,180*u,76*u};
  l.toggleHits[i]=touchTarget(l.toggles[i],touch);
 }
 const auto music=l.cards[0];l.slider={music.x+40*u,music.y+cardH-62*u,music.w-188*u,28*u};
 l.sliderHit=touchTarget(l.slider,touch);
 if(l.sliderHit.y<l.toggleHits[0].y+l.toggleHits[0].h){
  l.slider.w=std::min(l.slider.w,l.toggleHits[0].x-l.slider.x-8*points);
  l.sliderHit=touchTarget(l.slider,touch);
 }
 const float footerY=l.stacked?l.cards[3].y+cardH+28*u:l.cards[2].y+cardH+28*u;
 if(l.stacked){
  const float buttonH=std::max(64*u,touch);
  l.about={d.content.x,footerY+(buttonH+16*u)*.5f,d.content.w,buttonH};
  l.back={d.content.x,footerY,d.content.w,buttonH};l.copySupport={d.content.x,footerY+buttonH+16*u,d.content.w,buttonH};
 }else{
  l.about={d.frame.x+w*.5f-215*u,footerY,430*u,92*u};
  l.back={d.frame.x+w*.5f-430*u,footerY,400*u,92*u};l.copySupport={d.frame.x+w*.5f,footerY,430*u,92*u};
 }
 l.aboutHit=touchTarget(l.about,touch);l.backHit=touchTarget(l.back,touch);l.copySupportHit=touchTarget(l.copySupport,touch);
 return l;
}

void HudSettings::open(){open_=true;motion_={};page_=SettingsPage::Preferences;pressed_=focus_=-1;dragging_=false;notice_.clear();}
void HudSettings::close(){open_=false;pressed_=focus_=-1;dragging_=false;}
void HudSettings::feedback(){const auto& s=session_.domain().state().settings;if(s.sound)canvas_.sound(740,.65f);if(s.vibration)playHaptic();}
bool HudSettings::save(Action action,double value){
 const auto result=session_.command({.action=action,.value=value});
 notice_=result?"":"Could not save. Please try again.";
 if(result)feedback();return bool(result);
}
int HudSettings::hit(const SettingsLayout& l,SDL_FPoint p)const{
 const auto has=[&](Rect r){return r.has(p.x,p.y);};
 if(has(l.closeHit))return closeControl;
 if(!has(l.dialog.frame))return backdropControl;
 if(page_!=SettingsPage::Preferences){if(has(l.backHit))return backControl;if(has(l.copySupportHit))return copySupportControl;return -1;}
 for(int i=0;i<4;++i)if(has(l.toggleHits[i]))return i;
 if(has(l.sliderHit))return sliderControl;
 if(has(l.aboutHit))return aboutControl;
 return -1;
}
void HudSettings::setVolume(float x,const SettingsLayout& l){volumePreview_=std::round(std::clamp(double((x-l.slider.x)/l.slider.w),0.,1.)*100)/100;}
void HudSettings::activate(int control){
 const auto s=session_.domain().state().settings;
 switch(control){
 case 0:save(Action::SetMusic,!s.music);break;
 case 1:save(Action::SetSound,!s.sound);break;
 case 2:save(Action::SetReducedMotion,!s.reducedMotion);break;
 case 3:save(Action::SetVibration,!s.vibration);break;
 case aboutControl:page_=SettingsPage::About;notice_.clear();focus_=-1;feedback();break;
 case closeControl:case backdropControl:close();break;
 case backControl:
  page_=SettingsPage::Preferences;focus_=-1;
  notice_.clear();feedback();break;
 case copySupportControl:
  if(page_==SettingsPage::About){
   const std::string info="Fishius "+std::string(AQ_VERSION)+"\nPlatform: "+SDL_GetPlatform()+"\nSave: "+(session_.ephemeral()?"preview":session_.saveFailed()?"save failed":"local")+"\nMusic: "+(s.music?"on":"off")+"\nVolume: "+std::to_string(int(std::lround(s.volume*100)))+"%\nSound effects: "+(s.sound?"on":"off")+"\nReduce motion: "+(s.reducedMotion?"on":"off")+"\nVibration: "+(s.vibration?"on":"off");
   notice_=SDL_SetClipboardText(info.c_str())?"Support info copied.":"Could not copy. Please try again.";feedback();
  }
  break;
 default:break;
 }
}
bool HudSettings::event(const SDL_Event& e){
 if(!open_)return false;
 if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND||e.type==SDL_EVENT_FINGER_CANCELED||e.type==SDL_EVENT_WINDOW_RESIZED){pressed_=-1;dragging_=false;return false;}
 const auto l=layout();
 if(e.type==SDL_EVENT_KEY_DOWN){
  if(e.key.key==SDLK_ESCAPE){pressed_=-1;dragging_=false;if(page_==SettingsPage::Preferences)close();else{page_=SettingsPage::Preferences;notice_.clear();focus_=-1;}}
  else if(e.key.key==SDLK_TAB){
   const std::vector<int> order=page_==SettingsPage::Preferences?std::vector<int>{0,1,2,3,sliderControl,aboutControl,closeControl}:std::vector<int>{backControl,copySupportControl,closeControl};
   const auto at=std::find(order.begin(),order.end(),focus_);const int count=int(order.size());const bool reverse=e.key.mod&SDL_KMOD_SHIFT;
   const int index=at==order.end()?(reverse?count-1:0):(int(at-order.begin())+(reverse?count-1:1))%count;focus_=order[std::size_t(index)];
  }else if(focus_==sliderControl&&(e.key.key==SDLK_LEFT||e.key.key==SDLK_RIGHT||e.key.key==SDLK_HOME||e.key.key==SDLK_END)){
   const double value=e.key.key==SDLK_HOME?0:e.key.key==SDLK_END?1:volume()+(e.key.key==SDLK_RIGHT?.05:-.05);
   save(Action::SetVolume,std::clamp(std::round(value*100)/100,0.,1.));
  }else if(!e.key.repeat&&(e.key.key==SDLK_RETURN||e.key.key==SDLK_SPACE))activate(focus_);
  return true;
 }
 if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP){
  if(e.button.button!=SDL_BUTTON_LEFT)return true;
  const auto screenPoint=canvas_.inputPoint(e.button.x,e.button.y);const auto point=motion_.inputPoint(screenPoint);const int control=hit(l,point);
  if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){pressed_=control;down_=screenPoint;focus_=-1;dragging_=control==sliderControl;if(dragging_)setVolume(point.x,l);}
  else{
   const int pressed=pressed_;pressed_=-1;
   if(dragging_){setVolume(point.x,l);const double value=volumePreview_;dragging_=false;if(value!=volume())save(Action::SetVolume,value);}
   else if(pressed>=0&&pressed==control)activate(control);
  }
  return true;
 }
 if(e.type==SDL_EVENT_MOUSE_MOTION){
  const auto screenPoint=canvas_.inputPoint(e.motion.x,e.motion.y);const auto point=motion_.inputPoint(screenPoint);
  if(dragging_)setVolume(point.x,l);
  else if(std::hypot(screenPoint.x-down_.x,screenPoint.y-down_.y)>l.touch*.18f)pressed_=-1;
  return true;
 }
 return e.type==SDL_EVENT_MOUSE_WHEEL||e.type==SDL_EVENT_KEY_UP||e.type==SDL_EVENT_TEXT_INPUT||e.type==SDL_EVENT_FINGER_DOWN||e.type==SDL_EVENT_FINGER_UP||e.type==SDL_EVENT_FINGER_MOTION;
}

void HudSettings::paint(){
 if(!open_)return;
 const auto l=layout();const float u=l.unit;const auto& d=l.dialog;const auto& s=session_.domain().state().settings;
 const std::string title=page_==SettingsPage::About?"About & support":"Settings";
 const DialogPaint animation(canvas_,motion_,d.frame,s.reducedMotion);
 paintDialog(canvas_,d,{"",DialogSize::Large},false,pressed_==closeControl,DialogPresentation::ModalContent);
 canvas_.gradient(d.body,{245,242,232},{239,237,227},12*u);
 canvas_.text(title,d.title.x+d.title.w*.5f,d.title.y+(d.title.h-36*u)*.5f,36*u,shopTheme::white,true,d.title.w,true,true);
 auto button=[&](Rect r,std::string_view text,int control,bool enabled=true){
  shopTheme::panel(canvas_,r,u,enabled?shopTheme::Surface::Amber:shopTheme::Surface::Button,pressed_==control);
  label(canvas_,text,r.x+r.w*.5f,r.y+(r.h-40*u)*.5f,40*u,enabled?amberInk:shopTheme::white,r.w-20*u,true);
  if(focus_==control)canvas_.outline({r.x-4*u,r.y-4*u,r.w+8*u,r.h+8*u},shopTheme::white,14*u,3*u);
 };
 if(page_==SettingsPage::Preferences){
  const std::array<std::string_view,4> names{"Music","Sound effects","Reduce motion","Vibration"};
  const std::array<std::string_view,4> details{"Volume","Taps and rewards","Softer animations","Touch feedback"};
  const std::array<bool,4> enabled{s.music,s.sound,s.reducedMotion,s.vibration};
  for(int i=0;i<4;++i){
   const auto card=l.cards[i];shopTheme::panel(canvas_,card,u*1.08f,shopTheme::Surface::SettingsCard);
   label(canvas_,names[i],card.x+40*u,card.y+32*u,45*u,shopTheme::white,card.w-268*u);
   label(canvas_,details[i],card.x+40*u,card.y+104*u,34*u,shopTheme::ink,card.w-80*u);
   button(l.toggles[i],enabled[i]?"ON":"OFF",i,enabled[i]);
   if(i){bubble(canvas_,card.x+card.w-82*u,card.y+card.h-56*u,28*u);bubble(canvas_,card.x+card.w-56*u,card.y+card.h-84*u,18*u);}
  }
  const auto track=l.slider;const float value=float(volume());
  canvas_.round(track,{10,107,137},14*u,{12,73,93},3*u,false,false);
  if(value>0)canvas_.gradient({track.x+3*u,track.y+3*u,(track.w-6*u)*value,track.h-6*u},{100,255,215},mint,10*u);
  const Rect thumb{track.x+track.w*value-21*u,track.y-14*u,42*u,56*u};
  canvas_.round(thumb,{220,255,229},12*u,{19,74,90},3*u,true,true);
  label(canvas_,std::to_string(int(std::lround(value*100)))+"%",l.cards[0].x+l.cards[0].w-74*u,track.y-5*u,38*u,shopTheme::white,100*u,true);
  if(focus_==sliderControl)canvas_.outline(l.sliderHit,shopTheme::white,12*u,3*u);
  button(l.about,"About & support",aboutControl);
 }else{
  const Rect content{d.content.x,d.content.y,d.content.w,l.back.y-d.content.y-28*u};
  shopTheme::panel(canvas_,content,u*1.08f,shopTheme::Surface::SettingsCard);
  const float x=content.x+40*u,width=content.w-80*u;
  label(canvas_,"Fishius",x,content.y+28*u,46*u,shopTheme::white,width);
  label(canvas_,"Version " AQ_VERSION,x,content.y+84*u,28*u,shopTheme::ink,width);
  float y=content.y+148*u;
  y=paragraph(canvas_,session_.ephemeral()?"Preview mode. Changes are not saved.":"Your aquarium and settings are saved on this device.",{x,y,width,0},32*u,shopTheme::ink)+24*u;
  y=paragraph(canvas_,"Need help? Copy support info and include it when you report a problem.",{x,y,width,0},32*u,shopTheme::ink)+24*u;
  paragraph(canvas_,"Music: Coral Promenade. Fonts: Lilita One and Luckiest Guy. Built with SDL.",{x,y,width,0},26*u,shopTheme::ink);
  button(l.back,"Settings",backControl);button(l.copySupport,"Copy support info",copySupportControl);
 }
 const std::string messageText=notice_.empty()&&session_.saveFailed()?"Could not save. Please try again.":notice_;
 if(!messageText.empty()){
  const Rect message{d.content.x,d.body.y+4*u,d.content.w,32*u};
  label(canvas_,messageText,message.x+message.w*.5f,message.y,26*u,{112,47,34},message.w,true);
 }
 if(focus_==closeControl)canvas_.outline(l.closeHit,shopTheme::white,12*u,3*u);
}
}
