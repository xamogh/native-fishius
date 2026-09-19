#pragma once
#include "aquarium/hud.hpp"

namespace aq {
struct PlacementReceipt {WorldPoint point;Amount cost{},xp{};bool pearl{};double age{};bool decor{};};
struct FishPlacement {
 std::string species,error;GrowthSnapshot offer;WorldPoint point{544,317};
 bool pointerDown{},cancelPressed{};SDL_FPoint down{};
 std::vector<PlacementReceipt> receipts;
 std::optional<CurrencyShortfall> shortfall;
 bool active()const{return !species.empty();}
};
struct FishPlacementLayout {Rect page,water,banner,cancel;float unit;HudLayout hud;};
enum class PlacementEvent {None,Cancelled,Placed,TankFull};
FishPlacementLayout layoutFishPlacement(float width,float height,Insets,float minimumTouch=44);
Result startFishPlacement(const Domain&,FishPlacement&,std::string_view species);
Result confirmFishPlacement(Session&,FishPlacement&,WorldPoint);
void cancelFishPlacement(FishPlacement&);
PlacementEvent fishPlacementEvent(Session&,FishPlacement&,const FishPlacementLayout&,const SDL_Event&,SDL_FPoint);
void advanceFishPlacement(FishPlacement&,double seconds);
void paintPlacementReceipts(Canvas&,const FishPlacement&,float unit);
void advancePlacementReceipts(std::vector<PlacementReceipt>&,double seconds);
void paintPlacementReceipts(Canvas&,std::span<const PlacementReceipt>,float unit);
void paintFishPlacement(Canvas&,const Domain&,const FishPlacement&,const FishPlacementLayout&);
}
