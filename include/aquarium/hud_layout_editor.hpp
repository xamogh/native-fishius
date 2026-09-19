#pragma once
#include "aquarium/hud_decor_placement.hpp"
#include "aquarium/dialog_motion.hpp"

namespace aq {
struct LayoutEditorLayout {
 Rect page,water,toolbarArea,header,title,exit,exitHit,notice;
 float unit{},dragThreshold{};
};
LayoutEditorLayout layoutEditor(float width,float height,Insets,float minimumTouch);
struct StoredDecorStack {std::string kind;std::uint64_t firstId{};std::size_t count{};};

// Owns editing input so fish and ordinary game tools cannot intercept it.
class HudLayoutEditor {
 public:
 HudLayoutEditor(Canvas& canvas,Session& session):canvas_(canvas),session_(session){}
 bool active()const{return active_;}
 bool inventoryOpen()const{return inventoryOpen_;}
 const ShopState& inventoryState()const{return inventory_;}
 const DecorPlacement& placement()const{return placement_;}
 const Decoration* preview()const{return active_&&placement_.active()?&placement_.preview:nullptr;}
 void open(bool stored=false);void close();
 bool event(const SDL_Event&,bool blocked=false);
 void advance(double seconds);
 void paint();
 LayoutEditorLayout layout()const;
 ShopLayout inventoryLayout()const;
 DecorPlacementLayout placementLayout()const;
 std::vector<StoredDecorStack> storedStacks()const;
 private:
 enum class Control {None,Launch,Bag,Exit,Inventory,Backdrop,Card,Content};
 Canvas& canvas_;Session& session_;
 bool active_{},inventoryOpen_{},inventoryEdit_{},dragged_{};TankId tank_{};
 DialogMotion inventoryMotion_;
 Control pressed_{Control::None};SDL_FPoint down_{};
 std::uint64_t pressedItem_{},cycleItem_{};
 ShopState inventory_{ShopCategory::Plants};int inventoryControl_{-1};float scrollStart_{};
 DecorPlacement placement_;
 std::string error_;
 void clearPress();
 bool inventoryEvent(const SDL_Event&,SDL_FPoint);
 std::vector<ShopItem> inventoryOffers()const;
 Control control(const LayoutEditorLayout&,SDL_FPoint)const;
 std::uint64_t hitDecor(SDL_FPoint,std::uint64_t after=0)const;
 std::uint64_t cardAt(const ShopLayout&,SDL_FPoint)const;
};
}
