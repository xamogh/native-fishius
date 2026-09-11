#include "aquarium/view.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>

namespace aq {
namespace {
constexpr Color white{250,254,255,255},pale{186,244,255,255},navy{5,62,113,255};
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
 std::vector<const Species*> result;for(const auto& s:content.species)result.push_back(&s);
 std::stable_sort(result.begin(),result.end(),[](const Species* a,const Species* b){
  if(fishGroup(*a)!=fishGroup(*b))return fishGroup(*a)<fishGroup(*b);
  return a->level<b->level;
 });return result;
}
std::string eventDates(const Species& s){
 if(!s.eventConfigured)return "Dates coming soon";
 constexpr std::array<const char*,12> months{"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
 auto date=[&](int day){return std::string(months[std::clamp(day/100-1,0,11)])+" "+std::to_string(day%100);};
 return s.eventStart?date(s.eventStart)+" - "+date(s.eventEnd):"";
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
 if(r.w<=0||r.h<=0)return;
 r.x+=originX_;r.y+=originY_;
 auto* t=texture("skin/"+std::string(name)+".png");
 const float cut=86,edge=std::min({corner,r.w*.5f,r.h*.5f});
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
 SDL_FPoint a{},b{};
 if(tip.y<r.y){
  const float x=std::clamp(tip.x,r.x+36,r.x+r.w-36);
  // Outside the raised title, connect to the lower top edge of the body.
  const bool onTitle=x-20>=r.x+r.w*.205f&&x+20<=r.x+r.w*.79f;
  const float y=r.y+(onTitle?5.f:r.h*.086f+5.f);a={x-20,y};b={x+20,y};
 }
 else if(tip.y>r.y+r.h){const float x=std::clamp(tip.x,r.x+36,r.x+r.w-36);a={x-20,r.y+r.h-5};b={x+20,r.y+r.h-5};}
 else if(tip.x<r.x){const float y=std::clamp(tip.y,r.y+36,r.y+r.h-36);a={r.x+5,y-20};b={r.x+5,y+20};}
 else{const float y=std::clamp(tip.y,r.y+36,r.y+r.h-36);a={r.x+r.w-5,y-20};b={r.x+r.w-5,y+20};}
 const SDL_FPoint centre{(a.x+b.x)*.5f,(a.y+b.y)*.5f};
 auto triangle=[&](SDL_FPoint x,SDL_FPoint y,SDL_FPoint z,SDL_FColor color){
  x.x+=originX_;y.x+=originX_;z.x+=originX_;x.y+=originY_;y.y+=originY_;z.y+=originY_;
  const std::array<SDL_Vertex,3> v{{{x,color,{}},{y,color,{}},{z,color,{}}}};
  SDL_RenderGeometry(renderer_,nullptr,v.data(),3,nullptr,0);
 };
 triangle(a,b,tip,{1.f,.60f,0.f,1.f});
 const float length=std::max(1.f,std::hypot(tip.x-centre.x,tip.y-centre.y));
 auto inset=[&](float margin,float width,SDL_FColor color){
  const SDL_FPoint insetTip{tip.x+(centre.x-tip.x)*margin/length,tip.y+(centre.y-tip.y)*margin/length};
  triangle({a.x+(centre.x-a.x)*width,a.y+(centre.y-a.y)*width},{b.x+(centre.x-b.x)*width,b.y+(centre.y-b.y)*width},insetTip,color);
 };
 inset(2,.10f,{1.f,.94f,.25f,1});
 inset(5,.25f,{.08f,.94f,1.f,1});
 inset(7,.36f,{0,.63f,.89f,1});
 image("details/popover-frame.png",r);
}
void Canvas::icon(std::string_view name,Rect r,float alpha){
 auto* t=texture(name);float scale=std::min(r.w/float(t->width),r.h/float(t->height));
 image(name,{r.x+(r.w-float(t->width)*scale)*.5f,r.y+(r.h-float(t->height)*scale)*.5f,float(t->width)*scale,float(t->height)*scale},0,{.5f,.5f},alpha);
}
void Canvas::label(std::string_view s,float x,float y,float size,bool center,float maxWidth,Color c,float stroke){
 if(stroke>0){for(int i=0;i<8;++i){float a=float(i)*.78539816f;text(s,x+std::cos(a)*stroke,y+std::sin(a)*stroke+1,size,{3,62,121,c.a},center,maxWidth,true);}}
 text(s,x,y,size,c,center,maxWidth,true);
}
void View::touchTarget(std::string id,Rect r,std::function<void()> fn){r.x+=canvas_.origin();r.y+=canvas_.originY();buttons_.push_back({std::move(id),r,std::move(fn)});}
void View::glassButton(std::string id,Rect r,std::string text,std::function<void()> fn,std::string material,float size,std::string art,float alpha,bool interactive){
 const Rect target=r;const float scale=buttonScale(id);r=buttonVisual(id,r);size*=scale;
 canvas_.skin(material,r,std::min(32.f,r.h*.46f),alpha);
 if(!art.empty()){float a=r.h*.74f;canvas_.icon(art,{r.x+9,r.y+(r.h-a)*.5f,a,a},alpha);}
 Color color=material=="gold"?Color{100,46,0,255}:white;color.a=Uint8(255*alpha);
 if(!text.empty())canvas_.label(text,r.x+r.w*.5f+(art.empty()?0:r.h*.22f),r.y+(r.h-size)*.46f-2,size,true,r.w-(art.empty()?14:r.h+8),color,material=="gold"?0:2);
 if(interactive)touchTarget(std::move(id),target,std::move(fn));
}
void View::progress(Rect r,float fraction){canvas_.skin("blue",r,r.h*.5f);if(fraction>0)canvas_.skin("green",{r.x+2,r.y+2,std::max(7.f,(r.w-4)*std::clamp(fraction,0.f,1.f)),r.h-4},(r.h-4)*.5f);}

void View::ui(){
 const auto& d=session_.domain();const auto& state=d.state();
 const auto safe=canvas_.safeInsets();
 const float extra=canvas_.width()-1608.f;
 // The reference positions are in a 1608 x 830 content area. Keep the
 // controls at their original size and anchor them to each safe screen edge.
 const float left=std::max(0.f,safe.left-90.f);
 const bool cutoutRight=SDL_GetCurrentDisplayOrientation(SDL_GetDisplayForWindow(canvas_.window()))==SDL_ORIENTATION_LANDSCAPE_FLIPPED;
 const float right=extra-(cutoutRight?std::max(0.f,safe.right-8.f):0.f);
 const float bottom=std::max(0.f,safe.bottom-8.f);
 const float top=std::max({0.f,safe.top-8.f,canvas_.height()>=1000?36.f:0.f}),extraHeight=canvas_.height()-830.f;
 canvas_.origin(0,top);
 auto art=[&](std::string_view name,Rect rect){canvas_.image("aquarium/"+std::string(name)+".png",rect);};
 auto control=[&](std::string id,std::string_view name,Rect rect,std::function<void()> action){
  Rect visual=buttonVisual(id,rect);
  art(name,visual);touchTarget(std::move(id),rect,std::move(action));
 };
 art(state.wallet.coins==250?"coins":"coins-empty",{38+left,9,209,73});
 if(state.wallet.coins!=250)canvas_.label(compact(state.wallet.coins),137+left,28,27,true,75,white,1.3f);
 touchTarget("coin-more",{183+left,16,60,62},[this]{open(Panel::Gifts);});
 art(state.wallet.pearls==0?"pearls":"pearls-empty",{247+left,9,207,74});
 if(state.wallet.pearls!=0)canvas_.label(compact(state.wallet.pearls),331+left,28,27,true,65,white,1.3f);
 touchTarget("pearl-more",{392+left,16,60,62},[this]{open(Panel::Gifts);});
 int level=d.level();auto base=d.content().levels[level-1];auto next=level>=40?base+1:d.content().levels[level];
 if(state.xp==14)art("level",{1160+right,3,270,90});
 else{
  progress({1202+right,26,226,39},level>=40?1.f:float(state.xp-base)/float(next-base));
  canvas_.icon("skin/icon-star.png",{1160+right,3,94,90});
  canvas_.label("Lv "+std::to_string(level),1207+right,31,26,true,70,white,1.5f);
  canvas_.label(level>=40?"MAX LEVEL":compact(state.xp-base)+" / "+compact(next-base)+" XP",1360+right,35,17,true,130,white,1);
 }
 glassButton("notifications",{1434+right,14,74,76},"",[this]{open(Panel::Quests);},"nav");
 canvas_.icon("skin/control-bell.png",{1453+right,30,37,44});
 int readyQuests=0;for(const auto& q:d.questDefinitions()){auto it=state.quests.find(q.id);if(q.configured&&level>=q.level&&it!=state.quests.end()&&!it->second.claimed&&it->second.count>=q.target)++readyQuests;}
 if(readyQuests){canvas_.round({1484+right,12,26,26},{245,64,47,255},13,{255,155,88,255},1,false);canvas_.label(std::to_string(readyQuests),1497+right,13,19,true,23,white,0);}
 control("menu","menu",{1509+right,14,72,76},[this]{open(Panel::Settings);});
 canvas_.origin(0);
 control("nav0","tanks",{0+left,688+extraHeight-bottom,304,142},[this]{open(Panel::Tanks);});
 const float shopY=680+extraHeight-std::max(0.f,bottom-20.f);
 control("nav1","shop",{1216+right,shopY,360,150},[this]{open(Panel::Shop);});
 if(panel_!=Panel::None&&panel_!=Panel::Tanks&&panel_!=Panel::Shop&&panel_!=Panel::Details)return;
 // Keep tablet tools together above Shop instead of spreading them over the water.
 const bool tablet=canvas_.height()>=1000;
 const float toolGap=tablet?12.f:24+std::max(0.f,extraHeight-top-bottom)/3;
 const float stackHeight=134+3*132+3*toolGap;
 const float selectY=tablet?shopY-12-stackHeight:91+top,foodY=selectY+134+toolGap;
 const float masteryY=foodY+132+toolGap,sellY=masteryY+132+toolGap;
 control("tool-select","select",{1477+right,selectY,127,134},[this]{
  if(tool_==Tool::Select&&panel_==Panel::None)selectMenu(!selectMenuOpen_);else setTool(Tool::Select);
 });
 control("tool-food","food",{1477+right,foodY,127,132},[this]{setTool(Tool::Food);});
 control("nav3","mastery",{1477+right,masteryY,127,132},[this]{open(Panel::Collection);});
 control("tool-sell","sell",{1477+right,sellY,127,132},[this]{setTool(Tool::Sell);});
 if(panel_!=Panel::None)return;
 if(selectMenuOpen_||(!reducedMotion()&&std::any_of(selectMotion_.begin(),selectMotion_.end(),[&](const auto& motion){return motion.sample(now_).value>.001;}))){
  constexpr std::array<Tool,3> targets{Tool::Move,Tool::Stash,Tool::Restore};
  constexpr std::array<const char*,3> labels{"MOVE","STASH","BAG"};
  for(int i=0;i<3;++i){
   const Rect rect{1300+right,selectY+14+i*61,170,55};
   auto pose=menuPose(selectMotion_[i],{rect.x+rect.w,rect.y+rect.h*.5f});pose.rise=0;
   if(pose.alpha<=0)continue;
   glassButton("select-option"+std::to_string(i),pose.apply(rect),labels[i],[this,i,target=targets[i]]{closeSelectMenu();if(i==2)open(Panel::Inventory);else setTool(target);},"blue",24*pose.yScale,"",pose.alpha,selectMenuOpen_);
  }
 }
 if(tool_!=Tool::Select){
  const char* instruction=tool_==Tool::Food?"Tap the water to feed":tool_==Tool::Sell?"Tap a fish to sell":tool_==Tool::Medicine?"Tap a fish to care for it":tool_==Tool::Move?"Drag a fish to move it":tool_==Tool::Stash?"Tap a fish to store it":"Tap the water to place";
  const float centre=canvas_.width()*.5f;
  canvas_.skin("blue",{centre-205,744+extraHeight-bottom,410,57},28);
  canvas_.label(instruction,centre-30,755+extraHeight-bottom,21,true,318,white,1);
  glassButton("done",{centre+132,749+extraHeight-bottom,67,46},"DONE",[this]{setTool(Tool::Select);},"green",16);
 }
}

float View::panelOrigin()const{
 if(panel_==Panel::Details)return 0;
 // The Tanks artwork starts at x=111, including its coral. Leave a 24-unit
 // gap after the full 304-unit Tanks button, independent of screen width.
 if(panel_==Panel::Tanks)return std::max(0.f,canvas_.safeInsets().left-90.f)+304.f+24.f-111.f;
 return (canvas_.width()-1608.f)*.5f;
}
float View::panelOriginY()const{
 if(panel_==Panel::Details||panel_==Panel::Shop||panel_==Panel::Collection)return 0;
 if(panel_==Panel::Tanks)return std::max(0.f,canvas_.height()-830.f-std::max(0.f,canvas_.safeInsets().bottom-8.f));
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
 if(panel_==Panel::Shop)canvas_.fill({0,0,canvas_.width(),canvas_.height()},{0,25,55,Uint8(120*panelPose_.alpha)});
 canvas_.menuLayer(*panelLayer_,panelPose_);paintedPanel_=panel_;layerWidth_=canvas_.width();layerHeight_=canvas_.height();
}

void View::panelBody(){
 if(panel_==Panel::Details){details();return;}
 switch(panel_){case Panel::Tanks:panelRect_={122,92,376,710};break;case Panel::Shop:panelRect_={146,97,1394,710};break;case Panel::Collection:panelRect_={132,96,1387,708};break;case Panel::Settings:panelRect_={209,98,1220,704};break;default:panelRect_={315,142,1040,575};break;}
 if(panel_==Panel::Shop||panel_==Panel::Collection)panelRect_.h+=canvas_.height()-830.f;
 if(canvas_.height()>=1000&&(panel_==Panel::Shop||panel_==Panel::Collection)){
  const auto safe=canvas_.safeInsets();const float top=std::max(70.f,safe.top+32),bottom=std::max(45.f,safe.bottom+24);
  panelRect_={70,top,1468,canvas_.height()-top-bottom};
 }
 canvas_.skin("panel",panelRect_,60);
 if(panel_==Panel::Tanks){tanks();return;}if(panel_==Panel::Shop){shop();return;}if(panel_==Panel::Collection){collection();return;}if(panel_==Panel::Settings){settings();return;}
 const std::array<const char*,9> titles{"","TANKS","SHOP","INVENTORY","COLLECTION","SETTINGS","MY FISH","QUESTS","GIFTS"};
 canvas_.label(titles[static_cast<int>(panel_)],panelRect_.x+panelRect_.w*.5f,panelRect_.y+12,36,true,panelRect_.w-120);
 glassButton("panel-close",{panelRect_.x+panelRect_.w-65,panelRect_.y+12,50,50},"",[this]{open(Panel::None);},"nav");canvas_.icon("skin/control-close.png",{panelRect_.x+panelRect_.w-53,panelRect_.y+24,26,26});
 switch(panel_){case Panel::Inventory:inventory();break;case Panel::Details:details();break;case Panel::Quests:quests();break;case Panel::Gifts:gifts();break;default:break;}
}

void View::tanks(){
 const auto& d=session_.domain();canvas_.label("TANKS",164,111,38,false,225);
 glassButton("panel-close",{425,113,49,49},"",[this]{open(Panel::None);},"nav");canvas_.icon("skin/control-close.png",{437,125,25,25});
 constexpr std::array<const char*,5> names{"Starter Tank","Reef Tank","Deep Tank","Zen Tank","Aqua Palace"};
 for(int i=0;i<5;++i){int id=i+1;const auto* tank=d.tank({id});bool active=tank&&d.state().activeTank.value==id;Rect r{141,i==0?172.f:301.f+float(i-1)*96,335,i==0?121.f:88.f};
  if(active)canvas_.skin("green",{r.x-3,r.y-3,r.w+6,r.h+6},30);
  canvas_.skin(active?"panel":"card",r,27,active?1.f:.96f);
  canvas_.icon("skin/tank-"+std::to_string(id)+".png",{r.x+10,r.y+6,119,r.h-12});
  canvas_.label(names[i],r.x+143,r.y+20,23,false,183);
  canvas_.text(active?std::to_string(d.living({id}))+" / "+std::to_string(tank->slots)+" fish":tank?std::to_string(tank->slots)+" fish capacity":"Level "+std::to_string(d.content().tankLevels[i]),r.x+143,r.y+51,17,white,false,tank?178:128);
  touchTarget("tank-select"+std::to_string(id),r,[this,id,tank]{if(tank){command({Action::SwitchTank,{}, {id}});open(Panel::None);}else{selectedTank_=id;}});
  if(active)glassButton("tank-current",{r.x+215,r.y+73,107,38},tank->slots<40?"Expand":"Current",[this,id,tank]{if(tank->slots<40)selectedTank_=id;else open(Panel::None);},"green",20);
  else if(!tank)canvas_.icon("skin/icon-lock.png",{r.x+281,r.y+24,43,45});
 }
 int choice=selectedTank_;if(choice<1||choice>5)choice=1;
 const auto* chosen=d.tank({choice});const bool maximum=chosen&&chosen->slots>=40;
 const int step=chosen?std::min(3,chosen->slots/10):0;
 const Amount coins=d.content().tankCosts[choice-1][step],tokens=d.content().tankTokens[choice-1][step];
 const bool levelLocked=d.level()<d.content().tankLevels[choice-1];
 const std::string label=maximum?"Full capacity":levelLocked?"Level "+std::to_string(d.content().tankLevels[choice-1]):compact(coins)+" + "+compact(tokens)+" GT";
 glassButton("tank-buy",{176,683,273,56},label,[this,choice,chosen]{command({chosen?Action::ExpandTank:Action::UnlockTank,{}, {choice},"",{},Currency::Coins});},maximum||levelLocked?"blue":"green",23,maximum||levelLocked?"":"skin/icon-coin.png");
 canvas_.text(std::string(chosen?"Expand ":"Unlock ")+names[choice-1],313,743,17,pale,true,258);
 canvas_.text("Gift Tokens: "+compact(d.state().giftTokens),313,767,16,pale,true,240);
 canvas_.icon("skin/icon-coral.png",{111,687,74,116});canvas_.icon("skin/icon-coral.png",{434,695,71,108});
}

void View::shop(){
 const auto& d=session_.domain();const Rect p=panelRect_;
 canvas_.icon("skin/icon-shop.png",{p.x+32,p.y+15,94,94});
 canvas_.label("SHOP",p.x+144,p.y+19,46,false,470);
 canvas_.text("Choose a fish that fits your aquarium and schedule.",p.x+144,p.y+76,22,white,false,p.w-610);
 canvas_.skin("blue",{p.x+p.w-424,p.y+24,302,73},28);
 canvas_.icon("skin/icon-coin.png",{p.x+p.w-407,p.y+42,33,33});
 canvas_.label(compact(d.state().wallet.coins),p.x+p.w-361,p.y+40,25,false,133);
 canvas_.icon("skin/icon-pearl.png",{p.x+p.w-223,p.y+42,33,33});
 canvas_.label(compact(d.state().wallet.pearls),p.x+p.w-181,p.y+40,25,false,51);
 glassButton("panel-close",{p.x+p.w-90,p.y+25,61,61},"",[this]{open(Panel::None);},"nav");
 canvas_.icon("skin/control-close.png",{p.x+p.w-74,p.y+41,29,29});
 const std::array<const char*,4> tabs{"Fish","Food","Decor","Boosts"};
 const std::array<const char*,4> icons{"skin/icon-fish.png","ui/food.png","skin/icon-coral.png","skin/control-bolt.png"};
 const float tabWidth=(p.w-80)/4;
 for(int i=0;i<4;++i)glassButton("shop-tab"+std::to_string(i),{p.x+28+i*(tabWidth+8),p.y+117,tabWidth,64},tabs[i],[this,i]{category_=i;page_=0;},category_==i?"gold":"blue",28,icons[i]);
 const bool tabletGrid=canvas_.height()>=1000&&p.h>=865;
 const int columns=tabletGrid?4:6,rows=tabletGrid?2:1,perPage=columns*rows;
 const float cardsY=p.y+263,cardHeight=tabletGrid?(p.h-385)/2:326;
 int pages=1;
 if(category_==0){
  auto list=catalog(d.content());if(shopFilter_>0)std::erase_if(list,[&](const Species* s){return fishGroup(*s)!=shopFilter_;});
  const std::array<const char*,4> groups{"All fish","Coin fish","Premium","Limited"};
  for(int i=0;i<4;++i)glassButton("shop-filter"+std::to_string(i),{p.x+30+i*203.f,p.y+199,194,48},groups[i],[this,i]{shopFilter_=i;page_=0;},shopFilter_==i?"green":"blue",23);
  // Keep this right-aligned summary independent of name and price lengths.
  canvas_.text(std::to_string(list.size())+" species   |   Level "+std::to_string(d.level()),p.x+p.w-235,p.y+211,21,white,true,365);
  pages=std::max(1,(int(list.size())+perPage-1)/perPage);page_=std::clamp(page_,0,pages-1);
  const float cardWidth=(p.w-60-10*(columns-1))/columns;
  for(int i=0;i<perPage;++i){const int at=page_*perPage+i;if(at>=int(list.size()))break;
   const auto& s=*list[at];const auto gate=d.blocker(s);const bool available=bool(gate);
   const Rect r{p.x+30+(i%columns)*(cardWidth+10),cardsY+(i/columns)*(cardHeight+12),cardWidth,cardHeight};
   const float vertical=cardHeight/326.f;
   const float typeScale=std::min(1.f,vertical);
   canvas_.skin("card",r,27,available?1.f:.9f);
   speciesName(canvas_,s,r.x+r.w*.5f,r.y+9*vertical,r.w-16);
   canvas_.icon(fishArt(s.id),{r.x+17,r.y+64*vertical,r.w-34,109*vertical},available?1.f:.72f);
   if(gate.error==Error::Level)canvas_.icon("skin/icon-lock.png",{r.x+r.w-43,r.y+117*vertical,31,42});
   canvas_.text(titleCase(s.rarity)+" / Lv "+std::to_string(s.level),r.x+r.w*.5f,r.y+179*vertical,19*typeScale,pale,true,r.w-18);
   canvas_.text("Stage "+scheduleText(s.stageMs)+" / Feed "+scheduleText(s.feedMs),r.x+r.w*.5f,r.y+208*vertical,18*typeScale,white,true,r.w-18);
   if(s.eventStart||!s.eventConfigured)canvas_.text(eventDates(s),r.x+r.w*.5f,r.y+231*vertical,17*typeScale,pale,true,r.w-16);
   else canvas_.text(s.role,r.x+r.w*.5f,r.y+231*vertical,17*typeScale,pale,true,r.w-16);
   const auto amount=s.currency==Currency::Gift?"FREE":compact(s.price);
   if(s.currency!=Currency::Gift)canvas_.icon(s.currency==Currency::Pearls?"skin/icon-pearl.png":"skin/icon-coin.png",{r.x+22,r.y+253*vertical,28*typeScale,28*typeScale});
   canvas_.label(amount,r.x+r.w*.5f+(s.currency==Currency::Gift?0:10),r.y+252*vertical,26*typeScale,true,r.w-73);
   glassButton("buy"+s.id,{r.x+13,r.y+283*vertical,r.w-26,36*vertical},buyLabel(s,gate.error),[this,id=s.id]{
    const auto* item=session_.domain().content().find(id);if(!item)return;
    const auto current=session_.domain().blocker(*item);
    if(current)armBuy(id);else toast(current.error==Error::Level?"Unlocks at Level "+std::to_string(item->level):current.error==Error::EventClosed?item->name+": "+eventDates(*item):errorText(current.error));
   },available?"green":"blue",23*typeScale);
  }
  if(list.empty())canvas_.label("No fish in this category",p.x+p.w*.5f,cardsY+120,32,true,p.w-90);
 }else if(category_==2){
  canvas_.text("Decorate your tank. Place each item after choosing it.",p.x+35,p.y+211,24,white,false,p.w-70);
  const float count=float(d.decorations().size()),cardWidth=(p.w-60-12*std::max(0.f,count-1))/std::max(1.f,count);
  int i=0;for(const auto& decor:d.decorations()){
   Rect r{p.x+30+i*(cardWidth+12),cardsY,cardWidth,cardHeight};canvas_.skin("card",r,28);
   canvas_.label(decor.name,r.x+r.w*.5f,r.y+15,25,true,r.w-20);
   canvas_.icon("decor/"+decor.id+".png",{r.x+38,r.y+55,r.w-76,r.h-135});
   const bool affordable=d.state().wallet.coins>=decor.price;
   glassButton("decor-buy"+decor.id,{r.x+18,r.y+r.h-62,r.w-36,48},compact(decor.price)+" Coins",[this,id=decor.id,cost=decor.price]{if(session_.domain().state().wallet.coins>=cost)armDecor(id);else toast("Not enough coins.");},affordable?"green":"blue",23);++i;
  }
 }else{
  canvas_.skin("card",{p.x+30,cardsY,p.w-60,cardHeight},40);
  canvas_.icon(category_==1?"ui/food.png":"skin/control-bolt.png",{p.x+110,cardsY+60,148,156});
  canvas_.label(category_==1?"Fish food is free":"Growth follows each fish's schedule",p.x+304,cardsY+46,36,false,p.w-365);
  canvas_.text(category_==1?"Tap Food, then tap the water to drop a pellet.":"Feed on time to keep your fish healthy as they grow.",p.x+304,cardsY+109,24,white,false,p.w-365);
  canvas_.text(category_==1?"Each fish has its own feeding interval, shown on its Shop card.":"There are no growth boosts for sale.",p.x+304,cardsY+151,22,pale,false,p.w-365);
  glassButton("shop-care",{p.x+304,cardsY+231,360,63},category_==1?"Feed your fish":"Return to aquarium",[this]{if(category_==1)setTool(Tool::Food);else open(Panel::None);},"green",27);
 }
 const float footerY=p.y+p.h-95;
 if(category_==0){
  glassButton("shop-previous",{p.x+31,footerY,152,59},"Previous",[this]{page_=std::max(0,page_-1);},page_>0?"blue":"nav",23);
  canvas_.label(std::to_string(page_+1)+" / "+std::to_string(pages),p.x+263,footerY+14,27,true,139);
  glassButton("shop-next",{p.x+343,footerY,152,59},"Next",[this,pages]{page_=std::min(pages-1,page_+1);},page_+1<pages?"blue":"nav",23);
  canvas_.text("Buy, then tap the water to place an egg.",p.x+766,footerY+18,22,white,true,480);
 }
 glassButton("offers",{p.x+p.w-309,footerY,277,59},"Limited fish",[this]{category_=0;shopFilter_=3;page_=0;},"gold",25,"skin/icon-star.png");
}

void View::collection(){
 const auto& d=session_.domain();const Rect p=panelRect_;
 canvas_.icon("skin/icon-book.png",{p.x+27,p.y+22,92,83});
 canvas_.label("COLLECTION & MASTERY",p.x+139,p.y+21,39,false,748);
 canvas_.text("Discover every species. Raise fish to Adult to earn mastery badges.",p.x+140,p.y+77,22,white,false,945);
 glassButton("panel-close",{p.x+p.w-91,p.y+26,61,61},"",[this]{open(Panel::None);},"nav");
 canvas_.icon("skin/control-close.png",{p.x+p.w-75,p.y+42,29,29});
 canvas_.skin("blue",{p.x+p.w-306,p.y+22,191,79},29);
 canvas_.label(std::to_string(d.state().collected.size())+" / "+std::to_string(d.content().species.size()),p.x+p.w-210,p.y+30,29,true,160);
 canvas_.text("Discovered",p.x+p.w-210,p.y+66,19,white,true,170);
 const std::array<const char*,7> filters{"All fish","Common","Uncommon","Rare","Epic","Premium","Limited"};
 const float filterWidth=(p.w-62-48)/7;
 for(int i=0;i<7;++i)glassButton("filter"+std::to_string(i),{p.x+31+i*(filterWidth+8),p.y+119,filterWidth,51},filters[i],[this,i]{collectionFilter_=i;page_=0;},collectionFilter_==i?"green":"blue",22);
 glassButton("sort",{p.x+32,p.y+183,225,47},sortMode_==0?"Sort: Progression":sortMode_==1?"Sort: Name":"Sort: Rarity",[this]{sortMode_=(sortMode_+1)%3;page_=0;},"blue",21);
 canvas_.text("Mastery goals: raise 5, 25 and 100 Adults of each species.",p.x+283,p.y+195,22,pale,false,p.w-317);
 auto list=catalog(d.content());
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
   canvas_.icon(path,{r.x+12,r.y+55,fishWidth,79+lower},.3f);
   canvas_.icon("skin/icon-lock.png",{r.x+fishWidth*.5f-2,r.y+71,32,42});
  }
  const float detailX=r.x+fishWidth+24,detailWidth=r.w-fishWidth-36;
  canvas_.text(titleCase(s.rarity)+" / Lv "+std::to_string(s.level),detailX,r.y+59,17,pale,false,detailWidth);
  canvas_.text(known?"Discovered":"Not discovered",detailX,r.y+83,17,white,false,detailWidth);
  for(int tier=0;tier<3;++tier)canvas_.icon("skin/icon-star.png",{detailX+tier*29.f,r.y+108,25,25},tier<mastery.tier?1.f:.18f);
  progress({r.x+13,r.y+139+lower,r.w-26,18},mastery.complete?1.f:float(mastery.count)/float(mastery.target));
  canvas_.label("Adults: "+compact(mastery.count)+(mastery.complete?"":" / "+std::to_string(mastery.target)),r.x+r.w*.5f,r.y+137+lower,17,true,r.w-38,white,1);
  if(mastery.ready)glassButton("mastery-claim"+s.id,{r.x+46,r.y+162+lower,r.w-92,26},"Claim mastery badge",[this,id=s.id]{command({Action::ClaimMastery,{}, {},id});},"green",17);
  else canvas_.text(mastery.complete?"All 3 badges earned":std::to_string(mastery.tier)+" / 3 badges earned",r.x+r.w*.5f,r.y+165+lower,17,pale,true,r.w-20);
 }
 if(list.empty())canvas_.label("No fish in this category",p.x+p.w*.5f,p.y+400,31,true,p.w-100);
 const float footerY=p.y+p.h-60;
 glassButton("collection-prev",{p.x+32,footerY,154,44},"Previous",[this]{page_=std::max(0,page_-1);},page_>0?"blue":"nav",22);
 canvas_.label(std::to_string(page_+1)+" / "+std::to_string(pages),p.x+260,footerY+5,27,true,129);
 glassButton("collection-next",{p.x+334,footerY,154,44},"Next",[this,pages]{page_=std::min(pages-1,page_+1);},page_+1<pages?"blue":"nav",22);
 canvas_.text(std::to_string(list.size())+" species in this collection",p.x+p.w-291,footerY+12,21,white,true,483);
}

