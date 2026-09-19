#pragma once
#include "aquarium/hud.hpp"

namespace aq {
// Committed reward events drive a short visual delay, never an economic delay.
class HudRewards {
 public:
 void collect(const Event&,bool reducedMotion);
 void advance(double seconds,bool reducedMotion);
 void finish(){bursts_.clear();}
 bool active()const{return !bursts_.empty();}
 HudRewardDisplay display(const Domain&)const;
 void paint(Canvas&,const HudLayout&)const;
 private:
 struct Burst {WorldPoint origin;Amount coins{},xp{},pearls{};double age{};bool reduced{},label{};};
 std::vector<Burst> bursts_;
};
HudDialogLayout layoutPearlProgress(float width,float height,Insets safe,float minimumTouch=44);
void paintPearlProgress(Canvas&,const Domain&,const HudDialogLayout&);
}
