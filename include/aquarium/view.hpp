#pragma once
#include "aquarium/storage.hpp"
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
struct Texture {
 SDL_Texture* value{};int width{},height{};std::vector<SDL_Texture*> levels;
 ~Texture(){for(auto* level:levels)SDL_DestroyTexture(level);if(value)SDL_DestroyTexture(value);}
 Texture()=default;Texture(const Texture&)=delete;Texture& operator=(const Texture&)=delete;
 SDL_Texture* sampled(float pixelWidth,float pixelHeight)const;
 std::size_t bytes()const;
};
struct FishPose {WorldPoint position;double phase{},pitch{},facing{},speed{};};
FishPose fishPose(const Fish&,double interpolation);
// Sample by presentation time so motion stays consistent at any frame rate.
struct MenuMotion {
 struct Sample {double value{},velocity{};};
 bool visible{};double changed{-10},from{},velocity{};
 Sample sample(double now)const;
 void show(bool,double now);
 void settle(){from=visible?1:0;velocity=0;changed=-10;}
};
struct MenuPose {
 SDL_FPoint anchor{};float xScale{1},yScale{1},rise{},alpha{1};
 Rect apply(Rect)const;
 SDL_FPoint unapply(SDL_FPoint)const;
};
class Canvas {
 friend struct ViewTestAccess;
 public:
 Canvas(std::filesystem::path assets,int width,int height,bool software);~Canvas();
 Canvas(const Canvas&)=delete;Canvas& operator=(const Canvas&)=delete;
 SDL_Window* window()const{return window_;}SDL_Renderer* renderer()const{return renderer_;}
 float width()const{return width_;}float height()const{return height_;}float worldScale()const{return width_/1088.f;}
 // Display-safe margins in canvas units. Non-zero where a notch, cutout or
 // rounded corner overlaps the window, so chrome can stay clear of it.
 Insets safeInsets()const{return safeInsets_;}
 float minimumTouchSize()const{return 44.f*width_/displayRect_.w;}
 // Horizontal shift applied to every drawing primitive. Used to centre the
 // fixed-width panel layout on a display wider than the design space.
 void origin(float x,float y=0){originX_=x;originY_=y;}float origin()const{return originX_;}float originY()const{return originY_;}
 void begin(bool fitViewport=false);void present();void softenScene();WorldPoint toWorld(float,float)const;SDL_FPoint toScreen(WorldPoint)const;
 void beginMenuLayer(Texture&);void endMenuLayer();void menuLayer(const Texture&,const MenuPose&);
 void fill(Rect,Color);void round(Rect,Color,float radius=16,Color border={35,89,112,255},float thickness=2,bool shadow=true);
 void text(std::string_view,float x,float y,float size,Color color={37,75,94,255},bool center=false,float maxWidth=0,bool display=false,bool reference=false,bool gold=false);
 void image(std::string_view,Rect,double angle=0,SDL_FPoint pivot={.5f,.5f},float alpha=1);
 void skin(std::string_view,Rect,float corner=28,float alpha=1);
 void popover(Rect,SDL_FPoint tip);
 void icon(std::string_view,Rect,float alpha=1);
 void label(std::string_view,float x,float y,float size,bool center=false,float maxWidth=0,Color color={255,255,255,255},float stroke=2);
 SDL_FPoint inputPoint(float,float,bool normalized=false)const;
 SDL_FPoint fishSize(const Species&,const Fish&);
 void fish(const Species&,const Fish&,double alpha,bool held,Color outline={},bool selected=false,bool reduced=false,Care appearance=Care::Fed);
 void scene(const Domain&,double interpolation,double time,Tool,FishId held,bool careBadges=true,FishId hidden={});
 bool capture(const std::filesystem::path&,bool windowPixels=false);void sound(double frequency,float volume);
 private:
 std::filesystem::path assets_;SDL_Window* window_{};SDL_Renderer* renderer_{};SDL_AudioStream* audio_{};
 float width_{},height_{};std::unordered_map<std::string,std::unique_ptr<Texture>> textures_;
 SDL_Texture* frameTexture_{};SDL_Texture* softTexture_{};int softWidth_{},softHeight_{};Rect displayRect_{};float windowWidth_{},windowHeight_{};
 Insets safeInsets_{};float originX_{},originY_{},pixelScaleX_{1},pixelScaleY_{1};int frameWidth_{},frameHeight_{};Uint64 nextFrame_{};
 std::unordered_map<int,TTF_Font*> fonts_;std::filesystem::path fontPath_,displayFontPath_;
 struct TextEntry {std::unique_ptr<Texture> texture;std::uint64_t used{};float width{},height{};};std::unordered_map<std::string,TextEntry> textCache_;std::uint64_t frame_{};std::size_t textBytes_{};
 Texture* texture(std::string_view);TTF_Font* font(int,bool,bool reference=false);void mesh(SDL_Texture*,std::span<const SDL_Vertex>,std::span<const int>);void composeWindow();
};
enum class Panel {None,Tanks,Shop,Inventory,Collection,Settings,Details,Quests,Gifts};
struct Button {std::string id;Rect area;std::function<void()> action;};
struct Toast {std::string text;double start;};
struct Receipt {Event event;double start;};
class View {
 friend struct ViewTestAccess;
 public:
 View(Canvas&,Session&);void prepareMenus();void render(double presentationSeconds);void event(const SDL_Event&,double presentationSeconds);
 Tool tool()const{return tool_;}FishId held()const{return dragged_.value?dragged_:selected_;}
 void cancelGesture();void setPanel(Panel);void fixture(std::string_view);
 private:
 Canvas& canvas_;Session& session_;Tool tool_{Tool::Select};Panel panel_{Panel::None};
 std::vector<Button> buttons_;std::size_t panelButtonStart_{};Rect panelRect_;std::string pressed_,buySpecies_,decorId_;FishId selected_{},dragged_{},restore_{};WorldPoint dragOriginal_{};
 float pointerX_{},pointerY_{},armX_{},armY_{};bool pointerDown_{},touchOwned_{},armed_{},worldGesture_{};SDL_FingerID finger_{};int page_{},category_{};
 double now_{},pressStarted_{},jarAt_{-100},netAt_{-100};std::deque<Toast> toasts_;std::deque<Receipt> receipts_;
 struct Press {double start{};float from{1};bool down{};};std::unordered_map<std::string,Press> presses_;
 MenuMotion panelMotion_;std::array<MenuMotion,3> selectMotion_;std::unique_ptr<Texture> panelLayer_;Panel paintedPanel_{Panel::None};
 MenuPose panelPose_;Rect panelDisplayRect_,panelLayoutRect_;SDL_FPoint panelAnchor_{};float layerWidth_{},layerHeight_{};
 int shopFilter_{},collectionFilter_{},sortMode_{},selectedTank_{2};bool selectMenuOpen_{},tutorialVisible_{},referencePreview_{},notificationOn_{true},languageOpen_{},helpOpen_{};int graphicsQuality_{2};float musicVolume_{.7f},soundVolume_{.8f};
 std::string collectionSelected_;
 void glassButton(std::string id,Rect,std::string label,std::function<void()>,std::string skin="blue",float textSize=25,std::string icon="",float alpha=1,bool interactive=true);
 float buttonScale(std::string_view)const;Rect buttonVisual(std::string_view,Rect)const;void press(std::string_view,bool);
 bool reducedMotion()const;MenuPose menuPose(const MenuMotion&,SDL_FPoint)const;void selectMenu(bool);void closeSelectMenu();
 void touchTarget(std::string id,Rect,std::function<void()>);
 void progress(Rect,float);
 void button(std::string id,Rect,std::string label,std::function<void()>,Color color={128,209,195,255},std::string icon="");
 float panelOrigin()const;float panelOriginY()const;void panelBody();void open(Panel);void toast(std::string);Result command(Command);void setTool(Tool);
 void pointerDown(float,float);void pointerMove(float,float);void pointerUp(float,float);
 FishId hitFish(float,float,bool net=false)const;void aquariumPress();void ui();void panel();void shop();void tanks();void inventory();void collection();void settings();void details();void quests();void gifts();void tutorial();void effects();
 std::string price(const Species&)const;void armBuy(std::string);void armRestore(FishId);void armDecor(std::string);
};
double bezierOvershoot(double t);
std::string compact(Amount);std::string durationText(Millis);
std::string fishArt(std::string_view id, std::uint64_t variant=0);
struct FishDetailsContent {std::string title,care,growth,sale;float progress{};int stage{};Care condition{Care::Fed};};
FishDetailsContent fishDetailsContent(const Species&,const Fish&,Millis now);
}
