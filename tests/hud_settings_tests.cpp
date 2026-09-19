#include "aquarium/hud_settings.hpp"
#include "aquarium/hud_care.hpp"
#include "aquarium/game_audio.hpp"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
bool inside(Rect a,Rect b){return a.x>=b.x-.1f&&a.y>=b.y-.1f&&a.x+a.w<=b.x+b.w+.1f&&a.y+a.h<=b.y+b.h+.1f;}
bool overlaps(Rect a,Rect b){return a.x<b.x+b.w-.1f&&b.x<a.x+a.w-.1f&&a.y<b.y+b.h-.1f&&b.y<a.y+a.h-.1f;}
struct TemporaryDirectory {
 std::filesystem::path path=std::filesystem::temp_directory_path()/("fishius-settings-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 TemporaryDirectory(){std::filesystem::create_directories(path);}
 ~TemporaryDirectory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Input {
 Canvas& canvas;HudSettings& menu;HudPointer pointer;
 bool send(Uint32 type,SDL_FPoint p={},bool touch=false){
  int w{},h{};SDL_GetWindowSize(canvas.window(),&w,&h);SDL_Event e{};e.type=type;
  if(touch){e.tfinger.fingerID=1;e.tfinger.x=p.x/canvas.width();e.tfinger.y=p.y/canvas.height();}
  else if(type==SDL_EVENT_MOUSE_MOTION){e.motion.x=p.x/canvas.width()*w;e.motion.y=p.y/canvas.height()*h;}
  else if(type==SDL_EVENT_MOUSE_BUTTON_DOWN||type==SDL_EVENT_MOUSE_BUTTON_UP){e.button.button=SDL_BUTTON_LEFT;e.button.x=p.x/canvas.width()*w;e.button.y=p.y/canvas.height()*h;}
  if(!normalizeHudPointer(pointer,e,float(w),float(h)))return true;return menu.event(e);
 }
 void click(Rect r,bool touch=false){const auto p=center(r);send(touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,p,touch);send(touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP,p,touch);}
 void key(SDL_Keycode key){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=key;check(menu.event(e),"Settings lets keyboard input reach the game");}
};
void storage(const Content& content){
 Domain d(content,1000);auto legacy=encode(d.state());legacy["settings"].erase("vibration");legacy["settings"].erase("reducedMotionOverride");
 auto restored=decodeAndValidate(legacy,content);check(restored.settings.vibration&&!restored.settings.reducedMotionOverride,"Legacy defaults changed");
 legacy["settings"]["reducedMotion"]=true;check(decodeAndValidate(legacy,content).settings.reducedMotionOverride,"An existing reduced-motion choice is lost");
 check(bool(d.execute({.action=Action::SetSystemReducedMotion,.value=1})),"System motion preference failed");
 check(d.state().settings.reducedMotion&&!d.state().settings.reducedMotionOverride,"System motion becomes a manual override");
 d.execute({.action=Action::SetReducedMotion,.value=0});d.execute({.action=Action::SetSystemReducedMotion,.value=1});
 check(!d.state().settings.reducedMotion&&d.state().settings.reducedMotionOverride,"System preference replaces a manual choice");
 TemporaryDirectory temp;const auto path=temp.path/"save.json";Session session(content,path,1000);
 for(const auto [action,value]:std::array<std::pair<Action,double>,5>{{{Action::SetMusic,0},{Action::SetSound,0},{Action::SetVibration,0},{Action::SetReducedMotion,1},{Action::SetVolume,.27}}})
  check(bool(session.command({.action=action,.value=value})),"A setting did not save");
 Session reloaded(content,path,1000);const auto& s=reloaded.domain().state().settings;
 check(!s.music&&!s.sound&&!s.vibration&&s.reducedMotion&&s.reducedMotionOverride&&std::abs(s.volume-.27)<.001,"Preferences did not survive reopening");
 const auto balance=session.domain().state().wallet;
 check(balance.coins==reloaded.domain().state().wallet.coins&&balance.pearls==reloaded.domain().state().wallet.pearls,"Preferences change the wallet");
}
void viewport(const Content& content,const std::filesystem::path& assets,const std::filesystem::path& captures,int w,int h,int density=1,Insets safe={}){
 Canvas canvas(assets,w,h,true);canvas.previewViewport(PreviewViewport{w,h,density,safe});canvas.begin();
 Session session(content,"/tmp/settings-preview-unused.json",1000,true);HudSettings menu(canvas,session);Input input{canvas,menu,{}};menu.open();const auto l=menu.layout();
 const auto in=canvas.safeInsets();const Rect safeArea{in.left,in.top,canvas.width()-in.left-in.right,canvas.height()-in.top-in.bottom};
 check(inside(l.dialog.frame,safeArea),"Settings leaves the safe area");
 std::vector<Rect> targets{l.closeHit,l.sliderHit,l.aboutHit};targets.insert(targets.end(),l.toggleHits.begin(),l.toggleHits.end());
 for(std::size_t i=0;i<targets.size();++i){
  check(inside(targets[i],l.dialog.frame),"A settings target is outside its dialog");
  check(targets[i].w+.1f>=canvas.minimumTouchSize()&&targets[i].h+.1f>=canvas.minimumTouchSize(),"A settings target is smaller than 44 points");
  for(std::size_t j=i+1;j<targets.size();++j)check(!overlaps(targets[i],targets[j]),"Settings touch targets overlap");
 }
 const auto capture=[&](std::string name){
  menu.advance(DialogMotion::duration);
  if(captures.empty())return;const auto folder=captures/(std::to_string(w)+"x"+std::to_string(h)+(density==1?"":"@2x"));std::filesystem::create_directories(folder);
  canvas.begin();canvas.scene(session.domain(),1,0,Tool::Select,{});paintHud(canvas,session.domain(),layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()));menu.paint();
  check(canvas.capture(folder/(name+".png")),"Cannot capture settings");
 };
 capture("preferences");
 for(bool touch:{false,true}){
  const auto down=touch?SDL_EVENT_FINGER_DOWN:SDL_EVENT_MOUSE_BUTTON_DOWN,up=touch?SDL_EVENT_FINGER_UP:SDL_EVENT_MOUSE_BUTTON_UP,motion=touch?SDL_EVENT_FINGER_MOTION:SDL_EVENT_MOUSE_MOTION;
  const bool music=session.domain().state().settings.music;
  input.send(up,center(l.toggles[0]),touch);check(session.domain().state().settings.music==music,"Unmatched release changes music");
  input.send(down,center(l.toggles[0]),touch);input.send(motion,center(l.cards[3]),touch);input.send(up,center(l.toggles[0]),touch);
  check(session.domain().state().settings.music==music,"A dragged toggle activates");
  input.click(l.toggles[0],touch);check(session.domain().state().settings.music!=music,"Music does not toggle");
  input.click(l.toggles[1],touch);input.click(l.toggles[2],touch);input.click(l.toggles[3],touch);
  capture(touch?"touch-toggles":"mouse-toggles");
  const double volume=session.domain().state().settings.volume;const auto revision=session.domain().state().revision;
  input.send(down,center(l.slider),touch);input.send(motion,{l.slider.x+l.slider.w+100,l.slider.y},touch);
  check(menu.volume()==1&&session.domain().state().settings.volume==volume&&session.domain().state().revision==revision,"Dragging volume saves before release");
  input.send(touch?SDL_EVENT_FINGER_CANCELED:SDL_EVENT_WINDOW_FOCUS_LOST,{},touch);input.send(up,center(l.slider),touch);
  check(menu.volume()==volume,"An interrupted slider keeps a preview value");
  input.send(down,center(l.slider),touch);input.send(up,{l.slider.x-20,l.slider.y},touch);
  check(session.domain().state().settings.volume==0,"Slider does not clamp at zero");
  input.send(down,center(l.slider),touch);input.send(up,{l.slider.x+l.slider.w+20,l.slider.y},touch);
  check(session.domain().state().settings.volume==1,"Slider does not clamp at full volume");
  input.click(l.about,touch);check(menu.page()==SettingsPage::About,"About does not open");capture("about");
  input.click(l.copySupport,touch);check(!menu.notice().empty(),"Copy support info has no feedback");
  input.key(SDLK_ESCAPE);check(menu.active()&&menu.page()==SettingsPage::Preferences,"Escape does not return from a secondary page");
  input.click(l.closeHit,touch);check(!menu.active(),"Close does not work");menu.open();
 }
 // Keyboard access includes a focused slider and bounded five-percent steps.
 menu.open();for(int i=0;i<5;++i)input.key(SDLK_TAB);input.key(SDLK_HOME);input.key(SDLK_RIGHT);
 check(std::abs(session.domain().state().settings.volume-.05)<.001,"Keyboard cannot adjust volume");capture("keyboard-focus");
 input.key(SDLK_END);check(session.domain().state().settings.volume==1,"Keyboard cannot reach full volume");
 input.key(SDLK_ESCAPE);check(!menu.active(),"Escape does not close settings");menu.open();
 input.click({l.dialog.frame.x-8,l.dialog.frame.y,1,1});check(!menu.active(),"Backdrop cannot close settings");
 SDL_Event quit{};quit.type=SDL_EVENT_QUIT;menu.open();check(!menu.event(quit),"Settings swallows application quit");
 TemporaryDirectory temp;const auto parent=temp.path/"blocked";Session blocked(content,parent/"save.json",1000);std::ofstream(parent)<<"block save directory";
 HudSettings failure(canvas,blocked);Input failInput{canvas,failure,{}};failure.open();failInput.click(failure.layout().toggles[0]);
 check(blocked.domain().state().settings.music&&!failure.notice().empty(),"Save failure changes music or hides the failure");
 if(w==844&&h==390){
  const auto folder=captures/"844x390";if(!captures.empty()){canvas.begin();canvas.scene(blocked.domain(),1,0,Tool::Select,{});failure.paint();check(canvas.capture(folder/"save-failure.png"),"Cannot capture failure");}
  GameAudio audio(assets);check(audio.available(),"The authored music loop cannot load");Settings s{};
  audio.update(s,.1);check(audio.playing()&&std::abs(audio.gain()-.65f)<.001f,"Music does not play at the saved volume");
  s.sound=false;audio.update(s,.1);check(audio.playing(),"Sound effects mute the music stream");
  s.music=false;audio.update(s,.1);check(!audio.playing()&&audio.gain()==0,"Music toggle does not stop playback");
  s.music=true;s.volume=.27;audio.update(s,.1);check(audio.playing()&&std::abs(audio.gain()-.27f)<.001f,"Music does not use the new volume");
  audio.update(s,0,true);check(!audio.playing()&&audio.gain()==0,"Backgrounding does not stop music");
  audio.update(s,.1);check(audio.playing(),"Music does not resume");
  s.volume=0;audio.update(s,.1);check(!audio.playing(),"Zero volume leaves music playing");
 }
}
}
int main(int argc,char** argv){
 try{
  check(argc>=2,"Pass assets directory");const std::filesystem::path assets=argv[1],captures=argc>2?argv[2]:"";
  std::ifstream in(assets/"content.json");const auto content=aq::Content::fromJson(aq::Json::parse(in));storage(content);
  viewport(content,assets,captures,844,390);viewport(content,assets,captures,667,375);viewport(content,assets,captures,568,320);
  viewport(content,assets,captures,390,844);viewport(content,assets,captures,320,568);viewport(content,assets,captures,1024,768);
  viewport(content,assets,captures,852,393,2,{59,0,59,21});
  std::cout<<"Settings persistence, touch, keyboard, safe layouts, support, audio and save failures passed.\n";return 0;
 }catch(const std::exception& e){std::cerr<<"Settings: "<<e.what()<<'\n';return 1;}
}
