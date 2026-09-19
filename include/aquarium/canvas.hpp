#pragma once
#include "aquarium/storage.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <array>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <span>
namespace aq {
struct Rect {float x{},y{},w{},h{};bool has(float px,float py)const{return px>=x&&py>=y&&px<=x+w&&py<=y+h;}};
struct Insets {float left{},top{},right{},bottom{};};
struct Color {Uint8 r{},g{},b{},a{255};};
// Backgrounds and anchored objects share one crop. Swimming fish use the full
// window through Canvas::toScreen/toWorld instead.
struct SceneProjection {
 Rect viewport,artwork;
 SDL_FPoint toScreen(WorldPoint)const;
 WorldPoint toWorld(SDL_FPoint)const;
 float decorUnit()const;
 WorldPoint place(const DecorDef&,WorldPoint,double sizeMul=1)const;
};
SceneProjection sceneProjection(Rect viewport);
struct Texture {
 SDL_Texture* value{};int width{},height{};std::vector<SDL_Texture*> levels;
 ~Texture(){for(auto* level:levels)SDL_DestroyTexture(level);if(value)SDL_DestroyTexture(value);}
 Texture()=default;Texture(const Texture&)=delete;Texture& operator=(const Texture&)=delete;
 SDL_Texture* sampled(float pixelWidth,float pixelHeight)const;
 std::size_t bytes()const;
};
enum class CursorKind {Arrow,Hidden};
struct ToolCursorPose {double angle{};float scale{1};};
ToolCursorPose toolCursorPose(Tool,double elapsed,bool reduced=false);
struct FishPose {WorldPoint position;double phase{},pitch{},facing{},speed{};};
struct DecorVertexPose {double x{},y{},light{1};};
DecorVertexPose decorVertexPose(const DecorDef&,std::uint64_t copy,double time,double u,double v,bool animate);
FishPose fishPose(const Fish&,double interpolation);
struct PreviewViewport {int width{1088},height{635},pixelRatio{1};Insets safe;};
struct CanvasCacheStats {std::size_t textureLoads{},textRasterizations{},textBytes{};bool operator==(const CanvasCacheStats&)const=default;};
class Canvas {
 public:
 Canvas(std::filesystem::path assets,int width,int height,bool software);~Canvas();
 Canvas(const Canvas&)=delete;Canvas& operator=(const Canvas&)=delete;
 SDL_Window* window()const{return window_;}SDL_Renderer* renderer()const{return renderer_;}
 const std::filesystem::path& assets()const{return assets_;}
 CanvasCacheStats cacheStats()const{return {textures_.size(),textRasterizations_,textBytes_};}
 // Render test captures at a chosen point size, pixel density and safe area.
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
 void begin(bool fitViewport=false);void present();WorldPoint toWorld(float,float)const;SDL_FPoint toScreen(WorldPoint)const;
 void loadingScreen(float progress,std::string_view stage,double seconds,bool artwork=true,bool reduced=false);
 void fill(Rect,Color);void round(Rect,Color,float radius=16,Color border={35,89,112,255},float thickness=2,bool shadow=true,bool shine=true);
 void triangle(const std::array<SDL_FPoint,3>&,Color);
 // A vertical two-color fill with antialiased rounded edges.
 void gradient(Rect,Color top,Color bottom,float radius=0);
 // A soft translucent ribbon, with amplitude/width in canvas units.
 void wave(Rect,Color,float amplitude,float wavelength,float phase,float thickness);
 void outline(Rect,Color,float radius,float thickness);
 void text(std::string_view,float x,float y,float size,Color color={37,75,94,255},bool center=false,float maxWidth=0,bool display=false,bool reference=false,bool gold=false,bool medium=false,double angle=0,float horizontalScale=1,bool rounded=false,bool heavy=false,float renderScale=1);
 float textWidth(std::string_view,float size,bool display=false,bool medium=false,bool rounded=false,bool heavy=false,bool reference=false);
 void boldText(std::string_view s,float x,float y,float size,Color color,bool center=false,float maxWidth=0,float horizontalScale=1){text(s,x,y,size,color,center,maxWidth,false,false,false,false,0,horizontalScale,false,true);}
 void roundedText(std::string_view s,float x,float y,float size,Color color,bool center=false,float maxWidth=0,float horizontalScale=1){text(s,x,y,size,color,center,maxWidth,true,false,false,false,0,horizontalScale,true);}
 float roundedTextWidth(std::string_view s,float size){return textWidth(s,size,true,false,true);}
 void image(std::string_view,Rect,double angle=0,SDL_FPoint pivot={.5f,.5f},float alpha=1,bool flip=false,Color tint={255,255,255,255});
 void roundedImage(std::string_view,Rect,float radius,float feather=0,bool flip=false);
 void clip(Rect);void clearClip();
 void icon(std::string_view,Rect,float alpha=1,bool flip=false,Color tint={255,255,255,255});
 void label(std::string_view,float x,float y,float size,bool center=false,float maxWidth=0,Color color={7,54,79,255},float stroke=0,Color outline={3,62,121,255});
 SDL_FPoint inputPoint(float,float,bool normalized=false)const;
 SDL_FPoint fishSize(const Species&,const Fish&);
 void fish(const Species&,const Fish&,double alpha,bool held,Color outline={},bool selected=false,bool reduced=false,Care appearance=Care::Fed,double time=0,float opacity=1);
 Rect decorRect(const DecorDef&,WorldPoint,double sizeMul=1)const;
 // Front first, including covered items so repeated taps can reach them.
 std::vector<const Decoration*> decorHits(const Domain&,SDL_FPoint)const;
 SceneProjection decorProjection()const{return sceneProjection({0,0,width_,height_});}
 void decoration(const DecorDef&,const Decoration&,double time,bool animate=false,float alpha=1,int* fxBudget=nullptr,bool allowEmitter=true,bool highlight=false);
 void scene(const Domain&,double interpolation,double time,Tool,FishId held,bool careBadges=true,FishId hidden={},const Decoration* preview=nullptr,std::uint64_t selectedDecor=0,bool previewHighlight=true,bool backgroundArtwork=true);
 void environment(Rect,std::string_view backgroundId="sunlit-lagoon");
 void cursor(CursorKind);void toolCursor(Tool,SDL_FPoint,double elapsed,bool reduced=false);
 bool capture(const std::filesystem::path&,bool windowPixels=false);void sound(double frequency,float volume);
 private:
 CursorKind cursorKind_{CursorKind::Arrow};
 std::filesystem::path assets_;SDL_Window* window_{};SDL_Renderer* renderer_{};SDL_AudioStream* audio_{};
 float width_{},height_{};std::unordered_map<std::string,std::unique_ptr<Texture>> textures_;
 SDL_Texture* frameTexture_{};Rect displayRect_{};float windowWidth_{},windowHeight_{};
 std::array<SDL_Texture*,2> focusTextures_{};
 Insets safeInsets_{};float originX_{},originY_{},pixelScaleX_{1},pixelScaleY_{1};int frameWidth_{},frameHeight_{};Uint64 nextFrame_{};
 std::optional<PreviewViewport> previewViewport_;
 std::unordered_map<int,TTF_Font*> fonts_;std::filesystem::path fontPath_,displayFontPath_;
 struct TextEntry {std::unique_ptr<Texture> texture;std::uint64_t used{};float width{},height{};};std::unordered_map<std::string,TextEntry> textCache_;std::uint64_t frame_{};std::size_t textBytes_{},textRasterizations_{};
 Texture* texture(std::string_view);TTF_Font* font(int,bool,bool reference=false,bool medium=false,bool rounded=false,bool heavy=false);void mesh(SDL_Texture*,std::span<const SDL_Vertex>,std::span<const int>);void composeWindow();void softenScene();
};
std::string compact(Amount);std::string usdPrice(int cents);
std::string fishArt(std::string_view id);
}
