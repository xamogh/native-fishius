#include "aquarium/ui_project.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace aq {
void to_json(UiJson& j,const DesignBox& b){j=UiJson::array({b.x,b.y,b.w,b.h});}
void from_json(const UiJson& j,DesignBox& b){auto v=j.get<std::array<float,4>>();b={v[0],v[1],v[2],v[3]};}
namespace {
void range(float v,float lo,float hi,std::string_view name){if(!std::isfinite(v)||v<lo||v>hi)throw std::runtime_error(std::string(name)+" is outside its allowed range");}
void assetPath(const std::string& name,const std::filesystem::path& assets){
 if(name.empty()||name.find('{')!=std::string::npos)return;
 const std::filesystem::path path(name);if(path.is_absolute()||path.extension()!=".png"||name.find('\\')!=std::string::npos)throw std::runtime_error("Choose a PNG inside assets");
 for(const auto& p:path)if(p=="..")throw std::runtime_error("Artwork must stay inside assets");
 if(!assets.empty()&&!std::filesystem::is_regular_file(assets/path))throw std::runtime_error("Missing artwork: "+name);
}
UiNode* find(UiNode& n,std::string_view id){if(n.id==id)return &n;for(auto& c:n.children)if(auto* found=find(c,id))return found;return nullptr;}
UiNode* root(UiProject& p,std::string_view owner){if(owner.starts_with("@")){auto i=p.components.find(std::string(owner.substr(1)));return i==p.components.end()?nullptr:&i->second;}for(auto& s:p.screens)if(s.id==owner)return &s.root;return nullptr;}
UiNode* parent(UiNode& n,std::string_view id){for(auto& c:n.children){if(c.id==id)return &n;if(auto* p=parent(c,id))return p;}return nullptr;}
UiJson read(const std::filesystem::path& path,std::size_t max=32*1024*1024){if(std::filesystem::file_size(path)>max)throw std::runtime_error("Design file is too large");std::ifstream file(path);return UiJson::parse(file);}
}
UiScreen& UiProject::screen(std::string_view id){for(auto& s:screens)if(s.id==id)return s;throw std::runtime_error("Unknown screen: "+std::string(id));}
const UiScreen& UiProject::screen(std::string_view id)const{return const_cast<UiProject*>(this)->screen(id);}
UiNode* UiProject::node(std::string_view owner,std::string_view id){auto* n=root(*this,owner);return n?find(*n,id):nullptr;}
const UiNode* UiProject::node(std::string_view owner,std::string_view id)const{return const_cast<UiProject*>(this)->node(owner,id);}
std::string UiProject::newId(std::string_view prefix){return std::string(prefix)+"-"+std::to_string(nextId++);}
UiJson UiProject::toJson()const{return {{"schemaVersion",2},{"name",name},{"nextId",nextId},{"styles",styles},{"components",components},{"screens",screens},{"dialogMotion",dialogMotion}};}
UiProject UiProject::fromJson(const UiJson& j){
 if(j.at("schemaVersion")!=2)throw std::runtime_error("Unsupported Studio project version");UiProject p;
 p.name=j.at("name");p.nextId=j.value("nextId",100);p.styles=j.at("styles").get<decltype(styles)>();p.components=j.at("components").get<decltype(components)>();p.screens=j.at("screens").get<decltype(screens)>();p.dialogMotion=j.value("dialogMotion",UiMotion{});p.validate();return p;
}
std::string uiFingerprint(const UiProject& p){std::uint64_t hash=1469598103934665603ULL;for(unsigned char c:p.toJson().dump()){hash^=c;hash*=1099511628211ULL;}std::ostringstream out;out<<std::hex<<hash;return out.str();}
std::string uiText(std::string value,const std::map<std::string,std::string>& bindings){
 for(int pass=0;pass<4;++pass){std::string next;std::size_t at=0;bool changed=false;
  while(at<value.size()){auto start=value.find('{',at);if(start==std::string::npos){next+=value.substr(at);break;}next+=value.substr(at,start-at);auto end=value.find('}',start);if(end==std::string::npos){next+=value.substr(start);break;}
   auto key=value.substr(start+1,end-start-1);auto found=bindings.find(key);if(found!=bindings.end()){next+=found->second;changed=true;}else next+=value.substr(start,end-start+1);at=end+1;
  }value=std::move(next);if(!changed)break;
 }return value;
}
UiStyle uiStyle(const UiProject& p,const UiNode& n){auto style=p.styles.at(n.style);UiJson value=style;value.merge_patch(n.styleOverrides);return value.get<UiStyle>();}
void UiProject::validate(const std::filesystem::path& assets)const{
 if(styles.empty()||styles.size()>128||components.size()>128||screens.empty()||screens.size()>64||nextId<1)throw std::runtime_error("Invalid project size");
 auto color=[](const DesignColor& c){for(int v:c)if(v<0||v>255)throw std::runtime_error("Invalid color");};
 auto style=[&](const UiStyle& s){if(s.align!="left"&&s.align!="center"&&s.align!="right")throw std::runtime_error("Unknown text alignment");if(s.font!="baloo"&&s.font!="nunito"&&s.font!="lilita")throw std::runtime_error("Unknown font");range(s.size,8,160,"Font size");range(s.accentSize,8,180,"Accent size");range(s.stretch,.4f,1.6f,"Text width");range(s.lineHeight,.8f,3,"Line height");range(s.outlineWidth,0,8,"Outline");range(s.shadowX,-30,30,"Shadow X");range(s.shadowY,-30,30,"Shadow Y");range(s.pressedScale,.7f,1,"Pressed scale");range(s.disabledAlpha,.1f,1,"Disabled opacity");range(s.padding,0,200,"Style padding");range(s.gap,0,200,"Style gap");color(s.color);color(s.accent);color(s.alert);color(s.shadow);color(s.outline);};
 for(const auto& [_,s]:styles)style(s);
 std::size_t total=0;
 const auto validateTree=[&](const UiNode& tree){std::set<std::string> ids;
  std::function<void(const UiNode&,int)> visit=[&](const UiNode& n,int depth){
   if(depth>16||++total>2048||n.id.empty()||n.id.size()>120||!ids.insert(n.id).second)throw std::runtime_error("Duplicate node ID or too many nested layers");
   if(n.type!="group"&&n.type!="image"&&n.type!="text"&&n.type!="richtext"&&n.type!="button"&&n.type!="instance")throw std::runtime_error("Unknown layer type");
   if(!styles.contains(n.style))throw std::runtime_error("Missing style: "+n.style);if(n.type=="instance"&&!components.contains(n.component))throw std::runtime_error("Missing component: "+n.component);
   range(n.box.x,-4000,4000,"X");range(n.box.y,-4000,4000,"Y");range(n.box.w,1,4000,"Width");range(n.box.h,1,4000,"Height");range(n.slice,0,500,"Slice");range(n.rotation,-180,180,"Rotation");
   if(n.layout.mode!="absolute"&&n.layout.mode!="row"&&n.layout.mode!="column")throw std::runtime_error("Unknown layout mode");
   if(n.layout.horizontal!="left"&&n.layout.horizontal!="center"&&n.layout.horizontal!="center-alone"&&n.layout.horizontal!="right"&&n.layout.horizontal!="stretch")throw std::runtime_error("Unknown horizontal anchor");
   if(n.layout.vertical!="top"&&n.layout.vertical!="center"&&n.layout.vertical!="bottom")throw std::runtime_error("Unknown vertical anchor");
   if(n.layout.width!="fixed"&&n.layout.width!="fill"&&n.layout.width!="hug")throw std::runtime_error("Unknown width rule");if(n.layout.height!="fixed"&&n.layout.height!="hug")throw std::runtime_error("Unknown height rule");
   range(n.layout.gap,-1,300,"Gap");range(n.layout.padding,-1,300,"Padding");if(n.action!=""&&n.action!="primary"&&n.action!="close"&&n.action!="coins"&&n.action!="pearls"&&n.action!="buyCoins"&&n.action!="buyPearls"&&n.action!="coinBalance"&&n.action!="pearlBalance")throw std::runtime_error("Unknown UI action");if(n.text.size()>1024)throw std::runtime_error("Text exceeds 1024 bytes");assetPath(n.asset,assets);style(uiStyle(*this,n));
   std::map<std::string,std::string> after;for(const auto& c:n.children)after[c.id]=c.layout.after;
   for(const auto& [id,target]:after){std::set<std::string> seen{id};auto next=target;while(!next.empty()){if(!after.contains(next)||!seen.insert(next).second)throw std::runtime_error("Invalid or circular 'below' rule");next=after.at(next);}}
   for(const auto& c:n.children)visit(c,depth+1);
  };visit(tree,0);
 };
 for(const auto& [_,n]:components)validateTree(n);
 std::set<std::string> screenIds;for(const auto& s:screens){if(!screenIds.insert(s.id).second||s.id.empty())throw std::runtime_error("Duplicate screen");range(s.maxWidth,100,2400,"Screen width");range(s.safeFraction,.3f,1,"Safe area");validateTree(s.root);
  for(const auto& [_,v]:s.variants){for(const auto& [key,value]:v.values){if(value.size()>1024)throw std::runtime_error("Variant text too long");if(key=="illustration"||key=="art"||key=="cardArt")assetPath(value,assets);}for(const auto& [key,patch]:v.overrides){if(!patch.is_object()||patch.contains("id")||patch.contains("children")||patch.contains("component")||patch.contains("type"))throw std::runtime_error("Invalid variant override");if(patch.contains("asset"))assetPath(patch.at("asset"),assets);}}
 }
 std::function<void(const UiNode&,std::set<std::string>,int)> checkReferences=[&](const UiNode& n,std::set<std::string> stack,int depth){if(depth>16)throw std::runtime_error("Components are nested too deeply");if(n.type=="instance"){if(!stack.insert(n.component).second)throw std::runtime_error("Circular component reference");checkReferences(components.at(n.component),stack,depth+1);}for(const auto& c:n.children)checkReferences(c,stack,depth+1);};
 for(const auto& [id,n]:components)checkReferences(n,{id},0);
 for(const auto* id:{"general-dialog","shop-card","tank-upgrade","currency-hud"})if(!screenIds.contains(id))throw std::runtime_error("Missing integrated screen: "+std::string(id));
 // Validate every effective variant, including nested component overrides.
 // Invalid files never replace the last valid game design.
 for(const auto& screen:screens)for(const auto& [_,variant]:screen.variants){
  std::set<std::string> matched;std::size_t expanded=0;
  std::function<UiNode(const UiNode&,std::string,std::string,int)> resolve=[&](const UiNode& original,std::string owner,std::string chain,int depth){
   if(depth>24||++expanded>3000)throw std::runtime_error("Variant has too many expanded layers");
   UiNode n=original;const auto key=chain+"|"+owner+"|"+n.id;
   if(auto patch=variant.overrides.find(key);patch!=variant.overrides.end()){UiJson value=n;value.merge_patch(patch->second);n=value.get<UiNode>();matched.insert(key);}
   for(auto& child:n.children)child=resolve(child,owner,chain,depth+1);
   if(n.type=="instance"){
    if(!components.contains(n.component))throw std::runtime_error("Missing variant component");
    auto nested=resolve(components.at(n.component),"@"+n.component,chain.empty()?n.id:chain+"/"+n.id,depth+1);total=0;validateTree(nested);
   }
   return n;
  };
  auto effective=resolve(screen.root,screen.id,"",0);total=0;validateTree(effective);
  if(matched.size()!=variant.overrides.size())throw std::runtime_error("Variant override refers to a missing layer");
 }
 range(dialogMotion.duration,.1f,2,"Motion duration");range(dialogMotion.damping,4,35,"Motion damping");range(dialogMotion.frequency,1,40,"Motion frequency");range(dialogMotion.scale,.5f,1,"Starting scale");range(dialogMotion.rise,-100,150,"Motion offset");
}
void uiPruneOverrides(UiProject& p){
 for(auto& s:p.screens){std::set<std::string> keys;
  std::function<void(const UiNode&,std::string,std::string,int)> visit=[&](const UiNode& n,std::string owner,std::string chain,int depth){if(depth>24)return;keys.insert(chain+"|"+owner+"|"+n.id);if(n.type=="instance"&&p.components.contains(n.component))visit(p.components.at(n.component),"@"+n.component,chain.empty()?n.id:chain+"/"+n.id,depth+1);for(const auto& c:n.children)visit(c,owner,chain,depth+1);};
  visit(s.root,s.id,"",0);for(auto& [_,v]:s.variants)std::erase_if(v.overrides,[&](const auto& entry){return !keys.contains(entry.first);});
 }
}
void writeUiJson(const std::filesystem::path& path,const UiJson& json){if(path.has_parent_path())std::filesystem::create_directories(path.parent_path());auto temporary=std::filesystem::path(path.string()+".tmp");{std::ofstream out(temporary,std::ios::binary|std::ios::trunc);out<<json.dump(2)<<'\n';out.flush();if(!out)throw std::runtime_error("Cannot write "+path.filename().string());}std::filesystem::rename(temporary,path);}
UiDocument::UiDocument(std::filesystem::path file,std::filesystem::path assets,std::filesystem::path recovery):path_(std::move(file)),assets_(std::move(assets)),recovery_(std::move(recovery)){
 DialogDocument legacy(assets_/"ui/general-dialog.json");current_=approved_=UiProject::defaults(legacy.design());history_={current_};reload();
 if(!recovery_.empty()&&std::filesystem::exists(recovery_))try{auto j=read(recovery_);auto h=j.at("history").get<std::vector<UiJson>>();if(h.empty()||h.size()>64)throw std::runtime_error("Invalid draft history");std::vector<UiProject> restored;for(const auto& item:h){auto p=UiProject::fromJson(item);p.validate(assets_);restored.push_back(std::move(p));}
  auto at=j.at("cursor").get<std::size_t>();if(at>=restored.size())throw std::runtime_error("Invalid draft cursor");conflict_=j.at("base")!=uiFingerprint(approved_);history_=std::move(restored);cursor_=at;current_=history_[at];recovered_=dirty();++version_;if(conflict_)error_="The saved design changed. Your draft is recovered; compare it before saving.";
 }catch(const std::exception& e){error_=std::string("Draft recovery: ")+e.what();}
}
bool UiDocument::apply(UiProject next,std::string group){try{next.validate(assets_);if(next==current_)return false;history_.resize(cursor_+1);if(!group.empty()&&group==group_&&cursor_>0)history_[cursor_]=next;else{history_.push_back(next);++cursor_;if(history_.size()>64){history_.erase(history_.begin());--cursor_;}}current_=std::move(next);group_=std::move(group);++version_;if(!conflict_)error_.clear();return true;}catch(const std::exception& e){error_=e.what();return false;}}
bool UiDocument::undo(){endEdit();if(!canUndo())return false;current_=history_[--cursor_];++version_;return true;}
bool UiDocument::redo(){endEdit();if(!canRedo())return false;current_=history_[++cursor_];++version_;return true;}
bool UiDocument::reload(bool discard){try{
 std::error_code e;auto stamp=std::filesystem::last_write_time(path_,e);if(e)return false;modified_=stamp;
 auto json=read(path_,2*1024*1024);auto next=UiProject::fromJson(json);next.validate(assets_);auto preview=json.value("preview",UiJson::object());if(!preview.is_object()||preview.dump().size()>8192)throw std::runtime_error("Invalid preview settings");preview_=preview;available_=true;
 if(dirty()&&!discard&&next!=approved_){approved_=std::move(next);conflict_=true;error_="Saved design changed. Compare, save a version, or reload.";return false;}
 if(next==approved_&&!discard){if(!conflict_)error_.clear();return false;}current_=approved_=std::move(next);history_={current_};cursor_=0;conflict_=false;error_.clear();++version_;return true;
 }catch(const std::exception& e){error_=e.what();return false;}}
