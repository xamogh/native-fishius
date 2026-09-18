#include "aquarium/hud_rewards.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace aq {
namespace {
constexpr double burstTime=.30,flightTime=.58,countTime=.18,pulseTime=.42;
constexpr float pi=std::numbers::pi_v<float>;
int tokenCount(Amount amount,bool xp){return int(std::clamp<Amount>(amount,0,xp?5:7));}
Amount tokenAmount(Amount amount,int count,int index){return amount/count+(index<amount%count?1:0);}
double delayFor(int index,bool xp){return index*.045+(xp?.09:0);}
double arrivalFor(int index,bool xp){return delayFor(index,xp)+burstTime+flightTime+index*.015;}
double smooth(double value){const double t=std::clamp(value,0.,1.);return t*t*(3-2*t);}
float effectUnit(Canvas& canvas,const HudLayout& hud){return std::max(hud.unit,canvas.minimumTouchSize()/88.f);}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
SDL_FPoint targetFor(const HudLayout& hud,bool xp){
 if(!xp)return center(hud[HudPart::CoinIcon]);
 const auto bar=hud[HudPart::Xp];return {bar.x+std::min(28*hud.unit,bar.w*.5f),bar.y+bar.h*.5f};
}
SDL_FPoint tokenPosition(Canvas& canvas,const HudLayout& hud,WorldPoint origin,int index,int count,bool xp,double age){
 const float u=effectUnit(canvas,hud);const auto start=canvas.toScreen(origin),target=targetFor(hud,xp);const auto safe=canvas.safeInsets();
 const float fan=float(index)-float(count-1)*.5f;
 SDL_FPoint spread{start.x+(fan*23+(xp?-34:26))*u,start.y-(64+12*(index%3))*u};
 spread.x=std::clamp(spread.x,safe.left+20*u,canvas.width()-safe.right-20*u);
 spread.y=std::clamp(spread.y,safe.top+20*u,canvas.height()-safe.bottom-20*u);
 const double local=age-delayFor(index,xp);
 if(local<burstTime){
  const float t=float(std::clamp(local/.24,0.,1.)),ease=1-(1-t)*(1-t)*(1-t);
  return {start.x+(spread.x-start.x)*ease,start.y+(spread.y-start.y)*ease};
 }
 const float progress=float(std::clamp((local-burstTime)/(flightTime+index*.015),0.,1.));
 const float t=progress*progress,a=1-t;
 const SDL_FPoint c1{spread.x+(xp?-40:40)*u,std::max(safe.top+16*u,spread.y-80*u)};
 const SDL_FPoint c2{target.x+(spread.x-target.x)*.18f,target.y+80*u};
 return {a*a*a*spread.x+3*a*a*t*c1.x+3*a*t*t*c2.x+t*t*t*target.x,
         a*a*a*spread.y+3*a*a*t*c1.y+3*a*t*t*c2.y+t*t*t*target.y};
}
void star(Canvas& canvas,SDL_FPoint at,float radius,float angle,Color color){
 std::array<SDL_FPoint,10> points{};
 for(int i=0;i<10;++i){const float a=-pi*.5f+angle+i*pi/5,r=radius*(i%2?.48f:1.f);points[i]={at.x+std::cos(a)*r,at.y+std::sin(a)*r};}
 for(int i=0;i<10;++i)canvas.triangle({at,points[i],points[(i+1)%10]},color);
}
void sparkle(Canvas& canvas,SDL_FPoint at,float radius,Color color){
 canvas.triangle({SDL_FPoint{at.x,at.y-radius},{at.x+radius*.45f,at.y},{at.x,at.y+radius}},color);
 canvas.triangle({SDL_FPoint{at.x,at.y-radius},{at.x-radius*.45f,at.y},{at.x,at.y+radius}},color);
}
}

