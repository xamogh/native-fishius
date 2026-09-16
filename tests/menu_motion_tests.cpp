#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static Panel panel(const View& v){return v.panel_;}
 static MenuPose pose(const View& v){return v.panelPose_;}
 static bool snapshot(const View& v){return bool(v.panelLayer_);}
 static const auto& dialog(const View& v,int kind){return kind==5?v.helpAnimation_:kind==6?v.levelAnimation_:v.fundsAnimation_;}
 static bool dialogVisible(const View& v,int kind){return kind==5?v.helpOpen_:kind==6?!v.levelUps_.empty():v.fundsDialog_.has_value();}
 static void showDialog(View& v,int kind){
  if(kind==5)v.showHelp();
  else if(kind==6)v.session_.domain().fixture("level-up");
  else if(kind==7)v.showCurrencyFunds(Currency::Coins);
  else v.showFunds(std::array{CurrencyShortfall{240,0},CurrencyShortfall{0,3},CurrencyShortfall{2,3},CurrencyShortfall{},CurrencyShortfall{5,8}}[kind]);
 }
 static std::pair<std::size_t,std::size_t> resources(const Canvas& c){return {c.textures_.size(),c.textCache_.size()};}
 static bool has(const View& v,std::string_view id){return std::any_of(v.buttons_.begin(),v.buttons_.end(),[&](const auto& b){return b.id==id;});}
 static Rect button(const View& v,std::string_view id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing button: "+std::string(id));}
};
}
namespace {
using namespace aq;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void checkSpring(){
 MenuMotion motion;motion.show(true,1);double peak=0;
 for(int i=0;i<=120;++i){const auto p=motion.sample(1+i/120.);check(std::isfinite(p.value)&&p.value>=0&&p.value<1.2,"Menu spring is unstable");peak=std::max(peak,p.value);}
 check(peak>1.05&&motion.sample(2).value==1,"Menu never bounces or settles");
 for(double t:{1.04,1.13,1.21,1.25}){
  const auto before=motion.sample(t);motion.show(!motion.visible,t);const auto after=motion.sample(t);
  check(std::abs(before.value-after.value)<1e-9&&std::abs(before.velocity-after.velocity)<1e-9,"Reversing a menu jumps in position or velocity");
 }
 motion.show(false,2);check(motion.sample(2.5).value==0,"Dismissed menu does not finish");
 std::cout<<"PASS spring overshoot, settling and rapid reversal continuity\n";
}
void checkBlend(Canvas& canvas){
 Texture layer;canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,255,255});
 canvas.beginMenuLayer(layer);canvas.fill({0,0,canvas.width(),canvas.height()},{255,0,0,128});canvas.endMenuLayer();
 canvas.menuLayer(layer,{{},1,1,0,.5f});
 auto* raw=SDL_RenderReadPixels(canvas.renderer(),nullptr);check(raw,"Cannot read menu compositing");
 auto* rgba=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(raw);check(rgba,"Cannot read menu RGBA");
 const auto* pixel=static_cast<const Uint8*>(rgba->pixels)+rgba->h/2*rgba->pitch+rgba->w/2*4;
 const bool correct=std::abs(int(pixel[0])-64)<=2&&pixel[1]==0&&std::abs(int(pixel[2])-191)<=2;
 SDL_DestroySurface(rgba);check(correct,"Fading a transparent menu darkens or doubles its soft edges");canvas.present();
}
void checkPreparedMenus(const std::filesystem::path& assets,int w,int h){
 std::ifstream file(assets/"content.json");Session session(Content::fromJson(Json::parse(file)),"/tmp/aquarium-preparation-unused.json",0,true);
 Canvas canvas(assets,w,h,false);View view(canvas,session);const auto state=encode(session.domain().state());
 const auto start=SDL_GetTicksNS();view.prepareMenus();const double preparation=double(SDL_GetTicksNS()-start)/1e6;
 check(encode(session.domain().state())==state&&ViewTestAccess::panel(view)==Panel::None,"Menu preparation changes the player's game or view");
 view.render(1);canvas.present();double time=1;
 for(auto panel:{Panel::Shop,Panel::Collection}){
  const auto cached=ViewTestAccess::resources(canvas);view.setPanel(panel);
  const auto first=SDL_GetTicksNS();view.render(time+=.04);SDL_FlushRenderer(canvas.renderer());const double elapsed=double(SDL_GetTicksNS()-first)/1e6;canvas.present();
  check(ViewTestAccess::resources(canvas)==cached,"First menu opening still decodes artwork or rasterizes text");
  std::cout<<"PASS prepared "<<(panel==Panel::Shop?"Shop":"Mastery")<<' '<<w<<'x'<<h<<": first draw "<<elapsed<<" ms, no resource cache misses (startup preparation "<<preparation<<" ms)\n";
  view.setPanel(Panel::None);view.render(time+=1);canvas.present();
 }
}
void checkView(const std::filesystem::path& assets,int w,int h,const std::filesystem::path& captures){
 std::ifstream file(assets/"content.json");Session session(Content::fromJson(Json::parse(file)),"/tmp/aquarium-animation-unused.json",0,true);
 Canvas canvas(assets,w,h,false);checkBlend(canvas);View view(canvas,session);double time=1;
 auto render=[&](double dt){time+=dt;view.render(time);canvas.present();};
 auto capture=[&](std::string name){if(captures.empty())return;std::filesystem::create_directories(captures);view.render(time);check(canvas.capture(captures/(std::to_string(w)+"-"+name+".png")),"Cannot capture menu animation");canvas.present();};
 auto finger=[&](Uint32 type,float x,float y){SDL_Event e{};e.type=type;e.tfinger.touchID=1;e.tfinger.fingerID=1;e.tfinger.x=x/canvas.width();e.tfinger.y=y/canvas.height();view.event(e,time);};
 auto tap=[&](float x,float y){finger(SDL_EVENT_FINGER_DOWN,x,y);finger(SDL_EVENT_FINGER_UP,x,y);};
 auto mouse=[&](Uint32 type,float x,float y){
  int width{},height{};SDL_GetWindowSize(canvas.window(),&width,&height);
  SDL_Event e{};e.type=type;e.button.button=SDL_BUTTON_LEFT;e.button.x=x/canvas.width()*width;e.button.y=y/canvas.height()*height;view.event(e,time);
 };
 auto button=[&](std::string_view id){const auto r=ViewTestAccess::button(view,id);tap(r.x+r.w*.5f,r.y+r.h*.5f);};
 auto key=[&](SDL_Keycode keycode){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=keycode;view.event(e,time);};
 render(0);
 for(auto panel:{Panel::Tanks,Panel::Shop,Panel::Inventory,Panel::Collection,Panel::Settings,Panel::Quests,Panel::Gifts,Panel::CurrencyShop}){
  view.setPanel(panel);render(.04);auto early=ViewTestAccess::pose(view);
  check(early.alpha>0&&early.alpha<1&&early.xScale<1,"Menu appears instantly");capture("open-"+std::to_string(int(panel))+"-early");
  render(.125);check(ViewTestAccess::pose(view).xScale>1,"Menu has no visible spring overshoot");capture("open-"+std::to_string(int(panel))+"-bounce");
  render(.5);check(ViewTestAccess::pose(view).xScale==1&&ViewTestAccess::pose(view).alpha==1,"Menu does not settle at full size");capture("open-"+std::to_string(int(panel))+"-settled");
  button("panel-close");render(.04);check(ViewTestAccess::panel(view)==Panel::None&&ViewTestAccess::snapshot(view),"Close has no visual exit");
  check(!ViewTestAccess::has(view,"panel-close"),"Closing menu still has live controls");capture("close-"+std::to_string(int(panel)));
  render(.4);check(!ViewTestAccess::snapshot(view),"Closed menu retains its full-size texture");
 }
 // Buttons remain usable at their visible positions while their panel moves.
 view.setPanel(Panel::Shop);render(.065);button("panel-close");render(.02);check(ViewTestAccess::panel(view)==Panel::None,"Moving close button misses taps");
 const auto closing=ViewTestAccess::pose(view);key(SDLK_B);render(0);const auto reopened=ViewTestAccess::pose(view);
 check(std::abs(closing.xScale-reopened.xScale)<.001&&std::abs(closing.rise-reopened.rise)<.001,"Reopening Shop jumps to its initial size");
 render(.7);const auto state=encode(session.domain().state());tap(12,canvas.height()*.5f);render(.01);
 check(ViewTestAccess::panel(view)==Panel::None,"Shop backdrop does not dismiss");tap(canvas.width()*.5f,canvas.height()*.5f);render(.01);
 check(encode(session.domain().state())==state&&ViewTestAccess::panel(view)==Panel::None,"Dismissing Shop sends a tap through to the aquarium");render(.5);
 check(!ViewTestAccess::has(view,"tool-select")&&!ViewTestAccess::has(view,"select-option0"),"Select menu is still visible");
 button("inventory");render(.7);check(ViewTestAccess::panel(view)==Panel::Inventory,"Bag does not open inventory directly");
 button("panel-close");render(.5);
 view.fixture("details");render(.01);key(SDLK_ESCAPE);render(.5);
 const auto* fish=session.domain().fish(session.domain().state().fish.front().id);const auto position=canvas.toScreen(fish->position);tap(position.x,position.y);render(.04);
 check(ViewTestAccess::panel(view)==Panel::Details&&ViewTestAccess::pose(view).xScale<1,"Fish details do not animate");capture("details-early");
 button("details-close");render(.02);check(ViewTestAccess::panel(view)==Panel::None,"Moving fish close button misses taps");render(.5);
 session.domain().execute({.action=Action::SetReducedMotion,.value=1});view.setPanel(Panel::Shop);render(.01);
 check(ViewTestAccess::pose(view).xScale==1&&ViewTestAccess::pose(view).yScale==1&&ViewTestAccess::pose(view).rise==0,"Reduced Motion still moves menus");
 key(SDLK_ESCAPE);render(.01);check(!ViewTestAccess::snapshot(view),"Reduced Motion waits for a hidden exit");
 button("inventory");render(.01);check(ViewTestAccess::panel(view)==Panel::Inventory&&ViewTestAccess::pose(view).xScale==1,"Reduced Motion delays Bag");
 view.setPanel(Panel::None);render(.5);
 // Exercise each modal independently, including dialogs over an existing menu.
 for(bool reduced:{false,true})for(int kind=0;kind<8;++kind){
  session.domain().execute({.action=Action::SetReducedMotion,.value=reduced?1.:0.});
  view.setPanel(Panel::Settings);render(.7);
  const auto navigation=ViewTestAccess::button(view,"nav1");
  ViewTestAccess::showDialog(view,kind);render(0);
  const auto saved=encode(session.domain().state());
  const auto close=kind==5?"help-close":kind==6?"level-close":"funds-close";
  if(!reduced){
   render(.04);const auto early=ViewTestAccess::dialog(view,kind).pose;
   check(early.alpha>0&&early.alpha<1&&early.xScale<1,"Dialog appears instantly");capture("dialog-"+std::to_string(kind)+"-early");
   render(.125);check(ViewTestAccess::dialog(view,kind).pose.xScale>1,"Dialog does not bounce");capture("dialog-"+std::to_string(kind)+"-bounce");
   // Close at the overshoot, where the visible target differs from its layout.
  }else{
   const auto pose=ViewTestAccess::dialog(view,kind).pose;
   check(pose.xScale==1&&pose.yScale==1&&pose.rise==0&&pose.alpha==1,"Reduced Motion still animates a dialog");
  }
  const auto target=ViewTestAccess::button(view,close);
  check(target.w+.01f>=canvas.minimumTouchSize()&&target.h+.01f>=canvas.minimumTouchSize(),"Moving dialog shrinks its touch target");
  button(close);check(!ViewTestAccess::dialogVisible(view,kind),"Moving dialog close misses taps");render(.04);
  if(!reduced){
   check(ViewTestAccess::dialog(view,kind).layer&&!ViewTestAccess::has(view,close),"Closing dialog disappears instantly or retains controls");
   capture("dialog-"+std::to_string(kind)+"-closing");
   tap(navigation.x+navigation.w*.5f,navigation.y+navigation.h*.5f);key(SDLK_B);render(.01);
   check(ViewTestAccess::panel(view)==Panel::Settings&&encode(session.domain().state())==saved,"Closing dialog leaks input to the menu or aquarium");
   render(.4);
  }
  check(!ViewTestAccess::dialog(view,kind).layer,"Closed dialog retains its snapshot");
  for(bool useMouse:{false,true}){
   ViewTestAccess::showDialog(view,kind);render(0);render(.04);
   const auto& animation=ViewTestAccess::dialog(view,kind);
   const auto bounds=animation.pose.apply(animation.bounds);
   const float insideX=bounds.x+bounds.w*.05f,y=bounds.y+bounds.h*.5f;
   const auto down=[&](float x,float y){if(useMouse)mouse(SDL_EVENT_MOUSE_BUTTON_DOWN,x,y);else finger(SDL_EVENT_FINGER_DOWN,x,y);};
   const auto up=[&](float x,float y){if(useMouse)mouse(SDL_EVENT_MOUSE_BUTTON_UP,x,y);else finger(SDL_EVENT_FINGER_UP,x,y);};
   // Pressing the body, then releasing outside, must leave the dialog open.
   down(insideX,y);up(bounds.x-1,y);
   check(ViewTestAccess::dialogVisible(view,kind),"A press inside the dialog dismisses it");
   const auto before=encode(session.domain().state());
   down(bounds.x-1,y);up(bounds.x-1,y);
   check(!ViewTestAccess::dialogVisible(view,kind),"Mouse or touch backdrop does not close the moving dialog");
   // Queued taps and taps during the exit cannot activate the menu underneath.
   tap(navigation.x+navigation.w*.5f,navigation.y+navigation.h*.5f);render(.02);
   if(!reduced)tap(navigation.x+navigation.w*.5f,navigation.y+navigation.h*.5f);
   check(ViewTestAccess::panel(view)==Panel::Settings&&encode(session.domain().state())==before,"Backdrop dismissal reaches the underlying menu or changes state");
   render(.5);check(!ViewTestAccess::dialog(view,kind).layer,"Backdrop dismissal retains its closing layer");
  }
 }
 session.domain().execute({.action=Action::SetReducedMotion,.value=0});
 ViewTestAccess::showDialog(view,0);render(.7);
 ViewTestAccess::showDialog(view,6);ViewTestAccess::showDialog(view,6);render(0);render(.7);
 button("level-continue");render(.04);
 check(ViewTestAccess::dialog(view,6).pose.xScale<1&&!ViewTestAccess::has(view,"funds-close"),"Queued level-up does not animate or exposes the dialog underneath");
 render(.7);tap(1,canvas.height()*.5f);tap(1,canvas.height()*.5f);render(.04);
 check(!ViewTestAccess::dialogVisible(view,6)&&ViewTestAccess::dialogVisible(view,0),"Backdrop dismissal reaches the dialog underneath a closing level-up");
 render(.4);
 check(ViewTestAccess::has(view,"funds-close")&&ViewTestAccess::dialogVisible(view,0),"Level-up exit loses the underlying funds dialog");
 button("funds-close");render(.4);
 std::cout<<"PASS "<<w<<'x'<<h<<": menu and dialog bounces, exits, moving controls, mouse/touch backdrops, queued level-ups, modal input, reopening and Reduced Motion\n";
}
}
int main(int argc,char** argv){try{if(argc<2||argc>3)throw std::runtime_error("Usage: aquarium_menu_motion_tests ASSETS [CAPTURES]");checkSpring();for(auto [w,h]:{std::pair{852,393},std::pair{1024,768}}){checkPreparedMenus(argv[1],w,h);checkView(argv[1],w,h,argc==3?argv[2]:std::filesystem::path{});}return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
