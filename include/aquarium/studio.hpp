#pragma once
#include "aquarium/ui_renderer.hpp"
#include "imgui.h"
#include <set>

namespace aq::studio {
struct Options {
 std::filesystem::path assets{AQ_STUDIO_DEFAULT_ASSETS},project,capture,exportPath,report;
 std::string screen{"general-dialog"};int frames{},variant{},comparison{},scenario{};
 bool software{},init{},recovery{true},smoke{},flow{};
};
struct Device {const char* name;int w,h;};
inline constexpr std::array<Device,5> devices{{{"Desktop",1088,635},{"Wide phone",852,393},{"Small phone",667,375},{"Tablet",1024,768},{"Custom",1088,635}}};
inline constexpr ImVec4 accent{.31f,.80f,.73f,1},muted{.55f,.62f,.68f,1};
struct Stage {
 std::string screen,variant;int width{1088},height{635},density{2},scenario{},level{1},tank{1},upgrades{};
 Amount amount{240},coins{},pearls{};bool safe{},flow{},owned{true};double elapsed{2};
 bool operator==(const Stage&)const=default;
};
struct Preview {
 Session session;View view;Canvas& canvas;SDL_Texture* image{};
 Stage stage;UiRenderResult layout;std::uint64_t revision{},renders{};float logicalW{},logicalH{};bool invalid{true},initialized{};double started{};
 explicit Preview(Canvas&,const Content&);~Preview();
 void refresh(const UiProject&,std::uint64_t,const Stage&,bool play);
 void event(Uint32,float x,float y);void capture(const std::filesystem::path&);
};
struct Selection {std::string key,owner,id;bool operator==(const Selection&)const=default;};
class Studio {
 public:explicit Studio(Options);~Studio();int run();
 private:
 Options options_;UiDocument document_;Canvas canvas_;Content content_;
 std::array<std::unique_ptr<Preview>,4> previews_;
 std::string screen_,variant_,component_,status_{"Ready"};
 std::vector<Selection> selection_;std::vector<std::string> assets_;std::map<std::string,SDL_Texture*> thumbnails_;
 std::vector<std::filesystem::path> versions_;std::string versionName_,assetFilter_,componentFilter_;
 int device_{},density_{2},customW_{1088},customH_{635},comparison_{},scenario_{},bottomTab_{},leftTab_{};
 Amount amount_{240},coins_{},pearls_{};int level_{1},tank_{1},upgrades_{};
 bool selectTab_{true};
 bool play_{},flow_{},owned_{true},safe_{},guides_{true},snap_{true},override_{},bottom_{true},replaying_{},running_{true},autosaved_{},linkDraft_{true};
 float zoom_{1},elapsed_{2};ImVec2 pan_{};Uint64 replayAt_{},nextPoll_{},lastEdit_{},sentRevision_{};
 bool dragging_{},resizing_{},panning_{},marquee_{},inlineFocus_{};ImVec2 dragStart_{},panStart_{},marqueeStart_{};UiProject dragProject_;std::vector<UiPlaced> dragPlaced_;
 std::string inlineKey_,inlineBuffer_;Rect inlineRect_{};
 SDL_Process* previewProcess_{};std::string connectedRevision_,sentFingerprint_;std::filesystem::path livePath_;Stage sentStage_;
 std::string cachedFingerprint_;std::uint64_t fingerprintVersion_{},idleFrames_{},uiFrames_{};
 std::map<std::string,Rect> controls_;int smokeStep_{};std::vector<std::string> smokeChecks_;
 void draw();void toolbar();void navigation();void inspector();void workspace();void bottomPanel();void versionsPanel();void shortcuts();void bridge();
 void canvasTile(int,Preview&,const Stage&,bool approved,ImVec2 size);
 void manipulate(Preview&,Rect imageRect,bool hovered);void inlineEditor();void transformSelection(const std::string& operation);
 void inspectLayout(UiProject&,UiNode&,bool&);void inspectStyle(UiProject&,UiNode&,bool&);void inspectContent(UiProject&,UiNode&,bool&);void inspectBehavior(UiProject&,UiNode&,bool&);
 Stage stage(int index=0)const;UiProject previewProject(const UiProject&)const;
 const UiPlaced* placed(const Selection&)const;Selection pick(const UiPlaced&)const;
 void selectScreen(std::string);void selectComponent(std::string);void apply(UiProject,std::string group={});void editNode(const UiNode&,std::string group);
 UiNode editableNode(const UiProject&)const;void writeNode(UiProject&,const UiNode&)const;
 std::string owner()const;UiNode* ownerRoot(UiProject&)const;void addNode(std::string type,std::string component={},std::string asset={});
 void save();void snapshot();void loadVersions();void startGame();void stopGame();void remember(std::string key);void smoke();
 std::filesystem::path dataDirectory()const{return options_.assets.parent_path()/"local-data/studio";}
 std::string fingerprint();
};
bool editText(const char*,std::string&,bool multiline=false);bool editColor(const char*,DesignColor&);bool choice(const char*,std::string&,std::initializer_list<const char*>);
void label(const char*);void hint(const char*);Content loadContent(const std::filesystem::path&);
}
