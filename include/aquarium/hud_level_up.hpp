#pragma once
#include "aquarium/hud.hpp"
#include "aquarium/hud_dialog.hpp"
#include <deque>

namespace aq {
struct LevelUpItem {std::string id,name,asset;};
struct LevelUpNotice {int level{};Money reward{0,0};std::vector<LevelUpItem> items;};
std::vector<LevelUpItem> levelUpItems(const Domain&,int level);
struct LevelUpLayout {
 HudDialogLayout dialog;
 Rect badge,received,rewards,unlocks,continueButton,gallery,grid,divider,progress;
 float unit{},textUnit{},cardWidth{},cardHeight{},gap{};
 bool portrait{},compact{},sideReceipt{};
};
LevelUpLayout layoutLevelUp(float width,float height,Insets safe,float minimumTouch=44);
Rect levelUpCardBounds(const LevelUpLayout&,std::size_t index,float scroll=0);
float levelUpScrollLimit(const LevelUpLayout&,std::size_t count);

// A receipt for committed rewards. Dismissal never changes the wallet or save.
class HudLevelUp {
 public:
 void collect(const Domain&,const Event&);
 void reveal(int displayedLevel);
 void advance(double seconds,bool reducedMotion);
 HudRewardDisplay display(HudRewardDisplay)const;
 bool open()const{return dialog_.open;}
 const LevelUpNotice* current()const{return notices_.empty()?nullptr:&notices_.front();}
 float scroll()const{return scroll_;}
 void paint(Canvas&,const LevelUpLayout&,SDL_FPoint pointer={-1,-1})const;
 // Paint after the modal so the rewards can travel beyond its frame.
 void paintRewards(Canvas&,const LevelUpLayout&,const HudLayout&)const;
 bool event(const LevelUpLayout&,const SDL_Event&,SDL_FPoint);
 private:
 std::deque<LevelUpNotice> notices_;
 DialogState dialog_{false};
 float scroll_{},scrollStart_{};
 float scrollLimit_{},scrollUnit_{1};ScrollMotion scrollMotion_;
 int latestLevel_{1},displayedLevel_{1},pressed_{-1};
 double age_{};bool reducedMotion_{};
 SDL_FPoint down_{};bool dragged_{};
 void cancelPress();void dismiss();
 int control(const LevelUpLayout&,SDL_FPoint)const;
};
}
