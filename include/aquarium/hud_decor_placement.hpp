#pragma once
#include "aquarium/hud_placement.hpp"

namespace aq {
enum class DecorPlacementControl {None,Water,Outside,Cancel,Remove,Place};
struct DecorPlacement {
 Decoration preview;
 std::string error;
 std::optional<CurrencyShortfall> shortfall;
 std::vector<PlacementReceipt> receipts;
 DecorPlacementControl pressed{DecorPlacementControl::None};
 SDL_FPoint down{};WorldPoint original{},grabOffset{};bool dragged{},moving{};
 bool active()const{return !preview.kind.empty();}
};
struct DecorPlacementLayout {Rect page,water,cancel,place,notice,cancelHit,placeHit;float unit{},dragThreshold{};Rect item;SceneProjection scene;Rect itemHit;};
Rect decorDragBounds(Rect item,float minimumTouch);
DecorPlacementLayout layoutDecorPlacement(float width,float height,Insets,float minimumTouch=44,Rect item={});
DecorPlacementLayout layoutDecorPlacement(const Canvas&,const Domain&,const DecorPlacement&);
Result startDecorPlacement(const Domain&,DecorPlacement&,std::string_view id);
Result startDecorMove(const Domain&,DecorPlacement&,std::uint64_t id);
// Place one stored copy, then edit that same saved item.
Result startDecorRestore(Session&,DecorPlacement&,std::uint64_t id);
Result saveDecorMove(Session&,DecorPlacement&);
Result removeDecorPlacement(Session&,DecorPlacement&);
Result confirmDecorPlacement(Session&,DecorPlacement&);
void cancelDecorPlacement(DecorPlacement&);
PlacementEvent decorPlacementEvent(Session&,DecorPlacement&,const DecorPlacementLayout&,const SDL_Event&,SDL_FPoint);
void paintDecorPlacement(Canvas&,const Session&,const DecorPlacement&,const DecorPlacementLayout&,bool showNotice=true);
}
