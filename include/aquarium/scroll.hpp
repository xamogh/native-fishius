#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

namespace aq {
// Menus use card units, or pixels with unit set to the card stride. Position
// remains the single source of truth for painting, clipping and hit testing.
class ScrollMotion {
 public:
 bool moving()const{return seeking_||velocity_!=0;}
 float destination(float position)const{return seeking_?target_:position;}
 void stop(){velocity_=0;seeking_=dragging_=false;}
 void wheel(float& position,float delta,float limit,bool reduced=false){
  if(!std::isfinite(delta))return;
  if(!seeking_||(target_-position)*delta<0)target_=position;
  target_=std::clamp(target_+delta,0.f,std::max(0.f,limit));
  velocity_=0;dragging_=false;seeking_=target_!=position;
  if(reduced){position=target_;stop();}
 }
 void begin(float position,double time){
  stop();dragging_=true;samplePosition_=position;sampleTime_=lastMove_=time;
 }
 void drag(float& position,float next,float limit,double time,float unit=1){
  if(!dragging_||!std::isfinite(next))return;
  next=std::clamp(next,0.f,std::max(0.f,limit));
  if(next==position)return;
  position=next;lastMove_=time;
  const double dt=time-sampleTime_;
  // Coalesced events can share a timestamp. Keep their distance until the
  // next timed sample instead of inventing an enormous release velocity.
  if(dt<.001)return;
  const float speed=std::clamp(float((position-samplePosition_)/dt),-30*unit,30*unit);
  const float blend=float(-std::expm1(-dt/0.035));
  velocity_+=(speed-velocity_)*blend;
  samplePosition_=position;sampleTime_=time;
 }
 void release(double time,bool reduced=false){
  dragging_=false;
  // Holding a row still before releasing must not start an old fling.
  if(reduced||time-lastMove_>.09)velocity_=0;
 }
 void advance(float& position,double seconds,float limit,bool reduced=false,float unit=1){
  limit=std::max(0.f,limit);position=std::clamp(position,0.f,limit);
  if(dragging_)return;
  if(reduced){if(seeking_)position=std::clamp(target_,0.f,limit);stop();return;}
  if(!std::isfinite(seconds)||seconds<=0)return;
  if(seconds>.25){if(seeking_)position=std::clamp(target_,0.f,limit);stop();return;}
  if(seeking_){
   target_=std::clamp(target_,0.f,limit);
   position+=(target_-position)*float(-std::expm1(-24*seconds));
   if(std::abs(target_-position)<.0005f*unit){position=target_;seeking_=false;}
  }else if(velocity_!=0){
   constexpr float friction=4.5f;
   const float decay=float(std::exp(-friction*seconds));
   position+=velocity_*(1-decay)/friction;velocity_*=decay;
   if(position<=0||position>=limit||std::abs(velocity_)<.015f*unit){position=std::clamp(position,0.f,limit);velocity_=0;}
  }
 }
 private:
 float target_{},velocity_{},samplePosition_{};
 double sampleTime_{},lastMove_{};
 bool seeking_{},dragging_{};
};

inline double scrollEventTime(const SDL_Event& e){return double(e.common.timestamp?e.common.timestamp:SDL_GetTicksNS())/1e9;}
inline float horizontalWheel(const SDL_Event& e){
 // Ignore small cross-axis trackpad noise. SDL wheel values retain fractions.
 const float delta=std::abs(e.wheel.x)>std::abs(e.wheel.y)?e.wheel.x:-e.wheel.y;
 return e.wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-delta:delta;
}
}
