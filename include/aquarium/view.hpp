#pragma once
#include "aquarium/storage.hpp"
#include "aquarium/dialog_design.hpp"
#include "aquarium/ui_project.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <array>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>
namespace aq {
struct Rect {float x{},y{},w{},h{};bool has(float px,float py)const{return px>=x&&py>=y&&px<=x+w&&py<=y+h;}};
struct Insets {float left{},top{},right{},bottom{};};
struct Color {Uint8 r{},g{},b{},a{255};};
inline constexpr Color insufficientFundsColor{255,83,83,255};
using PreparationProgress=std::function<bool(float,std::string_view)>;
struct Texture {
 SDL_Texture* value{};int width{},height{};std::vector<SDL_Texture*> levels;
 ~Texture(){for(auto* level:levels)SDL_DestroyTexture(level);if(value)SDL_DestroyTexture(value);}
 Texture()=default;Texture(const Texture&)=delete;Texture& operator=(const Texture&)=delete;
 SDL_Texture* sampled(float pixelWidth,float pixelHeight)const;
 std::size_t bytes()const;
};
enum class CursorKind {Arrow,Move,Stash,Hidden};
struct ToolCursorPose {double angle{};float scale{1};};
ToolCursorPose toolCursorPose(Tool,double elapsed,bool reduced=false);
struct FishPose {WorldPoint position;double phase{},pitch{},facing{},speed{};};
struct DecorVertexPose {double x{},y{},light{1};};
DecorVertexPose decorVertexPose(const DecorDef&,std::uint64_t copy,double time,double u,double v,bool animate);
FishPose fishPose(const Fish&,double interpolation);
// Sample by presentation time so motion stays consistent at any frame rate.
struct MenuMotion {
 struct Sample {double value{},velocity{};};
 bool visible{};double changed{-10},from{},velocity{};double openingDuration{.58},openingDamping{13},openingFrequency{19};
 Sample sample(double now)const;
 void show(bool,double now);
 void settle(){from=visible?1:0;velocity=0;changed=-10;}
};
struct MenuPose {
 SDL_FPoint anchor{};float xScale{1},yScale{1},rise{},alpha{1};
 Rect apply(Rect)const;
 SDL_FPoint unapply(SDL_FPoint)const;
};
struct PreviewViewport {int width{1088},height{635},pixelRatio{1};Insets safe;};
class Canvas {
 friend struct ViewTestAccess;
 public:
 Canvas(std::filesystem::path assets,int width,int height,bool software);~Canvas();
 Canvas(const Canvas&)=delete;Canvas& operator=(const Canvas&)=delete;
 SDL_Window* window()const{return window_;}SDL_Renderer* renderer()const{return renderer_;}
 const std::filesystem::path& assets()const{return assets_;}
 // Studio uses the same frame target as the game, at the selected device's
 // point size and pixel density. The host window does not affect its layout.
 void previewViewport(std::optional<PreviewViewport> value){previewViewport_=value;}
 SDL_Texture* frameTexture()const{return frameTexture_;}
 float width()const{return width_;}float height()const{return height_;}float worldScale()const{return width_/1088.f;}
 // Display-safe margins in canvas units. Non-zero where a notch, cutout or
 // rounded corner overlaps the window, so chrome can stay clear of it.
 Insets safeInsets()const{return safeInsets_;}
 float minimumTouchSize()const{return 44.f*width_/displayRect_.w;}
 // Horizontal shift applied to every drawing primitive. Used to centre the
 // fixed-width panel layout on a display wider than the design space.
 void origin(float x,float y=0){originX_=x;originY_=y;}float origin()const{return originX_;}float originY()const{return originY_;}
 void begin(bool fitViewport=false);void present();void softenScene();WorldPoint toWorld(float,float)const;SDL_FPoint toScreen(WorldPoint)const;
 void loadingScreen(float progress,std::string_view stage,double seconds,bool artwork=true,bool reduced=false);
 void preload(std::string_view name);void retainPreparedText(bool enabled);
 void beginMenuLayer(Texture&);void endMenuLayer();void menuLayer(const Texture&,const MenuPose&);
 void fill(Rect,Color);void round(Rect,Color,float radius=16,Color border={35,89,112,255},float thickness=2,bool shadow=true,bool shine=true);
 // A vertical two-color fill with antialiased rounded edges.
 void gradient(Rect,Color top,Color bottom,float radius=0);
 // A soft translucent ribbon, with amplitude/width in canvas units.
 void wave(Rect,Color,float amplitude,float wavelength,float phase,float thickness);
 void outline(Rect,Color,float radius,float thickness);
 void text(std::string_view,float x,float y,float size,Color color={37,75,94,255},bool center=false,float maxWidth=0,bool display=false,bool reference=false,bool gold=false,bool medium=false,double angle=0,float horizontalScale=1,bool rounded=false,bool heavy=false);
 float textWidth(std::string_view,float size,bool display=false,bool medium=false,bool rounded=false,bool heavy=false,bool reference=false);
 void boldText(std::string_view s,float x,float y,float size,Color color,bool center=false,float maxWidth=0,float horizontalScale=1){text(s,x,y,size,color,center,maxWidth,false,false,false,false,0,horizontalScale,false,true);}
 void roundedText(std::string_view s,float x,float y,float size,Color color,bool center=false,float maxWidth=0,float horizontalScale=1){text(s,x,y,size,color,center,maxWidth,true,false,false,false,0,horizontalScale,true);}
 float roundedTextWidth(std::string_view s,float size){return textWidth(s,size,true,false,true);}
 void image(std::string_view,Rect,double angle=0,SDL_FPoint pivot={.5f,.5f},float alpha=1,bool flip=false,Color tint={255,255,255,255});
 // Each row maps a source image Y coordinate (x) to a destination Y offset (y).
 void imageRows(std::string_view,Rect,std::span<const SDL_FPoint> rows);
 void horizontalImage(std::string_view,Rect,float sourceLeft,float sourceRight);
 void skin(std::string_view,Rect,float corner=28,float alpha=1);
 void facet(std::string_view,Rect,float alpha=1);
 void symbol(std::string_view,Rect,Color color={255,255,255,255});
 void surface(std::string_view,Rect,float sourceCorner,float corner,float alpha=1);
 void clip(Rect);void clearClip();
 void popover(Rect,SDL_FPoint tip);
 void icon(std::string_view,Rect,float alpha=1,bool flip=false,Color tint={255,255,255,255});
 void label(std::string_view,float x,float y,float size,bool center=false,float maxWidth=0,Color color={7,54,79,255},float stroke=0,Color outline={3,62,121,255});
 SDL_FPoint inputPoint(float,float,bool normalized=false)const;
 SDL_FPoint fishSize(const Species&,const Fish&);
 void fish(const Species&,const Fish&,double alpha,bool held,Color outline={},bool selected=false,bool reduced=false,Care appearance=Care::Fed,double time=0);
 Rect decorRect(const DecorDef&,WorldPoint,double sizeMul=1)const;
 void decorPlacementMarker(Rect);
 void decoration(const DecorDef&,const Decoration&,double time,bool animate=false,float alpha=1,int* fxBudget=nullptr,bool allowEmitter=true,bool highlight=false);
 void scene(const Domain&,double interpolation,double time,Tool,FishId held,bool careBadges=true,FishId hidden={},const Decoration* preview=nullptr,std::uint64_t selectedDecor=0,bool previewHighlight=true,bool backgroundArtwork=true);
 void cursor(CursorKind);void toolCursor(Tool,SDL_FPoint,double elapsed,bool reduced=false);
 bool capture(const std::filesystem::path&,bool windowPixels=false);void sound(double frequency,float volume);
 private:
 CursorKind cursorKind_{CursorKind::Arrow};SDL_Cursor* moveCursor_{},*stashCursor_{};
 std::filesystem::path assets_;SDL_Window* window_{};SDL_Renderer* renderer_{};SDL_AudioStream* audio_{};
 float width_{},height_{};std::unordered_map<std::string,std::unique_ptr<Texture>> textures_;
 SDL_Texture* frameTexture_{};SDL_Texture* softTexture_{};int softWidth_{},softHeight_{};Rect displayRect_{};float windowWidth_{},windowHeight_{};
 Insets safeInsets_{};float originX_{},originY_{},pixelScaleX_{1},pixelScaleY_{1};int frameWidth_{},frameHeight_{};Uint64 nextFrame_{};
 std::optional<PreviewViewport> previewViewport_;
 std::unordered_map<int,TTF_Font*> fonts_;std::filesystem::path fontPath_,displayFontPath_;
 struct TextEntry {std::unique_ptr<Texture> texture;std::uint64_t used{};float width{},height{};bool retained{};};std::unordered_map<std::string,TextEntry> textCache_;std::uint64_t frame_{};std::size_t textBytes_{},retainedTextBytes_{};bool retainText_{};
 std::uint64_t textureLoads_{},textRasterizations_{};
 Texture* texture(std::string_view);TTF_Font* font(int,bool,bool reference=false,bool medium=false,bool rounded=false,bool heavy=false);void mesh(SDL_Texture*,std::span<const SDL_Vertex>,std::span<const int>);void composeWindow();
};
enum class Panel {None,Tanks,Shop,Inventory,Collection,Settings,Details,Quests,Gifts,CurrencyShop};
struct Button {std::string id;Rect area;std::function<void()> action;};
struct Receipt {Event event;double start;};
struct LevelUnlock {std::string name,art,id;int category{};TankId tank{0};};
// Shared illustrated notice. Callers supply the message and action; currency
// rules and modal lifetime stay with the caller.
struct MessageDialogText {std::string text;bool emphasis{};};
struct MessageDialogContent {
 std::string title,illustration;
 std::vector<MessageDialogText> message;
 std::string detail,extra,actionLabel{"OK"};
};
struct UiRenderResult;
class View {
 friend struct ViewTestAccess;
 public:
 View(Canvas&,Session&,std::filesystem::path uiProject={});bool prepareMenus(const PreparationProgress& progress={});void render(double presentationSeconds);void event(const SDL_Event&,double presentationSeconds);
 Tool tool()const{return tool_;}FishId held()const{return dragged_.value?dragged_:selected_;}
 void cancelGesture();void setPanel(Panel);void fixture(std::string_view);
 void setDialogDesign(std::optional<DialogDesign> design){dialogOverride_=std::move(design);}
 const DialogDesign& dialogDesign()const{return dialogOverride_?*dialogOverride_:dialogDocument_.design();}
 const std::string& dialogDesignError()const{return dialogDocument_.error();}
 void previewFunds(CurrencyShortfall,std::string requirement={},double elapsed=.6);
 std::array<Rect,8> dialogElementBounds()const;
 const UiProject* uiProject()const;
 void setUiProject(std::optional<UiProject> project){uiOverride_=std::move(project);dialogOverride_.reset();}
 const std::shared_ptr<UiRenderResult>& uiLayout()const{return uiLayout_;}
 void previewUiScreen(std::string screen,std::string variant,UiBindings bindings={},bool pressed=false,bool disabled=false,double elapsed=2);
 void previewPurchase(int level,Amount coins,Amount pearls,int tank,bool owned,int upgrades=0);
 std::string uiRevision()const;

