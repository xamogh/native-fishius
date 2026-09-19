#pragma once
#include "aquarium/hud.hpp"
#include "aquarium/hud_dialog.hpp"
#include "aquarium/hud_rewards.hpp"
#include "aquarium/hud_level_up.hpp"
#include "aquarium/hud_toast.hpp"

namespace aq {
struct DecorPlacement;
// Route one touch through the same controls as a mouse, ignoring SDL's duplicate
// mouse events and additional fingers until the first gesture ends.
struct HudPointer {std::optional<SDL_FingerID> finger;bool touch{};};
bool normalizeHudPointer(HudPointer&,SDL_Event&,float windowWidth,float windowHeight);

struct FishCareLayout {
 HudDialogLayout dialog;Rect status,progress,stages,rewards,breakdown,note,favorite,action;
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
 std::uint64_t selectedDecor()const{return selectedDecor_;}
 bool detailsOpen()const{return details_.open;}
 FishCareLayout detailsLayout()const;
 const std::string& notice()const{return toast_.visible()?toast_.message():notice_;}
 const HudToast& toast()const{return toast_;}
 HudToastLayout toastLayout()const{return toast_.animatedLayout(layoutHudToast(canvas_.width(),canvas_.height(),canvas_.safeInsets(),canvas_.minimumTouchSize()));}
 const HudRewards& rewards()const{return rewards_;}
 HudRewardDisplay rewardDisplay()const{return levelUp_.display(rewards_.display(session_.domain()));}
 HudLevelUp& levelUp(){return levelUp_;}
 const HudLevelUp& levelUp()const{return levelUp_;}
 // Blocked means a menu, another dialog or egg placement owns this event.
 bool event(const SDL_Event&,bool touch,bool blocked=false);
 // Hand a drag of the selected item to the shared placement controls.
 bool beginDecorMove(DecorPlacement&);
 void advance(double seconds,bool rewardsVisible=true);
 void paint();
 void reset();
 private:
 Canvas& canvas_;Session& session_;
 Tool tool_{Tool::Select};FishId selected_{},pressedFish_{};
 std::uint64_t selectedDecor_{},pressedDecor_{};
 DialogState details_{false};
 SDL_FPoint pointer_{},down_{};bool pointerSeen_{},touch_{},downActive_{},dragged_{};
 std::optional<SDL_FPoint> foodPoint_;double foodElapsed_{};
 int pressed_{-1};double tapAge_{10},noticeAge_{};
 std::string notice_;
 std::array<float,6> sellViewport_{};
 HudRewards rewards_;
 HudLevelUp levelUp_;
 HudToast toast_;
 struct Feedback {Event event;double age{};};std::vector<Feedback> feedback_;
 std::optional<Fish> visual(FishId)const;
 FishId hitFish(SDL_FPoint)const;
 const Decoration* visibleDecor(std::uint64_t)const;
 std::uint64_t hitDecor(SDL_FPoint)const;
 FishReward reward(const Fish&)const;
 Rect doneBounds()const;
 bool waterAt(SDL_FPoint)const;
 bool draggingFood()const;void pourFood(double seconds);
 void cancelPress();void closeDetails();void setTool(Tool);
 void arrangeForSale(bool force=false);
 void message(std::string);void favoriteNotice();bool command(const Command&);
 void activate(int);int control(SDL_FPoint)const;
 void paintDetails();
 void paintFavorite(const Fish&);
 void paintStageMeter(const Fish&,SDL_FPoint position,SDL_FPoint fishSize,bool locked);
};
}
