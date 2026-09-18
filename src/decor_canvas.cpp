#include "aquarium/canvas.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
void Canvas::decorPlacementMarker(Rect item){
 const float unit=minimumTouchSize()/44.f;
 const float cx=item.x+item.w*.5f,cy=item.y+item.h-3*unit;
 const float rx=std::max(item.w*.58f,34*unit),ry=std::max(12*unit,rx*.24f);
 const auto diamond=[&](float x,float y,float w,float h,SDL_FColor color){
  const SDL_Vertex vertices[]={{{x,y-h},color,{}},{{x+w,y},color,{}},{{x,y+h},color,{}},{{x-w,y},color,{}}};
  const int indices[]={0,1,2,0,2,3};SDL_RenderGeometry(renderer_,nullptr,vertices,4,indices,6);
 };
 diamond(cx,cy,rx,ry,{.5f,1.f,.25f,.75f});
 diamond(cx,cy,rx-3*unit,ry-2*unit,{.1f,.78f,.16f,.55f});
 const auto arrow=[&](float x,float y,float dx,float dy){
  const float length=8*unit;const SDL_FColor c{.7f,1.f,.34f,1};
  const SDL_Vertex v[]={{{x+dx*length,y+dy*length},c,{}},{{x-dy*length-dx*length*.5f,y+dx*length-dy*length*.5f},c,{}},{{x+dy*length-dx*length*.5f,y-dx*length-dy*length*.5f},c,{}}};
  SDL_RenderGeometry(renderer_,nullptr,v,3,nullptr,0);
 };
 arrow(cx-rx-12*unit,cy,-1,0);arrow(cx+rx+12*unit,cy,1,0);
 arrow(cx,cy-ry-12*unit,0,-1);arrow(cx,cy+ry+12*unit,0,1);
}

