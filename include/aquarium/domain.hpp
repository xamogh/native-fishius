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
#include <functional>
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
 std::string releaseGate{"Launch"},scheduleId;bool companion{};
 Millis durationMs{};int coinWeightBps{10000},xpWeightBps{10000},coinFactorBps{10000},xpFactorBps{10000};
};
struct Economy {
 Amount baseCoinDay{360},baseXpDay{96},minimumPrice{5};
 int coinSlopeBps{450},xpSlopeBps{100},principalShareBps{2500},earlyRefundBps{9500};
 std::array<int,5> stages{0,2500,5500,8000,10000},rewards{0,1000,4000,7000,10000};
};
struct GrowthSnapshot {
 int level{1};std::string configVersion,scheduleId;
 Amount principal{},profit{},xp{};Millis durationMs{},feedMs{};
 std::array<int,5> stages{0,2500,5500,8000,10000},rewards{0,1000,4000,7000,10000};int earlyRefundBps{9500};
 auto operator<=>(const GrowthSnapshot&)const=default;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GrowthSnapshot,level,configVersion,scheduleId,principal,profit,xp,durationMs,feedMs,stages,rewards,earlyRefundBps)
struct TankEntitlement {std::string id,prerequisite;TankId tank;int level{},slots{},addedSlots{};Money cost;};
enum class TreasureKind {Coins,Pearls,Bundle};
struct TreasureOffer {
 std::string id,name,eligibility,asset;TreasureKind kind{TreasureKind::Coins};
 int priceUsdCents{},coinDaysBps{},level{};Amount pearls{};bool oncePerAccount{},permanentFrame{};
};
struct DecorDef {
 std::string id,name;Amount price{};int score{};std::string source;
 std::string category,subcategory,theme,edition,rarity,size,layer,releaseGate,event,availability,asset;
 int level{1};Currency currency{Currency::Coins};Amount buyXp{};double width{1},height{1};
 Json art;bool legacy{},artReady{true};
};
struct DecorEvent {std::string name,proposedWindow;bool configured{};Millis startsAt{},endsAt{};};
struct DecorTuning {int placedLimit{32},animatedLimit{6},emitterLimit{2},particleLimit{6},launchLevelCap{40};Amount tutorialTotalXp{15};std::string tutorialItem{"CP-01"};};
struct Content {
 std::string configVersion;Economy economy;int displaySlots{8};Millis hatchMs{6000};
 std::vector<TankEntitlement> tankEntitlements;std::array<Money,40> levelRewards{};
 std::vector<TreasureOffer> treasureOffers;int referenceUtilizationNumerator{5},referenceUtilizationDenominator{6};
 std::vector<Species> species;std::array<Amount,40> levels{};Json workbook;Json supplement;
 std::array<std::array<Amount,4>,5> tankCosts{};
 std::array<std::array<Amount,4>,5> tankPearlCosts{};
 std::array<int,5> tankLevels{1,7,16,25,34};
 Money startingWallet;int startingTankCapacity{10};
 Amount levelRewardCoinsPerLevel{50},levelRewardPearls{1};
 std::vector<std::string> starters;
 std::array<int,4> masteryTargets{1,3,10,25};
 const Species* find(std::string_view id)const;
 std::vector<DecorDef> decorations;std::vector<DecorEvent> decorEvents;DecorTuning decorTuning;
 const DecorDef* findDecor(std::string_view id)const;
 void configureDecorEvents(const Json&);
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
 GrowthSnapshot purchase;bool favorite{},scripted{};
};
// Permanent display ownership carries no production or settlement values.
struct Companion {
 FishId id,origin;std::string species;TankId tank;WorldPoint position;Motion motion;
 bool stored{},favorite{};Millis lastFedAt{};
};
Fish companionVisual(const Companion&);
GrowthSnapshot purchaseQuote(const Content&,const Species&,int level,bool gift=false);
Money treasureContents(const Content&,const TreasureOffer&,int level);
struct FishReward {Amount principal{},profit{},xp{};Amount coins()const{return principal+profit;}};
FishReward fishReward(const Fish&);
int growthStage(const GrowthSnapshot&,Millis elapsed);
double growthProgress(const Fish&);
double nextStageProgress(const Fish&);
struct EnvironmentStyle {std::string id,name;Amount price;std::string asset;};
std::span<const EnvironmentStyle> environmentCatalog();
const EnvironmentStyle* findEnvironment(std::string_view);
struct Tank {TankId id;int slots{10};std::string backgroundId{"sunlit-lagoon"};};
struct Pellet {std::uint64_t id{};WorldPoint position;double speed{38},phase{},rotation{},rest{};WorldPoint previous{};bool hasPrevious{};};
struct Decoration {std::uint64_t id{};std::string kind;TankId tank;WorldPoint position;bool stored{},flipped{};double sizeMul{1};};
// Gameplay and the visible tank share the full world rectangle.
constexpr double waterWidth=1088,tankHeight=635,waterHeight=tankHeight;
// Keep decor perspective and initial previews in their original art space.
constexpr double decorReferenceHeight=512;
constexpr double decorSizeMin=.6,decorSizeMax=1.7,decorSizeStep=.12;
bool inTank(WorldPoint);
bool inPlacementWater(WorldPoint);
WorldPoint decorPlacementPoint(WorldPoint);
WorldPoint decorPlacementPoint(const DecorDef&,WorldPoint,double sizeMul=1);
double decorDepth(WorldPoint);
double decorScale(WorldPoint);
double decorHaze(WorldPoint);
struct Quest {std::string id,label,event;int target{1};Amount coins{},xp{},pearls{};bool weekly{};std::string source;int level{1};bool configured{true};std::string missing;};
struct MasteryProgress {std::int64_t count{};int target{5},tier{};bool ready{},complete{};};
struct ObjectiveProgress {std::int64_t count{};bool claimed{};};
struct Settings {bool reducedMotion{},sound{true},music{true};double volume{.65};int tankLook{};};
struct State {
 int version{4},contentVersion{4};Millis simNow{},wallAnchor{},calendarNow{};Money wallet;Amount xp{},lifetimeXp{};
 int highestRewardedLevel{1};TankId activeTank;std::vector<Tank> tanks{{TankId{1},10}};
 std::vector<Fish> fish;std::vector<Decoration> decor;std::uint64_t nextFishId{1},nextDecorId{1},rngState{0x94239abd};
 std::vector<std::string> decorOwned;bool decorOnboardingComplete{};
 std::vector<std::string> environmentOwned;
 // Cancelling legacy decor placement stores the paid copy.
 std::string pendingDecor;
 std::vector<std::string> claims;std::map<std::string,ObjectiveProgress> quests;
 std::map<std::string,std::int64_t> totalEvents,mastery;std::vector<std::string> collected;
 std::map<std::string,std::int64_t> adultRaised;
 int tutorialStep{};std::vector<std::string> tutorialClaims;
 std::int64_t dailyPeriod{-1},weeklyPeriod{-1},giftDay{-1},eggDay{-1};Settings settings;
 std::vector<Companion> companions;
 // Receipts and ledger are serialized with the state in one atomic save.
 Json receipts=Json::object(),settlements=Json::object(),ledger=Json::array();
 std::uint64_t nextRequestId{1},revision{};
 std::map<std::string,std::vector<std::int64_t>> careDays;
};
enum class Error {None,Unknown,NoArt,EventClosed,Level,Claimed,Full,Funds,InvalidPosition,InvalidFish,NotSellable,Dead,NotDead,NotStored,AlreadyOwned,PreviousTank,Maximum,NoFoodRoom,Unavailable,NotReady,Overflow,Conflict,SaveFailure,Protected};
struct CurrencyShortfall {Amount coins{},pearls{};};
struct Result {Error error{Error::None};FishId fish{};Amount coins{},xp{},pearls{};std::string message;CurrencyShortfall shortfall{};bool replayed{};std::uint64_t revision{};explicit operator bool()const{return error==Error::None;}};
struct Event {std::string kind;FishId fish{};WorldPoint position;Amount coins{},xp{},pearls{};std::string text;int reachedLevel{};};
enum class Action {Buy,Feed,Sell,Stash,Restore,Move,Revive,Remove,ReviveAll,RemoveAll,UnlockTank,ExpandTank,SwitchTank,DropFood,BuyDecor,ClaimQuest,SendGift,DailyEgg,ClaimMastery,Tutorial,SetLook,SetReducedMotion,SetSound,SetMusic,SetVolume,MoveDecor,StoreDecor,RestoreDecor,FlipDecor,PurchaseDecor,PlaceDecor,CancelDecor,ResizeDecor,Keep,Favorite,PurchaseEnvironment,EquipEnvironment};
struct Command {Action action;FishId fish{};TankId tank{};std::string key;WorldPoint point{};Currency currency{Currency::Coins};double value{};std::uint64_t decor{};std::string requestId;std::optional<GrowthSnapshot> offer;};
struct Calendar {std::int64_t day{},week{};int year{},monthDay{};};
Calendar calendarAt(Millis unixMs);
bool eventOpen(const Species&,const Calendar&);
int levelFor(const Content&,Amount xp);
Money levelReward(const Content&,int level);
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
 const Decoration* decoration(std::uint64_t)const;
 Result blocker(const DecorDef&)const;bool decorVisible(const DecorDef&)const;
 std::size_t placedDecor(TankId)const;int decorScore(TankId)const;Amount decorPurchaseXp(const DecorDef&)const;
 bool validDecorPoint(const DecorDef&,WorldPoint)const;
 const std::vector<Quest>& questDefinitions()const{return questDefs_;}
 MasteryProgress masteryProgress(std::string_view species)const;
 const Fish* fish(FishId)const;const Tank* tank(TankId)const;
 bool ownsEnvironment(std::string_view)const;
 const Companion* companion(FishId)const;std::size_t displaying(TankId)const;
 const TankEntitlement* nextTankEntitlement(TankId)const;
 GrowthSnapshot quote(const Species& s)const{return purchaseQuote(content_,s,level());}
 std::size_t living(TankId)const;int level()const{return levelFor(content_,state_.xp);}
 Result blocker(const Species&,bool includeCapacity=true)const;Result execute(const Command&);
 Result execute(const Command&,const std::function<bool(const State&)>& commit);
 void advanceCare(Millis delta);void stepMovement(double seconds,Tool tool,FishId held={});
 void setCalendar(Millis now);void setWallAnchor(Millis now){state_.wallAnchor=now;}
 void clearTransient();void install(State candidate);std::vector<Event> takeEvents();
 void fixture(std::string_view name); // Explicit developer/review fixture, never called in ordinary play.
 private:
 Content content_;State state_;std::vector<Pellet> pellets_;std::uint64_t nextPellet_{1};
 std::vector<Event> events_;std::vector<DecorDef> decorDefs_;std::vector<Quest> questDefs_;
 double random(double low,double high);Fish makeFish(std::string_view,WorldPoint,bool egg);
 Fish* mutableFish(FishId);Result grant(Amount coins,Amount xp,Amount pearls=0);
 Result requireFunds(Amount coins,Amount pearls=0)const;
 void count(std::string_view event,std::int64_t amount=1);void emit(Event);
 void beginTurn(Fish&,int direction);void configureExtras();void tutorialEvent(std::string_view);
 Result executeImpl(const Command&);Result tutorial();Result claimQuest(std::string_view);Result sendGift();Result dailyEgg();
 Result executeDecor(const Command&);
 Result executeEnvironment(const Command&);
 Result settle(const Command&);Result spend(Amount coins,Amount pearls=0);
 void ledger(std::string currency,Amount delta,Amount balance,std::string reason,std::string source);
 std::string requestId_,reason_,source_;
};
}
