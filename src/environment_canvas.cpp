#include "aquarium/canvas.hpp"
#include <algorithm>

namespace aq {
void Canvas::environment(Rect r,std::string_view backgroundId){
 if(r.w<=0||r.h<=0)return;
 const auto* background=findEnvironment(backgroundId);
 if(!background)background=findEnvironment("sunlit-lagoon");
 const bool clipped=SDL_RenderClipEnabled(renderer_);SDL_Rect previous{};SDL_GetRenderClipRect(renderer_,&previous);
 clip(r);
 if(clipped){SDL_Rect current{},intersection{};SDL_GetRenderClipRect(renderer_,&current);SDL_GetRectIntersection(&previous,&current,&intersection);SDL_SetRenderClipRect(renderer_,&intersection);}
 // One opaque scene, uniformly scaled. Bottom anchoring keeps the sand
 // visible on wide phones; the center stays clear when tablets crop the sides.
 const auto* art=texture(background->asset);
 const float scale=std::max(r.w/float(art->width),r.h/float(art->height));
 image(background->asset,{r.x+(r.w-art->width*scale)*.5f,r.y+r.h-art->height*scale,art->width*scale,art->height*scale});
 SDL_SetRenderClipRect(renderer_,clipped?&previous:nullptr);
}
}
