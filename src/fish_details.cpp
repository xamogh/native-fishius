#include "aquarium/fish_details.hpp"
#include <algorithm>

namespace aq {
namespace {
std::string remaining(Millis ms){
 const auto seconds=(std::max<Millis>(0,ms)+999)/1000;
 if(seconds>=3600)return std::to_string(seconds/3600)+"h "+std::to_string(seconds/60%60)+"m";
 if(seconds>=60)return std::to_string(seconds/60)+"m "+std::to_string(seconds%60)+"s";
 return std::to_string(seconds)+"s";
}
}
FishDetailsContent fishDetailsContent(const Species& s,const Fish& f,Millis now){
 FishDetailsContent out;out.title=s.name;out.stage=f.egg?-1:f.age;out.condition=careOf(s,f,now);out.progress=float(growthProgress(f));out.reward=fishReward(f);out.adultReward={f.purchase.principal,f.purchase.profit,f.scripted?0:f.purchase.xp};
 out.care=f.egg?"Egg":f.age==4?"Adult · Ready":out.condition==Care::Fed?"Fed · Growing":"Hungry · Paused";
 out.growth=f.egg?"Hatches in "+remaining(f.hatchAt-now):f.age==4?"Collect once":remaining(f.purchase.durationMs-f.growthMs)+" of growth left";
 out.sale=f.egg||f.age<1?"Selling unlocks at Junior":std::to_string(out.reward.coins())+" coins + "+std::to_string(out.reward.xp)+" XP";return out;
}
}
