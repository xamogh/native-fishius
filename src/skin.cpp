#include "aquarium/ui_renderer.hpp"
#include "aquarium/theme.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>

namespace aq {
namespace {
constexpr Color white=theme::white,pale=theme::body,navy=theme::ink;
std::string titleCase(std::string s){if(!s.empty())s[0]=char(std::toupper(static_cast<unsigned char>(s[0])));return s;}
std::string scheduleText(Millis ms){
 if(ms>=3600000&&ms%3600000==0)return ms%86400000==0?std::to_string(ms/86400000)+"d":std::to_string(ms/3600000)+"h";
 return durationText(ms);
}
int fishGroup(const Species& s){return s.modelId.starts_with("LE-")?3:s.modelId.starts_with("PF-")?2:1;}
int rarityRank(std::string_view rarity){
 constexpr std::array<std::string_view,6> order{"common","uncommon","rare","epic","premium","limited"};
 auto at=std::find(order.begin(),order.end(),rarity);return int(at-order.begin());
}
std::vector<const Species*> catalog(const Content& content){
 std::vector<const Species*> result;for(const auto& s:content.species)if(s.releaseGate=="Launch"&&s.artReady&&s.level<=40)result.push_back(&s);
 std::stable_sort(result.begin(),result.end(),[](const Species* a,const Species* b){
  if(a->level!=b->level)return a->level<b->level;
  return fishGroup(*a)<fishGroup(*b);
 });return result;
}
std::vector<const Species*> shopFish(const Domain& domain){
 auto list=catalog(domain.content());
 const auto calendar=calendarAt(domain.state().calendarNow);
 std::erase_if(list,[&](const Species* s){return s->releaseGate!="Launch"||!s->artReady||s->level>int(domain.content().levels.size())||(fishGroup(*s)==3&&!eventOpen(*s,calendar));});
 return list;
}
std::vector<const DecorDef*> shopDecor(const Domain& domain,int category){
 std::vector<const DecorDef*> list;
 for(const auto& item:domain.decorations()){
  if(!domain.decorVisible(item)||(item.category=="Plant")!=(category==1))continue;
  if(item.edition=="Limited Edition"){
   const auto& events=domain.content().decorEvents;
   const auto event=std::find_if(events.begin(),events.end(),[&](const auto& e){return e.name==item.event;});
   if(event==events.end()||!event->configured||domain.state().calendarNow<event->startsAt||domain.state().calendarNow>=event->endsAt)continue;
  }
  list.push_back(&item);
 }
 std::stable_sort(list.begin(),list.end(),[](const auto* a,const auto* b){return a->level<b->level;});
 return list;
}
std::string buyLabel(const Species& s,Error blocked){
 switch(blocked){
  case Error::None:return s.currency==Currency::Gift?"Claim":"Buy";
  case Error::Level:return "Level "+std::to_string(s.level);
  case Error::EventClosed:return s.eventConfigured?"Event closed":"Coming soon";
  case Error::Claimed:return "Claimed";
  case Error::Full:return "Tank full";
  case Error::Funds:return s.currency==Currency::Pearls?"Need pearls":"Need coins";
  case Error::NoArt:return "Unavailable";
  default:return "Unavailable";
 }
}
void speciesName(Canvas& canvas,const Species& s,float x,float y,float width,float size=24){
 if(s.name.size()<20){canvas.label(s.name,x,y+10,size,true,width);return;}
 auto middle=s.name.size()/2;auto space=s.name.find(' ',middle);auto before=s.name.rfind(' ',middle);
 if(before!=std::string::npos&&(space==std::string::npos||middle-before<space-middle))space=before;
 if(space==std::string::npos){canvas.label(s.name,x,y+10,size,true,width);return;}
 canvas.label(s.name.substr(0,space),x,y,size,true,width);
 canvas.label(s.name.substr(space+1),x,y+25,size,true,width);
}
}

std::string fishArt(std::string_view id,std::uint64_t){
 return "species/"+std::string(id)+".png";
}

void Canvas::skin(std::string_view name,Rect r,float corner,float alpha){
 (void)corner;
 if(name=="panel")surface("lagoon/panel.png",r,140,std::min(r.w,r.h)*.12f,alpha);
 else if(name=="wood")surface("lagoon/sign.png",r,120,std::min(r.w,r.h)*.38f,alpha);
 else if(name=="green"&&r.w>=r.h*1.65f)surface("lagoon/buy.png",r,90,r.h*.31f,alpha);
 else facet(name,r,alpha);
}
void Canvas::horizontalImage(std::string_view name,Rect r,float sourceLeft,float sourceRight){
 if(r.w<=0||r.h<=0)return;
 auto* t=texture(name);const float scale=r.h/float(t->height);
 const float edgeScale=std::min(scale,r.w/(sourceLeft+sourceRight));
 const float sx[4]{0,sourceLeft,float(t->width)-sourceRight,float(t->width)};
 const float dx[4]{r.x,r.x+sourceLeft*edgeScale,r.x+r.w-sourceRight*edgeScale,r.x+r.w};
 // Stretch the empty middle while preserving the coin and plus button.
 for(int x=0;x<3;++x){
  SDL_FRect src{sx[x],0,sx[x+1]-sx[x],float(t->height)},dst{dx[x]+originX_,r.y+originY_,dx[x+1]-dx[x],r.h};
  if(src.w<=0||dst.w<=0)continue;
  auto* sampled=t->sampled(t->width*dst.w*pixelScaleX_/src.w,r.h*pixelScaleY_);
  const float sampleX=float(sampled->w)/t->width,sampleY=float(sampled->h)/t->height;
  src={src.x*sampleX,0,src.w*sampleX,src.h*sampleY};
  SDL_RenderTexture(renderer_,sampled,&src,&dst);
 }
}
void Canvas::surface(std::string_view name,Rect r,float sourceCorner,float corner,float alpha){
 if(r.w<=0||r.h<=0)return;
 r.x+=originX_;r.y+=originY_;
 auto* t=texture(name);
 const float cut=std::min({sourceCorner,float(t->width)*.499f,float(t->height)*.499f}),edge=std::min({corner,r.w*.5f,r.h*.5f});
 const float sx[4]{0,cut,float(t->width)-cut,float(t->width)},sy[4]{0,cut,float(t->height)-cut,float(t->height)};
 const float dx[4]{r.x,r.x+edge,r.x+r.w-edge,r.x+r.w},dy[4]{r.y,r.y+edge,r.y+r.h-edge,r.y+r.h};
 // Choose a filtered level for each nine-slice patch, including its corners.
 for(int y=0;y<3;++y)for(int x=0;x<3;++x){
  SDL_FRect src{sx[x],sy[y],sx[x+1]-sx[x],sy[y+1]-sy[y]},dst{dx[x],dy[y],dx[x+1]-dx[x],dy[y+1]-dy[y]};
  if(src.w<=0||src.h<=0||dst.w<=0||dst.h<=0)continue;
  auto* sampled=t->sampled(t->width*dst.w*pixelScaleX_/src.w,t->height*dst.h*pixelScaleY_/src.h);
  const float scaleX=float(sampled->w)/t->width,scaleY=float(sampled->h)/t->height;src={src.x*scaleX,src.y*scaleY,src.w*scaleX,src.h*scaleY};
  SDL_SetTextureAlphaModFloat(sampled,alpha);SDL_RenderTexture(renderer_,sampled,&src,&dst);SDL_SetTextureAlphaModFloat(sampled,1);
 }
}
void Canvas::popover(Rect r,SDL_FPoint tip){
 image("fish-popover/frame.png",r);
 const float u=r.w/682.f,half=39.5f*u,inset=82*u;
 const auto sideY=std::clamp(tip.y,r.y+inset,r.y+r.h-inset);
 const auto sideX=std::clamp(tip.x,r.x+inset,r.x+r.w-inset);
 // The painted pointer overlaps the blue rim with its open cream base.
 // Rotate the same source sprite when an edge requires another placement.
 if(tip.x<r.x&&tip.y>=r.y&&tip.y<=r.y+r.h)
  image("fish-popover/pointer.png",{r.x-45*u,sideY-half,64*u,79*u},0,{.5f,.5f},1,true);
 else if(tip.x>=r.x+r.w&&tip.y>=r.y&&tip.y<=r.y+r.h)
  image("fish-popover/pointer.png",{r.x+r.w-19*u,sideY-half,64*u,79*u});
 else{
  const bool above=tip.y<r.y;
  const float edge=above?r.y:r.y+r.h;
  image("fish-popover/pointer.png",{sideX-32*u,edge+(above?-12.f:12.f)*u-half,64*u,79*u},above?-90:90);
 }
}

void Canvas::icon(std::string_view name,Rect r,float alpha,bool flip,Color tint){
 auto* t=texture(name);float scale=std::min(r.w/float(t->width),r.h/float(t->height));
 image(name,{r.x+(r.w-float(t->width)*scale)*.5f,r.y+(r.h-float(t->height)*scale)*.5f,float(t->width)*scale,float(t->height)*scale},0,{.5f,.5f},alpha,flip,tint);
}
void Canvas::label(std::string_view s,float x,float y,float size,bool center,float maxWidth,Color c,float stroke,Color outline){
 if(stroke>0){outline.a=c.a;for(int i=0;i<8;++i){float a=float(i)*.78539816f;text(s,x+std::cos(a)*stroke,y+std::sin(a)*stroke+1,size,outline,center,maxWidth,true);}}
 text(s,x,y,size,c,center,maxWidth,true);
}
void View::touchTarget(std::string id,Rect r,std::function<void()> fn){r.x+=canvas_.origin();r.y+=canvas_.originY();buttons_.push_back({std::move(id),r,std::move(fn)});}
void View::glassButton(std::string id,Rect r,std::string text,std::function<void()> fn,std::string material,float size,std::string art,float alpha,bool interactive,std::optional<Color> textColor){
 const Rect target=r;const float scale=buttonScale(id);r=buttonVisual(id,r);size*=scale;
 canvas_.skin(material,r,std::min(32.f,r.h*.46f),alpha);
 Color color=textColor.value_or((material=="gold"||material=="tab"||material=="nav-card")?theme::ink:white);color.a=Uint8(255*alpha);
 const float textStroke=material=="gold"||material=="tab"||material=="nav-card"?0.f:1.f;
 const Color edge=material=="green"?Color{0,93,53,255}:Color{4,42,119,255};
 if(!art.empty()){
  const float iconSize=r.h*.65f,gap=r.h*.10f,textWidth=canvas_.textWidth(text,size,true);
  const float fit=std::min(1.f,(r.w-18)/(iconSize+gap+textWidth));
  const float start=r.x+(r.w-(iconSize+gap+textWidth)*fit)*.5f;
  canvas_.icon(art,{start,r.y+(r.h-iconSize*fit)*.5f,iconSize*fit,iconSize*fit},alpha);
  canvas_.label(text,start+(iconSize+gap+textWidth*.5f)*fit,r.y+(r.h-size*fit)*.46f-2,size*fit,true,textWidth*fit,color,textStroke,edge);
 }else if(!text.empty())canvas_.label(text,r.x+r.w*.5f,r.y+(r.h-size)*.46f-2,size,true,r.w-14,color,textStroke,edge);
 if(interactive)touchTarget(std::move(id),target,std::move(fn));
}
void View::progress(Rect r,float fraction){canvas_.skin("track",r,r.h*.5f);if(fraction>0)canvas_.skin("green",{r.x+2,r.y+2,std::max(7.f,(r.w-4)*std::clamp(fraction,0.f,1.f)),r.h-4},(r.h-4)*.5f);}

void View::ui(){
 const auto& d=session_.domain();const auto& state=d.state();
 const auto safe=canvas_.safeInsets();const theme::HudLayout l(canvas_);const float u=l.u;
 const float left=l.left,right=l.right,top=l.top,floor=l.floor;
 canvas_.origin(0);
 // Shared HUD: level on the left, both wallets and utility controls on the right.
 const int level=d.level();const auto base=d.content().levels[level-1];
 const auto next=level>=int(d.content().levels.size())?base+1:d.content().levels[level];
 const Rect xp=l.xp();canvas_.skin("nav",xp);
 const float ratio=level>=int(d.content().levels.size())?1.f:std::clamp(float(state.xp-base)/float(next-base),0.f,1.f);
 if(ratio>0)canvas_.skin("cyan",{xp.x+5*u,xp.y+5*u,std::max(10.f,306*ratio)*u,xp.h-10*u});
 const float starY=top-5*u;
 canvas_.icon("skin/icon-star.png",{left+8*u,starY,85*u,88*u});
 canvas_.label(std::to_string(level),left+50*u,starY+27*u,32*u,true,60*u,theme::ink);
 canvas_.label(level>=int(d.content().levels.size())?"MAX LEVEL":compact(state.xp-base)+" / "+compact(next-base)+" XP",xp.x+205*u,xp.y+13*u,23*u,true,209*u,white);
 const auto wallet=[&](bool pearl,float x,float width){
  const auto currency=pearl?Currency::Pearls:Currency::Coins;
  const std::string prefix=pearl?"pearl":"coin";
  const Rect bar{x,top+16*u,width*u,54*u};canvas_.skin("nav",bar);
  canvas_.icon(pearl?"skin/icon-pearl.png":"skin/icon-coin.png",{x-12*u,top+5*u,65*u,66*u});
  canvas_.label(compact(pearl?state.wallet.pearls:state.wallet.coins),x+(width-8)*u*.5f,top+28*u,29*u,true,(width-113)*u,white);
  touchTarget(prefix+"-balance",{x-12*u,top+4*u,(width-48)*u,72*u},[this,currency]{openCurrencyShop(currency);});
  const Rect add{x+(width-57)*u,top+8*u,60*u,62*u};
  const auto plus=buttonVisual(prefix+"-more",add);
  canvas_.round(plus,{112,237,75,255},plus.w*.5f,{0,119,78,255},3*u,true);
  canvas_.round({plus.x+8*u,plus.y+5*u,plus.w-16*u,17*u},{211,255,162,90},9*u,{},0,false);
  canvas_.symbol("add",{add.x+17*u,add.y+18*u,26*u,27*u},{0,106,61,255});
  touchTarget(prefix+"-more",add,[this,currency]{openCurrencyShop(currency);});
 };
 if(uiProject()){
  UiBindings bindings;bindings.values={{"coins",compact(state.wallet.coins)},{"pearls",compact(state.wallet.pearls)}};
  drawUi("currency-hud","default",{right-650*u,top,395*u,77*u},bindings,{{"coins",{"coin-more",{},[this]{openCurrencyShop(Currency::Coins);}}},{"pearls",{"pearl-more",{},[this]{openCurrencyShop(Currency::Pearls);}}},{"coinBalance",{"coin-balance",{},[this]{openCurrencyShop(Currency::Coins);}}},{"pearlBalance",{"pearl-balance",{},[this]{openCurrencyShop(Currency::Pearls);}}}});

 }else{wallet(false,right-650*u,197);wallet(true,right-436*u,181);}
 const Rect mail{right-222*u,top+3*u,83*u,77*u};
 glassButton("notifications",mail,"",[this]{open(Panel::Quests);},"nav");
 canvas_.symbol("mail",{mail.x+23*u,mail.y+22*u,41*u,33*u});
 bool ready=false;for(const auto& q:d.questDefinitions()){auto it=state.quests.find(q.id);if(q.configured&&level>=q.level&&it!=state.quests.end()&&!it->second.claimed&&it->second.count>=q.target)ready=true;}
 if(ready)canvas_.round({mail.x+60*u,mail.y-5*u,27*u,27*u},{255,72,104,255},14*u,{255,155,165,255},2*u,false);
 const Rect settings{right-112*u,top+3*u,83*u,77*u};
 glassButton("menu",settings,"",[this]{open(Panel::Settings);},"nav");canvas_.icon("skin/icon-gear.png",{settings.x+20*u,settings.y+17*u,46*u,46*u});
 const auto nav=[&](std::string id,float py,std::string_view label,std::string_view art,std::function<void()> action){
  const Rect target{right-146*u,l.y+py*u,140*u,118*u};
  const auto r=buttonVisual(id,target);const float scale=r.h/118;
  canvas_.skin("nav-card",r);
  const Rect icon{r.x+36*scale,r.y+10*scale,68*scale,65*scale};
  if(art=="play")canvas_.symbol("play",icon,theme::teal);
  else canvas_.icon(art,icon);
  canvas_.label(label,r.x+r.w*.5f,r.y+80*scale,24*scale,true,r.w-14*scale,theme::ink);
  touchTarget(std::move(id),target,std::move(action));
 };
 nav("nav3",207,"MASTERY","skin/icon-star.png",[this]{open(Panel::Collection);});
 nav("play",339,"PLAY","play",[this]{resetActionTool();open(Panel::None);dismissFunds();});
 nav("tool-food",471,"FOOD","lagoon/food.png",[this]{setTool(tool_==Tool::Food?Tool::Select:Tool::Food);});
 if(panel_!=Panel::Tanks)tankNavigation();
 const Rect shop=l.shop();
 const auto shopVisual=buttonVisual("nav1",shop);canvas_.skin("gold-round",shopVisual);
 const float shopScale=shopVisual.w/200;
 canvas_.icon("lagoon/shop.png",{shopVisual.x+44*shopScale,shopVisual.y+18*shopScale,112*shopScale,98*shopScale});
 canvas_.label("SHOP",shopVisual.x+shopVisual.w*.5f,shopVisual.y+124*shopScale,35*shopScale,true,145*shopScale,theme::ink);
 touchTarget("nav1",shop,[this]{open(Panel::Shop);});
 const Rect bag{left+8*u,floor-246*u,78*u,75*u},sell{left+8*u,floor-334*u,78*u,75*u};
 glassButton("inventory",bag,"",[this]{inventoryCategory_=1;setTool(Tool::Select);open(Panel::Inventory);},"nav");canvas_.icon("ui/inventory.png",{bag.x+15*u,bag.y+12*u,48*u,48*u});
 glassButton("tool-sell",sell,"",[this]{setTool(tool_==Tool::Sell?Tool::Select:Tool::Sell);},tool_==Tool::Sell?"gold":"nav");canvas_.icon("ui/net.png",{sell.x+15*u,sell.y+12*u,48*u,48*u});
 if(panel_!=Panel::None)return;
 if(tool_==Tool::Buy&&!buySpecies_.empty()){
  const auto* species=d.content().find(buySpecies_);if(!species)return;
  const float height=std::max(80.f,canvas_.minimumTouchSize()),doneWidth=std::max(144.f,height*1.4f);
  const std::string hint=species->currency==Currency::Gift?"Tap to place your free "+species->name:"Tap to place "+species->name+". "+price(*species)+" each";
  const float width=std::min(canvas_.width()-safe.left-safe.right-32.f,canvas_.textWidth(hint,26)+doneWidth+60.f);
  const Rect pill{safe.left+(canvas_.width()-safe.left-safe.right-width)*.5f,canvas_.height()-safe.bottom-20.f-height,width,height};
  canvas_.round(pill,{28,62,74,235},height*.5f,{28,62,74,235},0,false);
  canvas_.text(hint,pill.x+20,pill.y+(height-32)*.5f,26,white,false,width-doneWidth-44.f);
  glassButton("done",{pill.x+width-doneWidth,pill.y,doneWidth,height},"DONE",[this]{setTool(Tool::Select);},"green",32);
  return;
 }
 if(tool_!=Tool::Select){
  // Keep an exit from the active tool without an instruction banner.
  const float doneHeight=std::max(80.f,canvas_.minimumTouchSize());
  const float doneWidth=std::max(144.f,doneHeight*1.4f);
  const Rect done{(canvas_.width()-doneWidth)*.5f,canvas_.height()-safe.bottom-20.f-doneHeight,doneWidth,doneHeight};
  glassButton("done",done,"DONE",[this]{setTool(Tool::Select);},"green",32);
 }
}

void View::tankNavigation(){
 const auto& d=session_.domain();const auto& state=d.state();
 const theme::HudLayout l(canvas_);const float u=l.u;
 const Rect tank=l.tank();canvas_.skin("nav",buttonVisual("nav0",tank));
 canvas_.icon("skin/icon-tank.png",{tank.x+63*u,tank.y-46*u,140*u,121*u});
 canvas_.icon("species/ocellarisClownfish.png",{tank.x+100*u,tank.y-6*u,67*u,45*u});
 canvas_.label("Tank "+std::to_string(state.activeTank.value),tank.x+132*u,tank.y+70*u,29*u,true,163*u,white);
 const auto* active=d.tank(state.activeTank);const auto count=d.living(state.activeTank);
 canvas_.label("Growing "+std::to_string(count)+"/"+std::to_string(active?active->slots:0),tank.x+132*u,tank.y+96*u,17*u,true,154*u,theme::cyan);
 canvas_.skin("blue",{tank.x+55*u,tank.y+123*u,154*u,12*u});
 canvas_.label("Display "+std::to_string(d.displaying(state.activeTank))+"/8",tank.x+132*u,tank.y+118*u,17*u,true,154*u,theme::cyan);
 touchTarget("nav0",{tank.x+57*u,tank.y-46*u,154*u,181*u},[this]{open(Panel::Tanks);});
 if(panel_==Panel::Tanks)touchTarget("tank-nav-close",{tank.x+57*u,tank.y-46*u,154*u,181*u},[this]{open(Panel::None);});
 const auto cycle=[this](int delta){const auto& state=session_.domain().state();std::vector<TankId> tanks;for(const auto& t:state.tanks)tanks.push_back(t.id);auto it=std::find(tanks.begin(),tanks.end(),state.activeTank);if(tanks.size()>1){const int i=(int(it-tanks.begin())+delta+int(tanks.size()))%int(tanks.size());command({.action=Action::SwitchTank,.tank=tanks[i]});}else open(Panel::Tanks);};
 for(int i=0;i<2;++i){const Rect arrow{tank.x+(i?215:5)*u,tank.y+44*u,48*u,66*u};glassButton(i?"tank-next":"tank-prev",arrow,"",[cycle,i]{cycle(i?1:-1);},"blue");canvas_.symbol(i?"next":"previous",{arrow.x+8*u,arrow.y+18*u,30*u,30*u});}
}

float View::panelOrigin()const{
 if(panel_==Panel::Details||panel_==Panel::Tanks||panel_==Panel::Shop||panel_==Panel::CurrencyShop||panel_==Panel::Collection)return 0;
 return (canvas_.width()-1608.f)*.5f;
}
float View::panelOriginY()const{
 if(panel_==Panel::Details||panel_==Panel::Shop||panel_==Panel::CurrencyShop||panel_==Panel::Collection||panel_==Panel::Tanks)return 0;
 return (canvas_.height()-830.f)*.5f;
}

void View::panel(){
 if(!panelLayer_)panelLayer_=std::make_unique<Texture>();
 canvas_.beginMenuLayer(*panelLayer_);
 canvas_.origin(panelOrigin(),panelOriginY());
 panelBody();
 canvas_.origin(0);
 canvas_.endMenuLayer();
 panelLayoutRect_=panelRect_;panelLayoutRect_.x+=panelOrigin();panelLayoutRect_.y+=panelOriginY();
 if(panel_!=Panel::Details)panelAnchor_={panelLayoutRect_.x+panelLayoutRect_.w*(panel_==Panel::Tanks?0:.5f),panelLayoutRect_.y+panelLayoutRect_.h*(panel_==Panel::Tanks?1:.55f)};
 panelPose_=menuPose(panelMotion_,panelAnchor_);panelDisplayRect_=panelPose_.apply(panelLayoutRect_);
 for(std::size_t i=panelButtonStart_;i<buttons_.size();++i)buttons_[i].area=panelPose_.apply(buttons_[i].area);
 if(panelPose_.alpha<=0)buttons_.resize(panelButtonStart_);
 if(panel_!=Panel::Details&&panel_!=Panel::Shop&&panel_!=Panel::CurrencyShop&&panel_!=Panel::Tanks)canvas_.fill({0,0,canvas_.width(),canvas_.height()},{0,25,55,Uint8(55*panelPose_.alpha)});
 canvas_.menuLayer(*panelLayer_,panelPose_);paintedPanel_=panel_;layerWidth_=canvas_.width();layerHeight_=canvas_.height();
}

void View::panelBody(){
 if(panel_==Panel::CurrencyShop){currencyShop();return;}
 if(panel_==Panel::Details){details();return;}
 if(panel_==Panel::Tanks){tanks();return;}
 if(panel_==Panel::Shop){
  const theme::ShopLayout l(canvas_);panelRect_=l.rect(62,88,1496,824);
  shop();return;
 }
 switch(panel_){case Panel::Collection:panelRect_={132,96,1387,708};break;case Panel::Settings:panelRect_={209,98,1220,704};break;default:panelRect_={315,142,1040,575};break;}
 if(panel_==Panel::Collection){
  const auto safe=canvas_.safeInsets();
  const float top=std::max({96.f,safe.top+24.f,(canvas_.height()-820.f)*.5f});
  panelRect_={std::max(278.f,(canvas_.width()-1672.f)*.5f+278.f),top,1124.f,std::min(820.f,canvas_.height()-top-26.f)};
 }
 canvas_.skin("panel",panelRect_,60);
 if(panel_==Panel::Collection){collection();return;}if(panel_==Panel::Settings){settings();return;}
 const std::array<const char*,9> titles{"","Tanks","Shop","Inventory","Collection","Settings","My fish","Quests","Gifts"};
 titleSign({panelRect_.x+panelRect_.w*.22f,panelRect_.y-29,panelRect_.w*.56f,90},titles[static_cast<int>(panel_)]);
 closeButton("panel-close",{panelRect_.x+panelRect_.w-65,panelRect_.y+12,50,50},[this]{open(Panel::None);});
 switch(panel_){case Panel::Inventory:inventory();break;case Panel::Details:details();break;case Panel::Quests:quests();break;case Panel::Gifts:gifts();break;default:break;}
}

int View::shopItemsPerPage()const{return 4;}
int View::shopPageCount(int category)const{
 if(category>2)return 1;
 const auto& d=session_.domain();const int count=int(category==0?shopFish(d).size():shopDecor(d,category).size());
 const int perPage=shopItemsPerPage();return std::max(1,(count+perPage-1)/perPage);
}
void View::openLevelUnlock(const LevelUnlock& item){
 int index=-1;
 if(item.tank.value==0){
  // Use the shop's filtered, sorted catalog so seasonal entries cannot shift
  // a link onto the wrong page.
  if(item.category==0){
   const auto entries=shopFish(session_.domain());
   for(std::size_t i=0;i<entries.size();++i)if(entries[i]->id==item.id){index=int(i);break;}
  }else if(item.category==1||item.category==2){
   const auto entries=shopDecor(session_.domain(),item.category);
   for(std::size_t i=0;i<entries.size();++i)if(entries[i]->id==item.id){index=int(i);break;}
  }
  if(index<0)return;
 }
 open(item.tank.value?Panel::Tanks:Panel::Shop);
 if(panel_!=(item.tank.value?Panel::Tanks:Panel::Shop))return;
 // Rewards were already credited by the domain. Browsing an unlock dismisses
 // the notifications and never purchases an item or awards another reward.
 levelUps_.clear();levelUnlockPage_=0;levelAnimation_.motion.show(false,now_);dismissFunds();
 if(item.tank.value){selectedTank_=item.tank.value;highlightedTank_=item.tank;}
 else{category_=item.category;page_=index/shopItemsPerPage();shopHighlightedId_=item.id;shopHighlightedCategory_=item.category;}
}
void View::shop(){
 const auto& d=session_.domain();const theme::ShopLayout l(canvas_);const float u=l.u,v=l.v;
 const auto rect=[&](float x,float y,float w,float h){return l.rect(x,y,w,h);};
 const auto label=[&](std::string_view value,float x,float y,float size,float width,Color color=theme::ink){canvas_.label(value,x,y,size*u,true,width,color);};
 const auto body=[&](std::string_view value,float x,float y,float size,float width){canvas_.text(value,x,y,size*u,theme::body,true,width);};
 canvas_.image("tank-menu/panel.png",panelRect_);
 titleSign(l.art(171,40,460,134),"Shop","lagoon/shop.png");
 canvas_.text("Fish, Plants, Decorations & More!",l.x+659*u,l.y+114*v,30*u,theme::body,false,685*u,true);
 closeButton("panel-close",l.art(1414,74,112,111),[this]{open(Panel::None);});
 canvas_.fill(rect(421,195,2,696),{240,228,209,255});
 canvas_.fill(rect(423,195,1,696),{255,255,255,255});
 constexpr std::array<const char*,5> tabs{"Fish","Plants","Decorations","Environment","Food"};
 constexpr std::array<const char*,5> icons{"species/ocellarisClownfish.png","decor/icons/CP-03.png","decor/icons/CD-07.png","decor/icons/CD-09.png","lagoon/food.png"};
 for(int i=0;i<5;++i){
  const auto target=rect(98,200+i*125,310,112);const auto id="shop-tab"+std::to_string(i);
  const auto r=buttonVisual(id,target);const float scale=r.w/310;
  canvas_.skin(category_==i?"gold":"tab",r);
  canvas_.icon(icons[i],{r.x+17*scale,r.y+(r.h-84*scale)*.5f,86*scale,84*scale},1,i==0);
  canvas_.label(tabs[i],r.x+116*scale,r.y+(r.h-42*scale)*.5f,(i==3?30.f:35.f)*scale,false,r.w-130*scale,theme::ink);
  touchTarget(id,target,[this,i]{category_=i;page_=0;shopHighlightedId_.clear();shopHighlightedCategory_=-1;});
 }
 canvas_.image("lagoon/banner.png",rect(442,188,1088,216));
 constexpr std::array<const char*,5> bannerTop{"Discover amazing","Bring your aquarium","Make a little world","A fresh view for","Happy fish start"};
 constexpr std::array<const char*,5> bannerBottom{"fish for your tanks!","to life with plants!","of your own!","your aquarium!","with a little care!"};
 const int tab=std::clamp(category_,0,4);
 canvas_.label(bannerTop[tab],l.x+1108*u,l.y+232*v,44*u,true,626*u,white,1.5f*u,theme::teal);
 canvas_.label(bannerBottom[tab],l.x+1108*u,l.y+278*v,44*u,true,626*u,white,1.5f*u,theme::teal);
 const Rect grid=rect(459,421,1053,433);pageArea_=grid;pageCount_=shopPageCount(category_);
 const int perPage=shopItemsPerPage();
 if(category_<=2){
  const auto fish=category_==0?shopFish(d):std::vector<const Species*>{};
  const auto decor=category_==0?std::vector<const DecorDef*>{}:shopDecor(d,category_);
  const int count=int(category_==0?fish.size():decor.size()),pages=std::max(1,(count+perPage-1)/perPage);
  page_=std::clamp(page_,0,pages-1);pageCount_=pages;
  const float offset=pageOffset();const int neighbor=std::abs(offset)>.01f?1:0;
  canvas_.clip({grid.x-3*u,grid.y-3*u,grid.w+6*u,grid.h+6*u});
  for(int pg=std::max(0,page_-neighbor);pg<=std::min(pages-1,page_+neighbor);++pg)for(int i=0;i<perPage;++i){
   const int at=pg*perPage+i;if(at>=count)break;
   Rect r=rect(459+i*267,421,252,433);r.x+=offset+float(pg-page_)*grid.w;
   if(r.x+r.w<grid.x-2*u||r.x>grid.x+grid.w+2*u)continue;
   canvas_.image("lagoon/card.png",r);const float cx=r.x+r.w*.5f;
   const bool interactive=pg==page_&&!pageSwiping_&&std::abs(offset)<.5f;
   const Rect buy{r.x+10*u,r.y+338*v,r.w-20*u,77*v};
   if(category_==0){
    const auto& item=*fish[at];const auto gate=d.blocker(item);
    label(item.name,cx,r.y+14*v,31,r.w-27*u);
    canvas_.icon(fishArt(item.id),{r.x+13*u,r.y+66*v,r.w-26*u,165*v});
    if(gate.error==Error::Level)canvas_.icon("skin/icon-lock.png",{r.x+r.w-46*u,r.y+179*v,31*u,38*u});
    const auto offer=d.quote(item);
    body(item.companion?"Permanent display fish":"Adult: "+scheduleText(offer.durationMs)+" while fed",cx,r.y+239*v,22,r.w-14*u);
    body(item.companion?"Uses a free display slot":"Collect: "+std::to_string(offer.principal+offer.profit)+" coins",cx,r.y+270*v,22,r.w-14*u);
    body(item.companion?"No coin or XP production":"Profit: "+std::to_string(offer.profit)+(d.level()<40?" | "+std::to_string(offer.xp)+" XP":""),cx,r.y+301*v,22,r.w-14*u);
    const bool priced=bool(gate)||gate.error==Error::Funds;
    glassButton("buy"+item.id,buy,priced?(item.currency==Currency::Gift?"FREE":compact(item.companion?item.price:offer.principal)):buyLabel(item,gate.error),[this,id=item.id]{
     const auto* s=session_.domain().content().find(id);if(!s)return;const auto blocked=session_.domain().blocker(*s);
     if(blocked)armBuy(id);else if(blocked.error==Error::Funds)showFunds(blocked.shortfall);
    },priced?"green":"disabled",(priced?38.f:25.f)*u,priced?(item.currency==Currency::Gift?"":item.currency==Currency::Pearls?"skin/icon-pearl.png":"skin/icon-coin.png"):(gate.error==Error::Level?"skin/icon-lock.png":""),1,interactive,gate.error==Error::Funds?insufficientFundsColor:white);
   }else{
    const auto& item=*decor[at];const auto gate=d.blocker(item);
    label(item.name,cx,r.y+14*v,29,r.w-27*u);
    canvas_.icon("decor/icons/"+item.id+".png",{r.x+28*u,r.y+64*v,r.w-56*u,166*v});
    body(item.edition=="Limited Edition"?item.event+" event active":item.rarity+" - Level "+std::to_string(item.level),cx,r.y+238*v,23,r.w-14*u);
    body("Tank score: "+std::to_string(item.score),cx,r.y+269*v,25,r.w-14*u);
    const auto xp=d.decorPurchaseXp(item);
    body(xp?"First purchase: "+compact(xp)+" XP":"XP collected",cx,r.y+302*v,22,r.w-14*u);
    const bool priced=bool(gate)||gate.error==Error::Funds;
    std::string title=compact(item.price),icon=item.currency==Currency::Pearls?"skin/icon-pearl.png":"skin/icon-coin.png";
    if(gate.error==Error::Level)title="Level "+std::to_string(item.level);else if(gate.error==Error::EventClosed)title="Event closed";else if(gate.error==Error::Maximum)title="Store an item";else if(gate.error==Error::NoArt)title="Unavailable";
    if(!priced)icon=gate.error==Error::Level?"skin/icon-lock.png":"";
    glassButton("decor-buy"+item.id,buy,title,[this,id=item.id]{const auto* def=session_.domain().content().findDecor(id);if(!def)return;const auto result=session_.domain().blocker(*def);if(result)armDecor(id);else if(result.error==Error::Funds)showFunds(result.shortfall);},priced?"green":"disabled",(priced?38.f:25.f)*u,icon,1,interactive,gate.error==Error::Funds?insufficientFundsColor:white);
   }
   if(category_==shopHighlightedCategory_&&shopHighlightedId_==(category_==0?fish[at]->id:decor[at]->id))canvas_.outline({r.x+4*u,r.y+4*u,r.w-8*u,r.h-8*u},{255,204,41,255},18*u,5*u);
  }
  canvas_.clearClip();
  if(count==0)body("No matching items",grid.x+grid.w*.5f,grid.y+160*v,29,grid.w-80*u);
  const auto arrow=[&](bool next){
   const auto r=l.art(next?1494:423,562,57,68);const auto id=next?"shop-next":"shop-prev";
   glassButton(id,r,"",[this,next,pages]{page_=std::clamp(page_+(next?1:-1),0,pages-1);},"arrow");
   const auto v=buttonVisual(id,r);canvas_.icon("skin/control-next.png",{v.x+16*u,v.y+17*u,25*u,34*u},1,!next);
  };arrow(false);arrow(true);
  const int dots=std::min(pages,4),first=std::clamp(page_-1,0,std::max(0,pages-dots));
  for(int i=0;i<dots;++i){const int target=first+i;const auto r=l.art(890+i*40,869,26,26);canvas_.round(r,target==page_?Color{9,204,250,255}:Color{190,221,226,255},13*u,target==page_?Color{0,151,224,255}:Color{174,209,218,255},2*u,false);}
 }else if(category_==3){
  body("Choose a look for your aquarium. All looks are free.",grid.x+grid.w*.5f,grid.y+5*v,26,grid.w-60*u);
  constexpr std::array<const char*,3> looks{"Lagoon","Sunset","Twilight"};
  for(int i=0;i<3;++i){
   const Rect r=rect(465+i*351,464,334,346);canvas_.skin("card",r);
   label(looks[i],r.x+r.w*.5f,r.y+16*v,32,r.w-30*u);
   const Rect preview{r.x+14*u,r.y+63*v,r.w-28*u,175*v};canvas_.image("lagoon/reef.png",preview);
   if(i==1)canvas_.fill(preview,{232,174,141,37});else if(i==2)canvas_.fill(preview,{57,49,112,65});
   const bool selected=d.state().settings.tankLook==i;
   glassButton("shop-look"+std::to_string(i),{r.x+15*u,r.y+252*v,r.w-30*u,63*v},selected?"Selected":"Apply look",[this,i]{command({Action::SetLook,{}, {},"",{},Currency::Coins,double(i)});},selected?"green":"blue",28*u);
  }
 }else{
  const auto r=rect(459,424,1053,419);canvas_.skin("card",r);
  canvas_.icon("lagoon/food.png",rect(510,499,190,220));
  label("Fish food is free",l.x+1100*u,l.y+473*v,42,690*u);
  body("Tap Food, then tap the water to drop a pellet.",l.x+1100*u,l.y+548*v,25,704*u);
  body("Feed your fish to help them grow.",l.x+1100*u,l.y+595*v,25,704*u);
  glassButton("shop-care",rect(879,690,442,78),"Feed your fish",[this]{setTool(Tool::Food);},"green",33*u);
 }
}

void View::collection(){
 const auto& d=session_.domain();const Rect p=panelRect_;
 auto list=catalog(d.content());
 const auto discovered=std::count_if(list.begin(),list.end(),[&](const auto* s){return std::find(d.state().collected.begin(),d.state().collected.end(),s->id)!=d.state().collected.end();});
 titleSign({p.x+34,p.y-37,586,101},"Collection & Mastery");
 canvas_.text("Keep or rehome adults to record your progress.",p.x+38,p.y+80,22,theme::body,false,p.w-400);
 closeButton("panel-close",{p.x+p.w-91,p.y+26,61,61},[this]{open(Panel::None);});
 canvas_.skin("blue",{p.x+p.w-306,p.y+22,191,79},29);
 canvas_.label(std::to_string(discovered)+" / "+std::to_string(list.size()),p.x+p.w-210,p.y+30,29,true,160,white);
 canvas_.text("Discovered",p.x+p.w-210,p.y+66,19,white,true,170);
 const std::array<const char*,7> filters{"All fish","Common","Uncommon","Rare","Epic","Premium","Limited"};
 const float filterWidth=(p.w-62-48)/7;
 for(int i=0;i<7;++i)glassButton("filter"+std::to_string(i),{p.x+31+i*(filterWidth+8),p.y+119,filterWidth,51},filters[i],[this,i]{collectionFilter_=i;page_=0;},collectionFilter_==i?"gold":"tab",22);
 glassButton("sort",{p.x+32,p.y+183,225,47},sortMode_==0?"Sort: Progression":sortMode_==1?"Sort: Name":"Sort: Rarity",[this]{sortMode_=(sortMode_+1)%3;page_=0;},"blue",21);
 canvas_.text("Adult milestones: 1, 3, 10 and 25. Rewards are being prepared.",p.x+283,p.y+195,22,pale,false,p.w-317);
 if(collectionFilter_>0)std::erase_if(list,[&](const Species* s){return titleCase(s->rarity)!=filters[collectionFilter_];});
 if(sortMode_==1)std::stable_sort(list.begin(),list.end(),[](const Species* a,const Species* b){return a->name<b->name;});
 if(sortMode_==2)std::stable_sort(list.begin(),list.end(),[](const Species* a,const Species* b){return rarityRank(a->rarity)<rarityRank(b->rarity);});
 const int pages=std::max(1,(int(list.size())+7)/8);page_=std::clamp(page_,0,pages-1);
 const float cardWidth=(p.w-64-36)/4,cardHeight=std::max(194.f,(p.h-314)/2);
 for(int i=0;i<8;++i){const int at=page_*8+i;if(at>=int(list.size()))break;
  const auto& s=*list[at];const bool known=std::find(d.state().collected.begin(),d.state().collected.end(),s.id)!=d.state().collected.end();
  const auto mastery=d.masteryProgress(s.id);
  const Rect r{p.x+32+float(i%4)*(cardWidth+12),p.y+242+float(i/4)*(cardHeight+10),cardWidth,cardHeight};
  canvas_.skin(mastery.ready?"panel":"card",r,27,known?1.f:.85f);
  const float lower=cardHeight-194;
  speciesName(canvas_,s,r.x+r.w*.5f,r.y+2,r.w-19,23);
  const float fishWidth=r.w*.43f;
  if(known)canvas_.icon(fishArt(s.id),{r.x+12,r.y+55,fishWidth,79+lower});
  else{
   auto path=fishArt(s.id);path=path.substr(0,path.size()-4)+"-mask.png";
   canvas_.icon(path,{r.x+12,r.y+55,fishWidth,79+lower},.24f,false,theme::ink);
   canvas_.icon("skin/icon-lock.png",{r.x+fishWidth*.5f-2,r.y+71,32,42});
  }
  const float detailX=r.x+fishWidth+24,detailWidth=r.w-fishWidth-36;
  canvas_.text(titleCase(s.rarity)+" / Lv "+std::to_string(s.level),detailX,r.y+59,17,pale,false,detailWidth);
  canvas_.text(known?"Discovered":"Not discovered",detailX,r.y+83,17,pale,false,detailWidth);
  if(!s.companion){
   for(int tier=0;tier<4;++tier)canvas_.icon("skin/icon-star.png",{detailX+tier*29.f,r.y+108,25,25},tier<mastery.tier?1.f:.18f);
   progress({r.x+13,r.y+139+lower,r.w-26,18},mastery.complete?1.f:float(mastery.count)/float(mastery.target));
   canvas_.label("Adults: "+compact(mastery.count)+(mastery.complete?"":" / "+std::to_string(mastery.target)),r.x+r.w*.5f,r.y+137+lower,17,true,r.w-38,white,1);
  }else canvas_.text("Display fish",detailX,r.y+108,17,pale,false,detailWidth);
  canvas_.text(s.companion?"Permanent companion":"Mastery rewards coming soon",r.x+r.w*.5f,r.y+165+lower,17,pale,true,r.w-20);
 }
 if(list.empty())canvas_.label("No fish in this category",p.x+p.w*.5f,p.y+400,31,true,p.w-100);
 const float footerY=p.y+p.h-60;
 glassButton("collection-prev",{p.x+32,footerY,154,44},"Previous",[this]{page_=std::max(0,page_-1);},page_>0?"blue":"nav",22);
 canvas_.label(std::to_string(page_+1)+" / "+std::to_string(pages),p.x+260,footerY+5,27,true,129);
 glassButton("collection-next",{p.x+334,footerY,154,44},"Next",[this,pages]{page_=std::min(pages-1,page_+1);},page_+1<pages?"blue":"nav",22);
 canvas_.text(std::to_string(list.size())+" species in this collection",p.x+p.w-291,footerY+12,21,pale,true,483);
}

void View::settings(){
 auto& d=session_.domain();const auto& s=d.state().settings;
 titleSign({239,63,490,123},"Settings");
 canvas_.text("Make yourself at home.",768,142,28,theme::body,false,519,true);
 closeButton("panel-close",{1331,124,66,66},[this]{open(Panel::None);});
 const std::array<Rect,6> tiles{{{239,219,575,167},{828,219,575,167},{239,398,575,132},{828,398,575,132},{239,539,575,173},{828,539,575,173}}};
 for(const auto& r:tiles)canvas_.skin("card",r,38);
 const std::array<const char*,6> icons{"music","sound","bell","globe","graphics","support"};
 const std::array<const char*,6> titles{"MUSIC","SOUND EFFECTS","NOTIFICATIONS","LANGUAGE","REDUCED MOTION","HELP & SUPPORT"};
 for(int i=0;i<6;++i){auto r=tiles[i];canvas_.icon("skin/icon-"+std::string(icons[i])+".png",{r.x+15,r.y+25,126,r.h-36});canvas_.label(titles[i],r.x+155,r.y+20,32,false,r.w-175);}
 auto toggle=[&](std::string id,Rect r,bool on,std::function<void()> fn){glassButton(std::move(id),r,on?"ON":"OFF",std::move(fn),on?"green":"blue",25);canvas_.icon("skin/icon-pearl.png",{on?r.x+r.w-55:r.x+5,r.y+3,49,r.h-6});};
 canvas_.label("Not available",395,321,26,false,390,pale,0);
 toggle("sound-toggle",{1238,238,144,56},s.sound,[this]{command({Action::SetSound,{}, {},"",{},Currency::Coins,session_.domain().state().settings.sound?0.:1.});});
 toggle("notifications-toggle",{653,417,144,56},notificationOn_,[this]{notificationOn_=!notificationOn_;});
 canvas_.text("Background music is not available.",394,283,19,pale,false,390);canvas_.text("Play sounds for taps, rewards",982,283,19,pale,false,378);canvas_.text("and more",982,305,19,pale,false,378);
 canvas_.text("Get updates about rewards, events",394,465,18,pale,false,396);canvas_.text("and new content",394,486,18,pale,false,396);canvas_.text("Choose your language",982,465,19,pale,false,230);
 const float soundVolume=float(s.volume),sliderX=980,sliderY=334;
 canvas_.icon("skin/control-volume.png",{sliderX,sliderY-4,37,37});progress({sliderX+54,sliderY,269,27},soundVolume);
 canvas_.icon("skin/icon-pearl.png",{sliderX+54+soundVolume*269-22,sliderY-9,47,47});canvas_.label(std::to_string(int(std::round(soundVolume*100)))+"%",sliderX+335,sliderY-4,24,false,76,navy,0);
 touchTarget("sound-volume",{sliderX+48,sliderY-14,282,55},[this,sliderX,shift=canvas_.origin()]{const auto point=panelPose_.unapply({pointerX_,pointerY_});const float value=std::clamp((point.x-shift-sliderX-54)/269.f,0.f,1.f);command({Action::SetVolume,{}, {},"",{},Currency::Coins,double(value)});});
 glassButton("language",{1191,440,190,59},"English",[this]{languageOpen_=!languageOpen_;},"blue",24);canvas_.icon("skin/control-down.png",{1336,458,28,28});
 canvas_.text("Keep decorations still and reduce",395,594,19,pale,false,390);canvas_.text("menu and tool animation.",395,618,19,pale,false,390);canvas_.text("Need help? We're here for you!",982,600,20,pale,false,395);
 toggle("reduced-motion-toggle",{653,649,144,50},s.reducedMotion,[this]{command({Action::SetReducedMotion,{}, {},"",{},Currency::Coins,session_.domain().state().settings.reducedMotion?0.:1.});});
 glassButton("support",{982,639,241,59},"Contact Support",[this]{showHelp();},"blue",24);glassButton("faq",{1237,639,146,59},"FAQ",[this]{showHelp();},"blue",25);
 canvas_.text("Game Version 1.0.0",790,735,19,pale,true,500);canvas_.text("Play, Relax, Build Your Dream Aquarium!",790,759,18,pale,true,650);
 canvas_.text("Privacy Policy   |   Terms of Service",1166,741,18,pale,true,410);touchTarget("legal",{993,736,367,34},[this]{showHelp();});
 canvas_.icon("skin/icon-coral.png",{180,682,142,142});canvas_.icon("skin/icon-coral.png",{1358,684,108,132});
 if(languageOpen_){canvas_.skin("panel",{1190,501,191,67},22);canvas_.label("English",1285,518,23,true,164);touchTarget("language-english",{1190,501,191,67},[this]{languageOpen_=false;});}
}
} // namespace aq
