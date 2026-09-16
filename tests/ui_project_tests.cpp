#include "aquarium/ui_project.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace aq;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class Fn>void rejects(Fn fn,const char* message){bool rejected=false;try{fn();}catch(const std::exception&){rejected=true;}check(rejected,message);}
}
int main(int argc,char** argv){try{
 const auto assets=std::filesystem::absolute(argc>1?argv[1]:"assets");auto p=UiProject::defaults();p.validate(assets);check(UiProject::fromJson(p.toJson())==p,"Project round trip changed its data");
 auto legacy=DialogDesign::defaults();legacy.coins.title="Custom dialog";legacy.element(DialogPart::Title).box.x=120;auto migrated=UiProject::defaults(legacy);check(migrated.screen("general-dialog").variants.at("coins").values.at("title")=="Custom dialog"&&migrated.node("@dialog","title")->box.x==120,"Legacy design was not migrated");
 auto bad=p;bad.screen("general-dialog").variants["pearls"].overrides["dialog/title|@dialog-header|label"]={{"box",DesignBox{0,0,-10,100}}};rejects([&]{bad.validate();},"Negative variant bounds were accepted");
 bad=p;bad.components["dialog"].children.push_back(p.screen("general-dialog").root);rejects([&]{bad.validate();},"Circular component reference accepted");
 bad=p;bad.screen("general-dialog").variants["pearls"].values["illustration"]="../secret.png";rejects([&]{bad.validate();},"Escaping image path accepted");
 bad=p;bad.screen("general-dialog").variants["pearls"].overrides["missing"]={{"hidden",true}};rejects([&]{bad.validate();},"Orphaned override accepted");
 bad=p;bad.screen("general-dialog").variants["pearls"].overrides["dialog|@dialog|detail"]={{"layout",{{"after","detail"}}}};rejects([&]{bad.validate();},"Variant constraint cycle accepted");
 auto before=p.components.at("dialog");check(uiGroup(p,"@dialog",{"illustration","message"}),"Sibling grouping failed");p.validate(assets);const auto group=p.components.at("dialog").children.back().id;check(uiUngroup(p,"@dialog",group),"Ungroup failed");check(p.node("@dialog","illustration")->box==before.children[2].box&&p.node("@dialog","message")->box==before.children[3].box,"Group changed positions");
 check(uiDuplicate(p,"@dialog",{"illustration"}),"Duplicate failed");p.validate(assets);
 const auto temp=std::filesystem::temp_directory_path()/("aq-project-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(temp);
 const auto file=temp/"approved.json",draft=temp/"draft.json";
 {UiDocument doc(file,assets,draft);check(doc.save(),"Initial save failed");auto edit=doc.project();edit.styles["heading"].size=72;check(doc.apply(edit,"size"),"Edit failed");edit.styles["heading"].size=76;doc.apply(edit,"size");check(doc.undo()&&doc.project().styles.at("heading").size==68,"Gesture did not coalesce");check(doc.redo()&&doc.dirty(),"Redo failed");check(doc.autosave(),"Draft autosave failed");}
 {UiDocument doc(file,assets,draft);check(doc.recovered()&&doc.project().styles.at("heading").size==76&&doc.canUndo(),"Draft and history did not recover");check(doc.undo()&&!doc.dirty()&&doc.redo(),"Recovered undo/redo failed");auto external=doc.approved();external.name="External change";writeUiJson(file,external.toJson());doc.poll();check(doc.conflict()&&!doc.save(),"External edit was silently overwritten");check(doc.project().styles.at("heading").size==76,"Conflict lost draft");check(doc.snapshot(temp/"milestone.json"),"Named version failed");check(doc.reload(true)&&doc.project().name=="External change","Explicit reload failed");std::ofstream(file)<<"{broken";doc.poll();check(doc.project().name=="External change"&&!doc.error().empty(),"Invalid file replaced last valid design");}
 std::filesystem::remove_all(temp);std::cout<<"PASS project migration, validation, variants, grouping, persistent undo, drafts, versions and conflicts\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
