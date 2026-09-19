#include "aquarium/domain.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace aq {
namespace {
constexpr Amount maximum=9'000'000'000'000'000LL;
Amount ratio(Amount amount,int bps){return amount/10000*bps+(amount%10000*bps)/10000;}
// Cancel rational factors before multiplying. This gives the same half-up
// integer result on every target, without floating-point rounding boundaries.
Amount product(std::vector<Amount> n,std::vector<Amount> d){
 if(std::find(n.begin(),n.end(),Amount{0})!=n.end())return 0;
 for(auto& denominator:d)for(auto& numerator:n){const auto g=std::gcd(numerator,denominator);numerator/=g;denominator/=g;}
 Amount numerator=1,denominator=1;
 for(auto value:n){if(value<0||value>maximum/numerator)throw std::overflow_error("Quote overflow");numerator*=value;}
 for(auto value:d){if(value<=0||value>maximum/denominator)throw std::overflow_error("Quote denominator overflow");denominator*=value;}
 return numerator/denominator+(numerator%denominator>=(denominator+1)/2);
}
Result replay(const Json& j){
 return {.fish={j.at("fish").get<std::uint64_t>()},.coins=j.at("coins"),.xp=j.at("xp"),.pearls=j.at("pearls"),
         .message=j.value("message",std::string{}),.replayed=true,.revision=j.value("revision",std::uint64_t{})};
}
Json receipt(const Result& r){return {{"fish",r.fish.value},{"coins",r.coins},{"xp",r.xp},{"pearls",r.pearls},{"message",r.message},{"revision",r.revision}};}
}

Money treasureContents(const Content& content,const TreasureOffer& offer,int level){
 if(level<1||level>40)throw std::invalid_argument("Invalid Treasure offer level");
 const Amount slots=content.referenceSlotsByLevel.at(level-1);
 const auto& e=content.economy;
 return {product({e.baseCoinDay,10000+e.coinSlopeBps*(level-1),slots,content.referenceUtilizationNumerator,offer.coinDaysBps},
                 {10000,content.referenceUtilizationDenominator,10000}),offer.pearls};
}

