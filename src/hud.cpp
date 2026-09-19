#include "aquarium/hud.hpp"
#include "aquarium/hud_dialog.hpp"
#include "aquarium/hud_tokens.hpp"
#include "aquarium/hud_tanks.hpp"
#include "aquarium/shop_theme.hpp"
#include "clay.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

namespace aq {
namespace {
std::string shopDuration(Millis ms){
 const auto seconds=(std::max<Millis>(0,ms)+999)/1000;
 const auto hours=seconds/3600,minutes=seconds/60%60;
 if(hours)return std::to_string(hours)+"h"+(minutes?" "+std::to_string(minutes)+"m":"");
 if(minutes)return std::to_string(minutes)+"m"+(seconds%60?" "+std::to_string(seconds%60)+"s":"");
 return std::to_string(seconds)+"s";
}
// Fit generated currency silhouettes without their transparent padding.
void paintCurrencyIcon(Canvas& canvas,Rect box,bool pearl){
 if(!pearl){
  const float sx=box.w/1081.f,sy=box.h/1076.f;
  canvas.image("hud-icons/coin-v4.png",{box.x-87.f*sx,box.y-91.f*sy,1254.f*sx,1254.f*sy});
 }
 if(pearl){
  const float sx=box.w/1028.f,sy=box.h/1012.f;
  canvas.image("hud-icons/pearl-v4.png",{box.x-113.f*sx,box.y-128.f*sy,1254.f*sx,1254.f*sy});
 }
}
constexpr std::array controls{
 HudPart::CoinPlus,HudPart::PearlPlus,HudPart::Tank,HudPart::Projects,HudPart::Bag,HudPart::Settings,HudPart::Food,HudPart::Rehome,HudPart::Layout,HudPart::Rewards,HudPart::Shop};
struct ContextGuard {
 Clay_Context* previous{Clay_GetCurrentContext()};
 ~ContextGuard(){Clay_SetCurrentContext(previous);}
};
struct HudContext {
 std::vector<std::byte> memory;Clay_Context* context{};std::string error;
 HudContext(){
  ContextGuard guard;Clay_SetCurrentContext(nullptr);
  Clay_SetMaxElementCount(256);Clay_SetMaxMeasureTextCacheWordCount(256);
  memory.resize(Clay_MinMemorySize());
  context=Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(memory.size(),memory.data()),{hudTokens::canvasWidth,hudTokens::canvasHeight},
   {[](Clay_ErrorData data){static_cast<HudContext*>(data.userData)->error.assign(data.errorText.chars,data.errorText.length);},this});
 }
};
Clay_ElementId id(HudPart part){return CLAY_IDI("fishius-hud",static_cast<std::uint32_t>(part));}
std::uint16_t pixels(float value){return static_cast<std::uint16_t>(std::clamp(std::round(value),0.f,65535.f));}
Clay_ElementDeclaration flow(Clay_SizingAxis width,Clay_SizingAxis height,Clay_LayoutDirection direction,float gap=0,float padding=0){
 Clay_ElementDeclaration element{};
 element.layout.sizing={width,height};element.layout.layoutDirection=direction;
 element.layout.childGap=pixels(gap);element.layout.padding=CLAY_PADDING_ALL(pixels(padding));
 return element;
}
Clay_ElementDeclaration named(HudPart part,Clay_ElementDeclaration element){element.id=id(part);return element;}
void leaf(HudPart part,Clay_SizingAxis width,Clay_SizingAxis height){CLAY(named(part,flow(width,height,CLAY_LEFT_TO_RIGHT))) {}}
void spacer(Clay_LayoutDirection direction){CLAY(flow(CLAY_SIZING_GROW(),direction==CLAY_TOP_TO_BOTTOM?CLAY_SIZING_GROW():CLAY_SIZING_FIXED(0),direction)) {}}
}

HudLayout layoutHud(float width,float height,Insets safe,float minimumTouch){
 if(!std::isfinite(width)||!std::isfinite(height)||!std::isfinite(minimumTouch)||width<=0||height<=0||minimumTouch<=0)throw std::invalid_argument("HUD dimensions must be positive and finite");
 static thread_local HudContext state;ContextGuard guard;
 Clay_SetCurrentContext(state.context);Clay_SetLayoutDimensions({width,height});state.error.clear();
 HudLayout result;
 const float density=minimumTouch/44.f;
 const float u=std::min(width/hudTokens::canvasWidth,height/hudTokens::canvasHeight);result.unit=u;
 const float margin=hudTokens::margin*u,gap=hudTokens::gap*u;
 // Preserve the same composition at every size. Controls scale with the HUD.
 result.fontSize=hudTokens::textBody*u;
 const float line=hudTokens::lineHeight*u;
 const float badge=hudTokens::icon*u,barHeight=hudTokens::barHeight*u;
 const float imageAdvance=badge-hudTokens::overlap*density;
 const float primary=hudTokens::shop*u,bagWidth=hudTokens::bagWidth*u;
 auto tile=[&](HudPart part,HudPart icon,HudPart label,float w,float h){
  const bool small=part==HudPart::Food||part==HudPart::Rewards||part==HudPart::Rehome||part==HudPart::Layout;
  CLAY(named(part,flow(CLAY_SIZING_FIXED(w),CLAY_SIZING_FIXED(h),CLAY_TOP_TO_BOTTOM,(small?hudTokens::smallGap:hudTokens::overlap)*u,(small?hudTokens::smallGap:hudTokens::padding)*u))){
   leaf(icon,CLAY_SIZING_GROW(),CLAY_SIZING_GROW());
   leaf(label,CLAY_SIZING_GROW(),CLAY_SIZING_FIXED(line));
  }
 };
 auto wallet=[&](bool pearl){
  auto row=flow(CLAY_SIZING_FIT(),CLAY_SIZING_FIXED(badge),CLAY_LEFT_TO_RIGHT);
  row.layout.childAlignment.y=CLAY_ALIGN_Y_CENTER;
  CLAY(row){
   auto image=named(pearl?HudPart::PearlIcon:HudPart::CoinIcon,flow(CLAY_SIZING_FIXED(badge),CLAY_SIZING_FIXED(badge),CLAY_LEFT_TO_RIGHT));
   image.floating.attachTo=CLAY_ATTACH_TO_PARENT;
   CLAY(image) {}
   CLAY(flow(CLAY_SIZING_FIXED(imageAdvance),CLAY_SIZING_FIXED(0),CLAY_LEFT_TO_RIGHT)) {}
   CLAY(named(pearl?HudPart::Pearls:HudPart::Coins,flow(CLAY_SIZING_FIXED((pearl?hudTokens::pearlWidth:hudTokens::coinWidth)*u),CLAY_SIZING_FIXED(barHeight),CLAY_LEFT_TO_RIGHT))){
    leaf(pearl?HudPart::PearlAmount:HudPart::CoinAmount,CLAY_SIZING_GROW(),CLAY_SIZING_GROW());
   }
   auto plus=named(pearl?HudPart::PearlPlus:HudPart::CoinPlus,flow(CLAY_SIZING_FIXED(44*u),CLAY_SIZING_FIXED(44*u),CLAY_LEFT_TO_RIGHT));
   plus.floating.attachTo=CLAY_ATTACH_TO_PARENT;plus.floating.attachPoints.parent=CLAY_ATTACH_POINT_RIGHT_CENTER;plus.floating.attachPoints.element=CLAY_ATTACH_POINT_RIGHT_CENTER;plus.floating.offset.x=-hudTokens::overlap*density;
   CLAY(plus){}
   CLAY(flow(CLAY_SIZING_FIXED(44*u),CLAY_SIZING_FIXED(0),CLAY_LEFT_TO_RIGHT)){}

  }
 };
 auto primaryActions=[&]{
  auto column=flow(CLAY_SIZING_FIT(),CLAY_SIZING_FIT(),CLAY_TOP_TO_BOTTOM,gap);
  column.layout.childAlignment.x=CLAY_ALIGN_X_RIGHT;
  CLAY(column){
   tile(HudPart::Layout,HudPart::LayoutIcon,HudPart::LayoutLabel,hudTokens::food*u,hudTokens::food*u);
   tile(HudPart::Rehome,HudPart::RehomeIcon,HudPart::RehomeLabel,hudTokens::food*u,hudTokens::food*u);
   tile(HudPart::Food,HudPart::FoodIcon,HudPart::FoodLabel,hudTokens::food*u,hudTokens::food*u);
   tile(HudPart::Projects,HudPart::ProjectsIcon,HudPart::ProjectsLabel,bagWidth,hudTokens::bagHeight*u);
   tile(HudPart::Bag,HudPart::BagIcon,HudPart::BagLabel,bagWidth,hudTokens::bagHeight*u);
   auto shopRow=flow(CLAY_SIZING_FIT(),CLAY_SIZING_FIT(),CLAY_LEFT_TO_RIGHT,gap);shopRow.layout.childAlignment.y=CLAY_ALIGN_Y_BOTTOM;
   CLAY(shopRow){
    tile(HudPart::Rewards,HudPart::RewardsIcon,HudPart::RewardsLabel,hudTokens::food*u,hudTokens::food*u);
    tile(HudPart::Shop,HudPart::ShopIcon,HudPart::ShopLabel,primary,primary);
   }
  }
 };
 Clay_BeginLayout();
 auto root=flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_TOP_TO_BOTTOM,gap);
 root.id=CLAY_ID("fishius-hud-root");
 root.layout.padding={pixels(safe.left+margin),pixels(safe.right+margin),pixels(safe.top+margin),pixels(safe.bottom+margin)};
 CLAY(root){
  CLAY(flow(CLAY_SIZING_GROW(),CLAY_SIZING_FIT(),CLAY_LEFT_TO_RIGHT,gap)){
   auto profile=flow(CLAY_SIZING_FIT(),CLAY_SIZING_FIT(),CLAY_TOP_TO_BOTTOM);
   CLAY(named(HudPart::Profile,profile)){
    auto xpRow=flow(CLAY_SIZING_FIT(),CLAY_SIZING_FIXED(badge),CLAY_LEFT_TO_RIGHT);
    xpRow.layout.childAlignment.y=CLAY_ALIGN_Y_CENTER;
    CLAY(xpRow){
     // Clay gaps are unsigned. Reserve less width than the floating image
     // to produce an eight-point overlap while the bar still grows normally.
     auto levelImage=named(HudPart::Level,flow(CLAY_SIZING_FIXED(badge),CLAY_SIZING_FIXED(badge),CLAY_LEFT_TO_RIGHT));
     levelImage.floating.attachTo=CLAY_ATTACH_TO_PARENT;
     CLAY(levelImage) {}
     CLAY(flow(CLAY_SIZING_FIXED(imageAdvance),CLAY_SIZING_FIXED(0),CLAY_LEFT_TO_RIGHT)) {}
     leaf(HudPart::Xp,CLAY_SIZING_FIXED(hudTokens::xpWidth*u),CLAY_SIZING_FIXED(barHeight));
    }
   }
   spacer(CLAY_LEFT_TO_RIGHT);
   CLAY(flow(CLAY_SIZING_FIT(),CLAY_SIZING_FIT(),CLAY_LEFT_TO_RIGHT,hudTokens::overlap*u)){
    wallet(false);wallet(true);
    CLAY(named(HudPart::Settings,flow(CLAY_SIZING_FIXED(badge),CLAY_SIZING_FIXED(badge),CLAY_LEFT_TO_RIGHT,0,hudTokens::overlap*u))){
     leaf(HudPart::SettingsIcon,CLAY_SIZING_GROW(),CLAY_SIZING_GROW());
    }
   }
  }
  CLAY(flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_LEFT_TO_RIGHT,gap)){
   leaf(HudPart::Water,CLAY_SIZING_GROW(),CLAY_SIZING_GROW());
  }
  auto footer=flow(CLAY_SIZING_GROW(),CLAY_SIZING_FIT(),CLAY_LEFT_TO_RIGHT,gap);
  footer.layout.childAlignment.y=CLAY_ALIGN_Y_BOTTOM;
  CLAY(footer){tile(HudPart::Tank,HudPart::TankIcon,HudPart::TankName,primary,primary);spacer(CLAY_LEFT_TO_RIGHT);primaryActions();}
 }
 Clay_EndLayout();
 if(!state.error.empty())throw std::runtime_error("HUD layout: "+state.error);
 for(std::size_t index=0;index<result.boxes.size();++index){
  const auto data=Clay_GetElementData(id(static_cast<HudPart>(index)));
  if(!data.found)throw std::runtime_error("HUD element is missing from the layout");
  const auto box=data.boundingBox;result.boxes[index]={box.x,box.y,box.width,box.height};
 }
 return result;
}

