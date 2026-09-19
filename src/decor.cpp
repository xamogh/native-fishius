#include "aquarium/domain.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace aq {
namespace {
Result fail(Error e,const char* message=nullptr){return {.error=e,.message=message?message:errorText(e)};}
bool owns(const State& s,std::string_view id){return std::find(s.decorOwned.begin(),s.decorOwned.end(),id)!=s.decorOwned.end();}
DecorDef legacyDimensions(){DecorDef d;d.width=1.1;d.height=.95;return d;}
}
const DecorDef* Content::findDecor(std::string_view id)const{
 const auto it=std::find_if(decorations.begin(),decorations.end(),[&](const auto& d){return d.id==id;});return it==decorations.end()?nullptr:&*it;
}
void Content::configureDecorEvents(const Json& configuration){
 if(!configuration.is_array()||configuration.size()>12)throw std::invalid_argument("Invalid decor event configuration");
 auto candidate=decorEvents;std::set<std::string> names;
 for(const auto& record:configuration){
  const auto name=record.at("name").get<std::string>();
  auto it=std::find_if(candidate.begin(),candidate.end(),[&](const auto& event){return event.name==name;});
  if(it==candidate.end()||!names.insert(name).second)throw std::invalid_argument("Unknown or duplicate decor event");
  const auto start=record.at("starts_at").get<Millis>(),end=record.at("ends_at").get<Millis>();
  if(start<=0||end<=start||end>9'000'000'000'000'000LL)throw std::invalid_argument("Invalid decor event timestamps");
  it->startsAt=start;it->endsAt=end;it->configured=true;
 }
 decorEvents=std::move(candidate);
}
const Decoration* Domain::decoration(std::uint64_t id)const{
 const auto it=std::find_if(state_.decor.begin(),state_.decor.end(),[&](const auto& d){return d.id==id;});return it==state_.decor.end()?nullptr:&*it;
}
std::size_t Domain::placedDecor(TankId tankId)const{return std::count_if(state_.decor.begin(),state_.decor.end(),[&](const auto& d){return d.tank==tankId&&!d.stored;});}
int Domain::decorScore(TankId tankId)const{
 std::set<std::string> seen;int result=0;for(const auto& d:state_.decor)if(d.tank==tankId&&!d.stored&&seen.insert(d.kind).second)if(const auto* def=content_.findDecor(d.kind))result+=def->score;return result;
}
bool Domain::decorVisible(const DecorDef& d)const{return !d.legacy&&d.level<=content_.decorTuning.launchLevelCap;}
Amount Domain::decorPurchaseXp(const DecorDef& def)const{return owns(state_,def.id)?0:def.buyXp;}
bool inPlacementWater(WorldPoint p){return inTank(p);}
bool inTank(WorldPoint p){return std::isfinite(p.x)&&std::isfinite(p.y)&&p.x>=0&&p.x<=waterWidth&&p.y>=0&&p.y<=tankHeight;}
WorldPoint decorPlacementPoint(WorldPoint p){return {std::clamp(p.x,0.,waterWidth),std::clamp(p.y,0.,tankHeight)};}
double decorDepth(WorldPoint p){
 // Preserve the existing perspective, without using it to restrict placement.
 const auto at=decorPlacementPoint(p);const double ny=std::clamp((at.y-decorReferenceHeight*.3)/(decorReferenceHeight*(.985-.3)),0.,1.);
 const double edge=std::min(1.,std::abs(at.x/waterWidth-.5)/.5);
 return std::clamp(std::pow(ny,.82)*(1-.44*std::pow(edge,1.6)),0.,1.);
}
bool decorBehind(const Decoration& a,const Decoration& b){
 // Layer by the bottom anchor. Sideways perspective changes size, not order.
 if(a.position.y!=b.position.y)return a.position.y<b.position.y;
 // Identity keeps copies with equal base heights stable during a move or reload.
 return a.id<b.id;
}
double decorScale(WorldPoint p){return tankArtScale*(.42+(1.18-.42)*decorDepth(p));}
double decorHaze(WorldPoint p){return (1-decorDepth(p))*.26;}
WorldPoint decorPlacementPoint(const DecorDef& d,WorldPoint p,double sizeMul){
 p=decorPlacementPoint(p);
 // Match the smaller of the authored scene's width/12 and height/7.
 // The view also fits active placements within the currently visible crop.
 // Moving an anchor inward can increase its perspective scale, so repeat
 // until the larger footprint also fits. Oversized art stays bottom aligned.
 for(int i=0;i<256;++i){
  const double scale=decorScale(p)*sizeMul;
  const double halfWidth=std::min(waterWidth*.5,d.width*waterWidth/12.*scale*.5);
  const double height=std::min(tankHeight,d.height*tankHeight/7.*scale);
  const WorldPoint next{std::clamp(p.x,halfWidth,waterWidth-halfWidth),std::clamp(p.y,height,tankHeight)};
  if(next.x==p.x&&next.y==p.y)return p;
  p=next;
 }
 return p;
}
bool Domain::validDecorPoint(const DecorDef&,WorldPoint p)const{
 // A tap inside the tank is accepted; placement adjusts its anchor so the
 // decoration's dimensions, perspective and custom size stay visible.
 return inTank(p);
}
Result Domain::blocker(const DecorDef& d)const{
 if(!state_.pendingDecor.empty())return fail(Error::NotReady,"Place or store your purchased decoration first.");
 if(d.level>content_.decorTuning.launchLevelCap)return fail(Error::Level,"Planned for a future release.");
 if(!decorVisible(d))return fail(Error::NotReady,"This item is no longer sold in the shop.");
 if(level()<d.level)return fail(Error::Level);
 if(d.edition=="Limited Edition"){
  const auto it=std::find_if(content_.decorEvents.begin(),content_.decorEvents.end(),[&](const auto& e){return e.name==d.event;});
  if(it==content_.decorEvents.end()||!it->configured)return fail(Error::EventClosed,"Event dates have not been scheduled.");
  if(state_.calendarNow<it->startsAt||state_.calendarNow>=it->endsAt)return fail(Error::EventClosed);
 }
 if(!d.artReady)return fail(Error::NoArt);
 if(placedDecor(state_.activeTank)>=static_cast<std::size_t>(content_.decorTuning.placedLimit))return {.error=Error::Maximum,.message="Store a decoration first. This tank has "+std::to_string(content_.decorTuning.placedLimit)+" placed items."};
 if(state_.decor.size()>=500)return fail(Error::Maximum);
 return requireFunds(d.currency==Currency::Pearls?0:d.price,d.currency==Currency::Pearls?d.price:0);
}
Result Domain::executeDecor(const Command& command){
 if(command.action==Action::CancelDecor){
  if(state_.pendingDecor.empty())return {};
  const auto* def=content_.findDecor(state_.pendingDecor);if(!def)return fail(Error::Unknown);
  if(state_.decor.size()>=500)return fail(Error::Maximum);
  if(state_.nextDecorId==std::numeric_limits<std::uint64_t>::max())return fail(Error::Overflow);
  state_.decor.push_back({state_.nextDecorId++,def->id,state_.activeTank,decorPlacementPoint(*def,{waterWidth*.5,tankHeight}),true});
  state_.pendingDecor.clear();return {};
 }
 if(command.action==Action::PlaceDecor){
  const auto* def=content_.findDecor(state_.pendingDecor);if(!def)return fail(Error::NotReady);
  if(!validDecorPoint(*def,command.point))return fail(Error::InvalidPosition);
  if(placedDecor(state_.activeTank)>=static_cast<std::size_t>(content_.decorTuning.placedLimit)||state_.decor.size()>=500)return fail(Error::Maximum);
  if(state_.nextDecorId==std::numeric_limits<std::uint64_t>::max())return fail(Error::Overflow);
  const auto p=decorPlacementPoint(*def,command.point);
  state_.decor.push_back({state_.nextDecorId++,def->id,state_.activeTank,p});state_.pendingDecor.clear();
  count("decor");emit({"decor",{},p,0,0,0,"Placed "+def->name});return {};
 }
 if(command.action==Action::BuyDecor||command.action==Action::PurchaseDecor){
  const bool queued=command.action==Action::PurchaseDecor;
  const auto* found=content_.findDecor(command.key);if(!found)return fail(Error::Unknown);
  // grant() may refresh presentation catalogs. The authoritative content is stable.
  const auto& def=*found;const auto gate=blocker(def);if(!gate)return gate;
  if(!queued&&!validDecorPoint(def,command.point))return fail(Error::InvalidPosition);
  if(state_.nextDecorId==std::numeric_limits<std::uint64_t>::max())return fail(Error::Overflow);
  const Amount xp=decorPurchaseXp(def);
  const bool intro=!state_.decorOnboardingComplete&&def.id==content_.decorTuning.tutorialItem;
  if(intro&&std::find(state_.tutorialClaims.begin(),state_.tutorialClaims.end(),"bonus:decor")==state_.tutorialClaims.end()){
   state_.tutorialClaims.push_back("bonus:decor");
  }
  reason_="decor_purchase";source_=std::to_string(state_.nextDecorId);if(auto paid=spend(def.currency==Currency::Coins?def.price:0,def.currency==Currency::Pearls?def.price:0);!paid)return paid;
  auto result=grant(0,xp);if(!result)return result;
  const auto position=queued?WorldPoint{544,240}:decorPlacementPoint(def,command.point);
  if(queued)state_.pendingDecor=def.id;else state_.decor.push_back({state_.nextDecorId++,def.id,state_.activeTank,position});
  if(!owns(state_,def.id))state_.decorOwned.push_back(def.id);
  if(intro)state_.decorOnboardingComplete=true;
  if(!queued)count("decor");emit({"decor",{},position,def.currency==Currency::Coins?-def.price:0,result.xp,def.currency==Currency::Pearls?-def.price:0,(queued?"Bought ":"Placed ")+def.name});
  result.coins=def.currency==Currency::Coins?-def.price:0;result.pearls-=def.currency==Currency::Pearls?def.price:0;return result;
 }
 auto it=std::find_if(state_.decor.begin(),state_.decor.end(),[&](const auto& d){return d.id==command.decor;});
 if(it==state_.decor.end())return fail(Error::Unknown);
 if(command.action==Action::ResizeDecor){
  if(it->stored||it->tank!=state_.activeTank)return fail(Error::NotReady);
  if(!std::isfinite(command.value))return fail(Error::InvalidPosition);
  const double size=std::clamp(it->sizeMul+command.value,decorSizeMin,decorSizeMax);
  if(size!=it->sizeMul){
   const auto* def=content_.findDecor(it->kind);const auto legacy=legacyDimensions();
   it->sizeMul=size;it->position=decorPlacementPoint(def?*def:legacy,it->position,size);count("decor");
  }
  return {};
 }
 if(command.action==Action::StoreDecor){if(it->stored||it->tank!=state_.activeTank)return fail(Error::NotReady);it->stored=true;return {};}
 if(command.action==Action::FlipDecor){if(it->stored||it->tank!=state_.activeTank)return fail(Error::NotReady);it->flipped=!it->flipped;count("decor");return {};}
 if(command.action==Action::MoveDecor||command.action==Action::RestoreDecor){
  const bool restoring=command.action==Action::RestoreDecor;
  if(restoring!=it->stored||(!restoring&&it->tank!=state_.activeTank))return fail(Error::NotReady);
  const TankId destination=command.tank;
  if(!tank(destination))return fail(Error::Unknown);
  if((restoring||destination!=it->tank)&&placedDecor(destination)>=static_cast<std::size_t>(content_.decorTuning.placedLimit))return fail(Error::Maximum);
  if(!std::isfinite(command.point.x)||!std::isfinite(command.point.y))return fail(Error::InvalidPosition);
  const auto* def=content_.findDecor(it->kind);const auto legacy=legacyDimensions();
  const auto position=decorPlacementPoint(def?*def:legacy,command.point,it->sizeMul);
  const bool changed=restoring||it->tank!=destination||it->position.x!=position.x||it->position.y!=position.y;
  it->tank=destination;it->stored=false;it->position=position;if(changed)count("decor");return {};
 }
 return fail(Error::Unknown);
}
}
