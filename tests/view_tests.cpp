#include "aquarium/view.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static Panel panel(const View& view){return view.panel_;}
 static Rect panelBounds(const View& view){auto r=view.panelRect_;r.x+=view.panelOrigin();return r;}
 static Rect button(const View& view,std::string_view id){
  auto found=std::find_if(view.buttons_.begin(),view.buttons_.end(),[&](const auto& b){return b.id==id;});
  if(found==view.buttons_.end())throw std::runtime_error("Missing control: "+std::string(id));
  return found->area;
 }
};
}

namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void checkViewport(const std::filesystem::path& assets,int width,int height){
 using namespace aq;
 std::ifstream input(assets/"content.json");
 Session session(Content::fromJson(Json::parse(input)),"/tmp/aquarium-ui-test-unused.json",0,true);
 session.domain().fixture("aquarium");
 Canvas canvas(assets,width,height,false);
 View view(canvas,session);
 double time=0;
 auto render=[&]{view.render(time+=.02);canvas.present();};
 render();
 auto finger=[&](Uint32 type,float x,float y){
  const float fit=std::min(float(width)/canvas.width(),float(height)/canvas.height());
  SDL_Event event{};event.type=type;event.tfinger.touchID=1;event.tfinger.fingerID=1;
  event.tfinger.x=((float(width)-canvas.width()*fit)*.5f+x*fit)/float(width);
  event.tfinger.y=((float(height)-canvas.height()*fit)*.5f+y*fit)/float(height);
  view.event(event,time);
 };
 auto tap=[&](float x,float y){finger(SDL_EVENT_FINGER_DOWN,x,y);finger(SDL_EVENT_FINGER_UP,x,y);render();};
 auto button=[&](std::string_view id){auto r=ViewTestAccess::button(view,id);tap(r.x+r.w*.5f,r.y+r.h*.5f);};
 auto close=[&]{SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=SDLK_ESCAPE;view.event(e,time);render();};
 auto panel=[&](Panel expected){require(ViewTestAccess::panel(view)==expected,"Wrong panel after touch");};
 auto world=[&](WorldPoint point){auto p=canvas.toScreen(point);tap(p.x,p.y);};

 panel(Panel::None);
 for(const char* id:{"coin-more","pearl-more","notifications","menu","nav0","nav1","tool-select","tool-food","nav3","tool-sell"}){
  auto r=ViewTestAccess::button(view,id);
  require(r.x>=0&&r.y>=0&&r.x+r.w<=canvas.width()+1&&r.y+r.h<=canvas.height()+1,"Control extends beyond canvas");
 }
 button("nav0");panel(Panel::Tanks);
 const auto tankButton=ViewTestAccess::button(view,"nav0"),tankMenu=ViewTestAccess::panelBounds(view);
 require(tankMenu.x>tankButton.x+tankButton.w,"Tanks menu overlaps its button");
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
 tap(plus.x+plus.w*.5f,plus.y-std::max(1.f,extension*.7f));panel(Panel::Gifts);button("panel-close");

 button("nav1");panel(Panel::Shop);
 const auto shopMenu=ViewTestAccess::panelBounds(view);
 for(const char* id:{"nav0","nav1","tool-select","tool-food","nav3","tool-sell","notifications","menu"})ViewTestAccess::button(view,id);
 // Shop is modal: a backdrop tap dismisses it and must not reach the menu
 // or tool behind it. A later, separate tap can activate that control.
 tap(shopMenu.x+5,shopMenu.y+150);panel(Panel::Shop);
 button("tool-select");panel(Panel::None);require(view.tool()==Tool::Select,"Backdrop changed the active tool");
 button("nav1");button("tool-food");panel(Panel::None);require(view.tool()==Tool::Select,"Backdrop activated Food");
 button("tool-food");require(view.tool()==Tool::Food,"Food did not activate after dismissing Shop");button("done");
 button("nav1");button("tool-sell");panel(Panel::None);require(view.tool()==Tool::Select,"Backdrop activated Sell");
 button("nav1");button("nav3");panel(Panel::None);
 button("nav3");panel(Panel::Collection);button("panel-close");
 button("nav1");button("notifications");panel(Panel::None);
 button("notifications");panel(Panel::Quests);button("panel-close");
 button("nav1");auto tanksBehind=ViewTestAccess::button(view,"nav0");tap(tanksBehind.x+10,tanksBehind.y+tanksBehind.h*.5f);panel(Panel::None);
 button("nav0");panel(Panel::Tanks);button("panel-close");
 button("nav1");button("offers");panel(Panel::Gifts);button("panel-close");
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
 require(session.domain().state().wallet.coins==coins+355,"Sell did not update live wallet");
 button("done");
 std::cout<<"PASS "<<width<<'x'<<height<<": navigation, 44-point touch target, buy/place, food, move, stash, restore, sell, live wallet\n";
}
}
int main(int argc,char** argv){
 try{
  if(argc!=2)throw std::runtime_error("Usage: aquarium_view_tests ASSETS");
  for(auto [w,h]:{std::pair{804,415},std::pair{852,393},std::pair{667,375}})checkViewport(argv[1],w,h);
  return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
