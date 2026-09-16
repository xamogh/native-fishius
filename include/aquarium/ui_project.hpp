#pragma once
#include "aquarium/dialog_design.hpp"
#include <functional>
#include <map>
#include <optional>
#include <set>

namespace aq {
using UiJson=nlohmann::json;
struct UiRun {std::string text;bool emphasis{};};
struct UiBindings {std::map<std::string,std::string> values;std::map<std::string,std::vector<UiRun>> rich;};
struct UiStyle {
 std::string font{"baloo"},align{"center"};float size{38},stretch{1},lineHeight{1.18f};
 DesignColor color{4,47,80,255},accent{231,82,19,255},alert{255,83,83,255},shadow{0,0,0,0},outline{17,106,72,255};
 float accentSize{51},shadowX{},shadowY{2},outlineWidth{},padding{},gap{12},pressedScale{.94f},disabledAlpha{.45f};
 bool operator==(const UiStyle&)const=default;
};
struct UiLayout {
 std::string mode{"absolute"},horizontal{"left"},vertical{"top"},width{"fixed"},height{"fixed"},after;
 float gap{-1},padding{-1};bool wrap{};
 bool operator==(const UiLayout&)const=default;
};
struct UiNode {
 std::string id,name,type{"group"},component,style{"body"},text,asset,action,visibleWhen,enabledWhen,alertWhen;
 DesignBox box{0,0,240,100};UiLayout layout;UiJson styleOverrides=UiJson::object();
 std::vector<UiNode> children;bool hidden{},locked{};float slice{},rotation{};
 bool operator==(const UiNode&)const=default;
};
struct UiVariant {
 std::map<std::string,std::string> values;
 std::map<std::string,UiJson> overrides;
 bool operator==(const UiVariant&)const=default;
};
struct UiScreen {
 std::string id,name;UiNode root;float maxWidth{620},safeFraction{.92f};
 std::map<std::string,UiVariant> variants;
 bool operator==(const UiScreen&)const=default;
};
struct UiMotion {
 float duration{.6f},damping{13},frequency{19},scale{.84f},rise{24};
 bool operator==(const UiMotion&)const=default;
};
struct UiProject {
 std::string name{"Aquarium"};int nextId{100};
 std::map<std::string,UiStyle> styles;
 std::map<std::string,UiNode> components;
 std::vector<UiScreen> screens;UiMotion dialogMotion;
 static UiProject defaults(const DialogDesign& legacy=DialogDesign::defaults());
 static UiProject fromJson(const UiJson&);UiJson toJson()const;void validate(const std::filesystem::path& assets={})const;
 UiScreen& screen(std::string_view);const UiScreen& screen(std::string_view)const;
 UiNode* node(std::string_view owner,std::string_view id);const UiNode* node(std::string_view owner,std::string_view id)const;
 std::string newId(std::string_view prefix="node");
 bool operator==(const UiProject&)const=default;
};
void to_json(UiJson&,const DesignBox&);void from_json(const UiJson&,DesignBox&);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UiStyle,font,align,size,stretch,lineHeight,color,accent,alert,shadow,outline,accentSize,shadowX,shadowY,outlineWidth,padding,gap,pressedScale,disabledAlpha)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UiLayout,mode,horizontal,vertical,width,height,after,gap,padding,wrap)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UiNode,id,name,type,component,style,text,asset,action,visibleWhen,enabledWhen,alertWhen,box,layout,styleOverrides,children,hidden,locked,slice,rotation)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UiVariant,values,overrides)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UiScreen,id,name,root,maxWidth,safeFraction,variants)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UiMotion,duration,damping,frequency,scale,rise)
std::string uiFingerprint(const UiProject&);
std::string uiText(std::string value,const std::map<std::string,std::string>& bindings);
UiStyle uiStyle(const UiProject&,const UiNode&);

// The document owns validation, conflict detection, persistent history, drafts
// and approved saves. Both UI gestures and tests edit through apply().
class UiDocument {
 public:
 UiDocument(std::filesystem::path file,std::filesystem::path assets,std::filesystem::path recovery={});
 const UiProject& project()const{return current_;}const UiProject& approved()const{return approved_;}
 const std::filesystem::path& path()const{return path_;}
 const std::string& error()const{return error_;}bool dirty()const{return current_!=approved_;}
 const UiJson& preview()const{return preview_;}
 bool available()const{return available_;}bool conflict()const{return conflict_;}bool recovered()const{return recovered_;}
 bool canUndo()const{return cursor_>0;}bool canRedo()const{return cursor_+1<history_.size();}
 std::uint64_t version()const{return version_;}
 bool apply(UiProject,std::string group={});void endEdit(){group_.clear();}
 bool undo();bool redo();bool save();bool reload(bool discard=false);bool poll();bool autosave();
 bool snapshot(const std::filesystem::path&)const;
 private:
 UiJson preview_=UiJson::object();
 std::filesystem::path path_,assets_,recovery_;UiProject current_,approved_;
 std::vector<UiProject> history_;std::size_t cursor_{};std::uint64_t version_{1},persisted_{};
 std::string error_,group_;bool available_{},conflict_{},recovered_{};
 std::filesystem::file_time_type modified_{};
};
void uiPruneOverrides(UiProject&);
void writeUiJson(const std::filesystem::path&,const UiJson&);
// Editing helpers preserve node IDs and group local coordinates.
std::vector<UiNode*> uiChildren(UiNode&);
bool uiGroup(UiProject&,std::string_view owner,const std::set<std::string>& ids);
bool uiUngroup(UiProject&,std::string_view owner,std::string_view id);
bool uiDelete(UiProject&,std::string_view owner,const std::set<std::string>& ids);
bool uiDuplicate(UiProject&,std::string_view owner,const std::set<std::string>& ids);
}
