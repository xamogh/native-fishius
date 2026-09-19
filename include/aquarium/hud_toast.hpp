#pragma once
#include "aquarium/hud.hpp"

namespace aq {
struct HudToastLayout {
 Rect frame,close,title,body;
 float scale{},titleSize{},bodySize{};
};
HudToastLayout layoutHudToast(float width,float height,Insets safe,float minimumTouch=44);

// A single replaceable notice. It owns only its own pointer gesture and never
// performs the action that failed, so dismissing it cannot change game state.
class HudToast {
 public:
 void show(std::string title,std::string message);
 void clear();
 bool visible()const{return !message_.empty();}
 const std::string& message()const{return message_;}
 void advance(double seconds,bool reducedMotion=false);
 HudToastLayout animatedLayout(HudToastLayout)const;
 bool event(const SDL_Event&,SDL_FPoint,const HudToastLayout&,float minimumTouch);
 void paint(Canvas&,const HudToastLayout&)const;
 private:
 std::string title_,message_;
 double elapsed_{},remaining_{};
 bool pressed_{},pressedClose_{},dragged_{},reduced_{};
 SDL_FPoint down_{};
};
}
