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
 static std::string toast(const View& view){return view.toasts_.empty()?"":view.toasts_.back().text;}
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
 require(starters==std::vector<std::string>{"neonTetra","guppy","platy","molly"},"New-game starter roster differs from the workbook");
 require(fresh.wallet.coins==250&&fresh.wallet.pearls==0&&fresh.xp==0,"New-game wallet or XP is a preview value");
 Canvas canvas(assets,width,height,false);
 View view(canvas,session);
 double time=0;
 // These tests check settled controls. Animation timing has its own suite.
 auto render=[&]{view.render(time+=.7);canvas.present();};
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
 auto world=[&](WorldPoint point){auto p=canvas.toScreen(point);tap(p.x,p.y);};
 const auto evidence=std::filesystem::absolute(assets).parent_path()/"evidence"/"workbook-catalog"/"667x375";
 auto capture=[&](const std::string& name){
  if(width!=667)return;
  std::filesystem::create_directories(evidence);
  // Capture the completed frame before presentation changes the SDL target.
  view.render(time+=.02);require(canvas.capture(evidence/(name+".png")),"Cannot save catalog visual evidence");canvas.present();
 };
 auto backdropPoint=[&](std::string_view id){
  const auto control=ViewTestAccess::button(view,id),modal=ViewTestAccess::panelBounds(view);
  for(float fx:{.1f,.5f,.9f})for(float fy:{.1f,.5f,.9f}){
   SDL_FPoint point{control.x+control.w*fx,control.y+control.h*fy};
   if(!modal.has(point.x,point.y))return point;
  }
  throw std::runtime_error("Control has no exposed backdrop area: "+std::string(id));
 };
 auto browse=[&](std::string_view prefix,std::string_view next,std::string_view previous,std::string_view captureStem=""){
  std::set<std::string> found;
  require(ViewTestAccess::page(view)==0,"Changing catalog filters did not reset pagination");
  button(previous);require(ViewTestAccess::page(view)==0,"Previous escaped the first catalog page");
  if(!captureStem.empty()){time+=3;render();}
  for(int guard=0;guard<10;++guard){
   const auto visible=ViewTestAccess::controls(view,prefix);require(!visible.empty(),"Catalog has an empty page");
   for(const auto& id:visible)require(found.insert(id).second,"Catalog repeats a species on different pages");
   if(!captureStem.empty())capture(std::string(captureStem)+"-page-"+std::to_string(ViewTestAccess::page(view)+1));
   const int pageBefore=ViewTestAccess::page(view);button(next);
   if(ViewTestAccess::page(view)==pageBefore)return found;
  }
  throw std::runtime_error("Catalog pagination did not stop at its final page");
 };
 auto unchangedBuy=[&](const std::string& id,Error expected){
  const auto* species=session.domain().content().find(id);require(species&&session.domain().blocker(*species).error==expected,"Wrong purchase gate in UI test");
  const auto before=encode(session.domain().state());button("buy"+id);panel(Panel::Shop);
  require(view.tool()==Tool::Select,"Blocked purchase armed placement");
  require(encode(session.domain().state())==before,"Blocked Shop purchase changed game state");
  require(!ViewTestAccess::toast(view).empty(),"Blocked purchase gave no explanation");
 };

 panel(Panel::None);
 for(const char* id:{"coin-more","pearl-more","notifications","menu","nav0","nav1","tool-select","tool-food","nav3","tool-sell"}){
  auto r=ViewTestAccess::button(view,id);
  require(r.x>=0&&r.y>=0&&r.x+r.w<=canvas.width()+1&&r.y+r.h<=canvas.height()+1,"Control extends beyond canvas");
 }
 if(height>600){
  const auto tanks=ViewTestAccess::button(view,"nav0"),shop=ViewTestAccess::button(view,"nav1");
  require(canvas.height()>1000,"Tablet kept the phone's short canvas");
  require(tanks.y+tanks.h>canvas.height()-90&&shop.y+shop.h>canvas.height()-90,"Tablet corner buttons are not at the bottom");
  float gap=-1;Rect prior{};
  for(const char* id:{"tool-select","tool-food","nav3","tool-sell"}){auto r=ViewTestAccess::button(view,id);if(prior.w){const float nextGap=r.y-prior.y-prior.h;if(gap<0)gap=nextGap;require(std::abs(nextGap-gap)<1,"Tablet tool spacing is uneven");}prior=r;}
 }
 button("nav0");panel(Panel::Tanks);
 const auto tankButton=ViewTestAccess::button(view,"nav0"),tankMenu=ViewTestAccess::panelBounds(view);
 require(tankMenu.x>tankButton.x+tankButton.w,"Tanks menu overlaps its button");
 require(tankMenu.x>=0&&tankMenu.y>=0&&tankMenu.x+tankMenu.w<=canvas.width()&&tankMenu.y+tankMenu.h<=canvas.height(),"Tanks panel footer extends off screen");
 // Expand is inside the active tank card. Its parent row must not swallow
 // the tap and close Tanks before the player can confirm the upgrade.
 button("tank-current");panel(Panel::Tanks);
 const auto tankBuy=ViewTestAccess::button(view,"tank-buy");
 const float footerPad=std::max(0.f,(canvas.minimumTouchSize()-tankBuy.h)*.5f);
 require(tankBuy.x>=tankMenu.x&&tankBuy.x+tankBuy.w<=tankMenu.x+tankMenu.w&&tankBuy.y-footerPad>=0&&tankBuy.y+tankBuy.h+footerPad<=canvas.height(),"Tanks purchase footer or its touch target extends off screen");
 auto tankState=fresh;tankState.wallet.coins=249;tankState.giftTokens=5;session.domain().install(tankState);render();
 auto tankBefore=encode(session.domain().state());button("tank-buy");panel(Panel::Tanks);
 require(encode(session.domain().state())==tankBefore,"Tank expansion with insufficient coins changed game state");
 tankState=fresh;tankState.wallet.coins=250;tankState.giftTokens=4;session.domain().install(tankState);render();
 tankBefore=encode(session.domain().state());button("tank-buy");panel(Panel::Tanks);
 require(encode(session.domain().state())==tankBefore,"Tank expansion with insufficient Gift Tokens changed game state");
 tankState=fresh;tankState.wallet.coins=1000;tankState.giftTokens=20;session.domain().install(tankState);time+=3;render();capture("tanks-expand");
 button("tank-buy");panel(Panel::Tanks);
 require(session.domain().tank({1})->slots==20,"Tanks Expand control did not increase the starter tank to 20 slots");
 require(session.domain().state().wallet.coins==750&&session.domain().state().giftTokens==15,"Tank expansion did not charge exactly 250 coins and 5 Gift Tokens");
 require(session.domain().state().wallet.pearls==fresh.wallet.pearls&&session.domain().state().xp==fresh.xp,"Tank expansion changed unrelated currency or XP");
 session.domain().install(fresh);render();
 // The relocated panel must keep its empty interior touch area. Right-side
 // controls remain reachable and can switch away from Tanks directly.
 tap(tankMenu.x+5,tankMenu.y+300);panel(Panel::Tanks);
 for(const char* id:{"tool-select","tool-food","nav3","tool-sell"})ViewTestAccess::button(view,id);
 button("nav3");panel(Panel::Collection);button("panel-close");
 button("nav0");button("tool-select");panel(Panel::None);
 require(view.tool()==Tool::Select,"Select did not dismiss Tanks");
 button("nav0");button("tool-sell");panel(Panel::None);
 require(view.tool()==Tool::Sell,"Sell is unreachable from Tanks");button("done");
 button("nav0");button("tank-select1");panel(Panel::None);
 button("notifications");panel(Panel::Quests);button("panel-close");
 button("menu");panel(Panel::Settings);button("panel-close");
 // Tap the minimum touch target outside the visible plus button. Both down
 // and up must use the expanded target, including at a compact phone size.
 auto plus=ViewTestAccess::button(view,"coin-more");
 const float extension=(canvas.minimumTouchSize()-plus.h)*.5f;
 tap(plus.x+plus.w*.5f,extension>0?plus.y-extension*.7f:plus.y+plus.h*.5f);panel(Panel::Gifts);button("panel-close");

 button("nav1");panel(Panel::Shop);
 const auto shopMenu=ViewTestAccess::panelBounds(view);
 require(shopMenu.w>=1394&&shopMenu.h>=canvas.height()*.82f,"Shop no longer uses its full-size modal layout");
 for(const char* id:{"nav0","nav1","tool-select","tool-food","nav3","tool-sell","notifications","menu"})ViewTestAccess::button(view,id);
 // Shop is modal: a backdrop tap dismisses it and must not reach the menu
 // or tool behind it. A later, separate tap can activate that control.
 tap(shopMenu.x+5,shopMenu.y+150);panel(Panel::Shop);
 // Probe the empty right border where Food is covered by Shop. Its vertical
 // position can align with a Buy button on tablets, so avoid the card area.
 const auto foodBehind=ViewTestAccess::button(view,"tool-food");
 const SDL_FPoint coveredFood{std::min(foodBehind.x+foodBehind.w,shopMenu.x+shopMenu.w)-5,foodBehind.y+foodBehind.h*.5f};
 if(shopMenu.has(coveredFood.x,coveredFood.y)&&foodBehind.has(coveredFood.x,coveredFood.y)){
  tap(coveredFood.x,coveredFood.y);panel(Panel::Shop);require(view.tool()==Tool::Select,"Shop interior activated Food through the modal");
 }
 for(const char* id:{"tool-select","tool-food","tool-sell","nav3","notifications","menu","nav0","nav1"}){
  const auto point=backdropPoint(id);const auto before=encode(session.domain().state());
  tap(point.x,point.y);panel(Panel::None);require(view.tool()==Tool::Select,"Shop backdrop activated an underlying tool");
  require(encode(session.domain().state())==before,"Shop backdrop changed game state");
  button("nav1");panel(Panel::Shop);
 }
 button("panel-close");panel(Panel::None);
 button("tool-food");require(view.tool()==Tool::Food,"Food did not activate after dismissing Shop");button("done");
 button("nav3");panel(Panel::Collection);button("panel-close");
 button("notifications");panel(Panel::Quests);button("panel-close");
 button("nav0");panel(Panel::Tanks);button("panel-close");

 button("nav1");button("shop-filter0");
 auto firstPage=ViewTestAccess::controls(view,"buy");
 require(firstPage.size()==(canvas.height()>=1000&&ViewTestAccess::panelBounds(view).h>=865?8u:6u)&&std::equal(starters.begin(),starters.end(),firstPage.begin()),"Shop does not begin with the real starter roster");
 unchangedBuy("zebraDanio",Error::Level);
 require(ViewTestAccess::toast(view)=="Unlocks at Level 2","Locked fish does not explain its unlock level");
 auto state=fresh;state.wallet.coins=0;session.domain().install(state);render();unchangedBuy("guppy",Error::Funds);
 state=fresh;while(state.fish.size()<10){auto fish=state.fish.front();fish.id={state.nextFishId++};state.fish.push_back(fish);}session.domain().install(state);render();unchangedBuy("guppy",Error::Full);
 session.domain().install(fresh);render();
 std::set<std::string> allSpecies;for(const auto& species:session.domain().content().species)allSpecies.insert(species.id);
 require(allSpecies.size()==46,"Source catalog no longer contains 46 species");
 require(browse("buy","shop-next","shop-previous","shop-all")==allSpecies,"Shop cannot reach every source species");
 for(int filter=1;filter<=3;++filter){
  button("shop-filter"+std::to_string(filter));std::set<std::string> expected;
  for(const auto& species:session.domain().content().species){int group=species.modelId.starts_with("LE-")?3:species.modelId.starts_with("PF-")?2:1;if(group==filter)expected.insert(species.id);}
  require(expected.size()==(filter==1?26u:10u),"Unexpected source fish group size");
  require(browse("buy","shop-next","shop-previous")==expected,"Shop filter omits or misclassifies a species");
 }
 button("offers");panel(Panel::Shop);require(ViewTestAccess::page(view)==0,"Limited offers did not reset pagination");
 unchangedBuy("heartfinTetra",Error::EventClosed);
 button("panel-close");

 // Claim controls provide a stable public path to verify all Collection pages.
 // Undiscovered masks are also rendered when the panel first opens below.
 button("nav3");panel(Panel::Collection);
 time+=3;render();capture("collection-undiscovered-page-1");
 for(int pageIndex=0;pageIndex<5;++pageIndex){button("collection-next");capture("collection-undiscovered-page-"+std::to_string(pageIndex+2));}
 require(ViewTestAccess::page(view)==5,"Collection cannot reach its sixth page");
 button("filter0");state=fresh;state.collected.assign(allSpecies.begin(),allSpecies.end());
 for(const auto& id:allSpecies)state.adultRaised[id]=5;
 session.domain().install(state);render();
 require(browse("mastery-claim","collection-next","collection-prev","collection-all")==allSpecies,"Collection cannot reach every species mastery card");
 const std::array<const char*,7> rarities{"","common","uncommon","rare","epic","premium","limited"};
 for(int filter=1;filter<=6;++filter){
  button("filter"+std::to_string(filter));std::set<std::string> expected;
  for(const auto& species:session.domain().content().species)if(species.rarity==rarities[filter])expected.insert(species.id);
  require(browse("mastery-claim","collection-next","collection-prev")==expected,"Collection rarity filter omits or misclassifies a species");
 }
 button("filter0");button("sort");require(ViewTestAccess::page(view)==0,"Changing Collection sort did not reset pagination");
 button("sort");button("sort");
 const auto beforeClaim=session.domain().state();button("mastery-claimneonTetra");
 require(session.domain().masteryProgress("neonTetra").tier==1,"Collection did not claim the earned Adult mastery badge");
 require(session.domain().state().wallet.coins==beforeClaim.wallet.coins&&session.domain().state().wallet.pearls==beforeClaim.wallet.pearls&&session.domain().state().xp==beforeClaim.xp,"Deferred mastery rewards changed the economy");
 auto claims=ViewTestAccess::controls(view,"mastery-claim");require(std::find(claims.begin(),claims.end(),"neonTetra")==claims.end(),"Collection still offers an already claimed mastery tier");
 button("panel-close");

 session.domain().install(fresh);session.domain().fixture("aquarium");render();
 button("nav1");button("shop-filter0");button("panel-close");
 button("nav1");button("shop-tab1");button("shop-care");panel(Panel::None);button("done");
 button("nav1");button("shop-tab2");button("shop-tab3");button("shop-tab0");button("buyguppy");panel(Panel::None);
 require(view.tool()==Tool::Buy,"Buy did not arm placement");
 require(session.domain().state().fish.size()==6,"Purchase happened before placement");
 world({590,330});
 require(session.domain().state().fish.size()==7,"Egg placement did not buy one fish");
 require(session.domain().state().wallet.coins==215,"Purchase did not update wallet");
 button("done");require(view.tool()==Tool::Select,"Done did not restore Select");

 button("nav0");button("tool-food");panel(Panel::None);require(view.tool()==Tool::Food,"Food tool did not activate");
 world({410,210});require(session.domain().pellets().size()==1,"Touch did not drop one food pellet");
 button("done");

 button("tool-select");button("select-option0");require(view.tool()==Tool::Move,"Move tool is unreachable");
 auto before=session.domain().state().fish[2];auto from=canvas.toScreen(before.position);auto to=canvas.toScreen({420,340});
 finger(SDL_EVENT_FINGER_DOWN,from.x,from.y);finger(SDL_EVENT_FINGER_MOTION,to.x,to.y);finger(SDL_EVENT_FINGER_UP,to.x,to.y);render();
 require(std::abs(session.domain().fish(before.id)->position.x-420)<.1,"Touch drag did not move fish");
 button("done");button("tool-select");button("select-option1");world({420,340});
 require(session.domain().fish(before.id)->stashed,"Stash did not store fish");
 button("done");button("tool-select");button("select-option2");panel(Panel::Inventory);
 button("restore"+std::to_string(before.id.value));world({430,310});
 require(!session.domain().fish(before.id)->stashed,"Bag did not restore fish");

 button("tool-sell");const auto count=session.domain().state().fish.size();const auto coins=session.domain().state().wallet.coins;
 world({430,310});
 require(session.domain().state().fish.size()==count-1,"Sell did not remove selected fish");
 require(session.domain().state().wallet.coins==coins+session.domain().content().find(before.species)->saleCoins[before.age],"Sell did not update live wallet from the source stage value");
 button("done");
 if(width==804)checkSpeciesArtwork(canvas,session.domain().content(),assets);
 std::cout<<"PASS "<<width<<'x'<<height<<": source starters, all 46 Shop and Collection entries, purchase gates, mastery, modal backdrop, navigation and fish controls\n";
}
}
int main(int argc,char** argv){
 try{
  if(argc!=2)throw std::runtime_error("Usage: aquarium_view_tests ASSETS");
  for(auto [w,h]:{std::pair{804,415},std::pair{852,393},std::pair{667,375},std::pair{1024,768},std::pair{1210,834}})checkViewport(argv[1],w,h);
  return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
