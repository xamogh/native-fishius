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
 test("v4 catalogs, release gates, purchase XP policies and startup",[&]{
  Domain d(content);CHECK(content.species.size()==99);CHECK(content.decorations.size()==120);CHECK(content.findDecor("CP-04")->price==305);CHECK(d.state().fish.size()==4);CHECK(d.state().wallet.coins==250);CHECK(d.state().xp==0);
  for(const auto& f:d.state().fish)CHECK(f.purchase.principal==0);
  for(const auto& s:content.species){CHECK(s.buyXp==0);if(s.releaseGate!="Launch")CHECK(!d.blocker(s));}
  for(const auto& dec:content.decorations)CHECK(dec.buyXp>0&&d.decorPurchaseXp(dec)==dec.buyXp);
  roundTrip(d);
 });
 test("all unlock-level production quotes follow workbook R25 exact rounding",[&]{
  for(const auto& row:json.at("species")){const auto* s=content.find(row.at("id").get<std::string>());if(s->level>40)continue;
   const auto q=purchaseQuote(content,*s,s->level);CHECK(q.principal==(s->currency==Currency::Coins?s->price:0));CHECK(q.profit==row.at("preview_profit"));CHECK(q.xp==row.at("preview_xp"));
   for(int stage=1;stage<=4;++stage){Fish f;f.purchase=q;f.age=stage;const auto reward=fishReward(f);CHECK(reward.coins()==s->saleCoins[stage]);CHECK(reward.xp==s->saleXp[stage]);}
  }
  CHECK(purchaseQuote(content,*content.find("neonTetra"),1).principal==5);
  CHECK(purchaseQuote(content,*content.find("guppy"),1).profit==39);
  CHECK(purchaseQuote(content,*content.find("molly"),1).profit==607);
 });
 test("Treasure quotes use level reference income and exact whole-coin rounding",[&]{
  const std::array<int,5> coinPrices{99,299,499,999,1999},pearlPrices{199,499,999,1999,2999};
  const std::array<Amount,5> level5{1328,4514,7965,17258,37170},level10{3161,10748,18968,41096,88515},level40{20663,70253,123975,268613,578550},pearls{20,55,120,260,420};
  int coinIndex=0,pearlIndex=0,bundles=0;
  for(const auto& offer:content.treasureOffers){
   CHECK(offer.level==0);CHECK(!offer.asset.empty());
   if(offer.kind==TreasureKind::Coins){const int i=coinIndex++;CHECK(offer.priceUsdCents==coinPrices.at(i));CHECK(treasureContents(content,offer,5).coins==level5.at(i));CHECK(treasureContents(content,offer,10).coins==level10.at(i));CHECK(treasureContents(content,offer,40).coins==level40.at(i));CHECK(treasureContents(content,offer,10).pearls==0);}
   else if(offer.kind==TreasureKind::Pearls){const int i=pearlIndex++;CHECK(offer.priceUsdCents==pearlPrices.at(i));for(int level:{1,5,10,40}){const auto reward=treasureContents(content,offer,level);CHECK(reward.coins==0&&reward.pearls==pearls.at(i));}}
   else{++bundles;CHECK(offer.oncePerAccount&&offer.permanentFrame&&offer.priceUsdCents==399);const auto reward=treasureContents(content,offer,10);CHECK(reward.coins==3161&&reward.pearls==35);}
   rejects([&]{treasureContents(content,offer,0);});rejects([&]{treasureContents(content,offer,41);});
  }
  CHECK(coinIndex==5&&pearlIndex==5&&bundles==1);
  auto malformed=json;malformed["treasure"]["offers"][0]["price_usd_cents"]=0;rejects([&]{Content::fromJson(malformed);});
  malformed=json;malformed["treasure"]["offers"].erase(0);rejects([&]{Content::fromJson(malformed);});
  malformed=json;malformed["treasure"]["offers"][0]["level"]=-1;rejects([&]{Content::fromJson(malformed);});
 });
 test("purchase snapshot survives level and configuration changes",[&]{
  Domain d(content);const auto id=buy(d);const auto snapshot=d.fish(id)->purchase;
  CHECK(snapshot.level==1&&snapshot.principal==5&&snapshot.profit==6&&snapshot.xp==10);CHECK(d.state().xp==0&&d.state().wallet.coins==245);
  auto changed=content;changed.configVersion="v4-tuning-2";auto* tetra=const_cast<Species*>(changed.find("neonTetra"));tetra->durationMs=28800000;tetra->feedMs=14400000;changed.economy.baseCoinDay=720;
  Domain next(changed);next.install(decodeAndValidate(encode(d.state()),changed));CHECK(next.fish(id)->purchase==snapshot);CHECK(next.quote(*tetra).profit>snapshot.profit);age(next,id,4);CHECK(next.execute({.action=Action::Sell,.fish=id}).coins==11);
 });
 test("six-second egg hatches hungry; only fed time advances total growth",[&]{
  Domain d(content);const auto id=buy(d);d.advanceCare(5999);CHECK(d.fish(id)->egg&&d.fish(id)->growthMs==5999);d.advanceCare(1);CHECK(!d.fish(id)->egg&&d.fish(id)->growthMs==6000);CHECK(careOf(*content.find("neonTetra"),*d.fish(id),d.state().simNow)==Care::Hungry);
  d.advanceCare(30*86400000LL);CHECK(d.fish(id)->growthMs==6000&&!d.fish(id)->dead);CHECK(d.execute({.action=Action::Feed,.fish=id}));const auto paid=d.state().wallet;
  d.advanceCare(600000);CHECK(d.fish(id)->growthMs==606000);d.advanceCare(600000);CHECK(d.fish(id)->growthMs==606000);CHECK(d.state().wallet.coins==paid.coins&&d.state().wallet.pearls==paid.pearls);
  CHECK(d.execute({.action=Action::Feed,.fish=id}));d.advanceCare(594000);CHECK(d.fish(id)->age==4&&d.fish(id)->growthMs==1200000);d.advanceCare(30*86400000LL);CHECK(d.fish(id)->growthMs==1200000);CHECK(d.state().xp==0);roundTrip(d);
 });
 test("stage boundaries and partial floor rounding from Junior onward",[&]{
  for(int stage=1;stage<=4;++stage){const auto q=purchaseQuote(content,*content.find("neonTetra"),1);const auto boundary=q.durationMs*q.stages[stage]/10000;CHECK(growthStage(q,boundary-1)==stage-1);CHECK(growthStage(q,boundary)==stage);}
  for(int stage=1;stage<=4;++stage){Domain d(content);const auto id=buy(d);age(d,id,stage);CHECK(sellable(*content.find("neonTetra"),*d.fish(id)));auto reward=d.execute({.action=Action::Sell,.fish=id});CHECK(reward);CHECK((reward.coins==std::array<Amount,5>{0,4,6,8,11}[stage]));CHECK((reward.xp==std::array<Amount,5>{0,1,4,7,10}[stage]));roundTrip(d);}
 });
 test("eggs and Babies cannot settle; selling unlocks exactly at Junior",[&]{
  Domain d(content);const auto id=buy(d);d.takeEvents();
  const auto blocked=[&]{d.takeEvents();const auto before=encode(d.state());CHECK(!sellable(*content.find("neonTetra"),*d.fish(id)));CHECK(d.execute({.action=Action::Sell,.fish=id,.requestId="young-sale"}).error==Error::NotReady);CHECK(encode(d.state())==before&&d.takeEvents().empty());roundTrip(d);};
  blocked();d.advanceCare(content.hatchMs);CHECK(!d.fish(id)->egg&&d.fish(id)->age==0);blocked();
  CHECK(d.execute({.action=Action::Feed,.fish=id}));d.takeEvents();
  const auto& q=d.fish(id)->purchase;const auto junior=q.durationMs*q.stages[1]/10000;
  d.advanceCare(junior-content.hatchMs-1);blocked();d.advanceCare(1);CHECK(d.fish(id)->age==1);
  CHECK(d.execute({.action=Action::Sell,.fish=id,.requestId="young-sale"}));roundTrip(d);
 });
 test("next-stage meter follows uneven saved thresholds and resets at each stage",[&]{
  Fish f;f.purchase.durationMs=10003;f.purchase.stages={0,2500,5500,8000,10000};
  const std::array<Millis,5> boundaries{0,2500,5501,8002,10003};
  for(int stage=0;stage<4;++stage){
   f.age=stage;f.growthMs=boundaries[stage];CHECK(nextStageProgress(f)==0.);
   f.growthMs=(boundaries[stage]+boundaries[stage+1])/2;CHECK(nextStageProgress(f)>.499&&nextStageProgress(f)<.501);
   f.growthMs=boundaries[stage+1]-1;CHECK(nextStageProgress(f)>.99&&nextStageProgress(f)<1.);
   ++f.growthMs;f.age=growthStage(f.purchase,f.growthMs);CHECK(f.age==stage+1);CHECK(nextStageProgress(f)==(f.age==4?1.:0.));
  }
  f.egg=true;f.age=0;f.boughtAt=1000;f.hatchAt=7000;f.growthMs=3000;CHECK(nextStageProgress(f)==.5);
  f.egg=false;f.age=4;CHECK(nextStageProgress(f)==1.);
 });
 test("historical baby refunds still validate and replay",[&]{
  for(bool egg:{false,true}){Domain d(content);const auto id=buy(d);age(d,id,4);auto state=d.state();auto& f=state.fish.back();f.purchase.profit=0;f.purchase.xp=0;d.install(state);
   const Command claim{.action=Action::Sell,.fish=id,.requestId="historical-refund"};const auto settled=d.execute(claim);CHECK(settled.coins==5&&settled.xp==0);
   auto saved=encode(d.state());auto& receipt=saved["settlements"][std::to_string(id.value)];receipt["stage"]=0;receipt["egg"]=egg;
   d.install(decodeAndValidate(saved,content));const auto before=encode(d.state());CHECK(d.execute(claim).replayed);CHECK(encode(d.state())==before);
  }
 });
 test("purchase retries, changed request payload and stale offers",[&]{
  Domain d(content);Command c{.action=Action::Buy,.key="neonTetra",.point={400,300},.requestId="purchase-1",.offer=d.quote(*content.find("neonTetra"))};const auto first=d.execute(c);CHECK(first);const auto saved=encode(d.state());CHECK(d.execute(c).replayed);CHECK(encode(d.state())==saved);c.key="guppy";CHECK(d.execute(c).error==Error::Conflict);CHECK(encode(d.state())==saved);
  c.key="neonTetra";c.requestId="purchase-2";c.offer->profit=999;CHECK(d.execute(c).error==Error::Conflict);CHECK(encode(d.state())==saved);roundTrip(d);
 });
 test("sales pay once across different requests, retries and reloads",[&]{
  Domain d(content);const auto id=buy(d);age(d,id,4);Command c{.action=Action::Sell,.fish=id,.requestId="claim-1"};const auto result=d.execute(c);CHECK(result.coins==11&&result.xp==10);CHECK(!d.fish(id));CHECK(d.living({1})==4);
  const auto saved=encode(d.state());d.install(decodeAndValidate(saved,content));CHECK(d.execute(c).replayed);CHECK(encode(d.state())==saved);const auto other=d.execute({.action=Action::Sell,.fish=id,.requestId="racing-other-claim"});CHECK(other.replayed);CHECK(d.state().wallet.coins==256&&d.state().xp==10);CHECK(!d.fish(id));roundTrip(d);
 });
 test("gift basis is zero, scripted fish do not add mastery",[&]{
  Domain d(content);const auto id=d.state().fish.front().id;age(d,id,4);const auto r=d.execute({.action=Action::Sell,.fish=id});CHECK(r.coins==6&&r.xp==10);CHECK(d.state().adultRaised.at("neonTetra")==1);
  const auto demo=buy(d);age(d,demo,4);auto state=d.state();state.fish.back().scripted=true;d.install(state);const auto reward=d.execute({.action=Action::Sell,.fish=demo});CHECK(reward.xp==0&&d.state().adultRaised.at("neonTetra")==1);
 });
 test("eggs, adults and premium fish share purchase and restore capacity",[&]{
  Domain d(content);funding(d,{100000,1000},5);
  const auto premium=buy(d,"koi");CHECK(d.living({1})==5);
  const auto adult=buy(d);age(d,adult,4);CHECK(d.execute({.action=Action::Favorite,.fish=adult}));
  while(d.living({1})<10)buy(d);
  const auto full=encode(d.state());
  for(const auto* species:{"neonTetra","koi","bubbleEyeGoldfish"}){CHECK(d.execute({.action=Action::Buy,.key=species,.point={400,300}}).error==Error::Full);CHECK(encode(d.state())==full);}
  CHECK(d.execute({.action=Action::Stash,.fish=premium}).error==Error::NotReady);age(d,premium,4);
  CHECK(d.execute({.action=Action::Stash,.fish=premium}));CHECK(d.living({1})==9);buy(d);
  const auto blocked=encode(d.state());CHECK(d.execute({.action=Action::Restore,.fish=premium,.point={500,300}}).error==Error::Full);CHECK(encode(d.state())==blocked);
  CHECK(d.execute({.action=Action::ExpandTank,.tank={1}}));CHECK(d.execute({.action=Action::Restore,.fish=premium,.point={500,300}}));CHECK(d.living({1})==11&&d.fish(adult)->favorite);
  roundTrip(d);
  auto invalid=encode(d.state());invalid["tanks"][0]["slots"]=10;rejects([&]{decodeAndValidate(invalid,content);});
  Domain premiumTank(content);funding(premiumTank,{100000,1000},5);auto empty=premiumTank.state();empty.fish.clear();premiumTank.install(empty);
  for(int i=0;i<10;++i)buy(premiumTank,"koi");
  CHECK(premiumTank.living({1})==10&&premiumTank.state().fish.size()==10);roundTrip(premiumTank);
 });
 test("older v4 adults retain ownership and paid rewards under shared capacity",[&]{
  Domain d(content);funding(d,{10000,100});Json adults=Json::array();
  for(int i=0;i<8;++i){
   const auto id=buy(d);age(d,id,4);CHECK(d.execute({.action=Action::Favorite,.fish=id}));
   const auto fish=encode(d.state()).at("fish").back();CHECK(d.execute({.action=Action::Favorite,.fish=id}));CHECK(d.execute({.action=Action::Sell,.fish=id}));
   adults.push_back({{"id",id.value},{"origin",id.value},{"species",fish.at("species")},{"tank",1},{"position",fish.at("position")},{"stored",false},{"favorite",true},{"lastFedAt",fish.at("lastFedAt")},{"motion",fish.at("motion")}});
  }
  while(d.living({1})<10)buy(d);
  auto legacy=encode(d.state());legacy.erase("fishCapacity");legacy.erase("fishLifecycle");legacy["companions"]=adults;
  for(auto& [id,settlement]:legacy["settlements"].items()){
   settlement["disposition"]="keep";
   settlement.erase("pearl_reward");
   legacy["receipts"][settlement.at("request_id").get<std::string>()]["command"]["action"]=33;
  }
  d.install(decodeAndValidate(legacy,content));CHECK(d.living({1})==10&&d.state().fish.size()==18);
  CHECK(d.state().wallet.coins==legacy.at("coins")&&d.state().xp==legacy.at("xp"));
  for(std::size_t i=10;i<18;++i){const auto& fish=d.state().fish[i];CHECK(fish.stashed&&fish.favorite&&fish.id.value>=legacy.at("nextFishId").get<std::uint64_t>());CHECK(fishReward(fish).coins()==0&&fishReward(fish).xp==0);}
  const auto before=encode(d.state());const auto id=d.state().fish.back().id;
  CHECK(!before.contains("companions"));
  CHECK(d.execute({.action=static_cast<Action>(33),.fish=id}).error==Error::Unavailable);CHECK(encode(d.state())==before);roundTrip(d);
  CHECK(d.execute({.action=Action::ExpandTank,.tank={1}}));CHECK(d.execute({.action=Action::Restore,.fish=id,.point={500,300}}));
  CHECK(d.execute({.action=Action::Sell,.fish=id}).error==Error::Protected);CHECK(d.execute({.action=Action::Favorite,.fish=id}));
  const auto wallet=d.state().wallet;const auto xp=d.state().xp;const auto raised=d.state().adultRaised;
  const auto sold=d.execute({.action=Action::Sell,.fish=id});CHECK(sold&&sold.coins==0&&sold.xp==0&&!d.fish(id));
  CHECK(d.state().wallet.coins==wallet.coins&&d.state().wallet.pearls==wallet.pearls&&d.state().xp==xp&&d.state().adultRaised==raised);roundTrip(d);
 });
 test("pearl fish hatch, pause when hungry and sell for coins and XP once",[&]{
  for(const auto* species:{"bubbleEyeGoldfish","koi"}){
   Domain d(content);funding(d,{10000,100},5);const auto before=d.state().wallet;const auto xp=d.state().xp;
   const auto id=buy(d,species,"pearl-egg");const auto q=d.fish(id)->purchase;
   CHECK(d.fish(id)->egg&&q.durationMs==7200000&&q.principal==0&&q.profit>0&&q.xp>0);
   CHECK(d.state().wallet.pearls==before.pearls-content.find(species)->price&&d.state().wallet.coins==before.coins&&d.state().xp==xp);
   const auto bought=encode(d.state());CHECK(buy(d,species,"pearl-egg")==id&&encode(d.state())==bought);
   CHECK(d.execute({.action=Action::Sell,.fish=id}).error==Error::NotReady);
   d.advanceCare(content.hatchMs);CHECK(!d.fish(id)->egg&&d.fish(id)->age==0);
   d.advanceCare(q.durationMs);CHECK(d.fish(id)->growthMs==content.hatchMs);
   CHECK(d.execute({.action=Action::Feed,.fish=id}));d.advanceCare(q.feedMs);CHECK(d.fish(id)->age==1);
   const auto paused=d.fish(id)->growthMs;d.advanceCare(1000);CHECK(d.fish(id)->growthMs==paused);
   CHECK(d.execute({.action=Action::Feed,.fish=id}));d.advanceCare(q.durationMs);CHECK(d.fish(id)->age==4&&sellable(*content.find(species),*d.fish(id)));
   roundTrip(d);const Command sale{.action=Action::Sell,.fish=id,.requestId="pearl-sale"};const auto sold=d.execute(sale);
   CHECK(sold&&sold.coins==q.profit&&sold.xp==q.xp&&sold.pearls==0&&!d.fish(id));
   const auto saved=encode(d.state());d.install(decodeAndValidate(saved,content));CHECK(d.execute(sale).replayed&&encode(d.state())==saved);roundTrip(d);
  }
 });
 test("Bubble Eye costs one pearl per egg and permits repeat purchases",[&]{
  Domain d(content);funding(d,{1000,0},2);const auto before=encode(d.state());
  CHECK(d.execute({.action=Action::Buy,.key="bubbleEyeGoldfish",.point={400,300}}).error==Error::Funds);CHECK(encode(d.state())==before);
  funding(d,{1000,2},2);const auto first=buy(d,"bubbleEyeGoldfish"),second=buy(d,"bubbleEyeGoldfish");
  CHECK(first!=second&&d.state().wallet.pearls==0&&d.state().wallet.coins==1000&&d.state().claims.empty());roundTrip(d);
 });
 test("old premium fish migrate as sellable adults and old free claims do not block purchases",[&]{
  Domain d(content);funding(d,{1000,3},2);const auto id=buy(d,"bubbleEyeGoldfish");auto legacy=encode(d.state());
  const auto fish=legacy["fish"].back();legacy["fish"].erase(legacy["fish"].size()-1);legacy.erase("fishLifecycle");legacy["claims"].push_back("bubbleEyeGoldfish");
  legacy["companions"]=Json::array({{{"id",id.value},{"origin",0},{"species","bubbleEyeGoldfish"},{"tank",1},{"position",fish.at("position")},{"stored",false},{"favorite",true},{"lastFedAt",-43200000},{"motion",fish.at("motion")}}});
  d.install(decodeAndValidate(legacy,content));CHECK(d.fish(id)&&d.fish(id)->age==4&&d.fish(id)->favorite&&d.living({1})==5);
  CHECK(d.state().wallet.coins==legacy["coins"]&&d.state().wallet.pearls==legacy["pearls"]&&d.state().xp==legacy["xp"]);
  CHECK(!encode(d.state()).contains("companions"));roundTrip(d);
  const auto second=buy(d,"bubbleEyeGoldfish");CHECK(second!=id&&d.fish(second)->egg);
  CHECK(d.execute({.action=Action::Sell,.fish=id}).error==Error::Protected);CHECK(d.execute({.action=Action::Favorite,.fish=id}));
  const auto sold=d.execute({.action=Action::Sell,.fish=id});CHECK(sold&&sold.coins>0&&sold.xp>0&&!d.fish(id));roundTrip(d);
  auto invalid=legacy;invalid["companions"][0]["origin"]=id.value;rejects([&]{decodeAndValidate(invalid,content);});
  invalid=legacy;invalid["companions"].push_back(invalid["companions"][0]);rejects([&]{decodeAndValidate(invalid,content);});
 });
 test("favorites retain adults and their sale value without paying or freeing space",[&]{
  Domain d(content);const auto id=buy(d);CHECK(d.execute({.action=Action::Favorite,.fish=id}));CHECK(d.execute({.action=Action::Sell,.fish=id}).error==Error::Protected);age(d,id,4);
  const auto wallet=d.state().wallet;const auto xp=d.state().xp;const auto payout=fishReward(*d.fish(id));
  d.advanceCare(30*86400000LL);CHECK(d.fish(id)&&d.fish(id)->favorite&&d.fish(id)->age==4);
  CHECK(d.living({1})==5&&d.state().wallet.coins==wallet.coins&&d.state().xp==xp&&d.state().settlements.empty());
  d.install(decodeAndValidate(encode(d.state()),content));d.takeEvents();const auto before=encode(d.state());
  CHECK(d.execute({.action=static_cast<Action>(33),.fish=id}).error==Error::Unavailable);
  CHECK(d.execute({.action=Action::Sell,.fish=id}).error==Error::Protected);CHECK(encode(d.state())==before&&d.takeEvents().empty());
  CHECK(d.execute({.action=Action::Favorite,.fish=id}));const auto sold=d.execute({.action=Action::Sell,.fish=id});CHECK(sold.coins==payout.coins()&&sold.xp==payout.xp&&!d.fish(id)&&d.living({1})==4);roundTrip(d);
 });
 test("local commit failure rolls back wallet, fish, receipt and events",[&]{
  Domain d(content);d.takeEvents();const auto before=encode(d.state());CHECK(d.execute({.action=Action::Buy,.key="neonTetra",.point={400,300},.requestId="retry-after-save"},[](const State&){return false;}).error==Error::SaveFailure);CHECK(encode(d.state())==before&&d.takeEvents().empty());CHECK(buy(d,"neonTetra","retry-after-save").value>0);roundTrip(d);
  const auto path=std::filesystem::temp_directory_path()/("aquarium-v4-blocked-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));{std::ofstream file(path);file<<"blocks a directory";}Session session(content,path/"save.json",100000000);const auto initial=encode(session.domain().state());CHECK(session.command({.action=Action::Buy,.key="neonTetra",.point={400,300}}).error==Error::SaveFailure);CHECK(encode(session.domain().state())==initial&&session.saveFailed());std::filesystem::remove(path);
 });
 test("only new tanks require levels; every owned tank can expand to 40 with either currency",[&]{
  for(auto currency:{Currency::Coins,Currency::Pearls}){
   Domain d(content);
   const Money funds=currency==Currency::Coins?Money{100000000,0}:Money{0,10000};
   funding(d,funds);
   for(int tank=1;tank<=5;++tank){
    const TankId id{tank};const int unlockLevel=content.tankLevels[tank-1];
    if(tank>1){
     funding(d,funds,unlockLevel-1);const auto locked=encode(d.state());
     CHECK(d.execute({.action=Action::UnlockTank,.tank=id,.currency=currency}).error==Error::Level);
     CHECK(d.execute({.action=Action::ExpandTank,.tank=id,.currency=currency}).error==Error::Unknown);
     CHECK(encode(d.state())==locked);funding(d,funds,unlockLevel);
     CHECK(d.execute({.action=Action::UnlockTank,.tank=id,.currency=currency}));CHECK(d.tank(id)->slots==10);
    }
    for(int capacity:{15,20,25,30,35,40}){
     const auto* next=d.nextTankEntitlement(id);CHECK(next&&next->slots==capacity);
     const auto cost=next->cost,old=d.state().wallet;
     CHECK(d.execute({.action=Action::ExpandTank,.tank=id,.currency=currency}));
     CHECK(d.tank(id)->slots==capacity&&d.level()==unlockLevel);
     CHECK(old.coins-d.state().wallet.coins==(currency==Currency::Coins?cost.coins:0));
     CHECK(old.pearls-d.state().wallet.pearls==(currency==Currency::Pearls?cost.pearls:0));roundTrip(d);
    }
    const auto maxed=encode(d.state());CHECK(!d.nextTankEntitlement(id));
    CHECK(d.execute({.action=Action::ExpandTank,.tank=id,.currency=currency}).error==Error::Maximum);
    CHECK(encode(d.state())==maxed);
   }
   int sum=0;for(const auto& t:d.state().tanks)sum+=t.slots;CHECK(sum==200);
  }
  Domain d(content);CHECK(d.execute({.action=Action::ExpandTank,.tank={1}}).error==Error::Funds);
 });
 test("tank upgrades preserve state on insufficient funds or save failure and replay once",[&]{
  for(auto currency:{Currency::Coins,Currency::Pearls}){
   Domain d(content);funding(d,currency==Currency::Coins?Money{299,1000}:Money{100000,1});
   auto before=encode(d.state());Command command{.action=Action::ExpandTank,.tank={1},.currency=currency,.requestId="tank-upgrade"};
   const auto shortfall=d.execute(command);CHECK(shortfall.error==Error::Funds);
   CHECK(shortfall.shortfall.coins==(currency==Currency::Coins?1:0));
   CHECK(shortfall.shortfall.pearls==(currency==Currency::Pearls?1:0));CHECK(encode(d.state())==before);
   funding(d,{100000,1000});d.takeEvents();before=encode(d.state());
   CHECK(d.execute(command,[](const State&){return false;}).error==Error::SaveFailure);
   CHECK(encode(d.state())==before&&d.takeEvents().empty());
   CHECK(d.execute(command));CHECK(d.tank({1})->slots==15);roundTrip(d);
   d.install(decodeAndValidate(encode(d.state()),content));before=encode(d.state());
   CHECK(d.execute(command).replayed);CHECK(encode(d.state())==before&&d.tank({1})->slots==15);
  }
 });
 test("five full 40-fish tanks survive saving and reject a forty-first fish",[&]{
  Domain d(content);funding(d,{100000000,10000},40);
  for(const auto& e:content.tankEntitlements){if(e.id=="TK-01-10")continue;CHECK(d.execute({.action=d.tank(e.tank)?Action::ExpandTank:Action::UnlockTank,.tank=e.tank}));}
  for(int tank=1;tank<=5;++tank){
   CHECK(d.execute({.action=Action::SwitchTank,.tank={tank}}));
   while(d.living({tank})<40)buy(d);
   const auto before=encode(d.state());CHECK(d.execute({.action=Action::Buy,.key="neonTetra",.point={400,300}}).error==Error::Full);
   CHECK(encode(d.state())==before);
  }
  CHECK(d.state().fish.size()==200);roundTrip(d);
  auto malformed=encode(d.state());malformed["tanks"][0]["slots"]=45;rejects([&]{decodeAndValidate(malformed,content);});
  malformed=encode(d.state());malformed["tanks"][0]["slots"]=37;rejects([&]{decodeAndValidate(malformed,content);});
 });
 test("account XP clamps at 40; reward receipt reports the actual XP",[&]{
  Domain d(content);funding(d,{100000,100},40);const auto id=buy(d);age(d,id,4);const auto old=d.state().lifetimeXp;const auto r=d.execute({.action=Action::Sell,.fish=id});CHECK(r.xp==0&&d.state().xp==content.levels.back()&&d.state().lifetimeXp>old);roundTrip(d);
 });
 test("first-purchase decor XP and no sickness, death or paid revival",[&]{
  Domain d(content);CHECK(d.execute({.action=Action::BuyDecor,.key="CP-01",.point={400,500}}));CHECK(d.state().xp==3&&d.state().wallet.coins==225);const auto before=encode(d.state());CHECK(!d.execute({.action=Action::ReviveAll}));CHECK(encode(d.state())==before);d.advanceCare(90*86400000LL);for(const auto& f:d.state().fish)CHECK(!f.dead&&careOf(*content.find(f.species),f,d.state().simNow)==Care::Hungry);roundTrip(d);
 });
 test("offline catch-up is applied once and stops at the meal boundary",[&]{
  Session session(content,std::filesystem::temp_directory_path()/"aquarium-v4-ephemeral.json",100000000,true);const auto id=session.domain().state().fish.front().id;CHECK(session.command({.action=Action::Feed,.fish=id}));session.suspend(100000000);session.resume(100000000+86400000);const auto growth=session.domain().fish(id)->growthMs;CHECK(growth==606000);session.resume(100000000+86400000);CHECK(session.domain().fish(id)->growthMs==growth);session.suspend(100000000+86400000);session.resume(1);CHECK(session.domain().fish(id)->growthMs==growth);roundTrip(session.domain());
 });
 test("strict new save validation rejects legacy, duplicate grants and altered balances",[&]{
  Domain d(content);auto j=encode(d.state());j["version"]=1;rejects([&]{decodeAndValidate(j,content);});j=encode(d.state());j["coins"]=999;rejects([&]{decodeAndValidate(j,content);});j=encode(d.state());j["fish"][0]["purchase"]["durationMs"]=0;rejects([&]{decodeAndValidate(j,content);});j=encode(d.state());j["fish"][0]["purchase"]["stages"]={0,5500,2500,8000,10000};rejects([&]{decodeAndValidate(j,content);});
  const auto id=buy(d);age(d,id,4);CHECK(d.execute({.action=Action::Sell,.fish=id}));j=encode(d.state());
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
