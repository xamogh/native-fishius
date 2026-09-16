#include "aquarium/view.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
namespace aq {
struct ViewTestAccess {
 static Rect control(const View& v,const std::string& id){for(const auto& b:v.buttons_)if(b.id==id)return b.area;throw std::runtime_error("Missing "+id);}
 static std::vector<std::string> controls(const View& v,std::string_view prefix){std::vector<std::string> r;for(const auto& b:v.buttons_)if(b.id.starts_with(prefix))r.push_back(b.id.substr(prefix.size()));return r;}
 static void tap(View& v,float x,float y){v.pointerDown(x,y);v.pointerUp(x,y);}
 static void drag(View& v,SDL_FPoint from,SDL_FPoint to){v.pointerDown(from.x,from.y);v.pointerMove(to.x,to.y);v.pointerUp(to.x,to.y);}
 static void settle(View& v){v.panelMotion_.settle();}
 static void down(View& v,SDL_FPoint p){v.pointerDown(p.x,p.y);}
 static void up(View& v,SDL_FPoint p){v.pointerUp(p.x,p.y);}
 static void move(View& v,SDL_FPoint p){v.pointerMove(p.x,p.y);}
 static void touchDrag(View& v,SDL_FPoint from,SDL_FPoint to){
  SDL_Event e{};e.type=SDL_EVENT_FINGER_DOWN;e.tfinger.touchID=1;e.tfinger.fingerID=1;e.tfinger.x=from.x;e.tfinger.y=from.y;v.event(e,v.now_);
  e.type=SDL_EVENT_FINGER_MOTION;e.tfinger.x=to.x;e.tfinger.y=to.y;v.event(e,v.now_);
  e.type=SDL_EVENT_FINGER_UP;v.event(e,v.now_);
 }
 static WorldPoint previewPoint(const View& v){return *v.decorPreview_;}
 static void select(View& v,std::uint64_t id){v.setTool(Tool::Decor);v.selectedDecor_=id;}
 static std::uint64_t hit(const View& v,float x,float y){return v.hitDecor(x,y);}
 static void escape(View& v){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=SDLK_ESCAPE;v.event(e,v.now_);}
 static bool preview(const View& v){return v.decorPreview_.has_value();}
 static void category(View& v,int category){v.open(Panel::Shop);v.category_=category;v.page_=0;v.panelMotion_.settle();}
 static void arm(View& v,const std::string& id){v.armDecor(id);}
 static Rect pageArea(const View& v){return v.pageArea_;}
};
}
using namespace aq;
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
void checkMotion(const Content& c){
 for(const auto& d:c.decorations){bool changes=false;
  for(int frame=0;frame<160;++frame)for(int y=0;y<=12;++y)for(int x=0;x<=12;++x){
   const double u=x/12.,v=y/12.,t=frame*.37;const auto a=decorVertexPose(d,23,t,u,v,true),still=decorVertexPose(d,23,t,u,v,false);
   require(std::isfinite(a.x)&&std::isfinite(a.y)&&a.x>=0&&a.x<=1&&a.y>=0&&a.y<=1,"Animation escaped authored footprint");
   require(still.x==u&&still.y==v&&still.light==1,"Reduced motion changed still pose");
   if(v==1)require(a.x==u&&a.y==v,"Animation moved the root/base");
   require(a.light>=.85&&a.light<=1,"Local light exceeded workbook amplitude");
   changes|=std::abs(a.x-u)>1e-6||std::abs(a.y-v)>1e-6||std::abs(a.light-1)>1e-6;
  }
  const auto kind=d.art["motion"].value("kind",std::string("static"));
  if(kind!="static"&&kind!="bubble"&&kind!="internalBubble"&&kind!="snowglobe"&&kind!="pinwheel")require(changes,"Animated item has no visible movement");
  if(kind=="static")require(!changes,"Coin item unexpectedly animates");
 }
}
void viewport(const Content& content,const std::filesystem::path& assets,int width,int height){
 Canvas canvas(assets,width,height,true);Session s(content,"/tmp/aquarium-decor-ui-test-save.json",1000,true);View v(canvas,s);double time=0;
 auto render=[&]{time+=.5;v.render(time);};
 auto checkSize=[&](std::uint64_t id,Rect preview){
  const double placedAt=time;for(double elapsed:{0.,.016,.1,.28,.55,.7}){
   time=placedAt+elapsed;v.render(time);const auto* item=s.domain().decoration(id);require(item,"Placed item is missing");
   const auto actual=canvas.decorRect(*content.findDecor(item->kind),item->position,item->sizeMul);
   require(std::abs(actual.x-preview.x)<.01&&std::abs(actual.y-preview.y)<.01&&std::abs(actual.w-preview.w)<.01&&std::abs(actual.h-preview.h)<.01,"Placed size or anchor jumps away from its preview");
  }
 };
 auto tap=[&](const std::string& id){render();const auto r=ViewTestAccess::control(v,id);ViewTestAccess::tap(v,r.x+r.w*.5f,r.y+r.h*.5f);ViewTestAccess::settle(v);render();};
 for(int category:{1,2}){ViewTestAccess::category(v,category);render();std::set<std::string> seen,expected;int lastLevel=0;
  for(const auto& item:content.decorations)if(item.level<=40&&item.edition!="Limited Edition"&&(item.category=="Plant")==(category==1))expected.insert(item.id);
  require(ViewTestAccess::controls(v,"decor-filter").empty(),"Decor shop still exposes filters");
  for(int page=0;page<20;++page){
   const auto ids=ViewTestAccess::controls(v,"decor-buy");std::size_t n=seen.size();seen.insert(ids.begin(),ids.end());if(seen.size()==n)break;
   require(seen.size()-n==ids.size(),"Decor catalog repeats an item across pages");
   for(const auto& id:ids){const auto* item=content.findDecor(id);require(item&&item->level>=lastLevel&&item->level<=40,"Decor catalog is out of level order or exceeds level 40");lastLevel=item->level;}
   const auto grid=ViewTestAccess::pageArea(v);
   const float x=grid.x+grid.w*.6f,y=grid.y+grid.h*.3f;
   ViewTestAccess::touchDrag(v,{x/canvas.width(),y/canvas.height()},
                              {(x-canvas.minimumTouchSize()*2)/canvas.width(),y/canvas.height()});render();
  }
  require(seen==expected&&seen.size()==36,"Shop is missing a regular level 1 through 40 item or still shows an event item");
  require(lastLevel==40,"Catalog omitted its level 40 items");
 }
 require(s.domain().state().decor.empty()&&s.domain().state().decorOnboardingComplete,"Browsing changed decor ownership or first-plant bonus state");
 ViewTestAccess::category(v,1);render();
 const auto directory=std::filesystem::path("evidence/confirm-placement")/(std::to_string(width)+"x"+std::to_string(height));std::filesystem::create_directories(directory);
 const auto before=s.domain().state();tap("decor-buyCP-01");require(v.tool()==Tool::Decor,"Buying did not enter decor placement");
 require(s.domain().state().decor.empty()&&s.domain().state().pendingDecor.empty(),"Preview should not queue a paid plant");
 require(s.domain().state().wallet.coins==before.wallet.coins&&s.domain().state().xp==before.xp,"Preview charged or granted XP before confirmation");
 require(ViewTestAccess::preview(v),"Shop purchase has no initial preview");
 require(ViewTestAccess::controls(v,"").size()==2,"Preview must expose only cancel and confirm");
 require(std::abs(ViewTestAccess::previewPoint(v).x-544)<.01&&std::abs(ViewTestAccess::previewPoint(v).y-441.1904)<.01,"Initial ghost is not at Fishium's centre-front position");
 auto p=canvas.toScreen({280,580});ViewTestAccess::move(v,p);render();
 require(std::abs(ViewTestAccess::previewPoint(v).x-544)<.01,"Preview follows the pointer without a drag");
 ViewTestAccess::tap(v,p.x,p.y);render();
 require(std::abs(ViewTestAccess::previewPoint(v).y-580)<.01,"Preview cannot reach the bottom of the tank");
 require(canvas.capture(directory/"plant-preview.png"),"Cannot capture the plant preview");
 for(const char* id:{"decor-cancel","decor-confirm"}){
  const auto control=ViewTestAccess::control(v,id);
  require(control.w>=canvas.minimumTouchSize()&&control.h>=canvas.minimumTouchSize(),"Placement target is too small");
  require(control.x>=0&&control.y>=0&&control.x+control.w<=canvas.width()&&control.y+control.h<=canvas.height(),"Placement controls are clipped");
 }
 // A release outside the water or after focus loss must not place an item.
 ViewTestAccess::down(v,p);ViewTestAccess::up(v,{-20,-20});render();require(s.domain().state().decor.empty(),"Release outside water placed decor");
 ViewTestAccess::down(v,p);v.cancelGesture();ViewTestAccess::up(v,p);render();require(s.domain().state().decor.empty()&&ViewTestAccess::preview(v),"Interrupted touch lost or placed the paid item");
 require(std::abs(ViewTestAccess::previewPoint(v).x-280)<.01,"Interrupted drag changed the preview position");
 require(s.domain().state().decor.empty(),"Hover committed the purchase");
 const auto plantPreview=canvas.decorRect(*content.findDecor("CP-01"),ViewTestAccess::previewPoint(v));
 tap("decor-confirm");checkSize(1,plantPreview);require(s.domain().state().decor.size()==1&&!ViewTestAccess::preview(v),"Confirmation did not place the plant");
 require(s.domain().state().wallet.coins==225&&s.domain().state().xp==0,"Confirmation did not charge exactly once");
 ViewTestAccess::tap(v,p.x,p.y);render();require(s.domain().state().decor.size()==1,"Second tap bought another plant");
 require(v.tool()==Tool::Decor,"Selecting decor left arrange mode");
 require(ViewTestAccess::controls(v,"decor-").empty()&&ViewTestAccess::controls(v,"done").size()==1,"Arrange mode has no visible Done button or still shows old decor controls");
 ViewTestAccess::control(v,"selection-stash");
 require(canvas.capture(directory/"selected-plant.png"),"Cannot capture selection controls");
 // A saved custom size remains intact even though selection has no size buttons.
 Command resize;resize.action=Action::ResizeDecor;resize.decor=1;resize.value=.12;require(bool(s.domain().execute(resize)),"Cannot prepare saved custom size");
 const auto original=s.domain().decoration(1)->position;
 const auto from=canvas.toScreen(original);p=canvas.toScreen({390,615});
 ViewTestAccess::down(v,from);ViewTestAccess::move(v,p);render();
 require(s.domain().decoration(1)->position.x==original.x,"Moving preview overwrote the saved position");
 require(ViewTestAccess::controls(v,"selection-stash").empty(),"Stash button remains active during a drag");
 const auto movePreview=canvas.decorRect(*content.findDecor("CP-01"),ViewTestAccess::previewPoint(v),1.12);
 ViewTestAccess::up(v,p);render();require(s.domain().decoration(1)->position.x==original.x,"Drag release saved before confirmation");tap("decor-confirm");checkSize(1,movePreview);require(std::abs(s.domain().decoration(1)->position.x-390)<.01&&!ViewTestAccess::preview(v),"Confirmed drag did not commit");
 tap("selection-stash");require(s.domain().decoration(1)->stored,"Contextual Stash did not store the decoration");require(s.domain().decorScore({1})==0,"Stored decoration contributes score");
 tap("inventory");tap("restore-decor1");require(ViewTestAccess::controls(v,"").size()==2,"Restore is missing placement controls");
 p=canvas.toScreen({280,590});const auto restorePreview=canvas.decorRect(*content.findDecor("CP-01"),{280,590},1.12);
 ViewTestAccess::touchDrag(v,{250.f/1088,400.f/635},{280.f/1088,590.f/635});render();require(s.domain().decoration(1)->stored,"Restore drag committed on release");tap("decor-confirm");checkSize(1,restorePreview);
 require(!s.domain().decoration(1)->stored&&!ViewTestAccess::preview(v),"Confirmation did not restore the stored copy");
 require(s.domain().state().wallet.coins==225&&s.domain().state().xp==0,"Rearranging or restoring changed currency or XP");
 ViewTestAccess::category(v,2);render();tap("decor-buyCD-01");require(ViewTestAccess::preview(v),"Decoration purchase did not start a ghost");
 p=canvas.toScreen({450,610});const auto decorationPreview=canvas.decorRect(*content.findDecor("CD-01"),{450,610});ViewTestAccess::touchDrag(v,{300.f/1088,340.f/635},{450.f/1088,610.f/635});render();require(s.domain().state().decor.size()==1,"Touch drag bought before confirmation");tap("decor-confirm");checkSize(2,decorationPreview);require(s.domain().state().decor.size()==2,"Decoration drag did not place once");
 ViewTestAccess::category(v,1);render();tap("decor-buyCP-01");const auto paid=s.domain().state().wallet.coins;const auto copies=s.domain().state().decor.size();ViewTestAccess::escape(v);render();
 require(!ViewTestAccess::preview(v)&&s.domain().state().pendingDecor.empty()&&s.domain().state().wallet.coins==paid,"Cancel failed to clear the paid ghost");
 require(s.domain().state().decor.size()==copies,"Cancelling bought or stored an unpurchased copy");
 ViewTestAccess::select(v,1);render();tap("selection-stash");tap("inventory");tap("restore-decor1");tap("decor-cancel");
 require(s.domain().decoration(1)->stored&&s.domain().state().decor.size()==copies&&s.domain().state().wallet.coins==paid,"Cancelling restore changed stored ownership or money");
 // Check scale continuity at both depth extremes, where the old pop was
 // most noticeable. The HUD returns only after the placement is complete.
 for(const auto point:{WorldPoint{544,0},WorldPoint{544,153.6},WorldPoint{544,635},WorldPoint{0,620},WorldPoint{1088,620}}){
  ViewTestAccess::category(v,1);render();tap("decor-buyCP-01");
  const auto adjusted=decorPlacementPoint(*content.findDecor("CP-01"),point);
  const auto size=canvas.decorRect(*content.findDecor("CP-01"),adjusted);
  const auto at=canvas.toScreen(point);ViewTestAccess::tap(v,at.x,at.y);render();
  for(const char* id:{"decor-cancel","decor-confirm"}){const auto r=ViewTestAccess::control(v,id);require(r.x>=0&&r.y>=0&&r.x+r.w<=canvas.width()&&r.y+r.h<=canvas.height(),"Edge controls are clipped");}
  tap("decor-confirm");checkSize(s.domain().state().decor.back().id,size);
  require(size.x>=-.02&&size.y>=-.02&&size.x+size.w<=canvas.width()+.02&&size.y+size.h<=canvas.height()+.02,"Edge placement leaves the decoration outside the tank");
  const auto actual=s.domain().state().decor.back().position;
  require(std::hypot(actual.x-adjusted.x,actual.y-adjusted.y)<.02,"Placement does not retain its clamped preview anchor");
  require(!ViewTestAccess::controls(v,"").empty(),"HUD did not return after placement");
 }
 ViewTestAccess::category(v,1);render();require(canvas.capture(directory/"plants.png"),"Plant capture failed");
 ViewTestAccess::category(v,2);render();require(canvas.capture(directory/"decorations.png"),"Decoration capture failed");
 v.setPanel(Panel::None);render();require(canvas.capture(directory/"placed.png"),"Placement capture failed");
 // A full tank must keep a restore attempt in
 // inventory, where the player can still choose another action.
 auto full=s.domain().state();full.decor.front().stored=true;
 while(std::count_if(full.decor.begin(),full.decor.end(),[](const auto& item){return !item.stored;})<32){auto copy=full.decor[1];copy.id=full.nextDecorId++;copy.stored=false;full.decor.push_back(copy);}
 s.domain().install(full);ViewTestAccess::category(v,1);render();tap("panel-close");tap("inventory");
 if(!ViewTestAccess::controls(v,"inventory-decor").empty())tap("inventory-decor");
 tap("restore-decor1");
 require(!ViewTestAccess::preview(v)&&s.domain().decoration(1)->stored&&!ViewTestAccess::controls(v,"").empty(),"Full-tank restore trapped the player in placement");
 std::cout<<"PASS decor UI "<<width<<"x"<<height<<'\n';
}
void purchaseLifecycle(const Content& content,const std::filesystem::path& assets){
 Canvas canvas(assets,852,393,true);Session session(content,"/tmp/aquarium-confirm-unused.json",1000,true);View view(canvas,session);
 auto& domain=session.domain();auto rich=domain.state();rich.fish.clear();rich.xp=content.levels.back();rich.highestRewardedLevel=40;rich.wallet={1000000,100000};rich.decorOnboardingComplete=true;domain.install(rich);
 double time=1;const auto render=[&]{view.render(time+=.5);};
 const auto tap=[&](const char* id){render();const auto r=ViewTestAccess::control(view,id);ViewTestAccess::tap(view,r.x+r.w*.5f,r.y+r.h*.5f);render();};
 for(const char* id:{"CP-01","PP-01","CD-01"}){
  const auto before=encode(domain.state());ViewTestAccess::arm(view,id);render();
  require(encode(domain.state())==before,"Opening a preview changes money, XP or ownership");
  tap("decor-cancel");require(encode(domain.state())==before&&!ViewTestAccess::preview(view),"Cancel changes state");
  ViewTestAccess::arm(view,id);render();view.setPanel(Panel::Shop);render();
  require(encode(domain.state())==before&&!ViewTestAccess::preview(view),"Panel change retains or purchases a preview");
  ViewTestAccess::arm(view,id);render();
  const auto preview=ViewTestAccess::previewPoint(view);const auto grab=canvas.toScreen({preview.x,preview.y-8});
  const auto dest=canvas.toScreen({preview.x+100,preview.y+40});
  ViewTestAccess::drag(view,grab,dest);render();
  require(std::abs(ViewTestAccess::previewPoint(view).y-(preview.y+48))<.02&&encode(domain.state())==before,"Preview drag loses its grab offset or commits on release");
  const auto r=ViewTestAccess::control(view,"decor-confirm");ViewTestAccess::tap(view,r.x+r.w*.5f,r.y+r.h*.5f);
  const auto bought=encode(domain.state());ViewTestAccess::tap(view,r.x+r.w*.5f,r.y+r.h*.5f);render();
  require(encode(domain.state())==bought,"Rapid repeat confirmation bought twice");
  const auto* def=content.findDecor(id);
  require(domain.state().decor.back().kind==id&&!domain.state().decor.back().stored,"Confirm did not create the placed copy");
  require(domain.state().wallet.coins==before["coins"].get<Amount>()-(def->currency==Currency::Coins?def->price:0),"Confirmation used the wrong coin amount");
  require(domain.state().wallet.pearls==before["pearls"].get<Amount>()-(def->currency==Currency::Pearls?def->price:0),"Confirmation used the wrong pearl amount");
 }
 // Recheck the wallet at confirmation, after the preview was created.
 domain.install(rich);ViewTestAccess::arm(view,"CP-01");render();auto poor=domain.state();poor.wallet.coins=0;domain.install(poor);
 const auto beforeFailure=encode(domain.state());tap("decor-confirm");
 require(ViewTestAccess::preview(view)&&encode(domain.state())==beforeFailure,"Failed confirmation changed state or discarded the preview");
 ViewTestAccess::escape(view);render();tap("decor-cancel");
 // Space can disappear while a preview is open. Keep cancellation reachable.
 domain.install(rich);ViewTestAccess::arm(view,"CP-01");render();auto full=domain.state();
 for(int i=0;i<32;++i)full.decor.push_back({full.nextDecorId++,"CP-01",full.activeTank,{544,500}});
 full.decorOwned={"CP-01"};domain.install(full);const auto fullBefore=encode(domain.state());tap("decor-confirm");
 require(ViewTestAccess::preview(view)&&encode(domain.state())==fullBefore,"Full-tank confirmation lost currency or placed an extra copy");tap("decor-cancel");
 // A saved paid copy from the older flow must never be charged again.
 domain.install(rich);require(bool(domain.execute({.action=Action::PurchaseDecor,.key="CP-01"})),"Cannot create legacy paid fixture");
 const auto paid=domain.state().wallet;View restored(canvas,session);restored.render(time+=.5);
 auto confirm=ViewTestAccess::control(restored,"decor-confirm");ViewTestAccess::tap(restored,confirm.x+confirm.w*.5f,confirm.y+confirm.h*.5f);
 require(domain.state().decor.size()==1&&domain.state().wallet.coins==paid.coins&&domain.state().pendingDecor.empty(),"Legacy confirmation charged a paid copy again");
 require(bool(domain.execute({.action=Action::PurchaseDecor,.key="CP-01"})),"Cannot create legacy cancel fixture");
 const auto pendingWallet=domain.state().wallet;View cancelled(canvas,session);cancelled.render(time+=.5);
 auto cancel=ViewTestAccess::control(cancelled,"decor-cancel");ViewTestAccess::tap(cancelled,cancel.x+cancel.w*.5f,cancel.y+cancel.h*.5f);
 require(domain.state().decor.size()==2&&domain.state().decor.back().stored&&domain.state().wallet.coins==pendingWallet.coins,"Legacy cancellation lost a paid item");
 std::cout<<"PASS preview purchase lifecycle, both currencies, revalidation and legacy saves\n";
}
void hitLayers(const Content& content,const std::filesystem::path& assets){
 auto c=content;auto front=*c.findDecor("CP-01"),back=*c.findDecor("CP-02");
 front.width=back.width=3;front.height=back.height=3;front.layer="Foreground";back.layer="Background";
 c.decorations={front,back};Canvas canvas(assets,852,393,true);Session s(c,"/tmp/aquarium-decor-hit-test-save.json",1000,true);View v(canvas,s);
 auto state=s.domain().state();state.fish.clear();state.decor={{1,front.id,{1},{544,360}},{2,back.id,{1},{544,390}}};state.nextDecorId=3;s.domain().install(state);
 const auto at=canvas.toScreen({544,345});
 require(ViewTestAccess::hit(v,at.x,at.y)==0,"Normal selection mode hit-tests decor");
 ViewTestAccess::select(v,0);
 require(ViewTestAccess::hit(v,at.x,at.y)==1,"Decor hit-testing ignores catalog layers");
 state.decor[1].kind=front.id;s.domain().install(state);
 require(ViewTestAccess::hit(v,at.x,at.y)==2,"Decor hit-testing ignores depth within a layer");
 state.decor[0].position=state.decor[1].position;s.domain().install(state);
 require(ViewTestAccess::hit(v,at.x,at.y)==2,"Equal-depth decor hit-testing differs from draw order");
 std::cout<<"PASS decor mode and layer-aware hit testing\n";
}
void depthRendering(const Content& content,const std::filesystem::path& assets){
 Canvas canvas(assets,852,393,true);
 const auto directory=std::filesystem::temp_directory_path()/"aquarium-depth-test";std::filesystem::create_directories(directory);
 auto solid=[&](const char* name,Uint8 red,Uint8 blue){
  auto* surface=SDL_CreateSurface(32,32,SDL_PIXELFORMAT_RGBA32);require(surface,"Cannot create test sprite");
  SDL_FillSurfaceRect(surface,nullptr,SDL_MapSurfaceRGBA(surface,red,0,blue,255));const auto path=directory/name;
  require(IMG_SavePNG(surface,path.string().c_str()),"Cannot write test sprite");SDL_DestroySurface(surface);return path.string();
 };
 DecorDef back;back.id="back";back.asset=solid("back.png",0,255);back.width=3;back.height=3;back.art=Json::object();back.layer="Foreground";
 DecorDef front=back;front.id="front";front.asset=solid("front.png",255,0);front.layer="Background";
 auto sample=[&](WorldPoint p){
  auto* raw=SDL_RenderReadPixels(canvas.renderer(),nullptr);require(raw,"Cannot read depth fixture");auto* rgba=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(raw);require(rgba,"Cannot convert depth fixture");
  const auto screen=canvas.toScreen(p);const int x=int(screen.x*rgba->w/canvas.width()),y=int(screen.y*rgba->h/canvas.height());
  const auto* pixel=static_cast<const Uint8*>(rgba->pixels)+y*rgba->pitch+x*4;const std::array<int,3> result{pixel[0],pixel[1],pixel[2]};SDL_DestroySurface(rgba);return result;
 };
 canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,255});canvas.decoration(front,{1,"front",{1},{544,153.6}},0);
 const auto haze=sample({544,135});require(haze[0]>=253&&std::abs(haze[1]-51)<=2&&std::abs(haze[2]-57)<=2,"Haze does not screen the water colour over the sprite");
 auto c=content;c.decorations={front,back};Domain d(c,0);auto state=d.state();state.fish.clear();state.decor={{2,"front",{1},{544,390}},{1,"back",{1},{544,360}}};state.nextDecorId=3;d.install(state);
 canvas.begin();canvas.scene(d,1,0,Tool::Select,{});const auto overlap=sample({544,345});require(overlap[2]>240&&overlap[0]<80,"Foreground decor is drawn behind background decor");
 state.decor={{1,"back",{1},{544,360}}};d.install(state);Decoration ghost{0,"front",{1},{544,390}};
 canvas.begin();canvas.scene(d,1,0,Tool::Select,{},false,{},&ghost);const auto preview=sample({544,345});require(preview[2]>240&&preview[0]<80,"Background preview is drawn above foreground decor");
 state.decor={{1,"front",{1},{544,635}}};d.install(state);
 canvas.begin();canvas.scene(d,1,0,Tool::Select,{});const auto floor=sample({544,610});require(floor[0]>250&&floor[2]<5,"Placed decor is clipped above the bottom of the tank");
 state.decor.clear();d.install(state);ghost.position={544,635};
 canvas.begin();canvas.scene(d,1,0,Tool::Select,{},false,{},&ghost);const auto floorPreview=sample({544,610});require(floorPreview[0]>230&&floorPreview[2]<30,"Preview is clipped above the bottom of the tank");
 std::cout<<"PASS screen-blended haze and shared decor/preview depth order\n";
}
void motionScenes(const Content& c,const std::filesystem::path& assets){
 Canvas canvas(assets,1024,768,true);const auto out=std::filesystem::path("evidence/decor-ui/motion");std::filesystem::create_directories(out);
 for(int frame=0;frame<5;++frame){canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{184,220,224,255});int budget=6;
  int index=0;for(const auto* id:{"PD-12","PL-07","PL-11"}){const auto* d=c.findDecor(id);require(d,"Missing animated fixture");Decoration item{std::uint64_t(index+1),d->id,{1},{180.+index*364.,480.}};item.flipped=frame==4;canvas.decoration(*d,item,frame*6.,frame!=3,1,&budget,true);canvas.text(id,(float(index)+.5f)*canvas.width()/3,canvas.height()*.8f,24,{26,72,81,255},true,200);++index;}
  canvas.text(frame==3?"Reduced motion: authored still poses":"Premium motion: "+std::to_string(frame*6)+" seconds",canvas.width()*.5f,canvas.height()*.1f,28,{26,72,81,255},true,800);
  require(canvas.capture(out/(std::to_string(frame)+".png")),"Animation frame capture failed");
 }
 std::cout<<"PASS clipped internal effects and pinwheel render fixtures\n";
}
int main(int argc,char** argv){try{if(argc<2)return 2;const auto assets=std::filesystem::absolute(argv[1]);std::ifstream f(assets/"content.json");const auto c=Content::fromJson(Json::parse(f));checkMotion(c);std::cout<<"PASS all 120 animation bounds and reduced-motion states\n";for(const auto& [w,h]:{std::pair{852,393},std::pair{667,375},std::pair{1024,768}})viewport(c,assets,w,h);purchaseLifecycle(c,assets);hitLayers(c,assets);depthRendering(c,assets);motionScenes(c,assets);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
