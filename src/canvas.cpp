#include "aquarium/canvas.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>
namespace aq {
namespace {
constexpr double pi=3.14159265358979323846;
SDL_FColor color(Color c){return {float(c.r)/255.f,float(c.g)/255.f,float(c.b)/255.f,float(c.a)/255.f};}
std::uint64_t patternHash(std::uint64_t value){
 value+=0x9e3779b97f4a7c15ULL;value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
 value=(value^(value>>27))*0x94d049bb133111ebULL;return value^(value>>31);
}
double patternUnit(std::uint64_t value){return double(patternHash(value)>>11)*0x1.0p-53;}
struct InkPatch {double u{},v{},width{},height{};bool red{};};
std::array<InkPatch,5> koiPatches(FishId id){
 std::array<InkPatch,5> result;
 for(std::size_t i=0;i<result.size();++i){
  const auto seed=patternHash(id.value+i*0x9e3779b97f4a7c15ULL);
  result[i]={.31+.40*patternUnit(seed),.34+.26*patternUnit(seed+1),.06+.065*patternUnit(seed+2),.08+.1*patternUnit(seed+3),patternUnit(seed+4)>.5};
 }
 return result;
}
double softSpot(double u,double v,double x,double y,double width,double height){
 const double dx=(u-x)/width,dy=(v-y)/height,t=std::clamp(1-dx*dx-dy*dy,0.,1.);return t*t*(3-2*t);
}
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
// Area filtering in premultiplied color space keeps transparent fins and icon
// edges clean. Each level averages a 2x2 footprint instead of skipping texels.
Surface halfSize(const SDL_Surface& source){
 Surface result(SDL_CreateSurface(std::max(1,(source.w+1)/2),std::max(1,(source.h+1)/2),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
 if(!result)throw std::runtime_error(SDL_GetError());
 for(int y=0;y<result->h;++y)for(int x=0;x<result->w;++x){
  unsigned alpha=0,rgb[3]{},count=0;
  for(int yy=y*2;yy<std::min(source.h,y*2+2);++yy)for(int xx=x*2;xx<std::min(source.w,x*2+2);++xx){
   const auto* p=static_cast<const Uint8*>(source.pixels)+yy*source.pitch+xx*4;
   alpha+=p[3];for(int k=0;k<3;++k)rgb[k]+=unsigned(p[k])*p[3];++count;
  }
  auto* out=static_cast<Uint8*>(result->pixels)+y*result->pitch+x*4;
  for(int k=0;k<3;++k)out[k]=alpha?Uint8((rgb[k]+alpha/2)/alpha):0;
  out[3]=Uint8((alpha+count/2)/count);
 }
 return result;
}
void opacityMask(SDL_Surface& surface){
 // Multiplicative blending reads RGB even in fully transparent pixels.
 // Store premultiplied white so every texture level follows its own alpha.
 for(int y=0;y<surface.h;++y)for(int x=0;x<surface.w;++x){
  auto* p=static_cast<Uint8*>(surface.pixels)+y*surface.pitch+x*4;p[0]=p[1]=p[2]=p[3];
 }
}
SDL_Texture* upload(SDL_Renderer* renderer,SDL_Surface* surface){
 auto* texture=SDL_CreateTextureFromSurface(renderer,surface);if(!texture)throw std::runtime_error(SDL_GetError());
 SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_LINEAR);return texture;
}
std::filesystem::path pickFont(const std::filesystem::path& assets,bool display){
 const char* env=std::getenv(display?"AQUARIUM_DISPLAY_FONT":"AQUARIUM_BODY_FONT");if(env&&std::filesystem::exists(env))return env;
 std::vector<std::filesystem::path> candidates={assets/"fonts"/(display?"LilitaOne-Regular.ttf":"Nunito-SemiBold.ttf"),"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf","/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf","/System/Library/Fonts/Supplemental/Arial.ttf","/Library/Fonts/Arial.ttf","C:/Windows/Fonts/arial.ttf","/system/fonts/Roboto-Regular.ttf","/System/Library/Fonts/CoreUI/SFUI.ttf"};
 for(auto& p:candidates)if(std::filesystem::exists(p))return p;return {};
}
} // namespace
SDL_Texture* Texture::sampled(float pixelWidth,float pixelHeight)const{
 SDL_Texture* result=value;
 for(auto* level:levels){if(level->w<std::max(1.f,pixelWidth)||level->h<std::max(1.f,pixelHeight))break;result=level;}
 return result;
}
std::size_t Texture::bytes()const{std::size_t total=std::size_t(width)*height*4;for(auto* level:levels)total+=std::size_t(level->w)*level->h*4;return total;}
FishPose fishPose(const Fish& f,double interpolation){
 const auto& m=f.motion;const double t=std::clamp(interpolation,0.,1.);
 auto mix=[&](double a,double b){return a+(b-a)*t;};
 if(!m.hasPrevious)return {f.position,m.phase,m.pitch,motionFacing(m),m.speed};
 return {{mix(m.previous.x,f.position.x),mix(m.previous.y,f.position.y)},mix(m.previousPhase,m.phase),mix(m.previousPitch,m.pitch),mix(m.previousFacing,motionFacing(m)),mix(m.previousSpeed,m.speed)};
}
std::string fishArt(std::string_view id){return "species/"+std::string(id)+".png";}
std::string compact(Amount n){std::string s=std::to_string(n);std::size_t sign=s[0]=='-'?1:0;for(std::ptrdiff_t i=static_cast<std::ptrdiff_t>(s.size())-3;i>static_cast<std::ptrdiff_t>(sign);i-=3)s.insert(static_cast<std::size_t>(i),",");return s;}
std::string usdPrice(int cents){return "$"+std::to_string(cents/100)+"."+(cents%100<10?"0":"")+std::to_string(cents%100);}
Canvas::Canvas(std::filesystem::path assets,int w,int h,bool software):assets_(std::move(assets)){
 if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))throw std::runtime_error(SDL_GetError());if(!TTF_Init())throw std::runtime_error(SDL_GetError());
 SDL_SetHint(SDL_HINT_ORIENTATIONS,"LandscapeLeft LandscapeRight");
 window_=SDL_CreateWindow("Fishius",w,h,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY);if(!window_)throw std::runtime_error(SDL_GetError());
 renderer_=SDL_CreateRenderer(window_,software?"software":nullptr);if(!renderer_)throw std::runtime_error(SDL_GetError());SDL_SetRenderVSync(renderer_,1);SDL_SetRenderDrawBlendMode(renderer_,SDL_BLENDMODE_BLEND);
 fontPath_=pickFont(assets_,false);displayFontPath_=pickFont(assets_,true);if(fontPath_.empty())throw std::runtime_error("No native font found. Run tools/setup_fonts.py or set AQUARIUM_BODY_FONT.");if(displayFontPath_.empty())displayFontPath_=fontPath_;
 SDL_AudioSpec spec{SDL_AUDIO_F32,1,44100};audio_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);if(audio_)SDL_ResumeAudioStreamDevice(audio_);begin();
}
Canvas::~Canvas(){SDL_ShowCursor();SDL_SetCursor(SDL_GetDefaultCursor());for(auto* target:focusTextures_)if(target)SDL_DestroyTexture(target);if(frameTexture_)SDL_DestroyTexture(frameTexture_);textCache_.clear();textures_.clear();for(auto& [_,f]:fonts_)TTF_CloseFont(f);if(audio_)SDL_DestroyAudioStream(audio_);if(renderer_)SDL_DestroyRenderer(renderer_);if(window_)SDL_DestroyWindow(window_);TTF_Quit();SDL_Quit();}
void Canvas::begin(bool fitViewport){
 int w=1,h=1;SDL_GetWindowSize(window_,&w,&h);
 if(previewViewport_){w=previewViewport_->width;h=previewViewport_->height;}
 windowWidth_=float(std::max(1,w));windowHeight_=float(std::max(1,h));originX_=originY_=0;
 // Grow the world vertically on tablets and horizontally on wide phones.
 // The scene and input mapping always fill exactly the same window rectangle.
 const float aspect=windowWidth_/windowHeight_;
 width_=std::max(fitViewport?1.f:1608.f,830.f*aspect);height_=width_/aspect;
 displayRect_={0,0,windowWidth_,windowHeight_};
 SDL_Rect safe{0,0,w,h};SDL_GetWindowSafeArea(window_,&safe);
 if(safe.w<=0||safe.h<=0)safe={0,0,w,h};
 auto toCanvasX=[&](float px){return (px-displayRect_.x)*width_/displayRect_.w;};
 auto toCanvasY=[&](float py){return (py-displayRect_.y)*height_/displayRect_.h;};
 safeInsets_={std::max(0.f,toCanvasX(float(safe.x))),std::max(0.f,toCanvasY(float(safe.y))),
              std::max(0.f,width_-toCanvasX(float(safe.x+safe.w))),std::max(0.f,height_-toCanvasY(float(safe.y+safe.h)))};
 if(previewViewport_){const auto insets=previewViewport_->safe;safeInsets_={insets.left*width_/w,insets.top*height_/h,insets.right*width_/w,insets.bottom*height_/h};}
 int needW=1,needH=1;SDL_GetWindowSizeInPixels(window_,&needW,&needH);
 if(previewViewport_){needW=w*previewViewport_->pixelRatio;needH=h*previewViewport_->pixelRatio;}
 needW=std::max(1,needW);needH=std::max(1,needH);
 pixelScaleX_=float(needW)/width_;pixelScaleY_=float(needH)/height_;
 if(frameTexture_&&(needW!=frameWidth_||needH!=frameHeight_)){SDL_DestroyTexture(frameTexture_);frameTexture_=nullptr;}
 if(!frameTexture_){frameTexture_=SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,needW,needH);if(!frameTexture_)throw std::runtime_error(SDL_GetError());SDL_SetTextureScaleMode(frameTexture_,SDL_SCALEMODE_LINEAR);frameWidth_=needW;frameHeight_=needH;}
 SDL_SetRenderTarget(renderer_,frameTexture_);SDL_SetRenderScale(renderer_,pixelScaleX_,pixelScaleY_);SDL_SetRenderDrawColor(renderer_,0,130,190,255);SDL_RenderClear(renderer_);++frame_;
}
void Canvas::composeWindow(){
 SDL_SetRenderTarget(renderer_,nullptr);int pw=1,ph=1;SDL_GetWindowSizeInPixels(window_,&pw,&ph);SDL_SetRenderScale(renderer_,float(pw)/windowWidth_,float(ph)/windowHeight_);
 SDL_SetRenderDrawColor(renderer_,0,91,139,255);SDL_RenderClear(renderer_);
 SDL_FRect src{0,0,float(frameWidth_),float(frameHeight_)},dest{displayRect_.x,displayRect_.y,displayRect_.w,displayRect_.h};SDL_RenderTexture(renderer_,frameTexture_,&src,&dest);
}
void Canvas::present(){
 composeWindow();SDL_RenderPresent(renderer_);
 // Metal's simulator backend can accept VSync without pacing its presents.
 // Bound the loop to 60 Hz, also avoiding bursts after an expensive frame.
 constexpr Uint64 interval=1000000000ULL/60;
 auto now=SDL_GetTicksNS();if(nextFrame_&&now<nextFrame_)SDL_DelayPrecise(nextFrame_-now);
 now=SDL_GetTicksNS();nextFrame_=!nextFrame_||now>nextFrame_+interval?now+interval:nextFrame_+interval;
}
SDL_FPoint Canvas::inputPoint(float x,float y,bool normalized)const{
 if(normalized){x*=windowWidth_;y*=windowHeight_;}
 return {(x-displayRect_.x)*width_/displayRect_.w,(y-displayRect_.y)*height_/displayRect_.h};
}
WorldPoint Canvas::toWorld(float x,float y)const{return {double(x/width_)*waterWidth,double(y/height_)*tankHeight};}
SDL_FPoint Canvas::toScreen(WorldPoint p)const{return {float(p.x/waterWidth)*width_,float(p.y/tankHeight)*height_};}
void Canvas::fill(Rect r,Color c){r.x+=originX_;r.y+=originY_;SDL_SetRenderDrawColor(renderer_,c.r,c.g,c.b,c.a);SDL_FRect rect{r.x,r.y,r.w,r.h};SDL_RenderFillRect(renderer_,&rect);}
void Canvas::triangle(const std::array<SDL_FPoint,3>& points,Color tint){
 std::array<SDL_Vertex,3> vertices{};
 for(std::size_t i=0;i<points.size();++i)vertices[i]={{points[i].x+originX_,points[i].y+originY_},color(tint),{}};
 constexpr std::array<int,3> indices{0,1,2};mesh(nullptr,vertices,indices);
}
void Canvas::clip(Rect r){
 SDL_Rect clip{int(std::floor(r.x+originX_)),int(std::floor(r.y+originY_)),int(std::ceil(r.w)),int(std::ceil(r.h))};
 if(!SDL_SetRenderClipRect(renderer_,&clip))throw std::runtime_error(SDL_GetError());
}
void Canvas::clearClip(){SDL_SetRenderClipRect(renderer_,nullptr);}
void Canvas::mesh(SDL_Texture* t,std::span<const SDL_Vertex> v,std::span<const int> indices){if(!SDL_RenderGeometry(renderer_,t,v.data(),static_cast<int>(v.size()),indices.data(),static_cast<int>(indices.size())))throw std::runtime_error(SDL_GetError());}
void Canvas::gradient(Rect rect,Color top,Color bottom,float radius){
 if(rect.w<=0||rect.h<=0)return;
 rect.x+=originX_;rect.y+=originY_;
 radius=std::clamp(radius,0.f,std::min(rect.w,rect.h)*.5f);
 auto shade=[&](float y){
  const float t=std::clamp((y-rect.y)/rect.h,0.f,1.f);
  const auto a=color(top),b=color(bottom);
  return SDL_FColor{a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t,a.a+(b.a-a.a)*t};
 };
 if(radius==0){
  const std::array<SDL_Vertex,4> v{{{{rect.x,rect.y},color(top),{}},{{rect.x+rect.w,rect.y},color(top),{}},{{rect.x+rect.w,rect.y+rect.h},color(bottom),{}},{{rect.x,rect.y+rect.h},color(bottom),{}}}};
  constexpr std::array<int,6> indices{0,1,2,0,2,3};mesh(nullptr,v,indices);return;
 }
 constexpr int segments=16,ring=4*(segments+1);
 const float fringe=1.f/std::max(pixelScaleX_,pixelScaleY_);
 std::array<SDL_Vertex,1+ring*2> vertices{};
 vertices[0]={{rect.x+rect.w*.5f,rect.y+rect.h*.5f},shade(rect.y+rect.h*.5f),{}};
 const std::array<SDL_FPoint,4> centers{{{rect.x+rect.w-radius,rect.y+radius},{rect.x+rect.w-radius,rect.y+rect.h-radius},{rect.x+radius,rect.y+rect.h-radius},{rect.x+radius,rect.y+radius}}};
 for(int corner=0;corner<4;++corner)for(int step=0;step<=segments;++step){
  const double angle=(-90.+corner*90.+step*90./segments)*pi/180.;
  const float nx=float(std::cos(angle)),ny=float(std::sin(angle));
  const float x=centers[corner].x+nx*radius,y=centers[corner].y+ny*radius;
  const int i=1+corner*(segments+1)+step;auto c=shade(y);
  vertices[i]={{x,y},c,{}};c.a=0;vertices[i+ring]={{x+nx*fringe,y+ny*fringe},c,{}};
 }
 std::array<int,ring*9> indices{};
 for(int i=1;i<=ring;++i){
  const int next=i==ring?1:i+1;const std::array<int,9> triangle{0,i,next,i,next,i+ring,next,next+ring,i+ring};
  std::copy(triangle.begin(),triangle.end(),indices.begin()+(i-1)*9);
 }
 mesh(nullptr,vertices,indices);
}
void Canvas::wave(Rect rect,Color tint,float amplitude,float wavelength,float phase,float thickness){
 if(rect.w<=0||rect.h<=0||thickness<=0||wavelength<=0)return;
 rect.x+=originX_;rect.y+=originY_;
 constexpr int segments=96,layers=3;
 std::array<SDL_Vertex,(segments+1)*layers> vertices{};
 for(int i=0;i<=segments;++i){
  const float x=rect.w*float(i)/segments;
  const float y=rect.y+rect.h*.5f+amplitude*std::sin(float(2*pi)*x/wavelength+phase);
  for(int j=0;j<layers;++j){auto c=color(tint);if(j!=1)c.a=0;
   vertices[i*layers+j]={{rect.x+x,y+(j-1)*thickness*.5f},c,{}};
  }
 }
 std::array<int,segments*12> indices{};
 for(int i=0;i<segments;++i)for(int j=0;j<2;++j){
  const int a=i*layers+j,b=a+layers;const std::array<int,6> triangle{a,b,a+1,b,b+1,a+1};
  std::copy(triangle.begin(),triangle.end(),indices.begin()+i*12+j*6);
 }
 mesh(nullptr,vertices,indices);
}
void Canvas::round(Rect rect,Color c,float radius,Color border,float thickness,bool shadow,bool shine){
 if(rect.w<=0||rect.h<=0)return;
 rect.x+=originX_;rect.y+=originY_;
 auto shape=[&](Rect r,Color col,float rad,bool gradient){
  rad=std::min({rad,r.w*.5f,r.h*.5f});constexpr int segments=16,ring=4*(segments+1);
  const float fringe=1.f/std::max(pixelScaleX_,pixelScaleY_);
  std::vector<SDL_Vertex> v(1+ring*2);v[0]={{r.x+r.w*.5f,r.y+r.h*.5f},color(col),{}};
  const std::array<SDL_FPoint,4> centers{{{r.x+r.w-rad,r.y+rad},{r.x+r.w-rad,r.y+r.h-rad},{r.x+rad,r.y+r.h-rad},{r.x+rad,r.y+rad}}};
  for(int corner=0;corner<4;++corner)for(int k=0;k<=segments;++k){
   const double a=(-90.+corner*90.+k*90./segments)*pi/180.;const float nx=float(std::cos(a)),ny=float(std::sin(a));
   const float x=centers[corner].x+nx*rad,y=centers[corner].y+ny*rad;
   auto fc=color(col);if(gradient){float mult=1.f-.10f*(y-r.y)/std::max(r.h,1.f);fc.r*=mult;fc.g*=mult;fc.b*=mult;}
   const int i=1+corner*(segments+1)+k;v[i]={{x,y},fc,{}};fc.a=0;v[i+ring]={{x+nx*fringe,y+ny*fringe},fc,{}};
  }
  std::vector<int> ix;ix.reserve(ring*9);for(int i=1;i<=ring;++i){const int next=i==ring?1:i+1;ix.insert(ix.end(),{0,i,next,i,next,i+ring,next,next+ring,i+ring});}mesh(nullptr,v,ix);
 };
 if(shadow){shape({rect.x,rect.y+4,rect.w,rect.h},{23,73,94,65},radius,false);}
 if(thickness>0){shape(rect,border,radius,false);rect={rect.x+thickness,rect.y+thickness,rect.w-2*thickness,rect.h-2*thickness};radius=std::max(0.f,radius-thickness);}shape(rect,c,radius,true);
 if(shine&&rect.h>22&&c.a>100){shape({rect.x+3,rect.y+3,rect.w-6,std::min(9.f,rect.h*.18f)},{255,255,255,62},std::min(6.f,radius),false);}
}
void Canvas::outline(Rect rect,Color border,float radius,float thickness){
 if(rect.w<=0||rect.h<=0||thickness<=0)return;
 rect.x+=originX_;rect.y+=originY_;
 radius=std::clamp(radius,thickness,std::min(rect.w,rect.h)*.5f);
 const float fringe=1.f/std::max(pixelScaleX_,pixelScaleY_);
 constexpr int segments=16,ring=4*(segments+1);
 const std::array<SDL_FPoint,4> centers{{{rect.x+rect.w-radius,rect.y+radius},{rect.x+rect.w-radius,rect.y+rect.h-radius},{rect.x+radius,rect.y+rect.h-radius},{rect.x+radius,rect.y+radius}}};
 std::array<SDL_Vertex,ring*4> vertices;
 const std::array<float,4> radii{radius+fringe,radius,std::max(0.f,radius-thickness),std::max(0.f,radius-thickness-fringe)};
 for(int layer=0;layer<4;++layer)for(int corner=0;corner<4;++corner)for(int k=0;k<=segments;++k){
  const double angle=(-90.+corner*90.+k*90./segments)*pi/180.;
  auto c=color(border);if(layer==0||layer==3)c.a=0;
  vertices[layer*ring+corner*(segments+1)+k]={{centers[corner].x+float(std::cos(angle))*radii[layer],centers[corner].y+float(std::sin(angle))*radii[layer]},c,{}};
 }
 std::vector<int> indices;indices.reserve(ring*18);
 for(int layer=0;layer<3;++layer)for(int i=0;i<ring;++i){
  const int a=layer*ring+i,b=layer*ring+(i+1)%ring;
  indices.insert(indices.end(),{a,b,a+ring,b,b+ring,a+ring});
 }
 mesh(nullptr,vertices,indices);
}
Texture* Canvas::texture(std::string_view key){
 auto it=textures_.find(std::string(key));if(it!=textures_.end())return it->second.get();
 const bool decorMask=key.starts_with("mask:");
 const bool species=key.starts_with("species/"),mask=species&&key.ends_with("-mask.png"),dead=species&&key.ends_with("-dead.png");
 std::string source(decorMask?key.substr(5):key);if(mask||dead)source=source.substr(0,source.size()-9)+".png";
 const auto path=assets_/source;
 Surface surface(IMG_Load(path.string().c_str()),SDL_DestroySurface);
 if(!surface)throw std::runtime_error("Asset load failed: "+path.string()+": "+SDL_GetError());
 if(decorMask){
  Surface rgba(SDL_ConvertSurface(surface.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
  if(!rgba)throw std::runtime_error(SDL_GetError());
  opacityMask(*rgba);
  surface=std::move(rgba);
 }
 if(species){
  // Keep approved source PNGs intact. Normalize their transparent padding and
  // orientation at load time so every UI and swimming mesh uses the same art.
  Surface rgba(SDL_ConvertSurface(surface.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
  if(!rgba)throw std::runtime_error(SDL_GetError());
  int left=rgba->w,top=rgba->h,right=-1,bottom=-1;
  const int threshold=4;
  for(int y=0;y<rgba->h;++y){const auto* row=static_cast<const Uint8*>(rgba->pixels)+y*rgba->pitch;for(int x=0;x<rgba->w;++x)if(row[x*4+3]>threshold){left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);}}
  if(right<left)throw std::runtime_error("Species artwork is empty: "+source);
  left=std::max(0,left-2);top=std::max(0,top-2);right=std::min(rgba->w-1,right+2);bottom=std::min(rgba->h-1,bottom+2);
  Surface cropped(SDL_CreateSurface(right-left+1,bottom-top+1,SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
  if(!cropped)throw std::runtime_error(SDL_GetError());
  const bool flip=source=="species/neonTetra.png"||source=="species/ocellarisClownfish.png";
  for(int y=0;y<cropped->h;++y){auto* dst=static_cast<Uint8*>(cropped->pixels)+y*cropped->pitch;const auto* row=static_cast<const Uint8*>(rgba->pixels)+(y+top)*rgba->pitch;
   for(int x=0;x<cropped->w;++x){const auto* pixel=row+(flip?right-x:left+x)*4;auto* out=dst+x*4;std::memcpy(out,pixel,4);
    if(mask)out[0]=out[1]=out[2]=255;
    else if(dead){const auto gray=Uint8((77*int(pixel[0])+150*int(pixel[1])+29*int(pixel[2]))/256);out[0]=out[1]=out[2]=gray;}
   }
  }
  // 512 pixels exceeds the largest displayed fish card while bounding GPU
  // memory when all 46 species and their outlines have been viewed.
  if(std::max(cropped->w,cropped->h)>512){
   while(std::max(cropped->w,cropped->h)>1024)cropped=halfSize(*cropped);
   const float finalScale=512.f/float(std::max(cropped->w,cropped->h));
   Surface scaled(SDL_CreateSurface(std::max(1,int(std::lround(cropped->w*finalScale))),std::max(1,int(std::lround(cropped->h*finalScale))),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
   if(!scaled)throw std::runtime_error(SDL_GetError());SDL_SetSurfaceBlendMode(cropped.get(),SDL_BLENDMODE_NONE);
   if(!SDL_BlitSurfaceScaled(cropped.get(),nullptr,scaled.get(),nullptr,SDL_SCALEMODE_LINEAR))throw std::runtime_error(SDL_GetError());cropped=std::move(scaled);
  }
  surface=std::move(cropped);
 }
 auto value=std::make_unique<Texture>();value->width=surface->w;value->height=surface->h;value->value=upload(renderer_,surface.get());
 Surface level(SDL_ConvertSurface(surface.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);if(!level)throw std::runtime_error(SDL_GetError());
 while(std::min(level->w,level->h)>16){level=halfSize(*level);if(decorMask)opacityMask(*level);value->levels.push_back(upload(renderer_,level.get()));}
 auto* result=value.get();textures_.emplace(key,std::move(value));return result;
}
TTF_Font* Canvas::font(int pixels,bool display,bool reference,bool medium,bool rounded,bool heavy){int key=pixels+(heavy?5000:rounded?4000:medium?3000:reference?2000:display?1000:0);auto i=fonts_.find(key);if(i!=fonts_.end())return i->second;auto path=heavy?assets_/"fonts/Nunito-ExtraBold.ttf":rounded?assets_/"fonts/Baloo2-ExtraBold.ttf":medium?assets_/"fonts/Nunito-Medium.ttf":reference?assets_/"fonts/LuckiestGuy-Regular.ttf":display?displayFontPath_:fontPath_;auto* f=TTF_OpenFont(path.string().c_str(),float(pixels));if(!f)throw std::runtime_error(SDL_GetError());fonts_[key]=f;return f;}
float Canvas::textWidth(std::string_view label,float size,bool display,bool medium,bool rounded,bool heavy,bool reference){
 int w{},h{};const int pixels=std::clamp(int(std::round(size)),9,256);
 if(!TTF_GetStringSize(font(pixels,display,reference,medium,rounded,heavy),label.data(),label.size(),&w,&h))throw std::runtime_error(SDL_GetError());
 return float(w);
}
void Canvas::text(std::string_view label,float x,float y,float size,Color c,bool center,float maxWidth,bool display,bool reference,bool gold,bool medium,double angle,float horizontalScale,bool rounded,bool heavy,float renderScale){
 if(label.empty())return;
 x+=originX_;y+=originY_;const int pixels=std::clamp(static_cast<int>(std::round(size)),9,256);
 // Rasterize above the physical display density, then filter once into the
 // native frame. Reuse the same white glyphs for every outline and tint.
 const int rasterPixels=std::clamp(int(std::ceil(pixels*std::max(pixelScaleX_,pixelScaleY_)*2)),9,768);
 std::string key=std::to_string(pixels)+":"+std::to_string(rasterPixels)+(heavy?":heavy:":rounded?":rounded:":medium?":m:":reference?":ref:":display?":d:":":b:")+std::string(label);
 auto it=textCache_.find(key);if(it==textCache_.end()){
  int logicalW{},logicalH{};if(!TTF_GetStringSize(font(pixels,display,reference,medium,rounded,heavy),label.data(),label.size(),&logicalW,&logicalH))throw std::runtime_error(SDL_GetError());
  auto* sf=TTF_RenderText_Blended(font(rasterPixels,display,reference,medium,rounded,heavy),label.data(),label.size(),{255,255,255,255});if(!sf)throw std::runtime_error(SDL_GetError());
  auto t=std::make_unique<Texture>();t->width=sf->w;t->height=sf->h;t->value=upload(renderer_,sf);
  Surface level(SDL_ConvertSurface(sf,SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);SDL_DestroySurface(sf);if(!level)throw std::runtime_error(SDL_GetError());
  while(std::min(level->w,level->h)>8){level=halfSize(*level);t->levels.push_back(upload(renderer_,level.get()));}
  const auto bytes=t->bytes();
  // Keep the prepared catalog resident at phone and tablet pixel densities, while still
  // bounding changing timers and receipts. Evict only the least recently used.
  while(textBytes_+bytes>96*1024*1024){
   auto oldest=textCache_.end();
   for(auto entry=textCache_.begin();entry!=textCache_.end();++entry)
    if(oldest==textCache_.end()||entry->second.used<oldest->second.used)oldest=entry;
   if(oldest==textCache_.end())break;
   textBytes_-=oldest->second.texture->bytes();textCache_.erase(oldest);
  }
  textBytes_+=bytes;
  it=textCache_.emplace(key,TextEntry{std::move(t),frame_,float(logicalW),float(logicalH)}).first;
  ++textRasterizations_;
 }
 it->second.used=frame_;auto& entry=it->second;auto& t=*entry.texture;
 const float width=entry.width*std::clamp(horizontalScale,.5f,2.f)*renderScale;
 const float factor=maxWidth>0&&width>maxWidth?maxWidth/width:1.f;
 SDL_FRect dst{x-(center?width*factor*.5f:0),y,width*factor,entry.height*factor*renderScale};
 auto* glyphs=t.sampled(dst.w*pixelScaleX_*1.25f,dst.h*pixelScaleY_*1.25f);
 SDL_SetTextureColorMod(glyphs,gold?255:c.r,gold?255:c.g,gold?255:c.b);SDL_SetTextureAlphaMod(glyphs,c.a);
 if(gold){const auto top=color({255,255,163,c.a}),bottom=color({255,170,0,c.a});std::array<SDL_Vertex,4> v{{{{dst.x,dst.y},top,{0,0}},{{dst.x+dst.w,dst.y},top,{1,0}},{{dst.x+dst.w,dst.y+dst.h},bottom,{1,1}},{{dst.x,dst.y+dst.h},bottom,{0,1}}}};const std::array<int,6> indices{0,1,2,0,2,3};mesh(glyphs,v,indices);}
 else if(angle!=0){const SDL_FPoint pivot{0,0};SDL_RenderTextureRotated(renderer_,glyphs,nullptr,&dst,angle,&pivot,SDL_FLIP_NONE);}
 else SDL_RenderTexture(renderer_,glyphs,nullptr,&dst);
}
void Canvas::image(std::string_view name,Rect r,double angle,SDL_FPoint pivot,float alpha,bool flip,Color tint){r.x+=originX_;r.y+=originY_;auto* t=texture(name);float sx{},sy{};SDL_GetRenderScale(renderer_,&sx,&sy);auto* sampled=t->sampled(r.w*sx,r.h*sy);SDL_FRect dst{r.x,r.y,r.w,r.h};SDL_FPoint p{pivot.x*r.w,pivot.y*r.h};SDL_SetTextureColorMod(sampled,tint.r,tint.g,tint.b);SDL_SetTextureAlphaModFloat(sampled,std::clamp(alpha*tint.a/255.f,0.f,1.f));SDL_RenderTextureRotated(renderer_,sampled,nullptr,&dst,angle,&p,flip?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE);SDL_SetTextureAlphaModFloat(sampled,1.f);SDL_SetTextureColorMod(sampled,255,255,255);}
void Canvas::roundedImage(std::string_view name,Rect rect,float radius,float feather,bool flip){
 if(rect.w<=0||rect.h<=0)return;
 radius=std::clamp(radius,0.f,std::min(rect.w,rect.h)*.5f);
 if(radius==0){image(name,rect,0,{.5f,.5f},1,flip);return;}
 rect.x+=originX_;rect.y+=originY_;
 auto* t=texture(name);float sx{},sy{};SDL_GetRenderScale(renderer_,&sx,&sy);
 auto* sampled=t->sampled(rect.w*sx,rect.h*sy);
 constexpr int segments=16;
 feather=std::clamp(feather,0.f,radius);
 const float fringe=feather>0?feather:1.f/std::max(pixelScaleX_,pixelScaleY_);
 auto vertex=[&](float x,float y,float alpha){const float u=std::clamp((x-rect.x)/rect.w,0.f,1.f);return SDL_Vertex{{x,y},{1,1,1,alpha},{flip?1-u:u,std::clamp((y-rect.y)/rect.h,0.f,1.f)}};};
 // Blit the opaque cross directly. Long, thin triangle fans can overflow the
 // software renderer's texture interpolation on wide dialog backgrounds.
 auto patch=[&](Rect r){
  if(r.w<=0||r.h<=0)return;
  SDL_FRect source{(r.x-rect.x)/rect.w*sampled->w,(r.y-rect.y)/rect.h*sampled->h,r.w/rect.w*sampled->w,r.h/rect.h*sampled->h};
  if(flip)source.x=sampled->w-source.x-source.w;
  SDL_FRect destination{r.x,r.y,r.w,r.h};SDL_RenderTextureRotated(renderer_,sampled,&source,&destination,0,nullptr,flip?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE);
 };
 // Overlap opaque joins by one physical pixel so fractional scales cannot
 // leave hairline gaps between the blits and the rounded corner meshes.
 const float join=std::min(radius-feather,1.f/std::max(pixelScaleX_,pixelScaleY_));
 patch({rect.x+radius-join,rect.y+feather,rect.w-2*radius+2*join,rect.h-2*feather});
 patch({rect.x+feather,rect.y+radius-join,radius-feather+join,rect.h-2*radius+2*join});
 patch({rect.x+rect.w-radius-join,rect.y+radius-join,radius-feather+join,rect.h-2*radius+2*join});
 auto band=[&](float x1,float y1,float x2,float y2,float dx,float dy){
  // Keep the feather's triangles short too, avoiding interpolation streaks
  // on the software renderer at portrait and small preview sizes.
  const int pieces=std::max(1,int(std::ceil(std::max(std::abs(x2-x1),std::abs(y2-y1))/128.f)));
  for(int i=0;i<pieces;++i){
   const float a=float(i)/pieces,b=float(i+1)/pieces;
   const float ax=x1+(x2-x1)*a,ay=y1+(y2-y1)*a,bx=x1+(x2-x1)*b,by=y1+(y2-y1)*b;
   const std::array<SDL_Vertex,4> v{vertex(ax,ay,1),vertex(bx,by,1),vertex(bx+dx,by+dy,0),vertex(ax+dx,ay+dy,0)};
   constexpr std::array<int,6> index{0,1,2,0,2,3};mesh(sampled,v,index);
  }
 };
 band(rect.x+radius,rect.y+feather,rect.x+rect.w-radius,rect.y+feather,0,-fringe);
 band(rect.x+radius,rect.y+rect.h-feather,rect.x+rect.w-radius,rect.y+rect.h-feather,0,fringe);
 band(rect.x+feather,rect.y+radius,rect.x+feather,rect.y+rect.h-radius,-fringe,0);
 band(rect.x+rect.w-feather,rect.y+radius,rect.x+rect.w-feather,rect.y+rect.h-radius,fringe,0);
 const std::array<SDL_FPoint,4> centers{{{rect.x+rect.w-radius,rect.y+radius},{rect.x+rect.w-radius,rect.y+rect.h-radius},{rect.x+radius,rect.y+rect.h-radius},{rect.x+radius,rect.y+radius}}};
 radius-=feather;
 for(int corner=0;corner<4;++corner){
  constexpr int count=segments+1;std::array<SDL_Vertex,1+count*2> vertices{};
  vertices[0]=vertex(centers[corner].x,centers[corner].y,1);
  for(int step=0;step<=segments;++step){
   const double angle=(-90.+corner*90.+step*90./segments)*pi/180.;
   const float nx=float(std::cos(angle)),ny=float(std::sin(angle));
   const float x=centers[corner].x+nx*radius,y=centers[corner].y+ny*radius;
   vertices[step+1]=vertex(x,y,1);vertices[step+1+count]=vertex(x+nx*fringe,y+ny*fringe,0);
  }
  std::array<int,segments*9> indices{};
  for(int i=1;i<=segments;++i){
   const std::array<int,9> triangle{0,i,i+1,i,i+1,i+count,i+1,i+1+count,i+count};
   std::copy(triangle.begin(),triangle.end(),indices.begin()+(i-1)*9);
  }
  mesh(sampled,vertices,indices);
 }
}
void Canvas::icon(std::string_view name,Rect r,float alpha,bool flip,Color tint){
 auto* t=texture(name);float scale=std::min(r.w/float(t->width),r.h/float(t->height));
 image(name,{r.x+(r.w-float(t->width)*scale)*.5f,r.y+(r.h-float(t->height)*scale)*.5f,float(t->width)*scale,float(t->height)*scale},0,{.5f,.5f},alpha,flip,tint);
}
void Canvas::label(std::string_view s,float x,float y,float size,bool center,float maxWidth,Color c,float stroke,Color outline){
 if(stroke>0){outline.a=c.a;for(int i=0;i<8;++i){float a=float(i)*.78539816f;text(s,x+std::cos(a)*stroke,y+std::sin(a)*stroke+1,size,outline,center,maxWidth,true);}}
 text(s,x,y,size,c,center,maxWidth,true);
}
SDL_FPoint Canvas::fishSize(const Species& s,const Fish& f){
 auto name=fishArt(s.id);auto* art=texture(name);
 float w=float(s.nominalLength*ageScale(s,f)*worldScale()*2.*tankArtScale);
 return {w,w*float(art->height)/float(art->width)};
}
void Canvas::fish(const Species& s,const Fish& f,double alpha,bool held,Color outline,bool selected,bool reduced,Care appearance,double time,float opacity){
 const auto pose=fishPose(f,alpha);auto p=toScreen(pose.position);if(f.egg){if(!reduced)p.y+=float(std::sin(f.motion.drift*2.4+f.motion.phase)*1.6*height_/635.);float size=std::max(20.f,worldScale()*30.f);icon("ui/egg.png",{p.x-size*.5f,p.y-size*.5f,size,size},opacity);return;}
 const auto art=fishArt(s.id);auto* t=texture(art.substr(0,art.size()-4)+(f.dead?"-dead.png":".png"));auto size=fishSize(s,f);double width=size.x,height=size.y;double angle=f.dead?pi+pose.pitch:-pose.facing*pose.pitch;
 const bool sick=appearance==Care::Sick;
 const double amp=f.dead?0:width*.025*std::min(1.15,pose.speed/50.)*(held?.45:sick?.18:1.)*(reduced?.25:1.);
 const bool koi=s.id=="koi",flashlight=s.id=="flashlightFish"&&!f.dead&&!reduced;
 const auto patches=koi?koiPatches(f.id):std::array<InkPatch,5>{};
 // More vertical samples localize these cosmetic effects to scales and the
 // cheek. Texture alpha clips every patch to the existing swimming mesh.
 const int rows=koi||flashlight?13:2;
 const double lamp=.5-.5*std::cos(std::max(0.,time)*2*pi/6+patternUnit(f.id.value)*2*pi);
 std::vector<SDL_Vertex> vertices;std::vector<int> idx;vertices.reserve(24*rows);idx.reserve(23*(rows-1)*6);
 const SDL_FColor careTint=sick?SDL_FColor{.62f,.82f,.45f,opacity}:SDL_FColor{1,1,1,opacity};
 for(int i=0;i<24;++i){
  const double u=double(i)/23.,xx=(u-.5)*width*pose.facing,wave=std::sin(pose.phase-u*4.7)*amp*std::pow(u,1.4);
  for(int row=0;row<rows;++row){
   const double v=double(row)/(rows-1),yy=(v-.5)*height+wave;
   const float vx=p.x+float(xx*std::cos(angle)-yy*std::sin(angle)),vy=p.y+float(xx*std::sin(angle)+yy*std::cos(angle));
   auto tint=careTint;
   if(koi)for(const auto& patch:patches){
    // Saved identity fixes each painted patch for the fish's whole life.
    // Multiply the authored shading so individual scales remain visible.
    const float ink=float(softSpot(u,v,patch.u,patch.v,patch.width,patch.height))*.65f;
    tint.r*=1-ink*(patch.red&&!f.dead?.42f:1.f);tint.g*=1-ink;tint.b*=1-ink;
   }
   if(flashlight){
    // A slow local light cycle uses the authored luminous cheek at its peak.
    // Reduced motion and dead fish keep their still artwork with no pulse.
    const float light=1-float(.28*(1-lamp)*softSpot(u,v,.24,.57,.12,.22));
    tint.r*=light;tint.g*=light;tint.b*=light;
   }
   vertices.push_back({{vx,vy},tint,{float(u),float(v)}});
   if(i<23&&row<rows-1){const int k=i*rows+row;idx.insert(idx.end(),{k,k+1,k+rows,k+1,k+rows+1,k+rows});}
  }
 }
 outline.a=Uint8(outline.a*opacity);
 if(outline.a>0){auto* mask=texture(art.substr(0,art.size()-4)+"-mask.png");for(int j=0;j<8;++j){double a=double(j)*pi/4;auto v=vertices;float d=selected?2.5f:1.8f;for(auto& x:v){x.position.x+=float(std::cos(a))*d;x.position.y+=float(std::sin(a))*d;x.color=color(outline);}mesh(mask->sampled(size.x*pixelScaleX_,size.y*pixelScaleY_),v,idx);}}
 mesh(t->sampled(size.x*pixelScaleX_,size.y*pixelScaleY_),vertices,idx);
}
void Canvas::softenScene(){
 // Reuse two small render targets. Progressive linear filtering keeps the
 // moving background soft without a CPU readback or a full-size blur pass.
 auto* source=frameTexture_;
 for(std::size_t i=0;i<focusTextures_.size();++i){
  const float divisor=i==0?2.f:6.f;
  const int w=std::max(1,int(std::ceil(width_/divisor))),h=std::max(1,int(std::ceil(height_/divisor)));
  auto*& target=focusTextures_[i];
  if(target&&(target->w!=w||target->h!=h)){SDL_DestroyTexture(target);target=nullptr;}
  if(!target){
   target=SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,w,h);
   if(!target)throw std::runtime_error(SDL_GetError());
   SDL_SetTextureScaleMode(target,SDL_SCALEMODE_LINEAR);SDL_SetTextureBlendMode(target,SDL_BLENDMODE_NONE);
  }
  if(!SDL_SetRenderTarget(renderer_,target)||!SDL_SetRenderScale(renderer_,1,1)||!SDL_RenderTexture(renderer_,source,nullptr,nullptr))throw std::runtime_error(SDL_GetError());
  source=target;
 }
 if(!SDL_SetRenderTarget(renderer_,frameTexture_)||!SDL_SetRenderScale(renderer_,pixelScaleX_,pixelScaleY_))throw std::runtime_error(SDL_GetError());
 const SDL_FRect area{0,0,width_,height_};
 if(!SDL_RenderTexture(renderer_,source,nullptr,&area))throw std::runtime_error(SDL_GetError());
 fill({0,0,width_,height_},{12,42,52,64});
}
void Canvas::scene(const Domain& d,double interpolation,double time,Tool tool,FishId held,bool careBadges,FishId hidden,const Decoration* preview,std::uint64_t selectedDecor,bool previewHighlight,bool backgroundArtwork){
 const bool focus=preview&&previewHighlight;
 const bool selling=tool==Tool::Sell&&!focus;
 if(backgroundArtwork){const auto* tank=d.tank(d.state().activeTank);environment({0,0,width_,height_},tank?tank->backgroundId:"sunlit-lagoon");}
 if(d.state().settings.tankLook==1)fill({0,0,width_,height_},{232,174,141,27});if(d.state().settings.tankLook==2)fill({0,0,width_,height_},{57,49,112,45});

 std::vector<const Decoration*> decor;for(const auto& item:d.state().decor)if(item.tank==d.state().activeTank&&!item.stored&&(!preview||preview->id!=item.id))decor.push_back(&item);
 if(preview)decor.push_back(preview);
 std::sort(decor.begin(),decor.end(),[](const auto* a,const auto* b){return decorBehind(*a,*b);});
 const Decoration* selection=focus?preview:nullptr;
 if(!selection&&selectedDecor)for(const auto* item:decor)if(item->id==selectedDecor){selection=item;break;}
 DecorDef legacy;legacy.width=1.1;legacy.height=.95;
 const auto decorBounds=[&](const Decoration& item){const auto* def=d.content().findDecor(item.kind);return decorRect(def?*def:legacy,item.position,item.sizeMul);};
 const Rect selectedBounds=selection?decorBounds(*selection):Rect{};
 const auto coversSelection=[&](Rect r){return selection&&r.x<selectedBounds.x+selectedBounds.w&&r.x+r.w>selectedBounds.x&&r.y<selectedBounds.y+selectedBounds.h&&r.y+r.h>selectedBounds.y;};
 std::vector<std::uint64_t> animated;int emitters=0,fxBudget=d.content().decorTuning.particleLimit;
 if(!d.state().settings.reducedMotion){
  auto priority=decor;std::stable_sort(priority.begin(),priority.end(),[&](const auto* a,const auto* b){const auto* da=d.content().findDecor(a->kind);const auto* db=d.content().findDecor(b->kind);return (da&&da->art.value("motion_class","")=="Feature")>(db&&db->art.value("motion_class","")=="Feature");});
  for(const auto* item:priority){if(int(animated.size())>=d.content().decorTuning.animatedLimit)break;const auto* def=d.content().findDecor(item->kind);if(!def||def->art.value("motion_class","")=="Static")continue;const auto r=decorRect(*def,item->position,item->sizeMul);if(r.x+r.w<=0||r.y+r.h<=0||r.x>=width_||r.y>=height_)continue;animated.push_back(item->id);}
 }
 // Keep the selected item at its actual depth. Only overlapping items in
 // front become translucent, so the player can see both the item and its layer.
 clip({0,0,width_,height_});
 const auto drawDecor=[&](const Decoration* item){
  const auto* source=d.content().findDecor(item->kind);DecorDef legacy;
  if(!source){legacy.id=item->kind;legacy.asset="decor/"+item->kind+".png";legacy.width=1.1;legacy.height=.95;legacy.art=Json::object();source=&legacy;}
  const auto& def=*source;const bool motion=std::find(animated.begin(),animated.end(),item->id)!=animated.end();
  const bool emitter=def.art.value("motion",Json::object()).value("kind","")=="bubble";
  const bool allow=!emitter||emitters<d.content().decorTuning.emitterLimit;if(motion&&emitter&&allow)++emitters;
  const float opacity=selection&&decorBehind(*selection,*item)&&coversSelection(decorBounds(*item))?.22f:1.f;
  decoration(def,*item,time,motion,opacity,&fxBudget,allow,item==selection);
 };
 for(const auto* item:decor)drawDecor(item);
 clearClip();
 const auto drawPellets=[&]{
  for(auto& p:d.pellets()){auto at=toScreen(p.hasPrevious?WorldPoint{p.previous.x+(p.position.x-p.previous.x)*interpolation,p.previous.y+(p.position.y-p.previous.y)*interpolation}:p.position);float radius=std::max(4.f,6*worldScale());float alpha=p.rest<=30?1.f:float((31.8-p.rest)/1.8);if(coversSelection({at.x-radius,at.y-radius,radius*2,radius*2}))alpha*=.22f;round({at.x-radius,at.y-radius,radius*2,radius*2},{166,105,65,static_cast<Uint8>(255*alpha)},radius,{113,77,53,static_cast<Uint8>(255*alpha)},1,false);}
 };
 // Sell keeps its own backdrop effect. Decoration placement stays sharp.
 if(selling){drawPellets();softenScene();}
 const auto drawFish=[&](const Fish& f){if(!f.stashed&&f.tank==d.state().activeTank&&f.id!=hidden){auto& s=*d.content().find(f.species);Color outline{0,0,0,0};if(tool==Tool::Sell&&sellable(s,f))outline={255,219,119,255};if(f.id==held&&tool!=Tool::Select)outline={183,255,249,255};
  const auto pose=fishPose(f,interpolation);const auto at=toScreen(pose.position);const auto size=f.egg?SDL_FPoint{std::max(20.f,worldScale()*30.f),std::max(20.f,worldScale()*30.f)}:fishSize(s,f);
  const float w=std::abs(float(std::cos(pose.pitch)))*size.x+std::abs(float(std::sin(pose.pitch)))*size.y;
  const float h=std::abs(float(std::sin(pose.pitch)))*size.x+std::abs(float(std::cos(pose.pitch)))*size.y+size.x*.06f;
  const float opacity=coversSelection({at.x-w*.5f,at.y-h*.5f,w,h})?.22f:1.f;
  fish(s,f,interpolation,f.id==held,outline,f.id==held,d.state().settings.reducedMotion,careOf(s,f,d.state().simNow),time,opacity);
  Care care=careOf(s,f,d.state().simNow);if(careBadges&&opacity==1&&!f.egg&&!f.dead&&care!=Care::Fed&&f.id!=held){auto p=toScreen(pose.position);float offset=fishSize(s,f).x*.22f;float x=std::clamp(p.x-float(pose.facing*std::cos(pose.pitch))*offset,38.f,width_-38.f);float y=p.y+float(std::sin(pose.pitch)*std::abs(pose.facing))*offset-28;bool ill=care==Care::Sick;round({x-28,y,56,18},ill?Color{178,218,127,255}:care==Care::Urgent?Color{248,158,115,255}:Color{248,220,125,255},9,{65,102,100,255},1,false);text(ill?"SICK":"HUNGRY",x,y+1,10,{54,79,83,255},true,50,true);}
 }};
 for(const auto& fish:d.state().fish)drawFish(fish);
 if(!selling)drawPellets();
 clip({0,0,width_,height_});
 for(const auto& ripple:d.ripples()){
  const auto at=toScreen(ripple.position);const bool reduced=d.state().settings.reducedMotion;
  const double age=ripple.previousAge+(ripple.age-ripple.previousAge)*std::clamp(interpolation,0.,1.);
  for(int ring=0;ring<(reduced?1:3);++ring){
   const double progress=(age-ring*.15)/(reduced?WaterRipple::lifetime:1.);
   if(progress<0||progress>=1)continue;
   const float radius=float(reduced?30.:14.+114.*(1.-std::pow(1.-progress,2)))*worldScale();
   const float opacity=float(1.-progress);
   const Rect bounds{at.x-radius,at.y-radius,radius*2,radius*2};
   outline(bounds,{13,99,143,Uint8(175*opacity)},radius,6.f*worldScale());
   outline(bounds,{224,255,255,Uint8(245*opacity)},radius,3.f*worldScale());
  }
 }
 clearClip();
}
bool Canvas::capture(const std::filesystem::path& path,bool windowPixels){
 // Both targets have the window's native pixel size, with no letterbox layer.
 if(windowPixels)composeWindow();
 auto* surface=SDL_RenderReadPixels(renderer_,nullptr);
 if(windowPixels){SDL_SetRenderTarget(renderer_,frameTexture_);SDL_SetRenderScale(renderer_,pixelScaleX_,pixelScaleY_);}
 if(!surface)return false;
 bool ok=IMG_SavePNG(surface,path.string().c_str());SDL_DestroySurface(surface);return ok;
}
void Canvas::sound(double frequency,float volume){if(!audio_||volume<=0||SDL_GetAudioStreamQueued(audio_)>44100)return;std::array<float,4410> samples{};for(std::size_t i=0;i<samples.size();++i){double t=double(i)/44100.;samples[i]=float(std::sin(2*pi*(frequency+30*std::sin(t*20))*t)*std::exp(-t*48)*.08*volume);}SDL_PutAudioStreamData(audio_,samples.data(),static_cast<int>(samples.size()*sizeof(float)));}
}
