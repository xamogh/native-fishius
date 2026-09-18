#include "aquarium/hud_care.hpp"
#include "fixtures.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
State ready(const Content& content,int count=1,Amount xp=0){
 Domain domain(content,1000);auto state=domain.state();auto fish=state.fish.front();state.fish.clear();
 fish.purchase=purchaseQuote(content,*content.find(fish.species),1);fish.scripted=false;
 testing::stage(fish,4);fish.motion={};fish.position={544,350};fish.lastFedAt=state.simNow;
 for(int i=0;i<count;++i){fish.id={state.nextFishId++};state.fish.push_back(fish);}
 state.xp=xp;state.highestRewardedLevel=levelFor(content,xp);testing::openingBalances(state);return state;
}
void exercise(Canvas& canvas,const Content& content){
 Session session(content,"/tmp/reward-animation-unused.json",1000,true);HudCare care(canvas,session);
 auto reset=[&](State state){care.reset();session.domain().install(std::move(state));care.advance(10);};
 auto display=[&](){return care.rewards().display(session.domain());};
 auto settle=[&](FishId id,Action action=Action::Sell){auto result=session.command({.action=action,.fish=id});check(bool(result),"Reward transaction failed");care.advance(0);return result;};
 const auto base=ready(content);reset(base);
 const auto reward=settle(base.fish.front().id);const auto saved=encode(session.domain().state());
 check(session.domain().state().wallet.coins==base.wallet.coins+reward.coins&&session.domain().state().xp==base.xp+reward.xp,"Animation delayed the committed reward");
 check(care.rewards().active()&&display().coins==base.wallet.coins&&display().xp==base.xp,"Counters changed before the reward left the fish");
 care.advance(.6);check(display().coins==base.wallet.coins&&display().xp==base.xp&&display().coinPulse==0&&display().xpPulse==0,"A counter updated before arrival");
 auto previous=display();bool partialCoins=false,partialXp=false,coinPulse=false,xpPulse=false;
 for(int i=0;i<100;++i){
  care.advance(.016);const auto now=display();
  check(now.coins>=previous.coins&&now.xp>=previous.xp,"A collecting counter moved backwards");
  partialCoins|=now.coins>base.wallet.coins&&now.coins<base.wallet.coins+reward.coins;
  partialXp|=now.xp>base.xp&&now.xp<base.xp+reward.xp;
  coinPulse|=now.coinPulse>0;xpPulse|=now.xpPulse>0;previous=now;
 }
 check(partialCoins&&partialXp&&coinPulse&&xpPulse,"Rewards did not count up and highlight on arrival");
 check(!care.rewards().active()&&display().coins==session.domain().state().wallet.coins&&display().xp==session.domain().state().xp,"Counters did not converge to the saved totals");
 check(encode(session.domain().state())==saved,"Presentation changed the saved transaction");
 check(session.command({.action=Action::Sell,.fish=base.fish.front().id}).replayed,"Sale replay setup failed");care.advance(0);
 check(!care.rewards().active(),"A replayed sale animated a second reward");

 // A second sale joins the existing flight without restarting either counter.
 auto pair=ready(content,2);reset(pair);settle(pair.fish[0].id);care.advance(1.02);const auto midway=display();
 settle(pair.fish[1].id,Action::Keep);check(display().coins==midway.coins&&std::abs(display().xp-midway.xp)<.000001,"A second reward jumped or reset the counters");
 check(session.domain().companion(pair.fish[1].id)!=nullptr,"Keep did not retain the fish");
 previous=display();for(int i=0;i<130;++i){care.advance(.016);const auto now=display();check(now.coins>=previous.coins&&now.xp>=previous.xp,"Overlapping rewards moved backwards");previous=now;}
 check(display().coins==session.domain().state().wallet.coins&&display().xp==session.domain().state().xp,"Overlapping rewards lost a balance");

 // Real spending remains available while the visual amount catches up.
 reset(base);settle(base.fish.front().id);
 const auto bought=session.command({.action=Action::Buy,.key="neonTetra",.point={500,320}});check(bool(bought),"Could not spend during a flight");care.advance(0);
 check(display().coins==base.wallet.coins+bought.coins,"A live debit was hidden by a pending reward");care.advance(3);
 check(display().coins==session.domain().state().wallet.coins,"Spending left a stale counter");

 // Cross a level using the actual sale and its automatically emitted bonus.
 const auto nearLevel=ready(content,1,content.levels[1]-1);reset(nearLevel);settle(nearLevel.fish.front().id);
 check(session.domain().level()==2&&levelFor(content,Amount(display().xp))==1,"Level display changed ahead of the XP flight");
 check(display().coins==nearLevel.wallet.coins,"Level bonus bypassed the reward presentation");care.advance(3);
 check(levelFor(content,Amount(display().xp))==2&&display().coins==session.domain().state().wallet.coins,"Level crossing lost XP or bonus coins");
 check(care.notice().starts_with("Level 2!"),"Level notice did not follow the visible XP");

 const auto maximum=ready(content,1,content.levels.back());reset(maximum);check(settle(maximum.fish.front().id).xp==0,"Maximum-level sale awards visible XP");
 for(int i=0;i<100;++i){care.advance(.02);check(display().xp==maximum.xp&&display().xpPulse==0,"Zero XP created a progress animation");}
 auto junior=base;testing::stage(junior.fish.front(),1);reset(junior);check(settle(junior.fish.front().id).xp==0,"Junior reward setup changed");care.advance(1);
 check(display().xp==0&&display().xpPulse==0,"An early sale animated nonexistent XP");

 auto reduced=base;reduced.settings.reducedMotion=true;reset(reduced);settle(reduced.fish.front().id);
 check(display().coins==session.domain().state().wallet.coins&&display().xp==session.domain().state().xp&&display().coinPulse>0&&display().reducedMotion,"Reduced motion must credit immediately with a highlight");
 care.advance(.5);check(!care.rewards().active(),"Reduced-motion feedback did not finish");
 reset(base);settle(base.fish.front().id);auto settings=session.domain().state();settings.settings.reducedMotion=true;session.domain().install(settings);care.advance(0);
 check(display().reducedMotion&&display().coins==settings.wallet.coins&&display().xp==settings.xp,"Turning on reduced motion left a delayed balance");

 reset(base);settle(base.fish.front().id);care.reset();check(care.rewards().active(),"Changing tools cancelled a visible reward");
 care.advance(0,false);check(!care.rewards().active()&&display().coins==session.domain().state().wallet.coins,"Opening Shop left a stale reward or total");
 reset(base);check(bool(session.command({.action=Action::Sell,.fish=base.fish.front().id})),"Hidden reward setup failed");care.advance(0,false);
 check(!care.rewards().active(),"A sale from the same frame as opening Shop left a hidden flight");
 reset(base);settle(base.fish.front().id);care.advance(60);
 check(!care.rewards().active()&&display().coins==session.domain().state().wallet.coins,"A long frame stranded a reward");

 // A burst limit may shorten presentation, but every committed amount survives.
 auto many=ready(content,25);reset(many);for(const auto& fish:many.fish)settle(fish.id);
 check(display().coins>=0&&display().coins<=session.domain().state().wallet.coins,"Rapid sales overflowed the displayed balance");care.advance(3);
 check(display().coins==session.domain().state().wallet.coins&&display().xp==session.domain().state().xp,"Rapid sales dropped a reward");
 std::cout<<"PASS reward arrival, overlapping sales, Keep, debits, level crossing, maximum XP, reduced motion and menu changes\n";
}
void captures(Canvas& canvas,const Content& content,const std::filesystem::path& output){
 if(output.empty())return;
 Session session(content,"/tmp/reward-captures-unused.json",1000,true);HudCare care(canvas,session);
 for(const auto& viewport:std::array{PreviewViewport{1088,635,1,{}},PreviewViewport{667,375,1,{}},PreviewViewport{852,393,1,{59,0,59,21}},PreviewViewport{1024,768,1,{}},PreviewViewport{390,844,1,{}}}){
  canvas.previewViewport(viewport);canvas.begin();
  auto state=ready(content);state.fish.front().purchase.profit=117;state.fish.front().purchase.xp=36;session.domain().install(state);care.advance(10);
  const auto folder=output/(std::to_string(viewport.width)+"x"+std::to_string(viewport.height));std::filesystem::create_directories(folder);
  auto capture=[&](const std::string& name){
   canvas.begin();canvas.scene(session.domain(),1,0,Tool::Select,{},false);
   const auto hud=layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());const auto values=care.rewards().display(session.domain());
   paintHud(canvas,session.domain(),hud,{},{},&values);care.rewards().paint(canvas,hud);
   check(canvas.capture(folder/(name+".png")),"Reward capture failed");
  };
  capture("before");check(bool(session.command({.action=Action::Sell,.fish=state.fish.front().id})),"Capture sale failed");care.advance(0);
  double elapsed=0;
  for(const auto& [name,time]:std::array<std::pair<const char*,double>,5>{{{"burst",.30},{"flight",.72},{"arrival",1.04},{"absorbed",1.25},{"settled",1.9}}}){
   care.advance(time-elapsed);elapsed=time;capture(name);
  }
  if(viewport.width==1088){
   session.domain().install(state);care.advance(10);const auto sequence=folder/"sequence";std::filesystem::create_directories(sequence);
   for(int frame=0;frame<108;++frame){
    if(frame==12){check(bool(session.command({.action=Action::Sell,.fish=state.fish.front().id})),"Sequence sale failed");care.advance(0);}
    else care.advance(1./40);
    const auto number=std::to_string(frame);capture("sequence/"+std::string(4-number.size(),'0')+number);
   }
  }
 }
 std::cout<<"PASS reward captures at desktop, phone, tablet, portrait and notched safe areas\n";
}
}
int main(int argc,char** argv){try{
 const auto assets=std::filesystem::absolute(argc>1?argv[1]:"assets");std::ifstream file(assets/"content.json");const auto content=Content::fromJson(Json::parse(file));
 Canvas canvas(assets,1088,635,true);exercise(canvas,content);captures(canvas,content,argc>2?argv[2]:"");return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