GrowthSnapshot purchaseQuote(const Content& content,const Species& s,int level,bool gift){
 if(level<1||level>40)throw std::invalid_argument("Invalid purchase level");
 GrowthSnapshot q;q.level=level;q.configVersion=content.configVersion;q.scheduleId=s.scheduleId;
 const auto& e=content.economy;q.durationMs=s.durationMs;q.feedMs=s.feedMs;q.stages=e.stages;q.rewards=e.rewards;q.earlyRefundBps=e.earlyRefundBps;
 q.profit=product({e.baseCoinDay,10000+e.coinSlopeBps*(level-1),s.durationMs,s.coinFactorBps,s.coinWeightBps},{86400000,10000,10000,10000});
 q.xp=product({e.baseXpDay,10000+e.xpSlopeBps*(level-1),s.durationMs,s.xpFactorBps,s.xpWeightBps},{86400000,10000,10000,10000});
 // Sale principal only returns coins actually paid for the egg.
 q.principal=gift||s.currency!=Currency::Coins?0:std::max(e.minimumPrice,product({q.profit,e.principalShareBps},{10000}));return q;
}
int growthStage(const GrowthSnapshot& q,Millis elapsed){
 int stage=0;for(int i=1;i<5;++i)if(elapsed>=q.durationMs/10000*q.stages[i]+q.durationMs%10000*q.stages[i]/10000)stage=i;return stage;
}
double growthProgress(const Fish& f){return f.purchase.durationMs>0?std::clamp(double(f.growthMs)/double(f.purchase.durationMs),0.,1.):1.;}
double nextStageProgress(const Fish& f){
 if(f.egg){const auto hatch=f.hatchAt-f.boughtAt;return hatch>0?std::clamp(double(f.growthMs)/double(hatch),0.,1.):0.;}
 if(f.age>=4||f.purchase.durationMs<=0)return 1.;
 const auto& q=f.purchase;const int stage=std::clamp(f.age,0,3);
 // Use the saved, uneven stage thresholds and the same rounding as growthStage.
 const auto boundary=[&](int i){return q.durationMs/10000*q.stages[i]+q.durationMs%10000*q.stages[i]/10000;};
 const auto start=boundary(stage),end=boundary(stage+1);
 return end>start?std::clamp(double(f.growthMs-start)/double(end-start),0.,1.):1.;
}
FishReward fishReward(const Fish& f){
 const auto& q=f.purchase;
 // Retain the old cancellation quote for validating historical settlements.
 // New egg and Baby sales are rejected by settle before any payout.
 if(f.egg||f.age==0)return {q.principal,0,0};
 if(f.age==4)return {q.principal,q.profit,f.scripted?0:q.xp};
 return {ratio(q.principal,q.earlyRefundBps),ratio(q.profit,q.rewards.at(f.age)),f.scripted?0:ratio(q.xp,q.rewards.at(f.age))};
}
bool pearlSaleEligible(const Content& content,const Fish& fish){
 const auto* species=content.find(fish.species);
 return species&&species->currency==Currency::Coins&&!fish.egg&&fish.age==4&&!fish.scripted&&
        (fish.purchase.profit>0||fish.purchase.xp>0);
}
Amount Domain::adultCoinSales()const{
 const auto it=state_.totalEvents.find("adult-coin-sale");return it==state_.totalEvents.end()?0:it->second;
}
const TankEntitlement* Domain::nextTankEntitlement(TankId id)const{
 const auto* owned=tank(id);const int current=owned?owned->slots:0;
 const TankEntitlement* next=nullptr;for(const auto& e:content_.tankEntitlements)if(e.tank==id&&e.slots>current&&(!next||e.slots<next->slots))next=&e;return next;
}
void Domain::ledger(std::string currency,Amount delta,Amount balance,std::string reason,std::string source){
 if(!delta)return;if(state_.ledger.size()>=200000)throw std::overflow_error("Ledger storage exhausted");
 state_.ledger.push_back({{"id",state_.ledger.size()+1},{"currency",currency},{"amount",delta},{"balance_after",balance},
     {"reason",reason},{"source_id",source},{"request_id",requestId_},{"created_at",state_.calendarNow},{"revision",state_.revision+1}});
}
Result Domain::spend(Amount coins,Amount pearls){
 if(auto r=requireFunds(coins,pearls);!r)return r;
 state_.wallet.coins-=coins;state_.wallet.pearls-=pearls;
 ledger("coins",-coins,state_.wallet.coins,reason_,source_);ledger("pearls",-pearls,state_.wallet.pearls,reason_,source_);return {};
}
Result Domain::settle(const Command& c){
 const std::string id=std::to_string(c.fish.value);
 if(state_.settlements.contains(id))return replay(state_.settlements.at(id).at("result"));
 auto* f=mutableFish(c.fish);if(!f||f->stashed||f->tank!=state_.activeTank)return {.error=Error::InvalidFish};
 if(f->favorite)return {.error=Error::Protected,.message="Unfavorite this fish before selling it."};
 if(f->egg||f->age<1)return {.error=Error::NotReady,.message="Selling unlocks at Junior."};
 const auto fishCopy=*f;const auto reward=fishReward(fishCopy);
 const bool eligible=pearlSaleEligible(content_,fishCopy);
 if(eligible&&adultCoinSales()>=maximum)throw std::overflow_error("Pearl progress exhausted");
 const Amount saleNumber=eligible?adultCoinSales()+1:0;
 const Amount pearls=eligible&&saleNumber%content_.pearlSalesTarget==0?content_.pearlSalesReward:0;
 reason_="fish_settlement";source_=id;
 auto result=grant(reward.coins(),reward.xp,pearls);if(!result)return result;
 result.fish=c.fish;result.coins=reward.coins();
 result.message="Fish sold.";
 if(fishCopy.age==4&&!fishCopy.scripted&&(reward.coins()>0||reward.xp>0)){++state_.adultRaised[fishCopy.species];count("adult-settlement");}
 if(eligible)count("adult-coin-sale");
 std::erase_if(state_.fish,[&](const auto& fish){return fish.id==c.fish;});
 result.revision=state_.revision+1;
 state_.settlements[id]={{"result",receipt(result)},{"species",fishCopy.species},{"stage",fishCopy.age},{"egg",fishCopy.egg},{"scripted",fishCopy.scripted},
     {"disposition","rehome"},{"principal",reward.principal},{"profit",reward.profit},
     {"purchase",fishCopy.purchase},{"request_id",requestId_},{"at",state_.calendarNow}};
 // Freeze the milestone rule with the settlement, just like the fish quote.
 // Old settlements have no such field and keep their original zero pearls.
 if(eligible)state_.settlements[id]["pearl_reward"]={{"sale",saleNumber},{"target",content_.pearlSalesTarget},{"amount",content_.pearlSalesReward}};
 count("sell");emit({"sale",c.fish,fishCopy.position,reward.coins(),result.xp,pearls,result.message});return result;
}