void View::settings(){
 auto& d=session_.domain();const auto& s=d.state().settings;
 canvas_.icon("skin/icon-gear.png",{250,115,92,89});canvas_.label("SETTINGS",358,117,52,false,795);canvas_.text("Customize your aquarium experience!",360,178,25,pale,false,850);
 glassButton("panel-close",{1331,124,66,66},"",[this]{open(Panel::None);},"nav");canvas_.icon("skin/control-close.png",{1348,141,32,32});
 const std::array<Rect,6> tiles{{{239,219,575,167},{828,219,575,167},{239,398,575,132},{828,398,575,132},{239,539,575,173},{828,539,575,173}}};
 for(const auto& r:tiles)canvas_.skin("card",r,38);
 const std::array<const char*,6> icons{"music","sound","bell","globe","graphics","support"};
 const std::array<const char*,6> titles{"MUSIC","SOUND EFFECTS","NOTIFICATIONS","LANGUAGE","GRAPHICS","HELP & SUPPORT"};
 for(int i=0;i<6;++i){auto r=tiles[i];canvas_.icon("skin/icon-"+std::string(icons[i])+".png",{r.x+15,r.y+25,126,r.h-36});canvas_.label(titles[i],r.x+155,r.y+20,32,false,r.w-175);}
 auto toggle=[&](std::string id,Rect r,bool on,std::function<void()> fn){glassButton(std::move(id),r,on?"ON":"OFF",std::move(fn),on?"green":"blue",25);canvas_.icon("skin/icon-pearl.png",{on?r.x+r.w-55:r.x+5,r.y+3,49,r.h-6});};
 toggle("music-toggle",{653,238,144,56},s.music,[this]{command({Action::SetMusic,{}, {},"",{},Currency::Coins,session_.domain().state().settings.music?0.:1.});});
 toggle("sound-toggle",{1238,238,144,56},s.sound,[this]{command({Action::SetSound,{}, {},"",{},Currency::Coins,session_.domain().state().settings.sound?0.:1.});});
 toggle("notifications-toggle",{653,417,144,56},notificationOn_,[this]{notificationOn_=!notificationOn_;});
 canvas_.text("Adjust the background music",394,283,19,pale,false,390);canvas_.text("Play sounds for taps, rewards",982,283,19,pale,false,378);canvas_.text("and more",982,305,19,pale,false,378);
 canvas_.text("Get updates about rewards, events",394,465,18,pale,false,396);canvas_.text("and new content",394,486,18,pale,false,396);canvas_.text("Choose your language",982,465,19,pale,false,230);
 auto slider=[&](std::string id,float x,float y,float value,bool music){canvas_.icon("skin/control-volume.png",{x,y-4,37,37});progress({x+54,y,269,27},value);canvas_.icon("skin/icon-pearl.png",{x+54+value*269-22,y-9,47,47});canvas_.label(std::to_string(int(std::round(value*100)))+"%",x+335,y-4,24,false,76,white,0);touchTarget(id,{x+48,y-14,282,55},[this,x,music,shift=canvas_.origin()]{const auto point=panelPose_.unapply({pointerX_,pointerY_});float v=std::clamp((point.x-shift-x-54)/269.f,0.f,1.f);if(music)musicVolume_=v;else{soundVolume_=v;command({Action::SetVolume,{}, {},"",{},Currency::Coins,double(v)});}});};
 slider("music-volume",395,334,musicVolume_,true);slider("sound-volume",980,334,soundVolume_,false);
 glassButton("language",{1191,440,190,59},"English",[this]{languageOpen_=!languageOpen_;},"blue",24);canvas_.icon("skin/control-down.png",{1336,458,28,28});
 canvas_.text("Adjust visual quality for best",395,594,19,pale,false,390);canvas_.text("performance",395,618,19,pale,false,390);canvas_.text("Need help? We're here for you!",982,600,20,pale,false,395);
 const std::array<const char*,3> quality{"Low","Medium","High"};for(int i=0;i<3;++i)glassButton("quality"+std::to_string(i),{395.f+i*132,649,125,50},quality[i],[this,i]{graphicsQuality_=i;command({Action::SetReducedMotion,{}, {},"",{},Currency::Coins,i==0?1.:0.});},graphicsQuality_==i?"green":"blue",22);
 glassButton("support",{982,639,241,59},"Contact Support",[this]{helpOpen_=true;},"blue",24);glassButton("faq",{1237,639,146,59},"FAQ",[this]{helpOpen_=true;},"blue",25);
 canvas_.text("Game Version 1.0.0",790,747,19,pale,true,500);canvas_.text("Play, Relax, Build Your Dream Aquarium!",790,773,18,pale,true,650);
 canvas_.text("Privacy Policy   |   Terms of Service",1166,751,18,pale,true,410);touchTarget("legal",{993,746,367,34},[this]{helpOpen_=true;});
 canvas_.icon("skin/icon-coral.png",{180,682,142,142});canvas_.icon("skin/icon-coral.png",{1358,684,108,132});
 if(languageOpen_){canvas_.skin("panel",{1190,501,191,67},22);canvas_.label("English",1285,518,23,true,164);touchTarget("language-english",{1190,501,191,67},[this]{languageOpen_=false;});}
 if(helpOpen_){canvas_.skin("panel",{440,270,790,310},48);canvas_.label("Aquarium Help",835,302,40,true,650);canvas_.text("Food is free. Tap FOOD, then tap the water.",835,367,24,white,true,712);canvas_.text("Shop purchases place eggs. Bag restores stored fish.",835,407,23,white,true,712);canvas_.text("Your progress stays on this device. Online support is not connected.",835,447,20,pale,true,712);glassButton("help-close",{733,497,204,55},"Got it",[this]{helpOpen_=false;},"green",26);}
}
} // namespace aq
