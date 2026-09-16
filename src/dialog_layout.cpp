#include "aquarium/dialog_design.hpp"
#include "clay.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace aq {
namespace {
struct LayoutContext {
 std::vector<std::byte> memory;Clay_Context* context{};std::string error;
 LayoutContext(){
  auto* previous=Clay_GetCurrentContext();Clay_SetCurrentContext(nullptr);
  Clay_SetMaxElementCount(128);Clay_SetMaxMeasureTextCacheWordCount(256);
  memory.resize(Clay_MinMemorySize());
  context=Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(memory.size(),memory.data()),{888,732},
   {[](Clay_ErrorData data){static_cast<LayoutContext*>(data.userData)->error.assign(data.errorText.chars,data.errorText.length);},this});
  Clay_SetCurrentContext(previous);
 }
};
Clay_ElementId elementId(std::size_t i){const auto* id=dialogPartIds[i];return Clay_GetElementId({false,static_cast<int32_t>(std::strlen(id)),id});}
}
DialogLayout layoutDialog(const DialogDesign& design,DesignBox safe,float unitsPerPoint){
 static thread_local LayoutContext state;
 struct Restore {Clay_Context* previous;~Restore(){Clay_SetCurrentContext(previous);}} restore{Clay_GetCurrentContext()};
 Clay_SetCurrentContext(state.context);state.error.clear();
 const auto& frame=design.element(DialogPart::Frame).box;
 const float u=std::min({design.maxWidth*unitsPerPoint/888.f,safe.w*design.safeFraction/888.f,safe.h*design.safeFraction/732.f});
 DialogLayout result;result.unit=u;
 const float x=safe.x+(safe.w-888*u)*.5f,y=safe.y+(safe.h-732*u)*.5f;
 Clay_SetLayoutDimensions({safe.x+safe.w,safe.y+safe.h});Clay_BeginLayout();
 Clay_ElementDeclaration root{};root.id=CLAY_ID("dialog-root");
 root.layout.sizing={CLAY_SIZING_FIXED(888*u),CLAY_SIZING_FIXED(732*u)};
 root.floating.attachTo=CLAY_ATTACH_TO_ROOT;root.floating.offset={x,y};
 CLAY(root){
  for(std::size_t i=0;i<design.elements.size();++i){const auto& box=design.elements[i].box;
   Clay_ElementDeclaration node{};node.id=elementId(i);
   node.layout.sizing={CLAY_SIZING_FIXED(box.w*u),CLAY_SIZING_FIXED(box.h*u)};
   node.floating.attachTo=CLAY_ATTACH_TO_PARENT;node.floating.offset={box.x*u,box.y*u};
   CLAY(node){}
  }
 }
 Clay_EndLayout();
 if(!state.error.empty())throw std::runtime_error("Dialog layout: "+state.error);
 for(std::size_t i=0;i<result.elements.size();++i){const auto b=Clay_GetElementData(elementId(i)).boundingBox;result.elements[i]={b.x,b.y,b.width,b.height};}
 result.frame={x+frame.x*u,y+frame.y*u,frame.w*u,frame.h*u};return result;
}
}