namespace {
ShopLayout layoutCatalog(float width,float height,Insets safe,float minimumTouch,bool inventory=false,float inventoryUnit=1,int columns=5){
 if(width<=0||height<=0)throw std::invalid_argument("Shop dimensions must be positive");
 static thread_local HudContext state;ContextGuard guard;
 Clay_SetCurrentContext(state.context);Clay_SetLayoutDimensions({width,height});state.error.clear();
 const float u=inventory?inventoryUnit:std::min(width/hudTokens::canvasWidth,height/hudTokens::canvasHeight);
 auto element=[&](int idValue,Clay_ElementDeclaration d){d.id=CLAY_IDI("shop",idValue);return d;};
 auto box=[&](int idValue,Clay_SizingAxis w,Clay_SizingAxis h){CLAY(element(idValue,flow(w,h,CLAY_LEFT_TO_RIGHT))) {}};
 Clay_BeginLayout();
 auto root=flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_TOP_TO_BOTTOM);
 root.layout.padding={pixels(safe.left+16*u),pixels(safe.right+16*u),pixels(safe.top+24*u),pixels(safe.bottom+16*u)};
 CLAY(root){
  auto header=flow(CLAY_SIZING_GROW(),CLAY_SIZING_FIXED((inventory?216:152)*u),CLAY_LEFT_TO_RIGHT,8*u);header.layout.childAlignment.y=CLAY_ALIGN_Y_BOTTOM;
  CLAY(element(0,header)){
   box(30,CLAY_SIZING_FIXED(64*u),CLAY_SIZING_FIXED(0));spacer(CLAY_LEFT_TO_RIGHT);
   for(const auto category:{ShopCategory::Fish,ShopCategory::Plants,ShopCategory::Decorations,
                            ShopCategory::Tanks,ShopCategory::Treasure,ShopCategory::Environment})
    if(!inventory||category==ShopCategory::Plants||category==ShopCategory::Decorations)
     box(10+static_cast<int>(category),CLAY_SIZING_FIXED(176*u),CLAY_SIZING_FIXED(128*u));
   spacer(CLAY_LEFT_TO_RIGHT);
   auto close=flow(CLAY_SIZING_FIXED(64*u),CLAY_SIZING_FIXED(64*u),CLAY_LEFT_TO_RIGHT);close.floating.attachTo=CLAY_ATTACH_TO_PARENT;close.floating.attachPoints.parent=CLAY_ATTACH_POINT_RIGHT_TOP;close.floating.attachPoints.element=CLAY_ATTACH_POINT_RIGHT_TOP;
   CLAY(element(1,close)){}
   box(31,CLAY_SIZING_FIXED(64*u),CLAY_SIZING_FIXED(0));
  }
  box(2,CLAY_SIZING_GROW(),CLAY_SIZING_FIXED(96*u));
  CLAY(flow(CLAY_SIZING_GROW(),CLAY_SIZING_FIXED(64*u),CLAY_LEFT_TO_RIGHT,12*u)){
   spacer(CLAY_LEFT_TO_RIGHT);for(int i=0;i<(inventory?1:2);++i)box(40+i,CLAY_SIZING_FIXED(240*u),CLAY_SIZING_FIXED(56*u));spacer(CLAY_LEFT_TO_RIGHT);
  }
  CLAY(element(3,flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_LEFT_TO_RIGHT,20*u,12*u))){
   for(int i=0;i<columns;++i)box(20+i,CLAY_SIZING_GROW(),CLAY_SIZING_GROW());
  }
  if(!inventory){
   auto footer=flow(CLAY_SIZING_GROW(),CLAY_SIZING_FIXED(96*u),CLAY_LEFT_TO_RIGHT,32*u,16*u);footer.layout.childAlignment.y=CLAY_ALIGN_Y_CENTER;
   CLAY(element(4,footer)){
    spacer(CLAY_LEFT_TO_RIGHT);for(int i=0;i<2;++i)box(16+i,CLAY_SIZING_FIXED(280*u),CLAY_SIZING_FIXED(48*u));spacer(CLAY_LEFT_TO_RIGHT);
   }
  }
 }
 Clay_EndLayout();if(!state.error.empty())throw std::runtime_error(state.error);
 auto bounds=[&](int i){auto b=Clay_GetElementData(CLAY_IDI("shop",i)).boundingBox;return Rect{b.x,b.y,b.width,b.height};};
 ShopLayout result{};result.page={0,0,width,height};result.unit=u;result.minimumTouch=minimumTouch;result.inventory=inventory;result.visibleCards=columns;
 result.header=bounds(0);result.close=bounds(1);result.title=bounds(2);result.body=bounds(3);if(!inventory)result.footer=bounds(4);
 result.scrollTrack={result.body.x+12*u,result.body.y+result.body.h-12*u,result.body.w-24*u,8*u};
 for(int i=0;i<6;++i)if(!inventory||i==int(ShopCategory::Plants)||i==int(ShopCategory::Decorations))result.tabs[i]=bounds(10+i);
 for(int i=0;i<(inventory?1:2);++i)result.subtabs[i]=bounds(40+i);
 if(!inventory)for(int i=0;i<2;++i)result.wallets[i]=bounds(16+i);
 for(int i=0;i<columns;++i)result.cards[i]=bounds(20+i);
 const float stride=columns>1?result.cards[1].x-result.cards[0].x:result.cards[0].w+20*u;
 for(int i=columns;i<5;++i){result.cards[i]=result.cards[0];result.cards[i].x+=float(i)*stride;}
 result.closeHit=result.close;
 if(inventory){
  const float size=std::max(result.close.w,minimumTouch),side=size+8*u;
  result.closeHit={result.close.x+result.close.w-size,result.close.y,size,size};
  result.dialogTitle={result.header.x+side,result.header.y,result.header.w-2*side,64*u};
 }
 const float inset=result.cards[0].x-result.body.x;
 result.cardViewport={result.cards[0].x,result.cards[0].y,result.body.w-2*inset,result.cards[0].h+4*u};
 if(!inventory){
  // Reuse the exact main HUD layout, translating the currency group only.
  result.currencyHud=layoutHud(width,height,safe,minimumTouch);
  const auto left=result.currencyHud[HudPart::CoinIcon],right=result.currencyHud[HudPart::Pearls];
  const float dx=result.footer.x+(result.footer.w-(right.x+right.w-left.x))*.5f-left.x;
  const float dy=result.footer.y+(result.footer.h-left.h)*.5f-left.y;
  for(auto& item:result.currencyHud.boxes){item.x+=dx;item.y+=dy;}
 }

 return result;
}
}
ShopLayout layoutShop(float width,float height,Insets safe,float minimumTouch){return layoutCatalog(width,height,safe,minimumTouch);}
ShopLayout layoutInventory(float width,float height,Insets safe,float minimumTouch){
 const float base=std::min(width/hudTokens::canvasWidth,height/hudTokens::canvasHeight),density=minimumTouch/44.f;
 const float scale=std::max(base,minimumTouch/64.f),margin=std::max(24*base,12*density);
 const float w=std::min(1280*scale,width-safe.left-safe.right-2*margin),h=std::min(800*scale,height-safe.top-safe.bottom-2*margin);
 const Rect frame{safe.left+(width-safe.left-safe.right-w)*.5f,safe.top+(height-safe.top-safe.bottom-h)*.5f,w,h};
 const float u=std::min({scale,h/776.f,w/584.f}),inset=8*u;
 const Rect page{frame.x+inset,frame.y+inset,w-2*inset,h-2*inset};
 const int columns=std::clamp(int((page.w-32*u)/(252*u)),1,5);
 auto l=layoutCatalog(page.w,page.h,{},minimumTouch,true,u,columns);
 auto move=[&](Rect& r){if(r.w>0&&r.h>0){r.x+=page.x;r.y+=page.y;}};
 for(auto* r:{&l.page,&l.header,&l.close,&l.closeHit,&l.dialogTitle,&l.title,&l.body,&l.footer,&l.scrollTrack,&l.cardViewport})move(*r);
 for(auto& r:l.tabs)move(r);for(auto& r:l.subtabs)move(r);for(auto& r:l.cards)move(r);for(auto& r:l.currencyHud.boxes)move(r);
 l.dialog=frame;return l;
}
std::vector<ShopItem> shopItems(const Domain& domain,const ShopState& state){
 std::vector<ShopItem> result;
 if(state.category==ShopCategory::Tanks)return result;
 if(state.category==ShopCategory::Treasure){
  std::vector<const TreasureOffer*> offers;
  for(const auto& offer:domain.content().treasureOffers){
   if(offer.kind!=TreasureKind::Coins&&offer.kind!=TreasureKind::Pearls)continue;
   if(state.subtab==1&&offer.kind!=TreasureKind::Coins)continue;
   if(state.subtab==2&&offer.kind!=TreasureKind::Pearls)continue;
   offers.push_back(&offer);
  }
  std::stable_sort(offers.begin(),offers.end(),[](const auto* a,const auto* b){return a->priceUsdCents<b->priceUsdCents;});
  for(const auto* entry:offers){
   const auto& offer=*entry;const bool coins=offer.kind==TreasureKind::Coins;
   const auto amount=treasureContents(domain.content(),offer,domain.level());
   result.push_back({offer.name,offer.asset,compact(coins?amount.coins:amount.pearls)+(coins?" Coins":" Pearls"),usdPrice(offer.priceUsdCents),domain.level()<offer.level,{},{},offer.id});
  }
 }else if(state.category==ShopCategory::Environment){
  const auto* tank=domain.tank(domain.state().activeTank);
  for(const auto& item:environmentCatalog()){
   const bool owned=domain.ownsEnvironment(item.id);if(state.subtab==1&&!owned)continue;
   const bool selected=tank&&tank->backgroundId==item.id;
   result.push_back({item.name,item.asset,"",selected?"Selected":owned?"Use background":compact(item.price)+" Coins",false,{},item.id});
  }
 }else if(state.category==ShopCategory::Fish){
  std::vector<const Species*> offers;
  for(const auto& item:domain.content().species){
   if(!item.artReady||item.releaseGate!="Launch")continue;
   if(state.subtab==1&&item.level>domain.level())continue;
   offers.push_back(&item);
  }
  std::stable_sort(offers.begin(),offers.end(),[](const auto* a,const auto* b){return a->level<b->level;});
  for(const auto* entry:offers){
   const auto& item=*entry;const bool locked=item.level>domain.level();
   const auto quote=domain.quote(item);
   const auto price=item.currency==Currency::Coins?quote.principal:item.price;
   result.push_back({item.name,item.asset,locked?"Level "+std::to_string(item.level):"",compact(price)+(item.currency==Currency::Pearls?(price==1?" Pearl":" Pearls"):" Coins"),locked,ShopFishOffer{item.id,quote}});
  }
 }else{
  std::vector<const DecorDef*> offers;
  for(const auto& item:domain.content().decorations){
   if(!item.artReady||item.releaseGate!="Launch"||(item.category=="Plant")!=(state.category==ShopCategory::Plants))continue;
   if(state.subtab==1&&item.level>domain.level())continue;
   offers.push_back(&item);
  }
  std::stable_sort(offers.begin(),offers.end(),[](const auto* a,const auto* b){return a->level<b->level;});
  for(const auto* entry:offers){
   const auto& item=*entry;const bool locked=item.level>domain.level();
   result.push_back({item.name,item.asset,"Level "+std::to_string(item.level),compact(item.price)+(item.currency==Currency::Pearls?" Pearls":" Coins"),locked,{},{},{},item.id});
   const auto xp=domain.decorPurchaseXp(item);
   const auto remaining=domain.content().levels.back()-domain.state().xp;
   result.back().purchaseReward=xp==0?"Owned: no bonus XP":remaining==0?"Max level":
    "First buy: +"+std::to_string(std::min(xp,remaining))+" XP";
  }
 }
 return result;
}
namespace {
void paintWallet(Canvas& canvas,const Domain& domain,const HudLayout& layout,std::optional<HudPart> hover={},std::optional<HudPart> pressed={},bool showPlus=true,const HudRewardDisplay* rewards=nullptr){
 const float u=layout.unit;
 for(bool pearl:{false,true}){
  const auto bar=layout[pearl?HudPart::Pearls:HudPart::Coins];
  const Color counterEdge=pearl?Color{64,48,101,255}:Color{91,57,22,255};
  const Color counterBase=pearl?Color{144,118,196,255}:Color{202,137,38,255};
  const Color counterFace=pearl?Color{218,209,248,255}:Color{255,225,140,255};
  canvas.round({bar.x,bar.y+4*u,bar.w,bar.h},{9,37,47,100},12*u,{},0,false,false);
  canvas.round(bar,counterBase,12*u,counterEdge,4*u,false,false);
  canvas.round({bar.x+4*u,bar.y+4*u,bar.w-8*u,bar.h-12*u},counterFace,8*u,{},0,false,false);
  canvas.round({bar.x+12*u,bar.y+4*u,std::min(40*u,bar.w*.25f),4*u},{255,255,246,190},4*u,{},0,false,false);
  const float pulse=rewards?(pearl?rewards->pearlPulse:rewards->coinPulse):0;
  const float bounce=rewards&&!rewards->reducedMotion?pulse:0;
  if(pulse>0)canvas.outline({bar.x-2*u,bar.y-2*u,bar.w+4*u,bar.h+4*u},{255,245,182,Uint8(220*pulse)},14*u,3*u);
  const auto amount=layout[pearl?HudPart::PearlAmount:HudPart::CoinAmount];
  const float font=hudTokens::textBody*u*(1+.08f*bounce);
  const Amount value=pearl?(rewards?rewards->pearls:domain.state().wallet.pearls):(rewards?rewards->coins:domain.state().wallet.coins);
  canvas.text(compact(value),amount.x+amount.w*.5f,amount.y+(amount.h-font)*.5f,font,counterEdge,true,amount.w,true,true);
  auto icon=layout[pearl?HudPart::PearlIcon:HudPart::CoinIcon];
  const float grow=icon.w*.08f*bounce;icon={icon.x-grow,icon.y-grow,icon.w+2*grow,icon.h+2*grow};
  paintCurrencyIcon(canvas,icon,pearl);
  if(!showPlus)continue;
  const auto plus=pearl?HudPart::PearlPlus:HudPart::CoinPlus;
  const auto button=layout[plus];const bool down=pressed==plus,over=hover==plus;
  // A dark lower rim and inset face give the small button depth without artwork.
  canvas.round(button,{43,82,22,255},12*u,over?Color{239,255,185,255}:Color{24,48,19,255},4*u,false,false);
  const float shift=down?4*u:0;
  const Rect face{button.x+4*u,button.y+4*u+shift,button.w-8*u,button.h-12*u-shift};
  canvas.round(face,down?Color{105,167,35,255}:over?Color{178,233,75,255}:Color{151,212,48,255},8*u,{},0,false,false);
  if(!down)canvas.round({face.x+4*u,face.y,face.w-8*u,8*u},{224,255,149,190},4*u,{},0,false,false);
  const float cx=button.x+button.w*.5f,cy=button.y+button.h*.5f+(down?4*u:0);
  const auto cross=[&](float arm,float thickness,Color color){
   canvas.round({cx-thickness*.5f,cy-arm*.5f,thickness,arm},color,2*u,{},0,false,false);
   canvas.round({cx-arm*.5f,cy-thickness*.5f,arm,thickness},color,2*u,{},0,false,false);
  };
  cross(24*u,12*u,{49,83,25,255});
  cross(20*u,8*u,{255,255,235,255});

 }
}
}
namespace {
int shopSubtabCount(ShopCategory category,bool inventory){return (category==ShopCategory::Tanks||inventory)?1:category==ShopCategory::Treasure?3:2;}
int shopSubtabControl(int index){return index==2?9:4+index;}
}
Rect shopSubtabBounds(const ShopLayout& layout,ShopCategory category,int index){
 if(index<0||index>=shopSubtabCount(category,layout.inventory))return {};
 if(category==ShopCategory::Treasure&&!layout.inventory){
  auto bounds=layout.subtabs[0];const float stride=layout.subtabs[1].x-bounds.x;
  bounds.x=layout.title.x+(layout.title.w-bounds.w)*.5f+(index-1)*stride;
  return bounds;
 }
 auto bounds=layout.subtabs[index];
 if(category==ShopCategory::Tanks)bounds.x=layout.title.x+(layout.title.w-bounds.w)*.5f;
 return bounds;
}
int shopControl(const ShopLayout& layout,SDL_FPoint point,ShopCategory category){
 if(layout.closeHit.has(point.x,point.y))return 6;
 for(int i=0;i<6;++i)if(layout.tabs[i].w>0&&layout.tabs[i].has(point.x,point.y))return i>=4?i+3:i;
 for(int i=0;i<shopSubtabCount(category,layout.inventory);++i)if(shopSubtabBounds(layout,category,i).has(point.x,point.y))return shopSubtabControl(i);
 return -1;
}
void activateShopControl(ShopState& state,int control){
 state.motion.stop();
 if(control==8){state.category=ShopCategory::Tanks;state.subtab=0;state.scroll=0;}
 else if(control==7){state.category=ShopCategory::Environment;state.subtab=0;state.scroll=0;}
 else if(control>=0&&control<4){state.category=static_cast<ShopCategory>(control);state.subtab=0;state.scroll=0;}
 else if((control==4||control==5)&&!(state.category==ShopCategory::Tanks&&control==5)){state.subtab=control-4;state.scroll=0;}
 else if(control==9&&state.category==ShopCategory::Treasure){state.subtab=2;state.scroll=0;}
}
float shopScrollLimit(const ShopState& state,std::size_t count,int visibleCards){return std::max(0.f,float(count)-(visibleCards>0?visibleCards:state.category==ShopCategory::Environment?2:5));}
void scrollShop(ShopState& state,float delta,std::size_t count,int visibleCards){state.scroll=std::clamp(state.scroll+delta,0.f,shopScrollLimit(state,count,visibleCards));}
std::optional<std::size_t> shopCardAt(const ShopLayout& layout,const ShopState& state,std::size_t count,SDL_FPoint point){
 if(!layout.cardViewport.has(point.x,point.y))return {};
 for(std::size_t i=0;i<count;++i)if(shopCardBounds(layout,state,i).has(point.x,point.y))return i;
 return {};
}
Rect shopCardBounds(const ShopLayout& layout,const ShopState& state,std::size_t index){if(state.category==ShopCategory::Environment){
 const float u=layout.unit,gap=28*u,height=layout.body.h-48*u;
 const float width=std::min((layout.body.w-76*u)*.5f,(height-104*u)*1672.f/941.f+24*u);
 return {layout.body.x+(layout.body.w-2*width-gap)*.5f+(float(index)-state.scroll)*(width+gap),layout.body.y+20*u,width,height};
 }auto card=layout.cards[0];card.x+=(float(index)-state.scroll)*(layout.cards[1].x-layout.cards[0].x);return card;}