void HudRewards::collect(const Event& event,bool reducedMotion){
 if((event.kind!="sale"&&event.kind!="level")||(event.coins<=0&&event.xp<=0))return;
 // Bound rendering work during a run of quick sales. Removing an old burst
 // reveals its already-saved balance immediately rather than losing a reward.
 if(bursts_.size()>=16)bursts_.erase(bursts_.begin());
 bursts_.push_back({event.position,std::max<Amount>(0,event.coins),std::max<Amount>(0,event.xp),0,reducedMotion,event.kind=="sale"});
}
void HudRewards::advance(double seconds,bool reducedMotion){
 const double elapsed=std::isfinite(seconds)?std::max(0.,seconds):0;
 for(auto& burst:bursts_){
  if(reducedMotion&&!burst.reduced){burst.reduced=true;burst.age=0;}
  burst.age+=elapsed;
 }
 std::erase_if(bursts_,[](const auto& burst){return burst.age>=(burst.reduced?.45:1.8);});
}
HudRewardDisplay HudRewards::display(const Domain& domain)const{
 HudRewardDisplay result{domain.state().wallet.coins,double(domain.state().xp),0,0,domain.state().settings.reducedMotion};
 long double coinsPending=0,xpPending=0;
 for(const auto& burst:bursts_){
  for(bool xp:{false,true}){
   const Amount amount=xp?burst.xp:burst.coins;const int count=tokenCount(amount,xp);
   for(int i=0;i<count;++i){
    const bool reduced=burst.reduced||result.reducedMotion;
    const double sinceArrival=burst.age-(reduced?0:arrivalFor(i,xp));
    const long double pending=reduced?0:tokenAmount(amount,count,i)*(1-static_cast<long double>(smooth(sinceArrival/countTime)));
    (xp?xpPending:coinsPending)+=pending;
    float pulse=0;
    if(sinceArrival>=0&&sinceArrival<pulseTime){
     const float t=float(sinceArrival/pulseTime);
     pulse=reduced?1-t:std::sin(pi*std::min(1.f,t*2.5f))*(1-t);
     // Keep a soft glow after the initial bounce.
     pulse=std::max(pulse,(1-t)*.35f);
    }
    auto& destination=xp?result.xpPulse:result.coinPulse;destination=std::max(destination,pulse);
   }
  }
 }
 // A player may spend a committed reward while its icons are still flying.
 // Apply live debits immediately and keep the displayed amount nonnegative.
 result.coins-=static_cast<Amount>(std::ceil(std::clamp(coinsPending,0.L,static_cast<long double>(result.coins))));
 result.xp=std::max(0.,result.xp-double(xpPending));
 return result;
}
void HudRewards::paint(Canvas& canvas,const HudLayout& hud)const{
 // Keep moving rewards readable even when a narrow viewport shrinks the HUD.
 const float u=effectUnit(canvas,hud);
 for(const auto& burst:bursts_){
  if(burst.reduced)continue;
  if(burst.label&&burst.age<.85){
   const auto source=canvas.toScreen(burst.origin);const auto safe=canvas.safeInsets();
   std::string label;
   if(burst.coins)label="+"+compact(burst.coins)+" coins";
   if(burst.xp)label+=(label.empty()?"":"  ")+std::string("+")+compact(burst.xp)+" XP";
   const Uint8 alpha=Uint8(255*(1-smooth((burst.age-.45)/.4)));
   const float x=std::clamp(source.x,safe.left+150*u,canvas.width()-safe.right-150*u);
   const float y=std::clamp(source.y+(28-float(burst.age)*18)*u,safe.top+16*u,canvas.height()-safe.bottom-40*u);
   canvas.text(label,x+u,y+2*u,26*u,{14,48,57,alpha},true,300*u,true,true);
   canvas.text(label,x,y,26*u,{255,249,204,alpha},true,300*u,true,true);
  }
  for(bool xp:{false,true}){
   const int count=tokenCount(xp?burst.xp:burst.coins,xp);
   const Color tint=xp?Color{124,235,255,255}:Color{255,227,119,255};
   for(int i=0;i<count;++i){
    const double local=burst.age-delayFor(i,xp),arrival=arrivalFor(i,xp);
    if(local<0)continue;
    if(burst.age>=arrival){
     const float t=float((burst.age-arrival)/.28);
     if(t>=1)continue;
     const auto at=targetFor(hud,xp);
     for(int ray=0;ray<4;++ray){
      const float a=ray*pi*.5f+.3f,r=(12+22*t)*u;
      sparkle(canvas,{at.x+std::cos(a)*r,at.y+std::sin(a)*r},(1-t)*7*u,{tint.r,tint.g,tint.b,Uint8(210*(1-t))});
     }
     continue;
    }
    const auto at=tokenPosition(canvas,hud,burst.origin,i,count,xp,burst.age);
    const float absorption=float(smooth((burst.age-arrival+.14)/.14));
    const float pop=float(smooth(local/.10));
    const float size=(xp?38:40)*u*(.45f+.55f*pop)*(1-.72f*absorption);
    const float alpha=pop*(1-.65f*absorption);
    if(local>burstTime){
     for(int trail=3;trail>=1;--trail){
      const auto p=tokenPosition(canvas,hud,burst.origin,i,count,xp,std::max(delayFor(i,xp),burst.age-trail*.027));
      const float r=(4-trail)*2.5f*u;
      canvas.round({p.x-r,p.y-r,2*r,2*r},{tint.r,tint.g,tint.b,Uint8((65-trail*14)*alpha)},r,{},0,false,false);
     }
    }
    canvas.round({at.x-size*.65f,at.y-size*.65f,size*1.3f,size*1.3f},{tint.r,tint.g,tint.b,Uint8(28*alpha)},size*.65f,{},0,false,false);
    const float angle=std::sin(float(local)*8+i)*.18f;
    if(xp){
     star(canvas,{at.x,at.y+u},size*.55f,angle,{13,77,121,Uint8(245*alpha)});
     star(canvas,at,size*.47f,angle,{66,199,246,Uint8(255*alpha)});
     star(canvas,{at.x-size*.02f,at.y-size*.035f},size*.33f,angle,{178,248,255,Uint8(255*alpha)});
    }else{
     // The same gold coin artwork as the destination, including its alpha.
     const float scale=size/1081.f;
     canvas.image("hud-icons/coin-v4.png",{at.x-size*.5f-87*scale,at.y-size*.5f-91*scale,1254*scale,1254*scale},angle*180/pi,{.5f,.5f},alpha);
    }
    sparkle(canvas,{at.x-size*.23f,at.y-size*.26f},size*.105f,{255,255,242,Uint8(235*alpha)});
   }
  }
 }
}
}
