#include "aquarium/dialog_design.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace aq {
namespace {
using Json=nlohmann::json;
void finiteRange(float value,float low,float high,const std::string& name){
 if(!std::isfinite(value)||value<low||value>high)throw std::runtime_error(name+" must be between "+std::to_string(low)+" and "+std::to_string(high));
}
void validAsset(const std::string& asset){
 if(asset.empty())return;
 const std::filesystem::path path(asset);
 if(path.is_absolute()||asset.find('\\')!=std::string::npos||path.extension()!=".png")throw std::runtime_error("Artwork must be a PNG path inside assets");
 for(const auto& part:path)if(part=="..")throw std::runtime_error("Artwork must stay inside assets");
}
void validText(const std::string& value){if(value.size()>1024||value.find('\0')!=std::string::npos)throw std::runtime_error("Text is too long or contains a null character");}
void validArtwork(const DialogDesign& design,const std::filesystem::path& assets){
 const auto exists=[&](const std::string& asset){if(!asset.empty()&&!std::filesystem::is_regular_file(assets/asset))throw std::runtime_error("Missing artwork: "+asset);};
 for(const auto& e:design.elements)exists(e.asset);
 exists(design.shopArtwork);exists(design.coins.illustration);exists(design.pearls.illustration);
}
Json variantJson(const DialogVariant& v){return {{"title",v.title},{"illustration",v.illustration},{"detail",v.detail},{"actionLabel",v.actionLabel},{"prefix",v.prefix},{"singularSuffix",v.singularSuffix},{"pluralSuffix",v.pluralSuffix}};}
DialogVariant readVariant(const Json& j){return {j.at("title"),j.at("illustration"),j.at("detail"),j.at("actionLabel"),j.at("prefix"),j.at("singularSuffix"),j.at("pluralSuffix")};}
}
DialogDesign DialogDesign::defaults(){
 DialogDesign d;
 d.elements={{
  {{0,0,888,732},"baloo",38,1,{4,47,80,255},"general-dialog/frame.png"},
  {{176.5f,3,545,100},"baloo",68,.89f,{4,47,80,255},""},
  {{100,123,696,284},"baloo",38,1,{4,47,80,255},""},
  {{107,392,674,86},"baloo",51,.94f,{4,47,80,255},""},
  {{116,458,656,48},"baloo",32,1,{4,47,80,255},""},
  {{109,466,670,54},"baloo",38,.88f,{76,130,158,255},""},
  {{119,543,652,135},"lilita",54,1,{255,255,245,255},"general-dialog/button.png"},
  {{764,43,100,100},"baloo",38,1,{4,47,80,255},"general-dialog/close.png"}
 }};
 d.coins={"Not Enough Coins","general-dialog/coins.png","Get more coins from the shop!","Open Shop","You need "," more coin"," more coins"};
 d.pearls={"Not Enough Pearls","general-dialog/pearls.png","Get more pearls from the shop!","Open Shop","You need "," more pearl"," more pearls"};
 return d;
}
Json DialogDesign::toJson()const{
 Json nodes=Json::object();
 for(std::size_t i=0;i<elements.size();++i){const auto& e=elements[i];nodes[dialogPartIds[i]]={{"box",{e.box.x,e.box.y,e.box.w,e.box.h}},{"font",e.font},{"fontSize",e.fontSize},{"stretch",e.stretch},{"color",e.color},{"asset",e.asset}};}
 return {{"schemaVersion",1},{"maxWidth",maxWidth},{"safeFraction",safeFraction},{"elements",nodes},
  {"emphasisSize",emphasisSize},{"emphasisColor",emphasisColor},{"detailExtraOffset",detailExtraOffset},{"detailExtraSize",detailExtraSize},
  {"useShopArtwork",useShopArtwork},{"shopArtwork",shopArtwork},{"variants",{{"coins",variantJson(coins)},{"pearls",variantJson(pearls)}}}};
}
DialogDesign DialogDesign::fromJson(const Json& j){
 if(j.at("schemaVersion")!=1)throw std::runtime_error("Unsupported dialog design version");
 auto d=defaults();d.maxWidth=j.at("maxWidth");d.safeFraction=j.at("safeFraction");
 for(std::size_t i=0;i<d.elements.size();++i){const auto& node=j.at("elements").at(dialogPartIds[i]);auto& e=d.elements[i];
  const auto box=node.at("box").get<std::array<float,4>>();e.box={box[0],box[1],box[2],box[3]};
  e.font=node.at("font");e.fontSize=node.at("fontSize");e.stretch=node.at("stretch");e.color=node.at("color").get<DesignColor>();e.asset=node.at("asset");
 }
 d.emphasisSize=j.at("emphasisSize");d.emphasisColor=j.at("emphasisColor").get<DesignColor>();d.detailExtraOffset=j.at("detailExtraOffset");d.detailExtraSize=j.at("detailExtraSize");
 d.useShopArtwork=j.at("useShopArtwork");d.shopArtwork=j.at("shopArtwork");d.coins=readVariant(j.at("variants").at("coins"));d.pearls=readVariant(j.at("variants").at("pearls"));d.validate();return d;
}
void DialogDesign::validate()const{
 finiteRange(maxWidth,240,1600,"Dialog width");finiteRange(safeFraction,.4f,1,"Safe area coverage");
 finiteRange(emphasisSize,12,140,"Amount size");finiteRange(detailExtraOffset,-100,150,"Detail offset");finiteRange(detailExtraSize,12,100,"Compact detail size");
 auto color=[](DesignColor c){for(int channel:c)if(channel<0||channel>255)throw std::runtime_error("Color channels must be between 0 and 255");};color(emphasisColor);
 for(std::size_t i=0;i<elements.size();++i){const auto& e=elements[i];const std::string name=dialogPartIds[i];
  finiteRange(e.box.x,-888,1776,name+" X");finiteRange(e.box.y,-732,1464,name+" Y");finiteRange(e.box.w,8,1776,name+" width");finiteRange(e.box.h,8,1464,name+" height");
  finiteRange(e.fontSize,12,140,name+" font size");finiteRange(e.stretch,.5f,1.5f,name+" text width");color(e.color);validAsset(e.asset);
  if(e.font!="baloo"&&e.font!="nunito"&&e.font!="lilita")throw std::runtime_error("Unknown font: "+e.font);
 }
 validAsset(shopArtwork);
 for(const auto* v:{&coins,&pearls}){validAsset(v->illustration);for(const auto* text:{&v->title,&v->detail,&v->actionLabel,&v->prefix,&v->singularSuffix,&v->pluralSuffix})validText(*text);}
}
DialogDocument::DialogDocument(std::filesystem::path path):path_(std::move(path)),current_(DialogDesign::defaults()),saved_(current_),history_{current_}{reload();}
bool DialogDocument::apply(DialogDesign next,std::string group){
 try{next.validate();validArtwork(next,path_.parent_path().parent_path());}catch(const std::exception& e){error_=e.what();return false;}
 if(next==current_)return false;
 history_.resize(cursor_+1);
 if(!group.empty()&&group==group_&&cursor_>0)history_[cursor_]=next;
 else{history_.push_back(next);++cursor_;if(history_.size()>128){history_.erase(history_.begin());--cursor_;}}
 current_=std::move(next);group_=std::move(group);if(!conflict_)error_.clear();return true;
}
bool DialogDocument::undo(){endEdit();if(!canUndo())return false;current_=history_[--cursor_];return true;}
bool DialogDocument::redo(){endEdit();if(!canRedo())return false;current_=history_[++cursor_];return true;}
bool DialogDocument::reload(bool discardEdits){
 try{
  std::error_code ec;const auto stamp=std::filesystem::last_write_time(path_,ec);
  if(ec){if(std::filesystem::exists(path_))throw std::runtime_error("Cannot read design file");return false;}
  modified_=stamp;
  if(dirty()&&!discardEdits){conflict_=true;throw std::runtime_error("Design changed on disk. Save a snapshot of your edits, then reload.");}
  if(std::filesystem::file_size(path_)>128*1024)throw std::runtime_error("Design file exceeds 128 KB");
  std::ifstream stream(path_);auto next=DialogDesign::fromJson(Json::parse(stream));
  validArtwork(next,path_.parent_path().parent_path());
  current_=saved_=std::move(next);history_={current_};cursor_=0;group_.clear();error_.clear();conflict_=false;return true;
 }catch(const std::exception& e){error_=e.what();return false;}
}
bool DialogDocument::poll(){std::error_code ec;const auto stamp=std::filesystem::last_write_time(path_,ec);return !ec&&stamp!=modified_?reload():false;}
bool DialogDocument::save(){
 try{
  endEdit();poll();if(conflict_)return false;current_.validate();validArtwork(current_,path_.parent_path().parent_path());
  std::filesystem::create_directories(path_.parent_path());
  const auto temporary=std::filesystem::path(path_.string()+".tmp");
  {std::ofstream stream(temporary,std::ios::trunc|std::ios::binary);if(!stream)throw std::runtime_error("Cannot write design file");stream<<current_.toJson().dump(2)<<'\n';stream.flush();if(!stream)throw std::runtime_error("Cannot finish writing design file");}
  std::filesystem::rename(temporary,path_);modified_=std::filesystem::last_write_time(path_);saved_=current_;error_.clear();return true;
 }catch(const std::exception& e){error_=e.what();return false;}
}
}