Rect shopInfoBounds(Rect card,float u){return {card.x+8*u,card.y+8*u,40*u,40*u};}
namespace {
void paintMenuChrome(Canvas& canvas,const Domain& domain,const ShopLayout& l,int hover,int pressed){
 const float u=l.unit;const auto white=shopTheme::white;
 auto label=[&](std::string_view text,Rect r,float size){canvas.text(text,r.x+r.w*.5f,r.y+(r.h-size*u)*.5f,size*u,white,true,r.w-12*u,true,true);};
 if(!l.inventory){shopTheme::footer(canvas,l);paintWallet(canvas,domain,l.currencyHud,{},{},false);}
 shopTheme::panel(canvas,l.close,u,shopTheme::Surface::Close,pressed==6);if(hover==6)canvas.outline(l.close,white,12*u,4*u);label("X",l.close,40);
}
void paintCatalog(Canvas& canvas,const Domain& domain,const ShopLayout& l,const ShopState& state,int hover,int pressed,const TankShopState* tanks,std::span<const ShopItem> inventory,bool dimBackdrop=true){
 const float u=l.unit;const auto ink=shopTheme::ink,white=shopTheme::white;
 using Surface=shopTheme::Surface;
 const std::array<std::string_view,6> names{"Fish","Plants","Decorations","Treasure","Backgrounds","Tanks"};
 auto label=[&](std::string_view text,Rect r,float size,Color color=shopTheme::white){canvas.text(text,r.x+r.w*.5f,r.y+(r.h-size*u)*.5f,size*u,color,true,r.w-12*u,true,true);};
 auto token=[&](Rect r,bool pearl){paintCurrencyIcon(canvas,r,pearl);};
 if(l.inventory){
  if(dimBackdrop)canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,150});
  canvas.gradient({l.dialog.x,l.dialog.y+4*u,l.dialog.w,l.dialog.h},{4,24,45,130},{4,24,45,130},20*u);
  canvas.gradient(l.dialog,shopTheme::dialogEdge,shopTheme::dialogEdge,20*u);
  canvas.outline({l.dialog.x+4*u,l.dialog.y+4*u,l.dialog.w-8*u,l.dialog.h-8*u},{112,201,225,115},16*u,u);
 }
 shopTheme::backdrop(canvas,l);
 for(int i=0;i<6;++i){
  if(l.tabs[i].w<=0)continue;
  const int control=i>=4?i+3:i;
  auto tab=l.tabs[i];const bool active=i==int(state.category);if(active){tab.y-=20*u;tab.h+=24*u;}
  shopTheme::panel(canvas,tab,u,active?Surface::SelectedTab:Surface::Tab,pressed==control);
  if(hover==control)canvas.outline(tab,white,12*u,4*u);
  if(i==4)canvas.icon("hud-icons/shop-tab-backgrounds-v1.png",{tab.x+16*u,tab.y+8*u,tab.w-32*u,tab.h-44*u});
  else if(i==5)canvas.icon("tank-grid/active.png",{tab.x+4*u,tab.y+8*u,tab.w-8*u,tab.h-40*u});
  else shopTheme::tabArt(canvas,{tab.x+16*u,tab.y+8*u,tab.w-32*u,tab.h-44*u},i);
  label(names[i],{tab.x,tab.y+tab.h-36*u,tab.w,32*u},24,ink);
 }
 if(l.inventory)label("Inventory",l.dialogTitle,48);
 label(names[int(state.category)],l.title,48,ink);
 if(state.category==ShopCategory::Treasure)label("USD prices · Coming soon",{l.title.x+l.title.w-360*u,l.title.y,360*u,l.title.h},18,ink);
 for(int i=0;i<shopSubtabCount(state.category,l.inventory);++i){
  const auto tab=shopSubtabBounds(l,state.category,i);
  const int control=shopSubtabControl(i);
  shopTheme::panel(canvas,tab,u,state.subtab==i?Surface::Positive:Surface::Button,pressed==control);
  if(hover==control)canvas.outline(tab,white,12*u,4*u);
  label(state.category==ShopCategory::Environment?(i==0?"All":"Owned"):state.category==ShopCategory::Treasure?(i==0?"All":i==1?"Coins":"Pearls"):(i==0?"All":"Unlocked"),tab,28);
 }
 shopTheme::panel(canvas,l.body,u,Surface::Well);
 if(state.category==ShopCategory::Tanks){paintTankShop(canvas,domain,l,tanks?*tanks:TankShopState{});paintMenuChrome(canvas,domain,l,hover,pressed);if(tanks)paintTankPurchase(canvas,l,*tanks,domain.state().settings.reducedMotion);return;}
 const auto offers=l.inventory?std::vector<ShopItem>{}:shopItems(domain,state);
 const std::span<const ShopItem> items=l.inventory?inventory:std::span<const ShopItem>{offers};
 canvas.clip(l.cardViewport);
 for(std::size_t i=0;i<items.size();++i){
  Rect card=shopCardBounds(l,state,i);if(card.x+card.w<l.body.x||card.x>l.body.x+l.body.w)continue;
  const auto& item=items[i];
  if(!item.environmentId.empty()){
   const bool selected=item.price=="Selected";
   canvas.gradient({card.x,card.y+7*u,card.w,card.h},{8,62,76,150},{8,62,76,150},18*u);
   canvas.gradient(card,{255,242,193},{223,185,112},16*u);
   canvas.outline(card,{255,251,219},16*u,3*u);
   const Rect art{card.x+12*u,card.y+12*u,card.w-24*u,card.h-104*u};
   canvas.environment(art,item.environmentId);
   canvas.outline(art,{37,111,120},2*u,3*u);
   canvas.gradient({art.x,art.y,art.w,5*u},{255,255,241,110},{255,255,241,0});
   const Rect footer{card.x+16*u,art.y+art.h+12*u,card.w-32*u,64*u};
   const float buttonWidth=std::min(180*u,footer.w*.38f);
   canvas.text(item.name,footer.x,footer.y+18*u,28*u,ink,false,footer.w-buttonWidth-12*u,true,true);
   const Rect button{footer.x+footer.w-buttonWidth,footer.y+4*u,buttonWidth,56*u};
   shopTheme::panel(canvas,button,u,selected?Surface::Button:Surface::Buy);
   if(item.price.ends_with(" Coins")){
    token({button.x+12*u,button.y+10*u,36*u,36*u},false);
    label(item.price.substr(0,item.price.size()-6),{button.x+48*u,button.y,button.w-54*u,button.h},28);
   }else label(selected?"Selected":"Use",button,24);
   continue;
  }
  shopTheme::card(canvas,card,u,item.locked);
  label(item.name,{card.x+(item.fish?52:4)*u,card.y+12*u,card.w-(item.fish?60:8)*u,56*u},28,white);
  if(item.fish){const auto info=shopInfoBounds(card,u);shopTheme::panel(canvas,info,u,Surface::Button);canvas.gradient({info.x+18*u,info.y+8*u,4*u,4*u},white,white,2*u);canvas.gradient({info.x+18*u,info.y+16*u,4*u,16*u},white,white,2*u);}
  if(l.inventory){
   const std::string quantity="×"+std::to_string(item.quantity);
   const float width=std::min(card.w-24*u,std::max(72*u,canvas.textWidth(quantity,28*u,true,false,false,false,true)+28*u));
   shopTheme::badge(canvas,{card.x+(card.w-width)*.5f,card.y+72*u,width,40*u},u,quantity);
  }
  else if(!item.fish)label(item.detail,{card.x,card.y+72*u,card.w,40*u},28,item.locked?Color{235,229,194,255}:white);
  else if(item.locked)label(item.detail,{card.x,card.y+60*u,card.w,28*u},20,Color{235,229,194,255});
  Rect art{card.x+24*u,card.y+124*u,card.w-48*u,std::max(32*u,card.h-224*u)};
  if(!item.purchaseReward.empty())art.h=std::max(32*u,card.h-248*u);
  if(item.fish)art={card.x+24*u,card.y+80*u,card.w-48*u,std::max(24*u,card.h-288*u)};
  if(!item.environmentId.empty()){
   const float previewHeight=std::min(art.h,art.w*941.f/1672.f);
   canvas.environment({art.x,art.y+(art.h-previewHeight)*.5f,art.w,previewHeight},item.environmentId);
  }else if(!item.asset.empty())canvas.icon(item.asset,art,item.locked?.6f:1.f,item.flipped);
  else{const float side=std::min(art.w,art.h)*.6f;token({art.x+(art.w-side)*.5f,art.y+(art.h-side)*.5f,side,side},item.detail.ends_with(" Pearls"));}
  if(item.locked){
   const float lock=std::min(76*u,art.h*.6f);
   shopTheme::lockIcon(canvas,{art.x+(art.w-lock)*.5f,art.y+(art.h-lock)*.5f,lock,lock});
  }
  if(!item.treasureId.empty())label("Coming soon",{card.x+8*u,card.y+card.h-102*u,card.w-16*u,26*u},18,white);
  if(!item.purchaseReward.empty())label(item.purchaseReward,{card.x+8*u,card.y+card.h-112*u,card.w-16*u,28*u},24,white);
  if(item.fish){
   const auto& offer=*item.fish;const auto& q=offer.quote;
   const Rect info{card.x+12*u,card.y+card.h-196*u,card.w-24*u,80*u};
   const auto textInk=white;
   {
    const std::string first=shopDuration(offer.firstGrowthMs()),adult="Adult: "+shopDuration(q.durationMs);
    const float size=24*u,firstWidth=canvas.textWidth(first,size,true,false,false,false,true),adultWidth=canvas.textWidth(adult,size,true,false,false,false,true);
    const float timingWidth=firstWidth+adultWidth+48*u,timingScale=std::min(1.f,(info.w-8*u)/timingWidth);
    const float timingLeft=info.x+(info.w-timingWidth*timingScale)*.5f;
    shopTheme::clockIcon(canvas,{timingLeft,info.y,24*u*timingScale,24*u*timingScale});
    canvas.text(first,timingLeft+32*u*timingScale,info.y,size*timingScale,textInk,false,firstWidth*timingScale,true,true);
    canvas.text(adult,timingLeft+(firstWidth+48*u)*timingScale,info.y,size*timingScale,textInk,false,adultWidth*timingScale,true,true);
    const std::string heading="Adult sale:",value=compact(q.principal+q.profit)+" · "+compact(q.xp)+" XP";
    const float headingWidth=canvas.textWidth(heading,size,true,false,false,false,true),valueWidth=canvas.textWidth(value,size,true,false,false,false,true);
    const float total=headingWidth+valueWidth+36*u,scale=std::min(1.f,(info.w-8*u)/total);
    const float left=info.x+(info.w-total*scale)*.5f,top=info.y+36*u;
    canvas.text(heading,left,top,size*scale,textInk,false,headingWidth*scale,true,true);
    token({left+(headingWidth+4*u)*scale,top,24*u*scale,24*u*scale},false);
    canvas.text(value,left+(headingWidth+36*u)*scale,top,size*scale,textInk,false,valueWidth*scale,true,true);
    if(offer.fastGrowing()){
     const float badgeWidth=std::min(card.w-32*u,std::ceil((canvas.textWidth("Fast Growing",24*u,true,false,false,false,true)/u+24)/4)*4*u);
     shopTheme::badge(canvas,{card.x+(card.w-badgeWidth)*.5f,card.y+card.h-120*u,badgeWidth,40*u},u,"Fast Growing");
    }
   }
  }
  Rect price{card.x+12*u,card.y+card.h-68*u,card.w-24*u,56*u};
  shopTheme::pricePanel(canvas,price,u);
  const bool pearlPrice=item.price.ends_with(" Pearls"),coinPrice=item.price.ends_with(" Coins");
  if(coinPrice||pearlPrice){
   const auto amount=item.price.substr(0,item.price.size()-(pearlPrice?7:6));
   const float textWidth=canvas.textWidth(amount,28*u,true,false,false,false,true);
   const float scale=std::min(1.f,(price.w-16*u)/(textWidth+40*u));
   const float left=price.x+(price.w-(textWidth+40*u)*scale)*.5f;
   canvas.text(amount,left,price.y+(price.h-28*u*scale)*.5f,28*u*scale,white,false,textWidth*scale,true,true);
   token({left+(textWidth+8*u)*scale,price.y+(price.h-32*u*scale)*.5f,32*u*scale,32*u*scale},pearlPrice);
  }else label(item.price,price,28);
 }
 if(items.empty())label(l.inventory?(state.category==ShopCategory::Plants?"No stored plants":"No stored decorations"):"No items available",l.body,32);
 canvas.clearClip();
 const float visible=state.category==ShopCategory::Environment?2.f:float(l.visibleCards);
 if(items.size()>visible){const auto track=l.scrollTrack;shopTheme::scrollbar(canvas,track,{track.x+track.w*state.scroll/items.size(),track.y,track.w*visible/items.size(),track.h},u);}
 paintMenuChrome(canvas,domain,l,hover,pressed);
}
}
void paintShop(Canvas& canvas,const Domain& domain,const ShopLayout& l,const ShopState& state,int hover,int pressed,const TankShopState* tanks){paintCatalog(canvas,domain,l,state,hover,pressed,tanks,{});}
void paintInventory(Canvas& canvas,const Domain& domain,const ShopLayout& l,const ShopState& state,std::span<const ShopItem> items,int hover,int pressed,bool dimBackdrop){paintCatalog(canvas,domain,l,state,hover,pressed,nullptr,items,dimBackdrop);}

