#include "aquarium/view.hpp"
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
std::filesystem::path pickFont(const std::filesystem::path& assets,bool display){
 const char* env=std::getenv(display?"AQUARIUM_DISPLAY_FONT":"AQUARIUM_BODY_FONT");if(env&&std::filesystem::exists(env))return env;
 std::vector<std::filesystem::path> candidates={assets/"fonts"/(display?"LilitaOne-Regular.ttf":"Nunito-SemiBold.ttf"),"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf","/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf","/System/Library/Fonts/Supplemental/Arial.ttf","/Library/Fonts/Arial.ttf","C:/Windows/Fonts/arial.ttf","/system/fonts/Roboto-Regular.ttf","/System/Library/Fonts/CoreUI/SFUI.ttf"};
 for(auto& p:candidates)if(std::filesystem::exists(p))return p;return {};
}
}
std::string compact(Amount n){std::string s=std::to_string(n);std::size_t sign=s[0]=='-'?1:0;for(std::ptrdiff_t i=static_cast<std::ptrdiff_t>(s.size())-3;i>static_cast<std::ptrdiff_t>(sign);i-=3)s.insert(static_cast<std::size_t>(i),",");return s;}
std::string durationText(Millis ms){if(ms<0)ms=0;if(ms<60000)return std::to_string((ms+999)/1000)+"s";if(ms<3600000)return std::to_string((ms+30000)/60000)+"m";if(ms<86400000)return std::to_string((ms+1800000)/3600000)+"h";return std::to_string((ms+43200000)/86400000)+"d";}
double bezierOvershoot(double progress){
 const double x=std::clamp(progress,0.,1.);auto sample=[](double t,double a,double b){double u=1-t;return 3*u*u*t*a+3*u*t*t*b+t*t*t;};double lo=0,hi=1,t=x;
 for(int i=0;i<20;++i){if(sample(t,.34,.64)<x)lo=t;else hi=t;t=(lo+hi)*.5;}return sample(t,1.56,1.);
}
Canvas::Canvas(std::filesystem::path assets,int w,int h,bool software):assets_(std::move(assets)){
 if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))throw std::runtime_error(SDL_GetError());if(!TTF_Init())throw std::runtime_error(SDL_GetError());
 SDL_SetHint(SDL_HINT_ORIENTATIONS,"LandscapeLeft LandscapeRight");
 window_=SDL_CreateWindow("FishX Aquarium",w,h,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY);if(!window_)throw std::runtime_error(SDL_GetError());
 renderer_=SDL_CreateRenderer(window_,software?"software":nullptr);if(!renderer_)throw std::runtime_error(SDL_GetError());SDL_SetRenderVSync(renderer_,1);SDL_SetRenderDrawBlendMode(renderer_,SDL_BLENDMODE_BLEND);
 fontPath_=pickFont(assets_,false);displayFontPath_=pickFont(assets_,true);if(fontPath_.empty())throw std::runtime_error("No native font found. Run tools/setup_fonts.py or set AQUARIUM_BODY_FONT.");if(displayFontPath_.empty())displayFontPath_=fontPath_;
 SDL_AudioSpec spec{SDL_AUDIO_F32,1,44100};audio_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);if(audio_)SDL_ResumeAudioStreamDevice(audio_);begin();
}
Canvas::~Canvas(){if(frameTexture_)SDL_DestroyTexture(frameTexture_);textCache_.clear();textures_.clear();for(auto& [_,f]:fonts_)TTF_CloseFont(f);if(audio_)SDL_DestroyAudioStream(audio_);if(renderer_)SDL_DestroyRenderer(renderer_);if(window_)SDL_DestroyWindow(window_);TTF_Quit();SDL_Quit();}
void Canvas::begin(){
 int w=1,h=1;SDL_GetWindowSize(window_,&w,&h);windowWidth_=float(w);windowHeight_=float(h);originX_=0;
 // The design space is 1608x830. A display wider than that ratio grows the
 // canvas horizontally instead of letterboxing, so the game fills the screen.
 height_=830;width_=std::max(1608.f,std::round(height_*windowWidth_/std::max(1.f,windowHeight_)));
 float fit=std::min(windowWidth_/width_,windowHeight_/height_);
 displayRect_={(windowWidth_-width_*fit)*.5f,(windowHeight_-height_*fit)*.5f,width_*fit,height_*fit};
 SDL_Rect safe{0,0,w,h};SDL_GetWindowSafeArea(window_,&safe);
 if(safe.w<=0||safe.h<=0)safe={0,0,w,h};
 auto toCanvasX=[&](float px){return (px-displayRect_.x)*width_/displayRect_.w;};
 auto toCanvasY=[&](float py){return (py-displayRect_.y)*height_/displayRect_.h;};
 safeInsets_={std::max(0.f,toCanvasX(float(safe.x))),std::max(0.f,toCanvasY(float(safe.y))),
              std::max(0.f,width_-toCanvasX(float(safe.x+safe.w))),std::max(0.f,height_-toCanvasY(float(safe.y+safe.h)))};
 int needW=int(std::lround(width_*2)),needH=int(std::lround(height_*2));
 if(frameTexture_&&(needW!=frameWidth_||needH!=frameHeight_)){SDL_DestroyTexture(frameTexture_);frameTexture_=nullptr;}
 if(!frameTexture_){frameTexture_=SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,needW,needH);if(!frameTexture_)throw std::runtime_error(SDL_GetError());SDL_SetTextureScaleMode(frameTexture_,SDL_SCALEMODE_LINEAR);frameWidth_=needW;frameHeight_=needH;}
 SDL_SetRenderTarget(renderer_,frameTexture_);SDL_SetRenderScale(renderer_,2,2);SDL_SetRenderDrawColor(renderer_,0,130,190,255);SDL_RenderClear(renderer_);++frame_;
}
void Canvas::present(){
 SDL_SetRenderTarget(renderer_,nullptr);int pw=1,ph=1;SDL_GetWindowSizeInPixels(window_,&pw,&ph);SDL_SetRenderScale(renderer_,float(pw)/windowWidth_,float(ph)/windowHeight_);
 SDL_SetRenderDrawColor(renderer_,0,91,139,255);SDL_RenderClear(renderer_);
 image("aquarium/reef.png",{0,0,windowWidth_,windowHeight_});
 SDL_FRect src{0,0,float(frameWidth_),float(frameHeight_)},dest{displayRect_.x,displayRect_.y,displayRect_.w,displayRect_.h};SDL_RenderTexture(renderer_,frameTexture_,&src,&dest);SDL_RenderPresent(renderer_);
}
SDL_FPoint Canvas::inputPoint(float x,float y,bool normalized)const{
 if(normalized){x*=windowWidth_;y*=windowHeight_;}
 return {(x-displayRect_.x)*width_/displayRect_.w,(y-displayRect_.y)*height_/displayRect_.h};
}
WorldPoint Canvas::toWorld(float x,float y)const{return {double(x/width_)*1088.,double(y/height_)*635.};}
SDL_FPoint Canvas::toScreen(WorldPoint p)const{return {float(p.x/1088.)*width_,float(p.y/635.)*height_};}
void Canvas::fill(Rect r,Color c){r.x+=originX_;SDL_SetRenderDrawColor(renderer_,c.r,c.g,c.b,c.a);SDL_FRect rect{r.x,r.y,r.w,r.h};SDL_RenderFillRect(renderer_,&rect);}
void Canvas::mesh(SDL_Texture* t,std::span<const SDL_Vertex> v,std::span<const int> indices){if(!SDL_RenderGeometry(renderer_,t,v.data(),static_cast<int>(v.size()),indices.data(),static_cast<int>(indices.size())))throw std::runtime_error(SDL_GetError());}
void Canvas::round(Rect rect,Color c,float radius,Color border,float thickness,bool shadow){
 if(rect.w<=0||rect.h<=0)return;
 rect.x+=originX_;
 auto shape=[&](Rect r,Color col,float rad,bool gradient){rad=std::min({rad,r.w*.5f,r.h*.5f});std::vector<SDL_Vertex> v;v.reserve(45);v.push_back({{r.x+r.w*.5f,r.y+r.h*.5f},color(col),{}});std::array<SDL_FPoint,4> centers{{{r.x+r.w-rad,r.y+rad},{r.x+r.w-rad,r.y+r.h-rad},{r.x+rad,r.y+r.h-rad},{r.x+rad,r.y+rad}}};
  for(int corner=0;corner<4;++corner)for(int k=0;k<=8;++k){double a=(-90.+double(corner)*90.+double(k)*90./8.)*pi/180.;float x=centers[corner].x+float(std::cos(a))*rad,y=centers[corner].y+float(std::sin(a))*rad;auto fc=color(col);if(gradient){float mult=1.f-.10f*(y-r.y)/std::max(r.h,1.f);fc.r*=mult;fc.g*=mult;fc.b*=mult;}v.push_back({{x,y},fc,{}});}
  std::vector<int> ix;for(int k=1;k<static_cast<int>(v.size())-1;++k){ix.push_back(0);ix.push_back(k);ix.push_back(k+1);}ix.insert(ix.end(),{0,static_cast<int>(v.size())-1,1});mesh(nullptr,v,ix);
 };
 if(shadow){shape({rect.x,rect.y+4,rect.w,rect.h},{23,73,94,65},radius,false);}
 if(thickness>0){shape(rect,border,radius,false);rect={rect.x+thickness,rect.y+thickness,rect.w-2*thickness,rect.h-2*thickness};radius=std::max(0.f,radius-thickness);}shape(rect,c,radius,true);
 if(rect.h>22&&c.a>100){shape({rect.x+3,rect.y+3,rect.w-6,std::min(9.f,rect.h*.18f)},{255,255,255,62},std::min(6.f,radius),false);}
}
Texture* Canvas::texture(std::string_view key){
 auto it=textures_.find(std::string(key));if(it!=textures_.end())return it->second.get();auto path=assets_/key;SDL_Surface* surface=IMG_Load(path.string().c_str());if(!surface)throw std::runtime_error("Asset load failed: "+path.string()+": "+SDL_GetError());auto value=std::make_unique<Texture>();value->width=surface->w;value->height=surface->h;value->value=SDL_CreateTextureFromSurface(renderer_,surface);SDL_DestroySurface(surface);if(!value->value)throw std::runtime_error(SDL_GetError());SDL_SetTextureBlendMode(value->value,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(value->value,SDL_SCALEMODE_LINEAR);auto* result=value.get();textures_.emplace(key,std::move(value));return result;
}
TTF_Font* Canvas::font(int pixels,bool display){int key=pixels+(display?1000:0);auto i=fonts_.find(key);if(i!=fonts_.end())return i->second;auto path=display?displayFontPath_:fontPath_;auto* f=TTF_OpenFont(path.string().c_str(),float(pixels));if(!f)throw std::runtime_error(SDL_GetError());fonts_[key]=f;return f;}
void Canvas::text(std::string_view label,float x,float y,float size,Color c,bool center,float maxWidth,bool display){
 if(label.empty())return;
 x+=originX_;int pixels=std::clamp(static_cast<int>(std::round(size)),9,96);std::string key=std::to_string(pixels)+":"+std::to_string(c.r)+","+std::to_string(c.g)+","+std::to_string(c.b)+","+std::to_string(c.a)+(display?":d:":":b:")+std::string(label);
 auto it=textCache_.find(key);if(it==textCache_.end()){
  if(textCache_.size()>=1536){auto oldest=std::min_element(textCache_.begin(),textCache_.end(),[](auto& a,auto& b){return a.second.used<b.second.used;});textCache_.erase(oldest);}
  SDL_Color col{c.r,c.g,c.b,c.a};auto* sf=TTF_RenderText_Blended(font(pixels,display),label.data(),label.size(),col);if(!sf)throw std::runtime_error(SDL_GetError());auto t=std::make_unique<Texture>();t->width=sf->w;t->height=sf->h;t->value=SDL_CreateTextureFromSurface(renderer_,sf);SDL_DestroySurface(sf);if(!t->value)throw std::runtime_error(SDL_GetError());it=textCache_.emplace(key,TextEntry{std::move(t),frame_}).first;
 }
 it->second.used=frame_;auto& t=*it->second.texture;float factor=maxWidth>0&&float(t.width)>maxWidth?maxWidth/float(t.width):1.f;SDL_FRect dst{x-(center?float(t.width)*factor*.5f:0),y,float(t.width)*factor,float(t.height)*factor};SDL_RenderTexture(renderer_,t.value,nullptr,&dst);
}
void Canvas::image(std::string_view name,Rect r,double angle,SDL_FPoint pivot,float alpha){r.x+=originX_;auto* t=texture(name);SDL_FRect dst{r.x,r.y,r.w,r.h};SDL_FPoint p{pivot.x*r.w,pivot.y*r.h};SDL_SetTextureAlphaModFloat(t->value,std::clamp(alpha,0.f,1.f));SDL_RenderTextureRotated(renderer_,t->value,nullptr,&dst,angle,&p,SDL_FLIP_NONE);SDL_SetTextureAlphaModFloat(t->value,1.f);}
SDL_FPoint Canvas::fishSize(const Species& s,const Fish& f){
 auto name=fishArt(s.id,f.id.value);auto* art=texture(name);
 // Source sprites use the same pixel density as the supplied reference.
 const double adultScale=.88*1.21;
 float w=name.starts_with("aquarium/")?float(art->width)*width_/2498.f*float(ageScale(s,f)/adultScale):float(s.nominalLength*ageScale(s,f)*worldScale()*2.);
 return {w,w*float(art->height)/float(art->width)};
}
void Canvas::fish(const Species& s,const Fish& f,double alpha,bool held,Color outline,bool selected,bool reduced,Care appearance){
 WorldPoint wp{f.motion.previous.x+(f.position.x-f.motion.previous.x)*alpha,f.motion.previous.y+(f.position.y-f.motion.previous.y)*alpha};auto p=toScreen(wp);if(f.egg){float size=std::max(13.f,worldScale()*18.f);image("ui/egg.png",{p.x-size*.5f,p.y-size*.65f,size,size*1.2f});return;}
 const auto& m=f.motion;const auto art=fishArt(s.id,f.id.value);auto* t=texture(art.substr(0,art.size()-4)+(f.dead?"-dead.png":".png"));auto size=fishSize(s,f);double width=size.x,height=size.y;double turn=m.turnRemaining>0?.12+.88*std::abs(std::cos(pi*(1-m.turnRemaining/m.turnDuration))):1.;double angle=f.dead?pi+m.pitch:double(m.direction)*m.pitch;
 const bool sick=appearance==Care::Sick;
 const double amp=f.dead?0:width*.025*std::min(1.15,m.speed/50.)*(held?.45:sick?.18:1.)*(reduced?.25:1.);
 std::vector<SDL_Vertex> vertices;std::vector<int> idx;vertices.reserve(48);idx.reserve(138);SDL_FColor tint=sick?SDL_FColor{.62f,.82f,.45f,1}:SDL_FColor{1,1,1,1};
 for(int i=0;i<24;++i){double u=double(i)/23.;double xx=(u-.5)*width*turn*(m.direction==1?-1:1);double wave=std::sin(m.phase-u*4.7)*amp*std::pow(u,1.4);for(int row=0;row<2;++row){double yy=(double(row)-.5)*height+wave;float vx=p.x+float(xx*std::cos(angle)-yy*std::sin(angle));float vy=p.y+float(xx*std::sin(angle)+yy*std::cos(angle));vertices.push_back({{vx,vy},tint,{float(u),float(row)}});}if(i<23){int k=i*2;idx.insert(idx.end(),{k,k+1,k+2,k+1,k+3,k+2});}}
 if(outline.a>0){auto* mask=texture(art.substr(0,art.size()-4)+"-mask.png");for(int j=0;j<8;++j){double a=double(j)*pi/4;auto v=vertices;float d=selected?2.5f:1.8f;for(auto& x:v){x.position.x+=float(std::cos(a))*d;x.position.y+=float(std::sin(a))*d;x.color=color(outline);}mesh(mask->value,v,idx);}}
 mesh(t->value,vertices,idx);
}
void Canvas::scene(const Domain& d,double interpolation,double time,Tool tool,FishId held,bool careBadges){
 auto* background=texture("aquarium/reef.png");float cover=std::max(width_/float(background->width),height_/float(background->height));float bw=float(background->width)*cover,bh=float(background->height)*cover;image("aquarium/reef.png",{(width_-bw)*.5f,height_-bh,bw,bh});
 if(d.state().settings.tankLook==1)fill({0,0,width_,height_},{232,174,141,27});if(d.state().settings.tankLook==2)fill({0,0,width_,height_},{57,49,112,45});

 for(auto& x:d.state().decor)if(x.tank==d.state().activeTank){auto p=toScreen(x.position);float size=100*worldScale();image("decor/"+x.kind+".png",{p.x-size*.5f,p.y-size*.86f,size,size*.86f});}
 for(auto& f:d.state().fish)if(!f.stashed&&f.tank==d.state().activeTank){auto& s=*d.content().find(f.species);Color outline{0,0,0,0};if(tool==Tool::Sell&&sellable(s,f))outline={255,219,119,255};if(f.id==held)outline={183,255,249,255};fish(s,f,interpolation,f.id==held,outline,f.id==held,d.state().settings.reducedMotion,careOf(s,f,d.state().simNow));
  Care care=careOf(s,f,d.state().simNow);if(careBadges&&!f.egg&&!f.dead&&care!=Care::Fed&&f.id!=held){auto p=toScreen(f.position);float turn=f.motion.turnRemaining>0?float(.12+.88*std::abs(std::cos(pi*(1-f.motion.turnRemaining/f.motion.turnDuration)))):1.f;float offset=float(s.nominalLength*ageScale(s,f)*.44)*worldScale()*turn;float x=std::clamp(p.x+float(f.motion.direction)*float(std::cos(f.motion.pitch))*offset,38.f,width_-38.f);float y=p.y+float(std::sin(f.motion.pitch))*offset-28;bool ill=care==Care::Sick;round({x-28,y,56,18},ill?Color{178,218,127,255}:care==Care::Urgent?Color{248,158,115,255}:Color{248,220,125,255},9,{65,102,100,255},1,false);text(ill?"SICK":"HUNGRY",x,y+1,10,{54,79,83,255},true,50,true);}
 }
 for(auto& p:d.pellets()){auto at=toScreen(p.position);float radius=std::max(4.f,6*worldScale());float alpha=p.rest<=30?1.f:float((31.8-p.rest)/1.8);round({at.x-radius,at.y-radius,radius*2,radius*2},{166,105,65,static_cast<Uint8>(255*alpha)},radius,{113,77,53,static_cast<Uint8>(255*alpha)},1,false);}
}
bool Canvas::capture(const std::filesystem::path& path){auto* surface=SDL_RenderReadPixels(renderer_,nullptr);if(!surface)return false;bool ok=IMG_SavePNG(surface,path.string().c_str());SDL_DestroySurface(surface);return ok;}
void Canvas::sound(double frequency,float volume){if(!audio_||volume<=0||SDL_GetAudioStreamQueued(audio_)>44100)return;std::array<float,4410> samples{};for(std::size_t i=0;i<samples.size();++i){double t=double(i)/44100.;samples[i]=float(std::sin(2*pi*(frequency+30*std::sin(t*20))*t)*std::exp(-t*48)*.08*volume);}SDL_PutAudioStreamData(audio_,samples.data(),static_cast<int>(samples.size()*sizeof(float)));}
}
