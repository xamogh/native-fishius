#include "aquarium/storage.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#endif
namespace aq {
namespace {
constexpr Amount limit=9'000'000'000'000'000LL;
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
bool bounded(std::int64_t x){return x>=0&&x<=limit;}
Json point(WorldPoint p){return Json::array({p.x,p.y});}
WorldPoint readPoint(const Json& j){require(j.is_array()&&j.size()==2,"Invalid position shape");WorldPoint p{j[0].get<double>(),j[1].get<double>()};require(inTank(p),"Position outside aquarium");return p;}
Json readFile(const std::filesystem::path& p){require(std::filesystem::file_size(p)<=64*1024*1024,"Save exceeds 64 MiB limit");std::ifstream in(p);if(!in)throw std::runtime_error("Cannot open save");return Json::parse(in);}
Json motion(const Motion& m){return {{"direction",m.direction},{"turnFrom",m.turnFrom},{"cruise",m.cruise},{"phase",m.phase},{"drift",m.drift},{"targetY",m.targetY},{"retarget",m.retarget},{"dashInterval",m.dashInterval},{"dashWait",m.dashWait},{"dashDuration",m.dashDuration},{"dashStrength",m.dashStrength},{"dashRemaining",m.dashRemaining},{"turnDuration",m.turnDuration},{"turnRemaining",m.turnRemaining},{"pitch",m.pitch},{"speed",m.speed},{"mealDelay",m.mealDelay}};}
Motion readMotion(const Json& j,WorldPoint position){
 Motion m;m.direction=j.at("direction");m.turnFrom=j.at("turnFrom");require(std::abs(m.direction)==1&&std::abs(m.turnFrom)==1,"Invalid direction");
 auto d=[&](const char* key,double lo,double hi){const double v=j.at(key);require(std::isfinite(v)&&v>=lo&&v<=hi,"Invalid motion parameter");return v;};
 m.cruise=d("cruise",20,42);m.phase=d("phase",0,1e15);m.drift=d("drift",0,1e15);m.targetY=d("targetY",0,635);m.retarget=d("retarget",-1e9,1e9);m.dashInterval=d("dashInterval",4,18);m.dashWait=d("dashWait",-1e9,100);m.dashDuration=d("dashDuration",.5,.9);m.dashStrength=d("dashStrength",2.6,4.6);m.dashRemaining=d("dashRemaining",0,.9);m.turnDuration=d("turnDuration",.28,.42);m.turnRemaining=d("turnRemaining",0,.42);m.pitch=d("pitch",-.5,.5);m.speed=d("speed",0,1000);m.mealDelay=d("mealDelay",0,10);m.previous=position;return m;
}
void validatePurchase(const GrowthSnapshot& q){
 require(q.level>=1&&q.level<=40&&!q.configVersion.empty()&&q.configVersion.size()<128&&!q.scheduleId.empty(),"Invalid purchase identity");
 require(bounded(q.principal)&&bounded(q.profit)&&bounded(q.xp)&&q.principal<=limit-q.profit,"Invalid fish reward snapshot");
 require(q.durationMs>=1200000&&q.durationMs<=158400000&&q.feedMs>0&&q.feedMs<=43200000,"Invalid growth snapshot");
 for(const auto& values:{q.stages,q.rewards})require(values.front()==0&&values.back()==10000&&std::adjacent_find(values.begin(),values.end(),std::greater_equal<int>())==values.end(),"Invalid snapshot fractions");
 require(q.earlyRefundBps>=0&&q.earlyRefundBps<=10000,"Invalid principal refund");
}
}
Json encode(const State& s){
 Json j={{"version",s.version},{"contentVersion",s.contentVersion},{"simNow",s.simNow},{"wallAnchor",s.wallAnchor},{"calendarNow",s.calendarNow},{"coins",s.wallet.coins},{"pearls",s.wallet.pearls},{"xp",s.xp},{"lifetimeXp",s.lifetimeXp},{"highestRewardedLevel",s.highestRewardedLevel},{"activeTank",s.activeTank.value},{"nextFishId",s.nextFishId},{"nextDecorId",s.nextDecorId},{"rngState",s.rngState},{"claims",s.claims},{"collected",s.collected},{"mastery",s.mastery},{"totalEvents",s.totalEvents},{"tutorialStep",s.tutorialStep},{"tutorialClaims",s.tutorialClaims},{"dailyPeriod",s.dailyPeriod},{"weeklyPeriod",s.weeklyPeriod},{"giftDay",s.giftDay},{"eggDay",s.eggDay},
  {"adultRaised",s.adultRaised},{"pendingDecor",s.pendingDecor},{"decorOwned",s.decorOwned},{"decorOnboardingComplete",s.decorOnboardingComplete},
  {"receipts",s.receipts},{"settlements",s.settlements},{"ledger",s.ledger},{"nextRequestId",s.nextRequestId},{"revision",s.revision},{"careDays",s.careDays}};
 j["settings"]={{"reducedMotion",s.settings.reducedMotion},{"sound",s.settings.sound},{"music",s.settings.music},{"volume",s.settings.volume},{"tankLook",s.settings.tankLook}};
 j["tanks"]=Json::array();for(const auto& t:s.tanks)j["tanks"].push_back({{"id",t.id.value},{"slots",t.slots}});
 j["fish"]=Json::array();for(const auto& f:s.fish)j["fish"].push_back({{"id",f.id.value},{"species",f.species},{"tank",f.tank.value},{"position",point(f.position)},{"age",f.age},{"egg",f.egg},{"growthMs",f.growthMs},{"hatchAt",f.hatchAt},{"lastFedAt",f.lastFedAt},{"boughtAt",f.boughtAt},{"motion",motion(f.motion)},{"purchase",f.purchase},{"favorite",f.favorite},{"scripted",f.scripted}});
 j["companions"]=Json::array();for(const auto& f:s.companions)j["companions"].push_back({{"id",f.id.value},{"origin",f.origin.value},{"species",f.species},{"tank",f.tank.value},{"position",point(f.position)},{"stored",f.stored},{"favorite",f.favorite},{"lastFedAt",f.lastFedAt},{"motion",motion(f.motion)}});
 j["decor"]=Json::array();for(const auto& d:s.decor)j["decor"].push_back({{"id",d.id},{"kind",d.kind},{"tank",d.tank.value},{"position",point(d.position)},{"stored",d.stored},{"flipped",d.flipped},{"sizeMul",d.sizeMul}});
 j["quests"]=Json::object();for(const auto& [id,q]:s.quests)j["quests"][id]={{"count",q.count},{"claimed",q.claimed}};return j;
}
State decodeAndValidate(const Json& j,const Content& c){
 require(j.is_object()&&j.at("version")==4&&j.at("contentVersion")==4,"This development save uses an older format. Start a fresh v4 game.");State s;
 s.simNow=j.at("simNow");s.wallAnchor=j.at("wallAnchor");s.calendarNow=j.at("calendarNow");s.wallet={j.at("coins"),j.at("pearls")};s.xp=j.at("xp");s.lifetimeXp=j.at("lifetimeXp");s.highestRewardedLevel=j.at("highestRewardedLevel");s.activeTank={j.at("activeTank").get<int>()};
 require(bounded(s.simNow)&&s.simNow<limit-315576000000LL&&bounded(s.wallAnchor)&&bounded(s.calendarNow),"Invalid time");
 require(bounded(s.wallet.coins)&&bounded(s.wallet.pearls)&&bounded(s.xp)&&s.xp<=c.levels.back()&&bounded(s.lifetimeXp)&&s.lifetimeXp>=s.xp,"Invalid balance");
 require(s.highestRewardedLevel==levelFor(c,s.xp),"Invalid level reward checkpoint");
 s.nextFishId=j.at("nextFishId");s.nextDecorId=j.at("nextDecorId");s.rngState=j.at("rngState");s.nextRequestId=j.at("nextRequestId");s.revision=j.at("revision");
 require(s.nextFishId>0&&s.nextDecorId>0&&s.rngState&&s.nextRequestId>0,"Invalid identity sequence");
 s.tanks.clear();std::set<int> tanks;
 for(const auto& x:j.at("tanks")){Tank t{{x.at("id").get<int>()},x.at("slots").get<int>()};require(t.id.value>=1&&t.id.value<=5&&tanks.insert(t.id.value).second&&(t.slots==10||t.slots==15||t.slots==20),"Invalid tank capacity");s.tanks.push_back(t);}
 require(tanks.contains(1)&&tanks.contains(s.activeTank.value),"Missing tank");for(int id:tanks)require(id==1||tanks.contains(id-1),"Tank sequence has a gap");
 require(j.at("fish").is_array()&&j.at("fish").size()<=100,"Invalid growing collection");std::set<std::uint64_t> ids;
 for(const auto& x:j.at("fish")){
  Fish f;f.id={x.at("id").get<std::uint64_t>()};f.species=x.at("species");f.tank={x.at("tank").get<int>()};f.position=readPoint(x.at("position"));f.age=x.at("age");f.egg=x.at("egg");f.growthMs=x.at("growthMs");f.hatchAt=x.at("hatchAt");f.lastFedAt=x.at("lastFedAt");f.boughtAt=x.at("boughtAt");f.purchase=x.at("purchase").get<GrowthSnapshot>();f.favorite=x.at("favorite");f.scripted=x.at("scripted");
  require(f.id.value>0&&f.id.value<s.nextFishId&&ids.insert(f.id.value).second,"Duplicate fish identity");const auto* spec=c.find(f.species);require(spec&&!spec->companion&&tanks.contains(f.tank.value),"Invalid productive fish");validatePurchase(f.purchase);
  require(f.growthMs>=0&&f.growthMs<=f.purchase.durationMs&&f.age==growthStage(f.purchase,f.growthMs)&&(!f.egg||f.age==0),"Invalid growth progress");
  require(f.hatchAt>=0&&f.hatchAt<=s.simNow+6000&&f.boughtAt>=0&&f.boughtAt<=s.simNow&&f.hatchAt>=f.boughtAt,"Invalid hatch time");
  require(f.lastFedAt>=-f.purchase.feedMs&&f.lastFedAt<=s.simNow,"Invalid feeding time");f.motion=readMotion(x.at("motion"),f.position);s.fish.push_back(f);
 }
 require(j.at("companions").is_array()&&j.at("companions").size()<=10000,"Invalid companion collection");std::set<std::uint64_t> origins;
 for(const auto& x:j.at("companions")){
  Companion f;f.id={x.at("id").get<std::uint64_t>()};f.origin={x.at("origin").get<std::uint64_t>()};f.species=x.at("species");f.tank={x.at("tank").get<int>()};f.position=readPoint(x.at("position"));f.stored=x.at("stored");f.favorite=x.at("favorite");f.lastFedAt=x.at("lastFedAt");
  const auto* spec=c.find(f.species);require(spec&&tanks.contains(f.tank.value)&&f.id.value>0&&f.id.value<s.nextFishId&&ids.insert(f.id.value).second,"Invalid companion identity");
  require(f.origin.value?f.origin==f.id&&origins.insert(f.origin.value).second:spec->companion,"Invalid companion origin");require(f.lastFedAt>=-158400000&&f.lastFedAt<=s.simNow,"Invalid companion feeding time");f.motion=readMotion(x.at("motion"),f.position);s.companions.push_back(f);
 }
 for(const auto& t:s.tanks){require(std::count_if(s.fish.begin(),s.fish.end(),[&](const auto& f){return f.tank==t.id;})<=t.slots,"Growing capacity exceeded");require(std::count_if(s.companions.begin(),s.companions.end(),[&](const auto& f){return f.tank==t.id&&!f.stored;})<=c.displaySlots,"Display capacity exceeded");}
 require(j.at("decor").is_array()&&j.at("decor").size()<=500,"Invalid decor collection");std::set<std::uint64_t> decorIds;
 for(const auto& x:j.at("decor")){
  Decoration d;d.id=x.at("id");d.kind=x.at("kind");d.tank={x.at("tank").get<int>()};d.position=readPoint(x.at("position"));d.stored=x.at("stored");d.flipped=x.at("flipped");d.sizeMul=x.at("sizeMul");
  require(c.findDecor(d.kind)&&tanks.contains(d.tank.value)&&d.id>0&&d.id<s.nextDecorId&&decorIds.insert(d.id).second,"Invalid decoration identity");require(std::isfinite(d.sizeMul)&&d.sizeMul>=decorSizeMin&&d.sizeMul<=decorSizeMax,"Invalid decoration size");s.decor.push_back(d);
 }
 auto strings=[&](const char* key){auto v=j.at(key).get<std::vector<std::string>>();require(v.size()<=10000,"Too many claims");std::set<std::string> seen;for(const auto& value:v)require(!value.empty()&&value.size()<256&&seen.insert(value).second,"Invalid claim identity");return v;};
 s.claims=strings("claims");s.collected=strings("collected");s.tutorialClaims=strings("tutorialClaims");s.decorOwned=strings("decorOwned");for(const auto& id:s.collected)require(c.find(id),"Unknown collected species");for(const auto& id:s.decorOwned)require(c.findDecor(id),"Unknown owned decor");
 for(const auto& d:s.decor)require(std::find(s.decorOwned.begin(),s.decorOwned.end(),d.kind)!=s.decorOwned.end(),"Placed decor missing ownership");
 s.pendingDecor=j.at("pendingDecor");require(s.pendingDecor.empty()||c.findDecor(s.pendingDecor),"Unknown pending decor");s.decorOnboardingComplete=j.at("decorOnboardingComplete");s.tutorialStep=j.at("tutorialStep");require(s.tutorialStep>=0&&s.tutorialStep<=11,"Invalid introduction progress");
 if(!s.pendingDecor.empty())require(s.decor.size()<500&&s.nextDecorId<std::numeric_limits<std::uint64_t>::max()&&std::find(s.decorOwned.begin(),s.decorOwned.end(),s.pendingDecor)!=s.decorOwned.end(),"Pending decor cannot be stored");
 s.dailyPeriod=j.at("dailyPeriod");s.weeklyPeriod=j.at("weeklyPeriod");s.giftDay=j.at("giftDay");s.eggDay=j.at("eggDay");s.totalEvents=j.at("totalEvents").get<decltype(s.totalEvents)>();s.mastery=j.at("mastery").get<decltype(s.mastery)>();s.adultRaised=j.at("adultRaised").get<decltype(s.adultRaised)>();s.careDays=j.at("careDays").get<decltype(s.careDays)>();
 for(const auto* values:{&s.totalEvents,&s.mastery,&s.adultRaised}){require(values->size()<=200,"Oversized progress map");for(const auto& [id,n]:*values)require(!id.empty()&&id.size()<128&&bounded(n),"Invalid progress count");}
 for(const auto& [id,days]:s.careDays)require(c.find(id)&&days.size()<=5&&std::set(days.begin(),days.end()).size()==days.size(),"Invalid care days");
 require(j.at("quests").is_object()&&j.at("quests").size()<=100,"Invalid quests");for(const auto& [id,v]:j.at("quests").items()){ObjectiveProgress q{v.at("count"),v.at("claimed")};require(bounded(q.count),"Invalid quest count");s.quests[id]=q;}
 const auto& st=j.at("settings");s.settings={st.at("reducedMotion"),st.at("sound"),st.at("music"),st.at("volume"),st.at("tankLook")};require(std::isfinite(s.settings.volume)&&s.settings.volume>=0&&s.settings.volume<=1&&s.settings.tankLook>=0&&s.settings.tankLook<=2,"Invalid settings");
 s.receipts=j.at("receipts");s.settlements=j.at("settlements");s.ledger=j.at("ledger");require(s.receipts.is_object()&&s.receipts.size()<=50000&&s.settlements.is_object()&&s.settlements.size()<=50000&&s.ledger.is_array()&&s.ledger.size()<=200000,"Invalid transaction storage");
 for(const auto& [id,r]:s.receipts.items()){
  require(!id.empty()&&id.size()<=128&&r.at("command").is_object()&&r.at("result").is_object(),"Invalid request receipt");const auto& result=r.at("result");
  const Amount coins=result.at("coins"),pearls=result.at("pearls");
  require(result.at("revision").get<std::uint64_t>()<=s.revision&&bounded(result.at("xp"))&&coins>=-limit&&coins<=limit&&pearls>=-limit&&pearls<=limit,"Invalid receipt result");
 }
 std::map<std::string,Amount> balances{{"coins",0},{"pearls",0},{"xp",0}};std::set<std::string> sources;
 for(std::size_t i=0;i<s.ledger.size();++i){const auto& entry=s.ledger[i];const std::string currency=entry.at("currency"),request=entry.at("request_id");const Amount amount=entry.at("amount");require(entry.at("id")==i+1&&balances.contains(currency)&&amount>=-limit&&amount<=limit,"Invalid ledger entry");
  auto& balance=balances[currency];require(amount>=-balance&&amount<=limit-balance,"Invalid ledger delta");balance+=amount;require(entry.at("balance_after")==balance,"Ledger does not reconcile");
  require(request=="initial"||request=="fixture"||s.receipts.contains(request),"Ledger is missing its receipt");const std::string key=currency+":"+entry.at("reason").get<std::string>()+":"+entry.at("source_id").get<std::string>();require(sources.insert(key).second,"Duplicate grant source");
 }
 require(balances["coins"]==s.wallet.coins&&balances["pearls"]==s.wallet.pearls&&balances["xp"]==s.xp,"Wallet does not match ledger");
 for(const auto& [id,r]:s.settlements.items()){
  const auto& result=r.at("result");const auto origin=result.at("fish").get<std::uint64_t>();const Amount principal=r.at("principal"),profit=r.at("profit");
  require(id==std::to_string(origin)&&origin>0&&origin<s.nextFishId&&bounded(principal)&&bounded(profit)&&principal<=limit-profit,"Invalid settlement");
  Fish settled;settled.age=r.at("stage");settled.egg=r.at("egg");settled.scripted=r.at("scripted");settled.purchase=r.at("purchase").get<GrowthSnapshot>();
  validatePurchase(settled.purchase);require(settled.age>=0&&settled.age<=4&&(!settled.egg||settled.age==0),"Invalid settled stage");
  const auto expected=fishReward(settled);const Amount xp=result.at("xp");
  require(expected.principal==principal&&expected.profit==profit&&result.at("coins")==principal+profit&&bounded(xp)&&xp<=expected.xp&&result.at("pearls")==0,"Invalid settlement amounts");
  const std::string disposition=r.at("disposition"),request=r.at("request_id"),species=r.at("species");
  require(c.find(species)&&!c.find(species)->companion&&(disposition=="rehome"||(disposition=="keep"&&settled.age==4)),"Invalid settled species or choice");
  require(s.receipts.contains(request)&&s.receipts.at(request).at("result")==result,"Settlement is missing its receipt");
  require(std::none_of(s.fish.begin(),s.fish.end(),[&](const auto& f){return f.id.value==origin;}),"Settled fish still growing");
 }
 for(const auto& f:s.companions)if(f.origin.value)require(s.settlements.contains(std::to_string(f.origin.value))&&s.settlements.at(std::to_string(f.origin.value)).at("disposition")=="keep","Kept fish is missing its settlement");
 return s;
}
LoadResult Storage::load(const Content& c){
 try{if(!std::filesystem::exists(path_))return {};return {decodeAndValidate(readFile(path_),c),"Progress restored.",false};}
 catch(const std::exception& e){protectOriginal_=true;std::string primary=e.what();
  for(const auto& suffix:{".recovered",".bak"}){try{auto p=std::filesystem::path(path_.string()+suffix);if(std::filesystem::exists(p))return {decodeAndValidate(readFile(p),c),"Original save preserved: "+primary+". Loaded recovery copy.",true};}catch(const std::exception&){/* Try the next separately preserved recovery candidate. */}}
  return {{},"Save preserved and not overwritten: "+primary+". New progress will use a separate recovery file.",true};
 }
}
std::string Storage::save(const State& s){
 try{
  auto destination=protectOriginal_?std::filesystem::path(path_.string()+".recovered"):path_;auto parent=destination.parent_path();if(!parent.empty())std::filesystem::create_directories(parent);auto temp=std::filesystem::path(destination.string()+".tmp");
  {std::ofstream out(temp,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Cannot create temporary save");const auto bytes=encode(s).dump();out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));out.flush();if(!out)throw std::runtime_error("Could not flush save");}
#if defined(__unix__) || defined(__APPLE__)
  int fd=::open(temp.c_str(),O_RDONLY);if(fd<0)throw std::runtime_error("Could not open temporary save for sync");const auto synced=::fsync(fd);::close(fd);if(synced!=0)throw std::runtime_error("Save sync failed");
#endif
  if(std::filesystem::exists(destination))std::filesystem::copy_file(destination,destination.string()+".bak",std::filesystem::copy_options::overwrite_existing);
#ifdef _WIN32
  if(std::filesystem::exists(destination))std::filesystem::remove(destination);
#endif
  std::filesystem::rename(temp,destination);
#if defined(__unix__) || defined(__APPLE__)
  if(!parent.empty()){int dir=::open(parent.c_str(),O_RDONLY);if(dir>=0){::fsync(dir);::close(dir);}}
#endif
  return {};
 }catch(const std::exception& e){return std::string("Progress is not saved. Keep the game open and retry. Details: ")+e.what();}
}
Session::Session(Content c,std::filesystem::path p,Millis wall,bool ephemeral):domain_(std::move(c),wall),storage_(std::move(p)),ephemeral_(ephemeral){
 if(!ephemeral_){auto loaded=storage_.load(domain_.content());if(loaded.preservedOriginal)recovery_=SessionNotification{SessionNotificationKind::Recovery,std::move(loaded.message)};if(loaded.state){domain_.install(std::move(*loaded.state));paused_=true;resume(wall);}}
}
const SessionNotification* Session::notification()const{return saveFailure_?&*saveFailure_:recovery_?&*recovery_:nullptr;}
void Session::dismissNotification(){if(!saveFailure_)recovery_.reset();}
const std::string& Session::status()const{static const std::string empty;const auto* notice=notification();return notice?notice->message:empty;}
Result Session::command(const Command& c){
 return domain_.execute(c,[&](const State& candidate){
  if(ephemeral_||c.action==Action::Move||c.action==Action::DropFood)return true;
  const auto message=storage_.save(candidate);
  if(!message.empty()){saveFailure_=SessionNotification{SessionNotificationKind::SaveFailure,message};return false;}
  saveFailure_.reset();sinceSave_=0;return true;
 });
}
void Session::update(Millis elapsed,Millis wall,Tool tool,FishId held){
 if(paused_||elapsed<=0)return;domain_.setCalendar(wall);domain_.advanceCare(elapsed);accumulator_+=elapsed;int steps=0;while(accumulator_>=20&&steps<16){domain_.stepMovement(.02,tool,held);accumulator_-=20;++steps;}if(accumulator_>=20)accumulator_%=20;sinceSave_+=elapsed;domain_.setWallAnchor(std::max(wall,domain_.state().wallAnchor));if(sinceSave_>=5000)checkpoint(wall);
}
bool Session::checkpoint(Millis wall){
 domain_.setWallAnchor(std::max(wall,domain_.state().wallAnchor));
 if(!ephemeral_){auto message=storage_.save(domain_.state());if(message.empty())saveFailure_.reset();else saveFailure_=SessionNotification{SessionNotificationKind::SaveFailure,std::move(message)};}
 sinceSave_=0;return !saveFailed();
}
void Session::suspend(Millis wall){if(paused_)return;paused_=true;domain_.clearTransient();accumulator_=0;checkpoint(wall);}
void Session::resume(Millis wall){if(!paused_)return;Millis elapsed=std::clamp<Millis>(wall-domain_.state().wallAnchor,0,315576000000LL);domain_.advanceCare(elapsed);domain_.clearTransient();domain_.setCalendar(wall);domain_.setWallAnchor(std::max(wall,domain_.state().wallAnchor));paused_=false;accumulator_=0;checkpoint(wall);}
}
