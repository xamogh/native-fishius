#pragma once
#include "aquarium/view.hpp"
#include "aquarium/ui_project.hpp"
namespace aq {
struct UiPlaced {
 std::string key,owner,id,name,type,text,action;DesignBox reference;Rect box;
 UiNode node;UiStyle style;bool overridden{},enabled{true},locked{};
};
struct UiProblem {std::string key,message;bool error{};};
struct UiRenderResult {std::vector<UiPlaced> elements;std::vector<UiProblem> problems;Rect bounds;};
struct UiPaint {
 std::function<Rect(std::string_view,Rect,float)> buttonVisual;
 std::function<void(std::string_view,Rect)> button;
 bool pressed{},disabled{};
};
MenuPose uiMotionPose(const UiMotion&,double elapsed,SDL_FPoint anchor);
UiRenderResult renderUiScreen(Canvas&,const UiProject&,std::string_view screen,std::string_view variant,Rect bounds,const UiBindings& bindings={},const UiPaint& paint={});
Rect uiScreenBounds(const Canvas&,const UiScreen&);
std::vector<UiRun> uiAmountRuns(std::string pattern,std::string amount);
}
