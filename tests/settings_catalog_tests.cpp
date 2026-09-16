#include "aquarium/view.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static bool has(const View& view,std::string_view id){return std::any_of(view.buttons_.begin(),view.buttons_.end(),[&](const auto& button){return button.id==id;});}
 static Rect button(const View& view,std::string_view id){for(const auto& button:view.buttons_)if(button.id==id)return button.area;throw std::runtime_error("Missing control: "+std::string(id));}
 static void click(View& view,std::string_view id){for(const auto& button:view.buttons_)if(button.id==id){auto action=button.action;action();return;}throw std::runtime_error("Missing control: "+std::string(id));}
 static void shopPage(View& view,int category,int page){view.category_=category;view.page_=page;view.panelMotion_.settle();}
 static int pages(const View& view,int category){return view.shopPageCount(category);}
 static bool textDrawn(const Canvas& canvas,std::string_view text){for(const auto& [key,entry]:canvas.textCache_)if(entry.used==canvas.frame_&&key.ends_with(":"+std::string(text)))return true;return false;}
};
}
namespace {
using namespace aq;
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
Millis date(int year,unsigned month,unsigned day){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::sys_days{std::chrono::year{year}/month/day}.time_since_epoch()).count();}
bool findShopItem(View& view,int category,std::string_view id,double& time){
 view.setPanel(Panel::Shop);
 for(int page=0;page<ViewTestAccess::pages(view,category);++page){
  ViewTestAccess::shopPage(view,category,page);view.render(time+=1);
  if(ViewTestAccess::has(view,id))return true;
 }
 return false;
}
void seasonalCatalog(const std::filesystem::path& assets,const Content& base){
 auto content=base;
 const auto starts=date(2026,12,10),ends=date(2027,1,7);
 content.configureDecorEvents(Json::array({{{"name","Winterfest"},{"starts_at",starts},{"ends_at",ends}}}));
 Session session(content,"/tmp/aquarium-seasonal-catalog-unused.json",date(2026,2,6),true);
 Canvas canvas(assets,667,375,true);View view(canvas,session);auto& domain=session.domain();double time=1;
 check(!findShopItem(view,0,"buyheartfinTetra",time),"Heartfin appears before its event starts");
 domain.setCalendar(date(2026,2,7));
 check(!findShopItem(view,0,"buyheartfinTetra",time),"Deferred event fish became a live shop offer");
 check(!findShopItem(view,0,"buyanniversaryRainbowfish",time),"Unscheduled event fish appears in the Shop");
 auto state=domain.state();state.xp=content.levels.back();state.highestRewardedLevel=40;state.wallet.coins=1000000;domain.install(state);
 domain.setCalendar(starts-1);check(!findShopItem(view,1,"decor-buyCL-11",time),"Seasonal decor appears before its configured event");
 domain.setCalendar(starts);state=domain.state();state.xp=0;domain.install(state);
 check(findShopItem(view,1,"decor-buyCL-11",time),"Seasonal Coin decor is hidden at its configured start or below its level gate");
 check(ViewTestAccess::textDrawn(canvas,"Winterfest event active"),"Seasonal decor does not identify its active event");
 check(domain.blocker(*content.findDecor("CL-11")).error==Error::Level,"An active event bypasses the decor level gate");
 state=domain.state();state.xp=content.levels.back();domain.install(state);
 check(findShopItem(view,1,"decor-buyCL-11",time),"Unlocked seasonal decor is hidden");ViewTestAccess::click(view,"decor-buyCL-11");
 check(domain.state().pendingDecor.empty(),"Shop selection prematurely bought seasonal decor");
 view.render(time+=1);ViewTestAccess::click(view,"decor-confirm");
 check(std::any_of(domain.state().decor.begin(),domain.state().decor.end(),[](const auto& item){return item.kind=="CL-11"&&!item.stored;}),"Confirm did not buy and place seasonal decor");
 check(findShopItem(view,2,"decor-buyPL-11",time),"Seasonal Pearl decor is hidden during its configured event");
 check(domain.blocker(*content.findDecor("PL-11")).error==Error::Funds,"Expected the seasonal Pearl decor currency gate");
 const auto poorBefore=encode(domain.state());ViewTestAccess::click(view,"decor-buyPL-11");
 check(encode(domain.state())==poorBefore,"An active event bypasses seasonal decor currency costs");
 view.render(time+=1);ViewTestAccess::click(view,"funds-close");
 domain.setCalendar(date(2027,1,2));check(!findShopItem(view,0,"buyfrostAngelfish",time),"Deferred seasonal companion became a productive shop offer");
 domain.setCalendar(ends-1);check(findShopItem(view,1,"decor-buyCL-11",time),"Seasonal decor closes before the configured end");
 domain.setCalendar(ends);check(!findShopItem(view,1,"decor-buyCL-11",time),"Seasonal decor remains visible at its exclusive end");
 Session unscheduled(base,"/tmp/aquarium-unscheduled-catalog-unused.json",starts,true);View unconfiguredView(canvas,unscheduled);
 check(!findShopItem(unconfiguredView,1,"decor-buyCL-11",time),"Proposed decor dates were used as a live event schedule");
 check(findShopItem(unconfiguredView,1,"decor-buyCP-01",time),"Evergreen decor disappeared from the Shop");
 std::cout<<"PASS seasonal catalog start/end dates, year crossing, explicit decor schedules, event labels, level and currency gates\n";
}
void restoredSettings(const std::filesystem::path& assets,const Content& content,int width,int height){
 const auto folder=std::filesystem::temp_directory_path()/("aquarium-settings-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(folder);const auto save=folder/"save.json";
 try{
  {
   Session first(content,save,0);
   check(bool(first.command({.action=Action::SetVolume,.value=.2})),"Could not save the sound volume");
   check(bool(first.command({.action=Action::SetReducedMotion,.value=1})),"Could not save reduced motion");
  }
  Session session(content,save,0);Canvas canvas(assets,width,height,true);View view(canvas,session);double time=1;
  const auto render=[&]{view.render(time+=1);};view.setPanel(Panel::Settings);render();
  check(ViewTestAccess::textDrawn(canvas,"20%"),"The reopened sound slider does not display the saved volume");
  check(ViewTestAccess::textDrawn(canvas,"REDUCED MOTION")&&ViewTestAccess::has(view,"reduced-motion-toggle"),"Settings has no clearly labeled reduced-motion switch");
  check(ViewTestAccess::textDrawn(canvas,"Not available")&&!ViewTestAccess::has(view,"music-toggle")&&!ViewTestAccess::has(view,"music-volume"),"Unavailable music still has working-looking controls");
  for(int quality=0;quality<3;++quality)check(!ViewTestAccess::has(view,"quality"+std::to_string(quality)),"A graphics quality control still changes motion settings");
  check(session.domain().state().settings.reducedMotion,"Saved reduced motion was reset when opening Settings");
  ViewTestAccess::click(view,"sound-toggle");render();
  check(!session.domain().state().settings.sound&&session.domain().state().settings.reducedMotion,"Sound changes reduced motion");
  ViewTestAccess::click(view,"reduced-motion-toggle");render();
  check(!session.domain().state().settings.reducedMotion&&std::abs(session.domain().state().settings.volume-.2)<1e-6,"Reduced motion changes the sound volume");
  ViewTestAccess::click(view,"reduced-motion-toggle");render();
  const auto slider=ViewTestAccess::button(view,"sound-volume");
  for(const auto type:{SDL_EVENT_FINGER_DOWN,SDL_EVENT_FINGER_UP}){
   SDL_Event event{};event.type=type;event.tfinger.touchID=1;event.tfinger.fingerID=1;
   event.tfinger.x=(slider.x+slider.w*.5f)/canvas.width();event.tfinger.y=(slider.y+slider.h*.5f)/canvas.height();view.event(event,time);
  }
  render();const auto volume=session.domain().state().settings.volume;
  check(volume>.48&&volume<.52,"Sound slider input no longer sets its displayed position");
  Session reloaded(content,save,0);View reopened(canvas,reloaded);reopened.setPanel(Panel::Settings);reopened.render(time+=1);
  check(reloaded.domain().state().settings.reducedMotion&&std::abs(reloaded.domain().state().settings.volume-volume)<1e-6,"Settings changes did not survive a reload");
  check(ViewTestAccess::textDrawn(canvas,std::to_string(int(std::round(volume*100)))+"%"),"Changed sound volume reopens with a different displayed value");
  std::cout<<"PASS restored sound volume, independent reduced motion and unavailable music controls "<<width<<'x'<<height<<'\n';
 }catch(...){std::filesystem::remove_all(folder);throw;}
 std::filesystem::remove_all(folder);
}
}
int main(int argc,char** argv){try{
 if(argc!=2)return 2;const auto assets=std::filesystem::absolute(argv[1]);std::ifstream file(assets/"content.json");const auto content=aq::Content::fromJson(aq::Json::parse(file));
 seasonalCatalog(assets,content);restoredSettings(assets,content,667,375);restoredSettings(assets,content,1024,768);return 0;
}catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}}
