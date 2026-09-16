#include "fixtures.hpp"
#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static Rect button(const View& v,std::string_view id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing button: "+std::string(id));}
 static void move(View& v,SDL_FPoint p){v.pointerMove(p.x,p.y);}
 static void tap(View& v,SDL_FPoint p){v.pointerDown(p.x,p.y);v.pointerUp(p.x,p.y);}
 static void setTool(View& v,Tool t){v.setTool(t);}
 static Panel panel(const View& v){return v.panel_;}
 static CursorKind cursor(const Canvas& c){return c.cursorKind_;}
 static double foodAt(const View& v){return v.jarAt_;}
 static double netAt(const View& v){return v.netAt_;}
 static bool preview(const View& v){return v.decorPreview_.has_value();}
};
}
namespace {
using namespace aq;
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
bool near(double a,double b){return std::abs(a-b)<.0001;}
void checkAnimation(){
 const auto can=toolCursorPose(Tool::Food,.1),net=toolCursorPose(Tool::Sell,.125);
 check(near(can.angle,-70)&&near(can.scale,.96),"Food can does not match Phaser's tilt and scale");
 check(near(net.angle,-18)&&near(net.scale,1.08),"Net does not match Phaser's swing and scale");
 for(double t:{-.1,0.,.2,1.})check(near(toolCursorPose(Tool::Food,t).angle,-30),"Food can does not return after 200 ms");
 for(double t:{-.1,0.,.25,1.})check(near(toolCursorPose(Tool::Sell,t).angle,0),"Net does not return after 250 ms");
 check(near(toolCursorPose(Tool::Food,.1,true).angle,-30)&&near(toolCursorPose(Tool::Sell,.125,true).scale,1),"Reduced motion still swings or tips");
}
void viewport(const Content& content,const std::filesystem::path& assets,int width,int height){
 Canvas canvas(assets,width,height,true);Session session(content,"/tmp/aquarium-tool-tests.json",1000,true);View view(canvas,session);double time=1;
 auto render=[&](double dt=.3){time+=dt;view.render(time);};
 auto button=[&](std::string_view id){const auto b=ViewTestAccess::button(view,id);ViewTestAccess::tap(view,{b.x+b.w*.5f,b.y+b.h*.5f});render(.7);};
 const auto out=std::filesystem::path("evidence/tool-actions")/(std::to_string(width)+"x"+std::to_string(height));std::filesystem::create_directories(out);
 // Keep tool interactions near the visible bottom on both phone and tablet.
 const auto water=canvas.toScreen({540,625});render();
 button("tool-food");ViewTestAccess::move(view,water);render();
 check(ViewTestAccess::cursor(canvas)==CursorKind::Hidden,"Food sprite does not replace the arrow");
 check(canvas.capture(out/"food-ready.png"),"Cannot capture ready can");
 const auto before=session.domain().pellets().size();ViewTestAccess::tap(view,water);render(.1);
 check(session.domain().pellets().size()==before+1,"One feed tap did not drop exactly one pellet");
 check(near(time-ViewTestAccess::foodAt(view),.1),"Feed tap did not start the can animation");
 check(canvas.capture(out/"food-tilted.png"),"Cannot capture tilted can");
 ViewTestAccess::tap(view,water);check(near(ViewTestAccess::foodAt(view),time),"Rapid feeding did not restart the animation");
 render(.3);check(session.domain().pellets().size()==before+2,"Holding the tool auto-dispensed food");
 button("tool-food");check(view.tool()==Tool::Select,"Food button does not toggle off");

 // UI chrome owns its taps. The can/net must not leak through a dialog.
 button("tool-food");button("nav1");check(ViewTestAccess::cursor(canvas)==CursorKind::Arrow,"Shop did not restore the default cursor");
 const auto modalCount=session.domain().pellets().size();button("shop-tab1");
 check(session.domain().pellets().size()==modalCount,"Shop tap dropped food underneath it");
 view.setPanel(Panel::None);ViewTestAccess::setTool(view,Tool::Select);render(.7);

 auto state=session.domain().state();state.fish.resize(1);auto& fish=state.fish.front();fish.position={540,625};fish.motion.previous=fish.position;fish.egg=false;fish.dead=false;testing::stage(fish,4);fish.lastFedAt=state.simNow;
 session.domain().install(state);render();button("tool-sell");ViewTestAccess::move(view,water);render();
 check(ViewTestAccess::cursor(canvas)==CursorKind::Hidden,"Net does not replace the arrow");
 check(canvas.capture(out/"net-ready.png"),"Cannot capture ready net");
 const auto coins=session.domain().state().wallet.coins;ViewTestAccess::tap(view,water);render(.125);
 check(session.domain().state().fish.size()==1&&session.domain().state().wallet.coins==coins&&ViewTestAccess::panel(view)==Panel::Details,"Net did not open a safe rehome preview");
 render(.7);button("fish-rehome");button("rehome-confirm");check(session.domain().state().fish.empty()&&session.domain().state().wallet.coins>coins,"Confirmed rehome did not settle");render(.7);

 // Touch-only taps show the can animation without requiring a prior hover.
 button("tool-food");const auto foodCount=session.domain().pellets().size();
 SDL_Event event{};event.type=SDL_EVENT_FINGER_DOWN;event.tfinger.touchID=1;event.tfinger.fingerID=1;event.tfinger.x=water.x/canvas.width();event.tfinger.y=water.y/canvas.height();view.event(event,time);
 event.type=SDL_EVENT_FINGER_UP;view.event(event,time);render(.1);
 check(session.domain().pellets().size()==foodCount+1&&near(time-ViewTestAccess::foodAt(view),.1),"Touch did not feed and animate without hover");
 check(canvas.capture(out/"food-touch.png"),"Cannot capture touch feedback");
 event={};event.type=SDL_EVENT_WINDOW_FOCUS_LOST;view.event(event,time);render();
 check(ViewTestAccess::cursor(canvas)==CursorKind::Arrow,"Focus loss left a hidden/custom cursor");
 std::cout<<"PASS tool cursors and Phaser tap feedback "<<width<<'x'<<height<<'\n';
}
void menuReset(const Content& content,const std::filesystem::path& assets,int width,int height){
 Canvas canvas(assets,width,height,true);Session session(content,"/tmp/aquarium-menu-reset-unused.json",1000,true);
 auto state=session.domain().state();state.fish.resize(1);auto& fish=state.fish.front();fish.position={540,320};fish.motion.previous=fish.position;fish.egg=false;fish.dead=false;testing::stage(fish,4);fish.lastFedAt=state.simNow;
 session.domain().install(state);View view(canvas,session);double time=1;
 const auto render=[&]{view.render(time+=.7);};render();
 for(bool touch:{false,true}){
  const auto tap=[&](SDL_FPoint p){
   if(!touch){ViewTestAccess::tap(view,p);return;}
   SDL_Event e{};e.type=SDL_EVENT_FINGER_DOWN;e.tfinger.touchID=1;e.tfinger.fingerID=1;e.tfinger.x=p.x/canvas.width();e.tfinger.y=p.y/canvas.height();view.event(e,time);
   e.type=SDL_EVENT_FINGER_UP;view.event(e,time);
  };
  const auto click=[&](const char* id){const auto r=ViewTestAccess::button(view,id);tap({r.x+r.w*.5f,r.y+r.h*.5f});render();};
  for(const auto* tool:{"tool-food","tool-sell"})for(const auto* menu:{"nav0","nav1","inventory","nav3","notifications","menu","coin-more","pearl-more","coin-balance","pearl-balance"}){
   click(tool);check(view.tool()==(std::string_view(tool)=="tool-food"?Tool::Food:Tool::Sell),"Tool did not activate");
   const auto before=encode(session.domain().state());const auto pellets=session.domain().pellets().size();
   ViewTestAccess::move(view,canvas.toScreen({540,320}));render();
   click(menu);
   check(view.tool()==Tool::Select&&ViewTestAccess::cursor(canvas)==CursorKind::Arrow,"Opening a menu kept Food or Sell active");
   check(ViewTestAccess::foodAt(view)<0&&ViewTestAccess::netAt(view)<0,"Menu retained a tool animation");
   click("panel-close");
   check(view.tool()==Tool::Select&&ViewTestAccess::panel(view)==Panel::None,"Closing a menu restored Food or Sell");
   tap(canvas.toScreen({540,320}));render();
   check(encode(session.domain().state())==before&&session.domain().pellets().size()==pellets,"Tap after closing a menu sold or fed a fish");
   check(ViewTestAccess::panel(view)==Panel::Details,"Tap after closing a menu did not return to normal fish selection");
   click("details-close");
  }
 }
 std::cout<<"PASS Food and Sell reset on all menu and currency buttons, mouse and touch "<<width<<'x'<<height<<'\n';
}

}
int main(int argc,char** argv){try{if(argc!=2)return 2;const auto assets=std::filesystem::absolute(argv[1]);std::ifstream input(assets/"content.json");const auto content=aq::Content::fromJson(aq::Json::parse(input));checkAnimation();for(auto [w,h]:{std::pair{852,393},std::pair{1024,768}}){viewport(content,assets,w,h);menuReset(content,assets,w,h);}return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
