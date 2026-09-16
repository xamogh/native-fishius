#include "aquarium/storage.hpp"
#include <filesystem>
#include <chrono>
#include <fstream>
#include <iostream>
#include <functional>
#include <stdexcept>
using namespace aq;
#define CHECK(value) do{if(!(value))throw std::runtime_error(std::string("Line ")+std::to_string(__LINE__)+": "+#value);}while(false)
namespace {
void funding(Domain& d,Money money={10000000,1000},int level=1){
 auto s=d.state();s.wallet=money;s.xp=d.content().levels[level-1];s.lifetimeXp=s.xp;s.highestRewardedLevel=level;s.ledger=Json::array();s.receipts=Json::object();s.settlements=Json::object();s.revision=0;
 for(const auto& [currency,amount]:std::vector<std::pair<std::string,Amount>>{{"coins",money.coins},{"pearls",money.pearls},{"xp",s.xp}})if(amount)s.ledger.push_back({{"id",s.ledger.size()+1},{"currency",currency},{"amount",amount},{"balance_after",amount},{"reason","fixture"},{"source_id",currency},{"request_id","fixture"},{"created_at",s.calendarNow},{"revision",0}});
 d.install(s);
}
FishId buy(Domain& d,std::string id="neonTetra",std::string request=""){const auto r=d.execute({.action=Action::Buy,.key=id,.point={400,300},.requestId=request});CHECK(r);return r.fish;}
void age(Domain& d,FishId id,int stage){auto s=d.state();auto& f=*std::find_if(s.fish.begin(),s.fish.end(),[&](const auto& value){return value.id==id;});f.egg=false;f.age=stage;f.growthMs=f.purchase.durationMs/10000*f.purchase.stages[stage];f.lastFedAt=s.simNow;d.install(s);}
void roundTrip(Domain& d){auto encoded=encode(d.state());CHECK(encode(decodeAndValidate(encoded,d.content()))==encoded);}
void rejects(const std::function<void()>& action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}CHECK(failed);}
}
int main(int argc,char** argv){
 try{
 std::ifstream in(argc>1?argv[1]:"assets/content.json");const auto json=Json::parse(in);const auto content=Content::fromJson(json);
 std::vector<std::pair<std::string,std::function<void()>>> tests;
 const auto test=[&](std::string name,std::function<void()> body){tests.emplace_back(name,body);};
 test("v4 catalogs, release gates, zero purchase XP and startup",[&]{
  Domain d(content);CHECK(content.species.size()==99);CHECK(content.decorations.size()==120);CHECK(content.findDecor("CP-04")->price==305);CHECK(d.state().fish.size()==4);CHECK(d.state().wallet.coins==250);CHECK(d.state().xp==0);
  for(const auto& f:d.state().fish)CHECK(f.purchase.principal==0);
  for(const auto& s:content.species){CHECK(s.buyXp==0);if(s.releaseGate!="Launch")CHECK(!d.blocker(s));}
  for(const auto& dec:content.decorations)CHECK(dec.buyXp==0&&d.decorPurchaseXp(dec)==0);
  roundTrip(d);
 });
 test("all unlock-level production quotes follow workbook R25 exact rounding",[&]{
  for(const auto& row:json.at("species")){const auto* s=content.find(row.at("id").get<std::string>());if(s->companion||s->level>40)continue;
   const auto q=purchaseQuote(content,*s,s->level);CHECK(q.principal==s->price);CHECK(q.profit==row.at("preview_profit"));CHECK(q.xp==row.at("preview_xp"));
   for(int stage=1;stage<=4;++stage){Fish f;f.purchase=q;f.age=stage;const auto reward=fishReward(f);CHECK(reward.coins()==s->saleCoins[stage]);CHECK(reward.xp==s->saleXp[stage]);}
  }
  CHECK(purchaseQuote(content,*content.find("neonTetra"),1).principal==5);
  CHECK(purchaseQuote(content,*content.find("guppy"),1).profit==39);
  CHECK(purchaseQuote(content,*content.find("molly"),1).profit==607);
 });
 test("purchase snapshot survives level and configuration changes",[&]{
  Domain d(content);const auto id=buy(d);const auto snapshot=d.fish(id)->purchase;
  CHECK(snapshot.level==1&&snapshot.principal==5&&snapshot.profit==6&&snapshot.xp==2);CHECK(d.state().xp==0&&d.state().wallet.coins==245);
  auto changed=content;changed.configVersion="v4-tuning-2";auto* tetra=const_cast<Species*>(changed.find("neonTetra"));tetra->durationMs=28800000;tetra->feedMs=14400000;changed.economy.baseCoinDay=720;
  Domain next(changed);next.install(decodeAndValidate(encode(d.state()),changed));CHECK(next.fish(id)->purchase==snapshot);CHECK(next.quote(*tetra).profit>snapshot.profit);age(next,id,4);CHECK(next.execute({.action=Action::Sell,.fish=id}).coins==11);
 });
 test("six-second egg hatches hungry; only fed time advances total growth",[&]{
  Domain d(content);const auto id=buy(d);d.advanceCare(5999);CHECK(d.fish(id)->egg&&d.fish(id)->growthMs==5999);d.advanceCare(1);CHECK(!d.fish(id)->egg&&d.fish(id)->growthMs==6000);CHECK(careOf(*content.find("neonTetra"),*d.fish(id),d.state().simNow)==Care::Hungry);
  d.advanceCare(30*86400000LL);CHECK(d.fish(id)->growthMs==6000&&!d.fish(id)->dead);CHECK(d.execute({.action=Action::Feed,.fish=id}));const auto paid=d.state().wallet;
  d.advanceCare(600000);CHECK(d.fish(id)->growthMs==606000);d.advanceCare(600000);CHECK(d.fish(id)->growthMs==606000);CHECK(d.state().wallet.coins==paid.coins&&d.state().wallet.pearls==paid.pearls);
  CHECK(d.execute({.action=Action::Feed,.fish=id}));d.advanceCare(594000);CHECK(d.fish(id)->age==4&&d.fish(id)->growthMs==1200000);d.advanceCare(30*86400000LL);CHECK(d.fish(id)->growthMs==1200000);CHECK(d.state().xp==0);roundTrip(d);
 });
 test("stage boundaries, partial floor rounding and cancellation",[&]{
  for(int stage=1;stage<=4;++stage){const auto q=purchaseQuote(content,*content.find("neonTetra"),1);const auto boundary=q.durationMs*q.stages[stage]/10000;CHECK(growthStage(q,boundary-1)==stage-1);CHECK(growthStage(q,boundary)==stage);}
  for(int stage=0;stage<=4;++stage){Domain d(content);const auto id=buy(d);age(d,id,stage);auto reward=d.execute({.action=Action::Sell,.fish=id});CHECK(reward);CHECK((reward.coins==std::array<Amount,5>{5,4,6,8,11}[stage]));CHECK((reward.xp==std::array<Amount,5>{0,0,0,1,2}[stage]));roundTrip(d);}
  Domain d(content);const auto id=buy(d);CHECK(d.execute({.action=Action::Sell,.fish=id}).coins==5);CHECK(d.state().xp==0&&d.state().wallet.coins==250);
 });
 test("purchase retries, changed request payload and stale offers",[&]{
  Domain d(content);Command c{.action=Action::Buy,.key="neonTetra",.point={400,300},.requestId="purchase-1",.offer=d.quote(*content.find("neonTetra"))};const auto first=d.execute(c);CHECK(first);const auto saved=encode(d.state());CHECK(d.execute(c).replayed);CHECK(encode(d.state())==saved);c.key="guppy";CHECK(d.execute(c).error==Error::Conflict);CHECK(encode(d.state())==saved);
  c.key="neonTetra";c.requestId="purchase-2";c.offer->profit=999;CHECK(d.execute(c).error==Error::Conflict);CHECK(encode(d.state())==saved);roundTrip(d);
 });
 test("keep and rehome use one terminal claim across retries and reloads",[&]{
  for(bool keep:{false,true}){Domain d(content);const auto id=buy(d);age(d,id,4);Command c{.action=keep?Action::Keep:Action::Sell,.fish=id,.requestId="claim-1"};const auto result=d.execute(c);CHECK(result.coins==11&&result.xp==2);CHECK(!d.fish(id));CHECK(bool(d.companion(id))==keep);CHECK(d.living({1})==4);
   const auto saved=encode(d.state());d.install(decodeAndValidate(saved,content));CHECK(d.execute(c).replayed);CHECK(encode(d.state())==saved);const auto other=d.execute({.action=keep?Action::Sell:Action::Keep,.fish=id,.requestId="racing-other-claim"});CHECK(other.replayed);CHECK(d.state().wallet.coins==256&&d.state().xp==2);CHECK(bool(d.companion(id))==keep);roundTrip(d);
  }
 });
 test("gift basis is zero, scripted fish do not add mastery",[&]{
  Domain d(content);const auto id=d.state().fish.front().id;age(d,id,4);const auto r=d.execute({.action=Action::Keep,.fish=id});CHECK(r.coins==6&&r.xp==2);CHECK(d.state().adultRaised.at("neonTetra")==1);
  const auto demo=buy(d);age(d,demo,4);auto state=d.state();state.fish.back().scripted=true;d.install(state);const auto reward=d.execute({.action=Action::Keep,.fish=demo});CHECK(reward.xp==0&&d.state().adultRaised.at("neonTetra")==1);
 });
 test("display capacity stores excess safely and never blocks Keep",[&]{
  Domain d(content);funding(d);for(int i=0;i<9;++i){const auto id=buy(d);age(d,id,4);CHECK(d.execute({.action=Action::Keep,.fish=id}));}
  CHECK(d.state().companions.size()==9&&d.displaying({1})==8&&d.state().companions.back().stored&&d.living({1})==4);
  const auto stored=d.state().companions.back().id;CHECK(d.execute({.action=Action::Restore,.fish=stored,.point={500,300}}).error==Error::Full);
  const auto first=d.state().companions.front().id;CHECK(d.execute({.action=Action::Stash,.fish=first}));CHECK(d.execute({.action=Action::Restore,.fish=stored,.point={500,300}}));CHECK(d.displaying({1})==8);roundTrip(d);
 });
 test("permanent premium fish and free one-time Bubble Eye produce no coins or XP",[&]{
  Domain d(content);funding(d,{10000,100},5);const auto before=d.state().wallet;const auto xp=d.state().xp;const auto koi=buy(d,"koi");CHECK(!d.fish(koi)&&d.companion(koi));CHECK(d.state().wallet.pearls==before.pearls-18&&d.state().xp==xp);CHECK(d.execute({.action=Action::Sell,.fish=koi}).error==Error::InvalidFish);
  CHECK(!sellable(*content.find("koi"),companionVisual(*d.companion(koi))));
  buy(d,"bubbleEyeGoldfish");CHECK(d.execute({.action=Action::Buy,.key="bubbleEyeGoldfish",.point={400,300}}).error==Error::Claimed);CHECK(d.state().xp==xp);roundTrip(d);
 });
 test("favorite protects rehome but permits Keep",[&]{Domain d(content);const auto id=buy(d);CHECK(d.execute({.action=Action::Favorite,.fish=id}));CHECK(d.execute({.action=Action::Sell,.fish=id}).error==Error::Protected);age(d,id,4);CHECK(d.execute({.action=Action::Keep,.fish=id}));CHECK(d.companion(id)->favorite);});
 test("local commit failure rolls back wallet, fish, receipt and events",[&]{
  Domain d(content);d.takeEvents();const auto before=encode(d.state());CHECK(d.execute({.action=Action::Buy,.key="neonTetra",.point={400,300},.requestId="retry-after-save"},[](const State&){return false;}).error==Error::SaveFailure);CHECK(encode(d.state())==before&&d.takeEvents().empty());CHECK(buy(d,"neonTetra","retry-after-save").value>0);roundTrip(d);
  const auto path=std::filesystem::temp_directory_path()/("aquarium-v4-blocked-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));{std::ofstream file(path);file<<"blocks a directory";}Session session(content,path/"save.json",100000000);const auto initial=encode(session.domain().state());CHECK(session.command({.action=Action::Buy,.key="neonTetra",.point={400,300}}).error==Error::SaveFailure);CHECK(encode(session.domain().state())==initial&&session.saveFailed());std::filesystem::remove(path);
 });
 test("every capacity entitlement uses either currency and tops out at 100",[&]{
  for(auto currency:{Currency::Coins,Currency::Pearls}){Domain d(content);funding(d,{10000000,10000},40);for(const auto& e:content.tankEntitlements){if(e.id=="TK-01-10")continue;const auto old=d.state().wallet;const auto r=d.execute({.action=d.tank(e.tank)?Action::ExpandTank:Action::UnlockTank,.tank=e.tank,.currency=currency});CHECK(r);CHECK(d.tank(e.tank)->slots==e.slots);CHECK(old.coins-d.state().wallet.coins==(currency==Currency::Coins?e.cost.coins:0));CHECK(old.pearls-d.state().wallet.pearls==(currency==Currency::Pearls?e.cost.pearls:0));}
   int sum=0;for(const auto& t:d.state().tanks){sum+=t.slots;CHECK(t.slots==20);}CHECK(sum==100);roundTrip(d);
  }
  Domain d(content);CHECK(d.execute({.action=Action::ExpandTank,.tank={1}}).error==Error::Level);
 });
 test("account XP clamps at 40; reward receipt reports the actual XP",[&]{
  Domain d(content);funding(d,{100000,100},40);const auto id=buy(d);age(d,id,4);const auto old=d.state().lifetimeXp;const auto r=d.execute({.action=Action::Sell,.fish=id});CHECK(r.xp==0&&d.state().xp==content.levels.back()&&d.state().lifetimeXp>old);roundTrip(d);
 });
 test("no decor purchase XP and no sickness, death or paid revival",[&]{
  Domain d(content);CHECK(d.execute({.action=Action::BuyDecor,.key="CP-01",.point={400,500}}));CHECK(d.state().xp==0&&d.state().wallet.coins==225);const auto before=encode(d.state());CHECK(!d.execute({.action=Action::ReviveAll}));CHECK(encode(d.state())==before);d.advanceCare(90*86400000LL);for(const auto& f:d.state().fish)CHECK(!f.dead&&careOf(*content.find(f.species),f,d.state().simNow)==Care::Hungry);roundTrip(d);
 });
 test("offline catch-up is applied once and stops at the meal boundary",[&]{
  Session session(content,std::filesystem::temp_directory_path()/"aquarium-v4-ephemeral.json",100000000,true);const auto id=session.domain().state().fish.front().id;CHECK(session.command({.action=Action::Feed,.fish=id}));session.suspend(100000000);session.resume(100000000+86400000);const auto growth=session.domain().fish(id)->growthMs;CHECK(growth==606000);session.resume(100000000+86400000);CHECK(session.domain().fish(id)->growthMs==growth);session.suspend(100000000+86400000);session.resume(1);CHECK(session.domain().fish(id)->growthMs==growth);roundTrip(session.domain());
 });
 test("strict new save validation rejects legacy, duplicate grants and altered balances",[&]{
  Domain d(content);auto j=encode(d.state());j["version"]=1;rejects([&]{decodeAndValidate(j,content);});j=encode(d.state());j["coins"]=999;rejects([&]{decodeAndValidate(j,content);});j=encode(d.state());j["fish"][0]["purchase"]["durationMs"]=0;rejects([&]{decodeAndValidate(j,content);});j=encode(d.state());j["fish"][0]["purchase"]["stages"]={0,5500,2500,8000,10000};rejects([&]{decodeAndValidate(j,content);});
  const auto id=buy(d);age(d,id,4);CHECK(d.execute({.action=Action::Keep,.fish=id}));j=encode(d.state());
  j["settlements"][std::to_string(id.value)]["profit"]=9'000'000'000'000'000LL;
  rejects([&]{decodeAndValidate(j,content);});j=encode(d.state());j["receipts"]=Json::object();rejects([&]{decodeAndValidate(j,content);});
 });
 test("invalid schedules, currency, level curve and duplicate capacity are rejected",[&]{
  auto j=json;j["species"][0]["schedule_id"]="missing";rejects([&]{Content::fromJson(j);});
  j=json;j["species"][0]["currency"]="unknown";rejects([&]{Content::fromJson(j);});
  j=json;j["levels"][3]=j["levels"][2];rejects([&]{Content::fromJson(j);});
  j=json;j["tank_entitlements"][2]["tank"]=1;rejects([&]{Content::fromJson(j);});
 });
 int failed=0;for(const auto& [name,run]:tests){try{run();std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
 std::cout<<tests.size()-failed<<" / "<<tests.size()<<" v4 tests passed\n";return failed?1:0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
