#pragma once
#include "aquarium/view.hpp"
#include <span>

namespace aq {
enum class HudPart {
 Profile,Level,Xp,
 Coins,CoinIcon,CoinAmount,CoinPlus,
 Pearls,PearlIcon,PearlAmount,PearlPlus,
 Tank,TankIcon,TankName,
 Projects,ProjectsIcon,ProjectsLabel,
 Bag,BagIcon,BagLabel,Settings,SettingsIcon,
 Rewards,RewardsIcon,RewardsLabel,
 Food,FoodIcon,FoodLabel,Shop,ShopIcon,ShopLabel,Water,Count
};
struct HudLayout {
 std::array<Rect,static_cast<std::size_t>(HudPart::Count)> boxes{};
 float unit{1},fontSize{24};
 Rect operator[](HudPart part)const{return boxes[static_cast<std::size_t>(part)];}
};

enum class ShopCategory { Fish,Plants,Decorations,Treasure };
struct ShopState { ShopCategory category{ShopCategory::Treasure}; int subtab{}; float scroll{}; };
struct ShopFishOffer {
 std::string id; GrowthSnapshot quote; bool companion{};
 Millis firstGrowthMs()const{return quote.durationMs/10000*quote.stages[1]+quote.durationMs%10000*quote.stages[1]/10000;}
 bool fastGrowing()const{return !companion&&quote.durationMs>0&&firstGrowthMs()<=300000;}
};
struct ShopItem { std::string name,asset,detail,price; bool locked{}; std::optional<ShopFishOffer> fish{}; };
struct ShopLayout {
 Rect page,header,close,title,body,footer;
 HudLayout currencyHud;
 std::array<Rect,4> tabs;std::array<Rect,2> subtabs,wallets;
 std::array<Rect,5> cards;
 float unit;
};
ShopLayout layoutShop(float width,float height,Insets safe,float minimumTouch=44);
std::array<Rect,6> layoutTankCards(const ShopLayout&);
std::vector<ShopItem> shopItems(const Domain&,const ShopState&);
std::optional<std::size_t> shopCardAt(const ShopLayout&,const ShopState&,std::size_t count,SDL_FPoint);
Rect shopCardBounds(const ShopLayout&,const ShopState&,std::size_t index);
Rect shopInfoBounds(Rect card,float unit);
struct HudDialogLayout;
void paintShopFishDetails(Canvas&,const Domain&,const HudDialogLayout&,const Species&);
int shopControl(const ShopLayout&,SDL_FPoint);
void activateShopControl(ShopState&,int);
void scrollShop(ShopState&,float,std::size_t itemCount);
void paintTankMenu(Canvas&,const Domain&,const ShopLayout&,int hover=-1,int pressed=-1);
void paintShop(Canvas&,const Domain&,const ShopLayout&,const ShopState&,int hover=-1,int pressed=-1);

// Clay owns placement. The painter consumes the same bounds that input uses.
HudLayout layoutHud(float width,float height,Insets safe,float minimumTouch);
std::span<const HudPart> hudControls();
std::optional<HudPart> hudHit(const HudLayout&,SDL_FPoint);
void paintHud(Canvas&,const Domain&,const HudLayout&,std::optional<HudPart> hover={},std::optional<HudPart> pressed={});
}
