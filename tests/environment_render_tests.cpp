#include "aquarium/hud.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace aq;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
Surface pixels(Canvas& canvas){return {SDL_RenderReadPixels(canvas.renderer(),nullptr),SDL_DestroySurface};}
Uint32 pixel(const SDL_Surface& surface,int x,int y){Uint32 value{};std::memcpy(&value,static_cast<const Uint8*>(surface.pixels)+y*surface.pitch+x*4,4);return value;}
int differences(const SDL_Surface& a,const SDL_Surface& b){
 int count=0;for(int y=0;y<a.h;y+=8)for(int x=0;x<a.w;x+=8)if(pixel(a,x,y)!=pixel(b,x,y))++count;return count;
}
}
int main(int argc,char** argv){try{
 const std::filesystem::path assets=argc>1?argv[1]:"assets";
 const std::filesystem::path output=argc>2?argv[2]:"/tmp/aquarium-environment";std::filesystem::create_directories(output);
 std::ifstream input(assets/"content.json");const auto content=Content::fromJson(Json::parse(input));
 for(const auto [w,h]:std::array{std::pair{1338,752},std::pair{1024,768},std::pair{852,393}}){
  Domain domain(content);domain.fixture("performance");auto state=domain.state();state.fish.clear();state.companions.clear();state.decor.clear();domain.install(state);
  Canvas canvas(assets,w,h,true);
  Surface reference(nullptr,SDL_DestroySurface);
  for(const auto& style:environmentCatalog()){
   Surface art(IMG_Load((assets/style.asset).string().c_str()),SDL_DestroySurface);check(bool(art),"Missing background artwork");
   check(art->w>=1600&&art->h>=900,"Background artwork resolution too small");
   for(int y=0;y<art->h;y+=64)for(int x=0;x<art->w;x+=64){Uint8 r,g,b,a;SDL_ReadSurfacePixel(art.get(),x,y,&r,&g,&b,&a);check(a==255,"Complete background has transparent gaps");}
   check(bool(domain.execute({.action=Action::PurchaseEnvironment,.tank={1},.key=style.id})),"Background purchase failed");
   check(bool(domain.execute({.action=Action::EquipEnvironment,.tank={1},.key=style.id})),"Background selection failed");
   canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false);
   auto current=pixels(canvas);check(bool(current),"Readback failed");
   if(style.price==0)reference=std::move(current);
   else check(differences(*reference,*current)>w*h/128,"Equipping a new background has no visible effect");
   check(canvas.capture(output/(std::to_string(w)+"-"+style.id+".png")),"Capture failed");
  }
  check(bool(domain.execute({.action=Action::SwitchTank,.tank={2}})),"Cannot switch tanks");
  canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false);auto other=pixels(canvas);
  check(differences(*reference,*other)==0,"Equipping a background changed another tank");
  canvas.begin();paintShop(canvas,domain,layoutShop(canvas.width(),canvas.height(),{},44),{ShopCategory::Environment,0});
  check(canvas.capture(output/(std::to_string(w)+"-background-shop.png")),"Shop capture failed");
 }
 std::cout<<"PASS complete backgrounds on phone, desktop and tablet; opacity, selection, and independent tanks\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