void paintShopFishDetails(Canvas& canvas,const Domain& domain,const HudDialogLayout& layout,const Species& species){
 const float u=layout.unit;const auto r=layout.content;const auto q=domain.quote(species);
 const Color ink{35,65,78,255},muted{78,109,115,255},white{249,255,247,255};
 auto box=[&](float x,float y,float w,float h){return Rect{r.x+x*u,r.y+y*u,w*u,h*u};};
 auto text=[&](std::string_view value,float x,float y,float w,float size,Color color,bool center=false,bool strong=false){
  canvas.text(value,r.x+(x+(center?w*.5f:0))*u,r.y+y*u,size*u,color,center,w*u,strong,false);
 };
 const float width=r.w/u,height=r.h/u,left=width*.44f,right=left+32,rw=width-right;
 // One generous portrait and a short, readable care card.
 canvas.gradient(box(0,0,left,height),{136,222,222},{43,143,172},24*u);
 canvas.outline(box(0,0,left,height),{225,255,241,255},24*u,3*u);
 canvas.gradient(box(36,108,left-72,330),{198,250,230,100},{170,242,232,0},140*u);
 for(int i=0;i<6;++i){const float d=12+(i%3)*10;canvas.outline(box(28+(i*89)%int(left-64),112+(i*63)%340,d,d),{227,255,247,100},d*u*.5f,2*u);}
 std::string rarity=species.rarity;if(!rarity.empty()&&rarity.front()>='a'&&rarity.front()<='z')rarity.front()-=32;
 canvas.round(box(24,24,176,48),{249,240,204,255},24*u,{255,255,238,255},2*u,false,false);
 text(rarity,32,29,160,28,ink,true,true);
 text("Lv. "+std::to_string(species.level),left-136,32,108,28,ink,true,true);
 canvas.icon(species.asset,box(28,132,left-56,300));
 text("Fish",24,height-110,left-48,34,white,true,true);
 text("From tiny egg to fully grown",24,height-62,left-48,26,white,true);
 text("Care guide",right,0,rw,40,ink,false,true);
 text("Hatches in "+shopDuration(domain.content().hatchMs)+". Uses 1 tank space.",right,58,rw,27,muted);
 auto row=[&](float y,std::string_view label,const std::string& value,bool clock){
  canvas.round(box(right,y,rw,100),{255,253,238,255},16*u,{211,219,199,255},2*u,false,false);
  if(clock)shopTheme::clockIcon(canvas,box(right+22,y+29,40,40),{58,129,139,255});
  else canvas.icon("hud-icons/shop-tab-fish-v1.png",box(right+16,y+20,52,56));
  text(label,right+82,y+12,rw-106,24,muted);
  text(value,right+82,y+44,rw-106,34,ink,false,true);
 };
 row(116,"Feed every",shopDuration(q.feedMs),true);
 row(232,"Fully grown in",shopDuration(q.durationMs),false);
 text("Keep it fed. Growth pauses when hungry.",right,348,rw,26,muted);
 const float rewardY=402;
 canvas.round(box(right,rewardY,rw,112),{218,236,191,255},16*u,{156,183,123,255},2*u,false,false);
 {
  text("Adult sale reward",right+22,rewardY+12,rw-44,25,muted);
  paintCurrencyIcon(canvas,box(right+22,rewardY+54,40,40),false);
  text(compact(q.principal+q.profit)+" coins  +  "+compact(q.xp)+" XP",right+76,rewardY+50,rw-98,34,ink,false,true);
 }
 const auto cost=species.currency==Currency::Coins?q.principal:species.price;
 const std::string price=compact(cost)+(species.currency==Currency::Pearls?(cost==1?" pearl":" pearls"):" coins");
 text("Shop price: "+price,right,height-80,rw,28,ink,false,true);
 text(species.level>domain.level()?"Unlocks at level "+std::to_string(species.level):species.currency==Currency::Pearls?"Sell for coins and XP. Pearls are not refunded.":"Adult sale includes your purchase cost",right,height-38,rw,24,muted);

}

