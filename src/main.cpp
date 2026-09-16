#include <cmath>
#include <cstdlib>
#include "aquarium/view.hpp"
#include "aquarium/hud.hpp"
#include "aquarium/hud_dialog.hpp"
#include "aquarium/hud_placement.hpp"
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace {
aq::Millis wallNow(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
struct Options {int w{1088},h{635},frames{};bool software{},fresh{},still{},captureWindow{},sceneOnly{},hudLayout{};double warp{1};std::string hudDialog,dialogTitle{"Dialog title"};float dialogWidth{960},dialogHeight{640};std::string fixture,uiScreen,uiVariant{"coins"};std::filesystem::path assets,save,capture,report,sequence,loadingCapture,uiProject;};
Options options(int argc,char** argv){
 Options o;for(int i=1;i<argc;++i){std::string s=argv[i];auto arg=[&](){if(i+1>=argc)throw std::runtime_error("Missing value for "+s);return std::string(argv[++i]);};if(s=="--width")o.w=std::stoi(arg());else if(s=="--height")o.h=std::stoi(arg());else if(s=="--frames")o.frames=std::stoi(arg());else if(s=="--warp")o.warp=std::stod(arg());else if(s=="--fixture")o.fixture=arg();else if(s=="--ui-project")o.uiProject=arg();else if(s=="--ui-screen")o.uiScreen=arg();else if(s=="--ui-variant")o.uiVariant=arg();else if(s=="--assets")o.assets=arg();else if(s=="--save")o.save=arg();else if(s=="--capture")o.capture=arg();else if(s=="--loading-capture")o.loadingCapture=arg();else if(s=="--report")o.report=arg();else if(s=="--sequence")o.sequence=arg();else if(s=="--software")o.software=true;else if(s=="--fresh")o.fresh=true;else if(s=="--still")o.still=true;else if(s=="--capture-window")o.captureWindow=true;else if(s=="--scene-only")o.sceneOnly=true;else if(s=="--hud-dialog"){o.hudDialog=arg();o.hudLayout=true;}else if(s=="--dialog-title")o.dialogTitle=arg();else if(s=="--dialog-width")o.dialogWidth=std::stof(arg());else if(s=="--dialog-height")o.dialogHeight=std::stof(arg());else if(s=="--hud-layout")o.hudLayout=true;else if(s=="--help"){std::cout<<"Fishius\n--assets PATH --save PATH --width N --height N\n--fixture aquarium|shop|tanks|collection|settings|inventory|care|performance|details\n--ui-project JSON --ui-screen ID --ui-variant NAME\n--scene-only  Show the aquarium without background artwork or UI controls.\n--hud-layout  Preview the responsive Clay HUD with placeholder artwork.\n--hud-dialog small|medium|large|custom --dialog-width N --dialog-height N --dialog-title TEXT\n--frames N --capture PNG --loading-capture PNG --sequence DIR --report JSON --software --warp N --fresh --still --capture-window\nReview fixtures and --fresh never overwrite your ordinary save.\n";std::exit(0);}else throw std::runtime_error("Unknown option "+s);}
 if(o.w<320||o.h<240||o.w>8192||o.h>8192||!std::isfinite(o.warp)||o.warp<1||o.warp>10000)throw std::runtime_error("Invalid viewport or time scale");
 return o;
}
}
int main(int argc,char** argv){
 try{
  auto o=options(argc,argv);if(o.assets.empty()){const char* base=SDL_GetBasePath();o.assets=base?std::filesystem::path(base)/"assets":std::filesystem::path("assets");if(!std::filesystem::exists(o.assets/"content.json"))o.assets="assets";}
  aq::Canvas canvas(o.assets,o.w,o.h,o.software);
  const bool bareView=o.sceneOnly||o.hudLayout;
  if(bareView)SDL_SetWindowTitle(canvas.window(),o.hudLayout?"Fishius | Clay HUD layout":"Fishius | Aquarium view");
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
   canvas.loadingScreen(progress,stage,double(SDL_GetTicksNS()-startupStart)/1e9,artwork&&!bareView,reduced);
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
  const bool review=!o.fixture.empty()||o.fresh||!o.uiProject.empty()||!o.uiScreen.empty();aq::Session session(std::move(content),o.save,wallNow(),review);aq::View view(canvas,session,o.uiProject);if(!o.fixture.empty()){session.domain().fixture(o.fixture);view.fixture(o.fixture);}
  if(!o.uiScreen.empty())view.previewUiScreen(o.uiScreen,o.uiVariant,{{{"amount",o.uiVariant=="pearls"?"12":"240"}}});
  loadingSession=&session;reduced=session.domain().state().settings.reducedMotion;
  const auto prepareStart=SDL_GetTicksNS();
  if(!bareView&&!view.prepareMenus([&](float progress,std::string_view stage){return loading(.08f+.9f*progress,stage);}))return 0;
  const double menuPreparationMs=double(SDL_GetTicksNS()-prepareStart)/1e6;
  // Render the actual starting scene behind the loader to initialize its
  // remaining effects before gameplay and before starting the frame clock.
  aq::DialogSpec dialogSpec{o.dialogTitle};
  if(o.hudDialog=="small")dialogSpec.size=aq::DialogSize::Small;
  else if(o.hudDialog=="medium")dialogSpec.size=aq::DialogSize::Medium;
  else if(o.hudDialog=="large")dialogSpec.size=aq::DialogSize::Large;
  else if(o.hudDialog=="custom"){dialogSpec.size=aq::DialogSize::Custom;dialogSpec.customWidth=o.dialogWidth;dialogSpec.customHeight=o.dialogHeight;}
  else if(!o.hudDialog.empty())throw std::runtime_error("Unknown dialog size");
  aq::DialogState dialogState{!o.hudDialog.empty(),false};
  bool shopOpen=o.hudLayout&&o.fixture=="shop";
  bool tankOpen=o.hudLayout&&o.fixture=="tanks";
  bool shopPointerDown=false,tankPointerDown=false;
  std::optional<aq::HudPart> pressedDialogButton,pressedCurrencyButton;
  aq::ShopState shopState;
  if(o.hudLayout&&o.fixture=="shop"){
   if(o.uiVariant=="fish")shopState.category=aq::ShopCategory::Fish;
   else if(o.uiVariant=="plants")shopState.category=aq::ShopCategory::Plants;
   else if(o.uiVariant=="decorations")shopState.category=aq::ShopCategory::Decorations;
   else if(o.uiVariant=="pearls")shopState.subtab=1;
  }
  std::string shopFishDetails,shopNotice;
  aq::FishPlacement placement;
  bool pressedFishInfo=false;
  if(o.hudLayout&&o.fixture=="shop"&&o.uiVariant=="fish-details"){
   shopState.category=aq::ShopCategory::Fish;
   const auto items=aq::shopItems(session.domain(),shopState);
   if(!items.empty()&&items.front().fish){shopFishDetails=items.front().fish->id;dialogSpec={items.front().name,aq::DialogSize::Medium};dialogState.open=true;}
  }
  if(o.hudLayout&&o.fixture=="shop"&&(o.uiVariant=="fish-placement"||o.uiVariant=="fish-placement-receipt")){aq::startFishPlacement(session.domain(),placement,"neonTetra");shopOpen=false;if(o.uiVariant=="fish-placement-receipt")aq::confirmFishPlacement(session,placement,{544,317});}
  std::optional<std::size_t> pressedFishCard;
  int pressedShopControl=-1;bool shopDragging=false,shopDragged=false;float shopDragX=0;SDL_FPoint shopDragStart{};
  auto render=[&](double seconds){
   if(bareView){
    canvas.begin();
    canvas.scene(session.domain(),session.interpolation(),seconds,aq::Tool::Select,{},false,{},nullptr,0,false,false);
    if(o.hudLayout){
     const auto layout=aq::layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
     float x{},y{};const auto buttons=SDL_GetMouseState(&x,&y);
     const auto point=canvas.inputPoint(x,y);const auto hover=aq::hudHit(layout,point);
     if(tankOpen){const auto menu=aq::layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());aq::paintTankMenu(canvas,session.domain(),menu,menu.close.has(point.x,point.y)?6:-1,pressedShopControl);}
     else if(shopOpen){const auto shop=aq::layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());aq::paintShop(canvas,session.domain(),shop,shopState,aq::shopControl(shop,point),pressedShopControl);}
     else aq::paintHud(canvas,session.domain(),layout,hover,(buttons&SDL_BUTTON_LMASK)?hover:std::optional<aq::HudPart>{});
     if(!shopOpen&&!tankOpen){if(placement.active())aq::paintFishPlacement(canvas,session.domain(),placement,aq::layoutFishPlacement(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()));aq::paintPlacementReceipts(canvas,placement,layout.unit);}
     if(dialogState.open){const auto dialog=aq::layoutDialog(canvas.width(),canvas.height(),canvas.safeInsets(),dialogSpec);aq::paintDialog(canvas,dialog,dialogSpec,dialog.close.has(point.x,point.y),dialogState.closePressed);if(const auto* species=session.domain().content().find(shopFishDetails))aq::paintShopFishDetails(canvas,session.domain(),dialog,*species);else if(!shopNotice.empty())canvas.text(shopNotice,dialog.content.x+dialog.content.w*.5f,dialog.content.y+32*dialog.unit,28*dialog.unit,aq::Color{20,74,84,255},true,dialog.content.w,true,true);}
    }
   }else view.render(seconds);
  };
  render(0);SDL_FlushRenderer(canvas.renderer());
  if(!loading(1,"Your reef is ready"))return 0;
  loadingSession=nullptr;
  const double startupMs=double(SDL_GetTicksNS()-startupStart)/1e6;
  if(!o.sequence.empty())std::filesystem::create_directories(o.sequence);bool running=true;std::uint64_t previous=SDL_GetTicksNS();double presentation=0,careFraction=0;int frame=0;std::vector<double> timings,intervals;timings.reserve(10000);intervals.reserve(10000);
  while(running){
   const auto frameStart=SDL_GetTicksNS();double elapsed=double(frameStart-previous)/1e9;previous=frameStart;presentation+=elapsed;aq::advanceFishPlacement(placement,elapsed);
   if(frame>=60&&!session.paused()&&!o.report.empty()){intervals.push_back(elapsed*1000);if(intervals.size()>36000)intervals.erase(intervals.begin(),intervals.begin()+18000);}
   SDL_Event e;while(SDL_PollEvent(&e)){
    if(e.type==SDL_EVENT_QUIT){
     if(session.checkpoint(wallNow()))running=false;
     else{
      const SDL_MessageBoxButtonData buttons[]={{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT|SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,"Keep open"},{0,1,"Quit without saving"}};
      const SDL_MessageBoxData warning{SDL_MESSAGEBOX_WARNING,canvas.window(),"Progress could not be saved","Keep the game open to retry saving. Quitting now may lose your recent progress.",2,buttons,nullptr};
      int choice=0;if(SDL_ShowMessageBox(&warning,&choice)&&choice==1)running=false;
     }
    }
    if(o.hudLayout&&dialogState.open){
     SDL_FPoint point{};if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)point=canvas.inputPoint(e.button.x,e.button.y);
     if(aq::dialogEvent(dialogState,aq::layoutDialog(canvas.width(),canvas.height(),canvas.safeInsets(),dialogSpec),e,point))continue;
    }
    if(o.hudLayout&&placement.active()&&!shopOpen&&!tankOpen){
     SDL_FPoint point{};
     if(e.type==SDL_EVENT_MOUSE_MOTION)point=canvas.inputPoint(e.motion.x,e.motion.y);
     else if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)point=canvas.inputPoint(e.button.x,e.button.y);
     const auto outcome=aq::fishPlacementEvent(session,placement,aq::layoutFishPlacement(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),e,point);
     (void)outcome;
     const bool overHud=aq::hudHit(aq::layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),point).has_value();
     if(!overHud&&(e.type==SDL_EVENT_MOUSE_MOTION||e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP||e.type==SDL_EVENT_MOUSE_WHEEL||e.type==SDL_EVENT_KEY_DOWN||e.type==SDL_EVENT_KEY_UP))continue;
    }
    if(o.hudLayout){
     if(e.type==SDL_EVENT_KEY_DOWN&&e.key.key==SDLK_ESCAPE){shopOpen=tankOpen=false;shopPointerDown=tankPointerDown=shopDragging=false;pressedShopControl=-1;}
     if(shopOpen&&e.type==SDL_EVENT_MOUSE_WHEEL){float delta=e.wheel.x!=0?e.wheel.x:-e.wheel.y;if(e.wheel.direction==SDL_MOUSEWHEEL_FLIPPED)delta=-delta;aq::scrollShop(shopState,delta,aq::shopItems(session.domain(),shopState).size());}
     if(shopOpen&&shopDragging&&e.type==SDL_EVENT_MOUSE_MOTION){
      const auto point=canvas.inputPoint(e.motion.x,e.motion.y);const auto shop=aq::layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
      if(std::hypot(point.x-shopDragStart.x,point.y-shopDragStart.y)>4*canvas.minimumTouchSize()/44)shopDragged=true;
      if(shopDragged){aq::scrollShop(shopState,(shopDragX-point.x)/(shop.cards[1].x-shop.cards[0].x),aq::shopItems(session.domain(),shopState).size());shopDragX=point.x;}
     }
     if((e.type==SDL_EVENT_MOUSE_BUTTON_DOWN||e.type==SDL_EVENT_MOUSE_BUTTON_UP)&&e.button.button==SDL_BUTTON_LEFT){
      const auto point=canvas.inputPoint(e.button.x,e.button.y);
      const auto shop=aq::layoutShop(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());const int control=tankOpen?(shop.close.has(point.x,point.y)?6:-1):aq::shopControl(shop,point);
      const auto hit=aq::hudHit(aq::layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize()),point);
      const bool onShop=hit==aq::HudPart::Shop,onTank=hit==aq::HudPart::Tank;
      const bool onCurrencyButton=!shopOpen&&!tankOpen&&(hit==aq::HudPart::CoinPlus||hit==aq::HudPart::PearlPlus);
      const bool onDialogButton=!shopOpen&&!tankOpen&&(hit==aq::HudPart::Bag||hit==aq::HudPart::Projects||hit==aq::HudPart::Rewards||hit==aq::HudPart::Settings);
      if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){pressedCurrencyButton=onCurrencyButton?hit:std::nullopt;pressedDialogButton=onDialogButton?hit:std::nullopt;pressedShopControl=(shopOpen||tankOpen)?control:-1;shopPointerDown=!shopOpen&&!tankOpen&&onShop;tankPointerDown=!shopOpen&&!tankOpen&&onTank;shopDragging=shopOpen&&shop.body.has(point.x,point.y);shopDragX=point.x;shopDragStart=point;shopDragged=false;pressedFishCard=shopOpen&&shopState.category==aq::ShopCategory::Fish?aq::shopCardAt(shop,shopState,aq::shopItems(session.domain(),shopState).size(),point):std::nullopt;pressedFishInfo=pressedFishCard&&aq::shopInfoBounds(aq::shopCardBounds(shop,shopState,*pressedFishCard),shop.unit).has(point.x,point.y);}
      else{if(onCurrencyButton&&pressedCurrencyButton==hit){shopState={aq::ShopCategory::Treasure,hit==aq::HudPart::CoinPlus?0:1,0};shopOpen=true;}else if(onDialogButton&&pressedDialogButton==hit){shopFishDetails.clear();shopNotice.clear();dialogSpec={hit==aq::HudPart::Bag?"Bag":hit==aq::HudPart::Projects?"Projects":hit==aq::HudPart::Rewards?"Rewards":"Settings",aq::DialogSize::Large};dialogState={true,false};}else if(shopOpen&&pressedFishCard&&!shopDragged){
       const auto items=aq::shopItems(session.domain(),shopState);
       const auto released=aq::shopCardAt(shop,shopState,items.size(),point);
       if(released==pressedFishCard&&items[*released].fish){
        const auto& item=items[*released];const bool info=aq::shopInfoBounds(aq::shopCardBounds(shop,shopState,*released),shop.unit).has(point.x,point.y);
        if(info&&pressedFishInfo){shopNotice.clear();shopFishDetails=item.fish->id;dialogSpec={item.name,aq::DialogSize::Medium};dialogState={true,false};}
        else if(!info&&!pressedFishInfo){
         const auto result=aq::startFishPlacement(session.domain(),placement,item.fish->id);
         if(result)shopOpen=false;
         else{shopFishDetails.clear();shopNotice=result.message.empty()?aq::errorText(result.error):result.message;dialogSpec={"Cannot place fish",aq::DialogSize::Small};dialogState={true,false};}
        }
       }
      }else if((shopOpen||tankOpen)&&control>=0&&pressedShopControl==control){if(control==6)shopOpen=tankOpen=false;else aq::activateShopControl(shopState,control);}else if(!shopOpen&&!tankOpen&&shopPointerDown&&onShop)shopOpen=true;else if(!shopOpen&&!tankOpen&&tankPointerDown&&onTank)tankOpen=true;shopPointerDown=tankPointerDown=shopDragging=false;pressedShopControl=-1;pressedFishCard.reset();pressedDialogButton.reset();pressedCurrencyButton.reset();}
     }
     if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST){pressedFishCard.reset();pressedDialogButton.reset();pressedCurrencyButton.reset();shopPointerDown=tankPointerDown=shopDragging=false;pressedShopControl=-1;}
    }
    if(!bareView)view.event(e,presentation);if(e.type==SDL_EVENT_WILL_ENTER_BACKGROUND){view.cancelGesture();session.suspend(wallNow());}else if(e.type==SDL_EVENT_DID_ENTER_FOREGROUND)session.resume(wallNow());else if(e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET){view.cancelGesture();}
   }
   if(session.paused()){SDL_Delay(50);continue;}
   careFraction+=elapsed*1000.*o.warp;auto careMs=static_cast<aq::Millis>(careFraction);careFraction-=double(careMs);if(!o.still)session.update(careMs,wallNow(),bareView?aq::Tool::Select:view.tool(),bareView?aq::FishId{}:view.held());render(presentation);
   if(!o.sequence.empty()){std::string number=std::to_string(frame);number=std::string(6-number.size(),'0')+number;if(!canvas.capture(o.sequence/(number+".png")))throw std::runtime_error("Frame capture failed");}
   if(o.frames>0&&frame+1>=o.frames){if(!o.capture.empty()&&!canvas.capture(o.capture,o.captureWindow))throw std::runtime_error("Screenshot capture failed");running=false;}
   const auto beforePresent=SDL_GetTicksNS();timings.push_back(double(beforePresent-frameStart)/1e6);if(timings.size()>36000)timings.erase(timings.begin(),timings.begin()+18000);canvas.present();++frame;
  }
  const bool saved=session.checkpoint(wallNow());if(!saved)std::cerr<<"Fishius: "<<session.status()<<'\n';if(!o.report.empty()){
   auto sorted=timings;std::sort(sorted.begin(),sorted.end());auto percentile=[&](double p){return sorted.empty()?0:sorted[std::min(sorted.size()-1,static_cast<std::size_t>(p*double(sorted.size()-1)))];};
   int w{},h{},pw{},ph{};SDL_GetWindowSize(canvas.window(),&w,&h);SDL_GetWindowSizeInPixels(canvas.window(),&pw,&ph);
   aq::Json report={{"renderer",SDL_GetRendererName(canvas.renderer())},{"frames",frame},{"scenario",o.fixture},{"window_width",w},{"window_height",h},{"pixel_width",pw},{"pixel_height",ph},{"metric","CPU update and draw submission milliseconds; excludes vsync wait; includes capture when enabled"},{"p50_ms",percentile(.5)},{"p95_ms",percentile(.95)},{"p99_ms",percentile(.99)},{"max_ms",sorted.empty()?0:sorted.back()},{"mobile_device_tested",false}};
   report["scene_only"]=o.sceneOnly;report["hud_layout"]=o.hudLayout;report["menu_preparation_ms"]=menuPreparationMs;report["startup_ms"]=startupMs;
   sorted=intervals;std::sort(sorted.begin(),sorted.end());report["frame_intervals"]={{"metric","Milliseconds between frames, including presentation wait; excludes first 60 frames"},{"samples",sorted.size()},{"p50_ms",percentile(.5)},{"p95_ms",percentile(.95)},{"p99_ms",percentile(.99)}};
   std::ofstream out(o.report);out<<report.dump(2);
  }
  return saved?0:1;
 }catch(const std::exception& e){std::cerr<<"Fishius: "<<e.what()<<'\n';SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Fishius could not continue",e.what(),nullptr);return 1;}
}
