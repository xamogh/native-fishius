#include "aquarium/dialog_motion.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
void DialogMotion::advance(bool open,double seconds,bool reducedMotion){
 if(!open){age_=0;visible_=false;reduced_=false;presented_={};return;}
 if(!visible_){age_=0;visible_=true;}
 if(std::isfinite(seconds))age_=std::min(duration,age_+std::max(0.,seconds));
 // Turning motion back on must not replay an already visible dialog.
 reduced_=reducedMotion;
 if(reduced_)age_=duration;
}
DialogPose DialogMotion::pose(Rect frame,Rect safe,bool reducedMotion)const{
 if(reducedMotion||reduced_||age_>=duration)return {};
 const double t=age_;
 // A damped spring: grow, overshoot, then settle with a smaller rebound.
 const double taper=std::clamp((duration-t)/.1,0.,1.);
 const float spring=float(1-.18*std::exp(-8*t)*(std::cos(19*t)+8./19*std::sin(19*t))*taper*taper*(3-2*taper));
 const float scale=std::min(spring,std::max(1.f,std::min(safe.w/frame.w,safe.h/frame.h)));
 const SDL_FPoint center{frame.x+frame.w*.5f,frame.y+frame.h*.5f};
 DialogPose result{scale,{center.x*(1-scale),center.y*(1-scale)}};
 // Keep dialogs near screen edges inside the safe area during the overshoot.
 const auto fit=[](float offset,float start,float size,float low,float span,float s){
  if(size*s>span+.01f)return offset;
  const float minimum=low-start*s,maximum=std::max(minimum,low+span-(start+size)*s);
  return std::clamp(offset,minimum,maximum);
 };
 result.offset.x=fit(result.offset.x,frame.x,frame.w,safe.x,safe.w,scale);
 result.offset.y=fit(result.offset.y,frame.y,frame.h,safe.y,safe.h,scale);
 return result;
}
DialogPaint::DialogPaint(Canvas& canvas,const DialogMotion& motion,Rect frame,bool reducedMotion,bool modal):canvas_(canvas){
 origin_={canvas.origin(),canvas.originY()};
 auto* renderer=canvas.renderer();SDL_GetRenderScale(renderer,&scale_.x,&scale_.y);
 clipped_=SDL_RenderClipEnabled(renderer);SDL_GetRenderClipRect(renderer,&clip_);
 if(modal)canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,150});
 const auto safe=canvas.safeInsets();
 const auto pose=motion.pose(frame,{safe.left,safe.top,canvas.width()-safe.left-safe.right,canvas.height()-safe.top-safe.bottom},reducedMotion);
 motion.presented_=pose;
 canvas.origin((origin_.x+pose.offset.x)/pose.scale,(origin_.y+pose.offset.y)/pose.scale);
 SDL_SetRenderScale(renderer,scale_.x*pose.scale,scale_.y*pose.scale);
 if(clipped_){
  const SDL_Rect clip{int(std::floor(clip_.x/pose.scale)),int(std::floor(clip_.y/pose.scale)),int(std::ceil(clip_.w/pose.scale)),int(std::ceil(clip_.h/pose.scale))};
  SDL_SetRenderClipRect(renderer,&clip);
 }
}
DialogPaint::~DialogPaint(){
 canvas_.origin(origin_.x,origin_.y);
 SDL_SetRenderScale(canvas_.renderer(),scale_.x,scale_.y);
 SDL_SetRenderClipRect(canvas_.renderer(),clipped_?&clip_:nullptr);
}
}
