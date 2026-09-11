#include <compare>
#include <string_view>
#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace aq {
using Json=nlohmann::json;
using Millis=std::int64_t;
using Amount=std::int64_t;
struct FishId {std::uint64_t value{};auto operator<=>(const FishId&)const=default;};
struct TankId {int value{1};auto operator<=>(const TankId&)const=default;};
struct WorldPoint {double x{},y{};};
struct Money {Amount coins{250},pearls{};};
enum class Currency {Coins,Pearls,Gift};
enum class Care {Fed,Hungry,Urgent,Sick,Dead};
enum class Tool {Select,Move,Stash,Food,Sell,Buy,Restore,Decor,Medicine};
struct Species {
 std::string id,modelId,name,rarity,role,badge,description,asset;
 int level{1};Currency currency{Currency::Coins};Amount price{},buyXp{};
 Millis stageMs{},feedMs{},graceMs{};std::array<Amount,5> saleCoins{},saleXp{};
 double nominalLength{38};int eventStart{},eventEnd{};bool eventConfigured{true},annual{},oneTime{},nonResellable{},artReady{true};
};
struct Content {
 std::vector<Species> species;std::array<Amount,40> levels{};Json workbook;Json supplement;
 std::array<std::array<Amount,4>,5> tankCosts{};
 std::array<std::array<Amount,4>,5> tankTokens{};
 std::array<int,5> tankLevels{1,7,16,25,34};
 Money startingWallet;int startingTankCapacity{10};Amount levelTwoPearls{1};
 std::vector<std::string> starters;
 std::array<int,3> masteryTargets{5,25,100};
 const Species* find(std::string_view id)const;
 static Content fromJson(const Json&);
};
struct Motion {
 int direction{1},turnFrom{1};double cruise{30},phase{},drift{},targetY{250},retarget{2};
 double dashInterval{10},dashWait{5},dashDuration{.7},dashStrength{3.5},dashRemaining{};
 double turnDuration{.35},turnRemaining{},pitch{},speed{},noticeDelay{},mealDelay{};
 std::uint64_t foodTarget{};WorldPoint previous{};
 // Presentation history is transient. It never changes care rules or saves.
 double previousPhase{},previousPitch{},previousFacing{},previousSpeed{};bool hasPrevious{};
};
double motionFacing(const Motion&);
struct Fish {
 FishId id;std::string species;TankId tank;WorldPoint position;int age{};bool egg{},dead{},stashed{};
 Millis growthMs{},hatchAt{},lastFedAt{},stashedAt{},boughtAt{};Motion motion;
};
struct Tank {TankId id;int slots{10};};
struct Pellet {std::uint64_t id{};WorldPoint position;double speed{38},phase{},rotation{},rest{};WorldPoint previous{};bool hasPrevious{};};
struct DecorDef {std::string id,name;Amount price{};int score{};std::string source;};
struct Decoration {std::uint64_t id{};std::string kind;TankId tank;WorldPoint position;};
struct Quest {std::string id,label,event;int target{1};Amount coins{},xp{},tokens{},pearls{};bool weekly{};std::string source;int level{1};bool configured{true};std::string missing;};
struct MasteryProgress {std::int64_t count{};int target{5},tier{};bool ready{},complete{};};
struct ObjectiveProgress {std::int64_t count{};bool claimed{};};
struct Settings {bool reducedMotion{},sound{true},music{true};double volume{.65};int tankLook{};};
struct State {
 int version{1},contentVersion{1};Millis simNow{},wallAnchor{},calendarNow{};Money wallet;Amount xp{};
 int highestRewardedLevel{1};TankId activeTank;std::vector<Tank> tanks{{TankId{1},10}};
 std::vector<Fish> fish;std::vector<Decoration> decor;std::uint64_t nextFishId{1},nextDecorId{1},rngState{0x94239abd};
 std::vector<std::string> claims;std::map<std::string,ObjectiveProgress> quests;
 std::map<std::string,std::int64_t> totalEvents,mastery;std::vector<std::string> collected;
 std::map<std::string,std::int64_t> adultRaised;
 Amount giftTokens{};int tutorialStep{};std::vector<std::string> tutorialClaims;
 std::int64_t dailyPeriod{-1},weeklyPeriod{-1},giftDay{-1},eggDay{-1};Settings settings;
};
enum class Error {None,Unknown,NoArt,EventClosed,Level,Claimed,Full,Funds,InvalidPosition,InvalidFish,NotSellable,Dead,NotDead,NotStored,AlreadyOwned,PreviousTank,Maximum,NoFoodRoom,Unavailable,NotReady,Overflow};
struct Result {Error error{Error::None};FishId fish{};Amount coins{},xp{},pearls{},tokens{};std::string message;explicit operator bool()const{return error==Error::None;}};
struct Event {std::string kind;FishId fish{};WorldPoint position;Amount coins{},xp{},pearls{};std::string text;};
enum class Action {Buy,Feed,Sell,Stash,Restore,Move,Revive,Remove,ReviveAll,RemoveAll,UnlockTank,ExpandTank,SwitchTank,DropFood,BuyDecor,ClaimQuest,SendGift,DailyEgg,ClaimMastery,Tutorial,SetLook,SetReducedMotion,SetSound,SetMusic,SetVolume};
struct Command {Action action;FishId fish{};TankId tank{};std::string key;WorldPoint point{};Currency currency{Currency::Coins};double value{};};
struct Calendar {std::int64_t day{},week{};int year{},monthDay{};};
Calendar calendarAt(Millis unixMs);
bool eventOpen(const Species&,const Calendar&);
int levelFor(const Content&,Amount xp);
Care careOf(const Species&,const Fish&,Millis now);
bool sellable(const Species&,const Fish&);
double ageScale(const Species&,const Fish&);
void advanceFish(const Species&,Fish&,Millis from,Millis to);
std::string errorText(Error);
std::string stageName(const Fish&);
std::string careName(Care);
Json encode(const State&);
State decodeAndValidate(const Json&,const Content&);

