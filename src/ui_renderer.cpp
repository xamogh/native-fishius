#include "aquarium/ui_renderer.hpp"
#include "clay.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace aq {
namespace {
Color color(DesignColor c,float alpha=1){return {Uint8(c[0]),Uint8(c[1]),Uint8(c[2]),Uint8(std::clamp(c[3]*alpha,0.f,255.f))};}
float measure(Canvas& c,const UiStyle& s,std::string_view value,float size){return c.textWidth(value,size,s.font!="nunito",false,s.font=="baloo")*s.stretch;}
void text(Canvas& c,const UiStyle& s,std::string_view value,float x,float y,float size,Color tint){c.text(value,x,y,size,tint,false,0,s.font!="nunito",false,false,false,0,s.stretch,s.font=="baloo");}
struct Line {std::vector<UiRun> runs;float width{},height{};};
std::vector<Line> lines(Canvas& c,const UiStyle& style,const std::vector<UiRun>& runs,float width,float scale,bool wrap){
 std::vector<Line> result(1);
 for(const auto& run:runs){std::size_t at=0;while(at<run.text.size()){
  if(run.text[at]=='\n'){result.emplace_back();++at;continue;}
  auto end=run.text.find_first_of(" \n",at);if(end==std::string::npos)end=run.text.size();if(end==at)++end;
  std::string token=run.text.substr(at,end-at);const float size=(run.emphasis?style.accentSize:style.size)*scale;
  float w=measure(c,style,token,size);
  if(wrap&&result.back().width>0&&result.back().width+w>width){result.emplace_back();if(token==" "){at=end;continue;}}
  if(wrap&&w>width&&token.size()>1){
   for(std::size_t i=0;i<token.size();){std::size_t count=1;const auto byte=static_cast<unsigned char>(token[i]);if(byte>=0xf0)count=4;else if(byte>=0xe0)count=3;else if(byte>=0xc0)count=2;auto glyph=token.substr(i,count);const auto gw=measure(c,style,glyph,size);
    if(result.back().width>0&&result.back().width+gw>width)result.emplace_back();auto& line=result.back();line.runs.push_back({glyph,run.emphasis});line.width+=gw;line.height=std::max(line.height,size*style.lineHeight);i+=count;
   }
  }else{auto& line=result.back();line.runs.push_back({std::move(token),run.emphasis});line.width+=w;line.height=std::max(line.height,size*style.lineHeight);}
  at=end;
 }}
 for(auto& line:result){
  std::vector<UiRun> merged;for(const auto& run:line.runs){if(!merged.empty()&&merged.back().emphasis==run.emphasis)merged.back().text+=run.text;else merged.push_back(run);}line.runs=std::move(merged);line.width=0;
  for(const auto& run:line.runs)line.width+=measure(c,style,run.text,(run.emphasis?style.accentSize:style.size)*scale);
  if(line.height==0)line.height=style.size*scale*style.lineHeight;
 }
 return result;
}
struct Entry {UiPlaced placed;std::vector<int> children;int parent{-1};float sx{1},sy{1},fontScale{1};DesignBox local;std::vector<UiRun> runs;std::vector<Line> lines;};
struct ClayState {std::vector<std::byte> memory;Clay_Context* context{};std::string error;
 ClayState(){auto* old=Clay_GetCurrentContext();Clay_SetCurrentContext(nullptr);Clay_SetMaxElementCount(4096);Clay_SetMaxMeasureTextCacheWordCount(4096);memory.resize(Clay_MinMemorySize());context=Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(memory.size(),memory.data()),{1000,1000},{[](Clay_ErrorData d){static_cast<ClayState*>(d.userData)->error.assign(d.errorText.chars,d.errorText.length);},this});Clay_SetCurrentContext(old);}
};
Clay_ElementId clayId(int index){return CLAY_IDI("studio-node",index);}
}
MenuPose uiMotionPose(const UiMotion& motion,double elapsed,SDL_FPoint anchor){
 float value=1;if(elapsed<motion.duration){const double t=std::max(0.,elapsed)*.6/motion.duration;value=float(1-std::exp(-motion.damping*t)*(std::cos(motion.frequency*t)+(motion.damping/motion.frequency)*std::sin(motion.frequency*t)));}
 return {anchor,motion.scale+(1-motion.scale)*value,motion.scale+(1-motion.scale)*value,motion.rise*(1-value),std::clamp(value*1.8f,0.f,1.f)};
}
std::vector<UiRun> uiAmountRuns(std::string pattern,std::string amount){std::vector<UiRun> runs;std::size_t at=0;while(true){const auto pos=pattern.find("{amount}",at);if(pos==std::string::npos){runs.push_back({pattern.substr(at),false});break;}runs.push_back({pattern.substr(at,pos-at),false});runs.push_back({amount,true});at=pos+8;}return runs;}
Rect uiScreenBounds(const Canvas& canvas,const UiScreen& s){const auto in=canvas.safeInsets();const float w=canvas.width()-in.left-in.right,h=canvas.height()-in.top-in.bottom;const auto b=s.root.box;float u=std::min({s.maxWidth*canvas.minimumTouchSize()/44/b.w,w*s.safeFraction/b.w,h*s.safeFraction/b.h});return {in.left+(w-b.w*u)*.5f,in.top+(h-b.h*u)*.5f,b.w*u,b.h*u};}
UiRenderResult renderUiScreen(Canvas& canvas,const UiProject& project,std::string_view screenId,std::string_view variantId,Rect bounds,const UiBindings& input,const UiPaint& paint){
 const auto& screen=project.screen(screenId);UiVariant variant;
 if(auto it=screen.variants.find(std::string(variantId));it!=screen.variants.end())variant=it->second;else if(!screen.variants.empty())variant=screen.variants.begin()->second;
 auto values=variant.values;for(const auto& [k,v]:input.values)values[k]=v;
 std::vector<Entry> entries;entries.reserve(256);UiRenderResult output;output.bounds=bounds;
 std::function<int(const UiNode&,int,std::string,std::string,float,float,bool,int)> expand=[&](const UiNode& original,int par,std::string owner,std::string chain,float sx,float sy,bool enabled,int depth)->int{
  if(depth>24||entries.size()>3000)throw std::runtime_error("UI expansion exceeds its limit");
  UiNode n=original;const std::string key=chain+"|"+owner+"|"+n.id;bool overridden=false;
  if(auto it=variant.overrides.find(key);it!=variant.overrides.end()){UiJson j=n;j.merge_patch(it->second);n=j.get<UiNode>();overridden=true;}
  const auto visible=values.contains(n.visibleWhen)?uiText(values.at(n.visibleWhen),values):"";if(n.hidden||(!n.visibleWhen.empty()&&(visible.empty()||visible=="0")))return -1;
  auto style=uiStyle(project,n);if(!n.alertWhen.empty()&&values.contains(n.alertWhen)&&!values.at(n.alertWhen).empty()&&values.at(n.alertWhen)!="0")style.color=style.alert;if(!n.enabledWhen.empty()){const auto v=values.find(n.enabledWhen);enabled&=v!=values.end()&&!v->second.empty()&&v->second!="0";}enabled&=!paint.disabled;
  Entry e;e.parent=par;e.sx=sx;e.sy=sy;e.fontScale=std::min(sx,sy);e.local={n.box.x*sx,n.box.y*sy,n.box.w*sx,n.box.h*sy};
  const auto resolved=uiText(n.text,values);e.placed={key,owner,n.id,n.name,n.type,resolved,uiText(n.action,values),n.box,{},n,style,overridden,enabled,n.locked||(par>=0&&entries[par].placed.locked)};
  if(n.type=="richtext"){
   const auto binding=n.text.size()>2&&n.text.front()=='{'?n.text.substr(1,n.text.size()-2):"message";
   auto rich=input.rich.find(binding);if(rich!=input.rich.end())e.runs=rich->second;else{auto pattern=values.contains(binding)?values.at(binding):n.text;auto copied=values;copied.erase("amount");e.runs=uiAmountRuns(uiText(pattern,copied),values.contains("amount")?values.at("amount"):"");}
  }else if(!resolved.empty())e.runs={{resolved,false}};
  const int index=int(entries.size());entries.push_back(std::move(e));
  if(n.type=="instance"){
   const auto& definition=project.components.at(n.component);auto child=definition;child.box.x=child.box.y=0;
   const auto nextChain=chain.empty()?n.id:chain+"/"+n.id;
   int childIndex=expand(child,index,"@"+n.component,nextChain,sx*n.box.w/definition.box.w,sy*n.box.h/definition.box.h,enabled,depth+1);if(childIndex>=0)entries[index].children.push_back(childIndex);
  }
  for(const auto& c:n.children){int childIndex=expand(c,index,owner,chain,sx,sy,enabled,depth+1);if(childIndex>=0)entries[index].children.push_back(childIndex);}
  return index;
 };
 auto root=screen.root;root.box.x=root.box.y=0;int rootIndex=expand(root,-1,screen.id,"",bounds.w/root.box.w,bounds.h/root.box.h,true,0);if(rootIndex<0)return output;
 std::function<void(int,float,float)> size=[&](int index,float w,float h){
  auto& e=entries[index];const auto& n=e.placed.node;const auto& style=e.placed.style;e.local.w=w;e.local.h=h;
  if(!e.runs.empty()){if(n.layout.width=="hug"){const auto measured=lines(canvas,style,e.runs,4000,e.fontScale,false);float natural=0;for(const auto& line:measured)natural=std::max(natural,line.width);w=std::min(w,natural+style.padding*2*e.sx);e.local.w=w;}e.lines=lines(canvas,style,e.runs,std::max(1.f,w-style.padding*2*e.sx),e.fontScale,n.layout.wrap);const float textHeight=std::accumulate(e.lines.begin(),e.lines.end(),0.f,[](float sum,const Line& line){return sum+line.height;});if(n.layout.height=="hug")e.local.h=textHeight+style.padding*2*e.sy;}
  const float pad=(n.layout.padding<0?style.padding:n.layout.padding)*e.sx,gap=(n.layout.gap<0?style.gap:n.layout.gap)*e.sx;
  float used=pad*2+(e.children.empty()?0:gap*float(e.children.size()-1));int fill=0;
  if(n.layout.mode=="row")for(int child:e.children){const auto& c=entries[child];if(c.placed.node.layout.width=="fill")++fill;else used+=c.local.w;}
  for(int child:e.children){auto& c=entries[child];const auto& layout=c.placed.node.layout;float cw=c.local.w,ch=c.local.h;
   if(layout.width=="fill"||layout.horizontal=="stretch")cw=std::max(1.f,n.layout.mode=="row"&&fill?(w-used)/fill:w-pad*2);
   if(n.type=="instance"&&c.placed.owner=="@"+n.component){cw=w;ch=h;}
   size(child,cw,ch);
  }
  float cursor=pad,maxBottom=pad;
  std::map<std::string,int> siblings;for(int child:e.children)siblings[entries[child].placed.id]=child;
  std::set<int> placed,placing;
  std::function<void(int)> position=[&](int child){if(placed.contains(child))return;if(!placing.insert(child).second)throw std::runtime_error("Circular layout constraint");auto& c=entries[child];const auto& layout=c.placed.node.layout;
   if(n.layout.mode=="column"){c.local.x=pad+c.placed.node.box.x*c.sx;c.local.y=cursor;cursor+=c.local.h+gap;}
   else if(n.layout.mode=="row"){c.local.x=cursor;c.local.y=pad+c.placed.node.box.y*c.sy;cursor+=c.local.w+gap;}
   else{
    if(layout.horizontal=="center-alone"&&e.children.size()==1)c.local.x=(w-c.local.w)*.5f;
    else if(layout.horizontal=="center")c.local.x=(w-c.local.w)*.5f+c.placed.node.box.x*c.sx;
    else if(layout.horizontal=="right")c.local.x=w-c.local.w-c.placed.node.box.x*c.sx;
    else if(layout.horizontal=="stretch")c.local.x=pad;
    if(layout.vertical=="center")c.local.y=(e.local.h-c.local.h)*.5f+c.placed.node.box.y*c.sy;
    else if(layout.vertical=="bottom")c.local.y=e.local.h-c.local.h-c.placed.node.box.y*c.sy;
    if(!layout.after.empty()){auto it=siblings.find(layout.after);if(it!=siblings.end()){position(it->second);const auto& target=entries[it->second].local;c.local.y=target.y+target.h+(layout.gap<0?c.placed.style.gap:layout.gap)*c.sy;}}
   }
   if(n.layout.mode=="column"&&layout.horizontal=="center")c.local.x=(w-c.local.w)*.5f;
   if(n.layout.mode=="column"&&layout.horizontal=="right")c.local.x=w-pad-c.local.w;
   maxBottom=std::max(maxBottom,c.local.y+c.local.h+pad);placing.erase(child);placed.insert(child);
  };
  for(int child:e.children)position(child);
  if(n.layout.height=="hug"&&!e.children.empty())e.local.h=std::max(1.f,maxBottom);
 };
 size(rootIndex,bounds.w,bounds.h);
 static thread_local ClayState state;struct Restore{Clay_Context* old;~Restore(){Clay_SetCurrentContext(old);}} restore{Clay_GetCurrentContext()};Clay_SetCurrentContext(state.context);state.error.clear();Clay_SetLayoutDimensions({canvas.width(),canvas.height()});Clay_BeginLayout();
 std::function<void(int)> declare=[&](int index){const auto& e=entries[index];Clay_ElementDeclaration n{};n.id=clayId(index);n.layout.sizing={CLAY_SIZING_FIXED(e.local.w),CLAY_SIZING_FIXED(e.local.h)};n.floating.attachTo=e.parent<0?CLAY_ATTACH_TO_ROOT:CLAY_ATTACH_TO_PARENT;n.floating.offset=e.parent<0?Clay_Vector2{bounds.x,bounds.y}:Clay_Vector2{e.local.x,e.local.y};CLAY(n){for(int child:e.children)declare(child);}};
 declare(rootIndex);Clay_EndLayout();if(!state.error.empty())throw std::runtime_error("Layout: "+state.error);
 for(std::size_t i=0;i<entries.size();++i){const auto b=Clay_GetElementData(clayId(int(i))).boundingBox;entries[i].placed.box={b.x,b.y,b.width,b.height};}
 auto transform=[](Rect r,Rect from,Rect to){return Rect{to.x+(r.x-from.x)*to.w/from.w,to.y+(r.y-from.y)*to.h/from.h,r.w*to.w/from.w,r.h*to.h/from.h};};
 for(std::size_t i=0;i<entries.size();++i){auto& e=entries[i];auto r=e.placed.box;const auto& n=e.placed.node;const auto& s=e.placed.style;
  if(n.type=="button"){
   const Rect original=r;if(paint.buttonVisual)r=paint.buttonVisual(e.placed.action,r,s.pressedScale);
   else if(paint.pressed){r={r.x+r.w*(1-s.pressedScale)*.5f,r.y+r.h*(1-s.pressedScale)*.5f,r.w*s.pressedScale,r.h*s.pressedScale};}
   for(std::size_t j=i+1;j<entries.size();++j){int ancestor=entries[j].parent;while(ancestor>=0&&ancestor!=int(i))ancestor=entries[ancestor].parent;if(ancestor==int(i)){auto& child=entries[j];child.placed.box=transform(child.placed.box,original,r);float scale=std::min(r.w/original.w,r.h/original.h);child.fontScale*=scale;for(auto& line:child.lines){line.width*=scale;line.height*=scale;}}}
   if(e.placed.enabled&&paint.button&&!e.placed.action.empty())paint.button(e.placed.action,original);
   if(original.w<canvas.minimumTouchSize()||original.h<canvas.minimumTouchSize())output.problems.push_back({e.placed.key,"Button is smaller than the minimum touch target",false});
   e.placed.box=r;
  }
  float opacity=e.placed.enabled?1:s.disabledAlpha;const auto asset=uiText(n.asset,values);
  if(!asset.empty()&&asset.find('{')==std::string::npos){if(n.slice>0)canvas.surface(asset,r,n.slice,n.slice*std::min(e.sx,e.sy),opacity);else canvas.image(asset,r,n.rotation,{.5f,.5f},opacity);}
  if(!e.lines.empty()){
   const float pad=s.padding*e.sx;float available=std::max(1.f,r.w-pad*2);float y=r.y+s.padding*e.sy;
   const float totalHeight=std::accumulate(e.lines.begin(),e.lines.end(),0.f,[](float sum,const Line& line){return sum+line.height;});
   if(n.type=="button")y=r.y+std::max(0.f,(r.h-totalHeight)*.5f);
   if(totalHeight>r.h+1&&n.layout.height!="hug")output.problems.push_back({e.placed.key,"Text extends below its box; use Hug content or increase height",true});
   for(const auto& line:e.lines){float fit=n.layout.wrap?1:std::min(1.f,available/std::max(1.f,line.width));if(fit<.7f)output.problems.push_back({e.placed.key,"Text needs substantial shrinking; enlarge the box or wrap",false});
    float x=r.x+pad+(available-line.width*fit)*(s.align=="left"?0.f:s.align=="right"?1.f:.5f);
    for(const auto& run:line.runs){const float fs=(run.emphasis?s.accentSize:s.size)*e.fontScale*fit;const float top=y+(line.height/s.lineHeight-fs)*.99f;const auto ink=run.emphasis?s.accent:s.color;
     if(s.shadow[3])text(canvas,s,run.text,x+s.shadowX*e.fontScale,top+s.shadowY*e.fontScale,fs,color(s.shadow,opacity));
     if(s.outlineWidth>0)for(const auto offset:std::array<SDL_FPoint,8>{{{-1,0},{1,0},{0,-1},{0,1},{-.7f,-.7f},{.7f,-.7f},{-.7f,.7f},{.7f,.7f}}})text(canvas,s,run.text,x+offset.x*s.outlineWidth*e.fontScale,top+offset.y*s.outlineWidth*e.fontScale,fs,color(s.outline,opacity));
     text(canvas,s,run.text,x,top,fs,color(ink,opacity));x+=measure(canvas,s,run.text,fs);
    }y+=line.height;
   }
  }
  const auto safe=canvas.safeInsets();if(r.x<safe.left-.5f||r.y<safe.top-.5f||r.x+r.w>canvas.width()-safe.right+.5f||r.y+r.h>canvas.height()-safe.bottom+.5f)output.problems.push_back({e.placed.key,"Layer crosses the device safe area",true});
  output.elements.push_back(std::move(e.placed));
 }
 for(std::size_t i=0;i<output.elements.size();++i){const auto& item=output.elements[i];if(item.type!="text"&&item.type!="richtext")continue;
  for(std::size_t j=0;j<output.elements.size();++j){const auto& control=output.elements[j];if(control.type!="button")continue;int parent=entries[i].parent;bool own=false;while(parent>=0){if(parent==int(j)){own=true;break;}parent=entries[parent].parent;}if(own)continue;
   auto a=item.box;const auto b=control.box;const float height=std::accumulate(entries[i].lines.begin(),entries[i].lines.end(),0.f,[](float total,const Line& line){return total+line.height;});a.y+=item.style.padding*entries[i].sy;a.h=std::min(a.h,height);if(std::min(a.x+a.w,b.x+b.w)-std::max(a.x,b.x)>2&&std::min(a.y+a.h,b.y+b.h)-std::max(a.y,b.y)>2)output.problems.push_back({item.key,"Text overlaps "+control.name+"; adjust the layout or wrap width",true});
  }
 }
 return output;
}
}
