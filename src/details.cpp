#include "aquarium/view.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace aq {
namespace {
constexpr Color white{247,253,255,255},yellow{255,229,119,255},blue{154,219,242,255};
std::string upper(std::string s){for(auto& c:s)c=char(std::toupper(static_cast<unsigned char>(c)));return s;}
std::string careTime(Millis ms){
 ms=std::max<Millis>(0,ms);const auto hours=ms/3600000,minutes=ms/60000%60,seconds=ms/1000%60;
 if(hours)return std::to_string(hours)+"H "+std::to_string(minutes)+"M";
 if(minutes)return std::to_string(minutes)+"M "+std::to_string(seconds)+"S";
 return std::to_string(seconds)+"S";
}
struct Popover {
 Rect body;
 SDL_FPoint tip;
};
Popover placePopover(Rect bounds,SDL_FPoint at,SDL_FPoint fishSize){
 const float w=std::min(580.f,bounds.w),h=w*650.f/808.f;
 const float gap=18,halfW=fishSize.x*.5f+8,halfH=fishSize.y*.5f+8;
 const float above=at.y-halfH-gap-bounds.y,below=bounds.y+bounds.h-at.y-halfH-gap;
 const float left=at.x-halfW-gap-bounds.x,right=bounds.x+bounds.w-at.x-halfW-gap;
 Rect r{at.x-w*.5f,at.y-halfH-gap-h,w,h};
 SDL_FPoint tip{at.x,at.y-halfH};
 if(above>=h){} // Prefer a speech bubble above the fish.
 else if(below>=h){r.y=at.y+halfH+gap;tip.y=at.y+halfH;}
 else if(left>=w||left>=right){r.x=at.x-halfW-gap-w;r.y=at.y-h*.5f;tip={at.x-halfW,at.y};}
 else{r.x=at.x+halfW+gap;r.y=at.y-h*.5f;tip={at.x+halfW,at.y};}
 r.x=std::clamp(r.x,bounds.x,bounds.x+bounds.w-w);
 r.y=std::clamp(r.y,bounds.y,bounds.y+bounds.h-h);
 return {r,tip};
}
// Preserve the reference card's proportions within an anchored popover.
struct DetailsLayout {
 float scale,x,y;
 explicit DetailsLayout(Rect p):scale(p.w/808.f),x(p.x-267.f*scale),y(p.y-110.f*scale){}
 Rect rect(float px,float py,float w,float h)const{return {x+px*scale,y+py*scale,w*scale,h*scale};}
};
void lettering(Canvas& c,const DetailsLayout& l,std::string_view text,float x,float y,float size,float width,Color color=white,bool center=true,bool title=false){
 const float stroke=(title?4.5f:.8f)*l.scale;
 for(int i=0;i<8;++i){const float a=float(i)*.78539816f;
  c.text(text,l.x+x*l.scale+std::cos(a)*stroke,l.y+y*l.scale+std::sin(a)*stroke+(title?4.f:1.6f)*l.scale,size*l.scale,{0,66,114,255},center,width*l.scale,true,true);
 }
 if(title)c.text(text,l.x+x*l.scale,l.y+(y+3)*l.scale,size*l.scale,{224,113,0,255},center,width*l.scale,true,true);
 c.text(text,l.x+x*l.scale,l.y+y*l.scale,size*l.scale,color,center,width*l.scale,true,true,title);
}


}

