#include "aquarium/storage.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#endif
namespace aq {
namespace {
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
bool bounded(std::int64_t x){return x>=0&&x<=9'000'000'000'000'000LL;}
Json point(WorldPoint p){return Json::array({p.x,p.y});}
WorldPoint readPoint(const Json& j){require(j.is_array()&&j.size()==2,"Invalid position shape");WorldPoint p{j[0].get<double>(),j[1].get<double>()};require(std::isfinite(p.x)&&std::isfinite(p.y)&&p.x>=0&&p.x<=1088&&p.y>=0&&p.y<=530,"Position outside aquarium");return p;}
Json readFile(const std::filesystem::path& p){auto size=std::filesystem::file_size(p);require(size<=8*1024*1024,"Save exceeds eight MiB limit");std::ifstream in(p);if(!in)throw std::runtime_error("Cannot open save");return Json::parse(in);}
}
Json encode(const State& s){
 Json j={{"version",s.version},{"contentVersion",s.contentVersion},{"simNow",s.simNow},{"wallAnchor",s.wallAnchor},{"calendarNow",s.calendarNow},{"coins",s.wallet.coins},{"pearls",s.wallet.pearls},{"xp",s.xp},{"highestRewardedLevel",s.highestRewardedLevel},{"activeTank",s.activeTank.value},{"nextFishId",s.nextFishId},{"nextDecorId",s.nextDecorId},{"rngState",s.rngState},{"claims",s.claims},{"collected",s.collected},{"mastery",s.mastery},{"totalEvents",s.totalEvents},{"giftTokens",s.giftTokens},{"tutorialStep",s.tutorialStep},{"tutorialClaims",s.tutorialClaims},{"dailyPeriod",s.dailyPeriod},{"weeklyPeriod",s.weeklyPeriod},{"giftDay",s.giftDay},{"eggDay",s.eggDay}};
 j["adultRaised"]=s.adultRaised;
 j["settings"]={{"reducedMotion",s.settings.reducedMotion},{"sound",s.settings.sound},{"music",s.settings.music},{"volume",s.settings.volume},{"tankLook",s.settings.tankLook}};
 j["tanks"]=Json::array();for(auto& t:s.tanks)j["tanks"].push_back({{"id",t.id.value},{"slots",t.slots}});
 j["fish"]=Json::array();for(auto& f:s.fish){const auto& m=f.motion;Json v={{"id",f.id.value},{"species",f.species},{"tank",f.tank.value},{"position",point(f.position)},{"age",f.age},{"egg",f.egg},{"dead",f.dead},{"stashed",f.stashed},{"growthMs",f.growthMs},{"hatchAt",f.hatchAt},{"lastFedAt",f.lastFedAt},{"stashedAt",f.stashedAt},{"boughtAt",f.boughtAt}};
  v["motion"]={{"direction",m.direction},{"turnFrom",m.turnFrom},{"cruise",m.cruise},{"phase",m.phase},{"drift",m.drift},{"targetY",m.targetY},{"retarget",m.retarget},{"dashInterval",m.dashInterval},{"dashWait",m.dashWait},{"dashDuration",m.dashDuration},{"dashStrength",m.dashStrength},{"dashRemaining",m.dashRemaining},{"turnDuration",m.turnDuration},{"turnRemaining",m.turnRemaining},{"pitch",m.pitch},{"speed",m.speed},{"mealDelay",m.mealDelay}};j["fish"].push_back(v);
 }
 j["decor"]=Json::array();for(auto& d:s.decor)j["decor"].push_back({{"id",d.id},{"kind",d.kind},{"tank",d.tank.value},{"position",point(d.position)}});
 j["quests"]=Json::object();for(auto& [id,q]:s.quests)j["quests"][id]={{"count",q.count},{"claimed",q.claimed}};return j;
}
State decodeAndValidate(const Json& j,const Content& c){
 require(j.is_object(),"Save must be an object");require(j.at("version").get<int>()==1,"Unsupported save version; original preserved");require(j.at("contentVersion").get<int>()==1,"Unsupported content version; original preserved");State s;
 s.simNow=j.at("simNow");s.wallAnchor=j.at("wallAnchor");s.calendarNow=j.at("calendarNow");s.wallet.coins=j.at("coins");s.wallet.pearls=j.at("pearls");s.xp=j.at("xp");s.highestRewardedLevel=j.at("highestRewardedLevel");s.activeTank={j.at("activeTank").get<int>()};
 require(bounded(s.simNow)&&bounded(s.wallAnchor)&&bounded(s.calendarNow),"Invalid clocks");require(bounded(s.wallet.coins)&&bounded(s.wallet.pearls)&&bounded(s.xp),"Invalid wallet");require(s.highestRewardedLevel>=levelFor(c,s.xp)&&s.highestRewardedLevel<=40,"Invalid rewarded level");
 s.nextFishId=j.at("nextFishId");s.nextDecorId=j.at("nextDecorId");s.rngState=j.at("rngState");require(s.nextFishId>0&&s.nextDecorId>0&&s.rngState!=0,"Invalid IDs or RNG");
 require(j.at("tanks").is_array()&&j.at("tanks").size()>=1&&j.at("tanks").size()<=5,"Invalid tanks");s.tanks.clear();std::set<int> tanks;
 for(auto& x:j.at("tanks")){Tank t{{x.at("id").get<int>()},x.at("slots").get<int>()};require(t.id.value>=1&&t.id.value<=5&&tanks.insert(t.id.value).second,"Invalid or duplicate tank");require(t.slots==10||t.slots==20||t.slots==30||t.slots==40,"Invalid capacity step");s.tanks.push_back(t);}
 require(tanks.contains(1)&&tanks.contains(s.activeTank.value),"Missing starter/active tank");for(int id:tanks)require(id==1||tanks.contains(id-1),"Tank sequence has a gap");
 require(j.at("fish").is_array()&&j.at("fish").size()<=10000,"Invalid fish collection");std::set<std::uint64_t> ids;
 for(auto& x:j.at("fish")){
  Fish f;f.id={x.at("id").get<std::uint64_t>()};f.species=x.at("species");f.tank={x.at("tank").get<int>()};f.position=readPoint(x.at("position"));f.age=x.at("age");f.egg=x.at("egg");f.dead=x.at("dead");f.stashed=x.at("stashed");f.growthMs=x.at("growthMs");f.hatchAt=x.at("hatchAt");f.lastFedAt=x.at("lastFedAt");f.stashedAt=x.at("stashedAt");f.boughtAt=x.at("boughtAt");
  require(f.id.value>0&&f.id.value<s.nextFishId&&ids.insert(f.id.value).second,"Duplicate/invalid fish identity");auto* spec=c.find(f.species);require(spec,"Unknown species in save");require(tanks.contains(f.tank.value),"Fish refers to missing tank");require(f.age>=0&&f.age<=4&&(!f.egg||f.age==0),"Invalid stage");require(!f.egg||!f.dead,"Dead egg is invalid");require(!f.stashed||!f.dead,"Dead fish cannot be stashed");require(f.growthMs>=0&&f.growthMs<spec->stageMs,"Invalid growth interval");require(f.hatchAt>=0&&f.hatchAt<=s.simNow+6000,"Invalid hatch deadline");require(f.boughtAt>=0&&f.boughtAt<=s.simNow,"Invalid purchase time");require(f.stashedAt>=0&&f.stashedAt<=s.simNow,"Invalid storage anchor");require(f.lastFedAt>=-spec->feedMs&&f.lastFedAt<=s.simNow,"Invalid feeding clock");
  const auto& m=x.at("motion");f.motion.direction=m.at("direction");f.motion.turnFrom=m.at("turnFrom");require(std::abs(f.motion.direction)==1&&std::abs(f.motion.turnFrom)==1,"Invalid direction");
  auto d=[&](const char* key,double lo,double hi){double v=m.at(key).get<double>();require(std::isfinite(v)&&v>=lo&&v<=hi,"Invalid motion parameter");return v;};
  f.motion.cruise=d("cruise",20,42);f.motion.phase=d("phase",0,1e15);f.motion.drift=d("drift",0,1e15);f.motion.targetY=d("targetY",0,635);f.motion.retarget=d("retarget",-1e9,1e9);f.motion.dashInterval=d("dashInterval",4,18);f.motion.dashWait=d("dashWait",-1e9,100);f.motion.dashDuration=d("dashDuration",.5,.9);f.motion.dashStrength=d("dashStrength",2.6,4.6);f.motion.dashRemaining=d("dashRemaining",0,.9);f.motion.turnDuration=d("turnDuration",.28,.42);f.motion.turnRemaining=d("turnRemaining",0,.42);f.motion.pitch=d("pitch",-.5,.5);f.motion.speed=d("speed",0,1000);f.motion.mealDelay=d("mealDelay",0,10);f.motion.previous=f.position;s.fish.push_back(f);
 }
 // Deliberately no living-count <= capacity check: revival may overflow capacity.
 require(j.at("decor").is_array()&&j.at("decor").size()<=500,"Invalid decor collection");std::set<std::uint64_t> decorIds;
 for(auto& x:j.at("decor")){Decoration d;d.id=x.at("id");d.kind=x.at("kind");d.tank={x.at("tank").get<int>()};d.position=readPoint(x.at("position"));require(d.id>0&&d.id<s.nextDecorId&&decorIds.insert(d.id).second,"Invalid decoration ID");require(tanks.contains(d.tank.value),"Invalid decoration tank");require(d.kind=="seaweed"||d.kind=="coral"||d.kind=="shell"||d.kind=="arch"||d.kind=="chest","Unknown decoration");s.decor.push_back(d);}
 auto strings=[&](const char* key){auto v=j.at(key).get<std::vector<std::string>>();require(v.size()<=10000,"Too many claim entries");std::set<std::string> seen;for(auto& x:v)require(!x.empty()&&x.size()<256&&seen.insert(x).second,"Invalid/duplicate claim");return v;};
 s.claims=strings("claims");s.collected=strings("collected");for(auto& id:s.collected)require(c.find(id),"Unknown collected species");s.tutorialClaims=strings("tutorialClaims");s.tutorialStep=j.at("tutorialStep");require(s.tutorialStep>=0&&s.tutorialStep<=11,"Invalid tutorial step");
 s.giftTokens=j.at("giftTokens");require(bounded(s.giftTokens),"Invalid Gift Tokens");s.dailyPeriod=j.at("dailyPeriod");s.weeklyPeriod=j.at("weeklyPeriod");s.giftDay=j.at("giftDay");s.eggDay=j.at("eggDay");
 s.totalEvents=j.at("totalEvents").get<std::map<std::string,std::int64_t>>();s.mastery=j.at("mastery").get<std::map<std::string,std::int64_t>>();require(s.totalEvents.size()<100&&s.mastery.size()<=46,"Oversized progress map");for(auto& [k,v]:s.totalEvents)require(bounded(v),"Invalid event counter");for(auto& [k,v]:s.mastery)require(c.find(k)&&bounded(v),"Invalid mastery counter");
 // Previous saves recorded sales in mastery, including Juniors. Retain those
 // counters as history, but do not convert them into unproven Adult growth.
 if(j.contains("adultRaised"))s.adultRaised=j.at("adultRaised").get<std::map<std::string,std::int64_t>>();
 require(s.adultRaised.size()<=46,"Oversized Adult progress map");for(auto& [id,count]:s.adultRaised)require(c.find(id)&&bounded(count),"Invalid Adult progress");
 require(j.at("quests").is_object()&&j.at("quests").size()<=100,"Invalid quests");for(auto& [id,v]:j.at("quests").items()){ObjectiveProgress p{v.at("count").get<std::int64_t>(),v.at("claimed").get<bool>()};require(id.size()<128&&bounded(p.count),"Invalid quest progress");s.quests[id]=p;}
 // Old daily-feed counted all feeds against a different target. Reset that
 // unclaimed objective when upgrading, preserving all claimed rewards.
 if(!j.contains("adultRaised")&&!s.quests["daily-feed"].claimed)s.quests["daily-feed"].count=0;
 auto& st=j.at("settings");s.settings.reducedMotion=st.at("reducedMotion");s.settings.sound=st.at("sound");s.settings.music=st.at("music");s.settings.volume=st.at("volume");s.settings.tankLook=st.at("tankLook");require(std::isfinite(s.settings.volume)&&s.settings.volume>=0&&s.settings.volume<=1,"Invalid volume");require(s.settings.tankLook>=0&&s.settings.tankLook<=2,"Invalid tank look");return s;
}
LoadResult Storage::load(const Content& c){
 if(!std::filesystem::exists(path_))return {};
 try{return {decodeAndValidate(readFile(path_),c),"Progress restored.",false};}
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
 }catch(const std::exception& e){return std::string("Save failed. Your previous file is safe: ")+e.what();}
}
Session::Session(Content c,std::filesystem::path p,Millis wall,bool ephemeral):domain_(std::move(c),wall),storage_(std::move(p)),ephemeral_(ephemeral){
 if(!ephemeral_){auto loaded=storage_.load(domain_.content());status_=loaded.message;if(loaded.state){domain_.install(std::move(*loaded.state));paused_=true;resume(wall);}}
}
Result Session::command(const Command& c){auto r=domain_.execute(c);if(r&&c.action!=Action::Move&&c.action!=Action::DropFood)checkpoint(domain_.state().wallAnchor);return r;}
void Session::update(Millis elapsed,Millis wall,Tool tool,FishId held){
 if(paused_||elapsed<=0)return;domain_.setCalendar(wall);domain_.advanceCare(elapsed);accumulator_+=elapsed;int steps=0;while(accumulator_>=20&&steps<16){domain_.stepMovement(.02,tool,held);accumulator_-=20;++steps;}if(accumulator_>=20)accumulator_%=20;sinceSave_+=elapsed;domain_.setWallAnchor(std::max(wall,domain_.state().wallAnchor));if(sinceSave_>=5000)checkpoint(wall);
}
void Session::checkpoint(Millis wall){domain_.setWallAnchor(std::max(wall,domain_.state().wallAnchor));if(!ephemeral_){auto message=storage_.save(domain_.state());status_=message.empty()?"Saved locally":message;}sinceSave_=0;}
void Session::suspend(Millis wall){if(paused_)return;paused_=true;domain_.clearTransient();accumulator_=0;checkpoint(wall);}
void Session::resume(Millis wall){if(!paused_)return;Millis elapsed=std::clamp<Millis>(wall-domain_.state().wallAnchor,0,315576000000LL);domain_.advanceCare(elapsed);domain_.clearTransient();domain_.setCalendar(wall);domain_.setWallAnchor(std::max(wall,domain_.state().wallAnchor));paused_=false;accumulator_=0;if(elapsed>60000)status_="Welcome back. Care advanced by "+std::to_string(elapsed/60000)+" minutes. Check your fish.";checkpoint(wall);}
}
