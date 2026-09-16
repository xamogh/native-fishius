#include "aquarium/theme.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace aq {
namespace {
std::string remaining(Millis ms){
 const auto seconds=(std::max<Millis>(0,ms)+999)/1000;
 if(seconds>=3600)return std::to_string(seconds/3600)+"h "+std::to_string(seconds/60%60)+"m";
 if(seconds>=60)return std::to_string(seconds/60)+"m "+std::to_string(seconds%60)+"s";
 return std::to_string(seconds)+"s";
}
struct Placement {Rect body;SDL_FPoint tip;};
Placement place(Rect bounds,SDL_FPoint at,SDL_FPoint size,float w,float h,float u){
 const float gap=44*u,halfW=size.x*.5f+8*u,halfH=size.y*.5f+8*u;
 const std::array<Placement,4> candidates{{
  {{at.x-halfW-gap-w,at.y-h*.5f+22*u,w,h},{at.x-halfW,at.y}},
  {{at.x+halfW+gap,at.y-h*.5f+22*u,w,h},{at.x+halfW,at.y}},
  {{at.x-w*.5f,at.y-halfH-gap-h,w,h},{at.x,at.y-halfH}},
  {{at.x-w*.5f,at.y+halfH+gap,w,h},{at.x,at.y+halfH}}
 }};
 auto chosen=candidates[0];float best=1e30f;
 for(auto candidate:candidates){
  const auto before=candidate.body;candidate.body.x=std::clamp(before.x,bounds.x,bounds.x+bounds.w-w);candidate.body.y=std::clamp(before.y,bounds.y,bounds.y+bounds.h-h);
  const auto& r=candidate.body;const float overlap=std::max(0.f,std::min(r.x+r.w,at.x+halfW)-std::max(r.x,at.x-halfW))*std::max(0.f,std::min(r.y+r.h,at.y+halfH)-std::max(r.y,at.y-halfH));
  const float score=overlap*10+std::abs(before.x-r.x)+std::abs(before.y-r.y);if(score<best){best=score;chosen=candidate;}
 }
 return chosen;
}
}
FishDetailsContent fishDetailsContent(const Species& s,const Fish& f,Millis now){
 FishDetailsContent out;out.title=s.name;out.stage=f.egg?-1:f.age;out.condition=careOf(s,f,now);out.progress=float(growthProgress(f));out.reward=fishReward(f);out.adultReward={f.purchase.principal,f.purchase.profit,f.scripted?0:f.purchase.xp};
 out.care=f.egg?"Egg":f.age==4?"Adult · Ready":out.condition==Care::Fed?"Fed · Growing":"Hungry · Paused";
 out.growth=f.egg?"Hatches in "+remaining(f.hatchAt-now):f.age==4?"Collect once":remaining(f.purchase.durationMs-f.growthMs)+" of growth left";
 out.sale=std::to_string(out.reward.coins())+" coins + "+std::to_string(out.reward.xp)+" XP";return out;
}
void View::details(){
 const auto& domain=session_.domain();const auto* growing=domain.fish(selected_);const auto* display=domain.companion(selected_);if(!growing&&!display)return;
 const Fish visual=growing?*growing:companionVisual(*display);const auto& species=*domain.content().find(visual.species);
 const bool adult=growing&&!visual.egg&&visual.age==4;
 auto content=fishDetailsContent(species,visual,domain.state().simNow);
 content.reward.xp=std::min(content.reward.xp,domain.content().levels.back()-domain.state().xp);
 content.adultReward.xp=std::min(content.adultReward.xp,domain.content().levels.back()-domain.state().xp);
 const auto safe=canvas_.safeInsets();const float native=canvas_.minimumTouchSize()/44.f;
 const Rect bounds{std::max(safe.left,10*native),std::max(safe.top,51*native),canvas_.width()-std::max(safe.left,10*native)-std::max(safe.right,85*native),canvas_.height()-std::max(safe.top,51*native)-std::max(safe.bottom,48*native)};
 const auto at=canvas_.toScreen(fishPose(visual,session_.interpolation()).position);
 const auto size=visual.egg?SDL_FPoint{36*canvas_.worldScale(),42*canvas_.worldScale()}:canvas_.fishSize(species,visual);
 // All positions below are measured from the supplied 1338 by 1002 reference.
 // One scale drives artwork, live text and input targets on every viewport.
 constexpr float width=682,height=621;
 const float natural=std::min({canvas_.width()/1338.f,canvas_.height()/1002.f,bounds.w/width,bounds.h/height});
 const float horizontal=std::max(at.x-size.x*.5f-bounds.x,bounds.x+bounds.w-at.x-size.x*.5f)-52*natural;
 const float vertical=std::max(at.y-size.y*.5f-bounds.y,bounds.y+bounds.h-at.y-size.y*.5f)-52*natural;
 const float u=std::min(natural,std::max(horizontal/width,vertical/height));
 const auto placed=place(bounds,at,size,width*u,height*u,u);panelRect_=placed.body;panelAnchor_=placed.tip;canvas_.popover(panelRect_,placed.tip);
 const auto p=panelRect_;
 const auto rect=[&](float x,float y,float w,float h){return Rect{p.x+(x-202)*u,p.y+(y-173)*u,w*u,h*u};};
 constexpr Color ink{5,44,99,255},muted{100,136,172,255},hungry{145,68,14,255},fed{34,135,87,255};
 const auto label=[&](std::string_view text,float x,float y,float font,float w,bool center=false,Color color=Color{5,44,99,255}){canvas_.boldText(text,p.x+(x-202)*u,p.y+(y-173)*u,font*u,color,center,w*u,.94f);};
 canvas_.roundedText(species.name,p.x+89*u,p.y+24*u,62*u,ink,false,399*u,.93f);
 const auto id=visual.id;
 const auto heart=rect(689,204,81,76);canvas_.image("fish-popover/heart.png",rect(700,217,60,53),0,{.5f,.5f},1,false,visual.favorite?Color{255,124,177,255}:Color{255,255,255,255});
 touchTarget("fish-favorite",heart,[this,id]{command({.action=Action::Favorite,.fish=id});});
 const auto close=rect(779,155,100,100);canvas_.image("fish-popover/close.png",buttonVisual("details-close",close));touchTarget("details-close",close,[this]{open(Panel::None);});
 const auto action=[&](std::string key,float x,float y,float w,std::string title,std::string icon,std::function<void()> fn){
  const auto target=rect(x,y,w,107),r=buttonVisual(key,target);const float s=r.h/107.f;
  const bool rehome=key=="fish-rehome"||key=="rehome-confirm";
  canvas_.image(rehome?"fish-popover/rehome-button.png":"fish-popover/button.png",r);
  if(!rehome)canvas_.icon(icon,{r.x+30*s,r.y+18*s,68*s,72*s},1,key=="rehome-back");
  const float tx=r.x+120*s,ty=r.y+25*s,textWidth=(w-135)*s;
  for(const auto d:std::array<SDL_FPoint,4>{{{-1,0},{1,0},{0,-1},{0,2}}})canvas_.boldText(title,tx+d.x*2*s,ty+d.y*2*s,40*s,{4,101,190,255},false,textWidth,.9f);
  canvas_.boldText(title,tx,ty,40*s,{255,255,255,255},false,textWidth,.9f);
  touchTarget(std::move(key),target,std::move(fn));
 };
 if(display){
  label("Display fish",292,268,34,450,false,fed);
  canvas_.image("fish-popover/growth-card.png",rect(237,328,619,189));
  canvas_.image("fish-popover/info.png",rect(275,457,47,45));
  label(display->origin.value?"Reward collected":"Permanent companion",267,359,32,555);
  label("No further coin or XP rewards",337,455,29,485);
  canvas_.image("fish-popover/info.png",rect(251,531,45,44));label("Lives in a display slot",304,531,28,510,false,muted);
  action("fish-store",552,664,300,"Store","controls/stash.png",[this,id]{if(command({.action=Action::Stash,.fish=id})){inventoryCategory_=0;open(Panel::None);}});return;
 }
 label(content.care,292,272,34,458,false,content.condition==Care::Hungry&&!adult?hungry:fed);
 canvas_.image("fish-popover/growth-card.png",rect(237,328,619,189));
 canvas_.image("fish-popover/clock.png",rect(275,457,47,45));
 label(std::to_string(int(std::floor(content.progress*100)))+"%",807,314,34,70,true);
 constexpr std::array<const char*,5> names{"Baby","Junior","Young","Mature","Adult"};
 constexpr std::array<float,5> stops{282,415,545,674,807},labels{286,418,545,665,795};
 canvas_.image("fish-popover/growth-track.png",rect(282,401,525,25));
 // The reference spaces all five stage names evenly. Interpolate between those
 // stops so the bar reaches each label at its actual saved growth threshold.
 float trackProgress=0;
 for(int i=0;i<4;++i){const float start=visual.purchase.stages[i]/10000.f,end=visual.purchase.stages[i+1]/10000.f;
  if(content.progress>=start)trackProgress=(float(i)+std::clamp((content.progress-start)/(end-start),0.f,1.f))/4.f;
 }
 if(trackProgress>0){canvas_.clip(rect(282,398,525*trackProgress,32));canvas_.image("fish-popover/growth-track.png",rect(282,401,525,25),0,{.5f,.5f},1,false,{80,206,108,255});canvas_.clearClip();}
 for(int i=0;i<5;++i){
  label(names[i],labels[i],356,25,i==0?82:104,true,i==0||i>=3?ink:Color{11,108,158,255});
  const bool reached=!visual.egg&&i<=visual.age;canvas_.image(reached?"fish-popover/stage-current.png":"fish-popover/stage-next.png",rect(stops[i]-(reached?23.5f:18.5f),reached?390:395,reached?47:37,reached?49:38));
 }
 canvas_.boldText(content.growth,p.x+137*u,p.y+286*u,32*u,ink,false,484*u,.88f);
 canvas_.image("fish-popover/reward-card.png",rect(237,527,619,139));
 const auto reward=[&](std::string title,float y,FishReward value){
  label(title,261,y+6,30,186);canvas_.image("fish-popover/coin.png",rect(487,y,55,55));label(std::to_string(value.coins()),554,y+8,32,110);
  canvas_.image("fish-popover/xp.png",rect(672,y,54,53));label(std::to_string(value.xp)+" XP",735,y+8,31,101);
 };
 if(adult){reward("Keep",536,content.reward);reward(rehomeConfirm_?"Confirm rehome":"Rehome",602,content.reward);}
 else{reward(rehomeConfirm_?"Confirm rehome":"Rehome now",536,content.reward);reward("At adulthood",602,content.adultReward);}
 if(rehomeConfirm_){
  action("rehome-back",239,664,300,"Back","skin/control-next.png",[this]{rehomeConfirm_=false;rehomeQuote_.reset();});
  action("rehome-confirm",552,664,300,"Rehome","fish-popover/bag.png",[this,id]{
   const auto* current=session_.domain().fish(id);if(!current)return;
   auto quote=fishReward(*current);quote.xp=std::min(quote.xp,session_.domain().content().levels.back()-session_.domain().state().xp);
   if(!rehomeQuote_||quote.coins()!=rehomeQuote_->coins()||quote.xp!=rehomeQuote_->xp){rehomeQuote_=quote;return;}
   if(command({.action=Action::Sell,.fish=id}))open(Panel::None);
  });
 }else{
  if(adult)action("fish-keep",239,664,300,"Keep","ui/keep.png",[this,id]{
   if(command({.action=Action::Keep,.fish=id})){rehomeConfirm_=false;if(const auto* kept=session_.domain().companion(id);kept&&kept->stored)open(Panel::None);}
  });
  else{
   canvas_.image("fish-popover/info.png",rect(251,690,45,44));label(visual.favorite?"Unfavorite to rehome":"Frees 1 growing slot",304,695,27,232,false,muted);
  }
  action("fish-rehome",552,664,300,visual.egg?"Cancel egg":"Rehome","fish-popover/bag.png",[this,id]{
   if(const auto* current=session_.domain().fish(id);current&&!current->favorite){rehomeConfirm_=true;rehomeQuote_=fishReward(*current);rehomeQuote_->xp=std::min(rehomeQuote_->xp,session_.domain().content().levels.back()-session_.domain().state().xp);}
  });
 }
}
} // namespace aq