bool UiDocument::poll(){std::error_code e;auto stamp=std::filesystem::last_write_time(path_,e);return !e&&stamp!=modified_?reload():false;}
bool UiDocument::save(){try{poll();if(conflict_)return false;current_.validate(assets_);writeUiJson(path_,current_.toJson());modified_=std::filesystem::last_write_time(path_);approved_=current_;available_=true;error_.clear();++version_;autosave();return true;}catch(const std::exception& e){error_=e.what();return false;}}
bool UiDocument::autosave(){if(recovery_.empty()||persisted_==version_)return true;try{UiJson history=UiJson::array();for(const auto& item:history_)history.push_back(item.toJson());writeUiJson(recovery_,{{"base",uiFingerprint(approved_)},{"cursor",cursor_},{"history",history}});persisted_=version_;return true;}catch(const std::exception& e){error_=e.what();return false;}}
bool UiDocument::snapshot(const std::filesystem::path& file)const{try{writeUiJson(file,current_.toJson());return true;}catch(...){return false;}}
std::vector<UiNode*> uiChildren(UiNode& n){std::vector<UiNode*> result;for(auto& c:n.children){result.push_back(&c);auto sub=uiChildren(c);result.insert(result.end(),sub.begin(),sub.end());}return result;}
bool uiGroup(UiProject& p,std::string_view owner,const std::set<std::string>& ids){if(ids.size()<2)return false;auto* r=root(p,owner);if(!r)return false;auto* par=parent(*r,*ids.begin());if(!par)return false;for(const auto& id:ids)if(parent(*r,id)!=par)return false;
 DesignBox b{4000,4000,0,0};float right=-4000,bottom=-4000;for(const auto& n:par->children)if(ids.contains(n.id)){b.x=std::min(b.x,n.box.x);b.y=std::min(b.y,n.box.y);right=std::max(right,n.box.x+n.box.w);bottom=std::max(bottom,n.box.y+n.box.h);}b.w=right-b.x;b.h=bottom-b.y;
 UiNode group;group.id=p.newId("group");group.name="Group";group.box=b;for(auto it=par->children.begin();it!=par->children.end();)if(ids.contains(it->id)){auto child=*it;child.box.x-=b.x;child.box.y-=b.y;child.layout.after.clear();group.children.push_back(child);it=par->children.erase(it);}else ++it;par->children.push_back(std::move(group));return true;
}
bool uiUngroup(UiProject& p,std::string_view owner,std::string_view id){auto* r=root(p,owner);if(!r)return false;auto* par=parent(*r,id);if(!par)return false;auto it=std::find_if(par->children.begin(),par->children.end(),[&](const auto& n){return n.id==id;});if(it==par->children.end()||it->type!="group")return false;auto group=*it;auto offset=it-par->children.begin();par->children.erase(it);for(auto child:group.children){child.box.x+=group.box.x;child.box.y+=group.box.y;par->children.insert(par->children.begin()+offset++,std::move(child));}return true;}
bool uiDelete(UiProject& p,std::string_view owner,const std::set<std::string>& ids){auto* r=root(p,owner);if(!r)return false;bool changed=false;std::function<void(UiNode&)> erase=[&](UiNode& n){auto count=n.children.size();std::erase_if(n.children,[&](const auto& c){return ids.contains(c.id);});changed|=count!=n.children.size();for(auto& c:n.children){if(ids.contains(c.layout.after))c.layout.after.clear();erase(c);}};erase(*r);return changed;}
bool uiDuplicate(UiProject& p,std::string_view owner,const std::set<std::string>& ids){auto* r=root(p,owner);if(!r)return false;bool changed=false;for(const auto& id:ids){auto* par=parent(*r,id);if(!par)continue;auto* source=find(*r,id);auto copy=*source;std::function<void(UiNode&)> rename=[&](UiNode& n){n.id=p.newId("copy");n.layout.after.clear();for(auto& c:n.children)rename(c);};rename(copy);copy.name+=" copy";copy.box.x+=16;copy.box.y+=16;par->children.push_back(std::move(copy));changed=true;}return changed;}
}
