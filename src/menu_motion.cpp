#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aq {
void View::prepareMenus(){
 // Prepare the actual current layouts before the first interactive frame.
 // A separate view warms shared Canvas art and glyph caches without changing
 // the player's selection, filters, gestures, events or saved game.
 View preview(canvas_,session_);
 canvas_.begin();preview.ui();
 for(const auto panel:{Panel::Tanks,Panel::Settings,Panel::Inventory,Panel::Quests,Panel::Gifts,Panel::Shop,Panel::Collection}){
  preview.panel_=panel;preview.panelMotion_.visible=true;preview.panelMotion_.settle();
  preview.buttons_.clear();preview.panelButtonStart_=0;preview.panel();
 }
 const auto& state=session_.domain().state();
 const auto fish=std::find_if(state.fish.begin(),state.fish.end(),[&](const Fish& f){return !f.stashed&&f.tank==state.activeTank;});
 if(fish!=state.fish.end()){
  preview.panel_=Panel::Details;preview.selected_=fish->id;preview.buttons_.clear();preview.panel();
 }
 // The smaller tool menu uses different label sizes from the main panels.
 preview.panel_=Panel::None;preview.selectMenu(true);for(auto& motion:preview.selectMotion_)motion.settle();preview.ui();
 SDL_FlushRenderer(canvas_.renderer());
 canvas_.begin();
}

MenuMotion::Sample MenuMotion::sample(double now)const{
 const double t=std::max(0.,now-changed),target=visible?1.:0.,offset=from-target;
 if(t>=(visible?.58:.32))return {target,0};
 if(visible){
  // A damped spring gives one soft overshoot and a small settling bounce.
  constexpr double damping=13,frequency=19;
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
void View::selectMenu(bool visible){
 selectMenuOpen_=visible;
 for(std::size_t i=0;i<selectMotion_.size();++i){
  auto& motion=selectMotion_[i];motion.show(visible,now_);
  if(visible&&motion.from==0&&motion.velocity==0)motion.changed+=double(i)*.045;
 }
}
void View::closeSelectMenu(){selectMenu(false);}
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
