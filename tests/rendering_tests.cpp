#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
Surface read(SDL_Renderer* renderer){
 Surface raw(SDL_RenderReadPixels(renderer,nullptr),SDL_DestroySurface);check(bool(raw),"Cannot read frame");
 Surface rgba(SDL_ConvertSurface(raw.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);check(bool(rgba),"Cannot convert frame");return rgba;
}
const Uint8* pixel(const SDL_Surface& s,int x,int y){return static_cast<const Uint8*>(s.pixels)+y*s.pitch+x*4;}
void checkFrames(const std::filesystem::path& assets){
 Canvas canvas(assets,852,393,false);
 const auto directory=std::filesystem::temp_directory_path()/"aquarium-rendering-tests";std::filesystem::create_directories(directory);
 for(auto [w,h]:{std::pair{852,393},std::pair{1024,768},std::pair{1210,834},std::pair{744,518},std::pair{667,375}}){
  check(SDL_SetWindowSize(canvas.window(),w,h),"Cannot resize test window");SDL_SyncWindow(canvas.window());SDL_PumpEvents();
  // Present the resized frame so Metal releases the previous window drawable.
  canvas.begin();canvas.present();canvas.begin();
  int pw{},ph{},ww{},wh{};SDL_GetWindowSizeInPixels(canvas.window(),&pw,&ph);SDL_GetWindowSize(canvas.window(),&ww,&wh);
  auto* target=SDL_GetRenderTarget(canvas.renderer());
  check(target&&target->w==pw&&target->h==ph,"Frame resolution differs from native display pixels");
  check(std::abs(canvas.width()/canvas.height()-float(ww)/wh)<.0001,"Canvas does not fill the window aspect ratio");
  auto first=canvas.inputPoint(0,0,true),last=canvas.inputPoint(1,1,true);
  check(first.x==0&&first.y==0&&std::abs(last.x-canvas.width())<.01&&std::abs(last.y-canvas.height())<.01,"Touch mapping leaves a letterbox margin");
  canvas.fill({0,0,canvas.width(),canvas.height()},{231,61,117,255});
  const auto capture=directory/"native.png";check(canvas.capture(capture,true),"Cannot capture composed window");
  Surface image(IMG_Load(capture.string().c_str()),SDL_DestroySurface);Surface rgba(SDL_ConvertSurface(image.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
  check(rgba->w==pw&&rgba->h==ph,"Capture does not use physical pixels");
  for(int y=0;y<ph;++y)for(int x=0;x<pw;++x){const auto* p=pixel(*rgba,x,y);check(p[0]==231&&p[1]==61&&p[2]==117,"Background seam, gutter or cropped frame after resize");}
 }
 // A fine opaque/transparent pattern must converge to its area average when
 // small, with no blue fringe from fully transparent texels and no shimmer.
 Surface checker(SDL_CreateSurface(256,256,SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
 for(int y=0;y<256;++y)for(int x=0;x<256;++x){auto* p=static_cast<Uint8*>(checker->pixels)+y*checker->pitch+x*4;const bool on=(x+y)%2==0;p[0]=on?255:0;p[1]=0;p[2]=on?0:255;p[3]=on?255:0;}
 const auto pattern=directory/"filter-pattern.png";check(IMG_SavePNG(checker.get(),pattern.string().c_str()),"Cannot write filter fixture");
 for(float offset:{0.f,.17f,.49f,.73f}){
  canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,255});canvas.image(pattern.string(),{20+offset,20,24,24});
  auto frame=read(canvas.renderer());const float sx=float(frame->w)/canvas.width(),sy=float(frame->h)/canvas.height();
  for(int y=int(24*sy);y<int(40*sy);++y)for(int x=int(24*sx);x<int(40*sx);++x){auto* p=pixel(*frame,x,y);check(std::abs(int(p[0])-128)<=2&&p[1]==0&&p[2]==0,"Minified art shimmers or has a transparent-color fringe");}
 }
 auto textFrame=[&]{
  canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,255});
  canvas.text("Molly 250",30,30,30,{255,0,0,255});canvas.text("Molly 250",330,30,30,{0,80,255,255});
  canvas.label("GROWTH PAUSED",320,120,38,true,500);canvas.text("BABY FISH",320,200,64,{255,229,119,255},true,230,true,true,true);
  return read(canvas.renderer());
 };
 auto a=textFrame(),b=textFrame();int red=0,blue=0;
 for(int y=0;y<a->h;++y)for(int x=0;x<a->w;++x){auto* p=pixel(*a,x,y);auto* q=pixel(*b,x,y);check(std::equal(p,p+4,q),"Static text changes pixels across identical frames");red+=p[0]>200&&p[1]<10&&p[2]<10;blue+=p[2]>200&&p[0]<10;}
 check(red>50&&blue>50,"Shared glyph cache leaks tint between labels");
 std::cout<<"PASS native frames, tablet/phone resizing, full-screen composition, texture filtering and stable tinted text\n";
}
void checkMotion(const std::filesystem::path& assets){
 std::ifstream input(assets/"content.json");Domain domain(Content::fromJson(Json::parse(input)),0);
 auto state=domain.state();auto& fish=state.fish.front();const auto id=fish.id;fish.position={450,280};fish.motion.turnFrom=1;fish.motion.direction=1;fish.motion.turnDuration=.4;fish.motion.turnRemaining=.21;fish.motion.pitch=.2;fish.motion.phase=2;
 domain.install(state);domain.stepMovement(.02,Tool::Select,{});const auto& updated=*domain.fish(id);
 auto a=fishPose(updated,0),mid=fishPose(updated,.5),b=fishPose(updated,1);
 check(a.facing<0&&b.facing>0&&std::abs(mid.facing)<.001,"Fish snaps instead of passing smoothly through its turn");
 check(std::abs(mid.phase-(a.phase+b.phase)*.5)<1e-9&&std::abs(mid.pitch-(a.pitch+b.pitch)*.5)<1e-9,"Fins or pitch remain locked to simulation steps");
 check(std::abs(mid.position.x-(a.position.x+b.position.x)*.5)<1e-9,"Position and animation use different interpolation");
 const auto previousEnd=b;domain.stepMovement(.02,Tool::Select,{});const auto nextStart=fishPose(*domain.fish(id),0);
 check(std::abs(previousEnd.phase-nextStart.phase)<1e-9&&std::abs(previousEnd.facing-nextStart.facing)<1e-9,"Animation jumps at the next simulation step");
 domain.clearTransient();const auto reset=fishPose(*domain.fish(id),0);check(reset.position.x==domain.fish(id)->position.x&&reset.position.y==domain.fish(id)->position.y,"Resume shows stale movement history");
 std::cout<<"PASS continuous fish position, fins, pitch and turns, including transient reset\n";
}
}
int main(int argc,char** argv){try{if(argc!=2)throw std::runtime_error("Usage: aquarium_rendering_tests ASSETS");checkMotion(argv[1]);checkFrames(argv[1]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
