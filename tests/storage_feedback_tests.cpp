#include "aquarium/storage.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace aq;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
#define CHECK(value) check(bool(value),#value)
struct TemporaryDirectory {
 std::filesystem::path path;
 TemporaryDirectory(){
  const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
  for(int attempt=0;attempt<100;++attempt){
   path=std::filesystem::temp_directory_path()/("fishx-storage-feedback-"+std::to_string(stamp)+"-"+std::to_string(attempt));
   if(std::filesystem::create_directory(path))return;
  }
  throw std::runtime_error("Cannot create isolated storage test directory");
 }
 ~TemporaryDirectory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
void write(const std::filesystem::path& path,const std::string& bytes){std::ofstream out(path);out<<bytes;check(bool(out),"Cannot create test file");}
std::string read(const std::filesystem::path& path){std::ifstream in(path);check(bool(in),"Cannot read test file");return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};}
}
int main(int argc,char** argv){
 try{
  if(argc<2)throw std::runtime_error("Pass assets/content.json");
  std::ifstream in(argv[1]);const auto content=Content::fromJson(Json::parse(in));
  using Test=std::pair<std::string,std::function<void()>>;std::vector<Test> tests;
  auto test=[&](std::string name,std::function<void()> run){tests.emplace_back(std::move(name),std::move(run));};
  test("failed saves stay visible through gameplay and clear after retry",[&]{
   TemporaryDirectory temp;const auto parent=temp.path/"blocked-parent";const auto path=parent/"save.json";
   Session session(content,path,1000);CHECK(!session.notification());write(parent,"regular file blocks the save directory");
   const auto before=session.domain().state().wallet.coins;
   CHECK(session.command({Action::Buy,{}, {},"neonTetra",{400,300}}).error==Error::SaveFailure);
   CHECK(session.domain().state().wallet.coins==before);CHECK(session.saveFailed());
   CHECK(session.notification()->kind==SessionNotificationKind::SaveFailure);
   CHECK(session.status().find("Progress is not saved")!=std::string::npos);
   session.dismissNotification();CHECK(session.saveFailed());CHECK(session.notification());
   session.update(5000,6000,Tool::Select);CHECK(session.saveFailed());
   CHECK(!session.command({Action::Buy,{}, {},"unknown",{400,300}}));CHECK(session.saveFailed());
   CHECK(!session.checkpoint(6000));CHECK(read(parent)=="regular file blocks the save directory");
   std::filesystem::remove(parent);CHECK(session.checkpoint(6000));
   CHECK(!session.saveFailed());CHECK(!session.notification());CHECK(session.status().empty());
   Storage storage(path);const auto loaded=storage.load(content);CHECK(loaded.state);
   CHECK(encode(*loaded.state)==encode(session.domain().state()));
   session.update(5000,11000,Tool::Select);CHECK(!session.notification());
  });
  test("healthy saves and restores do not create periodic notifications",[&]{
   TemporaryDirectory temp;const auto path=temp.path/"save.json";
   Session session(content,path,1000);CHECK(session.checkpoint(1000));CHECK(!session.notification());
   Session restored(content,path,2000);CHECK(!restored.notification());CHECK(restored.status().empty());
   restored.suspend(2000);restored.resume(122000);CHECK(!restored.notification());
  });
  test("recovery warning survives startup resume and successful checkpoints until dismissed",[&]{
   TemporaryDirectory temp;const auto path=temp.path/"save.json";Domain domain(content,1000);
   Storage storage(path);CHECK(storage.save(domain.state()).empty());CHECK(storage.save(domain.state()).empty());
   write(path,"broken original");Session recovered(content,path,61000);
   CHECK(recovered.notification());CHECK(recovered.notification()->kind==SessionNotificationKind::Recovery);
   CHECK(recovered.status().find("Loaded recovery copy")!=std::string::npos);CHECK(!recovered.saveFailed());
   CHECK(recovered.domain().state().simNow==domain.state().simNow+60000);
   const auto warning=recovered.status();CHECK(recovered.checkpoint(61000));CHECK(recovered.status()==warning);
   recovered.update(5000,66000,Tool::Select);CHECK(recovered.status()==warning);
   CHECK(read(path)=="broken original");CHECK(std::filesystem::exists(path.string()+".recovered"));
   recovered.dismissNotification();CHECK(!recovered.notification());CHECK(recovered.checkpoint(66000));CHECK(!recovered.notification());
  });
  test("failed recovery save takes priority without losing the recovery warning",[&]{
   TemporaryDirectory temp;const auto path=temp.path/"save.json";write(path,"broken original");
   Session recovered(content,path,1000);CHECK(recovered.notification());CHECK(recovered.notification()->kind==SessionNotificationKind::Recovery);
   const auto warning=recovered.status();const auto blocked=std::filesystem::path(path.string()+".recovered.tmp");
   CHECK(std::filesystem::create_directory(blocked));CHECK(!recovered.checkpoint(1000));
   CHECK(recovered.notification()->kind==SessionNotificationKind::SaveFailure);recovered.dismissNotification();CHECK(recovered.saveFailed());
   std::filesystem::remove(blocked);CHECK(recovered.checkpoint(1000));CHECK(!recovered.saveFailed());
   CHECK(recovered.notification()->kind==SessionNotificationKind::Recovery);CHECK(recovered.status()==warning);
   recovered.dismissNotification();CHECK(!recovered.notification());CHECK(read(path)=="broken original");
  });
  test("review sessions remain independent of unavailable save locations",[&]{
   TemporaryDirectory temp;const auto parent=temp.path/"blocked-parent";write(parent,"unchanged");
   Session review(content,parent/"save.json",1000,true);CHECK(review.checkpoint(1000));CHECK(!review.notification());CHECK(read(parent)=="unchanged");
  });
  test("current decoration positions and stored copies round-trip exactly",[&]{
   Domain domain(content,1000);State state=domain.state();state.decorOwned={"CP-01"};state.nextDecorId=4;
   state.decor={{1,"CP-01",{1},{120,500}},{2,"CP-01",{1},{900,500}},{3,"CP-01",{1},{0,0},true}};
   const auto restored=decodeAndValidate(encode(state),content);CHECK(restored.decor.size()==3);
   CHECK(restored.decor[0].position.x==120);CHECK(restored.decor[0].position.y==500);
   CHECK(restored.decor[1].position.x==900);CHECK(restored.decor[1].position.y==500);
   CHECK(restored.decor[2].stored);CHECK(restored.decor[2].position.x==0);CHECK(restored.decor[2].position.y==0);
   CHECK(restored.decorOwned==state.decorOwned);CHECK(restored.wallet.coins==state.wallet.coins);CHECK(restored.xp==state.xp);
   CHECK(encode(decodeAndValidate(encode(restored),content))==encode(restored));
  });
  test("pending paid decoration retains the capacity and identity needed for storage",[&]{
   Domain domain(content,1000);State state=domain.state();state.decorOwned={"CP-01"};state.pendingDecor="CP-01";state.nextDecorId=501;
   for(std::uint64_t id=1;id<=499;++id)state.decor.push_back({id,"CP-01",{1},{544,500},true});
   CHECK(decodeAndValidate(encode(state),content).pendingDecor=="CP-01");
   auto rejects=[&](const State& invalid){bool rejected=false;try{decodeAndValidate(encode(invalid),content);}catch(const std::exception&){rejected=true;}CHECK(rejected);};
   state.decor.push_back({500,"CP-01",{1},{544,500},true});rejects(state);
   state.decor.pop_back();state.nextDecorId=std::numeric_limits<std::uint64_t>::max();rejects(state);
  });
  int failures=0;for(auto& [name,run]:tests){try{run();std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& error){++failures;std::cerr<<"FAIL "<<name<<": "<<error.what()<<'\n';}}
  std::cout<<tests.size()-std::size_t(failures)<<" / "<<tests.size()<<" tests passed\n";return failures?1:0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
}
