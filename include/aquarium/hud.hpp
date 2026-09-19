#pragma once
#include "aquarium/canvas.hpp"
#include "aquarium/scroll.hpp"
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
 Food,FoodIcon,FoodLabel,Rehome,RehomeIcon,RehomeLabel,Layout,LayoutIcon,LayoutLabel,Shop,ShopIcon,ShopLabel,Water,Count
};
struct HudLayout {
 std::array<Rect,static_cast<std::size_t>(HudPart::Count)> boxes{};
 float unit{1},fontSize{24};
 Rect operator[](HudPart part)const{return boxes[static_cast<std::size_t>(part)];}
};
// Presentation values only. Rewards have already been committed to the save.
struct HudRewardDisplay {
 Amount coins{},pearls{};double xp{};float coinPulse{},pearlPulse{},xpPulse{};bool reducedMotion{};
};

enum class ShopCategory { Fish,Plants,Decorations,Treasure,Environment,Tanks };
struct ShopState { ShopCategory category{ShopCategory::Treasure}; int subtab{}; float scroll{}; ScrollMotion motion{}; };
struct ShopFishOffer {
 std::string id; GrowthSnapshot quote;
 Millis firstGrowthMs()const{return quote.durationMs/10000*quote.stages[1]+quote.durationMs%10000*quote.stages[1]/10000;}
 bool fastGrowing()const{return quote.durationMs>0&&firstGrowthMs()<=300000;}
};
struct ShopItem { std::string name,asset,detail,price; bool locked{}; std::optional<ShopFishOffer> fish{}; std::string environmentId{}; std::string treasureId{}; std::string decorId{}; bool flipped{}; std::size_t quantity{1}; std::string purchaseReward{}; };
struct ShopLayout {
 Rect page,header,close,title,body,footer,scrollTrack,cardViewport;
 HudLayout currencyHud;
 std::array<Rect,6> tabs;std::array<Rect,2> subtabs,wallets;
 std::array<Rect,5> cards;
 float unit,minimumTouch{44};
 Rect dialog,dialogTitle,closeHit;
 int visibleCards{5};bool inventory{};
};
ShopLayout layoutShop(float width,float height,Insets safe,float minimumTouch=44);
ShopLayout layoutInventory(float width,float height,Insets safe,float minimumTouch=44);
std::vector<ShopItem> shopItems(const Domain&,const ShopState&);
std::optional<std::size_t> shopCardAt(const ShopLayout&,const ShopState&,std::size_t count,SDL_FPoint);
Rect shopCardBounds(const ShopLayout&,const ShopState&,std::size_t index);
Rect shopInfoBounds(Rect card,float unit);
struct HudDialogLayout;
void paintShopFishDetails(Canvas&,const Domain&,const HudDialogLayout&,const Species&);
Rect shopSubtabBounds(const ShopLayout&,ShopCategory,int index);
int shopControl(const ShopLayout&,SDL_FPoint,ShopCategory=ShopCategory::Fish);
void activateShopControl(ShopState&,int);
void scrollShop(ShopState&,float,std::size_t itemCount,int visibleCards=0);
float shopScrollLimit(const ShopState&,std::size_t itemCount,int visibleCards=0);
struct TankShopState;
void paintShop(Canvas&,const Domain&,const ShopLayout&,const ShopState&,int hover=-1,int pressed=-1,const TankShopState* tanks=nullptr);
void paintInventory(Canvas&,const Domain&,const ShopLayout&,const ShopState&,std::span<const ShopItem>,int hover=-1,int pressed=-1,bool dimBackdrop=true);

// Clay owns placement. The painter consumes the same bounds that input uses.
HudLayout layoutHud(float width,float height,Insets safe,float minimumTouch);
std::span<const HudPart> hudControls();
std::optional<HudPart> hudHit(const HudLayout&,SDL_FPoint);
void paintHud(Canvas&,const Domain&,const HudLayout&,std::optional<HudPart> hover={},std::optional<HudPart> pressed={},const HudRewardDisplay* rewards=nullptr);
void paintXpBadge(Canvas&,Rect,float unit,float progress,std::string_view label,float textSize);
}
