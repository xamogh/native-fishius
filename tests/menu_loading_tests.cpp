#include "aquarium/loading.hpp"
#include "aquarium/hud_tanks.hpp"
#include <fstream>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void checkOpaqueShop(Canvas& canvas,const Domain& domain,const ShopLayout& layout){
 using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
 for(const auto category:{ShopCategory::Fish,ShopCategory::Tanks}){
  auto draw=[&](Color background){
   canvas.begin();canvas.fill(layout.page,background);paintShop(canvas,domain,layout,ShopState{category});
   return Surface(SDL_RenderReadPixels(canvas.renderer(),nullptr),SDL_DestroySurface);
  };
  const auto red=draw({255,0,0}),blue=draw({0,0,255});
  check(red&&blue&&red->w==blue->w&&red->h==blue->h&&red->format==blue->format,"Cannot inspect Shop opacity");
  for(int y=0;y<red->h;++y)check(std::memcmp(static_cast<const Uint8*>(red->pixels)+y*red->pitch,static_cast<const Uint8*>(blue->pixels)+y*blue->pitch,std::size_t(red->w)*SDL_BYTESPERPIXEL(red->format))==0,"Shop exposes the aquarium that rendering skips");
 }
}
void checkMenus(const std::filesystem::path& assets,const Content& content,PreviewViewport viewport,bool progressed){
 Domain domain(content,0);
 if(progressed){
  domain.fixture("shop");auto state=domain.state();
  state.tanks={{{1},20},{{2},15},{{3},15},{{4},15},{{5},20}};state.activeTank={3};domain.install(state);
 }
 const auto initial=encode(domain.state());
 Canvas canvas(assets,viewport.width,viewport.height,true);canvas.previewViewport(viewport);canvas.begin();
 const auto empty=canvas.cacheStats();
 check(!prepareMenus(canvas,domain,[](float,std::string_view){return false;}),"Cancelled menu preparation continued");
 check(canvas.cacheStats()==empty,"Cancelled menu preparation loaded artwork or text");
 float previous=-1;int updates=0;
 check(prepareMenus(canvas,domain,[&](float progress,std::string_view stage){
  check(progress>=0&&progress<=1&&progress>previous,"Loading progress is not increasing");
  check(!stage.empty(),"Loading stage has no label");previous=progress;++updates;
  // Match startup: the menu frame is flushed, then covered by the loader.
  canvas.loadingScreen(progress,stage,0,true,true);SDL_FlushRenderer(canvas.renderer());return true;
 }),"Menu preparation failed");
 check(previous==1&&updates>1,"Menu preparation did not finish or report progress");
 auto scene=[&]{canvas.begin();canvas.scene(domain,0,0,Tool::Select,{});};
 auto hud=[&]{return layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());};
 auto page=[&]{return layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());};
 scene();paintHud(canvas,domain,hud());SDL_FlushRenderer(canvas.renderer());
 canvas.loadingScreen(1,"Your reef is ready",0,true,true);SDL_FlushRenderer(canvas.renderer());
 const auto ready=canvas.cacheStats();
 std::cout<<"Prepared text cache: "<<ready.textBytes/(1024*1024)<<" MiB at "<<viewport.width<<'x'<<viewport.height<<" @"<<viewport.pixelRatio<<"x"<<std::endl;
 check(ready.textureLoads>0&&ready.textRasterizations>0,"Preparation did not populate the caches");
 checkOpaqueShop(canvas,domain,page());
 for(int pass=0;pass<2;++pass){
  // Check both entry points and the locked-tank route, including returning
  // to the aquarium. No cache misses should occur after the ready screen.
  scene();paintShop(canvas,domain,page(),ShopState{});SDL_FlushRenderer(canvas.renderer());
  check(canvas.cacheStats()==ready,"Opening Shop decoded artwork or rasterized text after startup");
  scene();paintHud(canvas,domain,hud());TankSwitcherState switcher;switcher.open=true;
  paintTankSwitcher(canvas,domain,hud(),switcher,{-1,-1});SDL_FlushRenderer(canvas.renderer());
  check(canvas.cacheStats()==ready,"Opening Tank decoded artwork or rasterized text after startup");
  scene();paintShop(canvas,domain,page(),ShopState{ShopCategory::Tanks});SDL_FlushRenderer(canvas.renderer());
  check(canvas.cacheStats()==ready,"Opening Tank Shop decoded artwork or rasterized text after startup");
  scene();paintHud(canvas,domain,hud());SDL_FlushRenderer(canvas.renderer());
  check(canvas.cacheStats()==ready,"Closing a menu lost cached aquarium resources");
  for(const auto category:{ShopCategory::Fish,ShopCategory::Plants,ShopCategory::Decorations,ShopCategory::Treasure,ShopCategory::Environment}){
   ShopState state{category};const float limit=shopScrollLimit(state,shopItems(domain,state).size());
   // Overlapping pages cover every card; fractional positions exercise clips.
   for(float scroll=0;;scroll=std::min(limit,scroll==0?.5f:scroll+float(page().visibleCards-1))){
    state.scroll=scroll;canvas.begin();paintShop(canvas,domain,page(),state);SDL_FlushRenderer(canvas.renderer());
    check(canvas.cacheStats()==ready,"Scrolling loads artwork or rasterizes card text after preparation");
    if(scroll>=limit)break;
   }
  }
 }
 check(encode(domain.state())==initial,"Menu preparation changed game state");
 std::cout<<"PASS cached menus at "<<viewport.width<<'x'<<viewport.height<<" @"<<viewport.pixelRatio<<"x, "<<(progressed?"owned tanks":"new game")<<'\n';
}
}
int main(int argc,char** argv){try{
 const std::filesystem::path assets=argc>1?argv[1]:"assets";std::ifstream input(assets/"content.json");const auto content=aq::Content::fromJson(aq::Json::parse(input));
 checkMenus(assets,content,{1088,635,1,{}},false);
 checkMenus(assets,content,{852,393,3,{59,0,59,21}},false);
 checkMenus(assets,content,{1210,834,2,{}},true);
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
