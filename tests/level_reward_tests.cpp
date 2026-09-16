#include "aquarium/storage.hpp"
#include "fixtures.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace aq;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
FishId adultAt(Domain& d,Amount xp){auto state=d.state();state.xp=xp;state.highestRewardedLevel=levelFor(d.content(),xp);testing::stage(state.fish.front(),4);testing::openingBalances(state);d.install(state);return state.fish.front().id;}
}
int main(int argc,char** argv){try{
 if(argc!=2)return 2;std::ifstream in(argv[1]);const auto json=Json::parse(in);const auto content=Content::fromJson(json);
 check(levelReward(content,1).coins==0&&levelReward(content,41).pearls==0,"Unsupported levels give no reward");
 for(int reached=2;reached<=40;++reached){
  Domain domain(content);const auto id=adultAt(domain,content.levels[reached-1]-1);const auto before=domain.state();const auto expected=content.levelRewards[reached-1];
  const Command settle{.action=Action::Keep,.fish=id,.requestId="level-crossing"};check(bool(domain.execute(settle)),"Cannot cross level threshold");
  check(domain.level()==reached,"Wrong reached level");check(domain.state().wallet.coins==before.wallet.coins+6+expected.coins&&domain.state().wallet.pearls==before.wallet.pearls+expected.pearls,"Level grant differs from workbook");
  int levels=0;for(const auto& event:domain.takeEvents())if(event.kind=="level"){++levels;check(event.reachedLevel==reached&&event.coins==expected.coins&&event.pearls==expected.pearls,"Level event differs from credited reward");}check(levels==1,"Expected one level event");
  const auto paid=encode(domain.state());domain.install(decodeAndValidate(paid,content));check(domain.execute(settle).replayed,"Reload lost receipt");check(encode(domain.state())==paid,"Reload repeated level reward");
 }
 std::cout<<"PASS all 39 level grants match v4 and persist exactly once\n";
 {
  Domain domain(content);auto s=domain.state();testing::stage(s.fish.front(),4);s.fish.front().purchase.xp=content.levels.back();domain.install(s);
  check(bool(domain.execute({.action=Action::Sell,.fish=s.fish.front().id})),"Multi-level settlement failed");
  Money sum{0,0};for(const auto& reward:content.levelRewards){sum.coins+=reward.coins;sum.pearls+=reward.pearls;}
  check(domain.level()==40&&domain.state().highestRewardedLevel==40,"Multi-level marker failed");check(domain.state().wallet.coins==256+sum.coins&&domain.state().wallet.pearls==sum.pearls,"Skipped levels lost a reward");
  int next=2;for(const auto& event:domain.takeEvents())if(event.kind=="level")check(event.reachedLevel==next++,"Level events out of order");check(next==41,"Missing multi-level events");
  check(encode(decodeAndValidate(encode(domain.state()),content))==encode(domain.state()),"Multi-level save is invalid");
 }
 for(bool pearls:{false,true}){
  Domain domain(content);const auto id=adultAt(domain,79);auto s=domain.state();if(pearls)s.wallet.pearls=9'000'000'000'000'000LL;else s.wallet.coins=9'000'000'000'000'000LL;testing::openingBalances(s);domain.install(s);const auto before=encode(s);
  check(domain.execute({.action=Action::Keep,.fish=id}).error==Error::Overflow,"Overflowing reward was accepted");check(encode(domain.state())==before&&domain.takeEvents().empty(),"Overflow partially paid reward");
 }
 auto invalid=json;invalid["level_rewards"][1]["coins"]=-1;bool rejected=false;try{Content::fromJson(invalid);}catch(const std::exception&){rejected=true;}check(rejected,"Invalid reward tuning was accepted");
 std::cout<<"PASS multi-level jumps, ordered events, cap, atomic overflow and tuning validation\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
