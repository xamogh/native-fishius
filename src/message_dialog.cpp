#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
namespace {
Color color(DesignColor value){return {Uint8(value[0]),Uint8(value[1]),Uint8(value[2]),Uint8(value[3])};}
Rect rect(DesignBox box){return {box.x,box.y,box.w,box.h};}
float textWidth(Canvas& canvas,const DialogElement& e,std::string_view text,float size){
 return canvas.textWidth(text,size,e.font!="nunito",false,e.font=="baloo")*e.stretch;
}
void text(Canvas& canvas,const DialogElement& e,std::string_view value,float x,float y,float size,Color tint,bool center,float maxWidth){
 canvas.text(value,x,y,size,tint,center,maxWidth,e.font!="nunito",false,false,false,0,e.stretch,e.font=="baloo");
}
}
Rect View::messageDialog(const MessageDialogContent& content,std::string_view actionId,std::string_view closeId,
                        std::function<void()> action,std::function<void()> close){
 if(uiProject())return uiMessageDialog(content,actionId,closeId,std::move(action),std::move(close));
 const auto& design=dialogDesign();const auto safe=canvas_.safeInsets();
 dialogLayout_=layoutDialog(design,{safe.left,safe.top,canvas_.width()-safe.left-safe.right,canvas_.height()-safe.top-safe.bottom},canvas_.minimumTouchSize()/44.f);
 const float u=dialogLayout_.unit;
 const auto bounds=[&](DialogPart part){return rect(dialogLayout_.element(part));};
 const auto& frame=design.element(DialogPart::Frame);
 canvas_.image(frame.asset,bounds(DialogPart::Frame));
 const auto art=bounds(DialogPart::Illustration);
 if(content.illustration.starts_with("general-dialog/"))canvas_.image(content.illustration,art);
 else if(!content.illustration.empty())canvas_.icon(content.illustration,{art.x+art.w*80/696,art.y+art.h*16/284,art.w*528/696,art.h*261/284});
 const auto label=[&](DialogPart part,std::string_view value,float offset=0,float size=0){
  const auto& e=design.element(part);const auto r=bounds(part);
  text(canvas_,e,value,r.x+r.w*.5f,r.y+offset*u,(size>0?size:e.fontSize)*u,color(e.color),true,r.w);
 };
 label(DialogPart::Title,content.title);
 const auto& sentence=design.element(DialogPart::Message);const auto line=bounds(DialogPart::Message);
 const auto lineWidth=[&](float scale){float w=0;for(const auto& part:content.message)w+=textWidth(canvas_,sentence,part.text,(part.emphasis?design.emphasisSize:sentence.fontSize)*u*scale);return w;};
 float fit=1;
 for(int i=0;i<4&&lineWidth(fit)>line.w;++i)fit*=line.w/lineWidth(fit);
 float x=line.x+line.w*.5f-lineWidth(fit)*.5f;
 for(const auto& part:content.message){
  const float size=std::round((part.emphasis?design.emphasisSize:sentence.fontSize)*u*fit);
  text(canvas_,sentence,part.text,x,line.y+(design.emphasisSize*u-size)*.99f,size,color(part.emphasis?design.emphasisColor:sentence.color),false,0);
  x+=textWidth(canvas_,sentence,part.text,size);
 }
 if(!content.extra.empty())label(DialogPart::Extra,content.extra);
 label(DialogPart::Detail,content.detail,content.extra.empty()?0:design.detailExtraOffset,content.extra.empty()?0:design.detailExtraSize);
 const auto primary=bounds(DialogPart::Action),primaryVisual=buttonVisual(actionId,primary);
 const auto& button=design.element(DialogPart::Action);
 if(design.useShopArtwork&&content.actionLabel=="Open Shop")canvas_.image(design.shopArtwork,primaryVisual);
 else{
  canvas_.image(button.asset,primaryVisual);
  if(button.font=="lilita"&&button.stretch==1)canvas_.label(content.actionLabel,primaryVisual.x+primaryVisual.w*.5f,primaryVisual.y+primaryVisual.h*33/135,
    button.fontSize*u,true,primaryVisual.w-64*u,color(button.color),2.5f*u,{17,106,72,255});
  else text(canvas_,button,content.actionLabel,primaryVisual.x+primaryVisual.w*.5f,primaryVisual.y+primaryVisual.h*33/135,button.fontSize*u,color(button.color),true,primaryVisual.w-64*u);
 }
 touchTarget(std::string(actionId),primary,std::move(action));
 const auto closeRect=bounds(DialogPart::Close);
 canvas_.image(design.element(DialogPart::Close).asset,buttonVisual(closeId,closeRect));
 touchTarget(std::string(closeId),closeRect,std::move(close));
 return rect(dialogLayout_.frame);
}
void View::previewFunds(CurrencyShortfall missing,std::string requirement,double elapsed){
 // Preview uses the ordinary message builder and the ordinary spring. Its
 // Session is separate from the player's save, as with the game's fixtures.
 panel_=Panel::None;panelLayer_.reset();panelMotion_={};paintedPanel_=Panel::None;
 showFunds(missing,std::move(requirement));
 fundsAnimation_.motion={true,1-std::clamp(elapsed,0.,.6),0,0};
}
std::array<Rect,8> View::dialogElementBounds()const{
 std::array<Rect,8> result;
 for(std::size_t i=0;i<result.size();++i)result[i]=fundsAnimation_.pose.apply(rect(dialogLayout_.elements[i]));
 return result;
}
} // namespace aq
