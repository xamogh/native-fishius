#include "aquarium/hud_rewards.hpp"
#include "aquarium/hud_dialog.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace aq {
namespace {
constexpr double burstTime=.30,flightTime=.58,countTime=.18,pulseTime=.42;
constexpr float pi=std::numbers::pi_v<float>;
enum class Token {Coins,Xp,Pearls};
constexpr std::array tokens{Token::Coins,Token::Xp,Token::Pearls};
int tokenCount(Amount amount,Token token){return int(std::clamp<Amount>(amount,0,token==Token::Xp?5:7));}
Amount tokenAmount(Amount amount,int count,int index){return amount/count+(index<amount%count?1:0);}
double delayFor(int index,Token token){return index*.045+(token==Token::Xp?.09:token==Token::Pearls?.18:0);}
double arrivalFor(int index,Token token){return delayFor(index,token)+burstTime+flightTime+index*.015;}
double smooth(double value){const double t=std::clamp(value,0.,1.);return t*t*(3-2*t);}
float effectUnit(Canvas& canvas,const HudLayout& hud){return std::max(hud.unit,canvas.minimumTouchSize()/88.f);}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
SDL_FPoint targetFor(const HudLayout& hud,Token token){
 if(token!=Token::Xp)return center(hud[token==Token::Pearls?HudPart::PearlIcon:HudPart::CoinIcon]);
 const auto bar=hud[HudPart::Xp];return {bar.x+std::min(28*hud.unit,bar.w*.5f),bar.y+bar.h*.5f};
}
SDL_FPoint tokenPosition(Canvas& canvas,const HudLayout& hud,WorldPoint origin,int index,int count,Token token,double age){
 const bool xp=token==Token::Xp;
 const float u=effectUnit(canvas,hud);const auto start=canvas.toScreen(origin),target=targetFor(hud,token);const auto safe=canvas.safeInsets();
 const float fan=float(index)-float(count-1)*.5f;
 SDL_FPoint spread{start.x+(fan*23+(xp?-34:26))*u,start.y-(64+12*(index%3))*u};
 spread.x=std::clamp(spread.x,safe.left+20*u,canvas.width()-safe.right-20*u);
 spread.y=std::clamp(spread.y,safe.top+20*u,canvas.height()-safe.bottom-20*u);
 const double local=age-delayFor(index,token);
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
 // Level bonuses leave the receipt after it has been visible for half a second.
 if(event.kind!="sale"||(event.coins<=0&&event.xp<=0&&event.pearls<=0))return;
 // Bound rendering work during a run of quick sales. Removing an old burst
 // reveals its already-saved balance immediately rather than losing a reward.
 if(bursts_.size()>=16)bursts_.erase(bursts_.begin());
 bursts_.push_back({event.position,std::max<Amount>(0,event.coins),std::max<Amount>(0,event.xp),std::max<Amount>(0,event.pearls),0,reducedMotion,event.kind=="sale"});
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
 HudRewardDisplay result{.coins=domain.state().wallet.coins,.pearls=domain.state().wallet.pearls,
  .xp=double(domain.state().xp),.reducedMotion=domain.state().settings.reducedMotion};
 long double coinsPending=0,xpPending=0,pearlsPending=0;
 for(const auto& burst:bursts_){
  for(const auto token:tokens){
   const Amount amount=token==Token::Xp?burst.xp:token==Token::Pearls?burst.pearls:burst.coins;const int count=tokenCount(amount,token);
   for(int i=0;i<count;++i){
    const bool reduced=burst.reduced||result.reducedMotion;
    const double sinceArrival=burst.age-(reduced?0:arrivalFor(i,token));
    const long double pending=reduced?0:tokenAmount(amount,count,i)*(1-static_cast<long double>(smooth(sinceArrival/countTime)));
    (token==Token::Xp?xpPending:token==Token::Pearls?pearlsPending:coinsPending)+=pending;
    float pulse=0;
    if(sinceArrival>=0&&sinceArrival<pulseTime){
     const float t=float(sinceArrival/pulseTime);
     pulse=reduced?1-t:std::sin(pi*std::min(1.f,t*2.5f))*(1-t);
     // Keep a soft glow after the initial bounce.
     pulse=std::max(pulse,(1-t)*.35f);
    }
    auto& destination=token==Token::Xp?result.xpPulse:token==Token::Pearls?result.pearlPulse:result.coinPulse;destination=std::max(destination,pulse);
   }
  }
 }
 // A player may spend a committed reward while its icons are still flying.
 // Apply live debits immediately and keep the displayed amount nonnegative.
 result.coins-=static_cast<Amount>(std::ceil(std::clamp(coinsPending,0.L,static_cast<long double>(result.coins))));
 result.pearls-=static_cast<Amount>(std::ceil(std::clamp(pearlsPending,0.L,static_cast<long double>(result.pearls))));
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
   if(burst.pearls)label+=(label.empty()?"":"  ")+std::string("+")+compact(burst.pearls)+(burst.pearls==1?" pearl":" pearls");
   const Uint8 alpha=Uint8(255*(1-smooth((burst.age-.45)/.4)));
   const float x=std::clamp(source.x,safe.left+150*u,canvas.width()-safe.right-150*u);
   const float y=std::clamp(source.y+(28-float(burst.age)*18)*u,safe.top+16*u,canvas.height()-safe.bottom-40*u);
   canvas.text(label,x+u,y+2*u,26*u,{14,48,57,alpha},true,300*u,true,true);
   canvas.text(label,x,y,26*u,{255,249,204,alpha},true,300*u,true,true);
  }
  for(const auto token:tokens){
   const bool xp=token==Token::Xp,pearl=token==Token::Pearls;
   const int count=tokenCount(xp?burst.xp:pearl?burst.pearls:burst.coins,token);
   const Color tint=xp?Color{124,235,255,255}:pearl?Color{243,202,251,255}:Color{255,227,119,255};
   for(int i=0;i<count;++i){
    const double local=burst.age-delayFor(i,token),arrival=arrivalFor(i,token);
    if(local<0)continue;
    if(burst.age>=arrival){
     const float t=float((burst.age-arrival)/.28);
     if(t>=1)continue;
     const auto at=targetFor(hud,token);
     for(int ray=0;ray<4;++ray){
      const float a=ray*pi*.5f+.3f,r=(12+22*t)*u;
      sparkle(canvas,{at.x+std::cos(a)*r,at.y+std::sin(a)*r},(1-t)*7*u,{tint.r,tint.g,tint.b,Uint8(210*(1-t))});
     }
     continue;
    }
    const auto at=tokenPosition(canvas,hud,burst.origin,i,count,token,burst.age);
    const float absorption=float(smooth((burst.age-arrival+.14)/.14));
    const float pop=float(smooth(local/.10));
    const float size=(xp?38:40)*u*(.45f+.55f*pop)*(1-.72f*absorption);
    const float alpha=pop*(1-.65f*absorption);
    if(local>burstTime){
     for(int trail=3;trail>=1;--trail){
      const auto p=tokenPosition(canvas,hud,burst.origin,i,count,token,std::max(delayFor(i,token),burst.age-trail*.027));
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
    }else if(pearl){
     canvas.image("hud-icons/pearl-v4.png",{at.x-size*.5f,at.y-size*.5f,size,size},angle*180/pi,{.5f,.5f},alpha);
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
HudDialogLayout layoutPearlProgress(float width,float height,Insets safe,float minimumTouch){
 // Canvas units are larger than window points on phones. Use the touch scale
 // so portrait labels remain readable instead of capping them at one unit.
 const float scale=minimumTouch/44.f;
 const float availableW=width-safe.left-safe.right-32*scale,availableH=height-safe.top-safe.bottom-32*scale;
 const float u=std::min({scale,availableW/640,availableH/600});
 const Rect frame{safe.left+(width-safe.left-safe.right-640*u)*.5f,safe.top+(height-safe.top-safe.bottom-600*u)*.5f,640*u,600*u};
 const float closeSize=std::max(minimumTouch,64*u),headerH=std::max(80*u,closeSize+16*u);
 const Rect header{frame.x+8*u,frame.y+8*u,frame.w-16*u,headerH};
 const Rect close{header.x+header.w-closeSize-8*u,header.y+(header.h-closeSize)*.5f,closeSize,closeSize};
 const Rect title{header.x+closeSize+16*u,header.y,header.w-2*closeSize-32*u,header.h};
 const Rect body{frame.x+8*u,header.y+header.h+8*u,frame.w-16*u,frame.h-header.h-32*u};
 const Rect content{body.x+16*u,body.y+16*u,body.w-32*u,body.h-32*u};
 return {{0,0,width,height},frame,header,title,close,body,content,u};
}
void paintPearlProgress(Canvas& canvas,const Domain& domain,const HudDialogLayout& dialog){
 const float u=dialog.unit;const auto area=dialog.content;const float cx=area.x+area.w*.5f;
 const float top=area.y+std::max(16*u,(area.h-400*u)*.5f);
 const auto target=domain.content().pearlSalesTarget;
 const auto progress=domain.adultCoinSales()%target;
 const auto reward=domain.content().pearlSalesReward;
 const Color ink{20,74,84,255},muted{53,102,110,255};
 canvas.icon("hud-icons/pearl-v4.png",{cx-64*u,top,128*u,128*u});
 canvas.text("Earn "+std::to_string(reward)+(reward==1?" pearl":" pearls"),cx,top+132*u,38*u,ink,true,area.w-32*u,true,true);
 canvas.text("Raise and sell "+std::to_string(target)+" adult coin fish",cx,top+194*u,28*u,ink,true,area.w-32*u,true,true);
 const Rect bar{cx-std::min(380*u,area.w*.42f),top+252*u,std::min(760*u,area.w*.84f),48*u};
 canvas.round(bar,{184,211,206,255},24*u,{},0,false,false);
 if(progress)canvas.round({bar.x,bar.y,bar.w*float(progress)/target,bar.h},{66,174,167,255},24*u,{},0,false,false);
 canvas.text(std::to_string(progress)+" / "+std::to_string(target),cx,bar.y+8*u,28*u,ink,true,bar.w,true,true);
 canvas.text("Added automatically. No daily limit.",cx,top+330*u,24*u,muted,true,area.w-32*u,true,true);
 canvas.text("Early sales and pearl fish do not count.",cx,top+374*u,24*u,muted,true,area.w-32*u,true,true);
}
}
