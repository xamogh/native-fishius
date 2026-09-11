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
bool validPoint(WorldPoint p){return std::isfinite(p.x)&&std::isfinite(p.y)&&p.x>=0&&p.x<=1088&&p.y>=0&&p.y<=512;}
bool contains(const std::vector<std::string>& v,std::string_view s){return std::find(v.begin(),v.end(),s)!=v.end();}
Result bad(Error e){return {e,{},{},{},{},{},errorText(e)};}
std::string claimKey(const Species& s,const Calendar& c){return s.id+(s.annual?":"+std::to_string(c.year):"");}
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
}
const Species* Content::find(std::string_view id)const{auto i=std::find_if(species.begin(),species.end(),[&](auto& s){return s.id==id;});return i==species.end()?nullptr:&*i;}
Content Content::fromJson(const Json& j){
 Content c;check(j.at("schema")==1,"Unsupported content schema");
 for(const auto& x:j.at("species")){
  Species s;s.id=x.at("id");s.modelId=x.at("model_id");s.name=x.at("name");s.rarity=x.at("rarity");s.role=x.value("role","");
  s.level=x.at("level");std::string currency=x.at("currency");s.currency=currency=="pearls"?Currency::Pearls:currency=="gift"?Currency::Gift:Currency::Coins;
  s.price=x.at("price");s.buyXp=x.at("buy_xp");s.stageMs=x.at("stage_ms");s.feedMs=x.at("feed_ms");s.graceMs=x.at("grace_ms");
  s.saleCoins=x.at("sale_coins").get<std::array<Amount,5>>();s.saleXp=x.at("sale_xp").get<std::array<Amount,5>>();
  s.badge=x.value("badge","");s.description=x.value("description","");s.nominalLength=x.value("length",38.);s.asset="species/"+s.id+".png";
  s.eventStart=x.value("event_start",0);s.eventEnd=x.value("event_end",0);s.eventConfigured=x.value("event_configured",true);s.annual=x.value("annual",false);s.oneTime=x.value("one_time",false);s.nonResellable=x.value("non_resellable",false);
  check(s.stageMs>0&&s.feedMs>0&&s.graceMs>0,"Invalid lifecycle duration");check(s.price>=0&&s.buyXp>=0,"Invalid price");
  check(!c.find(s.id),"Duplicate species id");c.species.push_back(std::move(s));
 }
 check(c.species.size()==46,"Expected the 46-entry launch catalog");
 c.levels=j.at("levels").get<std::array<Amount,40>>();check(c.levels[0]==0&&c.levels[1]==80,"Invalid initial XP curve");
 check(std::is_sorted(c.levels.begin(),c.levels.end()),"XP curve not monotonic");c.workbook=j.value("raw_sheets",Json::object());c.supplement=j.value("supplement",Json::object());
 c.tankCosts=j.at("tank_costs").get<decltype(c.tankCosts)>();c.tankLevels=j.at("tank_levels").get<decltype(c.tankLevels)>();
 if(j.contains("tank_tokens"))c.tankTokens=j.at("tank_tokens").get<decltype(c.tankTokens)>();
 const auto inputs=j.value("inputs",Json::object());c.startingWallet={inputs.value("starting_coins",Amount{250}),inputs.value("starting_pearls",Amount{0})};c.startingTankCapacity=inputs.value("starting_tank_capacity",10);c.levelTwoPearls=inputs.value("level_2_pearl_grant",Amount{1});
 c.starters=j.value("starter_species",std::vector<std::string>{"neonTetra","guppy","platy","molly"});for(const auto& id:c.starters)check(c.find(id)&&c.find(id)->level==1,"Invalid starter species");
 if(c.supplement.contains("mastery"))c.masteryTargets=c.supplement["mastery"]["adult_targets"].get<decltype(c.masteryTargets)>();check(c.masteryTargets[0]>0&&c.masteryTargets[0]<c.masteryTargets[1]&&c.masteryTargets[1]<c.masteryTargets[2],"Invalid mastery milestones");
 check(c.startingWallet.coins>=0&&c.startingWallet.pearls>=0&&c.levelTwoPearls>=0&&c.startingTankCapacity==10,"Invalid starting configuration");
 for(std::size_t i=0;i<c.tankCosts.size();++i){check(c.tankLevels[i]>=1&&c.tankLevels[i]<=40,"Invalid tank unlock level");for(auto cost:c.tankCosts[i])check(cost>=0,"Invalid tank price");for(auto cost:c.tankTokens[i])check(cost>=0,"Invalid tank material cost");}
 return c;
}
Calendar calendarAt(Millis unixMs){
 using namespace std::chrono;const auto d=floor<days>(sys_time<milliseconds>{milliseconds{unixMs}});year_month_day ymd{d};
 const auto day=d.time_since_epoch().count();const auto week=floor<days>(d-days{weekday{d}.iso_encoding()-1}).time_since_epoch().count()/7;
 return {day,week,int(ymd.year()),int(unsigned(ymd.month()))*100+int(unsigned(ymd.day()))};
}
bool eventOpen(const Species& s,const Calendar& c){if(!s.eventConfigured)return false;if(!s.eventStart)return true;return s.eventStart<=s.eventEnd?(c.monthDay>=s.eventStart&&c.monthDay<=s.eventEnd):(c.monthDay>=s.eventStart||c.monthDay<=s.eventEnd);}
int levelFor(const Content& c,Amount xp){return static_cast<int>(std::upper_bound(c.levels.begin(),c.levels.end(),xp)-c.levels.begin());}
Care careOf(const Species& s,const Fish& f,Millis now){if(f.dead)return Care::Dead;if(f.egg)return Care::Fed;Millis elapsed=(f.stashed?f.stashedAt:now)-f.lastFedAt;if(elapsed>=s.feedMs)return Care::Sick;if(elapsed*10>=s.feedMs*9)return Care::Urgent;if(elapsed*4>=s.feedMs*3)return Care::Hungry;return Care::Fed;}
bool sellable(const Species& s,const Fish& f){return !f.egg&&!f.dead&&!f.stashed&&f.age>=1&&!s.nonResellable;}
double ageScale(const Species& s,const Fish& f){constexpr std::array<double,5> steps{.75,1,1.07,1.14,1.21};int a=std::clamp(f.age,0,4);return .88*(steps[a]+(steps[std::min(4,a+1)]-steps[a])*std::clamp(double(f.growthMs)/double(s.stageMs),0.,1.)*.5);}
void advanceFish(const Species& s,Fish& f,Millis from,Millis to){
 if(to<=from||f.dead||f.stashed)return;
 if(f.egg){if(to<f.hatchAt)return;from=std::max(from,f.hatchAt);f.egg=false;f.age=0;f.growthMs=0;f.lastFedAt=f.hatchAt-s.feedMs*3/4;}
 const Millis pause=f.lastFedAt+s.feedMs*3/4;const Millis death=f.lastFedAt+s.feedMs+s.graceMs;
 if(f.age<4){Millis end=std::min({to,pause,death});if(end>from){f.growthMs+=end-from;while(f.growthMs>=s.stageMs&&f.age<4){f.growthMs-=s.stageMs;++f.age;}if(f.age==4)f.growthMs=0;}}
 if(to>=death){f.dead=true;f.motion.foodTarget=0;}
}
std::string errorText(Error e){switch(e){case Error::None:return "";case Error::Unknown:return "That item is not available.";case Error::NoArt:return "Artwork unavailable. Nothing was charged.";case Error::EventClosed:return "This seasonal event is closed.";case Error::Level:return "Reach the required level first.";case Error::Claimed:return "Already claimed for this period.";case Error::Full:return "This tank is full. Sell, stash or expand first.";case Error::Funds:return "Not enough currency.";case Error::InvalidPosition:return "Place inside the aquarium.";case Error::InvalidFish:return "That fish is no longer here.";case Error::NotSellable:return "Only Junior or older, resellable living fish can be sold.";case Error::Dead:return "A dead fish cannot be stored or fed.";case Error::NotDead:return "This fish is already alive.";case Error::NotStored:return "That fish is not in inventory.";case Error::AlreadyOwned:return "You already own this tank.";case Error::PreviousTank:return "Unlock the preceding tank first.";case Error::Maximum:return "Maximum capacity reached.";case Error::NoFoodRoom:return "There is enough food in the water already.";case Error::Unavailable:return "This external service is not connected.";case Error::NotReady:return "Complete the objective first.";case Error::Overflow:return "The transaction exceeds the supported save bounds.";}return "Unknown action.";}
std::string stageName(const Fish& f){if(f.dead)return "Dead";if(f.egg)return "Egg";constexpr std::array<const char*,5> names{"Baby","Junior","Young","Mature","Adult"};return names[std::clamp(f.age,0,4)];}
std::string careName(Care c){switch(c){case Care::Fed:return "Happy & fed";case Care::Hungry:return "Hungry";case Care::Urgent:return "Feed soon!";case Care::Sick:return "SICK";case Care::Dead:return "Needs revival";}return "";}
Domain::Domain(Content content,Millis calendarMs,std::uint64_t seed):content_(std::move(content)){
 state_.calendarNow=calendarMs;state_.wallAnchor=calendarMs;state_.rngState=seed?seed:1;
 state_.wallet=content_.startingWallet;state_.tanks[0].slots=content_.startingTankCapacity;
 for(std::size_t i=0;i<content_.starters.size();++i){auto f=makeFish(content_.starters[i],{220.+190.*double(i),220.+130.*double(i%2)},false);state_.collected.push_back(f.species);state_.fish.push_back(std::move(f));}
 configureExtras();setCalendar(calendarMs);
}
double Domain::random(double a,double b){auto x=state_.rngState;x^=x<<13;x^=x>>7;x^=x<<17;state_.rngState=x;return a+(b-a)*double(x>>11)/9007199254740992.;}
Fish Domain::makeFish(std::string_view id,WorldPoint p,bool egg){
 const auto* s=content_.find(id);check(s,"Unknown species in makeFish");Fish f;f.id={state_.nextFishId++};f.species=id;f.tank=state_.activeTank;f.position=p;f.egg=egg;f.hatchAt=state_.simNow+(egg?6000:0);f.lastFedAt=egg?state_.simNow:state_.simNow-s->feedMs*3/4;f.boughtAt=state_.simNow;
 auto& m=f.motion;m.cruise=random(20,42);m.direction=random(0,1)<.5?-1:1;m.phase=random(0,2*pi);m.drift=random(0,2*pi);m.targetY=random(100,460);m.retarget=random(1,4);m.dashInterval=random(4,18);m.dashWait=random(1,m.dashInterval);m.dashDuration=random(.5,.9);m.dashStrength=random(2.6,4.6);m.turnDuration=random(.28,.42);m.previous=p;return f;
}
const Fish* Domain::fish(FishId id)const{auto it=std::find_if(state_.fish.begin(),state_.fish.end(),[&](auto& f){return f.id==id;});return it==state_.fish.end()?nullptr:&*it;}
Fish* Domain::mutableFish(FishId id){return const_cast<Fish*>(std::as_const(*this).fish(id));}
const Tank* Domain::tank(TankId id)const{auto i=std::find_if(state_.tanks.begin(),state_.tanks.end(),[&](auto& t){return t.id==id;});return i==state_.tanks.end()?nullptr:&*i;}
std::size_t Domain::living(TankId id)const{return static_cast<std::size_t>(std::count_if(state_.fish.begin(),state_.fish.end(),[&](auto& f){return f.tank==id&&!f.stashed&&!f.dead;}));}
Result Domain::blocker(const Species& s)const{
 if(!s.artReady)return bad(Error::NoArt);auto cal=calendarAt(state_.calendarNow);if(!eventOpen(s,cal))return bad(Error::EventClosed);if(level()<s.level)return bad(Error::Level);if((s.annual||s.oneTime)&&contains(state_.claims,claimKey(s,cal)))return bad(Error::Claimed);
 auto* t=tank(state_.activeTank);if(!t||living(t->id)>=static_cast<std::size_t>(t->slots))return bad(Error::Full);Amount bal=s.currency==Currency::Pearls?state_.wallet.pearls:state_.wallet.coins;if(s.currency!=Currency::Gift&&bal<s.price)return bad(Error::Funds);return {};
}
Result Domain::grant(Amount coins,Amount xp,Amount pearls,Amount tokens){
 if(!addOk(state_.xp,xp))return bad(Error::Overflow);
 const int after=levelFor(content_,state_.xp+xp);const Amount levelPearl=state_.highestRewardedLevel<2&&after>=2?content_.levelTwoPearls:0;
 if(!addOk(state_.wallet.coins,coins)||!addOk(state_.xp,xp)||!addOk(state_.wallet.pearls,pearls+levelPearl)||!addOk(state_.giftTokens,tokens))return bad(Error::Overflow);
 const int before=level();state_.wallet.coins+=coins;state_.wallet.pearls+=pearls+levelPearl;state_.xp+=xp;state_.giftTokens+=tokens;state_.highestRewardedLevel=std::max(state_.highestRewardedLevel,after);
 if(after>before){configureExtras();emit({"level",{}, {544,90},0,0,levelPearl,"Level "+std::to_string(after)+"!"});}return {Error::None,{},coins,xp,pearls+levelPearl,tokens,{}};
}
void Domain::emit(Event e){if(events_.size()>=64)events_.erase(events_.begin());events_.push_back(std::move(e));}
std::vector<Event> Domain::takeEvents(){auto result=std::move(events_);events_.clear();return result;}
void Domain::count(std::string_view event,std::int64_t n){state_.totalEvents[std::string(event)]+=n;for(auto& q:questDefs_)if(q.event==event&&level()>=q.level){auto& p=state_.quests[q.id];p.count=std::min<std::int64_t>(q.target,p.count+n);}tutorialEvent(event);}
void Domain::setCalendar(Millis now){state_.calendarNow=std::max(state_.calendarNow,now);auto c=calendarAt(state_.calendarNow);if(c.day>state_.dailyPeriod){for(auto& q:questDefs_)if(!q.weekly)state_.quests[q.id]={};state_.dailyPeriod=c.day;}if(c.week>state_.weeklyPeriod){for(auto& q:questDefs_)if(q.weekly)state_.quests[q.id]={};state_.weeklyPeriod=c.week;}}
void Domain::advanceCare(Millis delta){
 if(delta<=0)return;if(delta>315576000000LL)throw std::invalid_argument("Care advance exceeds ten years");if(state_.simNow>limit-delta)throw std::overflow_error("Simulation clock exhausted");const auto to=state_.simNow+delta;
 const auto caretaker=std::find_if(questDefs_.begin(),questDefs_.end(),[](const Quest& q){return q.id=="daily-feed";});
 const int feedTarget=caretaker==questDefs_.end()?0:caretaker->target;
 for(auto& f:state_.fish){const auto* s=content_.find(f.species);const bool egg=f.egg,dead=f.dead;const int age=f.age;const auto before=careOf(*s,f,state_.simNow);advanceFish(*s,f,state_.simNow,to);if(egg&&!f.egg)emit({"hatch",f.id,f.position,0,0,0,"Hello, little "+s->name+"!"});if(age<f.age)emit({"growth",f.id,f.position,0,0,0,stageName(f)+"!"});if(age<4&&f.age==4){++state_.adultRaised[f.species];count("raised-adult");}if(!dead&&f.dead)emit({"death",f.id,f.position,0,0,0,s->name+" needs help"});if(!f.stashed&&!dead&&before!=Care::Sick&&(careOf(*s,f,to)==Care::Sick||f.dead)){auto& progress=state_.quests["daily-feed"];if(!progress.claimed&&progress.count<feedTarget)progress.count=0;}}
 state_.simNow=to;
}
void Domain::beginTurn(Fish& f,int d){auto& m=f.motion;if(d==m.direction||m.turnRemaining>0)return;m.turnFrom=m.direction;m.turnDuration=random(.28,.42);m.turnRemaining=m.turnDuration;}
double motionFacing(const Motion& m){return m.turnRemaining>0?-m.turnFrom*std::cos(pi*(1-m.turnRemaining/m.turnDuration)):-double(m.direction);}
void Domain::stepMovement(double dt,Tool tool,FishId held){
 if(!(dt>0&&dt<=.05))return;
 for(auto& p:pellets_){p.previous=p.position;p.hasPrevious=true;if(p.position.y<507){p.speed=std::min(105.,p.speed+34.*dt);p.position.y=std::min(507.,p.position.y+p.speed*dt);p.position.x=std::clamp(p.position.x+std::sin(p.phase)*3.*dt,8.,1080.);p.rotation+=22.*dt;p.phase+=dt;}else p.rest+=dt;}
 std::erase_if(pellets_,[](auto& p){return p.rest>=31.8;});
 std::vector<FishId> deadIds;for(auto& f:state_.fish)if(!f.stashed&&f.tank==state_.activeTank&&f.dead)deadIds.push_back(f.id);std::sort(deadIds.begin(),deadIds.end());
 struct Arrival {FishId fish;std::uint64_t pellet;double when;};std::vector<Arrival> arrivals;
 for(auto& f:state_.fish){if(f.stashed||f.tank!=state_.activeTank)continue;auto& m=f.motion;m.previous=f.position;m.previousPhase=m.phase;m.previousPitch=m.pitch;m.previousFacing=motionFacing(m);m.previousSpeed=m.speed;m.hasPrevious=true;const auto& s=*content_.find(f.species);m.drift+=dt;
  if(f.egg){f.position.y=std::min(517.,f.position.y+45.*dt);continue;}
  if(f.dead){auto it=std::find(deadIds.begin(),deadIds.end(),f.id);const double spacing=deadIds.size()<2?0:std::min(64.,256./double(deadIds.size()-1));double tx=544.+(double(it-deadIds.begin())-double(deadIds.size()-1)*.5)*spacing;f.position.x+=(tx-f.position.x)*(1-std::exp(-.28*dt));const double target=40.+std::sin(m.drift*.8)*2.5;f.position.y=std::max(target,f.position.y-18.*dt);m.pitch=std::sin(m.drift*.7)*.07;m.speed=0;continue;}
  Care care=careOf(s,f,state_.simNow);bool hungry=care!=Care::Fed;bool sick=care==Care::Sick;bool heldNow=f.id==held||(tool==Tool::Sell&&sellable(s,f));
  const double halfHeight=s.nominalLength*ageScale(s,f)*.3;const double top=56.+halfHeight,bottom=512.-halfHeight;
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
 }
 std::sort(arrivals.begin(),arrivals.end(),[](auto& a,auto& b){if(std::abs(a.when-b.when)>1e-9)return a.when<b.when;return a.fish<b.fish;});
 for(auto& a:arrivals){auto p=std::find_if(pellets_.begin(),pellets_.end(),[&](auto& x){return x.id==a.pellet;});auto* f=mutableFish(a.fish);if(p==pellets_.end()||!f||f->dead||careOf(*content_.find(f->species),*f,state_.simNow)==Care::Fed)continue;auto r=execute({Action::Feed,a.fish});if(r){f=mutableFish(a.fish);f->motion.mealDelay=random(.15,.45);f->motion.retarget=random(.6,1.4);f->motion.foodTarget=0;pellets_.erase(p);}}
}
void Domain::clearTransient(){pellets_.clear();for(auto& f:state_.fish){f.motion.foodTarget=0;f.motion.noticeDelay=0;f.motion.previous=f.position;f.motion.hasPrevious=false;}}
void Domain::install(State candidate){state_=std::move(candidate);configureExtras();clearTransient();events_.clear();}
Result Domain::execute(const Command& c){
 State original=state_;auto oldPellets=pellets_;auto oldEvents=events_;auto oldNext=nextPellet_;
 auto rollback=[&]{state_=std::move(original);pellets_=std::move(oldPellets);events_=std::move(oldEvents);nextPellet_=oldNext;configureExtras();};
 try{auto result=executeImpl(c);if(!result)rollback();return result;}
 catch(const std::overflow_error&){rollback();return bad(Error::Overflow);}
 catch(...){rollback();throw;}
}
Result Domain::executeImpl(const Command& c){
 Fish* f=mutableFish(c.fish);
 switch(c.action){
 case Action::Buy:{
  auto* s=content_.find(c.key);if(!s)return bad(Error::Unknown);auto gate=blocker(*s);if(!gate)return gate;if(!validPoint(c.point))return bad(Error::InvalidPosition);
  if(!addOk(state_.xp,s->buyXp)||state_.nextFishId==std::numeric_limits<std::uint64_t>::max())return bad(Error::Overflow);
  auto r=grant(0,s->buyXp);if(!r)return r;if(s->currency==Currency::Coins)state_.wallet.coins-=s->price;else if(s->currency==Currency::Pearls)state_.wallet.pearls-=s->price;
  Fish baby=makeFish(s->id,c.point,true);r.fish=baby.id;state_.fish.push_back(std::move(baby));if(s->oneTime||s->annual)state_.claims.push_back(claimKey(*s,calendarAt(state_.calendarNow)));
  if(!contains(state_.collected,s->id)){state_.collected.push_back(s->id);count("collection");}count("buy");count("place");
  emit({"purchase",r.fish,c.point,s->currency==Currency::Coins?-s->price:0,s->buyXp,s->currency==Currency::Pearls?-s->price:0,""});return r;
 }
 case Action::Feed:{
  if(!f||f->stashed)return bad(Error::InvalidFish);if(f->dead)return bad(Error::Dead);if(f->egg)return bad(Error::NotReady);auto* s=content_.find(f->species);if(careOf(*s,*f,state_.simNow)==Care::Fed)return bad(Error::NotReady);
  const bool healthy=std::none_of(state_.fish.begin(),state_.fish.end(),[&](const Fish& other){return !other.stashed&&!other.dead&&careOf(*content_.find(other.species),other,state_.simNow)==Care::Sick;});
  f->lastFedAt=state_.simNow;f->motion.foodTarget=0;f->motion.noticeDelay=0;emit({"feed",f->id,f->position,0,0,0,"Yum!"});count("feed");if(healthy)count("healthy-feed");return {};
 }
 case Action::Sell:{
  if(!f)return bad(Error::InvalidFish);const auto* s=content_.find(f->species);if(!sellable(*s,*f)||f->tank!=state_.activeTank)return bad(Error::NotSellable);
  const int age=f->age;const WorldPoint at=f->position;const auto species=f->species;const auto id=f->id;auto r=grant(s->saleCoins[age],s->saleXp[age]);if(!r)return r;
  std::erase_if(state_.fish,[&](auto& x){return x.id==id;});++state_.mastery[species];count("sell");if(age==4)count("adult-sale");emit({"sale",id,at,r.coins,r.xp,r.pearls,""});return r;
 }
 case Action::Stash:
  if(!f||f->stashed||f->tank!=state_.activeTank)return bad(Error::InvalidFish);if(f->dead)return bad(Error::Dead);f->stashed=true;f->stashedAt=state_.simNow;f->motion.foodTarget=0;emit({"stash",f->id,f->position,0,0,0,"Safe in inventory"});return {};
 case Action::Restore:{
  if(!f||!f->stashed)return bad(Error::NotStored);auto* t=tank(state_.activeTank);if(living(state_.activeTank)>=static_cast<std::size_t>(t->slots))return bad(Error::Full);if(!validPoint(c.point))return bad(Error::InvalidPosition);Millis paused=state_.simNow-f->stashedAt;
  f->hatchAt+=paused;f->lastFedAt+=paused;f->stashed=false;f->tank=state_.activeTank;f->position=c.point;f->motion.previous=c.point;f->motion.foodTarget=0;emit({"restore",f->id,c.point,0,0,0,"Welcome back!"});return {};
 }
 case Action::Move:
  if(!f||f->stashed||f->tank!=state_.activeTank)return bad(Error::InvalidFish);if(!validPoint(c.point))return bad(Error::InvalidPosition);f->position=c.point;f->motion.previous=c.point;f->motion.foodTarget=0;return {};
 case Action::Revive:
  if(!f||!f->dead)return bad(Error::NotDead);if(state_.wallet.pearls<1)return bad(Error::Funds);--state_.wallet.pearls;f->dead=false;f->lastFedAt=state_.simNow;f->motion.pitch=0;f->motion.foodTarget=0;emit({"revive",f->id,f->position,0,0,-1,"Back to life!"});return {};
 case Action::Remove:
  if(!f||!f->dead)return bad(Error::NotDead);std::erase_if(state_.fish,[&](auto& x){return x.id==c.fish;});return {};
 case Action::ReviveAll:{
  const auto countDead=static_cast<Amount>(std::count_if(state_.fish.begin(),state_.fish.end(),[](auto& x){return x.dead;}));if(!countDead)return bad(Error::NotDead);if(state_.wallet.pearls<countDead)return bad(Error::Funds);
  state_.wallet.pearls-=countDead;for(auto& x:state_.fish)if(x.dead){x.dead=false;x.lastFedAt=state_.simNow;x.motion.pitch=0;x.motion.foodTarget=0;}emit({"revive",{}, {544,160},0,0,-countDead,"All fish revived"});return {};
 }
 case Action::RemoveAll:{auto before=state_.fish.size();std::erase_if(state_.fish,[](auto& x){return x.dead;});return before==state_.fish.size()?bad(Error::NotDead):Result{};}
 case Action::UnlockTank:{
  // Coins are the common sink. The solo route supplies the listed Gift Tokens
  // instead of friend assists; tokens do not bypass the coin price.
  int id=c.tank.value;if(id<1||id>5)return bad(Error::Unknown);if(tank(c.tank))return bad(Error::AlreadyOwned);if(id>1&&!tank(TankId{id-1}))return bad(Error::PreviousTank);if(level()<content_.tankLevels[id-1])return bad(Error::Level);if(c.currency==Currency::Pearls)return bad(Error::Unavailable);
  const Amount coins=content_.tankCosts[id-1][0],tokens=content_.tankTokens[id-1][0];
  if(state_.wallet.coins<coins||state_.giftTokens<tokens)return {Error::Funds,{},{},{},{},{},"This tank needs "+std::to_string(coins)+" coins and "+std::to_string(tokens)+" Gift Tokens."};
  state_.wallet.coins-=coins;state_.giftTokens-=tokens;state_.tanks.push_back({c.tank,10});state_.activeTank=c.tank;clearTransient();emit({"tank",{}, {544,220},-coins,0,0,"A new aquarium!"});return {};
 }
 case Action::ExpandTank:{
  int id=c.tank.value;if(id<1||id>5)return bad(Error::Unknown);auto it=std::find_if(state_.tanks.begin(),state_.tanks.end(),[&](auto& t){return t.id==c.tank;});if(it==state_.tanks.end())return bad(Error::Unknown);if(it->slots>=40)return bad(Error::Maximum);if(c.currency==Currency::Pearls)return bad(Error::Unavailable);
  const auto step=it->slots/10;const Amount coins=content_.tankCosts[id-1][step],tokens=content_.tankTokens[id-1][step];
  if(state_.wallet.coins<coins||state_.giftTokens<tokens)return {Error::Funds,{},{},{},{},{},"This expansion needs "+std::to_string(coins)+" coins and "+std::to_string(tokens)+" Gift Tokens."};
  state_.wallet.coins-=coins;state_.giftTokens-=tokens;it->slots+=10;emit({"tank",{}, {544,220},-coins,0,0,"More room to swim!"});return {};
 }
 case Action::SwitchTank:
  if(!tank(c.tank))return bad(Error::Unknown);state_.activeTank=c.tank;clearTransient();return {};
 case Action::DropFood:
  if(!validPoint(c.point))return bad(Error::InvalidPosition);if(pellets_.size()>=48)return bad(Error::NoFoodRoom);pellets_.push_back({nextPellet_++,c.point,38,random(0,2*pi),0,0});return {};
 case Action::BuyDecor:{
  auto it=std::find_if(decorDefs_.begin(),decorDefs_.end(),[&](auto& d){return d.id==c.key;});if(it==decorDefs_.end())return bad(Error::Unknown);if(!validPoint(c.point))return bad(Error::InvalidPosition);if(state_.decor.size()>=500)return bad(Error::Maximum);if(state_.wallet.coins<it->price)return bad(Error::Funds);
  state_.wallet.coins-=it->price;state_.decor.push_back({state_.nextDecorId++,it->id,state_.activeTank,c.point});count("decor");emit({"decor",{},c.point,-it->price,0,0,"Looking lovely!"});return {};
 }
 case Action::ClaimQuest:return claimQuest(c.key);
 case Action::SendGift:return sendGift();
 case Action::DailyEgg:return dailyEgg();
 case Action::ClaimMastery:{
  const auto* s=content_.find(c.key);if(!s)return bad(Error::Unknown);const auto progress=masteryProgress(c.key);if(progress.complete)return bad(Error::Claimed);if(!progress.ready)return bad(Error::NotReady);state_.claims.push_back("mastery:"+c.key+":"+std::to_string(progress.target));emit({"mastery",{}, {544,220},0,0,0,s->name+" "+std::to_string(progress.target)+" Adult mastery badge earned!"});return {};
 }
 case Action::Tutorial:return tutorial();
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
 result.complete=result.tier==3;result.target=result.complete?goals.back():goals[static_cast<std::size_t>(result.tier)];result.ready=!result.complete&&result.count>=result.target;return result;
}
void Domain::configureExtras(){
 // Decor catalog prices remain local configuration because the workbook does
 // not contain individual decor prices or score values.
 decorDefs_={{"seaweed","Seaweed garden",25,5,"ASSUMPTION: local decor configuration"},{"coral","Sunset coral",60,12,"ASSUMPTION: local decor configuration"},{"shell","Pearl shell",40,8,"ASSUMPTION: local decor configuration"},{"arch","Little stone arch",100,20,"ASSUMPTION: local decor configuration"},{"chest","Treasure chest",150,30,"ASSUMPTION: local decor configuration"}};
 questDefs_.clear();
 const auto key=std::to_string(level());
 if(!content_.supplement.contains("quest_definitions"))return;
 for(const auto& definition:content_.supplement["quest_definitions"]){
  Quest q;q.id=definition.at("id");q.label=definition.at("label");q.event=definition.at("event");q.target=definition.at("target");q.level=definition.at("level");q.weekly=definition.at("weekly");q.configured=definition.value("configured",true);q.missing=definition.value("missing","");q.source=definition.at("source").dump();
  const auto& profiles=content_.supplement["quests_by_level"];
  if(profiles.contains(key)&&profiles[key].contains(q.id)){const auto& reward=profiles[key][q.id];q.coins=reward.value("coins",Amount{0});q.xp=reward.value("xp",Amount{0});q.tokens=reward.value("tokens",Amount{0});q.pearls=reward.value("pearls",Amount{0});q.source+="; "+reward.at("source").dump();}
  questDefs_.push_back(std::move(q));
 }
}

Result Domain::claimQuest(std::string_view id){
 auto q=std::find_if(questDefs_.begin(),questDefs_.end(),[&](auto& x){return x.id==id;});if(q==questDefs_.end())return bad(Error::Unknown);if(level()<q->level)return bad(Error::Level);if(!q->configured)return {Error::Unavailable,{},{},{},{},{},q->missing};auto& p=state_.quests[q->id];if(p.claimed)return bad(Error::Claimed);if(p.count<q->target)return bad(Error::NotReady);
 if(q->coins==0&&q->xp==0&&q->tokens==0&&q->pearls==0)return {Error::Unavailable,{},{},{},{},{},"No reward is configured for this quest at your level."};
 auto r=grant(q->coins,q->xp,q->pearls,q->tokens);if(!r)return r;p.claimed=true;tutorialEvent("quest");emit({"quest",{}, {544,240},r.coins,r.xp,r.pearls,"Quest complete!"});return r;
}
Result Domain::sendGift(){
 if(level()<5)return bad(Error::Level);
 return {Error::Unavailable,{},{},{},{},{},"Gift rewards will be available when their token amounts are configured."};
}
Result Domain::dailyEgg(){
 if(level()<6)return bad(Error::Level);
 return {Error::Unavailable,{},{},{},{},{},"The Daily Egg Basket needs its weekly fish table and resale rules."};
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
 if(state_.tutorialStep==7){if(level()<2)return {Error::NotReady,{},{},{},{},{},"Reach 80 XP through normal fish purchases and sales. No unexplained XP is added."};tutorialEvent("level");return {};}
 if(state_.tutorialStep==10){tutorialEvent("finish");return {};}
 return bad(Error::NotReady);
}
void Domain::fixture(std::string_view name){
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

 if(name=="shop"||name=="tanks"||name=="collection"||name=="settings"){
  state_.fish.clear();state_.wallet={250,0};state_.xp=14;state_.highestRewardedLevel=1;state_.tutorialStep=11;state_.tanks={{{1},10}};state_.activeTank={1};
  const std::array<WorldPoint,8> points{{{600,193},{477,284},{520,320},{702,357},{531,488},{726,482},{400,405},{792,287}}};
  for(std::size_t i=0;i<points.size();++i){auto f=makeFish(i==1?"neonTetra":"guppy",points[i],false);f.age=4;f.lastFedAt=state_.simNow;f.motion.cruise=20;state_.fish.push_back(f);}
  clearTransient();events_.clear();return;
 }
 state_.fish.clear();state_.wallet={12650,38};state_.xp=content_.levels[14]+100;state_.highestRewardedLevel=15;state_.tutorialStep=11;state_.tanks={{{1},40},{{2},20}};state_.activeTank={1};
 std::size_t countFish=name=="performance"?40:12;for(std::size_t i=0;i<countFish;++i){const auto& s=content_.species[i%content_.species.size()];auto f=makeFish(s.id,{140.+double(i%8)*112.,155.+double((i/8)%4)*85.},false);f.age=1+static_cast<int>(i%4);f.lastFedAt=state_.simNow;state_.fish.push_back(f);}
 if(name=="care"&&!state_.fish.empty()){auto& f=state_.fish[0];f.lastFedAt=state_.simNow-content_.find(f.species)->feedMs;if(state_.fish.size()>1){state_.fish[1].dead=true;}}
 if(name=="inventory"&&state_.fish.size()>2){state_.fish[0].stashed=true;state_.fish[0].stashedAt=state_.simNow;}
 clearTransient();events_.clear();
}
} // namespace aq
