#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
void Canvas::loadingScreen(float progress,std::string_view stage,double seconds,bool artwork,bool reduced){
 begin();progress=std::clamp(progress,0.f,1.f);
 const float w=width_,h=height_,cx=w*.5f;
 const float u=std::min(w/1608.f,h/830.f);
 const double time=reduced?0:seconds;
 fill({0,0,w,h},{3,51,88,255});
 if(artwork){
  auto* reef=texture("lagoon/reef.png");
  const float cover=std::max(w/reef->width,h/reef->height);
  image("lagoon/reef.png",{(w-reef->width*cover)*.5f,h-reef->height*cover,reef->width*cover,reef->height*cover});
 }
 // A soft bottom shade keeps the tip and progress readable over the sand.
 const SDL_FColor top{2/255.f,25/255.f,58/255.f,.035f},bottom{2/255.f,25/255.f,58/255.f,.6f};
 const std::array<SDL_Vertex,4> shade{{{{0,0},top,{}},{{w,0},top,{}},{{w,h},bottom,{}},{{0,h},bottom,{}}}};
 const std::array<int,6> indices{0,1,2,0,2,3};mesh(nullptr,shade,indices);
 if(artwork){
  const float bob=float(std::sin(time*1.3))*8*u;
  icon("species/yellowTang.png",{cx-510*u,h*.42f+bob,258*u,202*u},1,true);
  icon("species/ocellarisClownfish.png",{cx+264*u,h*.44f-bob*.7f,227*u,153*u});
  icon("species/neonTetra.png",{cx+186*u,h*.32f+bob*.5f,126*u,67*u},.9f);
 }
 for(int i=0;i<14;++i){
  const float x=w*(.12f+.76f*float((i*7)%17)/17);
  const float travel=float(std::fmod(time*(.021+i%3*.005)+double(i)*.173,1.));
  const float y=h*(1-travel),radius=(3+i%4*2)*u;
  round({x-radius,y-radius,radius*2,radius*2},{203,251,255,12},radius,{209,254,255,Uint8(38+i%3*14)},1*u,false);
 }
 const float titleY=h*.20f;
 label("FISHIUS",cx,titleY+5*u,96*u,true,700*u,{1,36,74,220},5*u,{1,36,74,220});
 text("FISHIUS",cx,titleY,96*u,{255,242,177,255},true,700*u,true,false,true);
 label("A Q U A R I U M",cx,titleY+112*u,29*u,true,570*u,{236,255,255,255},2*u,{4,72,112,255});
 text("A little ocean. A world of wonder.",cx,titleY+168*u,23*u,{205,247,252,255},true,660*u);

 const auto safe=safeInsets();
 const float barWidth=std::min(720*u,w-safe.left-safe.right-96*u);
 const float barY=std::min(h*.79f,h-safe.bottom-135*u);
 text(stage,cx,barY-44*u,25*u,{238,255,255,255},true,barWidth);
 const Rect track{cx-barWidth*.5f,barY,barWidth,42*u};
 round({track.x-4*u,track.y-4*u,track.w+8*u,track.h+8*u},{5,44,69,255},25*u,{103,196,211,255},2*u,true);
 round(track,{3,31,49,255},21*u,{3,31,49,255},0,false);
 const float fillWidth=(track.w-10*u)*progress;
 if(fillWidth>0){
  const Rect meter{track.x+5*u,track.y+5*u,fillWidth,track.h-10*u};
  round(meter,{104,211,63,255},16*u,{197,249,111,255},1*u,false);
  if(fillWidth>20*u)round({meter.x+5*u,meter.y+4*u,meter.w-10*u,7*u},{224,255,169,145},4*u,{},0,false);
 }
 label(std::to_string(int(progress*100))+"%",cx,barY+4*u,24*u,true,120*u,{255,255,240,255},1.5f*u,{12,64,46,255});
 text("TIP  Swipe the shop to discover more fish, plants and treasures.",cx,barY+68*u,20*u,{209,242,243,255},true,std::min(1000*u,w-safe.left-safe.right-60*u));
}
} // namespace aq
