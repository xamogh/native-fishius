#include "aquarium/ui_renderer.hpp"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace aq {struct ViewTestAccess {
 static Rect button(const View& v,std::string_view id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing button "+std::string(id));}
 static Panel panel(const View& v){return v.panel_;}static Currency category(const View& v){return v.currencyCategory_;}
 static bool funds(const View& v){return bool(v.fundsDialog_);}static bool notice(const View& v){return bool(v.noticeDialog_);}
};}
namespace {
using namespace aq;
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
Surface pixels(Canvas& c){Surface raw(SDL_RenderReadPixels(c.renderer(),nullptr),SDL_DestroySurface);check(bool(raw),"No pixels");return Surface(SDL_ConvertSurface(raw.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);}
int diff(const SDL_Surface& a,const SDL_Surface& b){check(a.w==b.w&&a.h==b.h,"Pixel size mismatch");int n=0;for(int y=0;y<a.h;++y)for(int x=0;x<a.w;++x)n+=std::memcmp(static_cast<const Uint8*>(a.pixels)+y*a.pitch+x*4,static_cast<const Uint8*>(b.pixels)+y*b.pitch+x*4,4)!=0;return n;}
void click(View& view,Canvas& c,std::string_view id,double now){const auto r=ViewTestAccess::button(view,id);SDL_Event e{};e.type=SDL_EVENT_FINGER_DOWN;e.tfinger.touchID=1;e.tfinger.fingerID=1;e.tfinger.x=(r.x+r.w*.5f)/c.width();e.tfinger.y=(r.y+r.h*.5f)/c.height();view.event(e,now);e.type=SDL_EVENT_FINGER_UP;view.event(e,now+.01);view.render(now+.8);}
}
int main(int argc,char** argv){try{const auto assets=std::filesystem::absolute(argc>1?argv[1]:"assets");bool native=argc>2&&std::string_view(argv[2])=="--native";std::ifstream input(assets/"content.json");const auto content=Content::fromJson(Json::parse(input));UiDocument design(assets/"ui/studio.json",assets);check(design.available(),"V2 design missing");auto project=design.project();Json results=Json::array();
 for(const auto [w,h]:{std::pair{1088,635},std::pair{852,393},std::pair{667,375},std::pair{1024,768}}){Canvas canvas(assets,w,h,!native);int pw,ph;SDL_GetWindowSizeInPixels(canvas.window(),&pw,&ph);int density=pw/w;Session session(content,"/tmp/aq-ui-runtime-unused.json",1000,true);auto s=session.domain().state();s.tutorialStep=11;s.settings.sound=false;session.domain().install(s);
  for(const auto& screen:project.screens)for(const auto& [variant,_]:screen.variants){
   canvas.previewViewport(std::nullopt);View game(canvas,session);UiBindings bindings;bindings.values["amount"]=variant=="pearls"?"12":"240";game.previewUiScreen(screen.id,variant,bindings);game.render(1);auto a=pixels(canvas);
   canvas.previewViewport(PreviewViewport{w,h,density,{}});View editor(canvas,session);editor.setUiProject(project);editor.previewUiScreen(screen.id,variant,bindings);editor.render(1);auto b=pixels(canvas);int count=diff(*a,*b);check(count==0,"Saved game and editor draft pixels differ");check(editor.uiLayout()&&!editor.uiLayout()->elements.empty(),"Editor has no editable layers");results.push_back({{"screen",screen.id},{"variant",variant},{"width",w},{"height",h},{"density",density},{"differentPixels",count}});
  }
  for(bool pearl:{false,true}){View view(canvas,session);view.setUiProject(project);view.previewPurchase(1,0,0,1,true);view.render(1);click(view,canvas,pearl?"tank-pearl1":"tank-buy",1);check(ViewTestAccess::funds(view),"Unfunded tank purchase did not show dialog");click(view,canvas,"funds-shop",2);check(ViewTestAccess::panel(view)==Panel::CurrencyShop&&ViewTestAccess::category(view)==(pearl?Currency::Pearls:Currency::Coins),"Shortfall opened wrong shop");click(view,canvas,"panel-close",3);check(ViewTestAccess::panel(view)==Panel::Tanks,"Shop lost tank return state");}
  View view(canvas,session);view.setUiProject(project);view.previewPurchase(40,1000000,1000,2,false);view.render(1);click(view,canvas,"tank-detail-buy",1);check(session.domain().tank({2})!=nullptr,"Funded tank unlock failed");view.previewPurchase(40,1000000,1000,1,true);view.render(3);auto coins=session.domain().state().wallet.coins;click(view,canvas,"tank-buy",3);check(session.domain().tank({1})->slots==15&&session.domain().state().wallet.coins<coins,"Funded tank upgrade failed");view.previewPurchase(1,1000000,1000,3,false);view.render(5);click(view,canvas,"tank-detail-buy",5);check(ViewTestAccess::notice(view),"Level requirement has no notice");
  if(w==1088){
   const auto temp=std::filesystem::temp_directory_path()/("aq-live-preview-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(temp);auto file=temp/"live.json";
   auto live=project.toJson();live["preview"]={{"screen","general-dialog"},{"variant","pearls"},{"width",w},{"height",h},{"density",density},{"values",{{"amount","77"}}}};writeUiJson(file,live);
   View connected(canvas,session,file);connected.render(1);check(std::filesystem::exists(file.string()+".ack.json"),"Connected game did not acknowledge draft");
   bool amount=false;for(const auto& e:connected.uiLayout()->elements)amount|=e.type=="richtext"&&e.text.find("77")!=std::string::npos;check(amount,"Connected preview did not receive scenario values");
   auto edited=project;edited.styles["heading"].size=71;live=edited.toJson();live["preview"]={{"flow",true},{"level",1},{"coins",0},{"pearls",0},{"tank",1},{"owned",true}};writeUiJson(file,live);SDL_Delay(260);connected.render(2);
   check(connected.uiRevision()==uiFingerprint(edited)&&ViewTestAccess::panel(connected)==Panel::Tanks,"Connected draft or purchase scenario failed to reload");click(connected,canvas,"tank-buy",2);check(ViewTestAccess::funds(connected),"Connected preview bypassed real purchase rules");
   std::ifstream ack(file.string()+".ack.json");check(Json::parse(ack).at("revision")==uiFingerprint(edited),"Acknowledged the wrong draft revision");
   UiDocument approved(assets/"ui/studio.json",assets);check(approved.project()==project,"Draft preview changed the approved design");std::filesystem::remove_all(temp);
  }
  // A composed screen exercises actual anchor, wrap, hug and below rules.
  auto layoutProject=project;UiScreen sample;sample.id="layout-test";sample.name="Layout test";sample.root.id="root";sample.root.box={0,0,400,400};UiNode first;first.id="first";first.type="text";first.text="A long translated sentence with several words";first.box={10,10,180,20};first.layout.wrap=true;first.layout.height="hug";UiNode second=first;second.id="second";second.text="Below";second.layout.after="first";second.layout.gap=12;sample.root.children={first,second};sample.variants["default"]={};layoutProject.screens.push_back(sample);layoutProject.validate();canvas.begin();auto layout=renderUiScreen(canvas,layoutProject,"layout-test","default",{100,100,400,400});const UiPlaced *a=nullptr,*b=nullptr;for(auto& e:layout.elements){if(e.id=="first")a=&e;if(e.id=="second")b=&e;}check(a&&b&&a->box.h>20&&b->box.y>=a->box.y+a->box.h+11,"Wrap/hug/below constraints failed");
 }
 const auto report=assets.parent_path()/"evidence/studio"/(native?"v2-native-parity.json":"v2-software-parity.json");writeUiJson(report,results);std::cout<<"PASS all component screens at four sizes, exact preview pixels, purchase flows and layout constraints\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
