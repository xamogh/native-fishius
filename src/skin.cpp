#include "aquarium/view.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>

namespace aq {
namespace {
constexpr Color white{250,254,255,255},pale{186,244,255,255},navy{5,62,113,255};
std::string titleCase(std::string s){if(!s.empty())s[0]=char(std::toupper(static_cast<unsigned char>(s[0])));return s;}
std::string fishName(const Species& s){if(s.id=="ocellarisClownfish")return "Clownfish";if(s.id=="queenAngelfish")return "Angelfish";return s.name;}
const std::array<const char*,10> featured{{"ocellarisClownfish","guppy","neonTetra","yellowTang","queenAngelfish","moorishIdol","mandarinDragonet","bluelineTriggerfish","discus","platinumBetta"}};
const std::array<const char*,6> shopOrder{{"ocellarisClownfish","neonTetra","guppy","queenAngelfish","bluelineTriggerfish","discus"}};
}

std::string fishArt(std::string_view id,std::uint64_t variant){
 if(id=="guppy")return variant%3==1?"aquarium/fish-guppy-tilted.png":variant%3==2?"aquarium/fish-guppy-lower.png":"aquarium/fish-guppy.png";
 if(id=="molly")return "aquarium/fish-molly.png";
 if(id=="platy")return "aquarium/fish-green.png";
 if(id=="emberTetra")return "aquarium/fish-turn.png";
 if(id=="ocellarisClownfish"||id=="santaClownfish")return "skin/fish-clownfish.png";
 if(id=="neonTetra"||id=="emberTetra")return "skin/fish-neon.png";
 if(id=="guppy")return "skin/fish-guppy.png";
 if(id=="queenAngelfish"||id=="moorishIdol")return "skin/fish-angelfish.png";
 if(id=="bluelineTriggerfish")return "skin/fish-blue-tang.png";
 if(id=="yellowTang")return "skin/fish-yellow-tang.png";
 if(id=="discus")return "skin/fish-discus.png";
 return "fish/"+std::string(id)+".png";
}

void Canvas::skin(std::string_view name,Rect r,float corner,float alpha){
 if(r.w<=0||r.h<=0)return;
 r.x+=originX_;
 auto* t=texture("skin/"+std::string(name)+".png");
 const float cut=86,edge=std::min({corner,r.w*.5f,r.h*.5f});
 const float sx[4]{0,cut,float(t->width)-cut,float(t->width)},sy[4]{0,cut,float(t->height)-cut,float(t->height)};
 const float dx[4]{r.x,r.x+edge,r.x+r.w-edge,r.x+r.w},dy[4]{r.y,r.y+edge,r.y+r.h-edge,r.y+r.h};
 SDL_SetTextureAlphaModFloat(t->value,alpha);
 for(int y=0;y<3;++y)for(int x=0;x<3;++x){SDL_FRect src{sx[x],sy[y],sx[x+1]-sx[x],sy[y+1]-sy[y]},dst{dx[x],dy[y],dx[x+1]-dx[x],dy[y+1]-dy[y]};SDL_RenderTexture(renderer_,t->value,&src,&dst);}
 SDL_SetTextureAlphaModFloat(t->value,1);
}
void Canvas::icon(std::string_view name,Rect r,float alpha){
 auto* t=texture(name);float scale=std::min(r.w/float(t->width),r.h/float(t->height));
 image(name,{r.x+(r.w-float(t->width)*scale)*.5f,r.y+(r.h-float(t->height)*scale)*.5f,float(t->width)*scale,float(t->height)*scale},0,{.5f,.5f},alpha);
}
void Canvas::label(std::string_view s,float x,float y,float size,bool center,float maxWidth,Color c,float stroke){
 if(stroke>0){for(int i=0;i<8;++i){float a=float(i)*.78539816f;text(s,x+std::cos(a)*stroke,y+std::sin(a)*stroke+1,size,{3,62,121,c.a},center,maxWidth,true);}}
 text(s,x,y,size,c,center,maxWidth,true);
}
void View::touchTarget(std::string id,Rect r,std::function<void()> fn){r.x+=canvas_.origin();buttons_.push_back({std::move(id),r,std::move(fn)});}
void View::glassButton(std::string id,Rect r,std::string text,std::function<void()> fn,std::string material,float size,std::string art){
 Rect visual=r;
 if(pressed_==id){visual.x+=2;visual.y+=2;visual.w-=4;visual.h-=4;}
 canvas_.skin(material,visual,std::min(32.f,r.h*.46f));
 if(!art.empty()){float a=r.h*.74f;canvas_.icon(art,{r.x+9,r.y+(r.h-a)*.5f,a,a});}
 if(!text.empty())canvas_.label(text,r.x+r.w*.5f+(art.empty()?0:r.h*.22f),r.y+(r.h-size)*.46f-2,size,true,r.w-(art.empty()?14:r.h+8),material=="gold"?Color{100,46,0,255}:white,material=="gold"?0:2);
 touchTarget(std::move(id),r,std::move(fn));
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
 auto art=[&](std::string_view name,Rect rect){canvas_.image("aquarium/"+std::string(name)+".png",rect);};
 auto control=[&](std::string id,std::string_view name,Rect rect,std::function<void()> action){
  Rect visual=rect;
  if(pressed_==id){visual.x+=2;visual.y+=2;visual.w-=4;visual.h-=4;}
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
 control("notifications","notifications",{1434+right,14,74,76},[this]{open(Panel::Quests);});
 control("menu","menu",{1509+right,14,72,76},[this]{open(Panel::Settings);});
 control("nav0","tanks",{0+left,688-bottom,304,142},[this]{open(Panel::Tanks);});
 control("nav1","shop",{1216+right,680-std::max(0.f,bottom-20.f),360,150},[this]{open(Panel::Shop);});
 if(panel_!=Panel::None&&panel_!=Panel::Tanks&&panel_!=Panel::Shop)return;
 constexpr float toolGap=24,selectY=91,foodY=selectY+134+toolGap;
 constexpr float masteryY=foodY+132+toolGap,sellY=masteryY+132+toolGap;
 control("tool-select","select",{1477+right,selectY,127,134},[this]{
  if(tool_==Tool::Select&&panel_==Panel::None)selectMenuOpen_=!selectMenuOpen_;else setTool(Tool::Select);
 });
 control("tool-food","food",{1477+right,foodY,127,132},[this]{setTool(Tool::Food);});
 control("nav3","mastery",{1477+right,masteryY,127,132},[this]{open(Panel::Collection);});
 control("tool-sell","sell",{1477+right,sellY,127,132},[this]{setTool(Tool::Sell);});
 if(panel_!=Panel::None)return;
 if(selectMenuOpen_){
  constexpr std::array<Tool,3> targets{Tool::Move,Tool::Stash,Tool::Restore};
  constexpr std::array<const char*,3> labels{"MOVE","STASH","BAG"};
  for(int i=0;i<3;++i)glassButton("select-option"+std::to_string(i),{1300+right,105.f+i*61,170,55},labels[i],[this,i,target=targets[i]]{selectMenuOpen_=false;if(i==2)open(Panel::Inventory);else setTool(target);},"blue",24);
 }
 if(tool_!=Tool::Select){
  const char* instruction=tool_==Tool::Food?"Tap the water to feed":tool_==Tool::Sell?"Tap a fish to sell":tool_==Tool::Medicine?"Tap a fish to care for it":tool_==Tool::Move?"Drag a fish to move it":tool_==Tool::Stash?"Tap a fish to store it":"Tap the water to place";
  const float centre=canvas_.width()*.5f;
  canvas_.skin("blue",{centre-205,744-bottom,410,57},28);
  canvas_.label(instruction,centre-30,755-bottom,21,true,318,white,1);
  glassButton("done",{centre+132,749-bottom,67,46},"DONE",[this]{setTool(Tool::Select);},"green",16);
 }
}

float View::panelOrigin()const{
 // The Tanks artwork starts at x=111, including its coral. Leave a 24-unit
 // gap after the full 304-unit Tanks button, independent of screen width.
 if(panel_==Panel::Tanks)return std::max(0.f,canvas_.safeInsets().left-90.f)+304.f+24.f-111.f;
 return (canvas_.width()-1608.f)*.5f;
}

void View::panel(){
 canvas_.origin(panelOrigin());
 panelBody();
 canvas_.origin(0);
}

void View::panelBody(){
 switch(panel_){case Panel::Tanks:panelRect_={122,92,376,710};break;case Panel::Shop:panelRect_={146,97,1394,710};break;case Panel::Collection:panelRect_={132,96,1387,708};break;case Panel::Settings:panelRect_={209,98,1220,704};break;default:panelRect_={315,142,1040,575};break;}
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
  canvas_.text(active?std::to_string(d.living({id}))+" / "+std::to_string(tank->slots)+" fish":tank?std::to_string(tank->slots)+" fish capacity":"Unlocks at Level "+std::to_string(d.content().tankLevels[i]),r.x+143,r.y+51,17,white,false,178);
  if(active)glassButton("tank-current",{r.x+215,r.y+73,107,38},"Current",[this]{open(Panel::None);},"green",20);
  else if(!tank)canvas_.icon("skin/icon-lock.png",{r.x+281,r.y+24,43,45});
  touchTarget("tank-select"+std::to_string(id),r,[this,id,tank]{if(tank){command({Action::SwitchTank,{}, {id}});open(Panel::None);}else{selectedTank_=id;}});
 }
 const auto* active=d.tank(d.state().activeTank);int choice=selectedTank_;if(choice<1||choice>5)choice=2;
 const auto* chosen=d.tank({choice});Amount cost=chosen?d.content().tankCosts[choice-1][std::min(3,chosen->slots/10)]:d.content().tankCosts[choice-1][0];
 if(active&&active->slots<40&&choice==d.state().activeTank.value)cost=d.content().tankCosts[choice-1][active->slots/10];
 glassButton("tank-buy",{194,690,241,61},compact(cost),[this,choice,chosen]{command({chosen?Action::ExpandTank:Action::UnlockTank,{}, {choice},"",{},Currency::Coins});},"green",29,"skin/icon-coin.png");
 canvas_.text("Buy with Coins",313,758,18,pale,true,235);canvas_.icon("skin/icon-coral.png",{111,674,100,131});canvas_.icon("skin/icon-coral.png",{420,681,94,125});
}

void View::shop(){
 auto& d=session_.domain();Rect p=panelRect_;
 const auto safe=canvas_.safeInsets();
 const bool cutoutRight=SDL_GetCurrentDisplayOrientation(SDL_GetDisplayForWindow(canvas_.window()))==SDL_ORIENTATION_LANDSCAPE_FLIPPED;
 const float rightInset=cutoutRight?std::max(0.f,safe.right-8.f):0.f;
 // Keep the full-size panel. Inset its content where floating controls overlap.
 p.w=std::min(p.w,canvas_.width()-131.f-rightInset-24.f-panelOrigin()-p.x);
 const float cardsY=p.y+233,offersY=p.y+p.h-145,cardHeight=offersY-cardsY-21;
 canvas_.icon("skin/icon-shop.png",{p.x+48,p.y+29,100,105});canvas_.label("SHOP",p.x+183,p.y+29,52,false,p.w-530);
 canvas_.text("Make your aquarium more amazing!",p.x+183,p.y+94,23,white,false,p.w-530);
 const Rect promo{p.x+p.w-399,p.y+30,370,167};
 canvas_.skin("panel",promo,52);canvas_.icon("skin/fish-clownfish.png",{promo.x+20,promo.y+26,135,115});
 canvas_.label("New fish",promo.x+169,promo.y+31,28,false,188);canvas_.label("new friends!",promo.x+169,promo.y+64,28,false,188);
 const std::array<const char*,4> tabs{"Fish","Food","Decor","Boosts"};const std::array<const char*,4> tabIcons{"skin/icon-fish.png","ui/food.png","skin/icon-coral.png","skin/control-bolt.png"};
 const float tabWidth=(p.w-510)/4;
 for(int i=0;i<4;++i)glassButton("shop-tab"+std::to_string(i),{p.x+33+i*(tabWidth+8),p.y+142,tabWidth,85},tabs[i],[this,i]{category_=i;page_=0;},category_==i?"gold":"blue",29,tabIcons[i]);
 canvas_.skin("panel",{p.x+8,cardsY-5,p.w-16,cardHeight+10},32);
 if(category_==0){
  std::vector<const Species*> list;for(auto id:shopOrder)if(auto* s=d.content().find(id))list.push_back(s);for(const auto& s:d.content().species)if(std::find(shopOrder.begin(),shopOrder.end(),s.id)==shopOrder.end())list.push_back(&s);
  const int pages=(int(list.size())+5)/6;page_=std::clamp(page_,0,pages-1);
  const std::array<const char*,6> sampleNames{"Clownfish","Neon Tetra","Guppy","Angelfish","Blue Tang","Discus"};const std::array<const char*,6> sampleRarity{"Common","Common","Rare","Epic","Epic","Legendary"};const std::array<Amount,6> samplePrices{500,700,1200,2500,3000,10};
  const float cardWidth=(p.w-36-55)/6;
  for(int i=0;i<6;++i){int at=page_*6+i;if(at>=int(list.size()))break;const auto& s=*list[at];Rect r{p.x+18+i*(cardWidth+11),cardsY,cardWidth,cardHeight};canvas_.skin("card",r,24);
   canvas_.label(referencePreview_&&page_==0?sampleNames[i]:fishName(s),r.x+r.w*.5f,r.y+16,25,true,r.w-17);
   canvas_.icon("skin/icon-coral.png",{r.x+8,r.y+r.h-174,48,90},.7f);canvas_.icon("skin/icon-coral.png",{r.x+r.w-56,r.y+r.h-181,49,97},.85f);
   canvas_.icon(fishArt(s.id),{r.x+25,r.y+52,r.w-50,139});
   std::string rarity=referencePreview_&&page_==0?sampleRarity[i]:titleCase(s.rarity);canvas_.skin(i>=3?"blue":"green",{r.x+44,r.y+r.h-146,r.w-88,37},16);canvas_.label(rarity,r.x+r.w*.5f,r.y+r.h-141,20,true,r.w-90);
   bool pearls=referencePreview_&&page_==0?i==5:s.currency==Currency::Pearls;canvas_.icon(pearls?"skin/icon-pearl.png":"skin/icon-coin.png",{r.x+38,r.y+r.h-102,34,34});canvas_.label(compact(referencePreview_&&page_==0?samplePrices[i]:s.price),r.x+83,r.y+r.h-101,28,false,r.w-91);
   glassButton("buy"+s.id,{r.x+21,r.y+r.h-61,r.w-42,49},"Buy",[this,id=s.id]{const auto* item=session_.domain().content().find(id);auto gate=session_.domain().blocker(*item);if(gate)armBuy(id);else toast(errorText(gate.error));},"green",29);
  }
  touchTarget("shop-previous",{p.x+2,cardsY+35,21,cardHeight-70},[this]{page_=std::max(0,page_-1);});touchTarget("shop-next",{p.x+p.w-23,cardsY+35,21,cardHeight-70},[this,pages]{page_=std::min(pages-1,page_+1);});
 }else if(category_==2){
  const float count=float(d.decorations().size()),cardWidth=(p.w-48-12*(count-1))/count;
  int i=0;for(const auto& decor:d.decorations()){Rect r{p.x+24+i*(cardWidth+12),cardsY,cardWidth,cardHeight};canvas_.skin("card",r,28);canvas_.label(decor.name,r.x+r.w*.5f,r.y+14,25,true,r.w-20);canvas_.icon("decor/"+decor.id+".png",{r.x+38,r.y+51,r.w-76,r.h-117});glassButton("decor-buy"+decor.id,{r.x+18,r.y+r.h-52,r.w-36,42},compact(decor.price)+" Coins",[this,id=decor.id]{armDecor(id);},"green",23);++i;}
 }else{
  canvas_.icon(category_==1?"ui/food.png":"skin/control-bolt.png",{p.x+118,cardsY+39,127,142});canvas_.label(category_==1?"Daily fish food":"Grow together",p.x+300,cardsY+34,38,false,p.w-335);canvas_.text(category_==1?"Drop a pellet into the water. Feeding your fish is free.":"Healthy, well-fed fish grow while you play.",p.x+300,cardsY+91,23,white,false,p.w-335);
  glassButton("shop-care",{p.x+300,cardsY+cardHeight-88,346,60},category_==1?"Feed your fish":"Return to aquarium",[this]{setTool(Tool::Food);},"green",27);
 }
 const float offersLeft=std::max(p.x+48,328.f+std::max(0.f,safe.left-90.f)-panelOrigin());
 const float offersRight=std::min(p.x+p.w-17,canvas_.width()-416.f-rightInset-panelOrigin());
 const float offersWidth=offersRight-offersLeft,offerText=offersLeft+offersWidth*.32f,dealsX=offersRight-234;
 canvas_.skin("nav",{offersLeft,offersY,offersWidth,130},46);canvas_.image("skin/offers.png",{offersLeft+13,offersY+12,offersWidth-26,106});
 canvas_.label("Special Offers",offerText,offersY+24,38,false,dealsX-offerText-16,{255,241,165,255},3);canvas_.text("Amazing fish, decor and more!",offerText,offersY+80,21,white,false,dealsX-offerText-16);
 glassButton("offers",{dealsX,offersY+28,218,66},"View Deals",[this]{open(Panel::Gifts);},"gold",27);canvas_.icon("skin/control-next.png",{dealsX+174,offersY+44,29,29});
}

void View::collection(){
 const auto& d=session_.domain();canvas_.icon("skin/icon-book.png",{154,121,94,76});canvas_.label("COLLECTION",272,112,44,false,670);canvas_.text("Discover and collect amazing fish!",273,162,23,white,false,795);
 glassButton("panel-close",{1426,124,66,66},"",[this]{open(Panel::None);},"nav");canvas_.icon("skin/control-close.png",{1443,141,32,32});
 const std::array<const char*,5> filters{"All Fish","Common","Uncommon","Rare","Epic"};
 for(int i=0;i<5;++i)glassButton("filter"+std::to_string(i),{162.f+i*171,213,163,58},filters[i],[this,i]{collectionFilter_=i;page_=0;},collectionFilter_==i?"green":"blue",22);
 glassButton("sort",{1069,210,228,63},sortMode_==0?"Sort: Default":sortMode_==1?"Sort: Name":"Sort: Rarity",[this]{sortMode_=(sortMode_+1)%3;},"blue",22);canvas_.icon("skin/control-down.png",{1255,231,24,24});
 canvas_.skin("blue",{1308,204,185,77},27);canvas_.icon("skin/icon-fish.png",{1322,225,47,37});canvas_.label(referencePreview_?"6 / 24":std::to_string(d.state().collected.size())+" / "+std::to_string(d.content().species.size()),1420,215,27,true,115);canvas_.text("Discovered",1422,246,19,white,true,116);
 std::vector<const Species*> list;for(auto id:featured)if(auto* s=d.content().find(id))list.push_back(s);if(!referencePreview_)for(const auto& s:d.content().species)if(std::find(featured.begin(),featured.end(),s.id)==featured.end())list.push_back(&s);
 const std::array<const char*,10> sampleNames{"Clownfish","Guppy","Neon Tetra","Yellow Tang","Angelfish","Moorish Idol","Mandarin Fish","Blue Tang","Discus","Betta Fish"};
 const std::array<const char*,10> sampleRarity{"Common","Common","Uncommon","Uncommon","Rare","Rare","Rare","Epic","Epic","Epic"};const std::array<int,10> sampleCount{3,5,2,1,1,0,0,0,0,0};
 auto rarityFor=[&](const Species* s){auto i=std::find(featured.begin(),featured.end(),s->id);return referencePreview_&&i!=featured.end()?std::string(sampleRarity[std::size_t(i-featured.begin())]):titleCase(s->rarity);};
 if(collectionFilter_>0)std::erase_if(list,[&](const Species* s){return rarityFor(s)!=filters[collectionFilter_];});
 if(sortMode_==1)std::stable_sort(list.begin(),list.end(),[](const Species* a,const Species* b){return a->name<b->name;});if(sortMode_==2)std::stable_sort(list.begin(),list.end(),[&](const Species* a,const Species* b){return rarityFor(a)<rarityFor(b);});
 int pages=std::max(1,(int(list.size())+9)/10);page_=std::clamp(page_,0,pages-1);
 for(int i=0;i<10;++i){int at=page_*10+i;if(at>=int(list.size()))break;const auto& s=*list[at];auto f=std::find(featured.begin(),featured.end(),s.id);int idx=f==featured.end()?-1:int(f-featured.begin());bool known=std::find(d.state().collected.begin(),d.state().collected.end(),s.id)!=d.state().collected.end();int count=known?1:0;
  if(referencePreview_&&idx>=0){count=sampleCount[std::size_t(idx)];known=count>0;}
  int goal=s.id=="guppy"?5:3;Rect r{168.f+float(i%5)*267,290.f+float(i/5)*264,251,252};bool selected=collectionSelected_==s.id||(collectionSelected_.empty()&&at==0);
  if(selected)canvas_.skin("panel",{r.x-4,r.y-4,r.w+8,r.h+8},28);
  canvas_.skin("card",r,25,known?1.f:.78f);
  canvas_.icon("skin/icon-coral.png",{r.x+11,r.y+127,44,75},.55f);canvas_.icon("skin/icon-coral.png",{r.x+r.w-55,r.y+125,47,78},.62f);
  if(known){int stars=idx==4?3:idx==2||idx==3?2:1;for(int k=0;k<stars;++k)canvas_.icon("skin/icon-star.png",{r.x+17+k*24.f,r.y+15,28,28});canvas_.icon(fishArt(s.id),{r.x+28,r.y+40,r.w-56,120});}
  else{auto path=fishArt(s.id);path=path.substr(0,path.size()-4)+"-mask.png";auto* texture=canvas_.renderer();(void)texture;canvas_.icon(path,{r.x+35,r.y+33,r.w-70,116},.12f);canvas_.icon("skin/icon-lock.png",{r.x+r.w*.5f-21,r.y+59,43,56});}
  canvas_.label(referencePreview_&&idx>=0?sampleNames[std::size_t(idx)]:fishName(s),r.x+r.w*.5f,r.y+153,25,true,r.w-18);
  progress({r.x+23,r.y+188,r.w-46,27},float(count)/float(goal));canvas_.label(std::to_string(count)+" / "+std::to_string(goal),r.x+r.w*.5f,r.y+188,21,true,r.w-55);
  std::string rarity=rarityFor(&s);canvas_.icon(rarity=="Common"?"skin/icon-pearl.png":rarity=="Epic"?"skin/icon-coin.png":"skin/icon-star.png",{r.x+72,r.y+223,23,23});canvas_.text(rarity,r.x+105,r.y+222,19,white,false,r.w-112);
  touchTarget("collection-item"+s.id,r,[this,id=s.id]{collectionSelected_=id;});
 }
 touchTarget("collection-prev",{136,338,23,400},[this]{page_=std::max(0,page_-1);});touchTarget("collection-next",{1495,338,22,400},[this,pages]{page_=std::min(pages-1,page_+1);});
 canvas_.icon("skin/icon-coral.png",{107,687,99,130});
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
 auto slider=[&](std::string id,float x,float y,float value,bool music){canvas_.icon("skin/control-volume.png",{x,y-4,37,37});progress({x+54,y,269,27},value);canvas_.icon("skin/icon-pearl.png",{x+54+value*269-22,y-9,47,47});canvas_.label(std::to_string(int(std::round(value*100)))+"%",x+335,y-4,24,false,76,white,0);touchTarget(id,{x+48,y-14,282,55},[this,x,music,shift=canvas_.origin()]{float v=std::clamp((pointerX_-shift-x-54)/269.f,0.f,1.f);if(music)musicVolume_=v;else{soundVolume_=v;command({Action::SetVolume,{}, {},"",{},Currency::Coins,double(v)});}});};
 slider("music-volume",395,334,musicVolume_,true);slider("sound-volume",980,334,soundVolume_,false);
 glassButton("language",{1191,440,190,59},"English",[this]{languageOpen_=!languageOpen_;},"blue",24);canvas_.icon("skin/control-down.png",{1336,458,28,28});
 canvas_.text("Adjust visual quality for best",395,594,19,pale,false,390);canvas_.text("performance",395,618,19,pale,false,390);canvas_.text("Need help? We're here for you!",982,600,20,pale,false,395);
 const std::array<const char*,3> quality{"Low","Medium","High"};for(int i=0;i<3;++i)glassButton("quality"+std::to_string(i),{395.f+i*132,649,125,50},quality[i],[this,i]{graphicsQuality_=i;command({Action::SetReducedMotion,{}, {},"",{},Currency::Coins,i==0?1.:0.});},graphicsQuality_==i?"green":"blue",22);
 glassButton("support",{982,639,241,59},"Contact Support",[this]{helpOpen_=true;},"blue",24);glassButton("faq",{1237,639,146,59},"FAQ",[this]{helpOpen_=true;},"blue",25);
 canvas_.text("Game Version 1.0.0",790,747,19,pale,true,500);canvas_.text("Play, Relax, Build Your Dream Aquarium!",790,773,18,pale,true,650);
 canvas_.text("Privacy Policy   |   Terms of Service",1166,751,18,pale,true,410);touchTarget("legal",{993,746,367,34},[this]{helpOpen_=true;});
 canvas_.icon("skin/icon-coral.png",{180,682,142,142});canvas_.icon("skin/icon-coral.png",{1358,684,108,132});
 if(languageOpen_){canvas_.skin("panel",{1190,501,191,67},22);canvas_.label("English",1285,518,23,true,164);touchTarget("language-english",{1190,501,191,67},[this]{languageOpen_=false;});}
 if(helpOpen_){canvas_.skin("panel",{440,270,790,310},48);canvas_.label("Aquarium Help",835,302,40,true,650);canvas_.text("Food is free. Tap FOOD, then tap the water.",835,367,24,white,true,712);canvas_.text("Shop purchases place eggs. Bag restores stored fish.",835,407,23,white,true,712);canvas_.text("Your progress stays on this device. Online support is not connected.",835,447,20,pale,true,712);glassButton("start-guide",{578,497,250,55},"Start guide",[this]{helpOpen_=false;tutorialVisible_=true;open(Panel::None);},"blue",26);glassButton("help-close",{853,497,204,55},"Got it",[this]{helpOpen_=false;},"green",26);}
}
} // namespace aq