namespace {
constexpr double pi=3.14159265358979323846;
double smooth(double t){t=std::clamp(t,0.,1.);return t*t*(3-2*t);}
double phase(std::uint64_t id){return double((id*2654435761ULL)%10000)/10000.;}
double period(const Json& m,std::uint64_t id){if(m.contains("period_range"))return m["period_range"][0].get<double>()+(m["period_range"][1].get<double>()-m["period_range"][0].get<double>())*phase(id);return m.value("seconds",10.);}
double beat(double t,double p,double hold){const double cycle=std::fmod(t,p);const double ramp=1.;if(cycle<ramp)return smooth(cycle/ramp);if(cycle<ramp+hold)return 1.;if(cycle<2*ramp+hold)return 1-smooth((cycle-ramp-hold)/ramp);return 0.;}
}
DecorVertexPose decorVertexPose(const DecorDef& d,std::uint64_t copy,double time,double u,double v,bool animate){
 DecorVertexPose out{u,v,1};if(!animate||!d.art.contains("motion"))return out;
 const auto& m=d.art["motion"];const auto kind=m.value("kind",std::string("static"));if(kind=="static")return out;
 const double p=period(m,copy),t=std::max(0.,time)+phase(copy)*p,angle=m.value("degrees",0.)*pi/180.;
 const double group=std::min(2.,std::floor(u*3)),wave=std::sin(t*2*pi/p+group*.7),root=1-smooth((v-.5)/.48);
 double dx=0,dy=0;
 if(kind=="sway"||kind=="nod"||kind=="sail"){
  const double amount=angle*wave*root;dx=(1-v)*std::sin(amount)*d.height/d.width;dy=(u-.5)*std::sin(amount)*.25;
 }else if(kind=="open"){
  const double open=(d.id=="PP-04"||d.id=="PP-11"||d.id=="PL-12")?beat(t,p,m.value("hold_seconds",2.)):(1-std::cos(t*2*pi/p))*.5;
  dx=(u-.5)*std::sin(angle)*open*root;dy=-std::abs(u-.5)*std::sin(angle)*open*root;
 }else if(kind=="clam"||kind=="lid"){
  const double lid=(1-smooth((v-.48)/.2));const double opening=beat(t,p,m.value("hold_seconds",2.));
  const double lift=std::sin(kind=="clam"?(-8*pi/180.+angle*opening):angle*opening);dy=-lift*(.65-v)*lid;
 }else if(kind=="pincer"){
  const double part=(1-smooth((u-.32)/.13))*(1-smooth((v-.55)/.15));dy=-std::sin(angle)*part*beat(t,p,2.)*.45;
 }else if(kind=="chime"||kind=="bucket"){
  const double local=smooth((v-.24)/.15)*(1-smooth((v-.7)/.12));const double swing=kind=="bucket"?wave*beat(t,p,m.value("hold_seconds",4.)):wave;
  dx=std::sin(angle)*swing*local*.4;
 }else if(kind=="carousel"){
  dx=std::sin(angle)*wave*(1-smooth((v-.35)/.1))*.3;
  dy=std::sin(t*2*pi/9+group*2)*(.03/d.height)*smooth((v-.35)/.1)*(1-smooth((v-.72)/.1));
 }else if(kind=="ring"){
  const double a=angle*wave,mask=1-smooth((v-.73)/.15);const double x=u-.5,y=v-.43;
  dx=(x*std::cos(a)-y*std::sin(a)-x)*mask;dy=(x*std::sin(a)+y*std::cos(a)-y)*mask;
 }else if(kind=="bubble"&&d.id=="PL-05"){
  dx=std::sin(angle)*wave*(1-smooth((v-.3)/.15))*.2;
 }
 // Secondary movement stays local to the corresponding authored group.
 if(d.id=="PP-04"||d.id=="PP-11"||d.id=="PP-15"){
  const double leaf=smooth((v-.55)/.1)*(1-smooth((v-.86)/.1));
  dx+=std::sin(t*2*pi/10+group)*std::sin(pi/180.)*leaf*.25;
 }
 if(d.id=="PP-10")dx+=std::sin(t*2*pi/8)*std::sin(2*pi/180.)*smooth((u-.68)/.1)*(1-smooth((v-.6)/.2))*.3;
 if(d.id=="PP-12")dx+=std::sin(t*2*pi/12-.35)*std::sin(2*pi/180.)*smooth((v-.4)/.1)*(1-smooth((v-.68)/.1))*.25;
 if(d.id=="PP-14")dx+=std::sin(t*2*pi/24)*std::sin(2*pi/180.)*(1-smooth(std::abs(u-.5)/.14))*(1-smooth(v/.55))*.3;
 if(d.id=="PD-10")dx+=std::sin(t*2*pi/6)*std::sin(6*pi/180.)*beat(t,p,3)*(1-smooth(std::abs(u-.5)/.16))*smooth((v-.4)/.1)*(1-smooth((v-.7)/.1))*.3;
 if(d.id=="PD-04"){
  const double x=u-.66,y=v-.27,r=std::hypot(x,y);if(r<.1){const double a=pi*.5*beat(t,p,2);dx+=x*std::cos(a)-y*std::sin(a)-x;dy+=x*std::sin(a)+y*std::cos(a)-y;}
 }
 if(d.id=="PD-16")dx+=std::sin(t*2*pi/18)*.015*(1-smooth(std::abs(u-.5)/.12))*(1-smooth(std::abs(v-.3)/.12));
 const double brightness=m.value("brightness",0.);
 if(brightness>0){
  // Restrict changing colour to local tips/windows, never the whole tank.
  const double mask=d.category=="Plant"?(1-smooth((v-.22)/.35))*smooth((u-.1)/.12)*(1-smooth((u-.78)/.12)):
   smooth((u-.23)/.1)*(1-smooth((u-.67)/.1))*smooth((v-.22)/.08)*(1-smooth((v-.61)/.1));
  const double lp=m.contains("light_period_range")?m["light_period_range"][0].get<double>()+(m["light_period_range"][1].get<double>()-m["light_period_range"][0].get<double>())*phase(copy):m.value("light_seconds",p);
  const double light=d.id=="PP-15"?beat(t,24,2):(1-std::cos(t*2*pi/std::max(1.,lp)+group*1.4))*.5;
  out.light=1-brightness*mask*(1-light);
 }
 // The workbook's dimensions include all movement. Never drift roots or
 // enlarge a saved footprint. Boundary falloff also preserves alpha padding.
 const double edge=std::min({smooth(u/.1),smooth((1-u)/.1),smooth(v/.1),smooth((1-v)/.1)});
 out.x=std::clamp(u+dx*edge,0.,1.);out.y=std::clamp(v+dy*edge,0.,1.);
 return out;
}
Rect Canvas::decorRect(const DecorDef& d,WorldPoint position,double sizeMul)const{
 const auto p=toScreen(position);const float unit=std::min(width_/12.f,height_/7.f)*float(decorScale(position)*sizeMul);
 const float w=float(d.width)*unit,h=float(d.height)*unit;
 return {p.x-w*.5f,p.y-h,w,h};
}
void Canvas::decoration(const DecorDef& def,const Decoration& item,double time,bool animate,float alpha,int* fxBudget,bool allowEmitter,bool highlight){
 const auto r=decorRect(def,item.position,item.sizeMul);const auto m=def.art.value("motion",Json::object());const auto kind=m.value("kind",std::string("static"));
 // Screen a water-coloured silhouette over the sprite. Two built-in blend
 // passes also work with SDL's software renderer: dst*(1-haze) + haze.
 auto draw=[&](const std::string& asset,const std::vector<SDL_Vertex>& vertices,const std::vector<int>& indices){
  auto* mask=texture("mask:"+asset)->sampled(r.w*pixelScaleX_,r.h*pixelScaleY_);
  if(highlight){
   for(int i=0;i<8;++i){const double angle=i*pi/4;auto rim=vertices;for(auto& v:rim){v.position.x+=float(std::cos(angle))*2.5f*worldScale();v.position.y+=float(std::sin(angle))*2.5f*worldScale();v.color={1,.7f,.22f,.5f*alpha};}mesh(mask,rim,indices);}
  }
  mesh(texture(asset)->sampled(r.w*pixelScaleX_,r.h*pixelScaleY_),vertices,indices);
  const float haze=float(decorHaze(item.position));auto pass=vertices;
  for(auto& v:pass)v.color={1-95/255.f,1-196/255.f,1-220/255.f,v.color.a*haze};
  SDL_SetTextureBlendMode(mask,SDL_BLENDMODE_MUL);mesh(mask,pass,indices);
  for(auto& v:pass){v.color.r=95/255.f;v.color.g=196/255.f;v.color.b=220/255.f;}
  SDL_SetTextureBlendMode(mask,SDL_BLENDMODE_ADD);mesh(mask,pass,indices);SDL_SetTextureBlendMode(mask,SDL_BLENDMODE_BLEND);
 };
 auto sprite=[&](const std::string& asset,Rect rect,double degrees=0,SDL_FPoint pivot=SDL_FPoint{.5f,1}){
  const double angle=degrees*pi/180.;std::vector<SDL_Vertex> vertices;
  for(const auto uv:{SDL_FPoint{0,0},SDL_FPoint{1,0},SDL_FPoint{1,1},SDL_FPoint{0,1}}){
   const float x=(uv.x-pivot.x)*rect.w,y=(uv.y-pivot.y)*rect.h;
   vertices.push_back({{rect.x+pivot.x*rect.w+float(x*std::cos(angle)-y*std::sin(angle)),rect.y+pivot.y*rect.h+float(x*std::sin(angle)+y*std::cos(angle))},{1,1,1,alpha},{item.flipped?1-uv.x:uv.x,uv.y}});
  }
  draw(asset,vertices,{0,1,2,0,2,3});
 };
 auto intersectClip=[&](Rect area){
  if(SDL_RenderClipEnabled(renderer_)){SDL_Rect current{};SDL_GetRenderClipRect(renderer_,&current);const float right=std::min(area.x+area.w,float(current.x+current.w)),bottom=std::min(area.y+area.h,float(current.y+current.h));area.x=std::max(area.x,float(current.x));area.y=std::max(area.y,float(current.y));area.w=std::max(0.f,right-area.x);area.h=std::max(0.f,bottom-area.y);}
  clip(area);
 };
 if(!animate||kind=="static"){sprite(def.asset,r);return;}
 if(kind=="pinwheel"){
  sprite("decor/motion/PL-07-base.png",r);
  SDL_Rect previous{};const bool hadClip=SDL_RenderClipEnabled(renderer_);SDL_GetRenderClipRect(renderer_,&previous);
  intersectClip(r);const Rect wheel{r.x+r.w*(item.flipped?-.011f:.011f),r.y-r.h*.044f,r.w,r.h};sprite("decor/motion/PL-07-wheel.png",wheel,(item.flipped?-1:1)*(std::max(0.,time)/period(m,item.id)+phase(item.id))*360,{item.flipped?.499f:.501f,.439f});
  // Keep the authored ivory hub fixed above the rotating blade layer.
  constexpr int segments=32;std::vector<SDL_Vertex> hub;std::vector<int> hubIndices;
  const float hu=.512f,hv=.395f,rx=.100f,ry=rx*r.w/r.h;
  auto vertex=[&](float u,float v){return SDL_Vertex{{r.x+(item.flipped?1-u:u)*r.w,r.y+v*r.h},{1,1,1,alpha},{u,v}};};
  hub.push_back(vertex(hu,hv));for(int i=0;i<=segments;++i){const float a=float(i*2*pi/segments);hub.push_back(vertex(hu+rx*std::cos(a),hv+ry*std::sin(a)));if(i)hubIndices.insert(hubIndices.end(),{0,i,i+1});}
  draw(def.asset,hub,hubIndices);
  SDL_SetRenderClipRect(renderer_,hadClip?&previous:nullptr);return;
 }
 const auto asset=(kind=="internalBubble"||kind=="snowglobe")?"decor/motion/"+def.id+".png":def.asset;
 constexpr int nx=18,ny=20;std::vector<SDL_Vertex> vertices;std::vector<int> indices;vertices.reserve((nx+1)*(ny+1));indices.reserve(nx*ny*6);
 for(int y=0;y<=ny;++y)for(int x=0;x<=nx;++x){
  const double u=double(x)/nx,v=double(y)/ny,sourceU=item.flipped?1-u:u;const auto pose=decorVertexPose(def,item.id,time,sourceU,v,true);
  vertices.push_back({{r.x+float(item.flipped?1-pose.x:pose.x)*r.w,r.y+float(pose.y)*r.h},{float(pose.light),float(pose.light),float(pose.light),alpha},{float(sourceU),float(v)}});
  if(x<nx&&y<ny){const int at=y*(nx+1)+x;indices.insert(indices.end(),{at,at+1,at+nx+1,at+1,at+nx+2,at+nx+1});}
 }
 draw(asset,vertices,indices);
 if(!fxBudget||*fxBudget<=0)return;
 const double p=period(m,item.id),tm=std::max(0.,time)+phase(item.id)*p;
 if(kind=="bubble"&&allowEmitter){
  const double elapsed=std::fmod(tm,p),travel=m.value("travel_units",.5)*std::min(width_/12.f,height_/7.f),life=3.;
  if(elapsed<life){
   const float x=r.x+r.w*(def.id=="PL-05"?.82f:.5f),y=r.y+float(r.h*.2-travel*(elapsed/life));
   const float radius=std::min(r.w*.04f,5*worldScale());
   // Leave reserved space for HUD and stop the effect at the tank edge.
   if(x>radius&&x<width_-radius&&y>height_*.18f+radius){const auto a=Uint8(130*(1-elapsed/life)*alpha);round({x-radius,y-radius,radius*2,radius*2},{203,242,245,Uint8(a/3)},radius,{223,254,255,a},1,false);--*fxBudget;}
  }
 }
 if(kind=="internalBubble"||kind=="snowglobe"){
  const int count=kind=="snowglobe"?3:2;
  SDL_Rect previous{};const bool hadClip=SDL_RenderClipEnabled(renderer_);SDL_GetRenderClipRect(renderer_,&previous);
  for(int i=0;i<count&&*fxBudget>0;++i){
   const double cycle=std::fmod(tm/p+i*.37,1.);
   const std::array<SDL_FPoint,3> chipHomes{{{.29f,.44f},{.51f,.30f},{.73f,.45f}}};
   const float chamberY=kind=="snowglobe"?chipHomes[i].y:i==0?.34f:.70f;
   const float sourceX=kind=="snowglobe"?chipHomes[i].x:.5f;
   const float chamberX=item.flipped?1-sourceX:sourceX;
   const float x=r.x+r.w*(chamberX+(kind=="snowglobe"?.018f:.065f)*float(std::sin(cycle*2*pi+i))),
    y=r.y+r.h*chamberY+(kind=="snowglobe"?float(.5*std::sin(cycle*2*pi)):float(.5-cycle))*float(m.value("travel_units",.15)*std::min(width_/12.f,height_/7.f));
   const float radius=std::max(1.f,std::min(r.w*.016f,3.f*worldScale()));
   // Clip each reusable chip/bubble to a conservative rectangle entirely
   // inside the authored glass. The lower hourglass chamber excludes its neck.
   const Rect interior{r.x+r.w*(chamberX-.085f),r.y+r.h*(chamberY-.075f),r.w*.17f,r.h*.15f};
   SDL_SetRenderClipRect(renderer_,hadClip?&previous:nullptr);intersectClip(interior);
   if(kind=="snowglobe"){
    // A tiny six-lobed chip, reused three times inside the dome.
    for(int pass=0;pass<2;++pass){std::vector<SDL_Vertex> chip;std::vector<int> triangles;const SDL_FColor color=pass?SDL_FColor{.74f,.66f,.9f,alpha}:SDL_FColor{.37f,.33f,.55f,alpha};const float scale=radius*(pass?.68f:1.f);
     chip.push_back({{x,y},color,{0,0}});for(int j=0;j<=24;++j){const float a=float(j*2*pi/24),rr=scale*(j%4==0?1.f:j%4==2?.62f:.48f);chip.push_back({{x+rr*std::cos(a),y+rr*std::sin(a)},color,{0,0}});if(j)triangles.insert(triangles.end(),{0,j,j+1});}mesh(nullptr,chip,triangles);
    }
   }
   else{const float fade=float(std::min({1.,cycle*8,(1-cycle)*8}));round({x-radius,y-radius,radius*2,radius*2},{194,185,224,Uint8(150*alpha*fade)},radius,{226,220,246,Uint8(200*alpha*fade)},1,false);}
   --*fxBudget;
  }
  SDL_SetRenderClipRect(renderer_,hadClip?&previous:nullptr);
 }
}
}