HudDialogLayout layoutDialog(float width,float height,Insets safe,const DialogSpec& spec){
 if(!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0)throw std::invalid_argument("Invalid dialog viewport");
 float w=640,h=392;
 switch(spec.size){case DialogSize::Small:break;case DialogSize::Medium:w=960;h=640;break;case DialogSize::Large:w=1280;h=768;break;
 case DialogSize::Custom:
  if(!std::isfinite(spec.customWidth)||!std::isfinite(spec.customHeight)||spec.customWidth<240||spec.customHeight<192)throw std::invalid_argument("Custom dialogs must be at least 240 by 192 design units");
  w=std::ceil(spec.customWidth/4)*4;h=std::ceil(spec.customHeight/4)*4;break;}
 const float base=std::min(width/hudTokens::canvasWidth,height/hudTokens::canvasHeight);
 const float availableW=width-safe.left-safe.right-48*base,availableH=height-safe.top-safe.bottom-48*base;
 if(availableW<=0||availableH<=0)throw std::invalid_argument("No safe area for dialog");
 const float u=std::min({base,availableW/w,availableH/h});
 static thread_local HudContext state;ContextGuard guard;Clay_SetCurrentContext(state.context);Clay_SetLayoutDimensions({w*u,h*u});state.error.clear();
 auto element=[](int index,Clay_ElementDeclaration declaration){declaration.id=CLAY_IDI("standard-dialog",index);return declaration;};
 auto leafBox=[&](int index,Clay_SizingAxis boxWidth,Clay_SizingAxis boxHeight){CLAY(element(index,flow(boxWidth,boxHeight,CLAY_LEFT_TO_RIGHT))) {}};
 Clay_BeginLayout();
 CLAY(flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_TOP_TO_BOTTOM,0,8*u)){
  auto header=flow(CLAY_SIZING_GROW(),CLAY_SIZING_FIXED(80*u),CLAY_LEFT_TO_RIGHT,8*u,8*u);header.layout.childAlignment.y=CLAY_ALIGN_Y_CENTER;
  CLAY(element(0,header)){
   leafBox(4,CLAY_SIZING_FIXED(64*u),CLAY_SIZING_FIXED(64*u));
   leafBox(1,CLAY_SIZING_GROW(),CLAY_SIZING_GROW());
   leafBox(2,CLAY_SIZING_FIXED(64*u),CLAY_SIZING_FIXED(64*u));
  }
  CLAY(element(3,flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_TOP_TO_BOTTOM,0,24*u))){leafBox(5,CLAY_SIZING_GROW(),CLAY_SIZING_GROW());}
 }
 Clay_EndLayout();if(!state.error.empty())throw std::runtime_error(state.error);
 const float x=safe.left+(width-safe.left-safe.right-w*u)*.5f,y=safe.top+(height-safe.top-safe.bottom-h*u)*.5f;
 auto bounds=[&](int index){const auto b=Clay_GetElementData(CLAY_IDI("standard-dialog",index)).boundingBox;return Rect{x+b.x,y+b.y,b.width,b.height};};
 return {{0,0,width,height},{x,y,w*u,h*u},bounds(0),bounds(1),bounds(2),bounds(3),bounds(5),u};
}
void paintDialog(Canvas& canvas,const HudDialogLayout& l,const DialogSpec& spec,bool hover,bool pressed,DialogPresentation presentation){
 const bool popover=presentation==DialogPresentation::Popover;
 const float u=l.unit;const Color outline=shopTheme::dialogEdge,white{255,255,246,255};
 if(presentation==DialogPresentation::Modal)canvas.fill(l.backdrop,{0,0,0,150});
 canvas.gradient({l.frame.x,l.frame.y+4*u,l.frame.w,l.frame.h},{4,24,45,130},{4,24,45,130},20*u);
 canvas.gradient(l.frame,outline,outline,20*u);
 canvas.gradient({l.frame.x+4*u,l.frame.y+4*u,l.frame.w-8*u,l.frame.h-8*u},{35,111,153},{13,57,100},16*u);
 canvas.outline({l.frame.x+4*u,l.frame.y+4*u,l.frame.w-8*u,l.frame.h-8*u},{112,201,225,115},16*u,u);
 shopTheme::blueHeader(canvas,l.header,u,12*u);
 if(popover)canvas.gradient(l.body,shopTheme::dialogPaper,shopTheme::dialogPaper,12*u);
 else canvas.round(l.body,shopTheme::dialogPaper,12*u,{},0,false,false);
 const float font=32*u;
 canvas.text(spec.title,l.title.x+(popover?0:l.title.w*.5f),l.title.y+(l.title.h-font)*.5f,font,white,!popover,l.title.w,true,true);
 canvas.round(l.close,pressed?Color{173,46,39,255}:Color{235,72,65,255},12*u,hover?white:outline,4*u,true,true);
 const float closeFont=(popover?32:40)*u;
 canvas.text("X",l.close.x+l.close.w*.5f,l.close.y+(l.close.h-closeFont)*.5f,closeFont,white,true,l.close.w,true,true);
}
bool dialogEvent(DialogState& state,const HudDialogLayout& layout,const SDL_Event& event,SDL_FPoint point){
 if(!state.open)return false;
 point=state.motion.inputPoint(point);
 if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST){state.closePressed=state.backdropPressed=false;return false;}
 if(event.type==SDL_EVENT_KEY_DOWN){if(event.key.key==SDLK_ESCAPE){state.open=false;state.closePressed=state.backdropPressed=false;}return true;}
 if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN){state.closePressed=event.button.button==SDL_BUTTON_LEFT&&layout.close.has(point.x,point.y);state.backdropPressed=event.button.button==SDL_BUTTON_LEFT&&!layout.frame.has(point.x,point.y);return true;}
 if(event.type==SDL_EVENT_MOUSE_BUTTON_UP){if(event.button.button==SDL_BUTTON_LEFT){if((state.closePressed&&layout.close.has(point.x,point.y))||(state.backdropPressed&&!layout.frame.has(point.x,point.y)))state.open=false;state.closePressed=state.backdropPressed=false;}return true;}
 return event.type==SDL_EVENT_MOUSE_MOTION||event.type==SDL_EVENT_MOUSE_WHEEL||event.type==SDL_EVENT_KEY_UP||event.type==SDL_EVENT_TEXT_INPUT||event.type==SDL_EVENT_FINGER_DOWN||event.type==SDL_EVENT_FINGER_UP||event.type==SDL_EVENT_FINGER_MOTION;
}

