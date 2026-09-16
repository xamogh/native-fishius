#include "aquarium/ui_renderer.hpp"
#include <algorithm>
#include <stdexcept>

namespace aq {
const UiProject* View::uiProject()const {
 if(dialogOverride_)return nullptr;
 if(uiOverride_)return &*uiOverride_;
 return uiDocument_&&uiDocument_->available()?&uiDocument_->project():nullptr;
}
std::string View::uiRevision()const {auto* p=uiProject();return p?uiFingerprint(*p):"legacy";}
void View::pollUi(){
 if(uiDocument_&&!uiOverride_)uiDocument_->poll();
 if(!uiAck_.empty()&&uiProject()){
  const auto& preview=uiDocument_->preview();
  if(!preview.empty()&&preview.dump()!=uiPreviewSpec_)try{
   const auto screen=preview.value("screen",std::string("general-dialog")),variant=preview.value("variant",std::string("coins"));
   const int width=std::clamp(preview.value("width",1088),320,2560),height=std::clamp(preview.value("height",635),240,1600),density=std::clamp(preview.value("density",2),1,2);
   SDL_SetWindowSize(canvas_.window(),width,height);canvas_.previewViewport(PreviewViewport{width,height,density,preview.value("safe",false)?Insets{32,0,32,16}:Insets{}});
   if(preview.value("flow",false))previewPurchase(preview.value("level",1),preview.value("coins",Amount{}),preview.value("pearls",Amount{}),preview.value("tank",1),preview.value("owned",true),preview.value("upgrades",0));
   else{uiProject()->screen(screen);UiBindings bindings;bindings.values=preview.value("values",std::map<std::string,std::string>{});previewUiScreen(screen,variant,bindings,preview.value("pressed",false),preview.value("disabled",false),preview.value("elapsed",2.));}
   uiPreviewSpec_=preview.dump();uiAckRevision_.clear();
  }catch(const std::exception&){}

  auto revision=uiRevision();if(revision!=uiAckRevision_)try{writeUiJson(uiAck_,{{"revision",revision},{"screen",uiPreviewScreen_}});uiAckRevision_=revision;}catch(const std::exception&){}
 }
}
UiRenderResult View::drawUi(std::string_view screen,std::string_view variant,Rect bounds,const UiBindings& bindings,const std::map<std::string,Button>& actions){
 UiPaint paint;paint.pressed=uiPreviewPressed_;paint.disabled=uiPreviewDisabled_;
 paint.buttonVisual=[&](std::string_view action,Rect r,float factor){
  auto it=actions.find(std::string(action));const float progress=uiPreviewPressed_?1.f:it==actions.end()?0.f:std::clamp((1-buttonScale(it->second.id))/.06f,0.f,1.f);
  const float scale=1-(1-factor)*progress;return Rect{r.x+r.w*(1-scale)*.5f,r.y+r.h*(1-scale)*.5f,r.w*scale,r.h*scale};
 };
 paint.button=[&](std::string_view action,Rect r){auto it=actions.find(std::string(action));if(it!=actions.end())touchTarget(it->second.id,r,it->second.action);};
 auto result=renderUiScreen(canvas_,*uiProject(),screen,variant,bounds,bindings,paint);uiLayout_=std::make_shared<UiRenderResult>(result);return result;
}
Rect View::uiMessageDialog(const MessageDialogContent& content,std::string_view actionId,std::string_view closeId,std::function<void()> action,std::function<void()> close){
 const bool pearl=fundsCurrency_?*fundsCurrency_==Currency::Pearls:fundsDialog_&&fundsDialog_->pearls>0&&!fundsDialog_->coins;
 UiBindings bindings;bindings.values={{"title",content.title},{"illustration",content.illustration},{"extra",content.extra},{"detail",content.detail},{"actionLabel",content.actionLabel},{"shopIcon",content.actionLabel=="Open Shop"?"1":""}};
 for(const auto& run:content.message)bindings.rich["message"].push_back({run.text,run.emphasis});
 // Keep the second line in normal flow when a level or second currency is shown.
 auto result=drawUi("general-dialog",pearl?"pearls":"coins",uiScreenBounds(canvas_,uiProject()->screen("general-dialog")),bindings,
  {{"primary",{std::string(actionId),{},std::move(action)}},{"close",{std::string(closeId),{},std::move(close)}}});
 const std::array<std::string,8> ids{"frame","title","illustration","message","extra","detail","primary","close"};
 for(const auto& e:result.elements)if(e.owner=="@dialog")for(std::size_t i=0;i<ids.size();++i)if(e.id==ids[i])dialogLayout_.elements[i]={e.box.x,e.box.y,e.box.w,e.box.h};
 dialogLayout_.frame={result.bounds.x,result.bounds.y,result.bounds.w,result.bounds.h};return result.bounds;
}
void View::previewUiScreen(std::string screen,std::string variant,UiBindings bindings,bool pressed,bool disabled,double elapsed){
 cancelGesture();buttons_.clear();panel_=Panel::None;panelLayer_.reset();panelMotion_={};paintedPanel_=Panel::None;fundsDialog_.reset();noticeDialog_.reset();fundsAnimation_={};levelUps_.clear();helpOpen_=false;
 uiPreviewScreen_=std::move(screen);uiPreviewVariant_=std::move(variant);uiPreviewBindings_=std::move(bindings);uiPreviewPressed_=pressed;uiPreviewDisabled_=disabled;uiPreviewElapsed_=elapsed;
}
void View::renderUiPreview(){
 const auto* project=uiProject();if(!project)return;
 const auto screen=uiPreviewScreen_,variant=uiPreviewVariant_;
 const auto bounds=uiScreenBounds(canvas_,project->screen(screen));
 const auto dismiss=[this]{uiPreviewScreen_.clear();uiPreviewPressed_=uiPreviewDisabled_=false;buttons_.clear();};
 const auto openShop=[this,dismiss,variant]{dismiss();openCurrencyShop(variant=="pearls"?Currency::Pearls:Currency::Coins);};
 const auto offer=[this,dismiss]{auto title=uiPreviewBindings_.values.contains("title")?uiPreviewBindings_.values.at("title"):"Currency pack";dismiss();showFunds({});currencyOfferTitle_=title;};
 const bool dialog=screen=="general-dialog";
 if(dialog){ui();buttons_.clear();if(!fundsAnimation_.layer)fundsAnimation_.layer=std::make_unique<Texture>();canvas_.beginMenuLayer(*fundsAnimation_.layer);}
 auto result=drawUi(screen,variant,bounds,uiPreviewBindings_,{{"primary",{"preview-primary",{},screen=="shop-card"?std::function<void()>(offer):std::function<void()>(openShop)}},{"close",{"preview-close",{},dismiss}},
  {"coins",{"preview-coins",{},[this,dismiss]{dismiss();openCurrencyShop(Currency::Coins);}}},{"pearls",{"preview-pearls",{},[this,dismiss]{dismiss();openCurrencyShop(Currency::Pearls);}}},{"coinBalance",{"preview-coin-balance",{},[this,dismiss]{dismiss();openCurrencyShop(Currency::Coins);}}},{"pearlBalance",{"preview-pearl-balance",{},[this,dismiss]{dismiss();openCurrencyShop(Currency::Pearls);}}}});
 if(dialog){canvas_.endMenuLayer();const auto pose=uiMotionPose(project->dialogMotion,uiPreviewElapsed_,{bounds.x+bounds.w*.5f,bounds.y+bounds.h*.55f});
  canvas_.fill({0,0,canvas_.width(),canvas_.height()},{0,20,48,Uint8(170*pose.alpha)});canvas_.menuLayer(*fundsAnimation_.layer,pose);
  for(auto& e:uiLayout_->elements)e.box=pose.apply(e.box);for(auto& button:buttons_)button.area=pose.apply(button.area);
 }
}
void View::previewPurchase(int level,Amount coins,Amount pearls,int tank,bool owned,int upgrades){
 if(!session_.ephemeral())throw std::runtime_error("Purchase scenarios require a separate preview session");
 auto& domain=session_.domain();domain.fixture("aquarium");auto state=domain.state();
 state.wallet={std::max<Amount>(0,coins),std::max<Amount>(0,pearls)};
 level=std::clamp(level,1,int(domain.content().levels.size()));state.xp=domain.content().levels[level-1];state.highestRewardedLevel=level;
 state.tutorialStep=11;state.settings.sound=false;state.settings.music=false;state.settings.reducedMotion=true;
 tank=std::clamp(tank,1,5);state.tanks.clear();for(int i=1;i<=std::max(1,tank-(owned?0:1));++i)state.tanks.push_back({{i},i==tank?10+std::clamp(upgrades,0,2)*5:20});
 state.activeTank={1};domain.install(std::move(state));domain.takeEvents();uiPreviewScreen_.clear();uiPreviewPressed_=uiPreviewDisabled_=false;fundsDialog_.reset();noticeDialog_.reset();fundsAnimation_={};levelUps_.clear();helpOpen_=false;
 open(Panel::Tanks);selectedTank_=tank;panelMotion_.settle();
}
}
