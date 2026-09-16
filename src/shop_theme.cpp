#include "aquarium/shop_theme.hpp"
#include <algorithm>

namespace aq::shopTheme {
namespace {
struct Finish {Color top,bottom,edge,rim;};
Finish finish(Surface surface){
 switch(surface){
 case Surface::Tab:return {{176,232,214},{123,199,192},{49,116,119},{225,255,235,185}};
 case Surface::SelectedTab:return {{251,249,229},{222,233,209},{81,132,124},{255,255,248,230}};
 case Surface::Positive:return {{123,217,155},{69,176,124},{31,101,99},{220,255,199,200}};
 case Surface::Button:return {{85,168,176},{40,119,133},{24,78,92},{183,234,223,160}};
 case Surface::Well:return {{30,113,127},{13,72,85},{24,74,84},{116,197,196,160}};
 case Surface::Card:return {{66,207,218},{49,186,202},{20,98,115},{156,246,237,210}};
 case Surface::LockedCard:return {{108,182,186},{78,149,159},{36,101,113},{180,224,212,150}};
 case Surface::Close:return {{255,117,101},{226,61,49},{139,54,47},{255,204,179,190}};
 case Surface::FishCard:return {{255,249,230},{245,232,194},{82,143,130},{255,255,239,225}};
 case Surface::LockedFishCard:return {{241,238,221},{224,224,199},{94,141,133},{255,255,239,160}};
 case Surface::FastBadge:return {{241,164,91},{214,130,56},{162,94,47},{255,222,166,180}};
 case Surface::Buy:return {{98,226,111},{42,181,76},{39,137,70},{213,255,183,200}};
 case Surface::LockedPrice:return {{157,182,143},{115,153,116},{92,129,91},{220,237,191,140}};
 }
 return {};
}
Rect inset(Rect rect,float value){return {rect.x+value,rect.y+value,rect.w-value*2,rect.h-value*2};}
Color darken(Color color){return {Uint8(color.r*.86f),Uint8(color.g*.86f),Uint8(color.b*.86f),color.a};}
void bubble(Canvas& canvas,float x,float y,float diameter){
 const Rect bounds{x,y,diameter,diameter};
 canvas.gradient(bounds,{204,255,252,45},{79,202,217,8},diameter*.5f);
 canvas.outline(bounds,{193,252,246,58},diameter*.5f,diameter*.06f);
 canvas.gradient({x+diameter*.2f,y+diameter*.12f,diameter*.28f,diameter*.24f},{246,255,255,200},{218,255,255,25},diameter*.12f);
}
}

void panel(Canvas& canvas,Rect rect,float unit,Surface surface,bool pressed){
 if(rect.w<=0||rect.h<=0)return;
 const auto palette=finish(surface);
 const float radius=12*unit;
 canvas.gradient({rect.x,rect.y+4*unit,rect.w,rect.h},{8,49,62,70},{8,49,62,110},radius);
 canvas.gradient(rect,palette.edge,palette.edge,radius);
 const auto face=inset(rect,4*unit);
 canvas.gradient(face,pressed?darken(palette.top):palette.top,pressed?darken(palette.bottom):palette.bottom,8*unit);
 if(pressed)return;
 // The narrow rim and soft top reflection keep every surface light and readable.
 canvas.outline(inset(rect,2*unit),palette.rim,10*unit,unit);
 const float height=std::min(12*unit,face.h*.24f);
 canvas.gradient({face.x+4*unit,face.y,face.w-8*unit,height},{255,255,240,90},{255,255,240,0},4*unit);
}

void badge(Canvas& canvas,Rect bounds,float u,std::string_view label,Surface style){
 panel(canvas,bounds,u,style);
 canvas.text(label,bounds.x+bounds.w*.5f,bounds.y+(bounds.h-24*u)*.5f,24*u,white,true,bounds.w-16*u,true,true);
}

void clockIcon(Canvas& canvas,Rect bounds,Color clockInk){
 const float side=std::min(bounds.w,bounds.h),u=side/24;
 const float x=bounds.x+(bounds.w-side)*.5f,y=bounds.y+(bounds.h-side)*.5f;
 canvas.gradient({x,y,side,side},clockInk,clockInk,side*.5f);
 canvas.gradient({x+2*u,y+2*u,side-4*u,side-4*u},{255,253,237},{246,229,186},side*.5f-2*u);
 canvas.gradient({x+11*u,y+5*u,2*u,8*u},clockInk,clockInk,u);
 canvas.gradient({x+11*u,y+11*u,7*u,2*u},clockInk,clockInk,u);
}

void backdrop(Canvas& canvas,const ShopLayout& layout){
 const float u=layout.unit;
 const Rect header{0,0,layout.page.w,layout.title.y};
 canvas.gradient(header,{22,129,142},{10,85,105});
 // Place decoration in the empty space beside the tabs.
 const float left=layout.tabs.front().x;
 bubble(canvas,left*.2f,header.h*.2f,28*u);
 bubble(canvas,left*.3f,header.h*.48f,20*u);
 bubble(canvas,left*.22f,header.h*.7f,12*u);
 const float right=layout.tabs.back().x+layout.tabs.back().w;
 const float space=std::max(0.f,layout.close.x-right);
 bubble(canvas,right+space*.48f,header.h*.38f,24*u);
 bubble(canvas,right+space*.72f,header.h*.64f,32*u);
 bubble(canvas,right+space*.52f,header.h*.68f,12*u);
 const Rect paper{0,header.h,layout.page.w,layout.footer.y-header.h};
 canvas.gradient(paper,{244,239,218},{244,243,226});
 const Rect light{paper.x,paper.y+12*u,paper.w,136*u};
 canvas.wave(light,{255,255,247,105},20*u,720*u,.3f,28*u);
 canvas.wave(light,{255,255,247,75},24*u,912*u,2.6f,36*u);
 canvas.fill({0,paper.y,layout.page.w,4*u},{245,255,232,150});
}

void footer(Canvas& canvas,const ShopLayout& layout){
 const float u=layout.unit;
 const Rect bounds{0,layout.footer.y,layout.page.w,layout.page.h-layout.footer.y};
 canvas.gradient(bounds,{146,224,196},{102,196,179});
 canvas.wave({0,bounds.y+8*u,bounds.w,bounds.h*.7f},{214,255,223,95},20*u,760*u,.2f,12*u);
 canvas.wave({0,bounds.y+28*u,bounds.w,bounds.h*.7f},{207,255,224,65},20*u,624*u,2.3f,12*u);
 canvas.fill({0,bounds.y,bounds.w,4*u},{237,255,217,210});
 // Keep the wallet clear, with bubbles confined to the outer edges.
 const float left=layout.currencyHud[HudPart::CoinIcon].x;
 bubble(canvas,left*.2f,bounds.y+32*u,20*u);
 bubble(canvas,left*.28f,bounds.y+56*u,12*u);
 bubble(canvas,left*.76f,bounds.y+72*u,20*u);
 const float right=layout.currencyHud[HudPart::Pearls].x+layout.currencyHud[HudPart::Pearls].w;
 const float space=bounds.w-right;
 bubble(canvas,right+space*.58f,bounds.y+52*u,16*u);
 bubble(canvas,right+space*.85f,bounds.y+32*u,12*u);
}

void card(Canvas& canvas,Rect bounds,float u,bool locked){
 panel(canvas,bounds,u,locked?Surface::LockedCard:Surface::Card);
 const auto face=inset(bounds,8*u);
 const Rect light{face.x,face.y+face.h*.3f,face.w,face.h*.45f};
 canvas.wave(light,{204,255,237,35},face.h*.06f,face.w*2,1.4f,48*u);
 canvas.wave(light,{214,255,244,22},face.h*.1f,face.w*3,3.2f,64*u);
 bubble(canvas,bounds.x+16*u,bounds.y+20*u,20*u);
 bubble(canvas,bounds.x+24*u,bounds.y+48*u,12*u);
 bubble(canvas,bounds.x+16*u,bounds.y+bounds.h-108*u,12*u);
 bubble(canvas,bounds.x+bounds.w-40*u,bounds.y+bounds.h-100*u,20*u);
 bubble(canvas,bounds.x+bounds.w-28*u,bounds.y+bounds.h-124*u,8*u);
}

void tabArt(Canvas& canvas,Rect box,int category){
 // Alpha bounds fit the illustrations consistently without modifying their PNGs.
 struct Art {std::string_view path;Rect bounds;float size;};
 constexpr std::array<Art,4> assets{{
  {"hud-icons/shop-tab-fish-v1.png",{88,177,1134,854},1254},
  {"hud-icons/shop-tab-plants-v1.png",{99,61,1082,1116},1254},
  {"hud-icons/shop-tab-decorations-v1.png",{43,291,1176,713},1254},
  {"hud-icons/shop-tab-treasure-v1.png",{35,129,1184,982},1254}
 }};
 if(category<0||category>=int(assets.size()))return;
 const auto& art=assets[category];
 const float scale=std::min(box.w/art.bounds.w,box.h/art.bounds.h);
 const float x=box.x+(box.w-art.bounds.w*scale)*.5f;
 const float y=box.y+(box.h-art.bounds.h*scale)*.5f;
 canvas.image(art.path,{x-art.bounds.x*scale,y-art.bounds.y*scale,art.size*scale,art.size*scale});
}
}
