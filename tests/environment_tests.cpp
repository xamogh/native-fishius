#include "aquarium/storage.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace aq;
#define CHECK(value) do{if(!(value))throw std::runtime_error(std::string("Line ")+std::to_string(__LINE__)+": "+#value);}while(false)
int main(int argc,char** argv){try{
 std::ifstream input(argc>1?argv[1]:"assets/content.json");const auto content=Content::fromJson(Json::parse(input));
 Domain d(content);
 CHECK(d.tank({1})->backgroundId=="sunlit-lagoon");
 CHECK(d.ownsEnvironment("sunlit-lagoon"));
 CHECK(!d.ownsEnvironment("coral-garden")&&!d.ownsEnvironment("unknown"));
 const auto original=encode(d.state());
 CHECK(d.execute({.action=Action::PurchaseEnvironment,.tank={1},.key="coral-garden"}).error==Error::Funds);
 CHECK(encode(d.state())==original);
 CHECK(d.execute({.action=Action::EquipEnvironment,.tank={1},.key="coral-garden"}).error==Error::NotReady);
 CHECK(d.execute({.action=Action::PurchaseEnvironment,.tank={5},.key="coral-garden"}).error==Error::Unavailable);
 CHECK(d.execute({.action=Action::PurchaseEnvironment,.key="unknown"}).error==Error::Unknown);
 CHECK(encode(d.state())==original);
 auto oldSave=original;oldSave.erase("environmentOwned");for(auto& tank:oldSave["tanks"])tank.erase("backgroundId");
 CHECK(encode(decodeAndValidate(oldSave,content))==original);
 oldSave["tanks"][0]["reefId"]="starter-reef";oldSave["tanks"][0]["sandId"]="golden-sand";
 CHECK(encode(decodeAndValidate(oldSave,content))==original);
 auto paidLegacy=oldSave;paidLegacy["environmentOwned"]={"coral-arch","pearl-sand"};
 paidLegacy["tanks"][0]["reefId"]="coral-arch";paidLegacy["tanks"][0]["sandId"]="pearl-sand";
 const auto migrated=decodeAndValidate(paidLegacy,content);
 CHECK(migrated.environmentOwned==std::vector<std::string>{"coral-garden"});
 CHECK(migrated.tanks.front().backgroundId=="coral-garden"&&migrated.wallet.coins==d.state().wallet.coins);
 CHECK(migrated.receipts==d.state().receipts&&migrated.ledger==d.state().ledger);
 CHECK(encode(decodeAndValidate(encode(migrated),content))==encode(migrated));
 d.fixture("performance");
 auto state=d.state();if(state.tanks.size()==1)state.tanks.push_back({{2},10});d.install(state);
 const auto coins=d.state().wallet.coins,xp=d.state().xp;const auto count=d.state().decor.size();
 const auto snapshot=encode(d.state());
 const Command purchase{.action=Action::PurchaseEnvironment,.tank={1},.key="coral-garden",.requestId="background-purchase"};
 CHECK(d.execute(purchase,[](const State&){return false;}).error==Error::SaveFailure);
 CHECK(encode(d.state())==snapshot);
 CHECK(d.execute(purchase));CHECK(d.tank({1})->backgroundId=="sunlit-lagoon");
 CHECK(d.state().wallet.coins==coins-1200&&d.state().environmentOwned.size()==1);
 CHECK(d.execute(purchase).replayed);CHECK(d.state().wallet.coins==coins-1200);
 CHECK(d.execute({.action=Action::PurchaseEnvironment,.tank={1},.key="coral-garden"}));
 CHECK(d.state().wallet.coins==coins-1200&&d.state().environmentOwned.size()==1);
 CHECK(d.state().xp==xp&&d.state().decor.size()==count);
 CHECK(d.tank({1})->backgroundId=="sunlit-lagoon");
 CHECK(d.tank({2})->backgroundId=="sunlit-lagoon");
 CHECK(d.execute({.action=Action::EquipEnvironment,.tank={2},.key="coral-garden"}));
 CHECK(d.execute({.action=Action::EquipEnvironment,.tank={1},.key="sunlit-lagoon"}));
 CHECK(d.tank({2})->backgroundId=="coral-garden"&&d.tank({1})->backgroundId=="sunlit-lagoon");
 CHECK(d.state().wallet.coins==coins-1200);
 auto saved=encode(d.state());CHECK(encode(decodeAndValidate(saved,content))==saved);
 const auto saveDir=std::filesystem::temp_directory_path()/("aquarium-environment-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(saveDir);
 Storage storage(saveDir/"save.json");CHECK(storage.save(d.state()).empty());
 {Session reopened(content,saveDir/"save.json",d.state().wallAnchor);
  CHECK(reopened.domain().ownsEnvironment("coral-garden"));
  CHECK(reopened.domain().tank({1})->backgroundId=="sunlit-lagoon"&&reopened.domain().tank({2})->backgroundId=="coral-garden");
  CHECK(reopened.command({.action=Action::EquipEnvironment,.tank={2},.key="sunlit-lagoon"}));
 }
 {Session reopened(content,saveDir/"save.json",d.state().wallAnchor);
  CHECK(reopened.domain().tank({2})->backgroundId=="sunlit-lagoon");CHECK(reopened.domain().state().wallet.coins==coins-1200);
 }
 std::filesystem::remove_all(saveDir);
 CHECK(d.execute({.action=Action::EquipEnvironment,.tank={1},.key="coral-garden"},[](const State&){return false;}).error==Error::SaveFailure);
 CHECK(encode(d.state())==saved);
 const auto rejects=[&](Json bad){bool rejected=false;try{decodeAndValidate(bad,content);}catch(const std::exception&){rejected=true;}CHECK(rejected);};
 auto bad=original;bad["tanks"][0]["backgroundId"]="coral-garden";rejects(bad);
 bad=original;bad["tanks"][0]["backgroundId"]="missing-style";rejects(bad);
 bad=original;bad["environmentOwned"]={"missing-style"};rejects(bad);
 bad=saved;bad["environmentOwned"].push_back("coral-garden");rejects(bad);
 bad=oldSave;bad["tanks"][0]["reefId"]="coral-arch";rejects(bad);
 bad=oldSave;bad["tanks"][0]["reefId"]="golden-sand";rejects(bad);
 std::cout<<"PASS complete background purchases, per-tank selection, migration, ownership, replay, rollback and save validation\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
