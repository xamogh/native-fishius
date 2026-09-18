#pragma once
#include "aquarium/hud.hpp"

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

struct TankShopState {
 int pressed{-1};
 SDL_FPoint pressPoint{};
 bool dragged{};
 bool pointerDown{},scrollbarDrag{};
 float scroll{},pressScroll{};
 TankId noticeTank{};
 std::string notice;
 std::optional<CurrencyShortfall> shortfall;
 std::optional<TankId> focusedTank;
};
struct TankShopLayout {
 Rect viewport,scrollTrack,scrollThumb,scrollHitArea;
 std::array<Rect,6> cards{};
 std::array<Rect,6> coins{},pearls{};
 std::array<Rect,6> coinTargets{},pearlTargets{};
 float unit{1},step{},scroll{},maxScroll{};
 bool portrait{};
};
TankShopLayout layoutTankShop(const ShopLayout&,float scroll=0);
void focusTankShop(TankShopState&,const ShopLayout&,TankId);
int tankShopControl(const TankShopLayout&,SDL_FPoint);
bool tankShopEvent(Session&,TankShopState&,const ShopLayout&,const SDL_Event&,SDL_FPoint);
void paintTankShop(Canvas&,const Domain&,const ShopLayout&,const TankShopState&);
}
