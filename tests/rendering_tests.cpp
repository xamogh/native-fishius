#include "aquarium/canvas.hpp"
#include "aquarium/hud_placement.hpp"
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
int differingPixels(const SDL_Surface& a,const SDL_Surface& b,int tolerance=0){
 check(a.w==b.w&&a.h==b.h,"Compared frames differ in size");int count=0;
 for(int y=0;y<a.h;++y)for(int x=0;x<a.w;++x){const auto* p=pixel(a,x,y);const auto* q=pixel(b,x,y);int difference=0;for(int k=0;k<4;++k)difference=std::max(difference,std::abs(int(p[k])-int(q[k])));count+=difference>tolerance;}
 return count;
}
void checkPremiumFish(const std::filesystem::path& assets){
 std::ifstream input(assets/"content.json");const auto content=Content::fromJson(Json::parse(input));Domain domain(content,0);
 Canvas canvas(assets,852,393,true);auto state=domain.state();auto fish=state.fish.front();
 fish.id={51};fish.position={544,320};fish.egg=false;fish.age=3;fish.motion={};fish.motion.direction=-1;
 auto frame=[&](const Fish& item,bool reduced,double time){
  canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,255});
  canvas.fish(*content.find(item.species),item,1,false,{0,0,0,0},false,reduced,Care::Fed,time);
  return read(canvas.renderer());
 };
 fish.species="koi";auto first=frame(fish,false,0),repeated=frame(fish,false,20);
 check(differingPixels(*first,*repeated)==0,"Koi pattern changes without a change to fish identity");
 for(std::uint64_t id=52;id<60;++id){auto other=fish;other.id={id};auto variant=frame(other,false,0);check(differingPixels(*first,*variant,3)>30,"Individual Koi share an indistinguishable painted pattern");}
 fish.purchase=purchaseQuote(content,*content.find("koi"),5);fish.age=4;fish.growthMs=fish.purchase.durationMs;fish.lastFedAt=0;
 state.fish={fish};state.nextFishId=52;first=frame(fish,false,0);const auto restored=decodeAndValidate(encode(state),content);auto reloaded=frame(restored.fish.front(),false,0);
 check(differingPixels(*first,*reloaded)==0,"Koi pattern changes after saving and loading");
 auto still=frame(fish,true,0),laterStill=frame(fish,true,47);
 check(differingPixels(*still,*laterStill)==0,"Reduced-motion Koi pattern animates");
 fish.species="flashlightFish";auto glow=frame(fish,false,0),dim=frame(fish,false,3),cycle=frame(fish,false,6);
 check(differingPixels(*glow,*dim,3)>15,"Flashlight Fish has no timed cheek glow");
 check(differingPixels(*glow,*cycle,1)==0,"Flashlight Fish glow jumps at the cycle boundary");
 for(double time=0;time<6;time+=.25){auto before=frame(fish,false,time),after=frame(fish,false,time+1./60);check(differingPixels(*before,*after,3)==0,"Flashlight Fish light changes abruptly between frames");}
 still=frame(fish,true,0);laterStill=frame(fish,true,3);
 check(differingPixels(*still,*laterStill)==0,"Reduced motion leaves the Flashlight Fish pulse active");
 fish.dead=true;still=frame(fish,false,0);laterStill=frame(fish,false,3);
 check(differingPixels(*still,*laterStill)==0,"Dead Flashlight Fish keeps pulsing");
 std::cout<<"PASS persistent individual Koi patterns, smooth cheek glow and reduced-motion stills\n";
}
void checkDecorTransparency(const std::filesystem::path& assets,bool software){
 Canvas canvas(assets,852,393,software);
 const auto directory=std::filesystem::temp_directory_path()/"aquarium-rendering-tests";std::filesystem::create_directories(directory);
 Surface art(SDL_CreateSurface(64,64,SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
 check(bool(art),"Cannot create decor transparency fixture");
 for(int y=0;y<64;++y)for(int x=0;x<64;++x){
  auto* p=static_cast<Uint8*>(art->pixels)+y*art->pitch+x*4;
  p[0]=80;p[1]=120;p[2]=40;p[3]=x>=16&&x<48&&y>=16&&y<48?Uint8(x<32?128:255):0;
 }
 const auto path=directory/"decor-transparency.png";check(IMG_SavePNG(art.get(),path.string().c_str()),"Cannot save decor transparency fixture");
 DecorDef def;def.asset=path.string();def.art=Json::object();
 const Color background{30,60,90,255};
 for(const auto viewport:{PreviewViewport{852,393,1,{}},PreviewViewport{1223,716,2,{}}}){
  canvas.previewViewport(viewport);canvas.begin();
  for(double anchorY:{320.,tankHeight})for(float side:{24.f,48.f,128.f,512.f}){
   const WorldPoint anchor{waterWidth*.5,anchorY};
   const float scale=canvas.decorProjection().decorUnit()*float(decorScale(anchor));
   def.width=def.height=side/scale;
   const Decoration item{1,"test",{1},anchor};
   auto draw=[&](float opacity,bool highlight=false){
    canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},background);canvas.decoration(def,item,0,false,opacity,nullptr,true,highlight);
    return read(canvas.renderer());
   };
   canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},background);auto clean=read(canvas.renderer());
   auto invisible=draw(0);
   check(differingPixels(*clean,*invisible,1)==0,"Zero-opacity decor adds a rectangular glow");
   auto invisiblePreview=draw(0,true);
   check(differingPixels(*clean,*invisiblePreview,1)==0,"Zero-opacity placement highlight changes the tank");
   auto visible=draw(1);
   auto highlighted=draw(1,true);
   check(differingPixels(*visible,*highlighted,3)>0,"Selected decor has no visible glow");
   const auto bounds=canvas.decorRect(def,anchor);const float sx=float(visible->w)/canvas.width(),sy=float(visible->h)/canvas.height();
   for(float fy:{.08f,.92f})for(float fx:{.08f,.92f}){
    const int x=int((bounds.x+fx*bounds.w)*sx),y=int((bounds.y+fy*bounds.h)*sy);
    if(x<0||y<0||x>=visible->w||y>=visible->h)continue;
    const auto* actual=pixel(*visible,x,y);const auto* expected=pixel(*clean,x,y);
    for(int k=0;k<4;++k)check(std::abs(int(actual[k])-int(expected[k]))<=1,"Transparent decor padding brightens the aquarium");
    if(side>=128){const auto* glowing=pixel(*highlighted,x,y);for(int k=0;k<4;++k)check(std::abs(int(glowing[k])-int(expected[k]))<=1,"Selection glow fills the transparent decor rectangle");}
   }
   // Screen tint must respect both semi-transparent edges and the depth amount.
   const float haze=float(decorHaze(anchor));
   for(float alpha:{.5f,1.f}){
    auto frame=draw(alpha);
    for(float fx:{.4f,.65f}){
     const int x=int((bounds.x+fx*bounds.w)*sx),y=int((bounds.y+.5f*bounds.h)*sy);
     if(x<0||y<0||x>=frame->w||y>=frame->h)continue;
     const float coverage=(fx<.5f?128/255.f:1.f)*alpha;
     const std::array<float,3> ink{80,120,40},base{30,60,90},tint{95/255.f,196/255.f,220/255.f};
     const auto* actual=pixel(*frame,x,y);
     for(int k=0;k<3;++k){const float sprite=ink[k]*coverage+base[k]*(1-coverage),amount=tint[k]*coverage*haze;
      const float expected=sprite*(1-amount)+255*amount;
      // Software rendering rounds modulation and each of the three passes to eight bits.
      if(std::abs(float(actual[k])-expected)>6)std::cerr<<"Decor tint: "<<SDL_GetRendererName(canvas.renderer())<<" size="<<side<<" density="<<viewport.pixelRatio<<" depth="<<anchorY<<" alpha="<<alpha<<" coverage="<<coverage<<" channel="<<k<<" actual="<<int(actual[k])<<" expected="<<expected<<'\n';
      check(std::abs(float(actual[k])-expected)<=6,"Decor depth tint ignores texture or preview opacity");
     }
    }
   }
  }
 }
 // Capture the reported plant at both full texture and reduced texture sizes.
 std::ifstream input(assets/"content.json");Domain domain(Content::fromJson(Json::parse(input)),0);
 auto state=domain.state();state.fish.clear();state.decor={
  {1,"CP-02",{1},{316,470}}, {2,"CP-02",{1},{535,625}}
 };state.nextDecorId=3;state.decorOwned={"CP-02"};domain.install(std::move(state));
 for(int density:{1,2}){
  canvas.previewViewport(PreviewViewport{1223,716,density,{}});canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false);
  check(canvas.capture(directory/(std::string(software?"decor-software-":"decor-native-")+std::to_string(density)+"x.png")),"Cannot capture the plant transparency check");
  canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false,{},nullptr,1);
  check(canvas.capture(directory/(std::string(software?"decor-glow-software-":"decor-glow-native-")+std::to_string(density)+"x.png")),"Cannot capture the selected plant glow");
 }
 std::cout<<"PASS "<<SDL_GetRendererName(canvas.renderer())<<" decor glow, transparency and depth tint at full size, reduced sizes and Retina density\n";
}
void checkPlacementFocus(const std::filesystem::path& assets,bool software){
 const auto directory=std::filesystem::temp_directory_path()/"aquarium-rendering-tests";std::filesystem::create_directories(directory);
 Surface art(SDL_CreateSurface(64,64,SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);check(bool(art),"Cannot create focus fixture");
 for(int y=0;y<64;++y)for(int x=0;x<64;++x){
  auto* p=static_cast<Uint8*>(art->pixels)+y*art->pitch+x*4;
  const bool stripe=(x/4)%2==0;p[0]=stripe?235:25;p[1]=stripe?210:100;p[2]=stripe?55:230;p[3]=x>=8&&x<56&&y>=8&&y<56?255:0;
 }
 const auto path=directory/"focus-preview.png";check(IMG_SavePNG(art.get(),path.string().c_str()),"Cannot save focus fixture");
 for(int y=0;y<64;++y)for(int x=0;x<64;++x){auto* p=static_cast<Uint8*>(art->pixels)+y*art->pitch+x*4;p[0]=220;p[1]=70;p[2]=40;}
 const auto occluder=directory/"focus-occluder.png";check(IMG_SavePNG(art.get(),occluder.string().c_str()),"Cannot save overlapping decor fixture");
 std::ifstream input(assets/"content.json");auto content=Content::fromJson(Json::parse(input));
 // Deliberately oppose catalog layers to physical depth.
 for(auto& def:content.decorations)if(def.id=="CD-01"){def.asset=path.string();def.width=def.height=2.5;def.layer="Foreground";}
 for(auto& def:content.decorations)if(def.id=="CP-01"){def.asset=occluder.string();def.width=def.height=2.5;def.layer="Background";}
 Domain domain(content,0);auto state=domain.state();state.fish.resize(2);state.fish.back().id={20};state.nextFishId=21;
 state.settings.reducedMotion=true;
 state.decor={{1,"CD-01",state.activeTank,{544,420}},{2,"CP-01",state.activeTank,{544,455}},
              {3,"CP-01",state.activeTank,{220,420}}};
 state.nextDecorId=4;state.decorOwned={"CD-01","CP-01"};
 Decoration preview=state.decor.front();const auto& def=*content.findDecor(preview.kind);
 Canvas canvas(assets,852,393,software);
 for(const auto viewport:{PreviewViewport{852,393,1,{}},PreviewViewport{1024,768,2,{}},PreviewViewport{390,844,1,{}}}){
  check(SDL_SetWindowSize(canvas.window(),viewport.width,viewport.height),"Cannot resize placement focus window");SDL_SyncWindow(canvas.window());SDL_PumpEvents();
  canvas.previewViewport(viewport);canvas.begin();preview=state.decor.front();const auto bounds=canvas.decorRect(def,preview.position);
  auto& fish=state.fish.front();fish.species="guppy";fish.egg=false;fish.age=4;fish.motion={};fish.motion.direction=1;
  fish.position=canvas.toWorld(bounds.x+bounds.w*.5f,bounds.y+bounds.h*.5f);
  state.fish.back()=fish;state.fish.back().id={20};state.fish.back().position={150,100};domain.install(state);
  auto frame=[&](bool focus){canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false,{},&preview,0,focus);return read(canvas.renderer());};
  auto normal=frame(false),focused=frame(true),repeated=frame(true),restored=frame(false);
  check(differingPixels(*normal,*focused)>100,"Selecting a covered item has no visible effect");
  check(differingPixels(*focused,*repeated)==0,"Selection opacity accumulates between frames");
  check(differingPixels(*normal,*restored)==0,"Leaving placement keeps translucent items or a stale highlight");
  // Independently compose the expected layers. The selected sprite stays below
  // the nearer sprite and fish, which retain 22 percent opacity while editing.
  canvas.begin();canvas.environment({0,0,canvas.width(),canvas.height()});
  canvas.decoration(*content.findDecor("CP-01"),state.decor[2],0);
  canvas.decoration(def,preview,0,false,1,nullptr,true,true);
  canvas.decoration(*content.findDecor("CP-01"),state.decor[1],0,false,.22f);
  canvas.fish(*content.find(fish.species),fish,1,false,{0,0,0,0},false,true,Care::Fed,0,.22f);
  canvas.fish(*content.find(fish.species),state.fish.back(),1,false,{0,0,0,0},false,true,Care::Fed,0);
  auto expected=read(canvas.renderer());
  check(differingPixels(*focused,*expected,1)==0,"Selection blurs the scene, changes depth order, or fades unrelated objects");
  // The normal view must use physical depth too, regardless of catalog labels.
  canvas.begin();canvas.environment({0,0,canvas.width(),canvas.height()});
  canvas.decoration(*content.findDecor("CP-01"),state.decor[2],0);
  canvas.decoration(def,preview,0);
  canvas.decoration(*content.findDecor("CP-01"),state.decor[1],0);
  for(const auto& item:state.fish)canvas.fish(*content.find(item.species),item,1,false,{0,0,0,0},false,true,Care::Fed,0);
  auto layered=read(canvas.renderer());
  check(differingPixels(*normal,*layered,1)==0,"Placed objects still follow catalog layers instead of depth");
  // Moving forward must restore the previous occluder to full opacity behind it.
  preview.position.y=490;
  auto forward=frame(true);
  canvas.begin();canvas.environment({0,0,canvas.width(),canvas.height()});
  canvas.decoration(*content.findDecor("CP-01"),state.decor[2],0);
  canvas.decoration(*content.findDecor("CP-01"),state.decor[1],0);
  canvas.decoration(def,preview,0,false,1,nullptr,true,true);
  canvas.fish(*content.find(fish.species),fish,1,false,{0,0,0,0},false,true,Care::Fed,0,.22f);
  canvas.fish(*content.find(fish.species),state.fish.back(),1,false,{0,0,0,0},false,true,Care::Fed,0);
  expected=read(canvas.renderer());
  check(differingPixels(*forward,*expected,1)==0,"Crossing another item's depth fails to restore its opacity and layer");
  // Equal-depth saved copies and their previews keep the same tie order.
  preview=state.decor.front();preview.position=state.decor[1].position;
  auto equalState=state;equalState.decor[0]=preview;domain.install(equalState);
  auto equalPreview=frame(false);
  canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false);auto equalSaved=read(canvas.renderer());
  check(differingPixels(*equalPreview,*equalSaved)==0,"Selecting an equal-depth copy changes its draw order");
  domain.install(state);preview=state.decor.front();frame(true);
  check(canvas.capture(directory/(std::string(software?"focus-software-":"focus-native-")+std::to_string(viewport.width)+".png")),"Cannot capture placement focus");
 }
 std::cout<<"PASS "<<SDL_GetRendererName(canvas.renderer())<<" sharp placement, real depth, selective transparency and stable preview order across sizes and densities\n";
}
void checkDecorBaseLayering(const std::filesystem::path& assets,bool software){
 std::ifstream input(assets/"content.json");const auto content=Content::fromJson(Json::parse(input));
 Domain domain(content,0);auto state=domain.state();state.fish.clear();state.settings.reducedMotion=true;
 state.decor={{501,"CP-08",state.activeTank,{460,575}},{502,"CD-09",state.activeTank,{535,574}}};
 state.decorOwned={"CP-08","CD-09"};state.nextDecorId=503;
 const auto directory=std::filesystem::temp_directory_path()/"aquarium-rendering-tests";std::filesystem::create_directories(directory);
 Canvas canvas(assets,852,393,software);
 for(const auto viewport:{PreviewViewport{852,393,1,{}},PreviewViewport{390,844,1,{}},PreviewViewport{1024,768,2,{}}}){
  canvas.previewViewport(viewport);canvas.begin();
  for(const double rockBase:{574.,575.,576.}){
   state.decor[1].position.y=rockBase;domain.install(state);const auto saved=encode(domain.state());
   const auto& plant=state.decor[0];const auto& rock=state.decor[1];const bool rockInFront=rockBase>=575;
   const auto& back=rockInFront?plant:rock;const auto& front=rockInFront?rock:plant;
   const auto paint=[&](const Decoration& item,bool selected=false,float opacity=1){canvas.decoration(*content.findDecor(item.kind),item,0,false,opacity,nullptr,true,selected);};
   // Compose the expected view explicitly from their ground anchors. The two
   // real assets have different heights, and the centred rock scales larger.
   canvas.begin();canvas.environment({0,0,canvas.width(),canvas.height()});paint(back);paint(front);auto expected=read(canvas.renderer());
   canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false);auto placed=read(canvas.renderer());
   check(differingPixels(*placed,*expected,1)==0,"A rock with a higher base covers the plant to its left");
   canvas.begin();canvas.environment({0,0,canvas.width(),canvas.height()});paint(front);paint(back);auto reversed=read(canvas.renderer());
   check(differingPixels(*placed,*reversed,3)>40,"The base-height regression fixture has no visible overlap");
   const auto p=canvas.decorRect(*content.findDecor(plant.kind),plant.position),r=canvas.decorRect(*content.findDecor(rock.kind),rock.position);
   const SDL_FPoint overlap{(std::max(p.x,r.x)+std::min(p.x+p.w,r.x+r.w))*.5f,
    (std::max(p.y,r.y)+std::min(p.y+p.h,r.y+r.h))*.5f};
   const auto hits=canvas.decorHits(domain,overlap);
   check(hits.size()==2&&hits.front()->id==front.id&&hits.back()->id==back.id,"Overlap selection disagrees with base-height drawing order");
   // Selecting the rock should only fade a plant whose base is lower.
   canvas.begin();canvas.environment({0,0,canvas.width(),canvas.height()});
   paint(back,back.id==rock.id);paint(front,front.id==rock.id,rockInFront?1.f:.22f);expected=read(canvas.renderer());
   canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false,{},&rock);auto selected=read(canvas.renderer());
   check(differingPixels(*selected,*expected,1)==0,"Selection transparency uses perspective size instead of base height");
   const auto stem=std::string(software?"base-software-":"base-native-")+std::to_string(viewport.width)+(rockBase==574?"-behind":rockBase==575?"-equal":"-front");
   check(canvas.capture(directory/(stem+"-selected.png")),"Cannot capture base-height selection");
   canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false);
   check(canvas.capture(directory/(stem+".png")),"Cannot capture base-height layering");
   check(encode(domain.state())==saved,"Rendering the layering correction changes saved placements");
  }
 }
 std::cout<<"PASS "<<SDL_GetRendererName(canvas.renderer())<<" plant and rock base-height layering, selection and transparency across horizontal positions\n";
}
void checkPlacementReceipts(const std::filesystem::path& assets){
 Canvas canvas(assets,852,393,true);
 auto frame=[&](Amount xp){
  canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,255});
  const std::array<PlacementReceipt,1> receipts{{{{544,400},70,xp,false,0}}};
  paintPlacementReceipts(canvas,receipts,1);return read(canvas.renderer());
 };
 auto cost=frame(0),withXp=frame(25);int greenCost=0,greenXp=0;
 for(int y=0;y<cost->h;++y)for(int x=0;x<cost->w;++x){
  auto green=[](const Uint8* p){return p[1]>150&&p[1]>p[0]*1.4&&p[1]>p[2]*1.4;};
  greenCost+=green(pixel(*cost,x,y));greenXp+=green(pixel(*withXp,x,y));
 }
 check(greenCost==0&&greenXp>10,"Purchase receipt shows zero XP or omits earned XP");
 std::cout<<"PASS shared purchase receipts show XP only when earned\n";
}
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
 // Rounded dialog artwork must leave the frame visible at all four corners.
 canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,255});
 canvas.origin(100,50);canvas.roundedImage(pattern.string(),{20,20,80,60},20);canvas.origin(0);
 auto rounded=read(canvas.renderer());const float sx=float(rounded->w)/canvas.width(),sy=float(rounded->h)/canvas.height();
 for(auto p:{SDL_FPoint{121,71},SDL_FPoint{198,71},SDL_FPoint{121,128},SDL_FPoint{198,128}}){
  const auto* corner=pixel(*rounded,int(p.x*sx),int(p.y*sy));check(corner[0]==0&&corner[1]==0&&corner[2]==0,"Rounded artwork covers a dialog corner");
 }
 const auto* middle=pixel(*rounded,int(160*sx),int(100*sy));
 check(std::abs(int(middle[0])-128)<=2&&middle[1]==0&&middle[2]==0,"Rounded artwork loses texture filtering or origin alignment");
 canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,255});
 canvas.roundedImage(pattern.string(),{120,70,80,60},12,12);auto feathered=read(canvas.renderer());
 const auto* edge=pixel(*feathered,int(122*sx),int(100*sy));const auto* solid=pixel(*feathered,int(160*sx),int(100*sy));
 check(edge[0]>0&&edge[0]<70&&std::abs(int(solid[0])-128)<=2,"Artwork feather does not blend its edge while preserving its center");
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
void checkWaterRipples(const std::filesystem::path& assets){
 std::ifstream input(assets/"content.json");Domain domain(Content::fromJson(Json::parse(input)),0);
 auto state=domain.state();state.fish.clear();state.decor.clear();
 Canvas canvas(assets,667,375,true);
 for(auto viewport:{PreviewViewport{667,375,1,{}},PreviewViewport{390,844,2,{}},PreviewViewport{1024,768,1,{}}}){
  canvas.previewViewport(viewport);
  const auto frame=[&]{canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{20,90,130,255});canvas.scene(domain,1,0,Tool::Select,{},false,{},nullptr,0,true,false);return read(canvas.renderer());};
  for(bool reduced:{false,true}){
   state.settings.reducedMotion=reduced;domain.install(state);auto clean=frame();
   domain.tapWater({544,400});for(int i=0;i<10;++i)domain.stepMovement(.02,Tool::Select);auto ripple=frame();
   check(differingPixels(*clean,*ripple,3)>30,"Water ripple is invisible");
   const auto center=canvas.toScreen({544,400});const float sx=float(ripple->w)/canvas.width(),sy=float(ripple->h)/canvas.height();
   for(int y=0;y<ripple->h;++y)for(int x=0;x<ripple->w;++x){
    if(std::hypot(x/sx-center.x,y/sy-center.y)<140*canvas.worldScale())continue;
    check(std::equal(pixel(*clean,x,y),pixel(*clean,x,y)+4,pixel(*ripple,x,y)),"Water ripple appears away from the tap");
   }
   for(int i=0;i<10;++i)domain.stepMovement(.02,Tool::Select);auto later=frame();
   check(differingPixels(*ripple,*later,3)>20,"Water ripple does not expand or fade");
   for(int i=0;i<20;++i)domain.stepMovement(.02,Tool::Select);auto lingering=frame();
   check(differingPixels(*clean,*lingering,12)>30,"Water ripple fades before the player can see it");
   for(int i=0;i<30;++i)domain.stepMovement(.02,Tool::Select);auto ended=frame();
   check(differingPixels(*clean,*ended)==0,"Water ripple leaves pixels after it ends");
  }
 }
 std::cout<<"PASS visible water ripples, tap alignment and fading across phone, tablet and reduced motion\n";
}
}
int main(int argc,char** argv){try{if(argc!=2)throw std::runtime_error("Usage: aquarium_rendering_tests ASSETS");checkMotion(argv[1]);checkWaterRipples(argv[1]);checkPremiumFish(argv[1]);checkDecorTransparency(argv[1],true);checkDecorTransparency(argv[1],false);checkPlacementFocus(argv[1],true);checkPlacementFocus(argv[1],false);checkDecorBaseLayering(argv[1],true);checkDecorBaseLayering(argv[1],false);checkPlacementReceipts(argv[1]);checkFrames(argv[1]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