// Owns authoritative state. No filesystem, SDL or system clock access.
class Domain {
 public:
 explicit Domain(Content content,Millis calendarMs=0,std::uint64_t seed=0x94239abd);
 const State& state()const{return state_;}const Content& content()const{return content_;}
 const std::vector<Pellet>& pellets()const{return pellets_;}
 const std::vector<DecorDef>& decorations()const{return decorDefs_;}
 const std::vector<Quest>& questDefinitions()const{return questDefs_;}
 MasteryProgress masteryProgress(std::string_view species)const;
 const Fish* fish(FishId)const;const Tank* tank(TankId)const;
 std::size_t living(TankId)const;int level()const{return levelFor(content_,state_.xp);}
 Result blocker(const Species&)const;Result execute(const Command&);
 void advanceCare(Millis delta);void stepMovement(double seconds,Tool tool,FishId held={});
 void setCalendar(Millis now);void setWallAnchor(Millis now){state_.wallAnchor=now;}
 void clearTransient();void install(State candidate);std::vector<Event> takeEvents();
 void fixture(std::string_view name); // Explicit developer/review fixture, never called in ordinary play.
 private:
 Content content_;State state_;std::vector<Pellet> pellets_;std::uint64_t nextPellet_{1};
 std::vector<Event> events_;std::vector<DecorDef> decorDefs_;std::vector<Quest> questDefs_;
 double random(double low,double high);Fish makeFish(std::string_view,WorldPoint,bool egg);
 Fish* mutableFish(FishId);Result grant(Amount coins,Amount xp,Amount pearls=0,Amount tokens=0);
 void count(std::string_view event,std::int64_t amount=1);void emit(Event);
 void beginTurn(Fish&,int direction);void configureExtras();void tutorialEvent(std::string_view);
 Result executeImpl(const Command&);Result tutorial();Result claimQuest(std::string_view);Result sendGift();Result dailyEgg();
};
}
