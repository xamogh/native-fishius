#include <cmath>
#include <cstdlib>
#include "aquarium/canvas.hpp"
#include "aquarium/loading.hpp"
#include "aquarium/hud.hpp"
#include "aquarium/hud_tanks.hpp"
#include "aquarium/hud_dialog.hpp"
#include "aquarium/hud_funds.hpp"
#include "aquarium/hud_placement.hpp"
#include "aquarium/hud_care.hpp"
#include "aquarium/shop_theme.hpp"
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
namespace {
aq::Millis wallNow(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
struct Options {
 int w{1088},h{635},frames{};
 bool software{},fresh{},still{},captureWindow{},sceneOnly{};
 double warp{1};
 std::string hudDialog,dialogTitle{"Dialog title"},fixture,uiVariant{"coins"};
 float dialogWidth{960},dialogHeight{640};
 std::filesystem::path assets,save,capture,report,sequence,loadingCapture;
};
Options options(int argc,char** argv){
 Options o;
 for(int i=1;i<argc;++i){
  const std::string s=argv[i];
  auto arg=[&](){if(i+1>=argc)throw std::runtime_error("Missing value for "+s);return std::string(argv[++i]);};
  if(s=="--width")o.w=std::stoi(arg());
  else if(s=="--height")o.h=std::stoi(arg());
  else if(s=="--frames")o.frames=std::stoi(arg());
  else if(s=="--warp")o.warp=std::stod(arg());
  else if(s=="--fixture")o.fixture=arg();
  else if(s=="--ui-variant")o.uiVariant=arg();
  else if(s=="--assets")o.assets=arg();
  else if(s=="--save")o.save=arg();
  else if(s=="--capture")o.capture=arg();
  else if(s=="--loading-capture")o.loadingCapture=arg();
  else if(s=="--report")o.report=arg();
  else if(s=="--sequence")o.sequence=arg();
  else if(s=="--software")o.software=true;
  else if(s=="--fresh")o.fresh=true;
  else if(s=="--still")o.still=true;
  else if(s=="--capture-window")o.captureWindow=true;
  else if(s=="--scene-only")o.sceneOnly=true;
  else if(s=="--hud-dialog")o.hudDialog=arg();
  else if(s=="--dialog-title")o.dialogTitle=arg();
  else if(s=="--dialog-width")o.dialogWidth=std::stof(arg());
  else if(s=="--dialog-height")o.dialogHeight=std::stof(arg());
  else if(s=="--hud-layout"){} // Compatibility alias: the current HUD is always used.
  else if(s=="--help"){
   std::cout<<"Fishius\nThe current interface opens by default.\n"
    "--assets PATH --save PATH --width N --height N\n"
    "--fixture aquarium|shop|tanks|tank-switcher|care|performance|bag|settings|projects|rewards\n"
    "--ui-variant fish|plants|decorations|backgrounds|tanks|coins|pearls\n"
    "--scene-only  Show the aquarium without background artwork or UI controls.\n"
    "--hud-layout  Compatibility alias for the default interface.\n"
    "--hud-dialog small|medium|large|custom --dialog-width N --dialog-height N --dialog-title TEXT\n"
    "--frames N --capture PNG --loading-capture PNG --sequence DIR --report JSON --software --warp N --fresh --still --capture-window\n"
    "Review fixtures and --fresh never overwrite your ordinary save.\n";
   std::exit(0);
  }else throw std::runtime_error("Unknown option "+s);
 }
 if(o.w<320||o.h<240||o.w>8192||o.h>8192||!std::isfinite(o.warp)||o.warp<1||o.warp>10000)throw std::runtime_error("Invalid viewport or time scale");
 const std::array<std::string_view,10> fixtures{"aquarium","shop","tanks","tank-switcher","care","performance","bag","settings","projects","rewards"};
 if(!o.fixture.empty()&&std::find(fixtures.begin(),fixtures.end(),o.fixture)==fixtures.end())throw std::runtime_error("Unknown fixture: "+o.fixture);
 if(o.sceneOnly&&!o.hudDialog.empty())throw std::runtime_error("Dialog previews require the interface");
 return o;
}
}
int main(int argc,char** argv){
 try{
  auto o=options(argc,argv);if(o.assets.empty()){const char* base=SDL_GetBasePath();o.assets=base?std::filesystem::path(base)/"assets":std::filesystem::path("assets");if(!std::filesystem::exists(o.assets/"content.json"))o.assets="assets";}
  aq::Canvas canvas(o.assets,o.w,o.h,o.software);
  const bool showHud=!o.sceneOnly;
  if(o.sceneOnly)SDL_SetWindowTitle(canvas.window(),"Fishius | Aquarium view");
  const auto startupStart=SDL_GetTicksNS();bool background=false,quit=false,reduced=true,loadingCaptured=false;
  aq::Session* loadingSession=nullptr;
  auto pollLoading=[&](const SDL_Event& e){
   if(e.type==SDL_EVENT_QUIT)quit=true;
   else if(e.type==SDL_EVENT_WILL_ENTER_BACKGROUND){background=true;if(loadingSession)loadingSession->suspend(wallNow());}
   else if(e.type==SDL_EVENT_DID_ENTER_FOREGROUND){background=false;if(loadingSession)loadingSession->resume(wallNow());}
  };
  auto loading=[&](float progress,std::string_view stage,bool artwork=true){
   SDL_Event e;while(SDL_PollEvent(&e))pollLoading(e);
   while(background&&!quit){if(SDL_WaitEventTimeout(&e,50))pollLoading(e);}
   if(quit)return false;
   canvas.loadingScreen(progress,stage,double(SDL_GetTicksNS()-startupStart)/1e9,artwork&&showHud,reduced);
   if(!loadingCaptured&&!o.loadingCapture.empty()&&progress>=.5f){
    if(!canvas.capture(o.loadingCapture))throw std::runtime_error("Loading screenshot capture failed");loadingCaptured=true;
   }
   canvas.present();return true;
  };
  // Present immediately, before content parsing, save recovery and asset work.
  if(!loading(0,"Opening your aquarium",false))return 0;
  std::ifstream contentFile(o.assets/"content.json");if(!contentFile)throw std::runtime_error("Missing assets/content.json. Run the documented content import.");auto content=aq::Content::fromJson(aq::Json::parse(contentFile));
  for(auto& s:content.species){s.artReady=s.artReady&&std::filesystem::exists(o.assets/s.asset);}
  for(auto& item:content.decorations)item.artReady=std::filesystem::exists(o.assets/item.asset);
  const auto decorEvents=o.assets/"decor-events.json";
  if(std::filesystem::exists(decorEvents)){std::ifstream f(decorEvents);content.configureDecorEvents(aq::Json::parse(f));}
  if(!loading(.04f,"Restoring your aquarium"))return 0;
  // Retain the original storage namespace so renaming never loses progress.
  if(o.save.empty()){char* p=SDL_GetPrefPath("Pixmot","FishX");if(!p)throw std::runtime_error(SDL_GetError());o.save=std::filesystem::path(p)/"save-v4.json";SDL_free(p);}
  const bool review=!o.fixture.empty()||o.fresh;
  aq::Session session(std::move(content),o.save,wallNow(),review);
  if(!o.fixture.empty())session.domain().fixture(o.fixture=="tank-switcher"?"shop":o.fixture);
  loadingSession=&session;reduced=session.domain().state().settings.reducedMotion;
  // Render the actual starting scene behind the loader to initialize its
  // remaining effects before gameplay and before starting the frame clock.
  aq::DialogSpec dialogSpec{o.dialogTitle};
  if(o.hudDialog=="small")dialogSpec.size=aq::DialogSize::Small;
  else if(o.hudDialog=="medium")dialogSpec.size=aq::DialogSize::Medium;
  else if(o.hudDialog=="large")dialogSpec.size=aq::DialogSize::Large;
  else if(o.hudDialog=="custom"){dialogSpec.size=aq::DialogSize::Custom;dialogSpec.customWidth=o.dialogWidth;dialogSpec.customHeight=o.dialogHeight;}
  else if(!o.hudDialog.empty())throw std::runtime_error("Unknown dialog size");
  aq::DialogState dialogState{!o.hudDialog.empty(),false};
  for(const auto& [fixture,title]:std::array<std::pair<std::string_view,std::string_view>,4>{{{"bag","Bag"},{"settings","Settings"},{"projects","Projects"},{"rewards","Rewards"}}}){
   if(o.fixture==fixture){dialogSpec={std::string(title),aq::DialogSize::Large};dialogState.open=true;}
  }
  bool shopOpen=showHud&&(o.fixture=="shop"||o.fixture=="tanks");
  aq::TankSwitcherState switcher;switcher.open=showHud&&o.fixture=="tank-switcher";
  bool shopPointerDown=false;
  std::optional<aq::HudPart> pressedDialogButton,pressedCurrencyButton;
  aq::ShopState shopState;
  aq::TankShopState tankShop;
  aq::FundsDialogState funds;
  aq::FundsShopReturn fundsShopReturn;
  if(o.fixture=="tanks")shopState.category=aq::ShopCategory::Tanks;
  if(showHud&&o.fixture=="shop"){
   if(o.uiVariant=="fish")shopState.category=aq::ShopCategory::Fish;
   else if(o.uiVariant=="plants")shopState.category=aq::ShopCategory::Plants;
   else if(o.uiVariant=="decorations")shopState.category=aq::ShopCategory::Decorations;
   else if(o.uiVariant=="environment"||o.uiVariant=="backgrounds")shopState.category=aq::ShopCategory::Environment;
   else if(o.uiVariant=="tanks")shopState.category=aq::ShopCategory::Tanks;
   else if(o.uiVariant=="pearls")shopState.subtab=1;
  }
  std::string shopFishDetails,shopNotice;
  bool tankShopNotice=false,tankShopPressed=false;
  auto tankShopButton=[](const aq::HudDialogLayout& dialog){const float u=dialog.unit;return aq::Rect{dialog.content.x+(dialog.content.w-256*u)*.5f,dialog.content.y+160*u,256*u,72*u};};
  if(showHud&&o.fixture=="shop"&&o.uiVariant=="tank-full"){
   shopState.category=aq::ShopCategory::Fish;shopNotice="Make room for more fish.";tankShopNotice=true;dialogSpec={"Tank full!",aq::DialogSize::Small};dialogState.open=true;
  }
  aq::FishPlacement placement;
  aq::HudCare care(canvas,session);aq::HudPointer hudPointer;
  auto showFunds=[&](const aq::Result& result){
   if(!aq::showFundsDialog(funds,result))return false;
   aq::cancelFishPlacement(placement);care.reset();switcher.open=false;
   dialogState.open=false;shopFishDetails.clear();shopNotice.clear();tankShopNotice=tankShopPressed=false;
   return true;
  };
  if(showHud&&o.fixture=="shop"&&(o.uiVariant=="not-enough-coins"||o.uiVariant=="not-enough-pearls")){
   shopState.category=aq::ShopCategory::Fish;
   showFunds({.error=aq::Error::Funds,.shortfall=o.uiVariant=="not-enough-pearls"?aq::CurrencyShortfall{0,12}:aq::CurrencyShortfall{101,0}});
  }
  bool pressedFishInfo=false;
  if(showHud&&o.fixture=="shop"&&o.uiVariant=="fish-details"){
   shopState.category=aq::ShopCategory::Fish;
   const auto items=aq::shopItems(session.domain(),shopState);
   if(!items.empty()&&items.front().fish){shopFishDetails=items.front().fish->id;dialogSpec={items.front().name,aq::DialogSize::Large};dialogState.open=true;}
  }
  if(showHud&&o.fixture=="shop"&&(o.uiVariant=="fish-placement"||o.uiVariant=="fish-placement-receipt")){aq::startFishPlacement(session.domain(),placement,"neonTetra");shopOpen=false;if(o.uiVariant=="fish-placement-receipt")aq::confirmFishPlacement(session,placement,{544,317});}
  std::optional<std::size_t> pressedFishCard;
  int pressedShopControl=-1;bool shopDragging=false,shopDragged=false;float shopDragX=0;SDL_FPoint shopDragStart{};
  auto render=[&](double seconds){
   canvas.begin();
    canvas.scene(session.domain(),session.interpolation(),seconds,showHud?care.tool():aq::Tool::Select,showHud?care.held():aq::FishId{},showHud,{},nullptr,0,false,showHud);
    if(showHud){
     const auto layout=aq::layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
     float x{},y{};const auto buttons=SDL_GetMouseState(&x,&y);
     const auto point=canvas.inputPoint(x,y);const auto hover=aq::hudHit(layout,point);
     if(shopOpen){const auto shop=aq::layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());aq::paintShop(canvas,session.domain(),shop,shopState,aq::shopControl(shop,point,shopState.category),pressedShopControl,&tankShop);}
     else {const auto rewardDisplay=care.rewards().display(session.domain());aq::paintHud(canvas,session.domain(),layout,hover,(buttons&SDL_BUTTON_LMASK)?hover:std::optional<aq::HudPart>{},&rewardDisplay);}
     if(!shopOpen){if(placement.active())aq::paintFishPlacement(canvas,session.domain(),placement,aq::layoutFishPlacement(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()));aq::paintPlacementReceipts(canvas,placement,layout.unit);}
     if(!shopOpen&&!switcher.open&&!dialogState.open&&!funds.open()&&!placement.active())care.paint();else canvas.cursor(aq::CursorKind::Arrow);
     if(!shopOpen&&!dialogState.open&&!funds.open())aq::paintTankSwitcher(canvas,session.domain(),layout,switcher,point);
     if(dialogState.open){const auto dialog=aq::layoutDialog(canvas.width(),canvas.height(),canvas.safeInsets(),dialogSpec);aq::paintDialog(canvas,dialog,dialogSpec,dialog.close.has(point.x,point.y),dialogState.closePressed);if(const auto* species=session.domain().content().find(shopFishDetails))aq::paintShopFishDetails(canvas,session.domain(),dialog,*species);else if(!shopNotice.empty()){
      canvas.text(shopNotice,dialog.content.x+dialog.content.w*.5f,dialog.content.y+(tankShopNotice?96:32)*dialog.unit,(tankShopNotice?36:28)*dialog.unit,aq::Color{20,74,84,255},true,dialog.content.w-32*dialog.unit,true,true);
      if(tankShopNotice){
       const auto& domain=session.domain();const auto tankId=domain.state().activeTank;const auto* tank=domain.tank(tankId);
       const std::string capacity=std::to_string(domain.living(tankId))+" / "+std::to_string(tank?tank->slots:0)+" slots used";
       canvas.text(capacity,dialog.content.x+dialog.content.w*.5f,dialog.content.y+32*dialog.unit,40*dialog.unit,aq::Color{20,74,84,255},true,dialog.content.w-32*dialog.unit,true,true);
       const auto button=tankShopButton(dialog);aq::shopTheme::panel(canvas,button,dialog.unit,aq::shopTheme::Surface::Buy,tankShopPressed);canvas.text("Tank Shop",button.x+button.w*.5f,button.y+20*dialog.unit,32*dialog.unit,aq::shopTheme::white,true,button.w-24*dialog.unit,true,true);}
     }}
     if(funds.open())aq::paintFundsDialog(canvas,aq::layoutFundsDialog(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),funds,point);
     if(!shopOpen)care.rewards().paint(canvas,layout);
    }
  };
  // Prepare the same menu painters used by clicks, including text and GPU
  // uploads. Present only the loader between steps, with normal quit/suspend
  // handling. Only the entry screens are prepared, not the entire catalog.
  if(showHud&&!aq::prepareMenus(canvas,session.domain(),[&](float progress,std::string_view stage){return loading(.10f+.80f*progress,stage);}))return 0;
  render(0);SDL_FlushRenderer(canvas.renderer());
  if(!loading(1,"Your reef is ready"))return 0;
  loadingSession=nullptr;
  const double startupMs=double(SDL_GetTicksNS()-startupStart)/1e6;
  if(!o.sequence.empty())std::filesystem::create_directories(o.sequence);bool running=true;std::uint64_t previous=SDL_GetTicksNS();double presentation=0,careFraction=0;int frame=0;std::vector<double> timings,intervals;timings.reserve(10000);intervals.reserve(10000);
  while(running){
   const auto frameStart=SDL_GetTicksNS();double elapsed=double(frameStart-previous)/1e9;previous=frameStart;presentation+=elapsed;aq::advanceFishPlacement(placement,elapsed);
   if(frame>=60&&!session.paused()&&!o.report.empty()){intervals.push_back(elapsed*1000);if(intervals.size()>36000)intervals.erase(intervals.begin(),intervals.begin()+18000);}
   SDL_Event e;while(SDL_PollEvent(&e)){
    if(showHud){
     int width{},height{};SDL_GetWindowSize(canvas.window(),&width,&height);
     if(!aq::normalizeHudPointer(hudPointer,e,float(width),float(height)))continue;
     if(funds.open()){
      SDL_FPoint point{};
      if(e.type==SDL_EVENT_MOUSE_MOTION)point=canvas.inputPoint(e.motion.x,e.motion.y);
      else if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)point=canvas.inputPoint(e.button.x,e.button.y);
      const auto action=aq::fundsDialogEvent(funds,aq::layoutFundsDialog(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),e,point);
      if(action==aq::FundsDialogEvent::OpenShop)fundsShopReturn.open(funds,shopOpen,shopState);
      if(action!=aq::FundsDialogEvent::Ignored)continue;
     }
     if(!shopOpen&&!dialogState.open&&!funds.open()&&!care.detailsOpen()){
      SDL_FPoint point{};
      if(e.type==SDL_EVENT_MOUSE_MOTION)point=canvas.inputPoint(e.motion.x,e.motion.y);
      else if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)point=canvas.inputPoint(e.button.x,e.button.y);
      if(aq::tankSwitcherEvent(session,switcher,aq::layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),e,point)){
       if(switcher.open){care.reset();if(placement.active())aq::cancelFishPlacement(placement);}
       if(const auto target=std::exchange(switcher.shopTarget,std::nullopt)){
        shopOpen=true;shopState={aq::ShopCategory::Tanks};tankShop={};
        aq::focusTankShop(tankShop,aq::layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),*target);
        care.reset();aq::cancelFishPlacement(placement);
       }
       continue;
      }
     }
     const bool menu=shopOpen||switcher.open||dialogState.open||funds.open();
     if(!menu&&placement.active()){
      const bool shortcut=e.type==SDL_EVENT_KEY_DOWN&&(e.key.key==SDLK_F||e.key.key==SDLK_S);
      std::optional<aq::HudPart> hit;
      if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT)hit=aq::hudHit(aq::layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),canvas.inputPoint(e.button.x,e.button.y));
      if(shortcut||hit==aq::HudPart::Food||hit==aq::HudPart::Rehome)aq::cancelFishPlacement(placement);
     }
     if(care.event(e,hudPointer.touch,menu||placement.active()))continue;
    }
    if(e.type==SDL_EVENT_QUIT){
     if(session.checkpoint(wallNow()))running=false;
     else{
      const SDL_MessageBoxButtonData buttons[]={{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT|SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,"Keep open"},{0,1,"Quit without saving"}};
      const SDL_MessageBoxData warning{SDL_MESSAGEBOX_WARNING,canvas.window(),"Progress could not be saved","Keep the game open to retry saving. Quitting now may lose your recent progress.",2,buttons,nullptr};
      int choice=0;if(SDL_ShowMessageBox(&warning,&choice)&&choice==1)running=false;
     }
    }
    if(showHud&&dialogState.open){
     SDL_FPoint point{};if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)point=canvas.inputPoint(e.button.x,e.button.y);
     const auto dialog=aq::layoutDialog(canvas.width(),canvas.height(),canvas.safeInsets(),dialogSpec);
     if(tankShopNotice&&!shopNotice.empty()){
      const bool over=tankShopButton(dialog).has(point.x,point.y);
      if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT)tankShopPressed=over;
      if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND)tankShopPressed=false;
      if(e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.button==SDL_BUTTON_LEFT){const bool activate=tankShopPressed&&over;tankShopPressed=false;if(activate){dialogState.open=false;shopNotice.clear();tankShopNotice=false;shopOpen=true;switcher.open=false;shopState={aq::ShopCategory::Tanks};tankShop={};continue;}}
     }
     if(aq::dialogEvent(dialogState,dialog,e,point))continue;
    }
    if(showHud&&shopOpen&&shopState.category==aq::ShopCategory::Tanks&&!dialogState.open&&!funds.open()){
     SDL_FPoint point{};
     if(e.type==SDL_EVENT_MOUSE_MOTION)point=canvas.inputPoint(e.motion.x,e.motion.y);
     else if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)point=canvas.inputPoint(e.button.x,e.button.y);
     else if(e.type==SDL_EVENT_MOUSE_WHEEL)point=canvas.inputPoint(e.wheel.mouse_x,e.wheel.mouse_y);
     const bool handled=aq::tankShopEvent(session,tankShop,aq::layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),e,point);
     if(const auto missing=std::exchange(tankShop.shortfall,std::nullopt))showFunds({.error=aq::Error::Funds,.shortfall=*missing});
     if(handled)continue;
    }
    if(showHud&&placement.active()&&!shopOpen&&!switcher.open){
     SDL_FPoint point{};
     if(e.type==SDL_EVENT_MOUSE_MOTION)point=canvas.inputPoint(e.motion.x,e.motion.y);
     else if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)point=canvas.inputPoint(e.button.x,e.button.y);
     const auto outcome=aq::fishPlacementEvent(session,placement,aq::layoutFishPlacement(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),e,point);
     (void)outcome;
     if(const auto missing=std::exchange(placement.shortfall,std::nullopt)){showFunds({.error=aq::Error::Funds,.shortfall=*missing});continue;}
     const bool overHud=aq::hudHit(aq::layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),point).has_value();
     if(!overHud&&(e.type==SDL_EVENT_MOUSE_MOTION||e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP||e.type==SDL_EVENT_MOUSE_WHEEL||e.type==SDL_EVENT_KEY_DOWN||e.type==SDL_EVENT_KEY_UP))continue;
    }
    if(showHud){
     if(e.type==SDL_EVENT_KEY_DOWN&&e.key.key==SDLK_ESCAPE){fundsShopReturn.close(shopOpen,shopState);switcher.open=false;shopPointerDown=shopDragging=false;pressedShopControl=-1;}
     if(shopOpen&&e.type==SDL_EVENT_MOUSE_WHEEL){float delta=e.wheel.x!=0?e.wheel.x:-e.wheel.y;if(e.wheel.direction==SDL_MOUSEWHEEL_FLIPPED)delta=-delta;aq::scrollShop(shopState,delta,aq::shopItems(session.domain(),shopState).size());}
     if(shopOpen&&shopDragging&&e.type==SDL_EVENT_MOUSE_MOTION){
      const auto point=canvas.inputPoint(e.motion.x,e.motion.y);const auto shop=aq::layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
      if(std::hypot(point.x-shopDragStart.x,point.y-shopDragStart.y)>4*canvas.minimumTouchSize()/44)shopDragged=true;
      if(shopDragged){aq::scrollShop(shopState,(shopDragX-point.x)/(aq::shopCardBounds(shop,shopState,1).x-aq::shopCardBounds(shop,shopState,0).x),aq::shopItems(session.domain(),shopState).size());shopDragX=point.x;}
     }
     if((e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)&&e.button.button==SDL_BUTTON_LEFT){
      const auto point=canvas.inputPoint(e.button.x,e.button.y);
      const auto shop=aq::layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());const int control=aq::shopControl(shop,point,shopState.category);
      const auto hit=aq::hudHit(aq::layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),point);
      const bool onShop=hit==aq::HudPart::Shop;
      const bool onCurrencyButton=!shopOpen&&!switcher.open&&(hit==aq::HudPart::CoinPlus||hit==aq::HudPart::PearlPlus);
      const bool onDialogButton=!shopOpen&&!switcher.open&&(hit==aq::HudPart::Bag||hit==aq::HudPart::Projects||hit==aq::HudPart::Rewards||hit==aq::HudPart::Settings);
      if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){pressedCurrencyButton=onCurrencyButton?hit:std::nullopt;pressedDialogButton=onDialogButton?hit:std::nullopt;pressedShopControl=shopOpen?control:-1;shopPointerDown=!shopOpen&&!switcher.open&&onShop;shopDragging=shopOpen&&shopState.category!=aq::ShopCategory::Tanks&&shop.body.has(point.x,point.y);shopDragX=point.x;shopDragStart=point;shopDragged=false;pressedFishCard=shopOpen&&(shopState.category==aq::ShopCategory::Fish||shopState.category==aq::ShopCategory::Environment||shopState.category==aq::ShopCategory::Treasure)?aq::shopCardAt(shop,shopState,aq::shopItems(session.domain(),shopState).size(),point):std::nullopt;pressedFishInfo=shopState.category==aq::ShopCategory::Fish&&pressedFishCard&&aq::shopInfoBounds(aq::shopCardBounds(shop,shopState,*pressedFishCard),shop.unit).has(point.x,point.y);}
      else{if(onCurrencyButton&&pressedCurrencyButton==hit){shopState={aq::ShopCategory::Treasure,hit==aq::HudPart::CoinPlus?0:1,0};shopOpen=true;}else if(onDialogButton&&pressedDialogButton==hit){shopFishDetails.clear();shopNotice.clear();dialogSpec={hit==aq::HudPart::Bag?"Bag":hit==aq::HudPart::Projects?"Projects":hit==aq::HudPart::Rewards?"Rewards":"Settings",aq::DialogSize::Large};dialogState={true,false};}else if(shopOpen&&pressedFishCard&&!shopDragged){
       const auto items=aq::shopItems(session.domain(),shopState);
       const auto released=aq::shopCardAt(shop,shopState,items.size(),point);
       if(released==pressedFishCard&&!items[*released].treasureId.empty()){
        const auto& item=items[*released];shopFishDetails.clear();tankShopNotice=false;shopNotice=item.detail+" for "+item.price+" USD";dialogSpec={"Purchases coming soon",aq::DialogSize::Small};dialogState={true,false};
       }else if(released==pressedFishCard&&!items[*released].environmentId.empty()){
        const auto& id=items[*released].environmentId;const auto& domain=session.domain();
        const auto result=session.command({.action=domain.ownsEnvironment(id)?aq::Action::EquipEnvironment:aq::Action::PurchaseEnvironment,.tank=domain.state().activeTank,.key=id});
        if(!result&&!showFunds(result)){shopFishDetails.clear();tankShopNotice=false;shopNotice=result.message.empty()?aq::errorText(result.error):result.message;dialogSpec={"Cannot apply background",aq::DialogSize::Small};dialogState={true,false};}
       }else if(released==pressedFishCard&&items[*released].fish){
        const auto& item=items[*released];const bool info=aq::shopInfoBounds(aq::shopCardBounds(shop,shopState,*released),shop.unit).has(point.x,point.y);
        if(info&&pressedFishInfo){shopNotice.clear();shopFishDetails=item.fish->id;dialogSpec={item.name,aq::DialogSize::Large};dialogState={true,false};}
        else if(!info&&!pressedFishInfo){
         const auto result=aq::startFishPlacement(session.domain(),placement,item.fish->id);
         if(result)shopOpen=false;
         else if(!showFunds(result)){shopFishDetails.clear();tankShopNotice=result.error==aq::Error::Full;tankShopPressed=false;shopNotice=tankShopNotice?"Make room for more fish.":(result.message.empty()?aq::errorText(result.error):result.message);dialogSpec={tankShopNotice?"Tank full!":"Cannot place fish",aq::DialogSize::Small};dialogState={true,false};}
        }
       }
      }else if(shopOpen&&control>=0&&pressedShopControl==control){if(control==6)fundsShopReturn.close(shopOpen,shopState);else {if(control<4||control==7||control==8)fundsShopReturn.previous.reset();aq::activateShopControl(shopState,control);tankShop={};}}else if(!shopOpen&&!switcher.open&&shopPointerDown&&onShop)shopOpen=true;shopPointerDown=shopDragging=false;pressedShopControl=-1;pressedFishCard.reset();pressedDialogButton.reset();pressedCurrencyButton.reset();}
     }
     if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST||e.type==SDL_EVENT_WILL_ENTER_BACKGROUND||e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET){pressedFishCard.reset();pressedDialogButton.reset();pressedCurrencyButton.reset();shopPointerDown=shopDragging=false;pressedShopControl=-1;dialogState.closePressed=dialogState.backdropPressed=false;}
    }
    if(e.type==SDL_EVENT_WILL_ENTER_BACKGROUND)session.suspend(wallNow());
    else if(e.type==SDL_EVENT_DID_ENTER_FOREGROUND)session.resume(wallNow());
   }
   if(session.paused()){SDL_Delay(50);continue;}
   careFraction+=elapsed*1000.*o.warp;auto careMs=static_cast<aq::Millis>(careFraction);careFraction-=double(careMs);if(!o.still)session.update(careMs,wallNow(),showHud?care.tool():aq::Tool::Select,showHud?care.held():aq::FishId{});if(showHud)care.advance(elapsed,!shopOpen);render(presentation);
   if(!o.sequence.empty()){std::string number=std::to_string(frame);number=std::string(6-number.size(),'0')+number;if(!canvas.capture(o.sequence/(number+".png")))throw std::runtime_error("Frame capture failed");}
   if(o.frames>0&&frame+1>=o.frames){if(!o.capture.empty()&&!canvas.capture(o.capture,o.captureWindow))throw std::runtime_error("Screenshot capture failed");running=false;}
   const auto beforePresent=SDL_GetTicksNS();timings.push_back(double(beforePresent-frameStart)/1e6);if(timings.size()>36000)timings.erase(timings.begin(),timings.begin()+18000);canvas.present();++frame;
  }
  const bool saved=session.checkpoint(wallNow());if(!saved)std::cerr<<"Fishius: "<<session.status()<<'\n';if(!o.report.empty()){
   auto sorted=timings;std::sort(sorted.begin(),sorted.end());auto percentile=[&](double p){return sorted.empty()?0:sorted[std::min(sorted.size()-1,static_cast<std::size_t>(p*double(sorted.size()-1)))];};
   int w{},h{},pw{},ph{};SDL_GetWindowSize(canvas.window(),&w,&h);SDL_GetWindowSizeInPixels(canvas.window(),&pw,&ph);
   aq::Json report={{"renderer",SDL_GetRendererName(canvas.renderer())},{"frames",frame},{"scenario",o.fixture},{"window_width",w},{"window_height",h},{"pixel_width",pw},{"pixel_height",ph},{"metric","CPU update and draw submission milliseconds; excludes vsync wait; includes capture when enabled"},{"p50_ms",percentile(.5)},{"p95_ms",percentile(.95)},{"p99_ms",percentile(.99)},{"max_ms",sorted.empty()?0:sorted.back()},{"mobile_device_tested",false}};
   report["scene_only"]=o.sceneOnly;report["hud_layout"]=showHud;report["interface"]=showHud?"clay":"scene";report["startup_ms"]=startupMs;
   sorted=intervals;std::sort(sorted.begin(),sorted.end());report["frame_intervals"]={{"metric","Milliseconds between frames, including presentation wait; excludes first 60 frames"},{"samples",sorted.size()},{"p50_ms",percentile(.5)},{"p95_ms",percentile(.95)},{"p99_ms",percentile(.99)}};
   std::ofstream out(o.report);out<<report.dump(2);
  }
  return saved?0:1;
 }catch(const std::exception& e){std::cerr<<"Fishius: "<<e.what()<<'\n';SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Fishius could not continue",e.what(),nullptr);return 1;}
}
