#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static Panel panel(const View& view){return view.panel_;}
 static Rect panelBounds(const View& view){auto r=view.panelRect_;r.x+=view.panelOrigin();r.y+=view.panelOriginY();return r;}
 static int page(const View& view){return view.page_;}
 static Rect pageArea(const View& view){return view.pageArea_;}
 static void paintHud(View& view){const auto count=view.buttons_.size();view.ui();view.buttons_.resize(count);}
 static const auto& shortfall(const View& view){return view.fundsDialog_;}
 static std::vector<std::string> controls(const View& view,std::string_view prefix){
  std::vector<std::string> ids;
  for(const auto& b:view.buttons_)if(b.id.starts_with(prefix))ids.push_back(b.id.substr(prefix.size()));
  return ids;
 }
 static Rect button(const View& view,std::string_view id){
  auto found=std::find_if(view.buttons_.begin(),view.buttons_.end(),[&](const auto& b){return b.id==id;});
  if(found==view.buttons_.end())throw std::runtime_error("Missing control: "+std::string(id));
  return found->area;
 }
};
}

namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}

// Exercise the public renderer path used by the aquarium, Shop and Collection.
// The derived variants must retain the base silhouette, including transparent fins.
void checkSpeciesArtwork(aq::Canvas& canvas,const aq::Content& content,const std::filesystem::path& assets){
 using namespace aq;
 auto* renderer=canvas.renderer();auto* originalTarget=SDL_GetRenderTarget(renderer);
 float originalScaleX{},originalScaleY{};SDL_GetRenderScale(renderer,&originalScaleX,&originalScaleY);
 std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> target(SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,192,128),SDL_DestroyTexture);
 require(bool(target),"Cannot create artwork test target");
 require(SDL_SetRenderTarget(renderer,target.get()),"Cannot select artwork test target");
 require(SDL_SetRenderScale(renderer,1,1),"Cannot set artwork test render scale");
 auto pixels=[&](const std::string& path){
  SDL_SetRenderDrawColor(renderer,0,0,0,0);require(SDL_RenderClear(renderer),"Cannot clear artwork test target");
  canvas.image(path,{0,0,192,128});
  std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> rendered(SDL_RenderReadPixels(renderer,nullptr),SDL_DestroySurface);
  require(bool(rendered),"Cannot read rendered fish artwork");
  std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> rgba(SDL_ConvertSurface(rendered.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
  require(bool(rgba),"Cannot read fish RGBA pixels");
  std::vector<Uint8> bytes(192*128*4);
  for(int y=0;y<128;++y)std::copy_n(static_cast<const Uint8*>(rgba->pixels)+y*rgba->pitch,192*4,bytes.data()+y*192*4);
  return bytes;
 };
 try{
  for(const auto& species:content.species){
   if(!species.artReady)continue;
   const auto base="species/"+species.id;
   require(fishArt(species.id)==base+".png","Fish still uses a substitute illustration");
   require(species.asset==base+".png","Species details still use placeholder artwork");
   require(std::filesystem::exists(assets/(base+".png")),"A species illustration is missing");
   const auto normal=pixels(base+".png"),mask=pixels(base+"-mask.png"),dead=pixels(base+"-dead.png");
   int visible=0,transparent=0;
   for(std::size_t i=0;i<normal.size();i+=4){
    require(normal[i+3]==mask[i+3]&&normal[i+3]==dead[i+3],"Derived artwork changes the fish silhouette");
    transparent+=normal[i+3]==0;
    if(normal[i+3]<240)continue;
    ++visible;
    require(mask[i]>220&&mask[i]==mask[i+1]&&mask[i+1]==mask[i+2],"Collection mask is not white");
    require(std::abs(int(dead[i])-int(dead[i+1]))<=1&&std::abs(int(dead[i+1])-int(dead[i+2]))<=1,"Dead fish variant is not grayscale");
   }
   require(visible>100&&transparent>100,"Fish artwork has no visible body or transparent background");
  }
 }catch(...){SDL_SetRenderTarget(renderer,originalTarget);SDL_SetRenderScale(renderer,originalScaleX,originalScaleY);throw;}
 require(SDL_SetRenderTarget(renderer,originalTarget),"Cannot restore aquarium render target");
 require(SDL_SetRenderScale(renderer,originalScaleX,originalScaleY),"Cannot restore aquarium render scale");
 std::cout<<"PASS all 46 species: canonical artwork, transparent silhouettes, Collection masks and dead variants\n";
}

void checkViewport(const std::filesystem::path& assets,int width,int height){
 using namespace aq;
 std::ifstream input(assets/"content.json");
 Session session(Content::fromJson(Json::parse(input)),"/tmp/aquarium-ui-test-unused.json",0,true);
 const auto fresh=session.domain().state();
 std::vector<std::string> starters;for(const auto& fish:fresh.fish){starters.push_back(fish.species);require(fish.age==0&&!fish.egg,"New game does not start with Baby fish");}
 require(starters==std::vector<std::string>{"neonTetra","neonTetra","guppy","platy"},"New-game starter roster differs from the workbook");
 require(fresh.wallet.coins==250&&fresh.wallet.pearls==0&&fresh.xp==0,"New-game wallet or XP is a preview value");
 Canvas canvas(assets,width,height,false);
 View view(canvas,session);
 double time=0;
 // These tests check settled controls. Animation timing has its own suite.
 auto render=[&]{view.render(time+=.7);canvas.present();};
 auto hudPixels=[&]{
  canvas.begin();ViewTestAccess::paintHud(view);
  std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> rendered(SDL_RenderReadPixels(canvas.renderer(),nullptr),SDL_DestroySurface);
  require(bool(rendered),"Cannot read the game HUD");
  std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> rgba(SDL_ConvertSurface(rendered.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
  require(bool(rgba),"Cannot convert the game HUD pixels");
  std::vector<Uint8> bytes(rgba->w*rgba->h*4);
  for(int y=0;y<rgba->h;++y)std::copy_n(static_cast<const Uint8*>(rgba->pixels)+y*rgba->pitch,rgba->w*4,bytes.data()+y*rgba->w*4);
  return bytes;
 };
 render();
 auto finger=[&](Uint32 type,float x,float y){
  int actualW{},actualH{};SDL_GetWindowSize(canvas.window(),&actualW,&actualH);
  const float windowW=float(actualW),windowH=float(actualH);
  const float fit=std::min(windowW/canvas.width(),windowH/canvas.height());
  SDL_Event event{};event.type=type;event.tfinger.touchID=1;event.tfinger.fingerID=1;
  event.tfinger.x=((windowW-canvas.width()*fit)*.5f+x*fit)/windowW;
  event.tfinger.y=((windowH-canvas.height()*fit)*.5f+y*fit)/windowH;
  view.event(event,time);
 };
 auto tap=[&](float x,float y){finger(SDL_EVENT_FINGER_DOWN,x,y);finger(SDL_EVENT_FINGER_UP,x,y);render();};
 std::string lastButton;
 auto button=[&](std::string_view id){lastButton=id;auto r=ViewTestAccess::button(view,id);tap(r.x+r.w*.5f,r.y+r.h*.5f);};
 auto panel=[&](Panel expected){if(ViewTestAccess::panel(view)!=expected)throw std::runtime_error("Wrong panel after "+lastButton+" at "+std::to_string(width)+"x"+std::to_string(height)+": expected "+std::to_string(int(expected))+", got "+std::to_string(int(ViewTestAccess::panel(view))));};
 auto swipe=[&](float dx,float dy=0,bool overBuy=false){
  const auto r=overBuy?ViewTestAccess::button(view,"buyguppy"):ViewTestAccess::pageArea(view);
  const float x=r.x+r.w*.5f,y=r.y+r.h*(overBuy?.5f:.3f);
  finger(SDL_EVENT_FINGER_DOWN,x,y);
  for(int step=1;step<=4;++step){
   time+=.04;finger(SDL_EVENT_FINGER_MOTION,x+dx*step/4,y+dy*step/4);render();
   if(width==667&&overBuy&&dx<0&&step==2){
    const auto path=std::filesystem::absolute(assets).parent_path()/"evidence"/"shop-reference"/"swipe-mid.png";
    std::filesystem::create_directories(path.parent_path());view.render(time);
    require(canvas.capture(path),"Cannot capture the Shop swipe");canvas.present();
   }
  }
  finger(SDL_EVENT_FINGER_UP,x+dx,y+dy);time+=.3;render();
 };
 auto world=[&](WorldPoint point){auto p=canvas.toScreen(point);tap(p.x,p.y);};
 const auto evidence=std::filesystem::absolute(assets).parent_path()/"evidence"/"workbook-catalog"/"667x375";
 auto capture=[&](const std::string& name){
  if(width!=667)return;
  std::filesystem::create_directories(evidence);
  // Capture the completed frame before presentation changes the SDL target.
  view.render(time+=.02);require(canvas.capture(evidence/(name+".png")),"Cannot save catalog visual evidence");canvas.present();
 };
 auto backdropPoint=[&](std::string_view id)->std::optional<SDL_FPoint>{
  const auto control=ViewTestAccess::button(view,id),modal=ViewTestAccess::panelBounds(view);
  for(float fx:{.1f,.5f,.9f})for(float fy:{.1f,.5f,.9f}){
   SDL_FPoint point{control.x+control.w*fx,control.y+control.h*fy};
   if(!modal.has(point.x,point.y))return point;
  }
  return std::nullopt;
 };
 auto within=[](Rect inner,Rect outer){
  return inner.w>0&&inner.h>0&&inner.x>=outer.x-1&&inner.y>=outer.y-1&&
   inner.x+inner.w<=outer.x+outer.w+1&&inner.y+inner.h<=outer.y+outer.h+1;
 };
 auto checkShopRow=[&](std::string_view prefix,const std::vector<std::string>& ids){
  const auto grid=ViewTestAccess::pageArea(view);
  require(within(grid,{0,0,canvas.width(),canvas.height()}),"Shop cards extend off screen");
  require(within(grid,ViewTestAccess::panelBounds(view)),"Shop cards extend beyond the frame");
  Rect previous{};
  for(const auto& id:ids){
   const auto buy=ViewTestAccess::button(view,std::string(prefix)+id);
   require(within(buy,grid),"Shop purchase button extends beyond its card row");
   if(previous.w){
    require(std::abs(buy.y-previous.y)<1&&std::abs(buy.h-previous.h)<1,"Shop cards no longer share one row");
    require(buy.x>=previous.x+previous.w,"Shop purchase buttons overlap");
   }
   previous=buy;
  }
 };
 auto browse=[&](std::string_view prefix,std::string_view next,std::string_view previous,std::string_view captureStem=""){
  std::set<std::string> found;int lastShopLevel=0;
  require(ViewTestAccess::page(view)==0,"Changing catalog filters did not reset pagination");
  if(next=="shop-next")swipe(canvas.minimumTouchSize()*2);else button(previous);
  require(ViewTestAccess::page(view)==0,"Previous escaped the first catalog page");
  if(!captureStem.empty()){time+=3;render();}
  constexpr std::size_t shopPageSize=4;
  for(int guard=0;guard<20;++guard){
   const auto visible=ViewTestAccess::controls(view,prefix);require(!visible.empty(),"Catalog has an empty page");
   if(next=="shop-next"){
    require(visible.size()<=shopPageSize,"Shop page exceeds four cards");
    checkShopRow(prefix,visible);
   }
   for(const auto& id:visible){
    require(found.insert(id).second,"Catalog repeats a species on different pages");
    if(next=="shop-next"){
     const int level=prefix=="buy"?session.domain().content().find(id)->level:session.domain().content().findDecor(id)->level;
     require(level>=lastShopLevel&&level<=40,"Shop items are out of unlock-level order or exceed level 40");lastShopLevel=level;
    }
   }
   if(!captureStem.empty())capture(std::string(captureStem)+"-page-"+std::to_string(ViewTestAccess::page(view)+1));
   const int pageBefore=ViewTestAccess::page(view);
   if(next=="shop-next")swipe(-canvas.minimumTouchSize()*2);else button(next);
   if(ViewTestAccess::page(view)==pageBefore)return found;
   if(next=="shop-next")require(visible.size()==shopPageSize,"Shop has a partial page before the final page");
  }
  throw std::runtime_error("Catalog pagination did not stop at its final page");
 };
 auto unchangedBuy=[&](const std::string& id,Error expected){
  const auto* species=session.domain().content().find(id);require(species&&session.domain().blocker(*species).error==expected,"Wrong purchase gate in UI test");
  const auto expectedShortfall=session.domain().blocker(*species).shortfall;
  const auto before=encode(session.domain().state());button("buy"+id);panel(Panel::Shop);
  require(view.tool()==Tool::Select,"Blocked purchase armed placement");
  require(encode(session.domain().state())==before,"Blocked Shop purchase changed game state");
  if(expected==Error::Funds){
   const auto& missing=ViewTestAccess::shortfall(view);
   require(missing&&missing->coins==expectedShortfall.coins&&missing->pearls==expectedShortfall.pearls,"Blocked purchase shows the wrong currency shortfall");
   button("funds-close");panel(Panel::Shop);
   require(!ViewTestAccess::shortfall(view)&&encode(session.domain().state())==before,"Closing the funds dialog changes the purchase state");
  }else require(!ViewTestAccess::shortfall(view),"Locked purchase opened the currency menu");
 };

 panel(Panel::None);
 for(const char* id:{"coin-more","pearl-more","notifications","menu","nav0","nav1","play","inventory","tool-food","nav3","tool-sell"}){
  auto r=ViewTestAccess::button(view,id);
  require(r.x>=0&&r.y>=0&&r.x+r.w<=canvas.width()+1&&r.y+r.h<=canvas.height()+1,"Control extends beyond canvas");
 }
 if(height>600){
  const auto tanks=ViewTestAccess::button(view,"nav0"),shop=ViewTestAccess::button(view,"nav1");
  require(canvas.height()>1000,"Tablet kept the phone's short canvas");
  require(tanks.y+tanks.h>canvas.height()-90&&shop.y+shop.h>canvas.height()-90,"Tablet corner buttons are not at the bottom");
  float gap=-1;Rect prior{};
  for(const char* id:{"nav3","play","tool-food"}){auto r=ViewTestAccess::button(view,id);if(prior.w){const float nextGap=r.y-prior.y-prior.h;if(gap<0)gap=nextGap;require(nextGap>=0,"Tablet navigation buttons overlap");}prior=r;}
 }
 button("nav0");panel(Panel::Tanks);
 const auto tankMenu=ViewTestAccess::panelBounds(view);
 require(tankMenu.w>tankMenu.h,"Tanks menu lost its two-column landscape layout");
 require(tankMenu.x>=0&&tankMenu.y>=0&&tankMenu.x+tankMenu.w<=canvas.width()&&tankMenu.y+tankMenu.h<=canvas.height(),"Tanks popup extends off screen");
 // Capacity actions live in the detail pane. A row selects a preview without
 // switching the active aquarium or swallowing its purchase controls.
 button("tank-current");panel(Panel::Tanks);
 const auto tankBuy=ViewTestAccess::button(view,"tank-buy");
 const float footerPad=std::max(0.f,(canvas.minimumTouchSize()-tankBuy.h)*.5f);
 require(tankBuy.x>=tankMenu.x&&tankBuy.x+tankBuy.w<=tankMenu.x+tankMenu.w&&tankBuy.y-footerPad>=0&&tankBuy.y+tankBuy.h+footerPad<=canvas.height(),"Tanks upgrade or its touch target extends off screen");
 auto tankState=fresh;tankState.xp=session.domain().content().levels[2];tankState.highestRewardedLevel=3;tankState.wallet={299,3};session.domain().install(tankState);render();
 const auto tankBefore=encode(session.domain().state());button("tank-buy");require(encode(session.domain().state())==tankBefore&&ViewTestAccess::shortfall(view)&&ViewTestAccess::shortfall(view)->coins==1,"Tank shortfall did not preserve the state");button("funds-close");
 tankState.wallet.coins=300;session.domain().install(tankState);render();button("tank-buy");require(session.domain().tank({1})->slots==15&&session.domain().state().wallet.coins==0&&session.domain().state().xp==tankState.xp,"Tank upgrade does not use v4 capacity and price");
 button("panel-close");session.domain().install(fresh);render();
 button("notifications");panel(Panel::Quests);button("panel-close");
 button("menu");panel(Panel::Settings);button("panel-close");
 // Tap the minimum touch target outside the visible plus button. Both down
 // and up must use the expanded target, including at a compact phone size.
 auto plus=ViewTestAccess::button(view,"coin-more");
 const float extension=(canvas.minimumTouchSize()-plus.h)*.5f;
 tap(plus.x+plus.w*.5f,extension>0?plus.y-extension*.7f:plus.y+plus.h*.5f);panel(Panel::CurrencyShop);
 require(!ViewTestAccess::shortfall(view),"Coin plus opened a funds dialog instead of Currency Shop");button("panel-close");

 const auto aquariumHud=hudPixels();
 const std::array<const char*,15> hudIds{"coin-balance","coin-more","pearl-balance","pearl-more","notifications","menu","nav0","tank-prev","tank-next","nav1","play","inventory","tool-food","nav3","tool-sell"};
 std::vector<Rect> aquariumControls;for(const auto id:hudIds)aquariumControls.push_back(ViewTestAccess::button(view,id));
 button("nav1");panel(Panel::Shop);
 require(hudPixels()==aquariumHud,"Opening Shop changes the XP bar or other game HUD artwork");
 const auto shopMenu=ViewTestAccess::panelBounds(view);
 require(within(shopMenu,{0,0,canvas.width(),canvas.height()}),"Shop frame extends off screen");
 require(std::abs(shopMenu.y-tankMenu.y)<1&&std::abs(shopMenu.h-tankMenu.h)<1&&std::abs(shopMenu.w-tankMenu.w)<tankMenu.w*.01f,"Shop frame does not match the tank dialog size");
 Rect priorTab{};
 const auto shopGrid=ViewTestAccess::pageArea(view);
 for(int i=0;i<5;++i){
  const auto tab=ViewTestAccess::button(view,"shop-tab"+std::to_string(i));
  require(within(tab,shopMenu)&&tab.x+tab.w<shopGrid.x,"Shop category does not fit in the left sidebar");
  if(i)require(tab.y>priorTab.y+priorTab.h,"Shop sidebar categories overlap");
  priorTab=tab;
 }
 for(std::size_t i=0;i<hudIds.size();++i){
  const auto control=ViewTestAccess::button(view,hudIds[i]),before=aquariumControls[i];
  require(control.x==before.x&&control.y==before.y&&control.w==before.w&&control.h==before.h,"Opening Shop moves a game control");
  require(control.x+control.w<=shopMenu.x||control.x>=shopMenu.x+shopMenu.w||control.y+control.h<=shopMenu.y||control.y>=shopMenu.y+shopMenu.h,"Shop covers a game control");
 }
 // Shop is modal: a backdrop tap dismisses it and must not reach the menu
 // or tool behind it. A later, separate tap can activate that control.
 tap(shopMenu.x+5,shopMenu.y+150);panel(Panel::Shop);
 int exposedBackdropChecks=0;
 for(const auto id:hudIds){
  const auto point=backdropPoint(id);require(point.has_value(),"Shop hides a game control from the backdrop");
  ++exposedBackdropChecks;const auto before=encode(session.domain().state());
  lastButton="Shop backdrop over "+std::string(id);
  tap(point->x,point->y);panel(Panel::None);require(view.tool()==Tool::Select,"Shop backdrop activated an underlying tool");
  require(encode(session.domain().state())==before,"Shop backdrop changed game state");
  button("nav1");panel(Panel::Shop);
 }
 require(exposedBackdropChecks==int(hudIds.size()),"A Shop backdrop control was not tested");
 const auto shopClose=ViewTestAccess::button(view,"panel-close");
 require(within(shopClose,{0,0,canvas.width(),canvas.height()}),"Shop close button extends off screen");
 const SDL_FPoint closeCenter{shopClose.x+shopClose.w*.5f,shopClose.y+shopClose.h*.5f};
 require(shopMenu.has(closeCenter.x,closeCenter.y),"Shop close center misses the top-right frame edge");
 lastButton="Shop close";
 finger(SDL_EVENT_FINGER_DOWN,closeCenter.x,closeCenter.y);panel(Panel::Shop);
 finger(SDL_EVENT_FINGER_UP,closeCenter.x,closeCenter.y);render();panel(Panel::None);
 button("tool-food");require(view.tool()==Tool::Food,"Food did not activate after dismissing Shop");button("done");
 button("nav3");panel(Panel::Collection);button("panel-close");
 button("notifications");panel(Panel::Quests);button("panel-close");
 button("nav0");panel(Panel::Tanks);button("panel-close");

 button("nav1");
 auto firstPage=ViewTestAccess::controls(view,"buy");
 constexpr std::size_t shopPageSize=4;
 require(firstPage.size()==shopPageSize&&firstPage==std::vector<std::string>{"neonTetra","guppy","platy","molly"},"Shop item count or starter order is incorrect");
 checkShopRow("buy",firstPage);
 const auto beforeSwipe=encode(session.domain().state());
 swipe(-canvas.minimumTouchSize()*2,0,true);panel(Panel::Shop);
 require(ViewTestAccess::page(view)==1&&view.tool()==Tool::Select,"Swiping from Buy did not advance the Shop safely");
 require(encode(session.domain().state())==beforeSwipe,"Swiping over Buy purchased a fish");
 swipe(canvas.minimumTouchSize()*2);require(ViewTestAccess::page(view)==0,"Right swipe did not return to the first Shop page");
 swipe(canvas.minimumTouchSize()*2);require(ViewTestAccess::page(view)==0,"Right swipe escaped the first Shop page");
 swipe(0,canvas.minimumTouchSize()*2,true);require(ViewTestAccess::page(view)==0&&view.tool()==Tool::Select,"Vertical drag turned a page or armed a purchase");
 require(encode(session.domain().state())==beforeSwipe,"Vertical drag over Buy changed the game");
 swipe(canvas.minimumTouchSize()*.15f);require(ViewTestAccess::page(view)==0,"A small touch movement changed the Shop page");
 swipe(-canvas.minimumTouchSize()*2);
 unchangedBuy("zebraDanio",Error::Level);
 swipe(canvas.minimumTouchSize()*2);
 auto state=fresh;state.wallet.coins=0;session.domain().install(state);render();unchangedBuy("guppy",Error::Funds);
 state=fresh;while(state.fish.size()<10){auto fish=state.fish.front();fish.id={state.nextFishId++};state.fish.push_back(fish);}session.domain().install(state);render();
 unchangedBuy("guppy",Error::Full);
 session.domain().install(fresh);render();
 std::set<std::string> allSpecies;for(const auto& species:session.domain().content().species)if(species.releaseGate=="Launch"&&species.artReady&&species.level<=40)allSpecies.insert(species.id);
 require(session.domain().content().species.size()==99,"Source catalog no longer contains 99 species");
 std::set<std::string> shopSpecies;for(const auto& species:session.domain().content().species)if(species.releaseGate=="Launch"&&species.artReady&&species.level<=40)shopSpecies.insert(species.id);
 require(!shopSpecies.empty(),"Shop has no ready launch fish");
 require(browse("buy","shop-next","shop-previous","shop-all")==shopSpecies,"Shop is missing a regular or active event fish, or shows a closed event fish");
 require(ViewTestAccess::page(view)==int((shopSpecies.size()-1)/4),"Shop pagination does not match the active catalog");
 require(ViewTestAccess::controls(view,"shop-filter").empty(),"Shop still exposes fish subtabs");
 button("panel-close");

 // Collection remains browseable while its lifetime reward system is deferred.
 button("nav3");panel(Panel::Collection);const auto collectionBefore=encode(session.domain().state());
 for(int page=0;page<12;++page)button("collection-next");
 require(ViewTestAccess::page(view)==int((allSpecies.size()-1)/8),"Collection pagination differs from its release gates");
 require(ViewTestAccess::controls(view,"mastery-claim").empty()&&encode(session.domain().state())==collectionBefore,"Collection exposed deferred rewards");button("panel-close");

 session.domain().install(fresh);session.domain().fixture("aquarium");render();
 button("nav1");button("panel-close");
 button("nav1");button("shop-tab4");button("shop-care");panel(Panel::None);require(view.tool()==Tool::Food,"Shop Food tab did not activate feeding");button("done");
 button("nav1");button("shop-tab1");
 {std::set<std::string> expected;for(const auto& item:session.domain().decorations())if(item.category=="Plant"&&item.level<=40&&item.edition!="Limited Edition")expected.insert(item.id);require(browse("decor-buy","shop-next","shop-previous")==expected,"Plants tab is missing a regular item or still shows an event item");}
 button("shop-tab2");
 {std::set<std::string> expected;for(const auto& item:session.domain().decorations())if(item.category=="Decoration"&&item.level<=40&&item.edition!="Limited Edition")expected.insert(item.id);require(browse("decor-buy","shop-next","shop-previous")==expected,"Decorations tab is missing a regular item or still shows an event item");}
 button("shop-tab3");
 const auto environmentWallet=session.domain().state().wallet;
 for(int look:{1,2,0}){button("shop-look"+std::to_string(look));require(session.domain().state().settings.tankLook==look,"Environment tab did not apply the selected look");}
 require(session.domain().state().wallet.coins==environmentWallet.coins&&session.domain().state().wallet.pearls==environmentWallet.pearls,"Free environment looks charged currency");
 button("shop-tab0");button("buyguppy");panel(Panel::None);
 require(view.tool()==Tool::Buy,"Buy did not arm placement");
 require(session.domain().state().fish.size()==6,"Arming placement created an egg");
 require(session.domain().state().wallet.coins==250,"Arming placement charged the wallet");
 world({590,330});
 require(session.domain().state().fish.size()==7,"Egg placement did not buy one fish");
 require(session.domain().state().wallet.coins==240,"Purchase did not update wallet");
 require(view.tool()==Tool::Buy,"Placement did not stay active after buying one egg");
 button("done");

 button("nav0");button("tool-food");panel(Panel::None);require(view.tool()==Tool::Food,"Food tool did not activate");
 world({410,210});require(session.domain().pellets().size()==1,"Touch did not drop one food pellet");
 button("done");

 require(ViewTestAccess::controls(view,"tool-select").empty()&&ViewTestAccess::controls(view,"select-option").empty(),"Old Select menu is still reachable");
 auto before=session.domain().state().fish[2];auto from=canvas.toScreen(before.position);auto to=canvas.toScreen({420,340});
 finger(SDL_EVENT_FINGER_DOWN,from.x,from.y);finger(SDL_EVENT_FINGER_MOTION,to.x,to.y);finger(SDL_EVENT_FINGER_UP,to.x,to.y);render();
 require(std::abs(session.domain().fish(before.id)->position.x-420)<.1,"Touch drag did not move fish");
 require(ViewTestAccess::controls(view,"selection-stash").empty(),"Fish offers a Stash action");
 button("inventory");panel(Panel::Inventory);
 require(ViewTestAccess::controls(view,"inventory-fish").empty(),"Bag offers storage for fish");button("panel-close");

 button("tool-sell");const auto count=session.domain().state().fish.size();const auto coins=session.domain().state().wallet.coins;
 const auto priorPearls=session.domain().state().wallet.pearls;const int priorLevel=session.domain().level();
 world({420,340});panel(Panel::Details);button("fish-rehome");button("rehome-confirm");
 require(session.domain().state().fish.size()==count-1,"Sell did not remove selected fish");
 Amount bonus=0;for(int reached=priorLevel+1;reached<=session.domain().level();++reached)bonus+=levelReward(session.domain().content(),reached).coins;
 require(session.domain().state().wallet.coins==coins+fishReward(before).coins()+bonus,"Sell did not credit its stage value and crossed-level rewards");
 require(session.domain().state().wallet.pearls==priorPearls+session.domain().level()-priorLevel,"Sell did not grant one pearl per crossed level");
 const auto afterSale=encode(session.domain().state());
 for(int reached=priorLevel+1;reached<=session.domain().level();++reached){render();button("level-continue");}
 require(encode(session.domain().state())==afterSale,"Dismissing a sale's level-up screen changes its rewards");
 if(width==804)checkSpeciesArtwork(canvas,session.domain().content(),assets);
 std::cout<<"PASS "<<width<<'x'<<height<<": source starters, release-gated Shop and Collection, purchase gates, modal backdrop, navigation and fish controls\n";
}
}
int main(int argc,char** argv){
 try{
  if(argc!=2)throw std::runtime_error("Usage: aquarium_view_tests ASSETS");
  for(auto [w,h]:{std::pair{804,415},std::pair{852,393},std::pair{667,375},std::pair{1024,768},std::pair{1210,834}})checkViewport(argv[1],w,h);
  return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
