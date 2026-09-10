#include <cmath>
#include <cstdlib>
#include "aquarium/view.hpp"
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace {
aq::Millis wallNow(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
struct Options {int w{1088},h{635},frames{};bool software{},fresh{},still{};double warp{1};std::string fixture;std::filesystem::path assets,save,capture,report,sequence;};
Options options(int argc,char** argv){
 Options o;for(int i=1;i<argc;++i){std::string s=argv[i];auto arg=[&](){if(i+1>=argc)throw std::runtime_error("Missing value for "+s);return std::string(argv[++i]);};if(s=="--width")o.w=std::stoi(arg());else if(s=="--height")o.h=std::stoi(arg());else if(s=="--frames")o.frames=std::stoi(arg());else if(s=="--warp")o.warp=std::stod(arg());else if(s=="--fixture")o.fixture=arg();else if(s=="--assets")o.assets=arg();else if(s=="--save")o.save=arg();else if(s=="--capture")o.capture=arg();else if(s=="--report")o.report=arg();else if(s=="--sequence")o.sequence=arg();else if(s=="--software")o.software=true;else if(s=="--fresh")o.fresh=true;else if(s=="--still")o.still=true;else if(s=="--help"){std::cout<<"FishX Aquarium\n--assets PATH --save PATH --width N --height N\n--fixture aquarium|shop|tanks|collection|settings|inventory|care|performance\n--frames N --capture PNG --sequence DIR --report JSON --software --warp N --fresh --still\nReview fixtures and --fresh never overwrite your ordinary save.\n";std::exit(0);}else throw std::runtime_error("Unknown option "+s);}
 if(o.w<320||o.h<240||o.w>8192||o.h>8192||!std::isfinite(o.warp)||o.warp<1||o.warp>10000)throw std::runtime_error("Invalid viewport or time scale");
 return o;
}
}
int main(int argc,char** argv){
 try{
  auto o=options(argc,argv);if(o.assets.empty()){const char* base=SDL_GetBasePath();o.assets=base?std::filesystem::path(base)/"assets":std::filesystem::path("assets");if(!std::filesystem::exists(o.assets/"content.json"))o.assets="assets";}
  std::ifstream contentFile(o.assets/"content.json");if(!contentFile)throw std::runtime_error("Missing assets/content.json. Run the documented content import.");auto content=aq::Content::fromJson(aq::Json::parse(contentFile));
  for(auto& s:content.species){s.artReady=std::filesystem::exists(o.assets/s.asset);if(!s.artReady)throw std::runtime_error("Required species art missing: "+s.asset);}
  aq::Canvas canvas(o.assets,o.w,o.h,o.software);if(o.save.empty()){char* p=SDL_GetPrefPath("Pixmot","FishX");if(!p)throw std::runtime_error(SDL_GetError());o.save=std::filesystem::path(p)/"save.json";SDL_free(p);}
  const bool review=!o.fixture.empty()||o.fresh;aq::Session session(std::move(content),o.save,wallNow(),review);aq::View view(canvas,session);if(!o.fixture.empty()){session.domain().fixture(o.fixture);view.fixture(o.fixture);}
  if(!o.sequence.empty())std::filesystem::create_directories(o.sequence);bool running=true;std::uint64_t previous=SDL_GetTicksNS();double presentation=0,careFraction=0;int frame=0;std::vector<double> timings;timings.reserve(10000);
  while(running){
   const auto frameStart=SDL_GetTicksNS();double elapsed=double(frameStart-previous)/1e9;previous=frameStart;presentation+=elapsed;
   SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_EVENT_QUIT)running=false;view.event(e,presentation);if(e.type==SDL_EVENT_WILL_ENTER_BACKGROUND){view.cancelGesture();session.suspend(wallNow());}else if(e.type==SDL_EVENT_DID_ENTER_FOREGROUND)session.resume(wallNow());else if(e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET){view.cancelGesture();}}
   if(session.paused()){SDL_Delay(50);continue;}
   careFraction+=elapsed*1000.*o.warp;auto careMs=static_cast<aq::Millis>(careFraction);careFraction-=double(careMs);if(!o.still)session.update(careMs,wallNow(),view.tool(),view.held());view.render(presentation);
   if(!o.sequence.empty()){std::string number=std::to_string(frame);number=std::string(6-number.size(),'0')+number;if(!canvas.capture(o.sequence/(number+".png")))throw std::runtime_error("Frame capture failed");}
   if(o.frames>0&&frame+1>=o.frames){if(!o.capture.empty()&&!canvas.capture(o.capture))throw std::runtime_error("Screenshot capture failed");running=false;}
   const auto beforePresent=SDL_GetTicksNS();timings.push_back(double(beforePresent-frameStart)/1e6);if(timings.size()>36000)timings.erase(timings.begin(),timings.begin()+18000);canvas.present();++frame;
  }
  session.checkpoint(wallNow());if(!o.report.empty()){auto sorted=timings;std::sort(sorted.begin(),sorted.end());auto percentile=[&](double p){return sorted.empty()?0:sorted[std::min(sorted.size()-1,static_cast<std::size_t>(p*double(sorted.size()-1)))];};aq::Json report={{"renderer",SDL_GetRendererName(canvas.renderer())},{"frames",frame},{"scenario",o.fixture},{"window_width",o.w},{"window_height",o.h},{"metric","CPU update and draw submission milliseconds; excludes vsync wait; includes capture when enabled"},{"p50_ms",percentile(.5)},{"p95_ms",percentile(.95)},{"p99_ms",percentile(.99)},{"max_ms",sorted.empty()?0:sorted.back()},{"mobile_device_tested",false}};std::ofstream out(o.report);out<<report.dump(2);}
  return 0;
 }catch(const std::exception& e){std::cerr<<"FishX: "<<e.what()<<'\n';SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"FishX could not continue",e.what(),nullptr);return 1;}
}
