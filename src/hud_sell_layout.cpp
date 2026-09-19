#include "aquarium/hud_care.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
namespace {
struct FishSpace {
 FishId id;
 SDL_FPoint preferred,position,size;
};

// Resolve crowding locally, keeping the tank's existing composition. A dense
// group may still overlap; it must never trigger a new row or grid layout.
void spread(std::vector<FishSpace>& fish,Rect area,float gap,float maxMove){
 auto clamp=[&](FishSpace& f){
  const float hx=std::min(f.size.x*.5f,area.w*.5f),hy=std::min(f.size.y*.5f,area.h*.5f);
  f.position.x=std::clamp(f.position.x,area.x+hx,area.x+area.w-hx);
  f.position.y=std::clamp(f.position.y,area.y+hy,area.y+area.h-hy);
 };
 for(auto& f:fish){f.position=f.preferred;clamp(f);f.preferred=f.position;}
 std::vector<SDL_FPoint> pushes(fish.size());
 std::vector<int> neighbors(fish.size());
 for(int pass=0;pass<80;++pass){
  std::fill(pushes.begin(),pushes.end(),SDL_FPoint{});
  std::fill(neighbors.begin(),neighbors.end(),0);
  for(std::size_t i=0;i<fish.size();++i)for(std::size_t j=0;j<i;++j){
   const auto& a=fish[i];const auto& b=fish[j];
   const float rx=(a.size.x+b.size.x)*.5f+gap,ry=(a.size.y+b.size.y)*.5f+gap;
   float dx=(a.position.x-b.position.x)/rx,dy=(a.position.y-b.position.y)/ry;
   const float distance=std::hypot(dx,dy);
   if(distance>=1)continue;
   if(distance>.001f){dx/=distance;dy/=distance;}
   else{
    // Give coincident fish a stable direction without changing their order.
    const float angle=float((a.id.value*37+b.id.value*17)%360)*.017453293f;
    dx=std::cos(angle);dy=std::sin(angle);
   }
   const float strength=(1-distance)*.5f;
   const SDL_FPoint push{dx*rx*strength,dy*ry*strength};
   pushes[i].x+=push.x;pushes[i].y+=push.y;++neighbors[i];
   pushes[j].x-=push.x;pushes[j].y-=push.y;++neighbors[j];
  }
  float largest=0;
  for(std::size_t i=0;i<fish.size();++i){
   auto& f=fish[i];const auto before=f.position;
   // Share the adjustment and lightly pull back toward the original spot.
   const float weight=.65f/std::max(1,neighbors[i]);
   f.position.x+=pushes[i].x*weight+(f.preferred.x-f.position.x)*.025f;
   f.position.y+=pushes[i].y*weight+(f.preferred.y-f.position.y)*.025f;
   const float dx=f.position.x-f.preferred.x,dy=f.position.y-f.preferred.y,distance=std::hypot(dx,dy);
   if(distance>maxMove)f.position={f.preferred.x+dx*maxMove/distance,f.preferred.y+dy*maxMove/distance};
   clamp(f);largest=std::max(largest,std::hypot(f.position.x-before.x,f.position.y-before.y));
  }
  if(largest<.05f)break;
 }
}
}

void HudCare::arrangeForSale(bool force){
 auto& domain=session_.domain();const auto& state=domain.state();
 const auto safe=canvas_.safeInsets();
 const std::array viewport{canvas_.width(),canvas_.height(),safe.left,safe.top,safe.right,safe.bottom};
 bool changed=force||viewport!=sellViewport_;
 auto available=[&](const Fish& f){return !f.egg&&!f.dead&&!f.stashed&&f.tank==state.activeTank;};
 for(const auto& f:state.fish)if(available(f)&&!f.motion.sellTarget)changed=true;
 if(!changed)return;
 sellViewport_=viewport;

 const auto hud=layoutHud(canvas_.width(),canvas_.height(),safe,canvas_.minimumTouchSize());
 const float density=canvas_.minimumTouchSize()/44.f,gap=8*density;
 const float left=std::max(safe.left+44*density,hud[HudPart::Tank].x+hud[HudPart::Tank].w+gap);
 float right=canvas_.width()-safe.right-44*density;
 for(const auto part:{HudPart::Layout,HudPart::Rehome,HudPart::Food,HudPart::Projects,HudPart::Bag,HudPart::Rewards,HudPart::Shop})
  right=std::min(right,hud[part].x-gap);
 const float top=std::max(safe.top+44*density,hud[HudPart::Profile].y+hud[HudPart::Profile].h+gap);
 const float bottom=std::min(canvas_.height()-safe.bottom-44*density,doneBounds().y-gap);
 const Rect area{left,top,std::max(1.f,right-left),std::max(1.f,bottom-top)};
 std::vector<FishSpace> fish;
 auto add=[&](const Fish& f){
  if(!available(f))return;
  const auto size=canvas_.fishSize(*domain.content().find(f.species),f);
  const float unit=std::max(.9f*density,canvas_.worldScale());
  // Reserve the hit target, stage meter and reward ribbon around each fish.
  const SDL_FPoint space{std::max(canvas_.minimumTouchSize(),size.x+32*canvas_.worldScale()),
   std::max(canvas_.minimumTouchSize(),size.y+(f.favorite?48:32)*unit)};
  fish.push_back({f.id,canvas_.toScreen(f.motion.sellTarget.value_or(f.position)),{},space});
 };
 for(const auto& f:state.fish)add(f);
 // Allow the same local adjustment relative to the larger fish artwork.
 spread(fish,area,gap,canvas_.minimumTouchSize()*float(tankArtScale));
 std::vector<SellTarget> targets;targets.reserve(fish.size());
 for(const auto& f:fish)targets.push_back({f.id,canvas_.toWorld(f.position.x,f.position.y)});
 domain.arrangeForSale(targets);
}
}