FishDetailsContent fishDetailsContent(const Species& s,const Fish& f,Millis now){
 FishDetailsContent out;out.stage=f.egg?-1:std::clamp(f.age,0,4);out.condition=careOf(s,f,now);
 const auto at=f.stashed?f.stashedAt:now;
 out.progress=f.age>=4?1.f:std::clamp(float(double(f.growthMs)/double(s.stageMs)),0.f,1.f);
 out.title=upper(stageName(f))+(f.egg?"":" FISH");
 if(f.egg){out.title="FISH EGG";out.care="HATCHES IN "+careTime(f.hatchAt-at);out.growth="YOUR BABY FISH WILL HATCH SOON";out.sale="FIRST SALE UNLOCKS AT JUNIOR";out.progress=0;return out;}
 if(f.dead){out.title="FISH NEEDS HELP";out.care="REVIVE TO BRING YOUR FISH BACK";out.growth="GROWTH PAUSED";out.sale="";return out;}
 if(out.condition==Care::Sick)out.care="SICK - FEED TO RECOVER";
 else out.care=std::string(out.condition==Care::Fed?"FED":out.condition==Care::Urgent?"FEED SOON":"HUNGRY")+" - FEED WITHIN "+careTime(f.lastFedAt+s.feedMs-at);
 if(f.stashed)out.growth="GROWTH PAUSED WHILE IN STORAGE";
 else if(f.age>=4)out.growth="FULLY GROWN";
 else if(out.condition!=Care::Fed)out.growth="GROWTH PAUSED - FEED TO RESUME";
 else out.growth="GROWING - NEXT STAGE IN "+careTime(s.stageMs-f.growthMs);
 if(s.nonResellable)out.sale="THIS FISH CANNOT BE SOLD";
 else if(f.age==0)out.sale="FIRST SALE UNLOCKS AT JUNIOR";
 else out.sale="SELL VALUE: "+compact(s.saleCoins[f.age])+" COINS + "+compact(s.saleXp[f.age])+" XP";
 return out;
}

void View::details(){
 const auto& d=session_.domain();const auto* f=d.fish(selected_);if(!f)return;const auto& s=*d.content().find(f->species);
 const auto content=fishDetailsContent(s,*f,d.state().simNow);
 const double alpha=session_.interpolation();
 const auto at=canvas_.toScreen(fishPose(*f,alpha).position);
 const auto safe=canvas_.safeInsets();
 const float left=std::max(24.f,safe.left+16),right=std::max(154.f,safe.right+16);
 const float top=std::max(canvas_.height()>=1000?140.f:104.f,safe.top+94),bottom=std::max(114.f,safe.bottom+94);
 const Rect available{left,top,canvas_.width()-left-right,canvas_.height()-top-bottom};
 const auto size=f->egg?SDL_FPoint{36*canvas_.worldScale(),42*canvas_.worldScale()}:canvas_.fishSize(s,*f);
 const auto placed=placePopover(available,at,size);panelRect_=placed.body;panelAnchor_=placed.tip;
 const auto p=panelRect_;const DetailsLayout l(p);canvas_.popover(p,placed.tip);
 lettering(canvas_,l,content.title,670,137,81,378,yellow,true,true);
 lettering(canvas_,l,upper(s.name),670,257,56,675);
 lettering(canvas_,l,content.care,420,326,42,595,content.condition==Care::Sick||f->dead?Color{255,202,126,255}:yellow,false);
 lettering(canvas_,l,"GROWTH",416,407,45,375,white,false);
 std::ostringstream percentage;percentage<<std::fixed<<std::setprecision(1)<<content.progress*100.f<<'%';
 lettering(canvas_,l,percentage.str(),978,407,45,118,yellow);
 if(content.progress>0)canvas_.skin("green",l.rect(332,474,675*content.progress,25),12*l.scale);
 constexpr std::array<float,5> centres{374,521,670,818,964};
 constexpr std::array<const char*,5> names{"BABY","JUNIOR","YOUNG","MATURE","ADULT"};
 for(int i=0;i<5;++i){
  if(i==content.stage)canvas_.image("details/stage-active.png",l.rect(centres[i]-25,517,50,50));
  lettering(canvas_,l,names[i],centres[i],570,30,142,i==content.stage?yellow:blue);
 }
 lettering(canvas_,l,content.growth,670,621,43,683,yellow);
 if(!f->dead)lettering(canvas_,l,content.sale,670,677,43,687);
 else{
  const auto id=f->id;
  glassButton("one-revive",l.rect(348,672,309,57),"REVIVE: 1 PEARL",[this,id]{command({.action=Action::Revive,.fish=id});},"green",25*l.scale);
  glassButton("one-remove",l.rect(685,672,290,57),"REMOVE",[this,id]{if(command({.action=Action::Remove,.fish=id}))open(Panel::None);},"blue",25*l.scale);
 }
 const auto close=l.rect(1010,201,30,30);
 canvas_.icon("skin/control-close.png",close);
 touchTarget("details-close",{close.x-7,close.y-7,close.w+14,close.h+14},[this]{open(Panel::None);});
}
}
