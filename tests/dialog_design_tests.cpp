#include "aquarium/dialog_design.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <chrono>

namespace {
using namespace aq;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F run,const char* message){bool rejected=false;try{run();}catch(const std::exception&){rejected=true;}check(rejected,message);}
struct Temporary {
 std::filesystem::path path=std::filesystem::temp_directory_path()/("aquarium-design-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 Temporary(){std::filesystem::create_directories(path/"ui");}
 ~Temporary(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
}
int main(){try{
 const auto defaults=DialogDesign::defaults();check(DialogDesign::fromJson(defaults.toJson())==defaults,"Default design does not round trip");
 auto data=defaults.toJson();data["schemaVersion"]=2;rejects([&]{DialogDesign::fromJson(data);},"Unknown schema accepted");
 data=defaults.toJson();data["elements"].erase("action");rejects([&]{DialogDesign::fromJson(data);},"Missing button accepted");
 auto bad=defaults;bad.element(DialogPart::Title).font="unknown";rejects([&]{bad.validate();},"Unknown font accepted");
 bad=defaults;bad.maxWidth=std::numeric_limits<float>::quiet_NaN();rejects([&]{bad.validate();},"NaN accepted");
 bad=defaults;bad.element(DialogPart::Action).box.w=0;rejects([&]{bad.validate();},"Invisible button accepted");
 bad=defaults;bad.coins.illustration="../private.png";rejects([&]{bad.validate();},"Artwork traversal accepted");
 Temporary temporary;const auto path=temporary.path/"ui/general-dialog.json";
 auto makeAsset=[&](const std::string& asset){if(!asset.empty()){std::filesystem::create_directories((temporary.path/asset).parent_path());std::ofstream(temporary.path/asset)<<"test asset";}};
 for(const auto& e:defaults.elements)makeAsset(e.asset);makeAsset(defaults.shopArtwork);makeAsset(defaults.coins.illustration);makeAsset(defaults.pearls.illustration);
 DialogDocument writer(path);check(writer.save(),"Cannot save initial design");DialogDocument reader(path);
 auto missingArt=defaults;missingArt.coins.illustration="general-dialog/missing.png";
 check(!writer.apply(missingArt)&&writer.design()==defaults,"Missing artwork entered the live document");
 auto edited=defaults;edited.element(DialogPart::Action).box.x+=12;writer.apply(edited,"drag");edited.element(DialogPart::Action).box.x+=12;writer.apply(edited,"drag");writer.endEdit();
 check(writer.undo()&&writer.design()==defaults,"Drag did not undo as one edit");check(writer.redo()&&writer.design()==edited,"Redo lost final drag");
 check(writer.save()&&reader.poll()&&reader.design()==edited,"Saved edits did not hot reload");
 auto branched=edited;branched.coins.title="More coins, please";writer.apply(branched);writer.undo();auto alternate=edited;alternate.pearls.title="More pearls, please";writer.apply(alternate);
 check(!writer.canRedo(),"Old redo branch survived a new edit");writer.undo();check(!writer.dirty(),"Undo to saved design remains dirty");
 {std::ofstream(path)<<"{invalid";}check(!reader.poll()&&reader.design()==edited&&!reader.error().empty(),"Malformed file replaced the last valid design");
 check(writer.save(),"Cannot restore the last good design");
 writer.apply(branched);check(writer.save(),"Cannot save corrected design");reader.reload(true);
 reader.apply(alternate);writer.apply(edited);check(writer.save(),"Cannot save external edit");
 check(!reader.poll()&&reader.conflict()&&!reader.save(),"External update overwrote unsaved work");
 check(reader.reload(true)&&reader.design()==edited&&!reader.dirty(),"Explicit reload did not resolve conflict");
 for(auto size:std::array<std::array<float,2>,4>{{{1088,635},{852,393},{667,375},{1024,768}}}){
  const float w=std::max(1608.f,830.f*size[0]/size[1]),h=w*size[1]/size[0],units=w/size[0];
  const auto layout=layoutDialog(defaults,{0,0,w,h},units);
  const float u=std::min({620*units/888,w*.92f/888,h*.92f/732});
  const auto close=layout.element(DialogPart::Close);
  check(std::abs(close.x-((w-888*u)*.5f+764*u))<.001f&&std::abs(close.y-((h-732*u)*.5f+43*u))<.001f,"Clay changed reference positions");
  const auto moved=layoutDialog(edited,{0,0,w,h},units).element(DialogPart::Action);
  check(std::abs(moved.x-layout.element(DialogPart::Action).x-24*u)<.001f,"Edited position did not reach the layout");
 }
 std::cout<<"PASS design validation, save/reload, last valid fallback, conflicts, undo/redo and reference geometry\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
