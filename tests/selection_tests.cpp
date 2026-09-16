#include "fixtures.hpp"
#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static bool has(const View& v,std::string_view id){return std::any_of(v.buttons_.begin(),v.buttons_.end(),[&](const auto& b){return b.id==id;});}
 static Rect button(const View& v,std::string_view id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing "+std::string(id));}
 static Panel panel(const View& v){return v.panel_;}
 static std::uint64_t decor(const View& v){return v.selectedDecor_;}
 static bool dragging(const View& v){return v.selectionDragging_;}
 static WorldPoint preview(const View& v){return v.decorPreview_.value();}
 static WorldPoint fishPreview(const View& v){return v.fishPreview_.value();}
 static std::uint64_t hitDecor(const View& v,SDL_FPoint p){return v.hitDecor(p.x,p.y);}
};
}
namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool near(WorldPoint a,WorldPoint b){return std::hypot(a.x-b.x,a.y-b.y)<.02;}
void run(const std::filesystem::path& assets,int width,int height,bool touch){
 std::ifstream input(assets/"content.json");Session session(Content::fromJson(Json::parse(input)),"/tmp/aquarium-selection-unused.json",1000,true);
 auto& domain=session.domain();auto state=domain.state();state.fish.resize(1);auto& fish=state.fish.front();
 const auto id=fish.id;fish.position={400,280};fish.motion.previous=fish.position;testing::stage(fish,1);fish.lastFedAt=state.simNow;
 state.decor={{1,"CP-01",{1},{700,480},false,false,1.12}};state.nextDecorId=2;state.decorOwned={"CP-01"};domain.install(state);
 Canvas canvas(assets,width,height,true);View view(canvas,session);double time=1;
 auto render=[&]{view.render(time+=.7);};render();
 auto send=[&](Uint32 type,SDL_FPoint p){
  SDL_Event e{};
  if(touch){e.type=type==SDL_EVENT_MOUSE_BUTTON_DOWN?SDL_EVENT_FINGER_DOWN:type==SDL_EVENT_MOUSE_BUTTON_UP?SDL_EVENT_FINGER_UP:SDL_EVENT_FINGER_MOTION;
   e.tfinger.touchID=1;e.tfinger.fingerID=1;e.tfinger.x=p.x/canvas.width();e.tfinger.y=p.y/canvas.height();
  }else{
   int w{},h{};SDL_GetWindowSize(canvas.window(),&w,&h);e.type=type;
   if(type==SDL_EVENT_MOUSE_MOTION){e.motion.x=p.x*w/canvas.width();e.motion.y=p.y*h/canvas.height();}
   else{e.button.button=SDL_BUTTON_LEFT;e.button.x=p.x*w/canvas.width();e.button.y=p.y*h/canvas.height();}
  }
  view.event(e,time);
 };
 auto down=[&](SDL_FPoint p){send(SDL_EVENT_MOUSE_BUTTON_DOWN,p);};
 auto move=[&](SDL_FPoint p){send(SDL_EVENT_MOUSE_MOTION,p);};
 auto up=[&](SDL_FPoint p){send(SDL_EVENT_MOUSE_BUTTON_UP,p);render();};
 auto tap=[&](SDL_FPoint p){down(p);up(p);};
 auto button=[&](std::string_view name){const auto r=ViewTestAccess::button(view,name);tap({r.x+r.w*.5f,r.y+r.h*.5f});};
 const auto captures=std::filesystem::absolute(assets).parent_path()/"evidence/direct-selection"/(std::to_string(width)+"x"+std::to_string(height))/(touch?"touch":"mouse");std::filesystem::create_directories(captures);
 auto capture=[&](const char* name){check(canvas.capture(captures/name),"Cannot capture selection");};
 check(!ViewTestAccess::has(view,"tool-select")&&!ViewTestAccess::has(view,"select-option0")&&ViewTestAccess::has(view,"inventory"),"Old selection menu remains or Bag is missing");
 const auto original=encode(domain.state());
 const auto grab=canvas.toScreen({680,460});tap(grab);
 check(!ViewTestAccess::decor(view)&&!ViewTestAccess::hitDecor(view,grab)&&encode(domain.state())==original,"Normal selection interacts with decor");
 button("inventory");button("arrange-tank");
 check(view.tool()==Tool::Decor&&ViewTestAccess::has(view,"done"),"Arrange tank does not enter a mode with a visible exit");
 tap(grab);
 check(ViewTestAccess::decor(view)==1&&encode(domain.state())==original,"Selecting decor changes its saved state");
 check(view.tool()==Tool::Decor,"Selecting decor exits arrange mode");
 auto stash=ViewTestAccess::button(view,"selection-stash");
 check(stash.w>=canvas.minimumTouchSize()&&stash.h>=canvas.minimumTouchSize(),"Stash touch target is too small");
 check(stash.x>=0&&stash.y>=0&&stash.x+stash.w<=canvas.width()&&stash.y+stash.h<=canvas.height(),"Stash is clipped");
 capture("plant-selected.png");
 down(grab);move({grab.x+2,grab.y+2});up({grab.x+2,grab.y+2});
 check(encode(domain.state())==original,"A small tap wobble moved decor");
 const auto destination=canvas.toScreen({780,500});down(grab);move(destination);render();
 check(ViewTestAccess::dragging(view)&&near(ViewTestAccess::preview(view),{800,520}),"Dragging snapped the grabbed point to the item's base");
 check(near(domain.decoration(1)->position,{700,480})&&!ViewTestAccess::has(view,"selection-stash"),"Decor drag commits early or leaves Stash active");
 capture("plant-dragging.png");up(destination);
 check(near(domain.decoration(1)->position,{700,480})&&ViewTestAccess::has(view,"decor-confirm"),"Release committed before confirmation");
 button("decor-confirm");
 check(near(domain.decoration(1)->position,{800,520})&&ViewTestAccess::has(view,"selection-stash"),"Release did not move decor and retain selection");
 move(canvas.toScreen({700,400}));render();check(near(domain.decoration(1)->position,{800,520}),"Released decor follows the pointer");
 auto saved=encode(domain.state());auto from=canvas.toScreen({780,500});
 down(from);move(canvas.toScreen({720,450}));render();
 SDL_Event blur{};blur.type=SDL_EVENT_WINDOW_FOCUS_LOST;view.event(blur,time);up(destination);
 check(encode(domain.state())==saved&&!ViewTestAccess::dragging(view),"Interrupted decor drag changed saved placement");
 down(from);move(canvas.toScreen({720,450}));up({-20,-20});check(encode(domain.state())==saved,"Release outside the tank moved decor");
 down(from);move(canvas.toScreen({720,450}));render();
 const auto cancel=ViewTestAccess::button(view,"decor-cancel");up({cancel.x+cancel.w*.5f,cancel.y+cancel.h*.5f});
 check(encode(domain.state())==saved&&ViewTestAccess::panel(view)==Panel::None,"Drag release activated a control or saved placement");
 if(ViewTestAccess::has(view,"decor-cancel"))button("decor-cancel");
 tap(canvas.toScreen({540,180}));check(!ViewTestAccess::has(view,"selection-stash"),"Empty water did not deselect");
 tap(from);button("selection-stash");
 check(domain.decoration(1)->stored&&!ViewTestAccess::has(view,"selection-stash"),"Stash did not store only the selected decor");
 button("inventory");check(ViewTestAccess::panel(view)==Panel::Inventory,"Bag did not open inventory");button("restore-decor1");tap(canvas.toScreen({700,480}));button("decor-confirm");
 check(!domain.decoration(1)->stored&&near(domain.decoration(1)->position,{700,480}),"Bag cannot restore the stored decoration");
 check(view.tool()==Tool::Decor&&ViewTestAccess::has(view,"done"),"Restore does not retain arrange mode");
 button("done");tap(canvas.toScreen({680,460}));
 check(view.tool()==Tool::Select&&!ViewTestAccess::decor(view)&&!ViewTestAccess::has(view,"selection-stash"),"Done leaves decor selectable in normal mode");

 const auto fishAt=canvas.toScreen(domain.fish(id)->position);tap(fishAt);
 check(ViewTestAccess::panel(view)==Panel::Details&&!ViewTestAccess::has(view,"selection-stash"),"Fish tap lost details or offers Stash");capture("fish-selected.png");
 const auto fishBefore=encode(domain.state());
 down(fishAt);move({fishAt.x+2,fishAt.y+2});up(fishAt);check(encode(domain.state())==fishBefore,"Fish tap wobble changes state");
 const auto fishGrab=canvas.toScreen({405,282}),fishTo=canvas.toScreen({505,602});down(fishGrab);move(fishTo);render();
 check(ViewTestAccess::panel(view)==Panel::None&&near(ViewTestAccess::fishPreview(view),{500,600}),"Fish cannot be dragged into the lower tank with its grab offset");
 check(encode(domain.state())==fishBefore,"Fish drag changes save data before release");
 capture("fish-dragging.png");up(fishTo);check(near(domain.fish(id)->position,{500,600})&&!ViewTestAccess::has(view,"selection-stash"),"Fish drag did not commit or offers Stash");
 down(canvas.toScreen({500,600}));move(canvas.toScreen({450,300}));view.cancelGesture();up(fishTo);
 check(near(domain.fish(id)->position,{500,600}),"Interrupted fish drag did not restore position");
 down(canvas.toScreen({500,600}));move(canvas.toScreen({450,300}));up({-20,-20});
 check(near(domain.fish(id)->position,{500,600}),"Invalid release did not restore fish position");
 const auto beforeStash=encode(domain.state());
 check(domain.execute({.action=Action::Stash,.fish=id}).error==Error::Unavailable&&encode(domain.state())==beforeStash,"Fish stash must be rejected without changing state");
 button("inventory");check(!ViewTestAccess::has(view,"inventory-fish"),"Bag shows a fish tab without legacy stored fish");button("panel-close");
 // Kept adults can be stored and restored without paying a second reward.
 auto grown=domain.state();testing::stage(grown.fish.front(),4);domain.install(grown);check(bool(domain.execute({.action=Action::Keep,.fish=id})),"Cannot keep adult");check(bool(domain.execute({.action=Action::Stash,.fish=id})),"Cannot store companion");const auto paid=domain.state();
 domain.install(decodeAndValidate(encode(paid),domain.content()));render();button("inventory");button("inventory-fish");button("restore"+std::to_string(id.value));tap(canvas.toScreen({420,600}));
 check(!domain.companion(id)->stored&&near(domain.companion(id)->position,{420,600}),"Companion cannot be restored to the lower tank through Bag");
 check(domain.state().wallet.coins==paid.wallet.coins&&domain.state().wallet.pearls==paid.wallet.pearls&&domain.state().xp==paid.xp,"Restoring a companion changes money or XP");
 check(domain.decoration(1)->sizeMul==1.12,"Rearranging changes a saved custom size");
 // Fish take priority when their hit area overlaps a decoration in arrange mode.
 auto overlapping=domain.state();overlapping.decor.front().position={420,630};domain.install(overlapping);
 button("inventory");button("arrange-tank");tap(canvas.toScreen({420,600}));
 check(ViewTestAccess::panel(view)==Panel::Details&&!ViewTestAccess::decor(view),"Decor intercepted a fish tap in arrange mode");
 std::cout<<"PASS direct selection, offset dragging, cancellation, Stash and Bag "<<width<<'x'<<height<<(touch?" touch":" mouse")<<'\n';
}
}
int main(int argc,char** argv){try{if(argc!=2)return 2;for(const auto [w,h]:{std::pair{667,375},std::pair{852,393},std::pair{1024,768}})for(bool touch:{true,false})run(argv[1],w,h,touch);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