std::span<const HudPart> hudControls(){return controls;}
std::optional<HudPart> hudHit(const HudLayout& layout,SDL_FPoint point){
 for(const auto control:controls)if(layout[control].has(point.x,point.y))return control;
 return {};
}

void paintXpBadge(Canvas& canvas,Rect xp,float u,float progress,std::string_view label,float textSize){
 canvas.round({xp.x,xp.y+4*u,xp.w,xp.h},{9,37,47,100},12*u,{},0,false,false);
 canvas.round(xp,{38,122,170,255},12*u,{16,56,91,255},4*u,false,false);
 const Rect track{xp.x+4*u,xp.y+4*u,xp.w-8*u,xp.h-12*u};
 canvas.round(track,{22,76,111,255},8*u,{},0,false,false);
 if(progress>0){
  const float fillWidth=track.w*std::clamp(progress,0.f,1.f);
  canvas.round({track.x,track.y,fillWidth,track.h},{62,202,245,255},8*u,{},0,false,false);
  if(fillWidth>8*u)canvas.round({track.x+4*u,track.y+4*u,fillWidth-8*u,4*u},{189,247,255,230},4*u,{},0,false,false);
 }
 canvas.round({xp.x+12*u,xp.y+4*u,std::min(40*u,xp.w-24*u),4*u},{206,247,255,150},4*u,{},0,false,false);
 canvas.text(label,xp.x+xp.w*.5f,xp.y+(xp.h-textSize)*.5f,textSize,{245,253,255,255},true,xp.w,true,true);
}

