#include "aquarium/loading.hpp"
#include "aquarium/hud_tanks.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
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
 check(ready.textureLoads>0&&ready.textRasterizations>0,"Preparation did not populate the caches");
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
