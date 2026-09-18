#include "aquarium/hud_funds.hpp"
#include "aquarium/shop_theme.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace aq {
namespace {
std::string amount(Amount value){
 auto text=std::to_string(value);
 for(int i=int(text.size())-3;i>0;i-=3)text.insert(std::size_t(i),",");
 return text;
}
std::string missingText(Amount value,bool pearls){return amount(value)+(pearls?(value==1?" pearl":" pearls"):(value==1?" coin":" coins"));}
}
bool showFundsDialog(FundsDialogState& state,const Result& result){
 if(result.error!=Error::Funds||(result.shortfall.coins<=0&&result.shortfall.pearls<=0))return false;
 state={};state.shortfall=result.shortfall;state.dialog.open=true;return true;
}
Currency fundsCurrency(const FundsDialogState& state){return state.shortfall.coins>0?Currency::Coins:Currency::Pearls;}
std::string fundsDialogMessage(const FundsDialogState& state){
 if(state.shortfall.coins>0&&state.shortfall.pearls>0)return missingText(state.shortfall.coins,false)+" and "+missingText(state.shortfall.pearls,true)+" needed";
 const bool pearls=fundsCurrency(state)==Currency::Pearls;
 return missingText(pearls?state.shortfall.pearls:state.shortfall.coins,pearls)+" needed";
}
FundsDialogLayout layoutFundsDialog(float width,float height,Insets safe,float minimumTouch){
 // Enlarge the illustration and message on phones, within the safe area.
 const float base=std::min(width/1608.f,height/908.f);
 const float scale=std::max(base,minimumTouch/64.f);
 auto dialog=layoutDialog(width,height,safe,{"",DialogSize::Custom,840*scale/base,688*scale/base});
 const float u=dialog.frame.w/840.f,close=std::max(64*u,minimumTouch),header=close+16*u;
 dialog.unit=u;
 dialog.header={dialog.frame.x+8*u,dialog.frame.y+8*u,dialog.frame.w-16*u,header};
 dialog.close={dialog.header.x+dialog.header.w-close-8*u,dialog.header.y+8*u,close,close};
 dialog.title={dialog.header.x+16*u,dialog.header.y,dialog.header.w-close-40*u,header};
 dialog.body={dialog.frame.x+8*u,dialog.header.y+dialog.header.h,dialog.frame.w-16*u,dialog.frame.h-header-16*u};
 dialog.content={dialog.body.x+24*u,dialog.body.y+24*u,dialog.body.w-48*u,dialog.body.h-48*u};
 const auto c=dialog.content;const float center=c.x+c.w*.5f;
 const float buttonHeight=std::max(88*u,minimumTouch);
 const Rect shop{center-240*u,c.y+c.h-buttonHeight-12*u,480*u,buttonHeight};
 const Rect message{c.x,c.y+24*u,c.w,56*u};
 // The body plates reserve 22% above the character and 23% below it.
 // Fit the whole scene proportionally when larger touch controls need that room.
 Rect scene=dialog.body;
 if(scene.y+scene.h*.77f>shop.y-8*u){
  constexpr float aspect=1479.f/1064.f;
  const float top=message.y+message.h+12*u;
  const float h=std::min({(shop.y-12*u-top)/.55f,(top-scene.y)/.22f,(scene.y+scene.h-top)/.78f});
  const float w=std::min(scene.w,h*aspect);
  scene={center-w*.5f,top-.22f*w/aspect,w,w/aspect};
 }
 const Rect illustration{scene.x+scene.w*.14f,scene.y+scene.h*.22f,scene.w*.72f,scene.h*.55f};
 return {dialog,illustration,message,shop,scene};
}
void paintFundsDialog(Canvas& canvas,const FundsDialogLayout& l,const FundsDialogState& state,SDL_FPoint pointer){
 if(!state.open())return;
 const bool pearls=fundsCurrency(state)==Currency::Pearls;
 const float u=l.dialog.unit;
 const char* title=state.shortfall.coins>0&&state.shortfall.pearls>0?"Not enough coins and pearls":pearls?"Not enough pearls":"Not enough coins";
 paintDialog(canvas,l.dialog,{title},l.dialog.close.has(pointer.x,pointer.y),state.dialog.closePressed);
 const bool compact=l.scene.w<l.dialog.body.w-.1f;
 if(compact)canvas.roundedImage("dialogs/funds-underwater-v1.png",l.dialog.body,12*u);
 const float edge=compact?std::min(36*u,l.scene.w*.06f):12*u;
 canvas.roundedImage(pearls?"dialogs/funds-octopus-pearls-v1.png":"dialogs/funds-octopus-coins-v1.png",l.scene,edge,compact?edge:0);
 auto text=[&](std::string_view value,Rect r,float size,Color color){canvas.text(value,r.x+r.w*.5f,r.y+(r.h-size)*.5f,size,color,true,r.w,true,true);};
 const auto value=amount(pearls?state.shortfall.pearls:state.shortfall.coins),message=fundsDialogMessage(state);
 const auto suffix=std::string_view(message).substr(value.size());
 const float font=44*u;
 const float valueWidth=canvas.textWidth(value,font,true,false,false,false,true),suffixWidth=canvas.textWidth(suffix,font,true,false,false,false,true);
 const float scale=std::min(1.f,l.message.w/(valueWidth+suffixWidth));
 const float x=l.message.x+(l.message.w-(valueWidth+suffixWidth)*scale)*.5f,y=l.message.y+(l.message.h-font*scale)*.5f;
 auto run=[&](std::string_view label,float left,Color ink){canvas.text(label,left,y,font,ink,false,0,true,true,false,false,0,1,false,false,scale);};
 run(value,x,pearls?Color{143,104,188,255}:Color{205,143,21,255});
 run(suffix,x+valueWidth*scale,shopTheme::ink);
 shopTheme::panel(canvas,l.shop,u,shopTheme::Surface::Buy,state.shopPressed&&!state.dragged);
 if(l.shop.has(pointer.x,pointer.y))canvas.outline(l.shop,shopTheme::white,12*u,2*u);
 text("Open Shop",l.shop,std::max(36*u,l.dialog.close.w*.4f),shopTheme::white);
}
FundsDialogEvent fundsDialogEvent(FundsDialogState& state,const FundsDialogLayout& l,const SDL_Event& event,SDL_FPoint point){
 if(!state.open())return FundsDialogEvent::Ignored;
 if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST||event.type==SDL_EVENT_WILL_ENTER_BACKGROUND||event.type==SDL_EVENT_RENDER_DEVICE_RESET||event.type==SDL_EVENT_RENDER_TARGETS_RESET){
  state.shopPressed=state.dragged=state.dialog.closePressed=state.dialog.backdropPressed=false;
  return FundsDialogEvent::Ignored;
 }
 if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat&&(event.key.key==SDLK_RETURN||event.key.key==SDLK_SPACE)){
  state.dialog.open=false;state.shopPressed=false;return FundsDialogEvent::OpenShop;
 }
 if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT){state.shopPressed=l.shop.has(point.x,point.y);state.pressPoint=point;state.dragged=false;}
 if(event.type==SDL_EVENT_MOUSE_MOTION&&std::hypot(point.x-state.pressPoint.x,point.y-state.pressPoint.y)>12*l.dialog.unit)state.dragged=true;
 if(event.type==SDL_EVENT_MOUSE_BUTTON_UP&&event.button.button==SDL_BUTTON_LEFT){
  const bool activate=state.shopPressed&&!state.dragged&&l.shop.has(point.x,point.y)&&std::hypot(point.x-state.pressPoint.x,point.y-state.pressPoint.y)<=12*l.dialog.unit;
  state.shopPressed=false;
  if(activate){state.dialog.open=false;return FundsDialogEvent::OpenShop;}
 }
 const bool handled=dialogEvent(state.dialog,l.dialog,event,point);
 if(!state.open())state.shopPressed=false;
 return handled?FundsDialogEvent::Handled:FundsDialogEvent::Ignored;
}
void FundsShopReturn::open(const FundsDialogState& funds,bool& shopOpen,ShopState& shop){
 previous=shopOpen?std::optional{shop}:std::nullopt;
 shop={ShopCategory::Treasure,fundsCurrency(funds)==Currency::Pearls?1:0,0};shopOpen=true;
}
void FundsShopReturn::close(bool& shopOpen,ShopState& shop){
 if(const auto origin=std::exchange(previous,std::nullopt);shopOpen&&origin){shop=*origin;shopOpen=true;}
 else shopOpen=false;
}
}