void paintHud(Canvas& canvas,const Domain& domain,const HudLayout& layout,std::optional<HudPart> hover,std::optional<HudPart> pressed,const HudRewardDisplay* rewards){
 constexpr Color ink{18,49,65,255},cream{246,233,201,255},gold{255,207,74,255},white{245,253,255,255};
 const float u=layout.unit;
 const auto& state=domain.state();
 auto panel=[&](HudPart part,Color fill){
  auto box=layout[part];
  if(pressed==part){fill.r=Uint8(fill.r*.9f);fill.g=Uint8(fill.g*.9f);fill.b=Uint8(fill.b*.9f);}
  canvas.round(box,fill,hudTokens::panelRadius*u,hover==part?white:ink,hudTokens::border*u,true,false);
 };
 auto text=[&](HudPart part,std::string_view value,float size,Color color,bool center=true){
  const auto box=layout[part];
  canvas.text(value,center?box.x+box.w*.5f:box.x,box.y+(box.h-size)*.5f,size,color,center,box.w,true,true);
 };
 auto artSlot=[&](HudPart part){
  auto box=layout[part];
  if(part==HudPart::SettingsIcon){canvas.icon("hud-icons/settings-v1.png",box);return;}
  const float step=hudTokens::smallGap*u;
  const float side=std::floor(std::min(box.w,box.h)/step)*step-hudTokens::overlap*u;
  if(side<step)return;
  box={box.x+(box.w-side)*.5f,box.y+(box.h-side)*.5f,side,side};
  canvas.round(box,{255,255,255,45},hudTokens::radius*u,{18,49,65,90},hudTokens::border*u,false,false);
  const float dot=std::min(hudTokens::dot*u,side);
  canvas.round({box.x+(side-dot)*.5f,box.y+(side-dot)*.5f,dot,dot},{18,49,65,35},hudTokens::radius*u,{},0,false,false);
 };
 const double displayedXp=rewards?rewards->xp:double(state.xp);
 const Amount wholeXp=static_cast<Amount>(std::floor(displayedXp));
 const int level=levelFor(domain.content(),wholeXp);const auto base=domain.content().levels[level-1];
 const bool maximum=level>=int(domain.content().levels.size());
 const auto next=maximum?base+1:domain.content().levels[level];
 const float progress=maximum?1.f:std::clamp(float(displayedXp-double(base))/float(next-base),0.f,1.f);
 auto xp=layout[HudPart::Xp];
 const float pulse=rewards?rewards->xpPulse:0,bounce=rewards&&!rewards->reducedMotion?pulse:0;
 const float grow=3*u*bounce;xp.y-=grow;xp.h+=2*grow;
 paintXpBadge(canvas,xp,u,progress,maximum?"MAX LEVEL":compact(wholeXp-base)+" / "+compact(next-base)+" XP",hudTokens::textSmall*u);
 if(pulse>0)canvas.outline({xp.x-2*u,xp.y-2*u,xp.w+4*u,xp.h+4*u},{178,248,255,Uint8(230*pulse)},14*u,3*u);
 panel(HudPart::Level,gold);text(HudPart::Level,std::to_string(level),hudTokens::textLarge*u,ink);
 paintWallet(canvas,domain,layout,hover,pressed,true,rewards);
 // Match the Settings button's bottom shadow beneath the generated artwork.
 for(const auto part:{HudPart::Tank,HudPart::Shop,HudPart::Food,HudPart::Rehome,HudPart::Layout,HudPart::Projects,HudPart::Bag,HudPart::Rewards}){
  auto box=layout[part];
  if(pressed==part){box.x+=2*u;box.y+=2*u;box.w-=4*u;box.h-=4*u;}
  canvas.round({box.x,box.y+4*u,box.w,box.h},{9,37,47,100},box.w*.16f,{},0,false,false);
 }
 auto layoutBox=layout[HudPart::Layout];
 if(pressed==HudPart::Layout){layoutBox.x+=2*u;layoutBox.y+=2*u;layoutBox.w-=4*u;layoutBox.h-=4*u;}
 const float layoutScaleX=layoutBox.w/1162.f,layoutScaleY=layoutBox.h/1154.f;
 canvas.image("hud-icons/layout-button-v1.png",{layoutBox.x-45.f*layoutScaleX,layoutBox.y-48.f*layoutScaleY,1254.f*layoutScaleX,1254.f*layoutScaleY});
 auto projectsBox=layout[HudPart::Projects];
 if(pressed==HudPart::Projects){projectsBox.x+=2*u;projectsBox.y+=2*u;projectsBox.w-=4*u;projectsBox.h-=4*u;}
 // Fit the visible outline to the original 108-unit button.
 const float projectsScaleX=projectsBox.w/1116.f,projectsScaleY=projectsBox.h/1092.f;
 canvas.image("hud-icons/projects-button-v3.png",{projectsBox.x-67.f*projectsScaleX,projectsBox.y-70.f*projectsScaleY,1254.f*projectsScaleX,1254.f*projectsScaleY});
 auto bagBox=layout[HudPart::Bag];
 if(pressed==HudPart::Bag){bagBox.x+=2*u;bagBox.y+=2*u;bagBox.w-=4*u;bagBox.h-=4*u;}
 // Fit the visible outline to the original 108-unit button.
 const float bagScaleX=bagBox.w/1118.f,bagScaleY=bagBox.h/1093.f;
 canvas.image("hud-icons/bag-button-v3.png",{bagBox.x-68.f*bagScaleX,bagBox.y-70.f*bagScaleY,1254.f*bagScaleX,1254.f*bagScaleY});
 auto foodBox=layout[HudPart::Food];
 if(pressed==HudPart::Food){foodBox.x+=2*u;foodBox.y+=2*u;foodBox.w-=4*u;foodBox.h-=4*u;}
 // Fit the visible outline to the original 64-unit button.
 const float foodScaleX=foodBox.w/1116.f,foodScaleY=foodBox.h/1093.f;
 canvas.image("hud-icons/food-button-v3.png",{foodBox.x-68.f*foodScaleX,foodBox.y-70.f*foodScaleY,1254.f*foodScaleX,1254.f*foodScaleY});
 auto rehomeBox=layout[HudPart::Rehome];
 if(pressed==HudPart::Rehome){rehomeBox.x+=2*u;rehomeBox.y+=2*u;rehomeBox.w-=4*u;rehomeBox.h-=4*u;}
 const float rehomeScaleX=rehomeBox.w/1081.f,rehomeScaleY=rehomeBox.h/1063.f;
 canvas.image("hud-icons/rehome-button-v1.png",{rehomeBox.x-87.f*rehomeScaleX,rehomeBox.y-94.f*rehomeScaleY,1254.f*rehomeScaleX,1254.f*rehomeScaleY});
 auto rewardsBox=layout[HudPart::Rewards];
 if(pressed==HudPart::Rewards){rewardsBox.x+=2*u;rewardsBox.y+=2*u;rewardsBox.w-=4*u;rewardsBox.h-=4*u;}
 // Exclude transparent padding when fitting the 64-unit button.
 const float rewardsScaleX=rewardsBox.w/1117.f,rewardsScaleY=rewardsBox.h/1090.f;
 canvas.image("hud-icons/rewards-button-v3.png",{rewardsBox.x-68.f*rewardsScaleX,rewardsBox.y-71.f*rewardsScaleY,1254.f*rewardsScaleX,1254.f*rewardsScaleY});
 auto tankBox=layout[HudPart::Tank];
 if(pressed==HudPart::Tank){tankBox.x+=2*u;tankBox.y+=2*u;tankBox.w-=4*u;tankBox.h-=4*u;}
 // Fit the visible outline to the same 144-unit bounds as Shop.
 const float tankScaleX=tankBox.w/1117.f,tankScaleY=tankBox.h/1094.f;
 canvas.image("hud-icons/tank-button-v3.png",{tankBox.x-68.f*tankScaleX,tankBox.y-69.f*tankScaleY,1254.f*tankScaleX,1254.f*tankScaleY});
 auto shopBox=layout[HudPart::Shop];
 if(pressed==HudPart::Shop){shopBox.x+=2*u;shopBox.y+=2*u;shopBox.w-=4*u;shopBox.h-=4*u;}
 // Map the opaque button outline, excluding generated transparent padding,
 // onto the original Clay button bounds. Preserve the source PNG alpha.
 const float shopScaleX=shopBox.w/1113.f,shopScaleY=shopBox.h/1090.f;
 canvas.image("hud-icons/shop-button-v3.png",{shopBox.x-70.f*shopScaleX,shopBox.y-72.f*shopScaleY,1254.f*shopScaleX,1254.f*shopScaleY});
 panel(HudPart::Settings,cream);artSlot(HudPart::SettingsIcon);
}
}
