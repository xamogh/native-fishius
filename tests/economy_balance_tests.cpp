#include "aquarium/storage.hpp"
#include "fixtures.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace aq;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void roundTrip(Domain& d){const auto saved=encode(d.state());d.install(decodeAndValidate(saved,d.content()));check(encode(d.state())==saved,"Save changed on reload");}
FishId buy(Domain& d,std::string species="neonTetra"){
 const auto r=d.execute({.action=Action::Buy,.key=species,.point={400,300}});check(bool(r),"Purchase failed");return r.fish;
}
void feedAll(Domain& d){
 std::vector<FishId> hungry;
 for(const auto& f:d.state().fish)if(!f.egg&&careOf(*d.content().find(f.species),f,d.state().simNow)==Care::Hungry)hungry.push_back(f.id);
 for(const auto id:hungry)check(bool(d.execute({.action=Action::Feed,.fish=id})),"Feeding failed");
}
void growTetras(Domain& d){
 d.advanceCare(d.content().hatchMs);feedAll(d);d.advanceCare(600000);feedAll(d);d.advanceCare(600000-d.content().hatchMs);
}
void sellAdults(Domain& d){
 std::vector<FishId> adults;for(const auto& f:d.state().fish)if(f.age==4&&!f.favorite)adults.push_back(f.id);
 for(const auto id:adults)check(bool(d.execute({.action=Action::Sell,.fish=id})),"Adult sale failed");
}
Domain capped(const Content& c){
 Domain d(c);auto s=d.state();s.fish.clear();s.wallet={1000000,0};s.xp=c.levels.back();s.highestRewardedLevel=40;testing::openingBalances(s);d.install(s);return d;
}
FishId adult(Domain& d,std::string species="neonTetra",int stage=4,bool scripted=false){
 const auto id=buy(d,species);auto s=d.state();testing::stage(s.fish.back(),stage);s.fish.back().scripted=scripted;d.install(s);return id;
}
void rejects(const Json& saved,const Content& c){bool failed=false;try{decodeAndValidate(saved,c);}catch(const std::exception&){failed=true;}check(failed,"Invalid pearl settlement accepted");}
}
int main(int argc,char** argv){try{
 if(argc!=2)return 2;std::ifstream file(argv[1]);const auto json=Json::parse(file);const auto content=Content::fromJson(json);
 {
  const auto* tetra=content.find("neonTetra"),*guppy=content.find("guppy");
  const auto first=purchaseQuote(content,*tetra,1),mid=purchaseQuote(content,*guppy,10),late=purchaseQuote(content,*guppy,39);
  check(first.principal==5&&first.profit==6&&first.xp==10,"Starter quote is outside active-play tuning");
  Fish junior;junior.purchase=first;junior.age=1;check(fishReward(junior).xp==1,"First eligible starter sale gives no XP");
  check(mid.xp==147&&40*mid.xp>=content.levels[10]-content.levels[9],"Forty adult Guppies cannot cover level 10");
  check(late.xp==418&&4*40*late.xp>=content.levels[39]-content.levels[38],"Late-level XP exceeds four Guppy batches");
  check(content.find("koi")->price==3&&content.find("platinumArowana")->price==14,"Pearl fish affordability changed");
  Amount total=0;for(const auto& t:content.tankEntitlements)if(t.tank==TankId{1})total+=t.cost.coins;
  check(total==16800&&content.tankCosts[0][2]==900,"Tank 1 still has the old price cliff");
 }
 {
  // Start from the real wallet, capacity and hungry starters. No injected XP,
  // money or growth stages. Keep both longer-lived starters throughout.
  Domain d(content);while(d.living({1})<10)buy(d);growTetras(d);sellAdults(d);
  check(d.level()==2&&d.state().simNow==1200000,"The first active batch does not reach level 2 in twenty minutes");
  check(bool(d.execute({.action=Action::ExpandTank,.tank={1}})),"First level cannot fund a capacity upgrade");
  while(d.living({1})<15)buy(d);growTetras(d);sellAdults(d);
  check(d.level()>=3&&d.state().simNow==2400000,"The second active batch does not reach level 3");
  check(d.adultCoinSales()==21&&d.state().wallet.pearls==2,"First renewable pearl was not added alongside the level pearl");
  check(d.state().fish.size()==2&&d.state().wallet.coins>=5,"Starter scenario lost retained fish or ran out of funds");roundTrip(d);
 }
 {
  auto d=capped(content);
  for(int i=0;i<19;++i){check(bool(d.execute({.action=Action::Sell,.fish=adult(d)})),"Progress sale failed");}
  check(d.adultCoinSales()==19&&d.state().wallet.pearls==0,"Pearl paid before twenty adult sales");roundTrip(d);
  // The twentieth sale and its pearl must commit or roll back together.
  const auto id=adult(d);const auto before=encode(d.state());const Command sale{.action=Action::Sell,.fish=id,.requestId="pearl-milestone"};
  check(d.execute(sale,[](const State&){return false;}).error==Error::SaveFailure&&encode(d.state())==before,"Failed save advanced pearl progress");
  auto overflow=d.state();overflow.wallet.pearls=9'000'000'000'000'000LL;d.install(overflow);
  const auto full=encode(d.state());check(d.execute(sale).error==Error::Overflow&&encode(d.state())==full,"Pearl overflow partially sold a fish");
  d.install(decodeAndValidate(before,content));d.takeEvents();const auto paid=d.execute(sale);
  check(paid&&paid.pearls==1&&paid.xp==0&&d.adultCoinSales()==20&&d.state().wallet.pearls==1,"Milestone failed at the XP cap");
  bool visible=false;for(const auto& event:d.takeEvents())if(event.kind=="sale"&&event.pearls==1)visible=true;check(visible,"Pearl reward has no visible event");
  roundTrip(d);const auto saved=encode(d.state());
  check(d.execute(sale).replayed&&d.execute({.action=Action::Sell,.fish=id,.requestId="second-request"}).replayed&&d.adultCoinSales()==20&&d.state().wallet.pearls==1,"Replay minted another pearl");
  auto changed=content;changed.pearlSalesTarget=10;check(encode(decodeAndValidate(saved,changed))==saved,"New tuning repriced a past pearl settlement");
  auto invalid=saved;invalid["settlements"][std::to_string(id.value)]["pearl_reward"]["target"]=0;rejects(invalid,content);
  invalid=saved;invalid["settlements"][std::to_string(id.value)]["pearl_reward"]["sale"]=19;rejects(invalid,content);
  invalid=saved;invalid["settlements"][std::to_string(id.value)].erase("pearl_reward");rejects(invalid,content);
  for(int i=0;i<20;++i)check(bool(d.execute({.action=Action::Sell,.fish=adult(d)})),"Second milestone failed");
  check(d.state().wallet.pearls==2&&d.adultCoinSales()==40,"Pearl earning stopped after the first reward");roundTrip(d);
  const auto prior=d.adultCoinSales();
  check(bool(d.execute({.action=Action::Sell,.fish=adult(d,"neonTetra",1)})),"Junior sale failed");
  check(bool(d.execute({.action=Action::Sell,.fish=adult(d,"neonTetra",4,true)})),"Scripted sale failed");
  check(bool(d.execute({.action=Action::Sell,.fish=adult(d,"bubbleEyeGoldfish")})),"Pearl fish sale failed");
  const auto protectedId=adult(d);check(bool(d.execute({.action=Action::Favorite,.fish=protectedId})),"Favorite failed");
  check(d.execute({.action=Action::Sell,.fish=protectedId}).error==Error::Protected,"Favorite sold");
  auto empty=d.state();empty.fish.back().favorite=false;empty.fish.back().purchase.profit=0;empty.fish.back().purchase.xp=0;d.install(empty);
  check(bool(d.execute({.action=Action::Sell,.fish=protectedId})),"Zero reward historical fish sale failed");
  check(d.adultCoinSales()==prior&&d.state().wallet.pearls==1,"Ineligible fish advanced pearl progress");roundTrip(d);
 }
 {
  Domain d(content);auto old=d.state();auto& fish=old.fish.front();
  fish.purchase.xp=2;fish.purchase.configVersion="v4-before-active-play";testing::stage(fish,4);
  const auto snapshot=fish.purchase;const auto id=fish.id;
  d.install(decodeAndValidate(encode(old),content));check(d.fish(id)->purchase==snapshot,"Active tuning changed an already owned fish");
  const auto sold=d.execute({.action=Action::Sell,.fish=id});check(sold&&sold.xp==2&&d.adultCoinSales()==1,"Historical fish lost its quote or future pearl eligibility");roundTrip(d);
 }
 {
  auto d=capped(content);const auto id=adult(d);check(bool(d.execute({.action=Action::Sell,.fish=id})),"Legacy fixture failed");
  auto saved=encode(d.state());saved["settlements"][std::to_string(id.value)].erase("pearl_reward");saved["totalEvents"].erase("adult-coin-sale");
  d.install(decodeAndValidate(saved,content));check(d.adultCoinSales()==0&&d.state().wallet.pearls==0,"Historical sales granted retroactive pearls");
  check(d.execute({.action=Action::Sell,.fish=id}).replayed&&d.adultCoinSales()==0,"Historical replay counted as a new sale");
 }
 std::cout<<"PASS active-play affordability, real first forty minutes, renewable pearls, cap, reload, retries, exclusions and rollback\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
