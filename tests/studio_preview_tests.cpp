#include "aquarium/view.hpp"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace aq {
struct ViewTestAccess {
 static Rect button(const View& view,std::string_view id){for(const auto& button:view.buttons_)if(button.id==id)return button.area;throw std::runtime_error("Missing button "+std::string(id));}
 static Panel panel(const View& view){return view.panel_;}
 static Currency currency(const View& view){return view.currencyCategory_;}
};
}
namespace {
using namespace aq;
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Surface pixels(Canvas& canvas){Surface raw(SDL_RenderReadPixels(canvas.renderer(),nullptr),SDL_DestroySurface);check(bool(raw),"Cannot read preview");return Surface(SDL_ConvertSurface(raw.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);}
int difference(const SDL_Surface& a,const SDL_Surface& b){
 check(a.w==b.w&&a.h==b.h,"Preview has the wrong pixel dimensions");int count=0;
 for(int y=0;y<a.h;++y)for(int x=0;x<a.w;++x){const auto* p=static_cast<const Uint8*>(a.pixels)+y*a.pitch+x*4;const auto* q=static_cast<const Uint8*>(b.pixels)+y*b.pitch+x*4;count+=std::memcmp(p,q,4)!=0;}return count;
}
struct TemporaryAssets {
 std::filesystem::path path=std::filesystem::temp_directory_path()/("aquarium-studio-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 explicit TemporaryAssets(const std::filesystem::path& source){
  std::filesystem::create_directories(path/"ui");
  for(const auto& entry:std::filesystem::directory_iterator(source))if(entry.path().filename()!="ui")std::filesystem::create_symlink(entry.path(),path/entry.path().filename());
  for(const auto& entry:std::filesystem::directory_iterator(source/"ui"))if(entry.path().filename()!="general-dialog.json"&&entry.path().filename()!="studio.json")std::filesystem::create_symlink(entry.path(),path/"ui"/entry.path().filename());
 }
 ~TemporaryAssets(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
}
int main(int argc,char** argv){try{
 check(argc>1,"Pass the assets directory");const auto source=std::filesystem::absolute(argv[1]);const bool native=argc>2&&std::string_view(argv[2])=="--native";
 TemporaryAssets assets(source);const auto file=assets.path/"ui/general-dialog.json";DialogDocument document(file);check(document.save(),"Cannot save design fixture");
 std::ifstream contentFile(source/"content.json");const auto content=Content::fromJson(Json::parse(contentFile));Json results=Json::array();
 for(const auto size:std::array<std::array<int,2>,4>{{{1088,635},{852,393},{667,375},{1024,768}}}){
  Canvas canvas(assets.path,size[0],size[1],!native);Session session(content,assets.path/"unused-save.json",1000,true);
  auto state=session.domain().state();state.settings.sound=false;state.tutorialStep=11;session.domain().install(state);const auto before=encode(session.domain().state());
  int pixelWidth{},pixelHeight{};SDL_GetWindowSizeInPixels(canvas.window(),&pixelWidth,&pixelHeight);const int ratio=pixelWidth/size[0];
  for(bool pearls:{false,true}){
   auto design=DialogDesign::defaults();if(pearls){design.pearls.title="Pearls for your next tank";design.element(DialogPart::Title).font="nunito";design.element(DialogPart::Title).fontSize=61;
    design.element(DialogPart::Action).box={151,554,550,111};design.pearls.actionLabel="Find pearls";design.useShopArtwork=false;design.element(DialogPart::Illustration).box.x+=8;}
   document.apply(design);check(document.save(),"Cannot save edited design");
   canvas.previewViewport(std::nullopt);View game(canvas,session);game.fixture(pearls?"not-enough-pearls":"not-enough-coins");game.render(1);const auto gamePixels=pixels(canvas);const auto gameButton=ViewTestAccess::button(game,"funds-shop");
   canvas.previewViewport(PreviewViewport{size[0],size[1],ratio,{}});View preview(canvas,session);preview.setDialogDesign(design);preview.previewFunds(pearls?CurrencyShortfall{0,12}:CurrencyShortfall{240,0});preview.render(1);const auto previewPixels=pixels(canvas);
   const int diff=difference(*gamePixels,*previewPixels);check(diff==0,"Studio and game do not render identical pixels");
   const auto previewButton=ViewTestAccess::button(preview,"funds-shop");check(std::abs(gameButton.x-previewButton.x)<.001&&std::abs(gameButton.y-previewButton.y)<.001&&std::abs(gameButton.w-previewButton.w)<.001,"Button hit area differs from the game");
   const auto visible=preview.dialogElementBounds()[static_cast<int>(DialogPart::Action)];check(previewButton.has(visible.x+visible.w*.5f,visible.y+visible.h*.5f),"Edited button is not clickable");
   if(size[0]==1088&&!pearls){
    auto updated=design;updated.coins.title="This title was reloaded";updated.element(DialogPart::Title).color={10,80,120,255};
    document.apply(updated);check(document.save(),"Cannot save live edit");SDL_Delay(270);
    game.render(1);const auto reloaded=pixels(canvas);preview.setDialogDesign(updated);preview.render(1);const auto livePreview=pixels(canvas);
    check(difference(*reloaded,*livePreview)==0&&game.dialogDesign()==updated,"Running game did not reload a saved design");
    {std::ofstream(file)<<"{broken";}SDL_Delay(270);game.render(1);const auto preserved=pixels(canvas);
    check(!game.dialogDesignError().empty()&&difference(*preserved,*livePreview)==0,"Bad file replaced the running game's last valid design");
    document.reload(true);check(document.save(),"Cannot restore valid test design");preview.setDialogDesign(design);preview.render(1);
   }
   const float x=(visible.x+visible.w*.5f)/canvas.width()*size[0],y=(visible.y+visible.h*.5f)/canvas.height()*size[1];
   SDL_Event click{};click.type=SDL_EVENT_MOUSE_BUTTON_DOWN;click.button.button=SDL_BUTTON_LEFT;click.button.x=x;click.button.y=y;preview.event(click,1);click.type=SDL_EVENT_MOUSE_BUTTON_UP;preview.event(click,1.1);preview.render(1.8);
   check(ViewTestAccess::panel(preview)==Panel::CurrencyShop&&ViewTestAccess::currency(preview)==(pearls?Currency::Pearls:Currency::Coins),"Edited button opened the wrong shop");
   check(encode(session.domain().state())==before,"Preview changed player progress");
   results.push_back({{"width",size[0]},{"height",size[1]},{"density",ratio},{"variant",pearls?"pearls, edited":"coins, reference"},{"differentPixels",diff},{"buttonAction","pass"}});
  }
 }
 const auto output=source.parent_path()/"evidence/studio";std::filesystem::create_directories(output);std::ofstream(output/(native?"native-parity.json":"software-parity.json"))<<results.dump(2)<<'\n';
 std::cout<<"PASS identical game and Studio pixels for both currencies at four device sizes; edited buttons open the correct shop without changing progress\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
