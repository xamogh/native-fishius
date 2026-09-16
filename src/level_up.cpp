#include "aquarium/theme.hpp"
#include <algorithm>

namespace aq {
std::vector<LevelUnlock> levelUnlocks(const Domain& domain,int level){
 std::vector<LevelUnlock> result;
 const auto& content=domain.content();const auto calendar=calendarAt(domain.state().calendarNow);
 for(const auto& fish:content.species)
  if(fish.level==level&&fish.releaseGate=="Launch"&&fish.artReady&&eventOpen(fish,calendar))
   result.push_back({fish.name,fishArt(fish.id),fish.id,0});
 for(const auto& item:domain.decorations()){
  if(item.level!=level||!item.artReady||!domain.decorVisible(item))continue;
  if(item.edition=="Limited Edition"){
   const auto event=std::find_if(content.decorEvents.begin(),content.decorEvents.end(),[&](const auto& e){return e.name==item.event;});
   if(event==content.decorEvents.end()||!event->configured||domain.state().calendarNow<event->startsAt||domain.state().calendarNow>=event->endsAt)continue;
  }
  result.push_back({item.name,"decor/icons/"+item.id+".png",item.id,item.category=="Plant"?1:2});
 }
 for(std::size_t i=1;i<content.tankLevels.size();++i)
  if(content.tankLevels[i]==level)result.push_back({"Tank "+std::to_string(i+1),"skin/tank-"+std::to_string(i+1)+".png","tank-"+std::to_string(i+1),-1,{int(i+1)}});
 return result;
}

void View::dismissLevelUp(){
 if(levelUps_.empty())return;
 cancelGesture();buttons_.clear();levelUps_.pop_front();levelUnlockPage_=0;
 if(levelUps_.empty())levelAnimation_.motion.show(false,now_);
 else{levelAnimation_={};levelAnimation_.motion.show(true,now_);}
}

void View::levelUp(){
 const auto& reward=levelUps_.front();
 const auto items=levelUnlocks(session_.domain(),reward.reachedLevel);
 const auto safe=canvas_.safeInsets();
 const float width=canvas_.width()-safe.left-safe.right,height=canvas_.height()-safe.top-safe.bottom;
 // Cap the dialog in display points as well as fitting the safe viewport.
 const float u=std::min({720.f*(canvas_.minimumTouchSize()/44.f)/1080.f,width*.9f/1080.f,height*.88f/720.f});
 const Rect p{safe.left+(width-1080*u)*.5f,safe.top+(height-720*u)*.5f,1080*u,720*u};
 levelAnimation_.bounds=p;
 const auto rect=[&](float x,float y,float w,float h){return Rect{p.x+x*u,p.y+y*u,w*u,h*u};};
 const auto label=[&](std::string_view value,float x,float y,float size,float width,Color color=theme::ink,float stroke=0){
  canvas_.label(value,p.x+x*u,p.y+y*u,size*u,true,width*u,color,stroke*u,{9,46,127,255});
 };
 const auto target=[&](std::string id,Rect r,std::function<void()> action){
  const float ex=std::max(0.f,(canvas_.minimumTouchSize()-r.w)*.5f),ey=std::max(0.f,(canvas_.minimumTouchSize()-r.h)*.5f);
  touchTarget(std::move(id),{r.x-ex,r.y-ey,r.w+2*ex,r.h+2*ey},std::move(action));
 };
 canvas_.skin("panel",rect(22,85,1036,616));
 titleSign(rect(255,26,570,118),"Level up!");
 closeButton("level-close",rect(965,96,65,65),[this]{dismissLevelUp();});
 canvas_.icon("skin/icon-star.png",rect(160,157,169,157));
 label("Lv "+std::to_string(reward.reachedLevel),244,218,39,141);
 const auto rewardPill=[&](float x,std::string_view icon,Amount amount,Color color,Color border){
  (void)color;(void)border;canvas_.skin("card",rect(x,190,253,87));
  canvas_.icon(icon,rect(x+15,201,62,64));
  label("+"+compact(amount),x+160,207,42,153,{16,51,113,255},0);
 };
 rewardPill(384,"skin/icon-coin.png",reward.coins,{255,241,173,255},{255,200,49,255});
 rewardPill(661,"skin/icon-pearl.png",reward.pearls,{226,211,255,255},{189,142,255,255});

 const int pages=std::max(1,(int(items.size())+3)/4);
 levelUnlockPage_=std::clamp(levelUnlockPage_,0,pages-1);
 const int first=levelUnlockPage_*4,count=std::min(4,int(items.size())-first);
 constexpr float cardWidth=218,gap=14;
 const float left=540-(count*cardWidth+(count-1)*gap)*.5f;
 for(int i=0;i<count;++i){
  const auto& item=items[first+i];const float x=left+i*(cardWidth+gap);
  const std::string id="level-item-"+item.id;
  const auto r=buttonVisual(id,rect(x,334,cardWidth,226));
  canvas_.skin("card",r);
  std::size_t split=std::string::npos;
  if(item.name.size()>18){
   const auto middle=item.name.size()/2,after=item.name.find(' ',middle),before=item.name.rfind(' ',middle);
   split=before!=std::string::npos&&(after==std::string::npos||middle-before<after-middle)?before:after;
  }
  if(split==std::string::npos)label(item.name,x+109,352,27,198);
  else{label(item.name.substr(0,split),x+109,346,25,198);label(item.name.substr(split+1),x+109,375,25,198);}
  canvas_.icon(item.art,rect(x+15,416,188,126));
  target(id,rect(x,334,cardWidth,226),[this,item]{openLevelUnlock(item);});
 }
 if(pages>1){
  const auto arrow=[&](bool next){
   const std::string id=next?"level-next":"level-prev";const auto area=rect(next?861:151,598,67,73);
   canvas_.icon("skin/control-next.png",buttonVisual(id,area),1,!next);
   target(id,area,[this,next]{levelUnlockPage_+=next?1:-1;buttons_.clear();});
  };
  if(levelUnlockPage_>0)arrow(false);
  if(levelUnlockPage_<pages-1)arrow(true);
  label(std::to_string(levelUnlockPage_+1)+" / "+std::to_string(pages),540,566,21,180);
 }
 glassButton("level-continue",rect(380,603,320,79),"CONTINUE",[this]{dismissLevelUp();},"green",39*u);

}
} // namespace aq
