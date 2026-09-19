#include "aquarium/hud.hpp"
#include "aquarium/hud_decor_placement.hpp"
#include <cmath>
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
void decorAlignment(Canvas& canvas,const SDL_Surface& art,const Content& content){
 // Derive the image's cover crop from the actual asset, independently of the
 // decoration transform. These source-image anchors must survive resizing.
 const double scale=std::max(canvas.width()/art.w,canvas.height()/art.h);
 const double width=art.w*scale,height=art.h*scale;
 const double left=(canvas.width()-width)*.5,top=canvas.height()-height;
 check(art.w*940==art.h*1672,"Background aspect ratio differs from the shared scene canvas");
 for(const auto anchor:{WorldPoint{230,380},WorldPoint{544,470},WorldPoint{820,570}}){
  const auto r=canvas.decorRect(*content.findDecor("CD-01"),anchor);
  check(std::abs((r.x+r.w*.5-left)/width-anchor.x/waterWidth)<.00001,
   "Decoration slides horizontally against the background when the aspect ratio changes");
  check(std::abs((r.y+r.h-top)/height-anchor.y/tankHeight)<.00001,
   "Decoration slides vertically against the background when the aspect ratio changes");
  const SDL_FPoint expected{float(left+anchor.x/waterWidth*width),float(top+anchor.y/tankHeight*height)};
  const auto restored=canvas.decorProjection().toWorld(expected);
  check(std::hypot(restored.x-anchor.x,restored.y-anchor.y)<.001,"Decoration input does not invert the background crop");
 }
}
void savedLayout(const Content& content,const std::filesystem::path& assets,const std::filesystem::path& output,bool software){
 Domain domain(content);domain.fixture("performance");auto state=domain.state();state.fish.clear();
 state.settings.reducedMotion=true;
 state.decor={{501,"CP-02",{1},{265,480}},{502,"CD-01",{1},{475,570}},
  {503,"CP-01",{1},{630,520}},{504,"CD-02",{1},{790,540}},
  {505,"CD-01",{1},{30,610}},{506,"CD-01",{1},{1058,610}}};
 state.nextDecorId=507;state.decorOwned={"CP-01","CP-02","CD-01","CD-02"};
 // Real save validation, followed by several viewport changes on one Canvas.
 domain.install(decodeAndValidate(encode(state),content));
 Canvas canvas(assets,1338,752,software);
 const std::array viewports{PreviewViewport{1338,752,1,{}},PreviewViewport{852,393,3,{59,0,59,21}},
  PreviewViewport{1024,768,2,{}},PreviewViewport{1180,820,2,{}},PreviewViewport{667,375,2,{}},PreviewViewport{1338,752,1,{}}};
 for(const auto& style:environmentCatalog()){
  domain.execute({.action=Action::PurchaseEnvironment,.tank={1},.key=style.id});
  domain.execute({.action=Action::EquipEnvironment,.tank={1},.key=style.id});
  const auto saved=encode(domain.state());
  Surface art(IMG_Load((assets/style.asset).string().c_str()),SDL_DestroySurface);
  std::vector<Rect> reference;
  for(const auto viewport:viewports){
   canvas.previewViewport(viewport);canvas.begin();decorAlignment(canvas,*art,content);
   const double scale=std::max(canvas.width()/art->w,canvas.height()/art->h);
   std::vector<Rect> sizes;
   for(const auto& item:domain.state().decor){
    const auto r=canvas.decorRect(*content.findDecor(item.kind),item.position,item.sizeMul);
    sizes.push_back({0,0,float(r.w/scale),float(r.h/scale)});
   }
   if(reference.empty())reference=sizes;
   else for(std::size_t i=0;i<sizes.size();++i)
    check(std::abs(sizes[i].w-reference[i].w)<.001&&std::abs(sizes[i].h-reference[i].h)<.001,
     "Decoration size changes relative to the artwork on another device");
   canvas.scene(domain,1,0,Tool::Select,{},false);
   check(encode(domain.state())==saved,"Resizing or rendering moves saved decorations");
   check(canvas.capture(output/(std::string(software?"aligned-software-":"aligned-native-")+style.id+"-"+std::to_string(viewport.width)+".png")),"Cannot capture the shared saved layout");
   // New drops fit the visible crop, including perspective growth at an edge.
   const auto view=canvas.decorProjection();
   for(const auto& def:content.decorations)for(double size:{.6,1.,1.7})
    for(const SDL_FPoint edge:{SDL_FPoint{0,0},{canvas.width(),0},{0,canvas.height()},{canvas.width(),canvas.height()}}){
     const auto placed=view.place(def,view.toWorld(edge),size);const auto r=canvas.decorRect(def,placed,size);
     if(r.w<=canvas.width())check(r.x>=-.01&&r.x+r.w<=canvas.width()+.01,"Drop can hide decor beside the visible crop");
     if(r.h<=canvas.height())check(r.y>=-.01&&r.y+r.h<=canvas.height()+.01,"Drop can hide decor above or below the visible crop");
     check(r.x+r.w>0&&r.x<canvas.width()&&r.y+r.h>0&&r.y<canvas.height(),"Oversized decor becomes unreachable");
    }
   // Tap the same independent source-art landmark through the real placement
   // event path, then reload it. Safe insets must only affect UI, not anchors.
   Session session(content,"/tmp/aquarium-alignment-unused.json",1000,true);DecorPlacement placement;
   check(bool(startDecorPlacement(session.domain(),placement,"CD-01")),"Cannot start an alignment placement");
   const auto layout=layoutDecorPlacement(canvas,session.domain(),placement);
   const WorldPoint landmark{waterWidth*.58,tankHeight*.82};
   const SDL_FPoint tap{float((canvas.width()-art->w*scale)*.5+art->w*scale*.58),float(canvas.height()-art->h*scale+art->h*scale*.82)};
   SDL_Event event{};event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;event.button.button=SDL_BUTTON_LEFT;
   decorPlacementEvent(session,placement,layout,event,tap);event.type=SDL_EVENT_MOUSE_BUTTON_UP;
   decorPlacementEvent(session,placement,layout,event,tap);
   check(std::hypot(placement.preview.position.x-landmark.x,placement.preview.position.y-landmark.y)<.001,"Tapping the same landmark saves different coordinates on another device");
   check(bool(confirmDecorPlacement(session,placement)),"Cannot save the alignment placement");
   const auto loaded=decodeAndValidate(encode(session.domain().state()),content);
   check(std::hypot(loaded.decor.back().position.x-landmark.x,loaded.decor.back().position.y-landmark.y)<.001,"Save reload changes the background anchor");
  }
 }
 std::cout<<"PASS "<<(software?"software":"native")<<" saved decor anchors, scale, input and visible edge placement across phones and tablets\n";
}
}
int main(int argc,char** argv){try{
 const std::filesystem::path assets=argc>1?argv[1]:"assets";
 const std::filesystem::path output=argc>2?argv[2]:"/tmp/aquarium-environment";std::filesystem::create_directories(output);
 std::ifstream input(assets/"content.json");const auto content=Content::fromJson(Json::parse(input));
 for(const auto [w,h]:std::array{std::pair{1338,752},std::pair{1024,768},std::pair{852,393}}){
  Domain domain(content);domain.fixture("performance");auto state=domain.state();state.fish.clear();state.decor.clear();domain.install(state);
  Canvas canvas(assets,w,h,true);
  Surface reference(nullptr,SDL_DestroySurface);
  for(const auto& style:environmentCatalog()){
   Surface art(IMG_Load((assets/style.asset).string().c_str()),SDL_DestroySurface);check(bool(art),"Missing background artwork");
   check(art->w>=1600&&art->h>=900,"Background artwork resolution too small");
   decorAlignment(canvas,*art,content);
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
 savedLayout(content,assets,output,true);
 if(argc>3&&std::string_view(argv[3])=="--native")savedLayout(content,assets,output,false);
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
