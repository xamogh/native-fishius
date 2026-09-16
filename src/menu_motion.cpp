#include "aquarium/view.hpp"
#include "aquarium/ui_renderer.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aq {
bool View::prepareMenus(const PreparationProgress& progress){
 // Render the real layouts into a separate view. No actions, simulation,
 // event draining or changes to the player's selection occur during loading.
 View preview(canvas_,session_);
 struct Work {std::string_view stage;std::function<void()> run;};
 std::vector<Work> work;
 canvas_.begin();
 for(bool mask:{false,true})for(const auto& species:session_.domain().content().species){
  if(!species.artReady||species.releaseGate!="Launch"||species.level>40)continue;
  const auto art=fishArt(species.id);
  const auto name=mask?art.substr(0,art.size()-4)+"-mask.png":art;
  work.push_back({mask?"Preparing your collection":"Welcoming your fish",[&,name]{canvas_.preload(name);}});
 }
 work.push_back({"Bringing your reef to life",[&]{
  const auto& d=session_.domain();canvas_.scene(d,0,0,preview.tool_,{},true);
  // Prime the alternate mesh path and render target used by fish details.
  canvas_.softenScene();
 }});
 auto page=[&](Panel panel,int category,int number,std::string_view stage){
  work.push_back({stage,[&,panel,category,number]{
   preview.panel_=panel;preview.category_=category;preview.page_=number;
   preview.panelMotion_.visible=true;preview.panelMotion_.settle();
   preview.buttons_.clear();preview.panelButtonStart_=0;preview.panel();
  }});
 };
 for(const auto panel:{Panel::Tanks,Panel::Settings,Panel::Inventory,Panel::Quests,Panel::Gifts,Panel::CurrencyShop})page(panel,0,0,"Preparing your menus");
 work.push_back({"Preparing your dialogs",[&]{
  for(const auto art:{"general-dialog/frame.png","general-dialog/coins.png","general-dialog/pearls.png","general-dialog/open-shop.png","general-dialog/button.png","general-dialog/close.png"})canvas_.preload(art);
  preview.fundsCurrency_=Currency::Coins;preview.fundsDialog_=CurrencyShortfall{};preview.fundsDialog();
  preview.fundsCurrency_=Currency::Pearls;preview.fundsDialog();
  preview.fundsCurrency_.reset();preview.fundsDialog();
  preview.fundsDialog_.reset();preview.helpDialog();
 }});
 for(int category=0;category<5;++category)
  for(int number=0;number<shopPageCount(category);++number)page(Panel::Shop,category,number,"Stocking your shop");
 for(int number=0;number<std::max(1,(int(session_.domain().content().species.size())+7)/8);++number)
  page(Panel::Collection,0,number,"Preparing your collection");
 const auto& state=session_.domain().state();
 const auto fish=std::find_if(state.fish.begin(),state.fish.end(),[&](const Fish& f){return !f.stashed&&f.tank==state.activeTank;});
 if(fish!=state.fish.end()){
  preview.selected_=fish->id;page(Panel::Details,0,0,"Getting your fish ready");
 }
 work.push_back({"Adding the finishing touches",[&]{
  preview.panel_=Panel::None;preview.ui();
  canvas_.preload("controls/stash.png");canvas_.preload("controls/move.png");
  canvas_.preload("ui/keep.png");canvas_.preload("ui/rehome.png");
  canvas_.toolCursor(Tool::Food,{0,0},-1,true);canvas_.toolCursor(Tool::Sell,{0,0},-1,true);
 }});
 for(std::size_t i=0;i<work.size();++i){
  if(progress&&!progress(float(i)/float(work.size()),work[i].stage))return false;
  // The progress callback presents a loading frame. Restore the offscreen
  // target before continuing, and keep its changing percentage unretained.
  canvas_.begin();canvas_.retainPreparedText(true);
  try{work[i].run();SDL_FlushRenderer(canvas_.renderer());}
  catch(...){canvas_.retainPreparedText(false);throw;}
  canvas_.retainPreparedText(false);
 }
 canvas_.begin();
 return !progress||progress(1,"Your reef is ready");
}

