#include "aquarium/domain.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
namespace aq {
namespace {
constexpr Amount limit=9'000'000'000'000'000LL;
constexpr double pi=3.14159265358979323846;
bool addOk(Amount a,Amount b){return a>=0&&b>=0&&a<=limit-b;}
bool validPoint(WorldPoint p){return inTank(p);}
bool contains(const std::vector<std::string>& v,std::string_view s){return std::find(v.begin(),v.end(),s)!=v.end();}
Result bad(Error e){return {.error=e,.message=errorText(e)};}
std::string claimKey(const Species& s,const Calendar& c){return s.id+(s.annual?":"+std::to_string(c.year):"");}
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
}
const Species* Content::find(std::string_view id)const{auto i=std::find_if(species.begin(),species.end(),[&](auto& s){return s.id==id;});return i==species.end()?nullptr:&*i;}
Content Content::fromJson(const Json& j){
 Content c;check(j.at("schema")==4,"Expected v4 content schema");c.configVersion=j.at("config_version");check(!c.configVersion.empty()&&c.configVersion.size()<128,"Missing config version");
 const auto& e=j.at("economy");c.economy={e.at("base_coin_day"),e.at("base_xp_day"),e.at("minimum_price"),e.at("coin_slope_bps"),e.at("xp_slope_bps"),e.at("principal_share_bps"),e.at("early_refund_bps"),e.at("stage_bps").get<std::array<int,5>>(),e.at("reward_bps").get<std::array<int,5>>()};
 check(c.economy.baseCoinDay>0&&c.economy.baseCoinDay<=1000000&&c.economy.baseXpDay>0&&c.economy.baseXpDay<=1000000&&c.economy.minimumPrice>0&&c.economy.minimumPrice<=limit&&c.economy.coinSlopeBps>=0&&c.economy.coinSlopeBps<=10000&&c.economy.xpSlopeBps>=0&&c.economy.xpSlopeBps<=10000,"Invalid economy rates");
 check(c.economy.principalShareBps>0&&c.economy.principalShareBps<=10000&&c.economy.earlyRefundBps>=0&&c.economy.earlyRefundBps<=10000,"Invalid principal shares");
 for(const auto& values:{c.economy.stages,c.economy.rewards})check(values.front()==0&&values.back()==10000&&std::adjacent_find(values.begin(),values.end(),std::greater_equal<int>())==values.end(),"Invalid stage fractions");
 for(const auto& x:j.at("species")){
  Species s;s.id=x.at("id");s.modelId=x.at("model_id");s.name=x.at("name");s.rarity=x.at("rarity");s.role=x.value("role","");
  s.level=x.at("level");std::string currency=x.at("currency");s.currency=currency=="pearls"?Currency::Pearls:currency=="gift"?Currency::Gift:Currency::Coins;
  check(currency=="coins"||currency=="pearls"||currency=="gift","Invalid fish currency");
  s.price=x.at("price");s.buyXp=x.at("buy_xp");s.durationMs=x.at("duration_ms");s.stageMs=s.durationMs;s.feedMs=x.at("feed_ms");
  s.releaseGate=x.at("release_gate");s.scheduleId=x.at("schedule_id");s.companion=x.at("companion");s.artReady=x.at("art_ready");s.coinWeightBps=x.at("coin_weight_bps");s.xpWeightBps=x.at("xp_weight_bps");s.coinFactorBps=x.at("coin_factor_bps");s.xpFactorBps=x.at("xp_factor_bps");
  s.saleCoins=x.at("sale_coins").get<std::array<Amount,5>>();s.saleXp=x.at("sale_xp").get<std::array<Amount,5>>();
  s.badge=x.value("badge","");s.description=x.value("description","");s.nominalLength=x.value("length",38.);s.asset="species/"+s.id+".png";
  s.eventStart=x.value("event_start",0);s.eventEnd=x.value("event_end",0);s.eventConfigured=x.value("event_configured",true);s.annual=x.value("annual",false);s.oneTime=x.value("one_time",false);s.nonResellable=x.value("non_resellable",false);
  check(s.companion?s.durationMs==0:s.durationMs>=1200000&&s.durationMs<=158400000,"Invalid total growth duration");
  check(s.feedMs>0&&s.feedMs<=43200000&&s.price>=0&&s.price<=limit&&s.buyXp==0&&s.level>=1&&s.level<=70,"Invalid price or meal duration");
  if(s.companion)check(std::all_of(s.saleCoins.begin(),s.saleCoins.end(),[](auto n){return n==0;})&&std::all_of(s.saleXp.begin(),s.saleXp.end(),[](auto n){return n==0;}),"Companion cannot produce rewards");
  else{
   check(!s.scheduleId.empty()&&s.scheduleId.size()<128&&s.currency==Currency::Coins&&s.coinWeightBps>=9000&&s.coinWeightBps<=11000&&s.xpWeightBps>=9000&&s.xpWeightBps<=11000&&s.coinFactorBps>0&&s.coinFactorBps<=15000&&s.xpFactorBps>0&&s.xpFactorBps<=15000,"Invalid production weights");
   const auto& schedules=j.at("schedules");const auto schedule=std::find_if(schedules.begin(),schedules.end(),[&](const auto& row){return row.at("id")==s.scheduleId;});
   check(schedule!=schedules.end()&&schedule->at("duration_ms")==s.durationMs&&schedule->at("coin_factor_bps")==s.coinFactorBps&&schedule->at("xp_factor_bps")==s.xpFactorBps,"Unknown or inconsistent schedule");
  }
  check(!c.find(s.id),"Duplicate species id");c.species.push_back(std::move(s));
 }
 check(c.species.size()==99,"Expected 72 coin species and 27 companions");
 if(j.contains("decorations")){
  const auto& dc=j.at("decorations");const auto& tuning=dc.at("tuning");
  c.decorTuning={tuning.at("placed_limit"),tuning.at("animated_limit"),tuning.at("emitter_limit"),tuning.at("particle_limit"),tuning.at("launch_level_cap"),tuning.at("tutorial_total_xp"),tuning.at("tutorial_item")};
  check(c.decorTuning.placedLimit>0&&c.decorTuning.placedLimit<=500&&c.decorTuning.animatedLimit>=0&&c.decorTuning.animatedLimit<=32&&c.decorTuning.launchLevelCap==40,"Invalid decor limits");
  for(const auto& x:dc.at("items")){
   DecorDef d;d.id=x.at("id");d.name=x.at("name");d.category=x.at("category");d.subcategory=x.at("subcategory");d.theme=x.at("theme");d.edition=x.at("edition");d.rarity=x.at("rarity");d.level=x.at("level");d.price=x.at("price");d.buyXp=x.at("buy_xp");d.score=x.at("score");d.size=x.at("size");d.width=x.at("width");d.height=x.at("height");d.layer=x.at("layer");d.releaseGate=x.at("release_gate");d.event=x.at("event");d.availability=x.at("availability");d.asset=x.at("asset");d.art=x.at("art");d.source="Decor Catalog!A"+std::to_string(x.at("source").at("row").get<int>());
   const std::string currency=x.at("currency");check(currency=="coins"||currency=="pearls","Invalid decor currency");d.currency=currency=="pearls"?Currency::Pearls:Currency::Coins;
   check(!c.findDecor(d.id)&&d.level>=1&&d.level<=70&&d.price>0&&d.price<=limit&&d.buyXp==0,"Invalid decor catalog identity or price");
   check(std::isfinite(d.width)&&std::isfinite(d.height)&&d.width>0&&d.height>0&&d.width<=4.5&&d.height<=4.5,"Invalid decor dimensions");
   check(d.layer=="Foreground"||d.layer=="Midground"||d.layer=="Background","Invalid decor layer");c.decorations.push_back(std::move(d));
  }
  check(c.decorations.size()==120,"Expected 120 workbook decor items");
  for(const auto& x:dc.at("events")){DecorEvent e{x.at("name"),x.at("proposed_window"),x.at("configured"),x.at("starts_at"),x.at("ends_at")};check(!e.configured||(e.startsAt>0&&e.endsAt>e.startsAt),"Invalid configured decor event window");c.decorEvents.push_back(e);}
 }
 c.levels=j.at("levels").get<std::array<Amount,40>>();check(c.levels[0]==0&&c.levels[1]==80,"Invalid initial XP curve");
 check(std::adjacent_find(c.levels.begin(),c.levels.end(),std::greater_equal<Amount>())==c.levels.end()&&c.levels.back()<limit,"XP curve not strictly increasing");c.workbook=j.value("raw_sheets",Json::object());c.supplement=j.value("supplement",Json::object());
 const auto& inputs=j.at("inputs");c.startingWallet={inputs.at("starting_coins"),inputs.at("starting_pearls")};c.startingTankCapacity=inputs.at("starting_slots");c.displaySlots=inputs.at("showcase_slots");c.hatchMs=j.at("overrides").at("hatch_ms");
 check(c.hatchMs==6000&&c.startingTankCapacity==10&&c.displaySlots==8,"Invalid starting capacity or hatch time");
 check(c.startingWallet.coins>=0&&c.startingWallet.coins<=limit&&c.startingWallet.pearls>=0&&c.startingWallet.pearls<=limit,"Invalid starting wallet");
 std::set<std::string> entitlements;std::set<std::pair<int,int>> capacities;int slots=0;
 for(const auto& x:j.at("tank_entitlements")){
  TankEntitlement t{x.at("id"),x.at("prerequisite"),{x.at("tank").get<int>()},x.at("level"),x.at("slots"),x.at("added_slots"),{x.at("coins"),x.at("pearls")}};
  check(t.tank.value>=1&&t.tank.value<=5&&t.level>=1&&t.level<=40&&(t.slots==10||t.slots==15||t.slots==20)&&t.addedSlots==(t.slots==10?10:5)&&t.cost.coins>=0&&t.cost.pearls>=0,"Invalid tank entitlement");
  check((t.prerequisite.empty()||entitlements.contains(t.prerequisite))&&entitlements.insert(t.id).second,"Invalid tank prerequisite");
  check(capacities.insert({t.tank.value,t.slots}).second&&t.cost.coins<=limit&&t.cost.pearls<=limit,"Duplicate tank entitlement or invalid price");
  slots+=t.addedSlots;const int step=(t.slots-10)/5;c.tankCosts[t.tank.value-1][step]=t.cost.coins;c.tankPearlCosts[t.tank.value-1][step]=t.cost.pearls;if(step==0)c.tankLevels[t.tank.value-1]=t.level;c.tankEntitlements.push_back(t);
 }
 check(c.tankEntitlements.size()==15&&slots==100,"Invalid total capacity");
 const auto& treasure=j.at("treasure");
 c.referenceUtilizationNumerator=treasure.at("reference_utilization_numerator");c.referenceUtilizationDenominator=treasure.at("reference_utilization_denominator");
 check(c.referenceUtilizationNumerator>0&&c.referenceUtilizationNumerator<=c.referenceUtilizationDenominator&&c.referenceUtilizationDenominator<=86400,"Invalid reference utilization");
 std::set<std::string> offerIds;std::array<int,3> offerCounts{};
 for(const auto& x:treasure.at("offers")){
  const std::string kind=x.at("kind");check(kind=="coins"||kind=="pearls"||kind=="bundle","Invalid Treasure offer kind");
  TreasureOffer offer;offer.id=x.at("id");offer.name=x.at("name");offer.eligibility=x.at("eligibility");offer.kind=kind=="coins"?TreasureKind::Coins:kind=="pearls"?TreasureKind::Pearls:TreasureKind::Bundle;
  offer.asset=x.at("asset");check(offer.asset.starts_with("treasure/")&&offer.asset.ends_with(".png")&&offer.asset.find("..") == std::string::npos,"Invalid Treasure artwork");
  offer.priceUsdCents=x.at("price_usd_cents");offer.coinDaysBps=x.at("coin_days_bps");offer.level=x.at("level");offer.pearls=x.at("pearls");offer.oncePerAccount=x.at("once_per_account");offer.permanentFrame=x.at("permanent_frame");
  check(!offer.id.empty()&&offerIds.insert(offer.id).second&&!offer.name.empty()&&offer.priceUsdCents>0&&offer.priceUsdCents<=100000&&offer.level>=0&&offer.level<=40,"Invalid Treasure offer identity or price");
  check(offer.coinDaysBps>=0&&offer.coinDaysBps<=1000000&&offer.pearls>=0&&offer.pearls<=1000000,"Invalid Treasure contents");
  check(offer.kind==TreasureKind::Coins?(offer.coinDaysBps>0&&offer.pearls==0):offer.kind==TreasureKind::Pearls?(offer.coinDaysBps==0&&offer.pearls>0):(offer.coinDaysBps>0&&offer.pearls>0&&offer.oncePerAccount),"Treasure contents do not match the offer kind");
  ++offerCounts[int(offer.kind)];c.treasureOffers.push_back(std::move(offer));
 }
 check(offerCounts==std::array<int,3>{5,5,1},"Expected five coin packs, five pearl packs and one starter bundle");
 const auto& rewards=j.at("level_rewards");check(rewards.size()==40,"Expected 40 level rewards");
 for(int i=0;i<40;++i){c.levelRewards[i]={rewards[i].at("coins"),rewards[i].at("pearls")};check(c.levelRewards[i].coins>=0&&c.levelRewards[i].coins<limit/100&&c.levelRewards[i].pearls>=0&&c.levelRewards[i].pearls<limit/100,"Invalid level reward");}
 c.starters=j.at("starter_species").get<std::vector<std::string>>();for(const auto& id:c.starters)check(c.find(id)&&c.find(id)->level==1&&!c.find(id)->companion,"Invalid gifted starter fish");
 return c;
}
Calendar calendarAt(Millis unixMs){
 using namespace std::chrono;const auto d=floor<days>(sys_time<milliseconds>{milliseconds{unixMs}});year_month_day ymd{d};
 const auto day=d.time_since_epoch().count();const auto week=floor<days>(d-days{weekday{d}.iso_encoding()-1}).time_since_epoch().count()/7;
 return {day,week,int(ymd.year()),int(unsigned(ymd.month()))*100+int(unsigned(ymd.day()))};
}
bool eventOpen(const Species& s,const Calendar& c){if(!s.eventConfigured)return false;if(!s.eventStart)return true;return s.eventStart<=s.eventEnd?(c.monthDay>=s.eventStart&&c.monthDay<=s.eventEnd):(c.monthDay>=s.eventStart||c.monthDay<=s.eventEnd);}
int levelFor(const Content& c,Amount xp){return static_cast<int>(std::upper_bound(c.levels.begin(),c.levels.end(),xp)-c.levels.begin());}
Money levelReward(const Content& c,int level){return level>=2&&level<=40?c.levelRewards[level-1]:Money{0,0};}
Care careOf(const Species&,const Fish& f,Millis now){if(f.egg)return Care::Fed;return now-f.lastFedAt>=f.purchase.feedMs?Care::Hungry:Care::Fed;}
bool sellable(const Species&,const Fish& f){return f.purchase.durationMs>0&&!f.egg&&f.age>=1&&!f.stashed&&!f.favorite;}
double ageScale(const Species&,const Fish& f){return .66+.41*growthProgress(f);}
void advanceFish(const Species&,Fish& f,Millis from,Millis to){
 if(to<=from||f.stashed||f.age>=4)return;
 if(f.egg){
  const auto end=std::min(to,f.hatchAt);if(end>from)f.growthMs+=end-from;
  if(to<f.hatchAt)return;
  from=std::max(from,f.hatchAt);f.egg=false;f.lastFedAt=f.hatchAt-f.purchase.feedMs;
 }
 const auto end=std::min(to,f.lastFedAt+f.purchase.feedMs);
 if(end>from)f.growthMs=std::min(f.purchase.durationMs,f.growthMs+end-from);
 f.age=growthStage(f.purchase,f.growthMs);
}
std::string errorText(Error e){switch(e){case Error::Conflict:return "The offer changed. Please review it and try again.";case Error::SaveFailure:return "The action was not saved. Please retry.";case Error::Protected:return "Unfavorite this fish before rehoming it.";case Error::None:return "";case Error::Unknown:return "That item is not available.";case Error::NoArt:return "Artwork unavailable. Nothing was charged.";case Error::EventClosed:return "This seasonal event is closed.";case Error::Level:return "Reach the required level first.";case Error::Claimed:return "Already claimed for this period.";case Error::Full:return "The growing slots are full. Keep or rehome a fish, expand or switch tanks.";case Error::Funds:return "Open the shop for more coins or pearls.";case Error::InvalidPosition:return "Place inside the aquarium.";case Error::InvalidFish:return "That fish is no longer here.";case Error::NotSellable:return "This fish cannot be rehomed.";case Error::Dead:return "A dead fish cannot be stored or fed.";case Error::NotDead:return "This fish is already alive.";case Error::NotStored:return "That fish is not in inventory.";case Error::AlreadyOwned:return "You already own this tank.";case Error::PreviousTank:return "Unlock the preceding tank first.";case Error::Maximum:return "Maximum capacity reached.";case Error::NoFoodRoom:return "There is enough food in the water already.";case Error::Unavailable:return "This external service is not connected.";case Error::NotReady:return "Complete the objective first.";case Error::Overflow:return "The transaction exceeds the supported save bounds.";}return "Unknown action.";}
std::string stageName(const Fish& f){if(f.dead)return "Dead";if(f.egg)return "Egg";constexpr std::array<const char*,5> names{"Baby","Junior","Young","Mature","Adult"};return names[std::clamp(f.age,0,4)];}
std::string careName(Care c){switch(c){case Care::Fed:return "Happy & fed";case Care::Hungry:return "Hungry";case Care::Urgent:return "Feed soon!";case Care::Sick:return "SICK";case Care::Dead:return "Needs revival";}return "";}
Domain::Domain(Content content,Millis calendarMs,std::uint64_t seed):content_(std::move(content)){
 state_.calendarNow=calendarMs;state_.wallAnchor=calendarMs;state_.rngState=seed?seed:1;
 state_.wallet=content_.startingWallet;state_.tanks[0].slots=content_.startingTankCapacity;state_.decorOnboardingComplete=true;state_.tutorialStep=11;
 requestId_="initial";ledger("coins",state_.wallet.coins,state_.wallet.coins,"opening","initial");ledger("pearls",state_.wallet.pearls,state_.wallet.pearls,"opening","initial");
 for(std::size_t i=0;i<content_.starters.size();++i){auto f=makeFish(content_.starters[i],{220.+190.*double(i),220.+130.*double(i%2)},false);f.purchase.principal=0;if(!contains(state_.collected,f.species))state_.collected.push_back(f.species);state_.fish.push_back(std::move(f));}
 configureExtras();setCalendar(calendarMs);
}
double Domain::random(double a,double b){auto x=state_.rngState;x^=x<<13;x^=x>>7;x^=x<<17;state_.rngState=x;return a+(b-a)*double(x>>11)/9007199254740992.;}
Fish Domain::makeFish(std::string_view id,WorldPoint p,bool egg){
 const auto* s=content_.find(id);check(s,"Unknown species in makeFish");Fish f;f.id={state_.nextFishId++};f.species=id;f.tank=state_.activeTank;f.position=p;f.egg=egg;f.purchase=purchaseQuote(content_,*s,level());f.hatchAt=state_.simNow+(egg?content_.hatchMs:0);f.lastFedAt=state_.simNow-f.purchase.feedMs;f.boughtAt=state_.simNow;f.growthMs=egg?0:content_.hatchMs;
 auto& m=f.motion;m.cruise=random(20,42);m.direction=random(0,1)<.5?-1:1;m.phase=random(0,2*pi);m.drift=random(0,2*pi);m.targetY=random(100,waterHeight-52);m.retarget=random(1,4);m.dashInterval=random(4,18);m.dashWait=random(1,m.dashInterval);m.dashDuration=random(.5,.9);m.dashStrength=random(2.6,4.6);m.turnDuration=random(.28,.42);m.previous=p;return f;
}
const Fish* Domain::fish(FishId id)const{auto it=std::find_if(state_.fish.begin(),state_.fish.end(),[&](auto& f){return f.id==id;});return it==state_.fish.end()?nullptr:&*it;}
Fish* Domain::mutableFish(FishId id){return const_cast<Fish*>(std::as_const(*this).fish(id));}
const Tank* Domain::tank(TankId id)const{auto i=std::find_if(state_.tanks.begin(),state_.tanks.end(),[&](auto& t){return t.id==id;});return i==state_.tanks.end()?nullptr:&*i;}
std::size_t Domain::living(TankId id)const{return static_cast<std::size_t>(std::count_if(state_.fish.begin(),state_.fish.end(),[&](auto& f){return f.tank==id&&!f.stashed&&!f.dead;}));}
Result Domain::blocker(const Species& s,bool includeCapacity)const{
 if(s.releaseGate!="Launch"||s.level>40)return {.error=Error::Unavailable,.message="This fish is planned for a later release."};if(!s.artReady)return bad(Error::NoArt);auto cal=calendarAt(state_.calendarNow);if(!eventOpen(s,cal))return bad(Error::EventClosed);if(level()<s.level)return bad(Error::Level);if((s.annual||s.oneTime)&&contains(state_.claims,claimKey(s,cal)))return bad(Error::Claimed);
 auto* t=tank(state_.activeTank);if(!s.companion&&includeCapacity&&(!t||living(t->id)>=static_cast<std::size_t>(t->slots)))return bad(Error::Full);return s.currency==Currency::Gift?Result{}:requireFunds(s.currency==Currency::Coins?quote(s).principal:0,s.currency==Currency::Pearls?s.price:0);
}
Result Domain::requireFunds(Amount coins,Amount pearls)const{
 const CurrencyShortfall missing{std::max(Amount{},coins-state_.wallet.coins),std::max(Amount{},pearls-state_.wallet.pearls)};
 if(missing.coins||missing.pearls)return {.error=Error::Funds,.message=missing.coins?(missing.pearls?"Not enough coins and pearls.":"Not enough coins."):"Not enough pearls.",.shortfall=missing};
 return {};
}
Result Domain::grant(Amount coins,Amount xp,Amount pearls){
 if(!addOk(state_.lifetimeXp,xp))return bad(Error::Overflow);
 const Amount awardedXp=std::min(xp,content_.levels.back()-state_.xp);
 const int before=level(),after=levelFor(content_,state_.xp+awardedXp);
 const int first=std::max({2,before+1,state_.highestRewardedLevel+1});
 Amount levelCoins=0,levelPearls=0;
 for(int reached=first;reached<=after;++reached){const auto reward=levelReward(content_,reached);levelCoins+=reward.coins;levelPearls+=reward.pearls;}
 if(!addOk(coins,levelCoins)||!addOk(pearls,levelPearls)||!addOk(state_.wallet.coins,coins+levelCoins)||!addOk(state_.wallet.pearls,pearls+levelPearls))return bad(Error::Overflow);
 state_.wallet.coins+=coins;state_.wallet.pearls+=pearls;state_.xp+=awardedXp;state_.lifetimeXp+=xp;
 ledger("coins",coins,state_.wallet.coins,reason_,source_);ledger("pearls",pearls,state_.wallet.pearls,reason_,source_);ledger("xp",awardedXp,state_.xp,reason_,source_);
 state_.highestRewardedLevel=std::max(state_.highestRewardedLevel,after);
 for(int reached=first;reached<=after;++reached){const auto reward=levelReward(content_,reached);state_.wallet.coins+=reward.coins;state_.wallet.pearls+=reward.pearls;
  ledger("coins",reward.coins,state_.wallet.coins,"level_reward",std::to_string(reached));ledger("pearls",reward.pearls,state_.wallet.pearls,"level_reward",std::to_string(reached));
  emit({"level",{}, {544,90},reward.coins,0,reward.pearls,"Level "+std::to_string(reached)+"!",reached});}
 if(after>before)configureExtras();return {Error::None,{},coins,awardedXp,pearls,{}};
}
void Domain::emit(Event e){if(events_.size()>=64)events_.erase(events_.begin());events_.push_back(std::move(e));}
std::vector<Event> Domain::takeEvents(){auto result=std::move(events_);events_.clear();return result;}
void Domain::count(std::string_view event,std::int64_t n){state_.totalEvents[std::string(event)]+=n;for(auto& q:questDefs_)if(q.event==event&&level()>=q.level){auto& p=state_.quests[q.id];p.count=std::min<std::int64_t>(q.target,p.count+n);}tutorialEvent(event);}
void Domain::setCalendar(Millis now){state_.calendarNow=std::max(state_.calendarNow,now);auto c=calendarAt(state_.calendarNow);if(c.day>state_.dailyPeriod){for(auto& q:questDefs_)if(!q.weekly)state_.quests[q.id]={};state_.dailyPeriod=c.day;}if(c.week>state_.weeklyPeriod){for(auto& q:questDefs_)if(q.weekly)state_.quests[q.id]={};state_.weeklyPeriod=c.week;}}
void Domain::advanceCare(Millis delta){
 if(delta<=0)return;if(delta>315576000000LL)throw std::invalid_argument("Care advance exceeds ten years");if(state_.simNow>limit-delta)throw std::overflow_error("Simulation clock exhausted");const auto to=state_.simNow+delta;
 for(auto& f:state_.fish){const auto* s=content_.find(f.species);const bool egg=f.egg;const int age=f.age;advanceFish(*s,f,state_.simNow,to);if(egg&&!f.egg)emit({"hatch",f.id,f.position,0,0,0,"Hello, little "+s->name+"!"});if(age<f.age)emit({"growth",f.id,f.position,0,0,0,stageName(f)+"!"});}
 state_.simNow=to;
}
void Domain::beginTurn(Fish& f,int d){auto& m=f.motion;if(d==m.direction||m.turnRemaining>0)return;m.turnFrom=m.direction;m.turnDuration=random(.28,.42);m.turnRemaining=m.turnDuration;}
double motionFacing(const Motion& m){return m.turnRemaining>0?-m.turnFrom*std::cos(pi*(1-m.turnRemaining/m.turnDuration)):-double(m.direction);}
void Domain::stepMovement(double dt,Tool tool,FishId held){
 if(!(dt>0&&dt<=.05))return;
 const double foodFloor=waterHeight-5;
 for(auto& p:pellets_){p.previous=p.position;p.hasPrevious=true;if(p.position.y<foodFloor){p.speed=std::min(105.,p.speed+34.*dt);p.position.y=std::min(foodFloor,p.position.y+p.speed*dt);p.position.x=std::clamp(p.position.x+std::sin(p.phase)*3.*dt,8.,waterWidth-8);p.rotation+=22.*dt;p.phase+=dt;}else p.rest+=dt;}
 std::erase_if(pellets_,[](auto& p){return p.rest>=31.8;});
 struct Arrival {FishId fish;std::uint64_t pellet;double when;};std::vector<Arrival> arrivals;
 const auto swim=[&](Fish& f){if(f.stashed||f.tank!=state_.activeTank)return;auto& m=f.motion;m.previous=f.position;m.previousPhase=m.phase;m.previousPitch=m.pitch;m.previousFacing=motionFacing(m);m.previousSpeed=m.speed;m.hasPrevious=true;const auto& s=*content_.find(f.species);m.drift+=dt;
  if(f.egg){const double sink=69.*waterHeight/472.*(.9+.2*(m.cruise-20.)/22.);const double floor=waterHeight-12;if(f.position.y<floor)f.position.y=std::min(floor,f.position.y+sink*dt);return;}
  Care care=careOf(s,f,state_.simNow);bool hungry=care!=Care::Fed;bool sick=care==Care::Sick;bool heldNow=f.id==held||(tool==Tool::Sell&&sellable(s,f));
  const double halfHeight=s.nominalLength*ageScale(s,f)*.3;const double top=56.+halfHeight,bottom=waterHeight-halfHeight;
  if(!hungry){m.foodTarget=0;m.noticeDelay=0;}
  auto target=std::find_if(pellets_.begin(),pellets_.end(),[&](auto& p){return p.id==m.foodTarget;});
  if(target==pellets_.end())m.foodTarget=0;
  if(hungry&&!heldNow&&!pellets_.empty()&&!m.foodTarget){if(m.noticeDelay<=0)m.noticeDelay=random(.08,.5);m.noticeDelay-=dt;if(m.noticeDelay<=0){auto nearest=std::min_element(pellets_.begin(),pellets_.end(),[&](auto& a,auto& b){return std::hypot(a.position.x-f.position.x,a.position.y-f.position.y)<std::hypot(b.position.x-f.position.x,b.position.y-f.position.y);});m.foodTarget=nearest->id;m.dashRemaining=m.dashDuration;target=nearest;}}
  const bool chase=m.foodTarget&&target!=pellets_.end();m.dashRemaining=std::max(0.,m.dashRemaining-dt);m.dashWait-=dt;m.retarget-=dt;m.mealDelay=std::max(0.,m.mealDelay-dt);
  if(!sick&&!chase&&m.dashWait<=0){m.dashRemaining=m.dashDuration;m.dashWait=m.dashInterval*random(.85,1.15);}
  if(m.retarget<=0&&!chase){m.targetY=random(top,bottom);m.retarget=random(2.5,7.5);}
  double boost=1.+(m.dashStrength-1.)*std::pow(std::clamp(m.dashRemaining/m.dashDuration,0.,1.),1.4);if(sick)boost=1;if(chase)boost=std::max(1.9,boost)*1.5;
  double speed=m.cruise*boost*(sick?.15:1.);double targetY=sick?bottom-26.:m.targetY;double dx=0;
  if(chase){targetY=std::clamp(target->position.y,top,bottom);dx=target->position.x-f.position.x;if(dx*m.direction<-46.)beginTurn(f,-m.direction);if(std::abs(dx)<28.)speed*=.15;}
  if(m.turnRemaining>0){m.turnRemaining=std::max(0.,m.turnRemaining-dt);double progress=1-m.turnRemaining/m.turnDuration;m.direction=progress>=.5?-m.turnFrom:m.turnFrom;speed*=.12+.88*std::abs(std::cos(pi*progress));m.pitch*=std::exp(-dt*20.);}
  double vy=(targetY-f.position.y)*(chase?2.8:.32);vy=std::clamp(vy,-speed*(chase?2.5:.8)-12.,speed*(chase?2.5:.8)+12.);if(!chase)vy+=std::sin(m.drift*1.1)*2.;
  if(heldNow){speed=0;vy=0;m.foodTarget=0;}
  f.position.x+=speed*double(m.direction)*dt;f.position.y=std::clamp(f.position.y+vy*dt,top,bottom);
  if(f.position.x<0){f.position.x=0;beginTurn(f,1);}if(f.position.x>1088){f.position.x=1088;beginTurn(f,-1);}
  if(m.turnRemaining<=0){double desired=std::clamp(std::atan2(vy,std::max(speed,20.)),-.42,.42);if(sick&&!chase)desired=.22;m.pitch+=(desired-m.pitch)*(1-std::exp(-dt*6.));}
  m.speed=speed;const double sizeMult=std::clamp(std::sqrt(46./s.nominalLength),.5,1.45);m.phase+=(5.6+speed*.03)*sizeMult*dt;
  if(chase&&std::abs(target->position.x-f.position.x)<=28.&&std::abs(targetY-f.position.y)<=40.){
   // Sweep ordering within this fixed step: earlier entry beats ID; exact ties use ID.
   double before=std::hypot(target->position.x-m.previous.x,targetY-m.previous.y),after=std::hypot(target->position.x-f.position.x,targetY-f.position.y);double when=std::clamp((before-40.)/std::max(.0001,before-after),0.,1.);arrivals.push_back({f.id,target->id,when});
  }
 };
 for(auto& fish:state_.fish)swim(fish);
 for(auto& display:state_.companions){auto visual=companionVisual(display);swim(visual);display.position=visual.position;display.motion=visual.motion;}
 std::sort(arrivals.begin(),arrivals.end(),[](auto& a,auto& b){if(std::abs(a.when-b.when)>1e-9)return a.when<b.when;return a.fish<b.fish;});
 for(const auto& arrival:arrivals){
  const auto pellet=std::find_if(pellets_.begin(),pellets_.end(),[&](const auto& value){return value.id==arrival.pellet;});if(pellet==pellets_.end())continue;
  if(auto result=execute({.action=Action::Feed,.fish=arrival.fish});result){
   Motion* m=nullptr;if(auto* fish=mutableFish(arrival.fish))m=&fish->motion;else if(auto* display=const_cast<Companion*>(companion(arrival.fish)))m=&display->motion;
   if(m){m->mealDelay=random(.15,.45);m->retarget=random(.6,1.4);m->foodTarget=0;}pellets_.erase(pellet);
  }
 }

}
void Domain::clearTransient(){pellets_.clear();for(auto& f:state_.companions){f.motion.foodTarget=0;f.motion.noticeDelay=0;f.motion.previous=f.position;f.motion.hasPrevious=false;}for(auto& f:state_.fish){f.motion.foodTarget=0;f.motion.noticeDelay=0;f.motion.previous=f.position;f.motion.hasPrevious=false;}}
void Domain::install(State candidate){state_=std::move(candidate);configureExtras();clearTransient();events_.clear();}
Result Domain::executeImpl(const Command& c){
 Fish* f=mutableFish(c.fish);
 switch(c.action){
 case Action::PurchaseEnvironment:case Action::EquipEnvironment:return executeEnvironment(c);
 case Action::Buy:{
  const auto* s=content_.find(c.key);if(!s)return bad(Error::Unknown);if(auto gate=blocker(*s);!gate)return gate;
  if(!validPoint(c.point))return bad(Error::InvalidPosition);
  if(state_.fish.size()+state_.companions.size()>=10000||state_.nextFishId==UINT64_MAX)return bad(Error::Maximum);
  const auto offer=quote(*s);if(c.offer&&*c.offer!=offer)return bad(Error::Conflict);
  reason_=s->companion?"companion_purchase":"egg_purchase";source_=std::to_string(state_.nextFishId);
  const auto coins=s->currency==Currency::Coins?offer.principal:0,pearls=s->currency==Currency::Pearls?s->price:0;
  if(auto r=spend(coins,pearls);!r)return r;
  Fish baby=makeFish(s->id,c.point,!s->companion);const auto id=baby.id;
  if(s->companion){const bool stored=displaying(state_.activeTank)>=static_cast<std::size_t>(content_.displaySlots);state_.companions.push_back({id,{},s->id,state_.activeTank,c.point,baby.motion,stored,false,state_.simNow-43200000});}
  else state_.fish.push_back(std::move(baby));
  if(!contains(state_.collected,s->id)){state_.collected.push_back(s->id);count("collection");}
  if(s->oneTime||s->annual)state_.claims.push_back(claimKey(*s,calendarAt(state_.calendarNow)));
  count("buy");count("place");emit({"purchase",id,c.point,-coins,0,-pearls,s->companion?"Permanent companion":""});return {.fish=id,.coins=-coins,.pearls=-pearls};
 }
 case Action::Feed:{
  if(auto* display=const_cast<Companion*>(companion(c.fish))){
   if(display->stored||display->tank!=state_.activeTank)return bad(Error::InvalidFish);
   if(state_.simNow-display->lastFedAt<43200000)return bad(Error::NotReady);
   display->lastFedAt=state_.simNow;emit({"feed",display->id,display->position,0,0,0,"Yum!"});return {};
  }
  if(!f||f->stashed)return bad(Error::InvalidFish);if(f->egg||careOf(*content_.find(f->species),*f,state_.simNow)==Care::Fed)return bad(Error::NotReady);
  f->lastFedAt=state_.simNow;f->motion.foodTarget=0;f->motion.noticeDelay=0;
  auto& days=state_.careDays[f->species];const auto day=calendarAt(state_.calendarNow).day;if(std::find(days.begin(),days.end(),day)==days.end()&&days.size()<5)days.push_back(day);
  emit({"feed",f->id,f->position,0,0,0,"Yum!"});count("feed");return {};
 }
 case Action::Sell:case Action::Keep:return settle(c);
 case Action::Favorite:{
  if(f){f->favorite=!f->favorite;return {};}
  if(auto* display=const_cast<Companion*>(companion(c.fish))){display->favorite=!display->favorite;return {};}
  return bad(Error::InvalidFish);
 }
 case Action::Stash:case Action::Restore:{
  auto* display=const_cast<Companion*>(companion(c.fish));if(!display)return {.error=Error::Unavailable,.message="Only display fish can be stored."};
  if(c.action==Action::Stash){display->stored=true;return {};}
  if(!display->stored)return bad(Error::NotStored);if(!validPoint(c.point))return bad(Error::InvalidPosition);
  if(displaying(state_.activeTank)>=static_cast<std::size_t>(content_.displaySlots))return {.error=Error::Full,.message="Store a display fish first. This tank has 8 display slots."};
  display->tank=state_.activeTank;display->position=c.point;display->stored=false;display->motion.previous=c.point;return {};
 }
 case Action::Move:
  if(!validPoint(c.point))return bad(Error::InvalidPosition);
  if(f&&!f->stashed&&f->tank==state_.activeTank){f->position=c.point;f->motion.previous=c.point;f->motion.foodTarget=0;return {};}
  if(auto* display=const_cast<Companion*>(companion(c.fish));display&&!display->stored&&display->tank==state_.activeTank){display->position=c.point;display->motion.previous=c.point;return {};}
  return bad(Error::InvalidFish);
 case Action::Revive:case Action::Remove:case Action::ReviveAll:case Action::RemoveAll:return bad(Error::Unavailable);
 case Action::UnlockTank:case Action::ExpandTank:{
  if(c.currency!=Currency::Coins&&c.currency!=Currency::Pearls)return bad(Error::Unavailable);
  const auto* owned=tank(c.tank);if(c.action==Action::UnlockTank&&owned)return bad(Error::AlreadyOwned);if(c.action==Action::ExpandTank&&!owned)return bad(Error::Unknown);
  const auto* e=nextTankEntitlement(c.tank);if(!e)return bad(Error::Maximum);if(level()<e->level)return bad(Error::Level);
  if(!e->prerequisite.empty()){
   const auto it=std::find_if(content_.tankEntitlements.begin(),content_.tankEntitlements.end(),[&](const auto& value){return value.id==e->prerequisite;});
   const auto* prior=it==content_.tankEntitlements.end()?nullptr:tank(it->tank);if(!prior||prior->slots<it->slots)return bad(Error::PreviousTank);
  }
  reason_="tank_purchase";source_=e->id;const Amount coins=c.currency==Currency::Coins?e->cost.coins:0,pearls=c.currency==Currency::Pearls?e->cost.pearls:0;
  if(auto r=spend(coins,pearls);!r)return r;
  if(owned)const_cast<Tank*>(owned)->slots=e->slots;else{state_.tanks.push_back({c.tank,e->slots});state_.activeTank=c.tank;clearTransient();}
  emit({"tank",{}, {544,220},-coins,0,-pearls,"More room to swim!"});return {.coins=-coins,.pearls=-pearls};
 }
 case Action::SwitchTank:
  if(!tank(c.tank))return bad(Error::Unknown);state_.activeTank=c.tank;clearTransient();return {};
 case Action::DropFood:
  if(!validPoint(c.point))return bad(Error::InvalidPosition);if(pellets_.size()>=48)return bad(Error::NoFoodRoom);pellets_.push_back({nextPellet_++,c.point,38,random(0,2*pi),0,0});return {};
 case Action::BuyDecor:case Action::MoveDecor:case Action::StoreDecor:case Action::RestoreDecor:case Action::FlipDecor:case Action::PurchaseDecor:case Action::PlaceDecor:case Action::CancelDecor:case Action::ResizeDecor:return executeDecor(c);
 case Action::ClaimQuest:return claimQuest(c.key);
 case Action::SendGift:return sendGift();
 case Action::DailyEgg:return dailyEgg();
 case Action::ClaimMastery:return {.error=Error::Unavailable,.message="Mastery rewards are being prepared."};

 case Action::Tutorial:return {.error=Error::Unavailable,.message="The new introduction is being prepared."};
 case Action::SetLook:state_.settings.tankLook=std::clamp(static_cast<int>(c.value),0,2);tutorialEvent("look");return {};
 case Action::SetReducedMotion:state_.settings.reducedMotion=c.value!=0;return {};
 case Action::SetSound:state_.settings.sound=c.value!=0;return {};
 case Action::SetMusic:state_.settings.music=c.value!=0;return {};
 case Action::SetVolume:if(!std::isfinite(c.value))return bad(Error::Unknown);state_.settings.volume=std::clamp(c.value,0.,1.);return {};
 }
 return bad(Error::Unknown);
}
MasteryProgress Domain::masteryProgress(std::string_view species)const{
 MasteryProgress result;const auto it=state_.adultRaised.find(std::string(species));if(it!=state_.adultRaised.end())result.count=it->second;
 const auto& goals=content_.masteryTargets;for(int goal:goals){if(!contains(state_.claims,"mastery:"+std::string(species)+":"+std::to_string(goal)))break;++result.tier;}
 result.complete=result.tier==int(goals.size());result.target=result.complete?goals.back():goals[static_cast<std::size_t>(result.tier)];const auto days=state_.careDays.find(std::string(species));result.ready=!result.complete&&result.count>=result.target&&(result.tier<3||(days!=state_.careDays.end()&&days->second.size()>=5));return result;
}
void Domain::configureExtras(){
 decorDefs_=content_.decorations;
 questDefs_.clear();
 const auto key=std::to_string(level());
 if(!content_.supplement.contains("quest_definitions"))return;
 for(const auto& definition:content_.supplement["quest_definitions"]){
  Quest q;q.id=definition.at("id");q.label=definition.at("label");q.event=definition.at("event");q.target=definition.at("target");q.level=definition.at("level");q.weekly=definition.at("weekly");q.configured=definition.value("configured",true);q.missing=definition.value("missing","");q.source=definition.at("source").dump();
  const auto& profiles=content_.supplement["quests_by_level"];
  if(profiles.contains(key)&&profiles[key].contains(q.id)){const auto& reward=profiles[key][q.id];q.coins=reward.value("coins",Amount{0});q.xp=reward.value("xp",Amount{0});q.pearls=reward.value("pearls",Amount{0});q.source+="; "+reward.at("source").dump();}
  questDefs_.push_back(std::move(q));
 }
}

Result Domain::claimQuest(std::string_view id){
 auto q=std::find_if(questDefs_.begin(),questDefs_.end(),[&](auto& x){return x.id==id;});if(q==questDefs_.end())return bad(Error::Unknown);if(level()<q->level)return bad(Error::Level);if(!q->configured)return {.error=Error::Unavailable,.message=q->missing};auto& p=state_.quests[q->id];if(p.claimed)return bad(Error::Claimed);if(p.count<q->target)return bad(Error::NotReady);
 if(q->coins==0&&q->xp==0&&q->pearls==0)return {.error=Error::Unavailable,.message="No reward is configured for this quest at your level."};
 auto r=grant(q->coins,q->xp,q->pearls);if(!r)return r;p.claimed=true;tutorialEvent("quest");emit({"quest",{}, {544,240},r.coins,r.xp,r.pearls,"Quest complete!"});return r;
}
Result Domain::sendGift(){
 if(level()<5)return bad(Error::Level);
 return {.error=Error::Unavailable,.message="Gift rewards will be available when their Pearl amounts are configured."};
}
Result Domain::dailyEgg(){
 if(level()<6)return bad(Error::Level);
 return {.error=Error::Unavailable,.message="The Daily Egg Basket needs its weekly fish table and resale rules."};
}
void Domain::tutorialEvent(std::string_view e){
 constexpr std::array<const char*,11> needed{"look","feed","buy","place","demo","sell","decor","level","quest","gift","finish"};
 if(state_.tutorialStep>=0&&state_.tutorialStep<static_cast<int>(needed.size())&&e==needed[state_.tutorialStep]){
  if(e=="feed"&&state_.totalEvents["feed"]<4)return;
  const std::string key="bonus:"+std::string(e);
  if(!contains(state_.tutorialClaims,key)&&content_.supplement.contains("onboarding_rewards")&&content_.supplement["onboarding_rewards"].contains(std::string(e))){
   Amount bonus=content_.supplement["onboarding_rewards"][std::string(e)].value("xp",Amount{0});
   if(!grant(0,bonus))throw std::overflow_error("Tutorial reward overflow");state_.tutorialClaims.push_back(key);
   if(bonus)emit({"tutorial",{}, {544,200},0,bonus,0,"Tutorial reward"});
  }
  ++state_.tutorialStep;
 }
 // A purchase creates its egg in one command, so the independent placement event
 // can advance the next step in the same authoritative transaction.
}
Result Domain::tutorial(){
 if(state_.tutorialStep>=11)return bad(Error::Claimed);
 if(state_.tutorialStep==0){state_.settings.tankLook=0;tutorialEvent("look");return {};}
 if(state_.tutorialStep==4){
  if(contains(state_.tutorialClaims,"growth-demo"))return bad(Error::Claimed);auto it=std::find_if(state_.fish.begin(),state_.fish.end(),[&](auto& f){return !f.dead&&!f.stashed&&f.tank==state_.activeTank;});if(it==state_.fish.end())return bad(Error::NotReady);
  it->egg=false;it->age=1;it->growthMs=0;it->lastFedAt=state_.simNow;state_.tutorialClaims.push_back("growth-demo");tutorialEvent("demo");emit({"growth",it->id,it->position,0,0,0,"Free tutorial growth demonstration"});return {};
 }
 if(state_.tutorialStep==7){if(level()<2)return {.error=Error::NotReady,.message="Reach 80 XP through normal fish purchases and sales. No unexplained XP is added."};tutorialEvent("level");return {};}
 if(state_.tutorialStep==10){tutorialEvent("finish");return {};}
 return bad(Error::NotReady);
}
void Domain::fixture(std::string_view name){
 const auto settings=state_.settings;
 state_=Domain(content_,state_.calendarNow).state();
 state_.settings=settings;
 const auto build=[&]{
 state_.decorOnboardingComplete=true;
 if(name=="level-up"||name=="level-up-10"||name=="level-up-40"){
  const int reached=name=="level-up-40"?40:name=="level-up-10"?10:2;
  state_.xp=content_.levels[reached-1]-1;state_.highestRewardedLevel=reached-1;state_.tutorialStep=11;
  grant(0,1);return;
 }
 if(name=="plants"||name.starts_with("decor")){
  auto state=state_;state.decorOnboardingComplete=name!="decor-intro";state.wallet={1000000,500};state.xp=content_.levels.back();state.highestRewardedLevel=40;
  if(name=="decor-intro"){state.xp=0;state.highestRewardedLevel=1;state.wallet={250,0};}
  state.decor.clear();state.decorOwned.clear();state.nextDecorId=1;
  if(name=="decor-scene")for(const auto& [id,pos]:std::vector<std::pair<std::string,WorldPoint>>{{"CP-01",{220,530}},{"CP-06",{370,530}},{"CD-01",{500,555}},{"CD-07",{820,520}},{"PP-01",{950,490}}}){
   state.decor.push_back({state.nextDecorId++,id,{1},pos});state.decorOwned.push_back(id);
  }
  install(std::move(state));return;
 }
 if(name=="aquarium"){
  state_.fish.clear();state_.nextFishId=1;state_.wallet={250,0};state_.xp=14;state_.highestRewardedLevel=1;state_.tutorialStep=11;state_.tanks={{{1},10}};state_.activeTank={1};
  constexpr std::array<const char*,6> species{"molly","emberTetra","guppy","guppy","guppy","platy"};
  constexpr std::array<WorldPoint,6> centres{{{895,243.5},{704,365},{764,399},{1041,460},{771.5,634.5},{1080.5,625}}};
  constexpr std::array<int,6> directions{-1,-1,1,-1,1,-1};
  for(std::size_t i=0;i<species.size();++i){
   auto f=makeFish(species[i],{centres[i].x*1088./1608.,centres[i].y*635./830.},false);
   f.age=4;f.lastFedAt=state_.simNow;f.motion.direction=directions[i];f.motion.cruise=20;f.motion.phase=0;f.motion.pitch=0;f.motion.speed=0;state_.fish.push_back(f);
  }
  clearTransient();events_.clear();return;
 }

 if(name=="currency-shop"||name=="currency-pearls"||name=="shop"||name=="tanks"||name=="collection"||name=="settings"){
  state_.fish.clear();state_.wallet={250,0};state_.xp=14;state_.highestRewardedLevel=1;state_.tutorialStep=11;state_.tanks={{{1},10}};state_.activeTank={1};
  const std::array<WorldPoint,8> points{{{600,193},{477,284},{520,320},{702,357},{531,488},{726,482},{400,405},{792,287}}};
  for(std::size_t i=0;i<points.size();++i){auto f=makeFish(i==1?"neonTetra":"guppy",points[i],false);f.age=4;f.lastFedAt=state_.simNow;f.motion.cruise=20;state_.fish.push_back(f);}
  clearTransient();events_.clear();return;
 }
 state_.fish.clear();state_.wallet={12650,38};state_.xp=content_.levels[14]+100;state_.highestRewardedLevel=15;state_.tutorialStep=11;state_.tanks={{{1},20},{{2},20}};state_.activeTank={1};
 std::vector<const Species*> available;for(const auto& s:content_.species)if(!s.companion&&s.artReady&&s.releaseGate=="Launch")available.push_back(&s);
 std::size_t countFish=name=="performance"?20:12;for(std::size_t i=0;i<countFish;++i){const auto& s=*available[i%available.size()];auto f=makeFish(s.id,{140.+double(i%8)*112.,155.+double((i/8)%4)*85.},false);f.age=1+static_cast<int>(i%4);f.lastFedAt=state_.simNow;state_.fish.push_back(f);}
 if(name=="care"&&!state_.fish.empty()){auto& f=state_.fish[0];f.lastFedAt=state_.simNow-f.purchase.feedMs;}
 clearTransient();events_.clear();
 };
 build();
 // Review fixtures are fresh development states, with the same invariants as
 // normal saves. Their opening ledger records the supplied test balances.
 for(auto& f:state_.fish){f.dead=false;f.growthMs=f.egg?0:f.purchase.durationMs/10000*f.purchase.stages[f.age];}
 state_.lifetimeXp=state_.xp;state_.receipts=Json::object();state_.settlements=Json::object();state_.ledger=Json::array();state_.revision=0;
 for(const auto& [currency,amount]:std::vector<std::pair<std::string,Amount>>{{"coins",state_.wallet.coins},{"pearls",state_.wallet.pearls},{"xp",state_.xp}})if(amount)
  state_.ledger.push_back({{"id",state_.ledger.size()+1},{"currency",currency},{"amount",amount},{"balance_after",amount},{"reason","fixture"},{"source_id",currency},{"request_id","fixture"},{"created_at",state_.calendarNow},{"revision",0}});
 if(name=="inventory"&&!state_.fish.empty()){
  auto& f=state_.fish.front();f.age=4;f.growthMs=f.purchase.durationMs;const auto id=f.id;
  execute({.action=Action::Keep,.fish=id});execute({.action=Action::Stash,.fish=id});events_.clear();
 }
}
} // namespace aq
