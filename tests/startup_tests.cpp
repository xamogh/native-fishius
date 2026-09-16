#include "aquarium/view.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static auto misses(const Canvas& c){return std::pair{c.textureLoads_,c.textRasterizations_};}
 static auto memory(const Canvas& c){return std::pair{c.textBytes_,c.retainedTextBytes_};}
 static bool retaining(const Canvas& c){return c.retainText_;}
 static void category(View& v,int category){v.cancelGesture();v.category_=category;v.page_=0;v.panelMotion_.settle();}
 static int page(const View& v){return v.page_;}
 static int pages(const View& v){return v.pageCount_;}
 static Rect grid(const View& v){return v.pageArea_;}
};
}
namespace {
using namespace aq;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void checkStartup(const std::filesystem::path& assets,int w,int h,const std::filesystem::path& captures){
 std::ifstream input(assets/"content.json");Session session(Content::fromJson(Json::parse(input)),"/tmp/aquarium-startup-unused.json",0,true);
 Canvas canvas(assets,w,h,false);View view(canvas,session);
 const auto original=encode(session.domain().state());
 // Work may be cancelled by Quit between steps, without exposing a half-ready game.
 int callbacks=0;
 check(!view.prepareMenus([&](float,std::string_view){return ++callbacks<4;}),"Preparation ignores cancellation");
 check(!ViewTestAccess::retaining(canvas)&&encode(session.domain().state())==original,"Cancelled preparation changes state or leaves text retention active");
 float previous=-1;callbacks=0;const auto start=SDL_GetTicksNS();
 check(view.prepareMenus([&](float progress,std::string_view stage){
  check(progress>=previous&&progress>=0&&progress<=1&&!stage.empty(),"Loading progress is invalid or moves backwards");previous=progress;++callbacks;
  canvas.loadingScreen(progress,stage,2,true,true);
  if(!captures.empty()&&progress>=.5f&&progress<.51f){
   std::filesystem::create_directories(captures);
   check(canvas.capture(captures/(std::to_string(w)+"-loading.png")),"Cannot capture loading screen");
  }
  canvas.present();return true;
 }),"Preparation did not finish");
 check(previous==1&&callbacks>50,"Loading progress skips catalog work");
 check(encode(session.domain().state())==original,"Loading changes the saved game");
 std::cout<<"Prepared "<<w<<'x'<<h<<" in "<<double(SDL_GetTicksNS()-start)/1e6<<" ms; retained text "<<ViewTestAccess::memory(canvas).second/1048576.<<" MiB\n";
 double time=1;std::vector<double> durations;
 auto render=[&](double dt=1./60){
  time+=dt;const auto begin=SDL_GetTicksNS();view.render(time);SDL_FlushRenderer(canvas.renderer());
  durations.push_back(double(SDL_GetTicksNS()-begin)/1e6);canvas.present();
 };
 auto finger=[&](Uint32 type,float x,float y){SDL_Event e{};e.type=type;e.tfinger.touchID=1;e.tfinger.fingerID=1;e.tfinger.x=x/canvas.width();e.tfinger.y=y/canvas.height();view.event(e,time);};
 auto swipe=[&](bool forward){
  const Rect grid=ViewTestAccess::grid(view);const float x=grid.x+grid.w*(forward?.82f:.18f),y=grid.y+grid.h*.4f;
  const float distance=grid.w*(forward?-.64f:.64f);const int page=ViewTestAccess::page(view),pages=ViewTestAccess::pages(view);
  finger(SDL_EVENT_FINGER_DOWN,x,y);
  for(int step=1;step<=6;++step){time+=1./60;finger(SDL_EVENT_FINGER_MOTION,x+distance*step/6,y);render(0);}
  finger(SDL_EVENT_FINGER_UP,x+distance,y);
  for(int step=0;step<5;++step)render(.055);
  check(ViewTestAccess::page(view)==std::clamp(page+(forward?1:-1),0,pages-1),"Swipe lands on the wrong page");
 };
 render();view.setPanel(Panel::Shop);
 for(int category=0;category<5;++category){
  ViewTestAccess::category(view,category);const auto before=ViewTestAccess::misses(canvas);render(.7);
  if(category<3){
   const int pages=ViewTestAccess::pages(view);
   for(int page=1;page<pages;++page)swipe(true);
   for(int page=1;page<pages;++page)swipe(false);
   swipe(false); // rubber band at the first page must not buy a card
  }
  const auto after=ViewTestAccess::misses(canvas);
  if(before!=after)std::cerr<<"Category "<<category<<": "<<after.first-before.first<<" image loads, "<<after.second-before.second<<" text rasterizations\n";
  check(before==after,"First catalog traversal still loads images or rasterizes text");
 }
 check(encode(session.domain().state())==original,"Swiping triggers a purchase or changes the game");
 // Dynamic labels must not evict the prepared shop. Exceed the transient budget.
 canvas.begin();for(int i=0;i<450;++i)canvas.text("Changing receipt "+std::to_string(i),0,0,45);
 const auto before=ViewTestAccess::misses(canvas);ViewTestAccess::category(view,0);render(.7);swipe(true);
 check(before==ViewTestAccess::misses(canvas),"Changing labels evict prepared shop content");
 std::sort(durations.begin(),durations.end());
 std::cout<<"PASS "<<w<<'x'<<h<<": all shop tabs, every page in both directions, edge swipe, real progress, cancellation, unchanged save and cache retention; draw p95 "<<durations[durations.size()*95/100]<<" ms, max "<<durations.back()<<" ms\n";
}
}
int main(int argc,char** argv){try{
 if(argc<2||argc>3)throw std::runtime_error("Usage: aquarium_startup_tests ASSETS [CAPTURES]");
 // The wider desktop window also exercises the pixel density of a 3x phone.
 for(auto [w,h]:{std::pair{852,393},std::pair{1024,768},std::pair{1311,603}})checkStartup(argv[1],w,h,argc==3?argv[2]:std::filesystem::path{});
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