MenuMotion::Sample MenuMotion::sample(double now)const{
 const double t=std::max(0.,now-changed),target=visible?1.:0.,offset=from-target;
 if(t>=(visible?openingDuration:.32))return {target,0};
 if(visible){
  // A damped spring gives one soft overshoot and a small settling bounce.
  const double damping=openingDamping,frequency=openingFrequency;
  const double b=(velocity+damping*offset)/frequency;
  const double wave=offset*std::cos(frequency*t)+b*std::sin(frequency*t),decay=std::exp(-damping*t);
  return {target+decay*wave,decay*((-offset*frequency*std::sin(frequency*t)+b*frequency*std::cos(frequency*t))-damping*wave)};
 }
 // Critical damping closes quickly without bouncing back into the scene.
 constexpr double damping=30;
 const double b=velocity+damping*offset,decay=std::exp(-damping*t);
 return {target+(offset+b*t)*decay,(b-damping*(offset+b*t))*decay};
}
void MenuMotion::show(bool show,double now){
 if(visible==show)return;
 const auto current=sample(now);from=current.value;velocity=current.velocity;changed=now;visible=show;
}
Rect MenuPose::apply(Rect r)const{return {anchor.x+(r.x-anchor.x)*xScale,anchor.y+(r.y-anchor.y)*yScale+rise,r.w*xScale,r.h*yScale};}
SDL_FPoint MenuPose::unapply(SDL_FPoint p)const{return {anchor.x+(p.x-anchor.x)/xScale,anchor.y+(p.y-anchor.y-rise)/yScale};}

void Canvas::beginMenuLayer(Texture& layer){
 if(layer.value&&(layer.width!=frameWidth_||layer.height!=frameHeight_)){SDL_DestroyTexture(layer.value);layer.value=nullptr;}
 if(!layer.value){
  layer.value=SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,frameWidth_,frameHeight_);
  if(!layer.value)throw std::runtime_error(SDL_GetError());layer.width=frameWidth_;layer.height=frameHeight_;
  SDL_SetTextureScaleMode(layer.value,SDL_SCALEMODE_LINEAR);
  // Drawing onto transparency stores premultiplied RGB. Preserve soft edges
  // when composing the whole menu and fade RGB together with alpha.
  if(!SDL_SetTextureBlendMode(layer.value,SDL_BLENDMODE_BLEND_PREMULTIPLIED))throw std::runtime_error(SDL_GetError());
 }
 if(!SDL_SetRenderTarget(renderer_,layer.value))throw std::runtime_error(SDL_GetError());
 SDL_SetRenderScale(renderer_,pixelScaleX_,pixelScaleY_);SDL_SetRenderDrawColor(renderer_,0,0,0,0);SDL_RenderClear(renderer_);
}
void Canvas::endMenuLayer(){SDL_SetRenderTarget(renderer_,frameTexture_);SDL_SetRenderScale(renderer_,pixelScaleX_,pixelScaleY_);}
void Canvas::menuLayer(const Texture& layer,const MenuPose& pose){
 const auto r=pose.apply({0,0,width_,height_});const SDL_FRect destination{r.x,r.y,r.w,r.h};
 SDL_SetTextureColorModFloat(layer.value,pose.alpha,pose.alpha,pose.alpha);SDL_SetTextureAlphaModFloat(layer.value,pose.alpha);
 SDL_RenderTexture(renderer_,layer.value,nullptr,&destination);
}

bool View::reducedMotion()const{return session_.domain().state().settings.reducedMotion;}
MenuPose View::menuPose(const MenuMotion& motion,SDL_FPoint anchor)const{
 if(reducedMotion())return {anchor,1,1,0,motion.visible?1.f:0.f};
 const float value=float(motion.sample(now_).value);
 return {anchor,.84f+.16f*value,.80f+.20f*value,24*(1-value),std::clamp(value*1.8f,0.f,1.f)};
}

