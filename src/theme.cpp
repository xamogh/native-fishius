#include "aquarium/theme.hpp"
#include <array>
#include <cmath>

namespace aq {
namespace {
struct Material {Color top,bottom,edge,light;};
Material material(std::string_view name) {
 if(name=="green")return {{85,228,47,255},{14,174,51,255},{5,121,52,255},{196,255,154,255}};
 if(name=="gold-round")return {{255,247,101,255},{255,191,33,255},{17,104,130,255},{255,255,216,255}};
 if(name=="gold")return {{255,247,101,255},{255,202,43,255},{232,164,37,255},{255,255,216,255}};
 if(name=="red"||name=="close")return {{255,128,99,255},{250,60,57,255},{184,54,45,255},{255,205,169,255}};
 if(name=="nav")return {{7,133,174,255},{3,76,112,255},{3,87,115,255},{70,189,214,255}};
 if(name=="nav-card")return {{255,254,248,255},{255,243,210,255},{17,104,130,255},{255,255,255,255}};
 if(name=="card"||name=="tab")return {{252,255,255,255},{229,245,244,255},{175,219,229,255},{255,255,255,255}};
 if(name=="coin-card")return {{255,254,236,255},{255,246,202,255},{237,217,140,255},{255,255,255,255}};
 if(name=="pearl-card")return {{250,248,255,255},{233,241,255,255},{194,205,233,255},{255,255,255,255}};
 if(name=="bundle-card")return {{251,242,255,255},{246,229,244,255},{214,191,222,255},{255,255,255,255}};
 if(name=="ribbon")return {{255,127,111,255},{245,79,87,255},{191,62,69,255},{255,212,187,255}};
 if(name=="disabled")return {{171,204,210,255},{127,174,184,255},{97,143,155,255},{220,241,239,255}};
 if(name=="track")return {{181,229,221,255},{157,213,204,255},{156,215,208,255},{209,247,237,255}};
 if(name=="cyan")return {{76,232,255,255},{0,179,242,255},{0,139,202,255},{179,255,255,255}};
 return {{60,213,255,255},{0,150,245,255},{0,121,186,255},{173,245,255,255}};
}
SDL_FColor rgba(Color c,float alpha=1){return {c.r/255.f,c.g/255.f,c.b/255.f,c.a/255.f*alpha};}
Color mix(Color a,Color b,float t){return {Uint8(a.r+(b.r-a.r)*t),Uint8(a.g+(b.g-a.g)*t),Uint8(a.b+(b.b-a.b)*t),255};}
}

// One rounded material renderer serves tabs, utility controls, cards and
// secondary actions. The API name is retained for existing native callers.
void Canvas::facet(std::string_view name,Rect r,float alpha){
 if(r.w<=0||r.h<=0)return;
 r.x+=originX_;r.y+=originY_;
 const auto m=material(name);
 const bool card=name=="card"||name=="tab"||name.ends_with("-card");
 const float radius=(name=="gold-round"||name=="close"||name=="arrow")?std::min(r.w,r.h)*.5f:
  name=="nav-card"?std::min(r.w,r.h)*.40f:
  name=="nav"&&r.w>r.h*2? r.h*.5f:std::min(r.w*.22f,r.h*(card?.16f:.30f));
 const float edge=std::max(.8f,std::min(r.w,r.h)*(name=="nav-card"?.028f:card?.010f:.027f));
 const auto shape=[&](Rect p,float rad,Color top,Color bottom){
  constexpr int segments=16,ring=4*(segments+1);
  rad=std::max(0.f,std::min({rad,p.w*.5f,p.h*.5f}));
  const std::array<SDL_FPoint,4> centers{{{p.x+p.w-rad,p.y+rad},{p.x+p.w-rad,p.y+p.h-rad},{p.x+rad,p.y+p.h-rad},{p.x+rad,p.y+rad}}};
  std::array<SDL_Vertex,1+ring*2> vertices;
  vertices[0]={{p.x+p.w*.5f,p.y+p.h*.5f},rgba(mix(top,bottom,.5f),alpha),{}};
  const float fringe=1.f/std::max(pixelScaleX_,pixelScaleY_);
  for(int corner=0;corner<4;++corner)for(int i=0;i<=segments;++i){
   const float angle=(-90.f+corner*90.f+i*90.f/segments)*.01745329252f;
   const float nx=std::cos(angle),ny=std::sin(angle);
   const SDL_FPoint point{centers[corner].x+nx*rad,centers[corner].y+ny*rad};
   auto color=rgba(mix(top,bottom,std::clamp((point.y-p.y)/p.h,0.f,1.f)),alpha);
   const int index=1+corner*(segments+1)+i;
   vertices[index]={point,color,{}};color.a=0;
   vertices[index+ring]={{point.x+nx*fringe,point.y+ny*fringe},color,{}};
  }
  std::vector<int> indices;indices.reserve(ring*9);
  for(int i=1;i<=ring;++i){const int next=i==ring?1:i+1;indices.insert(indices.end(),{0,i,next,i,next,i+ring,next,next+ring,i+ring});}
  mesh(nullptr,vertices,indices);
 };
 shape({r.x,r.y+edge*1.2f,r.w,r.h},radius,m.edge,m.edge);
 shape(r,radius,m.edge,m.edge);
 const Rect face{r.x+edge,r.y+edge,r.w-2*edge,r.h-3*edge};
 shape(face,std::max(0.f,radius-edge),m.light,m.bottom);
 const float inset=std::max(.5f,edge*.6f);
 shape({face.x+inset,face.y+inset,face.w-2*inset,face.h-2*inset},std::max(0.f,radius-edge-inset),m.top,m.bottom);
}

void Canvas::symbol(std::string_view name,Rect r,Color c){
 const auto polygon=[&](std::span<const SDL_FPoint> points,Color color){
  std::vector<SDL_Vertex> v;for(auto at:points)v.push_back({{r.x+at.x*r.w+originX_,r.y+at.y*r.h+originY_},rgba(color),{}});
  std::vector<int> ix;for(int i=1;i+1<int(v.size());++i){ix.push_back(0);ix.push_back(i);ix.push_back(i+1);}mesh(nullptr,v,ix);
 };
 if(name=="play"||name=="next"||name=="previous"){
  const bool left=name=="previous";
  const std::array<SDL_FPoint,3> p=left?std::array<SDL_FPoint,3>{{{.72f,.05f},{.12f,.5f},{.72f,.95f}}}:std::array<SDL_FPoint,3>{{{.24f,.05f},{.88f,.5f},{.24f,.95f}}};
  if(name=="play"){
   auto shadow=r;shadow.y+=r.h*.06f;symbol("next",shadow,{0,91,201,255});
   polygon(p,theme::teal);
  }else polygon(p,c);
 }else if(name=="close"){
  const std::array<SDL_FPoint,4> a{{{.08f,.25f},{.25f,.08f},{.92f,.75f},{.75f,.92f}}};
  const std::array<SDL_FPoint,4> b{{{.75f,.08f},{.92f,.25f},{.25f,.92f},{.08f,.75f}}};polygon(a,c);polygon(b,c);
 }else if(name=="add"){
  fill({r.x+r.w*.37f,r.y,r.w*.26f,r.h},c);fill({r.x,r.y+r.h*.37f,r.w,r.h*.26f},c);
 }else if(name=="mail"){
  const std::array<SDL_FPoint,4> p{{{0,.12f},{1,.12f},{1,.87f},{0,.87f}}};polygon(p,c);
  const std::array<SDL_FPoint,3> gap{{{0,.17f},{.5f,.61f},{1,.17f}}};polygon(gap,{21,63,135,255});
  const std::array<SDL_FPoint,3> flap{{{0,.1f},{.5f,.49f},{1,.1f}}};polygon(flap,c);
 }
}

void View::closeButton(std::string id,Rect r,std::function<void()> action){
 const auto visual=buttonVisual(id,r);
 canvas_.skin("close",visual,visual.h*.5f);
 canvas_.icon("skin/control-close.png",{visual.x+visual.w*.27f,visual.y+visual.h*.23f,visual.w*.46f,visual.h*.46f});
 const float growX=std::max(0.f,(canvas_.minimumTouchSize()-r.w)*.5f),growY=std::max(0.f,(canvas_.minimumTouchSize()-r.h)*.5f);
 touchTarget(std::move(id),{r.x-growX,r.y-growY,r.w+2*growX,r.h+2*growY},std::move(action));
}

void View::titleSign(Rect r,std::string_view title,std::string_view icon){
 canvas_.skin("wood",r);
 const float inset=icon.empty()?r.w*.06f:r.h*.98f;
 if(!icon.empty())canvas_.icon(icon,{r.x-r.h*.08f,r.y-r.h*.11f,r.h*1.12f,r.h*1.16f});
 canvas_.label(title,r.x+inset+(r.w-inset)*.5f,r.y+r.h*.17f,r.h*.58f,true,r.w-inset-r.w*.06f,theme::ink);
}

void View::shopHeader(Rect r,std::string_view subtitle){
 titleSign(r,"Shop","lagoon/shop.png");
 canvas_.text(subtitle,r.x+r.w+24,r.y+r.h*.56f,24,theme::body,false,470,true);
}
}
