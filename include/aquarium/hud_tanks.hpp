#pragma once
#include "aquarium/hud.hpp"
#include "aquarium/hud_dialog.hpp"

namespace aq {
struct TankOffer {
 const TankEntitlement* next{};
 std::string requirement;
 bool available()const{return next&&requirement.empty();}
};
TankOffer tankOffer(const Domain&,TankId);

// Owned tanks switch directly. Unowned tanks lead to their card in Shop.
struct TankSwitcherState {
 bool open{};
 int pressed{-1};
 SDL_FPoint pressPoint{};
 bool dragged{};
 std::string notice;
 std::optional<TankId> shopTarget;
 DialogMotion motion{};
};
struct TankSwitcherLayout {
 Rect header,toggle;
 std::array<Rect,6> tanks{};
 std::array<TankId,6> ids{};
 int count{};
 int ownedCount{};
 float unit{1};
};
TankSwitcherLayout layoutTankSwitcher(const Domain&,const HudLayout&);
int tankSwitcherControl(const TankSwitcherLayout&,SDL_FPoint);
bool tankSwitcherEvent(Session&,TankSwitcherState&,const HudLayout&,const SDL_Event&,SDL_FPoint);
void paintTankSwitcher(Canvas&,const Domain&,const HudLayout&,const TankSwitcherState&,SDL_FPoint);

struct TankPurchaseState {
 TankEntitlement offer;
 bool upgrade{};
 DialogState dialog{false};
 int pressed{-1};
 SDL_FPoint pressPoint{};
 bool dragged{};
 std::string error;
};
struct TankShopState {
 int pressed{-1};
 SDL_FPoint pressPoint{};
 bool dragged{};
 bool pointerDown{},scrollbarDrag{};
 float scroll{},pressScroll{};
 ScrollMotion motion{};
 TankId noticeTank{};
 std::string notice;
 std::optional<CurrencyShortfall> shortfall;
 std::optional<TankId> focusedTank;
 TankPurchaseState purchase;
};
struct TankShopLayout {
 Rect viewport,scrollTrack,scrollThumb,scrollHitArea;
 std::array<Rect,6> cards{},actions{};
 float unit{1},step{},scroll{},maxScroll{};
 bool portrait{};
};
struct TankPurchaseLayout {
 HudDialogLayout dialog;
 Rect illustration,capacity,message,coins,pearls,choice;
};
TankShopLayout layoutTankShop(const ShopLayout&,float scroll=0);
TankPurchaseLayout layoutTankPurchase(const ShopLayout&);
void focusTankShop(TankShopState&,const ShopLayout&,TankId);
int tankShopControl(const TankShopLayout&,SDL_FPoint);
bool tankShopEvent(Session&,TankShopState&,const ShopLayout&,const SDL_Event&,SDL_FPoint);
void advanceTankShop(TankShopState&,const ShopLayout&,double seconds,bool reducedMotion);
void paintTankShop(Canvas&,const Domain&,const ShopLayout&,const TankShopState&);
void paintTankPurchase(Canvas&,const ShopLayout&,const TankShopState&,bool reducedMotion=false);
}