Result Domain::execute(const Command& c){return execute(c,[](const State&){return true;});}
Result Domain::execute(const Command& supplied,const std::function<bool(const State&)>& commit){
 if(int(supplied.action)==33)return {.error=Error::Unavailable,.message="Favorite the fish to protect it from selling."};
 const bool recorded=supplied.action!=Action::Move&&supplied.action!=Action::DropFood;
 Command c=supplied;
 if(c.requestId.size()>128)return {.error=Error::Conflict,.message="Invalid request identity."};
 const Json fingerprint={{"action",int(c.action)},{"fish",c.fish.value},{"tank",c.tank.value},{"key",c.key},
     {"point",{c.point.x,c.point.y}},{"currency",int(c.currency)},{"value",c.value},{"decor",c.decor},
     {"offer",c.offer?Json(*c.offer):Json(nullptr)}};
 if(recorded&&!c.requestId.empty()&&state_.receipts.contains(c.requestId)){
  const auto& previous=state_.receipts.at(c.requestId);
  if(previous.at("command")!=fingerprint)return {.error=Error::Conflict,.message="This request was already used for another action."};
  return replay(previous.at("result"));
 }
 State original=state_;auto oldPellets=pellets_;auto oldRipples=ripples_;auto oldEvents=events_;auto oldNext=nextPellet_;
 auto rollback=[&]{state_=std::move(original);pellets_=std::move(oldPellets);ripples_=std::move(oldRipples);events_=std::move(oldEvents);nextPellet_=oldNext;configureExtras();};
 try{
  if(recorded){
   if(state_.receipts.size()>=50000||state_.nextRequestId==UINT64_MAX)throw std::overflow_error("Receipt storage exhausted");
   if(c.requestId.empty())do{if(state_.nextRequestId==UINT64_MAX)throw std::overflow_error("Request sequence exhausted");c.requestId="local:"+std::to_string(state_.nextRequestId++);}while(state_.receipts.contains(c.requestId));
   if(state_.revision==UINT64_MAX)throw std::overflow_error("Revision exhausted");
  }
  requestId_=c.requestId;reason_="action:"+std::to_string(int(c.action));source_=c.fish.value?std::to_string(c.fish.value):c.key;
  auto result=executeImpl(c);if(!result){rollback();return result;}
  if(recorded){result.revision=++state_.revision;state_.receipts[c.requestId]={{"command",fingerprint},{"result",receipt(result)}};}
  if(!commit(state_)){rollback();return {.error=Error::SaveFailure,.message="The action was not saved. Please retry."};}
  return result;
 }catch(const std::overflow_error&){rollback();return {.error=Error::Overflow};}
 catch(...){rollback();throw;}
}
} // namespace aq
