#include "aquarium/hud.hpp"
#include "aquarium/hud_dialog.hpp"
#include "aquarium/hud_tokens.hpp"
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
 HudPart::CoinPlus,HudPart::PearlPlus,HudPart::Tank,HudPart::Projects,HudPart::Bag,HudPart::Settings,HudPart::Food,HudPart::Rewards,HudPart::Shop};
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
  CLAY(named(part,flow(CLAY_SIZING_FIXED(w),CLAY_SIZING_FIXED(h),CLAY_TOP_TO_BOTTOM,((part==HudPart::Food||part==HudPart::Rewards)?hudTokens::smallGap:hudTokens::overlap)*u,((part==HudPart::Food||part==HudPart::Rewards)?hudTokens::smallGap:hudTokens::padding)*u))){
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

ShopLayout layoutShop(float width,float height,Insets safe,float minimumTouch){
 if(width<=0||height<=0)throw std::invalid_argument("Shop dimensions must be positive");
 static thread_local HudContext state;ContextGuard guard;
 Clay_SetCurrentContext(state.context);Clay_SetLayoutDimensions({width,height});state.error.clear();
 const float u=std::min(width/hudTokens::canvasWidth,height/hudTokens::canvasHeight);
 auto element=[&](int idValue,Clay_ElementDeclaration d){d.id=CLAY_IDI("shop",idValue);return d;};
 auto box=[&](int idValue,Clay_SizingAxis w,Clay_SizingAxis h){CLAY(element(idValue,flow(w,h,CLAY_LEFT_TO_RIGHT))) {}};
 Clay_BeginLayout();
 auto root=flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_TOP_TO_BOTTOM);
 root.layout.padding={pixels(safe.left+16*u),pixels(safe.right+16*u),pixels(safe.top+24*u),pixels(safe.bottom+16*u)};
 CLAY(root){
  auto header=flow(CLAY_SIZING_GROW(),CLAY_SIZING_FIXED(152*u),CLAY_LEFT_TO_RIGHT,8*u);header.layout.childAlignment.y=CLAY_ALIGN_Y_BOTTOM;
  CLAY(element(0,header)){
   box(30,CLAY_SIZING_FIXED(64*u),CLAY_SIZING_FIXED(0));spacer(CLAY_LEFT_TO_RIGHT);
   for(int i=0;i<4;++i)box(10+i,CLAY_SIZING_FIXED(176*u),CLAY_SIZING_FIXED(128*u));
   spacer(CLAY_LEFT_TO_RIGHT);
   auto close=flow(CLAY_SIZING_FIXED(64*u),CLAY_SIZING_FIXED(64*u),CLAY_LEFT_TO_RIGHT);close.floating.attachTo=CLAY_ATTACH_TO_PARENT;close.floating.attachPoints.parent=CLAY_ATTACH_POINT_RIGHT_TOP;close.floating.attachPoints.element=CLAY_ATTACH_POINT_RIGHT_TOP;
   CLAY(element(1,close)){}
   box(31,CLAY_SIZING_FIXED(64*u),CLAY_SIZING_FIXED(0));
  }
  box(2,CLAY_SIZING_GROW(),CLAY_SIZING_FIXED(96*u));
  CLAY(flow(CLAY_SIZING_GROW(),CLAY_SIZING_FIXED(64*u),CLAY_LEFT_TO_RIGHT,12*u)){
   spacer(CLAY_LEFT_TO_RIGHT);for(int i=0;i<2;++i)box(14+i,CLAY_SIZING_FIXED(240*u),CLAY_SIZING_FIXED(56*u));spacer(CLAY_LEFT_TO_RIGHT);
  }
  CLAY(element(3,flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_LEFT_TO_RIGHT,20*u,12*u))){
   for(int i=0;i<5;++i)box(20+i,CLAY_SIZING_GROW(),CLAY_SIZING_GROW());
  }
  auto footer=flow(CLAY_SIZING_GROW(),CLAY_SIZING_FIXED(96*u),CLAY_LEFT_TO_RIGHT,32*u,16*u);footer.layout.childAlignment.y=CLAY_ALIGN_Y_CENTER;
  CLAY(element(4,footer)){
   spacer(CLAY_LEFT_TO_RIGHT);for(int i=0;i<2;++i)box(16+i,CLAY_SIZING_FIXED(280*u),CLAY_SIZING_FIXED(48*u));spacer(CLAY_LEFT_TO_RIGHT);
  }
 }
 Clay_EndLayout();if(!state.error.empty())throw std::runtime_error(state.error);
 auto bounds=[&](int i){auto b=Clay_GetElementData(CLAY_IDI("shop",i)).boundingBox;return Rect{b.x,b.y,b.width,b.height};};
 ShopLayout result{};result.page={0,0,width,height};result.unit=u;
 result.header=bounds(0);result.close=bounds(1);result.title=bounds(2);result.body=bounds(3);result.footer=bounds(4);
 for(int i=0;i<4;++i)result.tabs[i]=bounds(10+i);
 for(int i=0;i<2;++i){result.subtabs[i]=bounds(14+i);result.wallets[i]=bounds(16+i);}
 for(int i=0;i<5;++i)result.cards[i]=bounds(20+i);
 // Reuse the exact main HUD layout, translating the currency group only.
 result.currencyHud=layoutHud(width,height,safe,minimumTouch);
 const auto left=result.currencyHud[HudPart::CoinIcon],right=result.currencyHud[HudPart::Pearls];
 const float dx=result.footer.x+(result.footer.w-(right.x+right.w-left.x))*.5f-left.x;
 const float dy=result.footer.y+(result.footer.h-left.h)*.5f-left.y;
 for(auto& item:result.currencyHud.boxes){item.x+=dx;item.y+=dy;}

 return result;
}
std::vector<ShopItem> shopItems(const Domain& domain,const ShopState& state){
 std::vector<ShopItem> result;
 if(state.category==ShopCategory::Treasure){
  const std::string resource=state.subtab==0?"Coins":"Pearls";
  for(const auto name:{"Pocket of ","Pile of ","Bag of ","Box of ","Chest of "})result.push_back({name+resource,"",resource,"Coming soon",false});
 }else if(state.category==ShopCategory::Fish){
  for(const auto& item:domain.content().species){
   if(!item.artReady||item.releaseGate!="Launch")continue;
   const bool locked=item.level>domain.level();if(state.subtab==1&&locked)continue;
   const auto quote=domain.quote(item);
   const auto price=item.currency==Currency::Coins?quote.principal:item.price;
   result.push_back({item.name,item.asset,locked?"Level "+std::to_string(item.level):"",compact(price)+(item.currency==Currency::Pearls?" Pearls":" Coins"),locked,ShopFishOffer{item.id,quote,item.companion}});
  }
 }else{
  for(const auto& item:domain.content().decorations){
   if(!item.artReady||item.releaseGate!="Launch"||(item.category=="Plant")!=(state.category==ShopCategory::Plants))continue;
   const bool locked=item.level>domain.level();if(state.subtab==1&&locked)continue;
   result.push_back({item.name,item.asset,"Level "+std::to_string(item.level),compact(item.price)+(item.currency==Currency::Pearls?" Pearls":" Coins"),locked});
  }
 }
 return result;
}
namespace {
void paintWallet(Canvas& canvas,const Domain& domain,const HudLayout& layout,std::optional<HudPart> hover={},std::optional<HudPart> pressed={},bool showPlus=true){
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
  const auto amount=layout[pearl?HudPart::PearlAmount:HudPart::CoinAmount];
  canvas.text(compact(pearl?domain.state().wallet.pearls:domain.state().wallet.coins),amount.x+amount.w*.5f,amount.y+(amount.h-hudTokens::textBody*u)*.5f,hudTokens::textBody*u,counterEdge,true,amount.w,true,true);
  const auto icon=layout[pearl?HudPart::PearlIcon:HudPart::CoinIcon];
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
int shopControl(const ShopLayout& layout,SDL_FPoint point){
 if(layout.close.has(point.x,point.y))return 6;
 for(int i=0;i<4;++i)if(layout.tabs[i].has(point.x,point.y))return i;
 for(int i=0;i<2;++i)if(layout.subtabs[i].has(point.x,point.y))return 4+i;
 return -1;
}
void activateShopControl(ShopState& state,int control){
 if(control>=0&&control<4){state.category=static_cast<ShopCategory>(control);state.subtab=0;state.scroll=0;}
 else if(control==4||control==5){state.subtab=control-4;state.scroll=0;}
}
void scrollShop(ShopState& state,float delta,std::size_t count){state.scroll=std::clamp(state.scroll+delta,0.f,std::max(0.f,float(count)-5));}
std::optional<std::size_t> shopCardAt(const ShopLayout& layout,const ShopState& state,std::size_t count,SDL_FPoint point){
 if(!layout.body.has(point.x,point.y))return {};
 for(std::size_t i=0;i<count;++i)if(shopCardBounds(layout,state,i).has(point.x,point.y))return i;
 return {};
}
Rect shopCardBounds(const ShopLayout& layout,const ShopState& state,std::size_t index){auto card=layout.cards[0];card.x+=(float(index)-state.scroll)*(layout.cards[1].x-layout.cards[0].x);return card;}
Rect shopInfoBounds(Rect card,float u){return {card.x+8*u,card.y+8*u,40*u,40*u};}
namespace {
void paintMenuChrome(Canvas& canvas,const Domain& domain,const ShopLayout& l,int hover,int pressed){
 const float u=l.unit;const auto white=shopTheme::white;
 auto label=[&](std::string_view text,Rect r,float size){canvas.text(text,r.x+r.w*.5f,r.y+(r.h-size*u)*.5f,size*u,white,true,r.w-12*u,true,true);};
 shopTheme::footer(canvas,l);
 paintWallet(canvas,domain,l.currencyHud,{},{},false);
 shopTheme::panel(canvas,l.close,u,shopTheme::Surface::Close,pressed==6);if(hover==6)canvas.outline(l.close,white,12*u,4*u);label("X",l.close,40);
}
}
std::array<Rect,6> layoutTankCards(const ShopLayout& menu){
 static thread_local HudContext state;ContextGuard guard;
 const float top=menu.title.y,u=menu.unit;
 Clay_SetCurrentContext(state.context);Clay_SetLayoutDimensions({menu.header.w,menu.footer.y-top});state.error.clear();
 Clay_BeginLayout();
 CLAY(flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_TOP_TO_BOTTOM,24*u,24*u)){
  for(int row=0;row<2;++row){
   CLAY(flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_LEFT_TO_RIGHT,24*u)){
    for(int col=0;col<3;++col){
     auto card=flow(CLAY_SIZING_GROW(),CLAY_SIZING_GROW(),CLAY_LEFT_TO_RIGHT);card.id=CLAY_IDI("tank-card",row*3+col);CLAY(card){}
    }
   }
  }
 }
 Clay_EndLayout();if(!state.error.empty())throw std::runtime_error(state.error);
 std::array<Rect,6> cards{};
 for(int i=0;i<6;++i){const auto box=Clay_GetElementData(CLAY_IDI("tank-card",i)).boundingBox;cards[i]={menu.header.x+box.x,top+box.y,box.width,box.height};}
 return cards;
}
void paintTankMenu(Canvas& canvas,const Domain& domain,const ShopLayout& layout,int hover,int pressed){
 canvas.fill(layout.page,{234,232,217,255});
 canvas.fill({0,0,layout.page.w,layout.title.y},{15,38,46,255});
 for(const auto card:layoutTankCards(layout))canvas.round(card,{68,183,208,255},12*layout.unit,{24,44,48,255},4*layout.unit,true,true);
 paintMenuChrome(canvas,domain,layout,hover,pressed);
}
void paintShop(Canvas& canvas,const Domain& domain,const ShopLayout& l,const ShopState& state,int hover,int pressed){
 const float u=l.unit;const auto ink=shopTheme::ink,white=shopTheme::white;
 using Surface=shopTheme::Surface;
 const std::array<std::string_view,4> names{"Fish","Plants","Decorations","Treasure"};
 auto label=[&](std::string_view text,Rect r,float size,Color color=shopTheme::white){canvas.text(text,r.x+r.w*.5f,r.y+(r.h-size*u)*.5f,size*u,color,true,r.w-12*u,true,true);};
 auto token=[&](Rect r,bool pearl){paintCurrencyIcon(canvas,r,pearl);};
 shopTheme::backdrop(canvas,l);
 for(int i=0;i<4;++i){
  auto tab=l.tabs[i];const bool active=i==int(state.category);if(active){tab.y-=20*u;tab.h+=24*u;}
  shopTheme::panel(canvas,tab,u,active?Surface::SelectedTab:Surface::Tab,pressed==i);
  if(hover==i)canvas.outline(tab,white,12*u,4*u);
  shopTheme::tabArt(canvas,{tab.x+16*u,tab.y+8*u,tab.w-32*u,tab.h-44*u},i);
  label(names[i],{tab.x,tab.y+tab.h-36*u,tab.w,32*u},24,ink);
 }
 label(names[int(state.category)],l.title,48,ink);
 for(int i=0;i<2;++i){shopTheme::panel(canvas,l.subtabs[i],u,state.subtab==i?Surface::Positive:Surface::Button,pressed==4+i);if(hover==4+i)canvas.outline(l.subtabs[i],white,12*u,4*u);label(state.category==ShopCategory::Treasure?(i==0?"Coins":"Pearls"):(i==0?"All":"Unlocked"),l.subtabs[i],28);}
 shopTheme::panel(canvas,l.body,u,Surface::Well);
 const auto items=shopItems(domain,state);
 canvas.clip(l.body);
 for(std::size_t i=0;i<items.size();++i){
  Rect card=shopCardBounds(l,state,i);if(card.x+card.w<l.body.x||card.x>l.body.x+l.body.w)continue;
  const auto& item=items[i];
  if(item.fish)shopTheme::panel(canvas,card,u,item.locked?Surface::LockedFishCard:Surface::FishCard);
  else shopTheme::card(canvas,card,u,item.locked);
  label(item.name,{card.x+(item.fish?52:4)*u,card.y+12*u,card.w-(item.fish?60:8)*u,56*u},28,item.fish?shopTheme::fishInk:white);
  if(item.fish){const auto info=shopInfoBounds(card,u);shopTheme::panel(canvas,info,u,Surface::Button);canvas.gradient({info.x+18*u,info.y+8*u,4*u,4*u},white,white,2*u);canvas.gradient({info.x+18*u,info.y+16*u,4*u,16*u},white,white,2*u);}
  if(!item.fish)label(item.detail,{card.x,card.y+72*u,card.w,40*u},28,item.locked?Color{235,229,194,255}:white);
  else if(item.locked)label(item.detail,{card.x,card.y+60*u,card.w,28*u},20,shopTheme::fishInk);
  Rect art{card.x+24*u,card.y+124*u,card.w-48*u,std::max(32*u,card.h-224*u)};
  if(item.fish)art={card.x+24*u,card.y+80*u,card.w-48*u,std::max(24*u,card.h-288*u)};
  if(!item.asset.empty())canvas.icon(item.asset,art,item.locked?.6f:1.f);
  else{const float side=std::min(art.w,art.h)*.6f;token({art.x+(art.w-side)*.5f,art.y+(art.h-side)*.5f,side,side},state.subtab==1);}
  if(item.fish){
   const auto& offer=*item.fish;const auto& q=offer.quote;
   const Rect info{card.x+12*u,card.y+card.h-196*u,card.w-24*u,80*u};
   const auto textInk=shopTheme::fishInk;
   auto line=[&](std::string_view text,float y){canvas.text(text,info.x+info.w*.5f,y,24*u,textInk,true,info.w-8*u,true,true);};
   if(offer.companion){line("Already adult",info.y);line("No sale rewards",info.y+36*u);}
   else{
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
  if(item.fish)shopTheme::panel(canvas,price,u,item.locked?Surface::LockedPrice:Surface::Buy);
  else canvas.gradient(price,{41,132,148},{19,89,109},8*u);
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
 if(items.empty())label("No items available",l.body,32);
 canvas.clearClip();
 if(items.size()>5){const float available=l.body.w-24*u;canvas.gradient({l.body.x+12*u,l.body.y+l.body.h-12*u,available,8*u},{12,67,82},{12,67,82},4*u);canvas.gradient({l.body.x+12*u+available*state.scroll/items.size(),l.body.y+l.body.h-12*u,available*5/items.size(),8*u},{168,239,182},{104,201,155},4*u);}
 paintMenuChrome(canvas,domain,l,hover,pressed);
}

void paintShopFishDetails(Canvas& canvas,const Domain& domain,const HudDialogLayout& layout,const Species& species){
 const float u=layout.unit;const auto r=layout.content;const auto ink=shopTheme::ink;
 const auto q=domain.quote(species);
 auto line=[&](std::string_view text,float x,float y,float width,float size=24){canvas.text(text,x,y,size*u,ink,false,width,true,true);};
 canvas.icon(species.asset,{r.x,r.y,220*u,156*u});
 const float x=r.x+244*u,w=r.w-244*u;
 line(species.companion?"Permanent display companion":"Raise this fish and collect its adult reward.",x,r.y,w);
 line("Price: "+compact(species.currency==Currency::Coins?q.principal:species.price)+(species.currency==Currency::Pearls?" pearls":" coins"),x,r.y+40*u,w,28);
 if(species.level>domain.level())line("Unlocks at level "+std::to_string(species.level),x,r.y+80*u,w);
 else line("Available at your level",x,r.y+80*u,w);
 line(species.companion?"Already adult. No coin or XP sale rewards.":"Growth times start at purchase and assume regular feeding.",r.x,r.y+176*u,r.w);
 line("Feed every "+shopDuration(species.companion?43200000:q.feedMs)+(species.companion?".":". Growth pauses when hungry."),r.x,r.y+208*u,r.w);
 if(species.companion){line("Uses a display slot. You can keep it in your aquarium.",r.x,r.y+260*u,r.w);return;}
 constexpr std::array<std::string_view,5> stages{"Baby","Junior","Young","Mature","Adult"};
 const float y=r.y+256*u,row=36*u;
 canvas.gradient({r.x,y-4*u,r.w,32*u},{193,222,205},{193,222,205},4*u);
 line("Stage",r.x+12*u,y,r.w*.23f);line("Time from purchase",r.x+r.w*.24f,y,r.w*.35f);line("Sale reward",r.x+r.w*.64f,y,r.w*.35f);
 for(int i=0;i<5;++i){
  Fish fish;fish.age=i;fish.purchase=q;const auto reward=fishReward(fish);
  const auto elapsed=q.durationMs/10000*q.stages[i]+q.durationMs%10000*q.stages[i]/10000;
  const float top=y+(i+1)*row;
  line(stages[i],r.x+12*u,top,r.w*.23f);
  line(shopDuration(i==0?domain.content().hatchMs:elapsed),r.x+r.w*.24f,top,r.w*.35f);
  line(compact(reward.coins())+" coins · "+compact(reward.xp)+" XP",r.x+r.w*.64f,top,r.w*.35f);
 }
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
void paintDialog(Canvas& canvas,const HudDialogLayout& l,const DialogSpec& spec,bool hover,bool pressed){
 const float u=l.unit;const Color outline{8,36,67,255},white{255,255,246,255};
 canvas.fill(l.backdrop,{0,0,0,150});
 canvas.gradient({l.frame.x,l.frame.y+4*u,l.frame.w,l.frame.h},{4,24,45,130},{4,24,45,130},20*u);
 canvas.gradient(l.frame,outline,outline,20*u);
 canvas.gradient({l.frame.x+4*u,l.frame.y+4*u,l.frame.w-8*u,l.frame.h-8*u},{35,111,153},{13,57,100},16*u);
 canvas.outline({l.frame.x+4*u,l.frame.y+4*u,l.frame.w-8*u,l.frame.h-8*u},{112,201,225,115},16*u,u);
 canvas.gradient(l.header,{22,78,121},{11,48,88},12*u);
 const Rect water{l.header.x+8*u,l.header.y+8*u,l.header.w-16*u,l.header.h-16*u};
 canvas.wave(water,{87,201,222,65},12*u,384*u,.3f,12*u);
 canvas.wave(water,{158,225,250,35},8*u,560*u,2.1f,8*u);
 canvas.round(l.body,{235,235,222,255},12*u,{},0,false,false);
 const float font=32*u;
 canvas.text(spec.title,l.title.x+l.title.w*.5f,l.title.y+(l.title.h-font)*.5f,font,white,true,l.title.w,true,true);
 canvas.round(l.close,pressed?Color{173,46,39,255}:Color{235,72,65,255},12*u,hover?white:outline,4*u,true,true);
 canvas.text("X",l.close.x+l.close.w*.5f,l.close.y+(l.close.h-40*u)*.5f,40*u,white,true,l.close.w,true,true);
}
bool dialogEvent(DialogState& state,const HudDialogLayout& layout,const SDL_Event& event,SDL_FPoint point){
 if(!state.open)return false;
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

void paintHud(Canvas& canvas,const Domain& domain,const HudLayout& layout,std::optional<HudPart> hover,std::optional<HudPart> pressed){
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
 const int level=domain.level();const auto base=domain.content().levels[level-1];
 const bool maximum=level>=int(domain.content().levels.size());
 const auto next=maximum?base+1:domain.content().levels[level];
 const float progress=maximum?1.f:std::clamp(float(state.xp-base)/float(next-base),0.f,1.f);
 const auto xp=layout[HudPart::Xp];
 canvas.round({xp.x,xp.y+4*u,xp.w,xp.h},{9,37,47,100},12*u,{},0,false,false);
 canvas.round(xp,{38,122,170,255},12*u,{16,56,91,255},4*u,false,false);
 const Rect xpTrack{xp.x+4*u,xp.y+4*u,xp.w-8*u,xp.h-12*u};
 canvas.round(xpTrack,{22,76,111,255},8*u,{},0,false,false);
 // Keep the fill continuous while matching the solid currency-counter finish.
 if(progress>0){
  const float fillWidth=xpTrack.w*progress;
  canvas.round({xpTrack.x,xpTrack.y,fillWidth,xpTrack.h},{62,202,245,255},8*u,{},0,false,false);
  if(fillWidth>8*u)canvas.round({xpTrack.x+4*u,xpTrack.y+4*u,fillWidth-8*u,4*u},{189,247,255,230},4*u,{},0,false,false);
 }
 canvas.round({xp.x+12*u,xp.y+4*u,40*u,4*u},{206,247,255,150},4*u,{},0,false,false);
 text(HudPart::Xp,maximum?"MAX LEVEL":compact(state.xp-base)+" / "+compact(next-base)+" XP",hudTokens::textSmall*u,white);
 panel(HudPart::Level,gold);text(HudPart::Level,std::to_string(domain.level()),hudTokens::textLarge*u,ink);
 paintWallet(canvas,domain,layout,hover,pressed);
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
