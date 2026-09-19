#pragma once
#include "aquarium/canvas.hpp"

namespace aq {
struct DialogPose {
 float scale{1};SDL_FPoint offset{};
 SDL_FPoint apply(SDL_FPoint p)const{return {p.x*scale+offset.x,p.y*scale+offset.y};}
 SDL_FPoint inverse(SDL_FPoint p)const{return {(p.x-offset.x)/scale,(p.y-offset.y)/scale};}
};

// One entrance per opening. Input uses the pose last shown on screen.
class DialogMotion {
 public:
 static constexpr double duration=.6;
 void advance(bool open,double seconds,bool reducedMotion);
 DialogPose pose(Rect frame,Rect safeArea,bool reducedMotion=false)const;
 SDL_FPoint inputPoint(SDL_FPoint point)const{return presented_.inverse(point);}
 private:
 double age_{};bool visible_{},reduced_{};
 mutable DialogPose presented_{};
 friend class DialogPaint;
};

// Keep this scope alive while painting the frame and all of its contents.
// The backdrop stays still; the dialog's text, artwork and clips move together.
class DialogPaint {
 public:
 DialogPaint(Canvas&,const DialogMotion&,Rect frame,bool reducedMotion=false,bool modal=true);
 ~DialogPaint();
 DialogPaint(const DialogPaint&)=delete;DialogPaint& operator=(const DialogPaint&)=delete;
 private:
 Canvas& canvas_;SDL_FPoint origin_,scale_;SDL_Rect clip_{};bool clipped_{};
};
}
