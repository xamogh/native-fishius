#include "fixtures.hpp"
#include "aquarium/storage.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
using namespace aq;
namespace {
void check(bool value,const char* expression){if(!value)throw std::runtime_error(expression);}
#define CHECK(x) check(bool(x),#x)
Domain ready(Content c){for(auto& e:c.decorEvents){e.configured=true;e.startsAt=1;e.endsAt=2'000'000'000'000;}Domain d(std::move(c),1000);auto s=d.state();s.decorOnboardingComplete=true;s.wallet={100000000,100000};s.xp=d.content().levels.back();s.highestRewardedLevel=40;testing::openingBalances(s);d.install(s);return d;}
Command buy(std::string id,WorldPoint p={250,500}){Command c;c.action=Action::BuyDecor;c.key=std::move(id);c.point=p;return c;}
Command edit(Action action,std::uint64_t id,WorldPoint p={250,500},TankId tank={1}){Command c;c.action=action;c.decor=id;c.point=p;c.tank=tank;return c;}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}CHECK(threw);}
}
int main(int argc,char** argv){
 if(argc<2)return 2;std::ifstream file(argv[1]);const auto content=Content::fromJson(Json::parse(file));
 std::vector<std::pair<std::string,std::function<void()>>> tests;
 auto test=[&](std::string name,auto f){tests.emplace_back(name,f);};
 test("complete catalog with ordinary rarity and separate edition",[&]{CHECK(content.decorations.size()==120);int plants=0,animated=0;std::set<std::string> ids;for(const auto& d:content.decorations){plants+=d.category=="Plant";animated+=d.art.at("motion_class")!="Static";CHECK(ids.insert(d.id).second);CHECK(d.buyXp==0);CHECK(d.rarity!="premium"&&d.rarity!="limited");}CHECK(plants==60);CHECK(animated==44);CHECK(content.decorEvents.size()==12);});
 test("fresh and saved games browse and buy decor without a first plant",[&]{
  Domain d(content);const auto before=encode(d.state());
  CHECK(d.state().decorOnboardingComplete);
  for(const auto& item:content.decorations)CHECK(d.decorVisible(item)==(item.level<=40));
  CHECK(d.blocker(*content.findDecor("CP-02")).error==Error::Level);
  CHECK(d.blocker(*content.findDecor("CL-12")).error==Error::EventClosed);
  CHECK(d.decorPurchaseXp(*content.findDecor("CD-01"))==0);
  CHECK(d.execute(buy("CD-01")));
  CHECK(d.state().wallet.coins==250-content.findDecor("CD-01")->price&&d.state().xp==0);
  CHECK(d.state().decorOnboardingComplete);
  CHECK(encode(d.state())["fish"]==before["fish"]);
  const auto saved=encode(d.state());d.install(decodeAndValidate(saved,content));
  CHECK(encode(d.state())==saved);
  for(const auto& item:content.decorations)CHECK(d.decorVisible(item)==(item.level<=40));
  CHECK(d.execute(buy("CD-01",{400,500})));
  CHECK(d.state().wallet.coins==250-2*content.findDecor("CD-01")->price&&d.state().xp==0);
  CHECK(d.state().decorOnboardingComplete);
 });
 test("first plant costs 25 and grants no XP",[&]{Domain d(content);const auto before=d.state();CHECK(d.decorPurchaseXp(*content.findDecor("CP-01"))==0);CHECK(d.execute(buy("CP-01")));CHECK(d.state().wallet.coins==225);CHECK(d.state().xp==0);CHECK(d.state().decorOnboardingComplete);CHECK(d.decorScore({1})==10);CHECK(encode(d.state())["fish"]==encode(before)["fish"]);CHECK(d.state().tanks[0].slots==10);CHECK(d.decorPurchaseXp(*content.findDecor("CP-01"))==0);CHECK(d.execute(buy("CP-01",{400,500})));CHECK(d.state().xp==0);CHECK(d.decorScore({1})==10);});
 test("every launch item charges its exact currency and first-ownership XP",[&]{int checked=0;for(const auto& def:content.decorations){auto d=ready(content);const auto before=d.state();const auto r=d.execute(buy(def.id,{544,550}));if(def.level>40){CHECK(r.error==Error::Level);CHECK(encode(before)==encode(d.state()));continue;}++checked;CHECK(r);CHECK(d.state().wallet.coins==before.wallet.coins-(def.currency==Currency::Coins?def.price:0));CHECK(d.state().wallet.pearls==before.wallet.pearls-(def.currency==Currency::Pearls?def.price:0));CHECK(d.state().xp==before.xp+def.buyXp);CHECK(d.decorScore({1})==def.score);CHECK(d.execute(buy(def.id,{544,550})));CHECK(d.state().xp==before.xp+def.buyXp);CHECK(d.decorScore({1})==def.score);}CHECK(checked>80);});
 test("stored copies transfer for free and never double-count score",[&]{auto d=ready(content);auto s=d.state();s.tanks.push_back({{2},10});d.install(s);CHECK(d.execute(buy("CP-01")));CHECK(d.execute(buy("CP-01")));const auto wallet=d.state().wallet;const auto xp=d.state().xp;CHECK(d.execute(edit(Action::StoreDecor,1)));CHECK(d.decorScore({1})==10);CHECK(d.execute(edit(Action::StoreDecor,2)));CHECK(d.decorScore({1})==0);CHECK(d.execute(edit(Action::RestoreDecor,1,{250,500},{2})));CHECK(d.decorScore({2})==10);CHECK(d.decoration(1)->tank==TankId{2});CHECK(d.execute({Action::SwitchTank,{},TankId{2}}));CHECK(d.execute(edit(Action::MoveDecor,1,{350,500},{1})));CHECK(d.decorScore({1})==10&&d.decorScore({2})==0);CHECK(d.state().wallet.coins==wallet.coins&&d.state().wallet.pearls==wallet.pearls&&d.state().xp==xp);});
 test("rearranging decor is free while daily quest rewards are deferred",[&]{auto d=ready(content);CHECK(d.execute(buy("CP-01")));const auto before=d.state();CHECK(d.execute(edit(Action::MoveDecor,1,{350,500})));CHECK(d.state().xp==before.xp&&d.state().wallet.coins==before.wallet.coins);CHECK(!d.execute({Action::ClaimQuest,{}, {},"daily-decor"}));});
 test("cosmetics remain usable outside their acquisition window",[&]{auto d=ready(content);CHECK(d.execute(buy("PL-11")));const auto xp=d.state().xp;d.setCalendar(2'000'000'000'001);CHECK(d.execute(buy("PL-11")).error==Error::EventClosed);CHECK(d.execute(edit(Action::StoreDecor,1)));CHECK(d.execute(edit(Action::RestoreDecor,1)));CHECK(d.execute(edit(Action::FlipDecor,1)));CHECK(d.state().xp==xp);CHECK(d.decorScore({1})==content.findDecor("PL-11")->score);});
 test("proposal dates are not silently made live",[&]{Domain d(content,1'800'000'000'000);auto s=d.state();s.decorOnboardingComplete=true;s.xp=content.levels.back();s.highestRewardedLevel=40;s.wallet={100000000,100000};d.install(s);for(const auto& item:content.decorations)if(item.edition=="Limited Edition")CHECK(d.execute(buy(item.id)).error==Error::EventClosed);});
 test("event configuration validates atomically and uses exclusive end timestamps",[&]{auto c=content;c.configureDecorEvents(Json::array({{{"name","Winterfest"},{"starts_at",1000},{"ends_at",2000}}}));Domain d(c,1000);auto s=d.state();s.decorOnboardingComplete=true;s.xp=c.levels.back();s.highestRewardedLevel=40;s.wallet={1000000,1000};d.install(s);CHECK(d.execute(buy("PL-11")));d.setCalendar(1999);CHECK(d.execute(buy("PL-11")));d.setCalendar(2000);CHECK(d.execute(buy("PL-11")).error==Error::EventClosed);const auto before=c.decorEvents;rejects([&]{c.configureDecorEvents(Json::array({{{"name","Winterfest"},{"starts_at",3000},{"ends_at",4000}},{{"name","unknown"},{"starts_at",3000},{"ends_at",4000}}}));});const auto it=std::find_if(c.decorEvents.begin(),c.decorEvents.end(),[](const auto& e){return e.name=="Winterfest";});CHECK(it->startsAt==1000&&it->endsAt==2000);});
 test("invalid positions, missing art, insufficient funds and overflow are atomic",[&]{auto d=ready(content);for(const auto p:{WorldPoint{-1,500},WorldPoint{1089,500},WorldPoint{250,-1},WorldPoint{250,636},WorldPoint{std::nan(""),300}}){auto before=encode(d.state());CHECK(d.execute(buy("CP-01",p)).error==Error::InvalidPosition);CHECK(encode(d.state())==before);}auto s=d.state();s.wallet.coins=0;d.install(s);auto before=encode(d.state());CHECK(d.execute(buy("CP-01")).error==Error::Funds);CHECK(encode(d.state())==before);auto c=content;c.decorations[0].artReady=false;auto noart=ready(c);CHECK(noart.execute(buy("CP-01")).error==Error::NoArt);auto over=ready(content);s=over.state();s.nextDecorId=std::numeric_limits<std::uint64_t>::max();over.install(s);before=encode(over.state());CHECK(over.execute(buy("CP-01")).error==Error::Overflow);CHECK(encode(over.state())==before);});
 test("32 placed copies is independent of fish slots and stored ownership",[&]{auto d=ready(content);for(int i=0;i<32;++i)CHECK(d.execute(buy("CP-01")));CHECK(d.placedDecor({1})==32);const auto before=encode(d.state());CHECK(d.execute(buy("CP-01")).error==Error::Maximum);CHECK(encode(d.state())==before);CHECK(d.execute(edit(Action::StoreDecor,1)));CHECK(d.execute(buy("CP-01")));CHECK(d.state().decor.size()==33);CHECK(d.execute(edit(Action::RestoreDecor,1)).error==Error::Maximum);CHECK(d.state().tanks[0].slots==10);});
 test("ownership, placement and flipping survive save/load",[&]{auto d=ready(content);CHECK(d.execute(buy("CP-01")));CHECK(d.execute(edit(Action::FlipDecor,1)));CHECK(d.execute(edit(Action::StoreDecor,1)));auto bytes=encode(d.state());CHECK(encode(decodeAndValidate(bytes,content))==bytes);d.install(decodeAndValidate(bytes,content));const auto xp=d.state().xp;CHECK(d.execute(buy("CP-01")));CHECK(d.state().xp==xp);bytes["decorOwned"]=Json::array();rejects([&]{decodeAndValidate(bytes,content);});});
 test("decor footprints remain visible at tank edges with perspective",[&]{
  const auto top=decorPlacementPoint({0,0}),bottom=decorPlacementPoint({1088,635});
  CHECK(top.x==0&&top.y==0);CHECK(bottom.x==1088&&bottom.y==635);
  CHECK(std::abs(decorScale({544,153.6})-.42)<1e-9);CHECK(std::abs(decorScale({544,504.32})-1.18)<1e-9);
  CHECK(std::abs(decorScale({0,504.32})-.8456)<1e-9);CHECK(std::abs(decorHaze({544,153.6})-.26)<1e-9);CHECK(decorHaze({544,504.32})<1e-9);
  CHECK(std::abs(decorScale({544,0})-.42)<1e-9);CHECK(std::abs(decorScale({544,635})-1.18)<1e-9);
  auto d=ready(content);const auto& def=*content.findDecor("CP-01");
  auto visible=[&](const Decoration& item){
   const double scale=decorScale(item.position)*item.sizeMul;
   const double halfWidth=def.width*waterWidth/12.*scale*.5,height=def.height*tankHeight/7.*scale;
   CHECK(item.position.x-halfWidth>=-1e-7&&item.position.x+halfWidth<=waterWidth+1e-7);
   CHECK(item.position.y-height>=-1e-7&&item.position.y<=tankHeight);
  };
  CHECK(d.execute(buy("CP-01",{0,0})));visible(*d.decoration(1));CHECK(d.decoration(1)->position.x>0&&d.decoration(1)->position.y>0);
  CHECK(d.execute(edit(Action::MoveDecor,1,{1088,635})));visible(*d.decoration(1));CHECK(d.decoration(1)->position.x<1088&&d.decoration(1)->position.y==635);
  auto resize=edit(Action::ResizeDecor,1);resize.value=decorSizeMax;CHECK(d.execute(resize));visible(*d.decoration(1));
  for(const auto point:{WorldPoint{0,0},WorldPoint{544,0},WorldPoint{1088,0},WorldPoint{0,635}}){CHECK(d.execute(edit(Action::MoveDecor,1,point)));visible(*d.decoration(1));}
  CHECK(d.execute(edit(Action::StoreDecor,1)));CHECK(d.execute(edit(Action::RestoreDecor,1,{544,600})));CHECK(d.decoration(1)->position.y==600);
  const auto bytes=encode(d.state());CHECK(encode(decodeAndValidate(bytes,content))==bytes);
 });
 test("all catalog dimensions keep an accessible footprint at every custom size",[&]{
  for(const auto& def:content.decorations)for(const auto size:{decorSizeMin,1.,decorSizeMax})
   for(const auto point:{WorldPoint{0,0},WorldPoint{544,0},WorldPoint{1088,0},WorldPoint{0,635},WorldPoint{544,635},WorldPoint{1088,635}}){
    const auto p=decorPlacementPoint(def,point,size);CHECK(inTank(p));
    const double scale=decorScale(p)*size,half=def.width*waterWidth/12.*scale*.5,height=def.height*tankHeight/7.*scale;
    CHECK(p.x-std::min(half,waterWidth*.5)>=-1e-7&&p.x+std::min(half,waterWidth*.5)<=waterWidth+1e-7);
    CHECK(p.y-std::min(height,tankHeight)>=-1e-7);
    // Art larger than the tank remains anchored at the bottom so the player
    // can still select, shrink or store it.
    if(height>tankHeight)CHECK(p.y==tankHeight);
   }
 });
 test("paid decor places once with no second charge, gate or XP award",[&]{
  Domain d(content,1000);CHECK(d.execute({Action::PurchaseDecor,{}, {},"CP-01"}));
  CHECK(d.state().decor.empty()&&d.state().pendingDecor=="CP-01");CHECK(d.state().wallet.coins==225&&d.state().xp==0);
  CHECK(!d.state().totalEvents.contains("decor"));CHECK(d.questDefinitions().empty());const auto paid=encode(d.state());CHECK(encode(decodeAndValidate(paid,content))==paid);
  d.install(decodeAndValidate(paid,content));CHECK(d.execute({Action::PlaceDecor,{}, {},"",{0,0}}));CHECK(d.state().pendingDecor.empty());
  CHECK(d.state().decor.size()==1&&d.state().wallet.coins==225&&d.state().xp==0&&d.state().totalEvents.at("decor")==1);
  const auto placed=encode(d.state());CHECK(d.execute({Action::PlaceDecor,{}, {},"",{400,400}}).error==Error::NotReady);CHECK(encode(d.state())==placed);
  CHECK(d.execute({Action::PurchaseDecor,{}, {},"CP-01"}));CHECK(d.execute({Action::CancelDecor}));CHECK(d.state().pendingDecor.empty()&&d.state().wallet.coins==200&&d.state().decor.size()==2&&d.decoration(2)->stored);
  CHECK(d.state().xp==0&&d.state().totalEvents.at("decor")==1);
 });
 test("cancelled Pearl purchases preserve paid copies and first ownership through reload",[&]{
  auto d=ready(content);const auto before=d.state();const auto* def=content.findDecor("PP-01");CHECK(def);
  CHECK(d.execute({Action::PurchaseDecor,{}, {},def->id}));
  const auto queued=encode(d.state());CHECK(d.execute({Action::PurchaseDecor,{}, {},"CP-01"}).error==Error::NotReady);CHECK(encode(d.state())==queued);
  CHECK(d.execute(buy("CP-01")).error==Error::NotReady);CHECK(encode(d.state())==queued);
  d.install(decodeAndValidate(queued,content));CHECK(d.execute({Action::CancelDecor}));
  CHECK(d.state().decor.size()==1&&d.decoration(1)->stored&&d.decoration(1)->kind==def->id);
  CHECK(d.state().pendingDecor.empty()&&d.decorScore({1})==0&&!d.state().totalEvents.contains("decor"));
  CHECK(d.state().wallet.pearls==before.wallet.pearls-def->price&&d.state().xp==before.xp+def->buyXp);
  const auto stored=encode(d.state());d.install(decodeAndValidate(stored,content));CHECK(encode(d.state())==stored);
  CHECK(d.execute({Action::CancelDecor}));CHECK(d.state().decor.size()==1&&d.state().wallet.pearls==before.wallet.pearls-def->price);
  CHECK(d.execute({Action::PurchaseDecor,{}, {},def->id}));CHECK(d.execute({Action::CancelDecor}));
  CHECK(d.state().decor.size()==2&&d.state().xp==before.xp+def->buyXp&&d.state().wallet.pearls==before.wallet.pearls-2*def->price);
  const auto wallet=d.state().wallet;CHECK(d.execute(edit(Action::RestoreDecor,1,{544,0})));
  CHECK(!d.decoration(1)->stored&&d.decoration(2)->stored&&d.decorScore({1})==def->score&&d.state().totalEvents.at("decor")==1);
  CHECK(d.state().wallet.coins==wallet.coins&&d.state().wallet.pearls==wallet.pearls&&d.state().xp==before.xp+def->buyXp);
 });
 test("pending copies reserve inventory and can be cancelled when the tank fills",[&]{
  auto d=ready(content);for(int i=0;i<32;++i)CHECK(d.execute(buy("CP-01")));
  CHECK(d.execute(edit(Action::StoreDecor,1)));CHECK(d.execute({Action::PurchaseDecor,{}, {},"CP-01"}));
  CHECK(d.execute(edit(Action::RestoreDecor,1)));CHECK(d.placedDecor({1})==32);
  const auto pending=encode(d.state());CHECK(d.execute({Action::PlaceDecor,{}, {},"",{544,500}}).error==Error::Maximum);CHECK(encode(d.state())==pending);
  CHECK(d.execute({Action::CancelDecor}));CHECK(d.state().decor.size()==33&&d.decoration(33)->stored&&d.placedDecor({1})==32);
  auto s=d.state();for(auto& item:s.decor)item.stored=true;
  while(s.decor.size()<499){auto copy=s.decor.front();copy.id=s.nextDecorId++;s.decor.push_back(copy);}d.install(s);
  CHECK(d.execute({Action::PurchaseDecor,{}, {},"CP-01"}));CHECK(d.execute({Action::CancelDecor}));CHECK(d.state().decor.size()==500);
  const auto full=encode(d.state());CHECK(d.execute({Action::PurchaseDecor,{}, {},"CP-01"}).error==Error::Maximum);CHECK(encode(d.state())==full);
  s=d.state();s.decor.resize(1);s.nextDecorId=std::numeric_limits<std::uint64_t>::max()-1;d.install(s);
  CHECK(d.execute({Action::PurchaseDecor,{}, {},"CP-01"}));CHECK(d.execute({Action::CancelDecor}));CHECK(d.state().nextDecorId==std::numeric_limits<std::uint64_t>::max());
  const auto last=encode(d.state());CHECK(d.execute({Action::PurchaseDecor,{}, {},"CP-01"}).error==Error::Overflow);CHECK(encode(d.state())==last);
 });
 test("size controls clamp and persist without changing purchase rewards",[&]{
  auto d=ready(content);CHECK(d.execute(buy("CD-01")));const auto before=d.state();auto c=edit(Action::ResizeDecor,1);c.value=decorSizeStep;
  CHECK(d.execute(c));CHECK(std::abs(d.decoration(1)->sizeMul-1.12)<1e-9);
  for(int i=0;i<20;++i)CHECK(d.execute(c));CHECK(d.decoration(1)->sizeMul==1.7);
  c.value=-decorSizeStep;for(int i=0;i<20;++i)CHECK(d.execute(c));CHECK(d.decoration(1)->sizeMul==.6);
  CHECK(d.state().wallet.coins==before.wallet.coins&&d.state().xp==before.xp);auto bytes=encode(d.state());CHECK(encode(decodeAndValidate(bytes,content))==bytes);
  bytes["decor"][0]["sizeMul"]=2;rejects([&]{decodeAndValidate(bytes,content);});bytes["decor"][0].erase("sizeMul");rejects([&]{decodeAndValidate(bytes,content);});
 });
 int failures=0;for(const auto& [name,f]:tests){try{f();std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failures;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}}
 std::cout<<tests.size()-failures<<" / "<<tests.size()<<" decor tests passed\n";return failures?1:0;
}