void View::renderDialog(DialogAnimation& animation,bool visible,void(View::*draw)(),Uint8 shade){
 if(&animation==&fundsAnimation_&&uiProject()){const auto& m=uiProject()->dialogMotion;animation.motion.openingDuration=m.duration;animation.motion.openingDamping=m.damping*.6/m.duration;animation.motion.openingFrequency=m.frequency*.6/m.duration;}
 animation.motion.show(visible,now_);
 if(visible){
  buttons_.clear();
  if(!animation.layer)animation.layer=std::make_unique<Texture>();
  canvas_.beginMenuLayer(*animation.layer);
  (this->*draw)();
  canvas_.endMenuLayer();
  animation.width=canvas_.width();animation.height=canvas_.height();
 }
 if(!animation.layer)return;
 const auto r=animation.bounds;
 animation.pose=menuPose(animation.motion,{r.x+r.w*.5f,r.y+r.h*.55f});
 if(&animation==&fundsAnimation_&&uiProject()&&!reducedMotion()){const auto& m=uiProject()->dialogMotion;const float value=float(animation.motion.sample(now_).value);animation.pose={{r.x+r.w*.5f,r.y+r.h*.55f},m.scale+(1-m.scale)*value,m.scale+(1-m.scale)*value,m.rise*(1-value),std::clamp(value*1.8f,0.f,1.f)};}
 if(&animation==&fundsAnimation_&&uiLayout_)for(auto& e:uiLayout_->elements)e.box=animation.pose.apply(e.box);
 if(!visible&&(animation.pose.alpha<=0||animation.width!=canvas_.width()||animation.height!=canvas_.height())){
  animation.layer.reset();return;
 }
 // Keep the backdrop fixed while the dialog springs. Its hit targets follow
 // the same transform, and a fading snapshot never exposes controls beneath it.
 if(visible&&animation.pose.alpha>0)for(auto& button:buttons_){
  const auto target=animation.pose.apply(button.area);
  const float ex=std::max(0.f,(canvas_.minimumTouchSize()-target.w)*.5f),ey=std::max(0.f,(canvas_.minimumTouchSize()-target.h)*.5f);
  button.area={target.x-ex,target.y-ey,target.w+2*ex,target.h+2*ey};
 }
 else buttons_.clear();
 canvas_.fill({0,0,canvas_.width(),canvas_.height()},{0,20,48,Uint8(shade*animation.pose.alpha)});
 canvas_.menuLayer(*animation.layer,animation.pose);
}

void View::dialogs(){
 renderDialog(helpAnimation_,helpOpen_,&View::helpDialog,156);
 renderDialog(fundsAnimation_,fundsDialog_||noticeDialog_,&View::fundsDialog,170);
 renderDialog(levelAnimation_,!levelUps_.empty(),&View::levelUp,156);
 if(dialogBlocking())canvas_.cursor(CursorKind::Arrow);
}

bool View::dialogBlocking()const{
 return helpOpen_||fundsDialog_||noticeDialog_||!levelUps_.empty()||helpAnimation_.layer||fundsAnimation_.layer||levelAnimation_.layer;
}

void View::showHelp(){
 cancelGesture();buttons_.clear();helpOpen_=true;helpAnimation_.motion.show(true,now_);
}
void View::dismissHelp(){
 cancelGesture();buttons_.clear();helpOpen_=false;helpAnimation_.motion.show(false,now_);
}
void View::helpDialog(){
 constexpr Color white{7,83,166,255};
 const auto safe=canvas_.safeInsets();
 const float width=canvas_.width()-safe.left-safe.right,height=canvas_.height()-safe.top-safe.bottom;
 const float u=std::min({1.f,(width-48)/790.f,(height-48)/380.f});
 const float h=std::max(55*u,canvas_.minimumTouchSize());
 const float dialogHeight=std::min(height-48,std::max(380*u,246*u+h));
 const Rect p{safe.left+(width-790*u)*.5f,safe.top+(height-dialogHeight)*.5f,790*u,dialogHeight};
 helpAnimation_.bounds=p;
 canvas_.skin("panel",p,48*u);
 titleSign({p.x+155*u,p.y-22*u,480*u,93*u},"Aquarium help");
 canvas_.text("Food is free. Tap FOOD, then tap the water.",p.x+395*u,p.y+97*u,24*u,white,true,712*u);
 canvas_.text("Shop purchases place eggs. Bag restores stored fish.",p.x+395*u,p.y+137*u,23*u,white,true,712*u);
 canvas_.text("Your progress stays on this device. Online support is not connected.",p.x+395*u,p.y+177*u,20*u,{7,83,166,255},true,712*u);
 glassButton("help-close",{p.x+293*u,p.y+p.h-h-28*u,204*u,h},"Got it",[this]{dismissHelp();},"green",26*u);
}
float View::buttonScale(std::string_view id)const{
 if(reducedMotion())return pressed_==id?.97f:1.f;
 const auto found=presses_.find(std::string(id));if(found==presses_.end())return 1;
 const auto& p=found->second;const double age=std::max(0.,now_-p.start);
 if(p.down){const float t=float(std::clamp(age/.075,0.,1.));return p.from+(.95f-p.from)*(1-std::pow(1-t,3));}
 if(age>=.4)return 1;
 return 1+(p.from-1)*float(std::exp(-12*age)*std::cos(24*age));
}
void View::press(std::string_view id,bool down){const float from=buttonScale(id);presses_[std::string(id)]={now_,from,down};}
Rect View::buttonVisual(std::string_view id,Rect rect)const{
 const float scale=buttonScale(id);return {rect.x+rect.w*(1-scale)*.5f,rect.y+rect.h*(1-scale)*.5f,rect.w*scale,rect.h*scale};
}
} // namespace aq
