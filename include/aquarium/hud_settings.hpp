#pragma once
#include "aquarium/hud_dialog.hpp"

namespace aq {
enum class SettingsPage {Preferences,About};
struct SettingsLayout {
 HudDialogLayout dialog;
 std::array<Rect,4> cards,toggles,toggleHits;
 Rect closeHit,slider,sliderHit,about,aboutHit,back,backHit,copySupport,copySupportHit;
 float unit{},touch{};bool stacked{};
};
SettingsLayout layoutSettings(float width,float height,Insets safe,float minimumTouch=44);

class HudSettings {
 public:
 HudSettings(Canvas& canvas,Session& session):canvas_(canvas),session_(session){}
 void open();void close();bool active()const{return open_;}
 SettingsPage page()const{return page_;}
 SettingsLayout layout()const{return layoutSettings(canvas_.width(),canvas_.height(),canvas_.safeInsets(),canvas_.minimumTouchSize());}
 bool event(const SDL_Event&);void paint();
 void advance(double seconds){motion_.advance(open_,seconds,session_.domain().state().settings.reducedMotion);}
 double volume()const{return dragging_?volumePreview_:session_.domain().state().settings.volume;}
 const std::string& notice()const{return notice_;}
 private:
 Canvas& canvas_;Session& session_;bool open_{},dragging_{};
 DialogMotion motion_;
 SettingsPage page_{SettingsPage::Preferences};int pressed_{-1},focus_{-1};
 SDL_FPoint down_{};double volumePreview_{};std::string notice_;
 int hit(const SettingsLayout&,SDL_FPoint)const;
 void activate(int);void setVolume(float,const SettingsLayout&);bool save(Action,double);void feedback();
};
}
