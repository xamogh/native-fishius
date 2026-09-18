#pragma once
#include "aquarium/hud.hpp"
#include "aquarium/hud_dialog.hpp"
#include "aquarium/hud_rewards.hpp"

namespace aq {
// Route one touch through the same controls as a mouse, ignoring SDL's duplicate
// mouse events and additional fingers until the first gesture ends.
struct HudPointer {std::optional<SDL_FingerID> finger;bool touch{};};
bool normalizeHudPointer(HudPointer&,SDL_Event&,float windowWidth,float windowHeight);

struct FishCareLayout {
 HudDialogLayout dialog;Rect status,progress,stages,rewards,breakdown,note,favorite,primary,secondary;
 std::array<SDL_FPoint,3> tail{};
 bool tailAtTop{};
};
FishCareLayout layoutFishCare(float width,float height,Insets safe,Rect fish,float minimumTouch=44,bool extended=false);

class HudCare {
 public:
 HudCare(Canvas& canvas,Session& session):canvas_(canvas),session_(session){}
 Tool tool()const{return tool_;}
 FishId held()const{return selected_.value?selected_:pressedFish_;}
 FishId selected()const{return selected_;}
 bool detailsOpen()const{return details_.open;}
 FishCareLayout detailsLayout()const;
 const std::string& notice()const{return notice_;}
 const HudRewards& rewards()const{return rewards_;}
 // Blocked means a menu, another dialog or egg placement owns this event.
 bool event(const SDL_Event&,bool touch,bool blocked=false);
 void advance(double seconds,bool rewardsVisible=true);
 void paint();
 void reset();
 private:
 Canvas& canvas_;Session& session_;
 Tool tool_{Tool::Select};FishId selected_{},pressedFish_{};
 DialogState details_{false};
 SDL_FPoint pointer_{},down_{};bool pointerSeen_{},touch_{},downActive_{},dragged_{};
 int pressed_{-1};double tapAge_{10},noticeAge_{};
 std::string notice_;
 HudRewards rewards_;
 struct Feedback {Event event;double age{};};std::vector<Feedback> feedback_;
 std::optional<Fish> visual(FishId)const;
 FishId hitFish(SDL_FPoint)const;
 FishReward reward(const Fish&)const;
 Rect doneBounds()const;
 bool waterAt(SDL_FPoint)const;
 void cancelPress();void closeDetails();void setTool(Tool);
 void message(std::string);bool command(const Command&);
 void activate(int);int control(SDL_FPoint)const;
 void paintDetails();
 void paintStageMeter(const Fish&,SDL_FPoint position,SDL_FPoint fishSize,bool locked);
};
}
