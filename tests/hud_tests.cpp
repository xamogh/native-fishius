#include "aquarium/hud.hpp"
#include "aquarium/hud_dialog.hpp"
#include "aquarium/hud_placement.hpp"
#include "aquarium/hud_tokens.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool overlaps(Rect a,Rect b){return std::min(a.x+a.w,b.x+b.w)-std::max(a.x,b.x)>.5f&&std::min(a.y+a.h,b.y+b.h)-std::max(a.y,b.y)>.5f;}
HudLayout viewport(float w,float h,Insets safe={}){
 // Match Canvas's coordinate mapping, including its phone/tablet behavior.
 const float width=std::max(1608.f,830.f*w/h),height=width*h/w,density=width/w;
 const Insets in{safe.left*density,safe.top*density,safe.right*density,safe.bottom*density};
 const auto layout=layoutHud(width,height,in,44*density);
 for(const auto size:{DialogSize::Small,DialogSize::Medium,DialogSize::Large,DialogSize::Custom}){
  DialogSpec spec{"A configurable dialog title",size,803,483};const auto dialog=layoutDialog(width,height,in,spec);
  check(dialog.frame.x>=in.left&&dialog.frame.y>=in.top&&dialog.frame.x+dialog.frame.w<=width-in.right&&dialog.frame.y+dialog.frame.h<=height-in.bottom,"Dialog crosses safe area");
  check(dialog.content.w>0&&dialog.content.h>0&&dialog.content.y>=dialog.header.y+dialog.header.h,"Dialog has no content area");
  check(std::abs(dialog.close.w/dialog.unit-64)<.01f&&std::abs(dialog.close.h/dialog.unit-64)<.01f,"Dialog close size is incorrect");
  check(dialog.title.x+dialog.title.w<=dialog.close.x,"Dialog title overlaps close");
  if(size==DialogSize::Custom)check(std::abs(dialog.frame.w/dialog.unit-804)<.01f&&std::abs(dialog.frame.h/dialog.unit-484)<.01f,"Custom size does not snap to grid");
 }

 const auto shopPage=layoutShop(width,height,in);
 check(shopPage.page.w==width&&shopPage.page.h==height,"Shop is not full screen");
 check(std::abs(shopPage.close.w/shopPage.unit-64)<.01f&&std::abs(shopPage.close.h/shopPage.unit-64)<.01f,"Shop close button is not 64 by 64");
 check(shopPage.close.x+shopPage.close.w<=width-in.right&&shopPage.close.y>=in.top,"Shop close crosses safe area");
 check(shopPage.body.y>shopPage.close.y+shopPage.close.h,"Shop body overlaps header");
 for(int i=0;i<4;++i){const auto tab=shopPage.tabs[i];check(shopControl(shopPage,{tab.x+tab.w*.5f,tab.y+tab.h*.5f})==i,"Shop tab hit target mismatch");check(!overlaps(tab,shopPage.close),"Tab overlaps close button");}
 for(const auto card:shopPage.cards)check(card.w>0&&card.h>0&&card.y+card.h<=shopPage.footer.y,"Shop card crosses wallet bar");
 check(shopControl(shopPage,{shopPage.close.x+shopPage.close.w*.5f,shopPage.close.y+shopPage.close.h*.5f})==6,"Shop close target mismatch");
 for(const auto box:layout.boxes){
  check(std::isfinite(box.x)&&std::isfinite(box.y)&&std::isfinite(box.w)&&std::isfinite(box.h),"Invalid HUD geometry");
  check(box.w>0&&box.h>0,"Empty HUD element");
  check(box.x>=in.left-.5f&&box.y>=in.top-.5f&&box.x+box.w<=width-in.right+.5f&&box.y+box.h<=height-in.bottom+.5f,"HUD crosses the safe area");
 }
 const auto coins=layout[HudPart::Coins],pearls=layout[HudPart::Pearls],xp=layout[HudPart::Xp];
 for(const auto [button,bar]:std::array{std::pair{HudPart::CoinPlus,HudPart::Coins},std::pair{HudPart::PearlPlus,HudPart::Pearls}}){const auto p=layout[button],b=layout[bar];check(std::abs(p.w/layout.unit-44)<.01f&&std::abs(p.h/layout.unit-44)<.01f,"Currency plus size is incorrect");check(std::abs((b.x+b.w-p.x)/density-8)<.1f&&std::abs(p.y+p.h*.5f-b.y-b.h*.5f)<.5f,"Currency plus is not right aligned and centered");}

 check(std::abs(coins.w/layout.unit-hudTokens::coinWidth)<.1f,"Coin bar does not use its grid size");
 check(std::abs(xp.w/layout.unit-hudTokens::xpWidth)<.1f,"XP bar does not use its grid size");
 check(std::abs(pearls.w*2-coins.w)<.5f,"Pearl bar is not half the coin bar width");
 check(std::abs(coins.h-xp.h)<.5f&&std::abs(pearls.h-xp.h)<.5f,"Currency bar heights differ from XP");
 for(const auto [image,bar]:std::array{std::pair{HudPart::Level,HudPart::Xp},std::pair{HudPart::CoinIcon,HudPart::Coins},std::pair{HudPart::PearlIcon,HudPart::Pearls}}){
  const auto icon=layout[image],track=layout[bar];
  check(std::abs((icon.x+icon.w-track.x)/density-8)<.1f,"Bar image does not overlap by eight points");
  check(std::abs(icon.y+icon.h*.5f-track.y-track.h*.5f)<.5f,"Bar and image are not vertically centered");
 }
 check(std::abs(xp.y+xp.h*.5f-coins.y-coins.h*.5f)<.5f,"Wallet moves to a different header row");
 check(layout[HudPart::Profile].x+layout[HudPart::Profile].w<layout[HudPart::CoinIcon].x,"Profile overlaps the wallets");
 const auto shop=layout[HudPart::Shop];
 for(const auto part:{HudPart::Tank}){
  const auto box=layout[part];
  check(std::abs(box.y+box.h-shop.y-shop.h)<.5f,"Footer wraps into multiple rows");
 }
 for(const auto part:{HudPart::Xp,HudPart::Coins,HudPart::Pearls,HudPart::Level,HudPart::CoinIcon,HudPart::PearlIcon,HudPart::Shop,HudPart::Food,HudPart::Settings,HudPart::Projects,HudPart::Bag,HudPart::Tank}){
  const auto box=layout[part];
  for(float size:{box.w,box.h})check(std::abs(size/layout.unit/4-std::round(size/layout.unit/4))<.01f,"Authored component size is off the four-unit grid");
 }
 const auto controls=hudControls();
 for(std::size_t i=0;i<controls.size();++i){
  const auto box=layout[controls[i]];
  const float minimum=(controls[i]==HudPart::CoinPlus||controls[i]==HudPart::PearlPlus)?44:64;
  check(box.w>=minimum*layout.unit-.5f&&box.h>=minimum*layout.unit-.5f,"Control shrinks below the scaled layout size");
  check(hudHit(layout,{box.x+box.w*.5f,box.y+box.h*.5f})==controls[i],"Hit target differs from visible control");
  for(std::size_t j=i+1;j<controls.size();++j)check(!overlaps(box,layout[controls[j]]),"HUD controls overlap");
  check(!overlaps(box,layout[HudPart::Water]),"Control covers the central aquarium layout area");
 }
 check(!hudHit(layout,{-1,-1}),"A point outside the HUD activates a control");
 const auto food=layout[HudPart::Food];
 const auto rewards=layout[HudPart::Rewards];
 check(std::abs(rewards.w-food.w)<.5f&&std::abs(rewards.h-food.h)<.5f,"Rewards does not match Food size");
 check(rewards.x+rewards.w<shop.x&&std::abs(rewards.y+rewards.h-shop.y-shop.h)<.5f,"Rewards is not left of Shop and bottom aligned");
 check(std::abs(food.w-layout[HudPart::Settings].w)<.5f&&std::abs(food.h-layout[HudPart::Settings].h)<.5f,"Food does not match Settings size");
 const auto bag=layout[HudPart::Bag];
 check(std::abs(bag.w-shop.w*.75f)<.5f&&std::abs(bag.h-shop.h*.75f)<.5f,"Bag is not 75 percent of Shop");
 const auto projects=layout[HudPart::Projects];
 check(std::abs(projects.w-bag.w)<.5f&&std::abs(projects.h-bag.h)<.5f,"Projects does not match Bag size");
 check(food.y+food.h<projects.y&&projects.y+projects.h<bag.y&&bag.y+bag.h<shop.y,"Action column order is incorrect");
 check(std::abs(projects.x+projects.w-shop.x-shop.w)<.5f,"Projects is not right aligned");
 check(std::abs(bag.x+bag.w-shop.x-shop.w)<.5f,"Bag and Shop are not right aligned");
 check(std::abs(food.x+food.w-shop.x-shop.w)<.5f,"Food and Shop are not right aligned");
 const auto settings=layout[HudPart::Settings],currencyIcon=layout[HudPart::PearlIcon];
 check(settings.x>=pearls.x+pearls.w,"Settings is not to the right of the pearl bar");
 check(std::abs(settings.w-currencyIcon.w)<.5f&&std::abs(settings.h-currencyIcon.h)<.5f,"Settings does not match the currency image size");
 check(std::abs(settings.y-currencyIcon.y)<.5f,"Settings is not aligned with the currency images");
 std::cout<<"PASS "<<w<<"x"<<h<<" fixed composition"<<" safe area, spacing and hit targets\n";
 return layout;
}
}
int main(int argc,char** argv){try{
 ShopFishOffer badgeOffer;badgeOffer.quote.durationMs=1200000;
 check(badgeOffer.firstGrowthMs()==300000&&badgeOffer.fastGrowing(),"Five-minute milestone is missing its badge");
 badgeOffer.quote.durationMs=1199996;check(badgeOffer.fastGrowing(),"Under-five-minute milestone is missing its badge");
 badgeOffer.quote.durationMs=1200004;check(!badgeOffer.fastGrowing(),"Over-five-minute milestone incorrectly has a badge");
 badgeOffer.quote.durationMs=1200000;badgeOffer.companion=true;check(!badgeOffer.fastGrowing(),"Companion incorrectly has a growth badge");
 badgeOffer.companion=false;badgeOffer.quote.durationMs=0;check(!badgeOffer.fastGrowing(),"Zero-duration offer incorrectly has a growth badge");
 if(argc>1){
  std::ifstream input(std::filesystem::path(argv[1])/"content.json");
  Domain domain(Content::fromJson(Json::parse(input)),0);
  Session purchase(domain.content(),"/tmp/clay-placement-unused.json",0,true);
  FishPlacement placement;const auto placementLayout=layoutFishPlacement(1608,908,{});
  const auto initialCoins=purchase.domain().state().wallet.coins;
  const auto initialCount=purchase.domain().state().fish.size();
  check(bool(startFishPlacement(purchase.domain(),placement,"neonTetra")),"Cannot select egg for placement");
  check(purchase.domain().state().wallet.coins==initialCoins&&purchase.domain().state().fish.size()==initialCount,"Selecting a card purchases immediately");
  const auto cost=placement.offer.principal;
  SDL_Event placementInput{};placementInput.type=SDL_EVENT_MOUSE_BUTTON_UP;placementInput.button.button=SDL_BUTTON_LEFT;
  fishPlacementEvent(purchase,placement,placementLayout,placementInput,{600,350});
  check(placement.active()&&purchase.domain().state().wallet.coins==initialCoins,"Opening card release placed an egg");
  placementInput.type=SDL_EVENT_MOUSE_BUTTON_DOWN;fishPlacementEvent(purchase,placement,placementLayout,placementInput,{600,350});
  placementInput.type=SDL_EVENT_MOUSE_BUTTON_UP;
  check(fishPlacementEvent(purchase,placement,placementLayout,placementInput,{600,350})==PlacementEvent::Placed,"Aquarium tap did not place egg");
  check(placement.active()&&purchase.domain().state().wallet.coins==initialCoins-cost&&purchase.domain().state().fish.size()==initialCount+1,"Placement did not buy exactly one egg");
  check(purchase.domain().state().fish.back().egg&&purchase.domain().state().fish.back().species=="neonTetra","Wrong egg placed");
  check(placement.receipts.size()==1&&placement.receipts.back().cost==cost&&placement.receipts.back().xp==0,"Placement feedback differs from transaction");
  check(bool(confirmFishPlacement(purchase,placement,{650,350}))&&placement.active(),"Second egg requires selecting the fish again");
  check(purchase.domain().state().fish.size()==initialCount+2&&purchase.domain().state().wallet.coins==initialCoins-2*cost,"Repeated placement charges incorrectly");
  const SDL_FPoint done{placementLayout.cancel.x+20,placementLayout.cancel.y+20};
  placementInput.type=SDL_EVENT_MOUSE_BUTTON_DOWN;fishPlacementEvent(purchase,placement,placementLayout,placementInput,done);
  placementInput.type=SDL_EVENT_MOUSE_BUTTON_UP;fishPlacementEvent(purchase,placement,placementLayout,placementInput,done);
  check(!placement.active()&&placement.receipts.size()==2,"Done did not finish or discarded active feedback");
  advanceFishPlacement(placement,2);check(placement.receipts.empty(),"Placement feedback never expires");
  startFishPlacement(purchase.domain(),placement,"neonTetra");
  placementInput.type=SDL_EVENT_MOUSE_BUTTON_DOWN;fishPlacementEvent(purchase,placement,placementLayout,placementInput,{600,350});
  placementInput.type=SDL_EVENT_MOUSE_BUTTON_UP;fishPlacementEvent(purchase,placement,placementLayout,placementInput,{800,350});
  check(placement.active()&&purchase.domain().state().fish.size()==initialCount+2,"Dragging places an egg");
  placementInput.type=SDL_EVENT_KEY_DOWN;placementInput.key.key=SDLK_ESCAPE;
  check(fishPlacementEvent(purchase,placement,placementLayout,placementInput,{})==PlacementEvent::Cancelled&&!placement.active(),"Escape does not cancel placement");
  check(purchase.domain().state().wallet.coins==initialCoins-2*cost,"Cancelling charges currency");
  check(startFishPlacement(purchase.domain(),placement,"zebraDanio").error==Error::Level&&!placement.active(),"Locked fish can be placed");
  auto poor=purchase.domain().state();poor.wallet.coins=0;purchase.domain().install(poor);
  check(startFishPlacement(purchase.domain(),placement,"neonTetra").error==Error::Funds&&!placement.active(),"Unaffordable fish can be selected");
  poor.wallet.coins=1000;poor.tanks.front().slots=int(poor.fish.size());purchase.domain().install(poor);
  check(startFishPlacement(purchase.domain(),placement,"neonTetra").error==Error::Full,"Full tank permits placement");
  std::cout<<"PASS egg selection, placement purchase, cancellation, drag and purchase blockers\n";
  ShopState fishState{ShopCategory::Fish};
  auto verifyOffers=[&]{
   for(const auto& item:shopItems(domain,fishState)){
    check(item.fish.has_value(),"Fish card lacks offer data");
    const auto* species=domain.content().find(item.fish->id);check(species!=nullptr,"Fish card species missing");
    const auto quote=domain.quote(*species);
    check(item.fish->quote==quote,"Fish card does not use live purchase quote");
    check(item.fish->fastGrowing()==(!species->companion&&quote.durationMs>0&&quote.durationMs*quote.stages[1]/10000<=300000),"Catalog badge differs from first growth milestone");
    check(item.price==compact(species->currency==Currency::Coins?quote.principal:species->price)+(species->currency==Currency::Pearls?" Pearls":" Coins"),"Fish card price differs from purchase price");
    check(item.detail.empty()==!item.locked,"Unlocked fish shows an unlock requirement");
    if(species->companion)check(item.fish->companion&&quote.durationMs==0&&quote.profit==0&&quote.xp==0,"Companion shows growth rewards");
    else{Fish adult;adult.age=4;adult.purchase=quote;const auto reward=fishReward(adult);check(reward.coins()==quote.principal+quote.profit&&reward.xp==quote.xp,"Adult card reward differs from settlement");}
   }
  };
  verifyOffers();auto high=domain.state();high.xp=domain.content().levels[9];domain.install(high);verifyOffers();
  const auto beforeTreasure=encode(domain.state());
  const std::array<std::string,5> sizes{"Pocket","Pile","Bag","Box","Chest"};
  const std::array<std::string,5> coinPrices{"$0.99","$2.99","$4.99","$9.99","$19.99"},pearlPrices{"$1.99","$4.99","$9.99","$19.99","$29.99"};
  const std::array<std::string,5> coinAmounts{"3,161","10,748","18,968","41,096","88,515"},pearlAmounts{"20","55","120","260","420"};
  for(int tab=0;tab<2;++tab){
   ShopState treasure{ShopCategory::Treasure,tab};const auto offers=shopItems(domain,treasure);check(offers.size()==5,"Treasure does not have five offers per currency");
   for(int i=0;i<5;++i){const auto& offer=offers[i];const std::string currency=tab?" Pearls":" Coins";check(offer.name==sizes[i]+" of"+currency,"Wrong Treasure tier name");check(offer.detail==(tab?pearlAmounts[i]:coinAmounts[i])+currency&&offer.price==(tab?pearlPrices[i]:coinPrices[i]),"Treasure shows the wrong amount or USD price");check(!offer.treasureId.empty()&&!offer.fish&&!offer.locked,"Treasure card is not a currency offer");}
  }
  check(encode(domain.state())==beforeTreasure,"Browsing Treasure changes the wallet or progress");
  if(argc>2){
   const std::filesystem::path captures=argv[2];std::filesystem::create_directories(captures);
   for(const auto [width,height]:std::array{std::pair{1608,908},std::pair{667,375},std::pair{1024,768}}){
    Canvas canvas(argv[1],width,height,true);
    for(int tab=0;tab<2;++tab){
     canvas.begin();paintShop(canvas,domain,layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),{ShopCategory::Treasure,tab});
     const auto name=std::string(tab?"pearls-":"coins-")+std::to_string(width)+"x"+std::to_string(height)+".png";
     check(canvas.capture(captures/name),"Cannot capture Treasure page");
    }
   }
  }
  Domain beginner(domain.content());std::vector<std::string> treasureAssets;
  for(int tab=0;tab<2;++tab)for(const auto& offer:shopItems(beginner,{ShopCategory::Treasure,tab})){
   check(!offer.locked,"A Treasure pack is locked for a new player");
   check(!offer.asset.empty()&&std::filesystem::is_regular_file(std::filesystem::path(argv[1])/offer.asset),"Treasure artwork is missing");
   check(std::find(treasureAssets.begin(),treasureAssets.end(),offer.asset)==treasureAssets.end(),"Treasure tiers reuse the same artwork");
   treasureAssets.push_back(offer.asset);
  }
  std::cout<<"PASS ten unlocked Treasure offers, distinct artwork, exact amounts, prices and unchanged wallet\n";
  const auto page=layoutShop(1608,908,{});fishState.scroll=.5f;
  const auto card=page.cards[0];const float stride=page.cards[1].x-card.x;
  const auto scrolled=shopCardBounds(page,fishState,1),info=shopInfoBounds(scrolled,page.unit);
  check(shopCardAt(page,fishState,20,{info.x+info.w*.5f,info.y+info.h*.5f})==1,"Info target does not follow scrolled card");
  check(shopCardAt(page,fishState,20,{card.x+stride*.5f+card.w*.5f,card.y+20})==1,"Scrolled fish hit target is wrong");
  check(!shopCardAt(page,fishState,20,{page.body.x-1,card.y+20}),"Clipped fish is clickable outside row");
  check(!shopCardAt(page,fishState,20,{card.x+stride*.5f-4,card.y+20}),"Card gap opens details");
  std::cout<<"PASS live fish prices, adult rewards, companions and scrolled card targets\n";
 }
 DialogSpec spec{"Test"};const auto dialog=layoutDialog(1608,908,{},spec);DialogState modal;
 SDL_Event event{};event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;event.button.button=SDL_BUTTON_LEFT;
 check(dialogEvent(modal,dialog,event,{0,0})&&modal.open,"Backdrop input is not blocked");
 event.type=SDL_EVENT_MOUSE_BUTTON_UP;dialogEvent(modal,dialog,event,{dialog.close.x+4,dialog.close.y+4});check(modal.open,"Release without matching press closes dialog");
 event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;dialogEvent(modal,dialog,event,{dialog.close.x+4,dialog.close.y+4});
 event.type=SDL_EVENT_MOUSE_BUTTON_UP;dialogEvent(modal,dialog,event,{dialog.close.x+4,dialog.close.y+4});check(!modal.open,"Close button does not dismiss dialog");
 modal={};event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;dialogEvent(modal,dialog,event,{0,0});
 event.type=SDL_EVENT_MOUSE_BUTTON_UP;check(dialogEvent(modal,dialog,event,{0,0})&&!modal.open,"Backdrop click did not close and consume input");
 modal={};event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;dialogEvent(modal,dialog,event,{dialog.body.x+20,dialog.body.y+20});
 event.type=SDL_EVENT_MOUSE_BUTTON_UP;dialogEvent(modal,dialog,event,{dialog.body.x+20,dialog.body.y+20});check(modal.open,"Inside click dismissed dialog");
 event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;dialogEvent(modal,dialog,event,{dialog.body.x+20,dialog.body.y+20});
 event.type=SDL_EVENT_MOUSE_BUTTON_UP;dialogEvent(modal,dialog,event,{0,0});check(modal.open,"Drag from dialog to backdrop dismissed dialog");
 event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;dialogEvent(modal,dialog,event,{0,0});
 event.type=SDL_EVENT_WINDOW_FOCUS_LOST;dialogEvent(modal,dialog,event,{});
 event.type=SDL_EVENT_MOUSE_BUTTON_UP;dialogEvent(modal,dialog,event,{0,0});check(modal.open,"Stale backdrop press survived focus loss");
 modal.open=true;event.type=SDL_EVENT_KEY_DOWN;event.key.key=SDLK_ESCAPE;check(dialogEvent(modal,dialog,event)&&!modal.open,"Escape does not dismiss dialog");
 bool invalidCustom=false;try{layoutDialog(1608,908,{},DialogSpec{"Bad",DialogSize::Custom,0,0});}catch(const std::invalid_argument&){invalidCustom=true;}check(invalidCustom,"Invalid custom dimensions accepted");
 for(float density:{22.f,44.f,88.f}){
  const auto main=layoutHud(852,393,{},density);
  const auto shop=layoutShop(852,393,{},density);
  const auto origin=main[HudPart::CoinIcon],moved=shop.currencyHud[HudPart::CoinIcon];
  for(auto part:{HudPart::CoinIcon,HudPart::Coins,HudPart::CoinAmount,HudPart::CoinPlus,HudPart::PearlIcon,HudPart::Pearls,HudPart::PearlAmount,HudPart::PearlPlus}){
   const auto a=main[part],b=shop.currencyHud[part];
   check(std::abs(a.w-b.w)<.001f&&std::abs(a.h-b.h)<.001f,"Shop wallet sizes differ from HUD");
   check(std::abs((a.x-origin.x)-(b.x-moved.x))<.001f&&std::abs((a.y-origin.y)-(b.y-moved.y))<.001f,"Shop wallet spacing differs from HUD");
  }
  check(shopControl(shop,{shop.currencyHud[HudPart::CoinPlus].x+1,shop.currencyHud[HudPart::CoinPlus].y+1})==-1,"Hidden Shop plus remains clickable");
 }
 std::ifstream envContent(std::filesystem::path(argc>1?argv[1]:"assets")/"content.json");
 Domain environmentDomain(Content::fromJson(Json::parse(envContent)));
 ShopState environment{ShopCategory::Environment};
 auto backgrounds=shopItems(environmentDomain,environment);check(backgrounds.size()==2,"Background catalog missing");
 check(backgrounds.front().environmentId=="sunlit-lagoon"&&backgrounds.front().price=="Selected","Default background not selected");
 check(backgrounds.back().environmentId=="coral-garden"&&backgrounds.back().price=="1,200 Coins","Background price incorrect");
 activateShopControl(environment,5);auto owned=shopItems(environmentDomain,environment);
 check(owned.size()==1&&owned.front().environmentId=="sunlit-lagoon","Owned filter includes an unowned background");
 environmentDomain.fixture("performance");
 check(bool(environmentDomain.execute({.action=Action::PurchaseEnvironment,.tank={1},.key="coral-garden"})),"Background purchase failed");
 owned=shopItems(environmentDomain,environment);check(owned.size()==2&&owned.back().price=="Use background"&&owned.front().price=="Selected","Shop ownership does not refresh after purchase");
 const auto envLayout=layoutShop(1608,908,{},44);const auto envTab=envLayout.tabs[4];
 check(shopControl(envLayout,{envTab.x+envTab.w/2,envTab.y+envTab.h/2})==7,"Environment tab not clickable");
 for(std::size_t i=0;i<backgrounds.size();++i){
  const auto card=shopCardBounds(envLayout,environment,i);
  check(shopCardAt(envLayout,environment,backgrounds.size(),{card.x+card.w/2,card.y+card.h/2})==i,"Background preview does not match purchase hit area");
 }
 scrollShop(environment,100,2);check(environment.scroll==0,"Two backgrounds should fit without scrolling");
 scrollShop(environment,100,3);check(environment.scroll==1,"Background gallery scroll limit incorrect");
 activateShopControl(environment,7);check(environment.subtab==0&&environment.category==ShopCategory::Environment,"Environment tab failed");
 ShopState state;scrollShop(state,100,12);check(state.scroll==7,"Shop scroll exceeds last card");
 activateShopControl(state,0);check(state.category==ShopCategory::Fish&&state.scroll==0&&state.subtab==0,"Category switch does not reset scroll");
 activateShopControl(state,5);check(state.subtab==1,"Shop subtab does not switch");
 scrollShop(state,-100,12);check(state.scroll==0,"Shop scroll goes before first card");
 scrollShop(state,10,3);check(state.scroll==0,"Short product row scrolls");
 for(const auto [w,h]:std::array{std::pair{667.f,375.f},std::pair{852.f,393.f},std::pair{1088.f,635.f},std::pair{1024.f,768.f},std::pair{1920.f,1080.f}})viewport(w,h);
 viewport(852,393,{44,0,44,21});
 viewport(390,844,{0,47,0,34});
 viewport(320,240);viewport(360,240);viewport(580,400);
 const auto small=viewport(667,375),large=viewport(1210,834);
 check(small[HudPart::Water].w>320*.8f,"Too little space is reserved for the aquarium");
 check(large[HudPart::Water].h>small[HudPart::Water].h,"Aquarium space does not grow on a tablet");
 bool rejected=false;try{layoutHud(0,0,{},44);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid viewport is accepted");
 return 0;
}catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}}
