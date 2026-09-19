#include "aquarium/hud_settings.hpp"
#include "aquarium/hud_funds.hpp"
#include "aquarium/hud_care.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
Rect safeArea(const Canvas& c){const auto s=c.safeInsets();return {s.left,s.top,c.width()-s.left-s.right,c.height()-s.top-s.bottom};}
void motion(){
 const Rect frame{200,100,800,500},safe{0,0,1200,800};DialogMotion a,b;
 check(a.pose(frame,safe).scale<.9f,"Dialog does not start smaller");
 a.advance(true,.16,false);check(a.pose(frame,safe).scale>1.03f,"Dialog has no visible overshoot");
 a.advance(true,.17,false);check(a.pose(frame,safe).scale<1,"Dialog has no settling rebound");
 for(int i=0;i<33;++i)b.advance(true,.01,false);
 check(std::abs(a.pose(frame,safe).scale-b.pose(frame,safe).scale)<.00001f,"Bounce depends on frame rate");
 const auto pose=a.pose(frame,safe);const auto p=center(frame),roundTrip=pose.inverse(pose.apply(p));
 check(std::hypot(p.x-roundTrip.x,p.y-roundTrip.y)<.001,"Input does not match the drawing transform");
 a.advance(true,std::numeric_limits<double>::quiet_NaN(),false);a.advance(true,-3,false);
 check(a.pose(frame,safe).scale==pose.scale,"Invalid time changes the bounce");
 a.advance(true,5,false);check(a.pose(frame,safe).scale==1,"Dialog never settles");
 a.advance(false,0,false);check(a.pose(frame,safe).scale<.9f,"Reopening does not restart the bounce");
 a.advance(true,0,true);check(a.pose(frame,safe).scale==1,"Reduced motion still bounces");
 a.advance(true,0,false);check(a.pose(frame,safe).scale==1,"Changing motion preference replays the bounce");
 a={};a.advance(true,.16,false);
 for(const Rect edge:std::array{Rect{0,0,800,500},Rect{400,300,800,500},Rect{0,0,1190,790}}){
  const auto transform=a.pose(edge,safe);const auto top=transform.apply({edge.x,edge.y}),bottom=transform.apply({edge.x+edge.w,edge.y+edge.h});
  check(top.x>=-.01f&&top.y>=-.01f&&bottom.x<=1200.01f&&bottom.y<=800.01f,"Overshoot leaves the safe area");
 }
}
void rendering(const std::filesystem::path& assets){
 Canvas c(assets,852,393,true);c.begin();const Rect frame{250,120,650,450};DialogMotion motion;
 c.origin(3,5);c.clip({0,0,1000,700});
 SDL_FPoint before;SDL_GetRenderScale(c.renderer(),&before.x,&before.y);SDL_Rect clip;SDL_GetRenderClipRect(c.renderer(),&clip);
 const auto ready=c.cacheStats();
 for(int i=0;i<=36;++i){
  motion.advance(true,i?1./60:0,false);
  {
   const DialogPaint animation(c,motion,frame);
   c.clip(frame);c.fill(frame,{12,100,140});c.text("Bouncy dialog",400,300,28);c.clearClip();
  }
  SDL_FPoint after;SDL_GetRenderScale(c.renderer(),&after.x,&after.y);SDL_Rect restored;SDL_GetRenderClipRect(c.renderer(),&restored);
  check(c.origin()==3&&c.originY()==5&&before.x==after.x&&before.y==after.y,"Dialog leaks its render transform");
  check(SDL_RenderClipEnabled(c.renderer())&&clip.x==restored.x&&clip.y==restored.y&&clip.w==restored.w&&clip.h==restored.h,"Dialog leaks its clipping region");
 }
 check(c.cacheStats().textRasterizations==ready.textRasterizations+1,"Bounce rasterizes text on every frame");
 c.origin(0);c.clearClip();
 // Click the visible close button at the initial, scaled position.
 DialogState state;const auto layout=layoutDialog(c.width(),c.height(),{},{});
 {
  const DialogPaint animation(c,state.motion,layout.frame);
  paintDialog(c,layout,{},false,false,DialogPresentation::ModalContent);
 }
 const auto point=state.motion.pose(layout.frame,safeArea(c)).apply(center(layout.close));
 SDL_Event event{};event.button.button=SDL_BUTTON_LEFT;event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;dialogEvent(state,layout,event,point);
 event.type=SDL_EVENT_MOUSE_BUTTON_UP;dialogEvent(state,layout,event,point);
 check(!state.open,"Animated close button does not respond at its visible position");
 FundsDialogState funds;showFundsDialog(funds,{.error=Error::Funds,.shortfall={10,0}});
 const auto fundsLayout=layoutFundsDialog(c.width(),c.height(),{},c.minimumTouchSize());
 paintFundsDialog(c,fundsLayout,funds);
 const auto shop=funds.dialog.motion.pose(fundsLayout.dialog.frame,safeArea(c)).apply(center(fundsLayout.shop));
 event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;fundsDialogEvent(funds,fundsLayout,event,shop);
 event.type=SDL_EVENT_MOUSE_BUTTON_UP;
 check(fundsDialogEvent(funds,fundsLayout,event,shop)==FundsDialogEvent::OpenShop,"Animated funds button does not open Shop");
}
void settings(const std::filesystem::path& assets,const Content& content){
 for(bool touch:{false,true}){
  Canvas c(assets,852,393,true);c.begin();Session session(content,"/tmp/dialog-motion-unused.json",0,true);HudSettings menu(c,session);menu.open();menu.paint();
  const auto l=menu.layout();DialogMotion motion;const auto p=motion.pose(l.dialog.frame,safeArea(c)).apply(center(l.toggles[0]));
  const bool music=session.domain().state().settings.music;HudPointer pointer;
  for(bool down:{true,false}){
   SDL_Event e{};
   if(touch){e.type=down?SDL_EVENT_FINGER_DOWN:SDL_EVENT_FINGER_UP;e.tfinger.fingerID=1;e.tfinger.x=p.x/c.width();e.tfinger.y=p.y/c.height();}
   else{e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;e.button.button=SDL_BUTTON_LEFT;e.button.x=p.x/c.width()*852;e.button.y=p.y/c.height()*393;}
   check(normalizeHudPointer(pointer,e,852,393),"Pointer was not normalized");check(menu.event(e),"Animated Settings does not consume input");
  }
  check(session.domain().state().settings.music!=music,"Animated Settings toggle misses mouse or touch input");
 }
}
}
int main(int argc,char** argv){try{
 check(argc>1,"Pass the assets directory");const std::filesystem::path assets=argv[1];std::ifstream input(assets/"content.json");const auto content=aq::Content::fromJson(aq::Json::parse(input));
 motion();rendering(assets);settings(assets,content);
 std::cout<<"PASS dialog bounce, reopening, reduced motion, safe areas, rendering state, text cache and animated mouse/touch controls\n";
 return 0;
}catch(const std::exception& e){std::cerr<<"Dialog motion: "<<e.what()<<'\n';return 1;}}
