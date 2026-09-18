#include "aquarium/domain.hpp"
#include <algorithm>

namespace aq {
std::span<const EnvironmentStyle> environmentCatalog(){
 static const std::array<EnvironmentStyle,2> styles{{
  {"sunlit-lagoon","Sunlit Lagoon",0,"environment/sunlit-lagoon.png"},
  {"coral-garden","Coral Garden",1200,"environment/coral-garden.png"}
 }};
 return styles;
}
const EnvironmentStyle* findEnvironment(std::string_view id){
 for(const auto& style:environmentCatalog())if(style.id==id)return &style;
 return nullptr;
}
bool Domain::ownsEnvironment(std::string_view id)const{
 const auto* style=findEnvironment(id);
 return style&&(style->price==0||std::find(state_.environmentOwned.begin(),state_.environmentOwned.end(),id)!=state_.environmentOwned.end());
}
Result Domain::executeEnvironment(const Command& c){
 const auto* style=findEnvironment(c.key);
 if(!style)return {.error=Error::Unknown};
 auto tank=std::find_if(state_.tanks.begin(),state_.tanks.end(),[&](const auto& t){return t.id==c.tank;});
 if(tank==state_.tanks.end())return {.error=Error::Unavailable,.message="Unlock this tank first."};
 const bool owned=ownsEnvironment(style->id);
 if(!owned){
  if(c.action!=Action::PurchaseEnvironment)return {.error=Error::NotReady,.message="Buy this background before using it."};
  reason_="environment_purchase";source_=style->id;
  if(auto result=spend(style->price);!result)return result;
  state_.environmentOwned.push_back(style->id);
 }
 if(c.action==Action::EquipEnvironment){
  tank->backgroundId=style->id;
  return {.message="Background applied."};
 }
 return {.message=owned?"Background already owned.":"Background purchased."};
}
}
