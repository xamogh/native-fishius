#include "aquarium/theme.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
namespace {
constexpr Color white{7,83,166,255};
std::string legacyName(std::string_view id){if(id=="seaweed")return "Seaweed garden";if(id=="coral")return "Sunset coral";if(id=="shell")return "Pearl shell";if(id=="arch")return "Little stone arch";return "Treasure chest";}
DecorDef legacyDef(std::string_view id){DecorDef d;d.id=id;d.name=legacyName(id);d.asset="decor/"+std::string(id)+".png";d.width=1.1;d.height=.95;d.art=Json::object();return d;}
int layerOrder(const DecorDef& d){return d.layer=="Background"?0:d.layer=="Foreground"?2:1;}
}
WorldPoint View::decorPreviewPoint(WorldPoint point)const{
 const auto& domain=session_.domain();
 const auto* item=domain.decoration(restoreDecor_?restoreDecor_:selectedDecor_);
 const auto& id=item?item->kind:decorId_;
 if(id.empty())return decorPlacementPoint(point);
 const auto* source=domain.content().findDecor(id);
 const auto def=source?*source:legacyDef(id);
 return decorPlacementPoint(def,point,item?item->sizeMul:1);
}
Rect View::decorPreviewRect()const{
 const auto& domain=session_.domain();
 const auto* item=domain.decoration(restoreDecor_?restoreDecor_:selectedDecor_);
 const auto id=item?item->kind:decorId_;
 const auto* def=domain.content().findDecor(id);const auto legacy=legacyDef(id);
 return canvas_.decorRect(def?*def:legacy,decorPreview_.value_or(WorldPoint{}),item?item->sizeMul:1);
}
void View::cancelDecor(){setTool(Tool::Decor);}
void View::armStoredDecor(std::uint64_t id){
 const auto& domain=session_.domain();const auto* stored=domain.decoration(id);
 if(!stored||!stored->stored)return;
 if(domain.placedDecor(domain.state().activeTank)>=static_cast<std::size_t>(domain.content().decorTuning.placedLimit))return;
 setTool(Tool::Decor);restoreDecor_=id;const auto* d=session_.domain().decoration(id);
 if(d){decorId_=d->kind;decorPreview_=decorPreviewPoint({waterWidth*.5,decorReferenceHeight*(.3+.82*(.985-.3))});}
 receipts_.clear();
}
std::uint64_t View::hitDecor(float x,float y)const{
 const auto& d=session_.domain();std::uint64_t result=0;double best=-1;int bestLayer=-1;
 if(tool_!=Tool::Decor||!inTank(canvas_.toWorld(x,y)))return 0;
 for(const auto& item:d.state().decor){
  if(item.stored||item.tank!=d.state().activeTank)continue;
  const auto* source=d.content().findDecor(item.kind);const auto def=source?*source:legacyDef(item.kind);
  const int layer=layerOrder(def);
  if(canvas_.decorRect(def,item.position,item.sizeMul).has(x,y)&&(layer>bestLayer||(layer==bestLayer&&item.position.y>=best))){result=item.id;best=item.position.y;bestLayer=layer;}
 }
 return result;
}
void View::beginMoveDecor(std::uint64_t id){
 const auto& domain=session_.domain();const auto* item=domain.decoration(id);
 if(!item||item->stored||item->tank!=domain.state().activeTank)return;
 selected_={};selectedDecor_=id;decorPreview_=decorPreviewPoint(item->position);draggingDecor_=true;
 receipts_.clear();
}
void View::confirmDecor(){
 if(!decorPreview_)return;auto& d=session_.domain();Command c;
 c.point=*decorPreview_;c.tank=d.state().activeTank;
 if(restoreDecor_){c.action=Action::RestoreDecor;c.decor=restoreDecor_;}
 else if(!decorId_.empty()){
  // Older saves can contain an already paid copy. New previews are view-only.
  c.action=d.state().pendingDecor.empty()?Action::BuyDecor:Action::PlaceDecor;c.key=decorId_;
 }
 else if(selectedDecor_&&draggingDecor_){c.action=Action::MoveDecor;c.decor=selectedDecor_;}
 else return;
 const auto result=command(c);
 if(result){
  decorPreview_.reset();decorId_.clear();restoreDecor_=0;draggingDecor_=false;
  decorPlacementError_.clear();buttons_.clear();tool_=Tool::Decor;
 }else decorPlacementError_=result.message;
}
void View::decorPlacementControls(){
 if(!decorPreview_)return;
 const auto& domain=session_.domain();const auto item=decorPreviewRect();
 const auto safe=canvas_.safeInsets();
 const float unit=canvas_.minimumTouchSize()/44.f;
 const float size=std::max(100.f,48*unit),gap=12*unit,width=size*2+gap;
 const float caption=24*unit,height=size+caption+8*unit;
 const float left=std::max(12*unit,safe.left+8*unit),right=canvas_.width()-std::max(12*unit,safe.right+8*unit);
 const float top=std::max(12*unit,safe.top+8*unit),bottom=canvas_.height()-std::max(12*unit,safe.bottom+8*unit);
 float x=std::clamp(item.x+item.w*.5f-width*.5f,left,std::max(left,right-width));
 float y=item.y-height-12*unit;
 if(y<top)y=item.y+item.h+18*unit;
 y=std::clamp(y,top,std::max(top,bottom-height));
 // Keep the action row reachable when tall artwork fills the viewport.
 if(y<item.y+item.h&&y+height>item.y){
  if(item.x+item.w+width+12*unit<=right)x=item.x+item.w+12*unit;
  else if(item.x-width-12*unit>=left)x=item.x-width-12*unit;
 }
 const auto* def=domain.content().findDecor(decorId_);
 const bool purchase=def&&!restoreDecor_&&!draggingDecor_&&domain.state().pendingDecor.empty();
 const std::string hint=purchase?compact(def->price)+(def->currency==Currency::Pearls?" pearls":" coins"):
                        draggingDecor_?"Move item":restoreDecor_?"Place item":"Already purchased";
 canvas_.label(hint,x+width*.5f,y+size+6*unit,18*unit,true,width,{255,255,240,255},1.5f);
 // Draw the symbols as strokes so they stay sharp without a font glyph.
 const auto stroke=[&](SDL_FPoint a,SDL_FPoint b,float weight,Color color){
  const float length=std::hypot(b.x-a.x,b.y-a.y),dx=(b.y-a.y)*weight/(2*length),dy=(a.x-b.x)*weight/(2*length);
  const SDL_FColor c{color.r/255.f,color.g/255.f,color.b/255.f,color.a/255.f};
  const SDL_Vertex vertices[]={{{a.x+dx,a.y+dy},c,{}},{{b.x+dx,b.y+dy},c,{}},{{b.x-dx,b.y-dy},c,{}},{{a.x-dx,a.y-dy},c,{}}};
  const int indices[]={0,1,2,0,2,3};SDL_RenderGeometry(canvas_.renderer(),nullptr,vertices,4,indices,6);
 };
 const auto action=[&](std::string id,Rect target,bool confirm){
  const auto r=buttonVisual(id,target);
  const Color border=confirm?Color{42,83,25,255}:Color{109,35,28,255};
  canvas_.skin(confirm?"green":"red",r);
  const auto point=[&](float u,float v){return SDL_FPoint{r.x+u*r.w,r.y+v*r.h};};
  const auto symbol=[&](float thickness,Color color){
   if(confirm){stroke(point(.24f,.51f),point(.44f,.7f),thickness,color);stroke(point(.43f,.69f),point(.77f,.28f),thickness,color);}
   else{stroke(point(.3f,.29f),point(.7f,.71f),thickness,color);stroke(point(.7f,.29f),point(.3f,.71f),thickness,color);}
  };
  symbol(r.w*.16f,border);symbol(r.w*.105f,{255,255,240,255});
  touchTarget(std::move(id),target,[this,confirm]{if(confirm)confirmDecor();else cancelDecor();});
 };
 closeButton("decor-cancel",{x,y,size,size},[this]{cancelDecor();});
 action("decor-confirm",{x+size+gap,y,size,size},true);
 if(!decorPlacementError_.empty()){
  const float errorWidth=std::min(600.f,right-left);
  const float errorX=std::clamp(x+width*.5f,left+errorWidth*.5f,right-errorWidth*.5f);
  canvas_.label(decorPlacementError_,errorX,bottom-24*unit,16*unit,true,errorWidth,{255,235,210,255},1.5f);
 }
}
void View::decorInventory(){
 auto& d=session_.domain();const auto p=panelRect_;
 glassButton("arrange-tank",{p.x+20,p.y+74,235,44},"Arrange tank",[this]{setTool(Tool::Decor);},"green",23);
 if(std::any_of(d.state().companions.begin(),d.state().companions.end(),[](const Companion& fish){return fish.stored;}))
  glassButton("inventory-fish",{p.x+p.w-210,p.y+74,190,44},"Fish",[this]{inventoryCategory_=0;page_=0;},"blue",23);
 std::vector<const Decoration*> list;for(const auto& item:d.state().decor)if(item.stored)list.push_back(&item);
 const int per=6,pages=std::max(1,(int(list.size())+per-1)/per);page_=std::clamp(page_,0,pages-1);
 const float gap=12,cw=(p.w-64)/3,ch=(p.h-202)/2;
 for(int i=0;i<per&&page_*per+i<int(list.size());++i){
  const auto& item=*list[page_*per+i];const auto* source=d.content().findDecor(item.kind);const auto def=source?*source:legacyDef(item.kind);
  const Rect r{p.x+20+(i%3)*(cw+gap),p.y+124+(i/3)*(ch+gap),cw,ch};
  canvas_.skin("card",r,18);canvas_.label(def.name,r.x+cw*.5f,r.y+8,22,true,cw-18);
  canvas_.icon(source?"decor/icons/"+def.id+".png":def.asset,{r.x+cw*.22f,r.y+40,cw*.56f,ch-85});
  glassButton("restore-decor"+std::to_string(item.id),{r.x+12,r.y+ch-43,cw-24,34},"Place for free",[this,id=item.id]{armStoredDecor(id);},"green",19);
 }
 if(list.empty()){
  const float cx=p.x+p.w*.5f;
  canvas_.icon("decor/icons/CD-07.png",{cx-55,p.y+163,110,110});
  canvas_.label("No stored decorations",cx,p.y+291,32,true,p.w-100,theme::ink);
  canvas_.text("Choose Arrange tank, then tap a decoration to move or store it.",cx,p.y+342,24,theme::body,true,p.w-130);
  canvas_.text("Place stored decorations in any tank for free.",cx,p.y+378,24,theme::body,true,p.w-130);
 }
 glassButton("decor-inv-prev",{p.x+20,p.y+p.h-48,100,34},"Previous",[this]{page_=std::max(0,page_-1);},"blue",19);
 glassButton("decor-inv-next",{p.x+p.w-120,p.y+p.h-48,100,34},"Next",[this,pages]{page_=std::min(pages-1,page_+1);},"blue",19);
 canvas_.text(std::to_string(list.size())+" stored decorations",p.x+p.w*.5f,p.y+p.h-43,20,white,true,p.w-260);
}
}
