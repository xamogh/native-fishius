#include "fixtures.hpp"
#include "aquarium/storage.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace aq;
namespace {
void expect(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
#define CHECK(x) expect(bool(x),#x)
Millis date(int year,int month,int day){using namespace std::chrono;return duration_cast<milliseconds>(sys_days{std::chrono::year{year}/std::chrono::month{unsigned(month)}/std::chrono::day{unsigned(day)}}.time_since_epoch()).count();}
Domain rich(const Content& c){Domain d(c,date(2026,1,1),17);State s=d.state();s.wallet={1'000'000'000,1'000'000};s.xp=c.levels[39];s.highestRewardedLevel=40;s.fish.clear();s.tanks={{{1},20}};testing::openingBalances(s);d.install(std::move(s));return d;}
template<class F>void rejects(F fn){bool threw=false;try{fn();}catch(const std::exception&){threw=true;}CHECK(threw);}
}
int main(int argc,char** argv){
 try{
  if(argc<2)throw std::runtime_error("Pass assets/content.json");std::ifstream in(argv[1]);Content content=Content::fromJson(Json::parse(in));
  using Test=std::pair<std::string,std::function<void()>>;std::vector<Test> tests;
  auto test=[&](std::string name,std::function<void()> f){tests.emplace_back(std::move(name),std::move(f));};
  test("large and small lifecycle advances agree",[&]{Domain a(content),b(content);for(auto& f:a.state().fish)a.execute({Action::Feed,f.id});for(auto& f:b.state().fish)b.execute({Action::Feed,f.id});constexpr Millis total=7*86400000LL;a.advanceCare(total);for(Millis i=0;i<total;i+=60000)b.advanceCare(std::min<Millis>(60000,total-i));CHECK(encode(a.state())==encode(b.state()));});
  test("purchase rejection is atomic",[&]{Domain d(content);auto before=encode(d.state());CHECK(!d.execute({Action::Buy,{}, {},"unknown",{300,300}}));CHECK(encode(d.state())==before);CHECK(!d.execute({Action::Buy,{}, {},"guppy",{-1,300}}));CHECK(encode(d.state())==before);State s=d.state();s.wallet.coins=0;d.install(s);before=encode(d.state());CHECK(!d.execute({Action::Buy,{}, {},"guppy",{300,300}}));CHECK(encode(d.state())==before);});
  test("art gate precedes event and funds",[&]{auto c=content;c.species[0].artReady=false;Domain d(c);auto r=d.execute({Action::Buy,{}, {},c.species[0].id,{300,300}});CHECK(r.error==Error::NoArt);});
  test("tank sequencing and level gates apply to either currency",[&]{
   for(auto currency:{Currency::Coins,Currency::Pearls}){Domain d(content);auto before=encode(d.state());CHECK(!d.execute({.action=Action::UnlockTank,.tank={3},.currency=currency}));CHECK(d.execute({.action=Action::UnlockTank,.tank={2},.currency=currency}).error==Error::Level);CHECK(encode(d.state())==before);}
  });
  test("tank purchases reject unsupported currencies without charging",[&]{
   auto d=rich(content);auto state=d.state();state.tanks[0].slots=10;d.install(state);const auto before=encode(d.state());
   CHECK(d.execute({.action=Action::ExpandTank,.tank={1},.currency=Currency::Gift}).error==Error::Unavailable);
   CHECK(d.execute({.action=Action::UnlockTank,.tank={2},.currency=Currency::Gift}).error==Error::Unavailable);CHECK(encode(d.state())==before);
  });
  test("one pellet feeds exactly one fish after its notice delay",[&]{Domain d(content);State s=d.state();s.fish.resize(2);for(auto& f:s.fish){f.position={400,300};f.motion.previous=f.position;f.motion.noticeDelay=.02;f.motion.direction=1;f.motion.cruise=30;}d.install(s);CHECK(d.execute({Action::DropFood,{}, {},"",{400,300}}));for(int i=0;i<100&&!d.pellets().empty();++i)d.stepMovement(.02,Tool::Food);CHECK(d.pellets().empty());int fed=0;for(const auto& f:d.state().fish)fed+=careOf(*content.find(f.species),f,d.state().simNow)==Care::Fed;CHECK(fed==1);CHECK(d.state().totalEvents.at("feed")==1);});
  test("full tank accepts fish actions and preserves lower positions in saves",[&]{
   auto d=rich(content);
   for(const WorldPoint p:std::array{WorldPoint{0,0},WorldPoint{1088,635},WorldPoint{544,600}}){
    CHECK(inTank(p)&&inPlacementWater(p));
    const auto bought=d.execute({.action=Action::Buy,.key="guppy",.point=p});CHECK(bought);
    CHECK(d.fish(bought.fish)->position.y==p.y);
    CHECK(d.execute({.action=Action::Move,.fish=bought.fish,.point={544,634}}));
    auto grown=d.state();testing::stage(grown.fish.back(),4);d.install(grown);
    CHECK(d.execute({.action=Action::Keep,.fish=bought.fish}));
    CHECK(d.execute({.action=Action::Stash,.fish=bought.fish}));
    CHECK(d.execute({.action=Action::Restore,.fish=bought.fish,.point=p}));
    const auto saved=encode(d.state());CHECK(encode(decodeAndValidate(saved,content))==saved);
    CHECK(d.execute({.action=Action::DropFood,.point=p}));
   }
   const auto id=d.state().companions.back().id;
   for(const WorldPoint p:std::array{WorldPoint{-1,600},WorldPoint{1089,600},WorldPoint{544,636}}){
    const auto before=encode(d.state());const auto food=d.pellets().size();
    CHECK(!inTank(p)&&!inPlacementWater(p));
    CHECK(d.execute({.action=Action::Buy,.key="guppy",.point=p}).error==Error::InvalidPosition);
    CHECK(d.execute({.action=Action::Move,.fish=id,.point=p}).error==Error::InvalidPosition);
    CHECK(d.execute({.action=Action::DropFood,.point=p}).error==Error::InvalidPosition);
    CHECK(encode(d.state())==before&&d.pellets().size()==food);
    auto invalid=before;invalid["fish"][0]["position"]={p.x,p.y};rejects([&]{decodeAndValidate(invalid,content);});
   }
  });
  test("food and eggs sink through the old cutoff to the tank floor",[&]{
   auto d=rich(content);const auto egg=d.execute({.action=Action::Buy,.key="guppy",.point={544,400}});CHECK(egg);
   CHECK(d.execute({.action=Action::DropFood,.point={544,400}}));
   for(int i=0;i<300;++i)d.stepMovement(.02,Tool::Select);
   CHECK(d.fish(egg.fish)->position.y>620&&d.fish(egg.fish)->position.y<635);
   CHECK(d.pellets().size()==1&&d.pellets().front().position.y>625&&d.pellets().front().position.y<635);
  });
  test("fish swim and eat in the lower tank",[&]{
   Domain d(content);auto state=d.state();state.fish.resize(1);auto& fish=state.fish.front();const auto id=fish.id;
   fish.position={544,550};fish.motion.previous=fish.position;fish.motion.targetY=620;fish.motion.retarget=100;fish.lastFedAt=state.simNow;
   d.install(state);for(int i=0;i<300;++i)d.stepMovement(.02,Tool::Select);
   CHECK(d.fish(id)->position.y>590&&d.fish(id)->position.y<635);
   state=d.state();state.fish.front().position={544,600};state.fish.front().lastFedAt=state.simNow-content.find(state.fish.front().species)->feedMs;
   d.install(state);CHECK(d.execute({.action=Action::DropFood,.point={544,630}}));
   for(int i=0;i<300&&!d.pellets().empty();++i)d.stepMovement(.02,Tool::Food);
   CHECK(d.pellets().empty());CHECK(careOf(*content.find(d.fish(id)->species),*d.fish(id),d.state().simNow)==Care::Fed);
   CHECK(d.fish(id)->position.y>580);
  });
  test("food is bounded and cleared on tank switch",[&]{auto d=rich(content);for(int i=0;i<48;++i)CHECK(d.execute({Action::DropFood,{}, {},"",{400,300}}));CHECK(d.execute({Action::DropFood,{}, {},"",{400,300}}).error==Error::NoFoodRoom);CHECK(d.execute({Action::UnlockTank,{}, {2}}));CHECK(d.pellets().empty());});
  test("deferred gifts and egg rewards preserve state and enforce source unlocks",[&]{
   Domain d(content,date(2026,9,8));auto before=encode(d.state());CHECK(d.execute({Action::SendGift}).error==Error::Level);CHECK(d.execute({Action::DailyEgg}).error==Error::Level);CHECK(encode(d.state())==before);
   State s=d.state();s.xp=content.levels[4];s.highestRewardedLevel=5;d.install(s);before=encode(d.state());CHECK(d.execute({Action::SendGift}).error==Error::Unavailable);CHECK(d.execute({Action::DailyEgg}).error==Error::Level);CHECK(encode(d.state())==before);
   s=d.state();s.xp=content.levels[5];s.highestRewardedLevel=6;d.install(s);before=encode(d.state());CHECK(d.execute({Action::DailyEgg}).error==Error::Unavailable);CHECK(encode(d.state())==before);
  });
  test("save round-trip, unknown IDs, duplicate IDs and future schemas",[&]{Domain d(content);auto j=encode(d.state());CHECK(encode(decodeAndValidate(j,content))==j);auto corrupt=j;corrupt["fish"][1]["id"]=corrupt["fish"][0]["id"];rejects([&]{decodeAndValidate(corrupt,content);});corrupt=j;corrupt["version"]=999;rejects([&]{decodeAndValidate(corrupt,content);});corrupt=j;corrupt["fish"][0]["species"]="missing";rejects([&]{decodeAndValidate(corrupt,content);});corrupt=j;corrupt["coins"]=-1;rejects([&]{decodeAndValidate(corrupt,content);});});
  test("atomic storage retains corrupt primary and recovers backup",[&]{auto path=std::filesystem::temp_directory_path()/"fishx-storage-contract.json";for(auto suffix:{"",".bak",".recovered",".recovered.bak",".tmp"})std::filesystem::remove(path.string()+suffix);Domain d(content);Storage st(path);CHECK(st.save(d.state()).empty());CHECK(st.save(d.state()).empty());{std::ofstream out(path);out<<"broken";}Storage again(path);auto loaded=again.load(content);CHECK(loaded.state.has_value());CHECK(loaded.preservedOriginal);CHECK(again.save(*loaded.state).empty());std::ifstream in(path);std::string bytes;in>>bytes;CHECK(bytes=="broken");for(auto suffix:{"",".bak",".recovered",".recovered.bak",".tmp"})std::filesystem::remove(path.string()+suffix);});
  test("suspension interval is applied once",[&]{auto path=std::filesystem::temp_directory_path()/"fishx-ephemeral.json";Millis t=date(2026,9,8);Session s(content,path,t,true);s.update(1000,t+1000,Tool::Select);auto now=s.domain().state().simNow;s.suspend(t+1000);s.resume(t+61000);CHECK(s.domain().state().simNow==now+60000);s.resume(t+61000);CHECK(s.domain().state().simNow==now+60000);});
  Json results=Json::array();int failures=0;for(auto& [name,fn]:tests){try{fn();std::cout<<"PASS "<<name<<'\n';results.push_back({{"name",name},{"passed",true}});}catch(const std::exception& e){++failures;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';results.push_back({{"name",name},{"passed",false},{"error",e.what()}});}}
  std::cout<<tests.size()-std::size_t(failures)<<" / "<<tests.size()<<" tests passed\n";std::ofstream report("domain-test-results.json");report<<Json{{"tests",results},{"failures",failures}}.dump(2);return failures?1:0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