 private:
 Canvas& canvas_;Session& session_;Tool tool_{Tool::Select};Panel panel_{Panel::None};
 DialogDocument dialogDocument_;std::optional<DialogDesign> dialogOverride_;DialogLayout dialogLayout_;Uint64 nextDesignPoll_{};
 std::unique_ptr<UiDocument> uiDocument_;std::optional<UiProject> uiOverride_;std::shared_ptr<UiRenderResult> uiLayout_;
 std::string uiPreviewScreen_,uiPreviewVariant_;UiBindings uiPreviewBindings_;bool uiPreviewPressed_{},uiPreviewDisabled_{};double uiPreviewElapsed_{2};
 std::filesystem::path uiAck_;std::string uiAckRevision_,uiPreviewSpec_;
 void pollUi();void renderUiPreview();
 UiRenderResult drawUi(std::string_view screen,std::string_view variant,Rect,const UiBindings&,const std::map<std::string,Button>& actions);
 Rect uiMessageDialog(const MessageDialogContent&,std::string_view,std::string_view,std::function<void()>,std::function<void()>);
 std::vector<Button> buttons_;std::size_t panelButtonStart_{};Rect panelRect_;std::string pressed_,decorId_,buySpecies_;FishId selected_{},dragged_{},restore_{};WorldPoint dragOriginal_{},dragOffset_{};
 SDL_FPoint selectionDown_{};bool selectionGesture_{},selectionDragging_{};
 std::optional<WorldPoint> fishPreview_;std::optional<GrowthSnapshot> buyOffer_;
 bool rehomeConfirm_{};std::optional<FishReward> rehomeQuote_;
 std::optional<CurrencyShortfall> fundsDialog_;
 std::optional<MessageDialogContent> noticeDialog_;
 std::string fundsRequirement_;
 std::optional<Currency> fundsCurrency_;Rect fundsBounds_;
 std::deque<Event> levelUps_;int levelUnlockPage_{};
 std::string shopHighlightedId_;int shopHighlightedCategory_{-1};TankId highlightedTank_{0};
 void levelUp();void dismissLevelUp();void openLevelUnlock(const LevelUnlock&);
 Currency currencyCategory_{Currency::Coins};
 std::string currencyOfferTitle_,currencyOfferDetail_;
 struct CurrencyReturn {Panel panel{Panel::None};FishId fish{};int category{},page{};} currencyReturn_;
 void showFunds(CurrencyShortfall,std::string requirement={});void showNotice(MessageDialogContent);void showCurrencyFunds(Currency);void dismissFunds();void fundsDialog();void currencyShop();void openCurrencyShop(Currency);void closeCurrencyShop();
 Rect messageDialog(const MessageDialogContent&,std::string_view actionId,std::string_view closeId,std::function<void()> action,std::function<void()> close);
 Rect storageNoticeRect_;void storageNotice();
 float pointerX_{},pointerY_{},armX_{},armY_{};bool pointerDown_{},touchOwned_{},armed_{},worldGesture_{};SDL_FingerID finger_{};int page_{},category_{};
 Rect pageArea_;int pageCount_{1};bool pageGesture_{},pageSwiping_{},pageDragCancelled_{};
 float pageStartX_{},pageStartY_{},pageDragOffset_{},pageSettleFrom_{};double pageSettleAt_{-10};
 bool pointerSeen_{},pointerTouch_{};
 bool pointerInTank()const;void toolPointer();
 int shopItemsPerPage()const;int shopPageCount(int category)const;
 double now_{},pressStarted_{},jarAt_{-100},netAt_{-100};std::deque<Receipt> receipts_;
 struct Press {double start{};float from{1};bool down{};};std::unordered_map<std::string,Press> presses_;
 MenuMotion panelMotion_;std::unique_ptr<Texture> panelLayer_;Panel paintedPanel_{Panel::None};
 MenuPose panelPose_;Rect panelDisplayRect_,panelLayoutRect_;SDL_FPoint panelAnchor_{};float layerWidth_{},layerHeight_{};
 struct DialogAnimation {MenuMotion motion;std::unique_ptr<Texture> layer;MenuPose pose;Rect bounds;float width{},height{};};
 DialogAnimation fundsAnimation_,levelAnimation_,helpAnimation_;
 void renderDialog(DialogAnimation&,bool visible,void(View::*draw)(),Uint8 shade);
 void dialogs();bool dialogBlocking()const;void showHelp();void dismissHelp();void helpDialog();
 int collectionFilter_{},sortMode_{},selectedTank_{1};bool tutorialVisible_{},referencePreview_{},notificationOn_{true},languageOpen_{},helpOpen_{};
 std::string collectionSelected_;
 int inventoryCategory_{1};std::uint64_t selectedDecor_{},restoreDecor_{};
 std::optional<WorldPoint> decorPreview_,decorGestureStart_;std::string decorPlacementError_;bool draggingDecor_{},placementGesture_{};
 void glassButton(std::string id,Rect,std::string label,std::function<void()>,std::string skin="blue",float textSize=25,std::string icon="",float alpha=1,bool interactive=true,std::optional<Color> textColor={});
 void closeButton(std::string id,Rect,std::function<void()>);
 void titleSign(Rect,std::string_view title,std::string_view icon="");
 void shopHeader(Rect,std::string_view subtitle);
 float buttonScale(std::string_view)const;Rect buttonVisual(std::string_view,Rect)const;void press(std::string_view,bool);
 bool reducedMotion()const;MenuPose menuPose(const MenuMotion&,SDL_FPoint)const;
 void touchTarget(std::string id,Rect,std::function<void()>);
 void progress(Rect,float);
 void button(std::string id,Rect,std::string label,std::function<void()>,Color color={128,209,195,255},std::string icon="");
 float panelOrigin()const;float panelOriginY()const;void panelBody();void open(Panel);Result command(Command);void setTool(Tool);void resetActionTool();
 void pointerDown(float,float);void pointerMove(float,float);void pointerUp(float,float);
 void selectItem(float,float);void moveSelection(float,float);void finishSelection(float,float);void selectionControls();void stashSelection();
 float pageOffset()const;
 FishId hitFish(float,float,bool net=false)const;void aquariumPress();void ui();void tankNavigation();void panel();void shop();void tanks();void inventory();void collection();void settings();void details();void quests();void gifts();void tutorial();void effects();
 std::string price(const Species&)const;void armBuy(std::string);void armRestore(FishId);void armDecor(std::string);
 void beginMoveDecor(std::uint64_t);void decorInventory();void armStoredDecor(std::uint64_t);void confirmDecor();void cancelDecor();void decorPlacementControls();
 Rect decorPreviewRect()const;
 WorldPoint decorPreviewPoint(WorldPoint)const;
 std::uint64_t hitDecor(float,float)const;
};
double bezierOvershoot(double t);
std::string compact(Amount);std::string durationText(Millis);
std::string fishArt(std::string_view id, std::uint64_t variant=0);
struct FishDetailsContent {std::string title,care,growth,sale;FishReward reward,adultReward;float progress{};int stage{};Care condition{Care::Fed};};
std::vector<LevelUnlock> levelUnlocks(const Domain&,int level);
FishDetailsContent fishDetailsContent(const Species&,const Fish&,Millis now);
}
