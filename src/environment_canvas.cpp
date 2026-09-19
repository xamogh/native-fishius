#include "aquarium/canvas.hpp"
#include <algorithm>
#include <cmath>

namespace aq {
SceneProjection sceneProjection(Rect viewport){
 // Both approved backgrounds use this art canvas. Cover it uniformly and
 // anchor its bottom so the foreground floor remains visible on wide phones.
 constexpr float artWidth=1672,artHeight=940;
 const float scale=std::max(viewport.w/artWidth,viewport.h/artHeight);
 const float w=artWidth*scale,h=artHeight*scale;
 return {viewport,{viewport.x+(viewport.w-w)*.5f,viewport.y+viewport.h-h,w,h}};
}
SDL_FPoint SceneProjection::toScreen(WorldPoint p)const{
 return {artwork.x+float(p.x/waterWidth)*artwork.w,artwork.y+float(p.y/tankHeight)*artwork.h};
}
WorldPoint SceneProjection::toWorld(SDL_FPoint p)const{
 return {double((p.x-artwork.x)/artwork.w)*waterWidth,double((p.y-artwork.y)/artwork.h)*tankHeight};
}
float SceneProjection::decorUnit()const{return std::min(artwork.w/12.f,artwork.h/7.f);}
WorldPoint SceneProjection::place(const DecorDef& def,WorldPoint point,double sizeMul)const{
 point=decorPlacementPoint(def,point,sizeMul);
 // Clamp only an active placement, never a saved layout during rendering.
 // Perspective grows as an item moves inward, so fit its updated footprint.
 for(int i=0;i<256;++i){
  const float unit=decorUnit()*float(decorScale(point)*sizeMul);
  const float half=std::min(viewport.w*.5f,float(def.width)*unit*.5f);
  const float height=std::min(viewport.h,float(def.height)*unit);
  const auto p=toScreen(point);
  if(p.x>=viewport.x+half-.0001f&&p.x<=viewport.x+viewport.w-half+.0001f&&
     p.y>=viewport.y+height-.0001f&&p.y<=viewport.y+viewport.h+.0001f)return point;
  const auto next=decorPlacementPoint(def,toWorld({std::clamp(p.x,viewport.x+half,viewport.x+viewport.w-half),
   std::clamp(p.y,viewport.y+height,viewport.y+viewport.h)}),sizeMul);
  if(std::hypot(next.x-point.x,next.y-point.y)<.00001)return next;
  point=next;
 }
 return point;
}
void Canvas::environment(Rect r,std::string_view backgroundId){
 if(r.w<=0||r.h<=0)return;
 const auto* background=findEnvironment(backgroundId);
 if(!background)background=findEnvironment("sunlit-lagoon");
 const bool clipped=SDL_RenderClipEnabled(renderer_);SDL_Rect previous{};SDL_GetRenderClipRect(renderer_,&previous);
 clip(r);
 if(clipped){SDL_Rect current{},intersection{};SDL_GetRenderClipRect(renderer_,&current);SDL_GetRectIntersection(&previous,&current,&intersection);SDL_SetRenderClipRect(renderer_,&intersection);}
 // The approved artwork supplies its own restrained colors and broad shapes.
 // The same projection anchors decorations and their size to this artwork.
 image(background->asset,sceneProjection(r).artwork);
 SDL_SetRenderClipRect(renderer_,clipped?&previous:nullptr);
}
}
