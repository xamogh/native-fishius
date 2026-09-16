#include "aquarium/studio.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace aq::studio {
namespace {
struct PreviewDraw {SDL_Renderer* renderer;SDL_Texture* texture;Rect rect;};
void drawSoftwarePreview(const ImDrawList*,const ImDrawCmd* cmd){
 const auto& data=*static_cast<const PreviewDraw*>(cmd->UserCallbackData);auto* renderer=data.renderer;float sx,sy;SDL_GetRenderScale(renderer,&sx,&sy);const auto* viewport=ImGui::GetMainViewport();
 SDL_Rect clip{int((cmd->ClipRect.x-viewport->Pos.x)*sx),int((cmd->ClipRect.y-viewport->Pos.y)*sy),int((cmd->ClipRect.z-cmd->ClipRect.x)*sx),int((cmd->ClipRect.w-cmd->ClipRect.y)*sy)};
 SDL_SetRenderScale(renderer,1,1);SDL_SetRenderClipRect(renderer,&clip);SDL_FRect target{(data.rect.x-viewport->Pos.x)*sx,(data.rect.y-viewport->Pos.y)*sy,data.rect.w*sx,data.rect.h*sy};SDL_RenderTexture(renderer,data.texture,nullptr,&target);SDL_SetRenderClipRect(renderer,nullptr);SDL_SetRenderScale(renderer,sx,sy);
}
}

Preview::Preview(Canvas& c,const Content& content):session(content,std::filesystem::temp_directory_path()/"aquarium-studio-preview-unused.json",1000,true),view(c,session),canvas(c){auto s=session.domain().state();s.settings.sound=false;s.settings.music=false;s.tutorialStep=11;session.domain().install(s);}
Preview::~Preview(){if(image)SDL_DestroyTexture(image);}
void Preview::refresh(const UiProject& project,std::uint64_t version,const Stage& config,bool play){
 if(initialized&&!invalid&&revision==version&&stage==config&&!play)return;
 const bool reset=!initialized||stage!=config;stage=config;revision=version;invalid=false;
 canvas.previewViewport(PreviewViewport{stage.width,stage.height,stage.density,stage.safe?Insets{32,0,32,16}:Insets{}});view.setUiProject(project);
 if(reset){started=SDL_GetTicks()/1000.;
  if(stage.flow)view.previewPurchase(stage.level,stage.coins,stage.pearls,stage.tank,stage.owned,stage.upgrades);
  else{UiBindings bindings;if(stage.screen=="general-dialog"||stage.screen=="__component")bindings.values["amount"]=stage.scenario==1?"9,999,999":compact(stage.amount);
   if(stage.scenario==1){if(stage.screen=="currency-hud")bindings.values={{"coins","999,999,999"},{"pearls","999,999"}};else if(stage.screen=="shop-card")bindings.values["amount"]="999,999,999";}
   if(stage.scenario==2)bindings.values={{"title","Nicht genügend Münzen"},{"detail","Du kannst zusätzliche Münzen im Shop erhalten."},{"actionLabel","Shop jetzt öffnen"},{"message","Du brauchst noch {amount} Münzen"},{"amount",compact(stage.amount)}};
   if(stage.scenario==3)bindings.values["extra"]="Requires level 12";
   view.previewUiScreen(stage.screen,stage.variant,bindings,stage.scenario==4,stage.scenario==5,stage.elapsed);
  }
 }
 const double time=play?1+SDL_GetTicks()/1000.-started:1;
 view.render(time);logicalW=canvas.width();logicalH=canvas.height();layout=view.uiLayout()?*view.uiLayout():UiRenderResult{};
 const float w=float(stage.width*stage.density),h=float(stage.height*stage.density);float oldW=0,oldH=0;if(image)SDL_GetTextureSize(image,&oldW,&oldH);
 auto* renderer=canvas.renderer();if(!image||oldW!=w||oldH!=h){if(image)SDL_DestroyTexture(image);image=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,int(w),int(h));if(!image)throw std::runtime_error(SDL_GetError());SDL_SetTextureBlendMode(image,SDL_BLENDMODE_NONE);SDL_SetTextureScaleMode(image,SDL_SCALEMODE_LINEAR);}
 SDL_SetRenderTarget(renderer,image);SDL_SetRenderScale(renderer,1,1);SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);SDL_RenderTexture(renderer,canvas.frameTexture(),nullptr,nullptr);SDL_SetRenderTarget(renderer,nullptr);initialized=true;++renders;
}
void Preview::event(Uint32 kind,float x,float y){canvas.previewViewport(PreviewViewport{stage.width,stage.height,stage.density,stage.safe?Insets{32,0,32,16}:Insets{}});SDL_Event e{};e.type=kind;if(kind==SDL_EVENT_MOUSE_MOTION){e.motion.x=x;e.motion.y=y;}else{e.button.button=SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;}view.event(e,1+SDL_GetTicks()/1000.-started);invalid=true;}
void Preview::capture(const std::filesystem::path& path){if(!image)throw std::runtime_error("The preview has not rendered yet");if(path.has_parent_path())std::filesystem::create_directories(path.parent_path());auto* renderer=canvas.renderer();auto* previous=SDL_GetRenderTarget(renderer);SDL_SetRenderTarget(renderer,image);auto* surface=SDL_RenderReadPixels(renderer,nullptr);bool saved=surface&&IMG_SavePNG(surface,path.string().c_str());if(surface)SDL_DestroySurface(surface);SDL_SetRenderTarget(renderer,previous);if(!saved)throw std::runtime_error(SDL_GetError());}
void Studio::workspace(){
 ImGui::SetNextItemWidth(140);if(ImGui::BeginCombo("##variant",variant_.c_str())){for(const auto& [id,_]:document_.project().screen(screen_).variants)if(ImGui::Selectable(id.c_str(),id==variant_))variant_=id;ImGui::EndCombo();}ImGui::SameLine();
 ImGui::SetNextItemWidth(142);ImGui::Combo("##device",&device_,[](void*,int i){return devices[i].name;},nullptr,int(devices.size()));ImGui::SameLine();
 ImGui::SetNextItemWidth(142);ImGui::Combo("##comparison",&comparison_,"Single preview\0Coins + pearls\0Four devices\0Four states\0Draft + approved\0");remember("comparison");ImGui::SameLine();
 if(ImGui::SmallButton("Fit")){zoom_=1;pan_={};}ImGui::SameLine();ImGui::TextDisabled("%.0f%%",zoom_*100);
 if(device_==4){int dimensions[]{customW_,customH_};ImGui::SetNextItemWidth(200);if(ImGui::InputInt2("Device points",dimensions)){customW_=std::clamp(dimensions[0],320,2560);customH_=std::clamp(dimensions[1],240,1600);}}
 if(!component_.empty()){ImGui::TextColored(accent,"Component / %s",component_.c_str());ImGui::SameLine();if(ImGui::SmallButton("Return to screen"))selectScreen(screen_);}
 if(flow_)ImGui::TextColored(accent,"PURCHASE TEST  /  Level %d  /  %lld coins  /  %lld pearls",level_,static_cast<long long>(coins_),static_cast<long long>(pearls_));
 const int count=(comparison_==2||comparison_==3)?4:comparison_?2:1;const int columns=count>1?2:1;
 const float bottomHeight=bottom_?204.f:0;const float canvasHeight=std::max(230.f,ImGui::GetContentRegionAvail().y-bottomHeight-37);
 const float tileW=(ImGui::GetContentRegionAvail().x-(columns-1)*8)/columns,tileH=(canvasHeight-(count==4?8:0))/(count==4?2:1);
 auto project=previewProject(document_.project());auto approved=previewProject(document_.approved());
 for(int i=0;i<count;++i){auto config=stage(i);auto& preview=*previews_[i];const bool original=comparison_==4&&i==1;
  // Each view owns its own session and cached texture. Inactive comparisons
  // never borrow a mutable frame from another device or currency.
  preview.refresh(original?approved:project,document_.version(),config,play_&&i==0);
  canvasTile(i,preview,config,original,{tileW,tileH});if(columns==2&&i%2==0)ImGui::SameLine(0,8);
 }
 ImGui::Checkbox("Guides",&guides_);ImGui::SameLine();ImGui::Checkbox("Snap",&snap_);ImGui::SameLine();ImGui::Checkbox("Safe area",&safe_);ImGui::SameLine();ImGui::SetNextItemWidth(56);int density=density_-1;if(ImGui::Combo("##density",&density,"1x\0 2x\0"))density_=density+1;ImGui::SameLine();if(ImGui::SmallButton(bottom_?"Hide tools":"Show tools"))bottom_=!bottom_;
 if(bottom_){ImGui::BeginChild("Tools",{0,0},ImGuiChildFlags_Borders);bottomPanel();ImGui::EndChild();}
}
void Studio::canvasTile(int index,Preview& preview,const Stage& config,bool approved,ImVec2 size){ImGui::PushID(index);ImGui::BeginChild("Canvas tile",size,ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
 std::string name=approved?"APPROVED":comparison_==4?"DRAFT":config.variant;
 ImGui::TextDisabled("%s  /  %d x %d  /  %dx",name.c_str(),config.width,config.height,config.density);
 const auto pos=ImGui::GetCursorScreenPos(),available=ImGui::GetContentRegionAvail();float scale=std::max(.01f,std::min(available.x/config.width,available.y/config.height)*zoom_);
 Rect rect{pos.x+(available.x-config.width*scale)*.5f+pan_.x,pos.y+(available.y-config.height*scale)*.5f+pan_.y,config.width*scale,config.height*scale};auto* draw=ImGui::GetWindowDrawList();if(index==0)controls_["canvas-image"]=rect;
 ImGui::InvisibleButton("Canvas",available,ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonMiddle);const bool hovered=ImGui::IsItemHovered();
 draw->PushClipRect(pos,{pos.x+available.x,pos.y+available.y},true);draw->AddRectFilled(pos,{pos.x+available.x,pos.y+available.y},IM_COL32(19,24,30,255));if(preview.image){
  if(options_.software){PreviewDraw data{canvas_.renderer(),preview.image,rect};draw->AddCallback(drawSoftwarePreview,&data,sizeof(data));draw->AddCallback(ImDrawCallback_ResetRenderState,nullptr);}
  else draw->AddImage(ImTextureRef((ImTextureID)(intptr_t)preview.image),{rect.x,rect.y},{rect.x+rect.w,rect.y+rect.h});
 }
 if(index==0){if(!play_){
   if(ImGui::BeginDragDropTarget()){
    if(const auto* payload=ImGui::AcceptDragDropPayload("AQ_COMPONENT"))addNode("instance",static_cast<const char*>(payload->Data));
    if(const auto* payload=ImGui::AcceptDragDropPayload("AQ_ASSET")){const std::string path=static_cast<const char*>(payload->Data);bool replaced=false;for(auto it=preview.layout.elements.rbegin();it!=preview.layout.elements.rend();++it){const auto& e=*it;Rect hit{rect.x+e.box.x*rect.w/preview.logicalW,rect.y+e.box.y*rect.h/preview.logicalH,e.box.w*rect.w/preview.logicalW,e.box.h*rect.h/preview.logicalH};if(hit.has(ImGui::GetIO().MousePos.x,ImGui::GetIO().MousePos.y)&&e.type=="image"&&!e.locked){selection_={pick(e)};auto p=document_.project();auto n=editableNode(p);if(n.asset.starts_with("{")&&n.asset.ends_with("}"))p.screen(screen_).variants[variant_].values[n.asset.substr(1,n.asset.size()-2)]=path;else n.asset=path;writeNode(p,n);apply(p,"drop-artwork");replaced=true;break;}}if(!replaced)addNode("image",{},path);}
    ImGui::EndDragDropTarget();
   }manipulate(preview,rect,hovered);
  }else{auto& io=ImGui::GetIO();auto send=[&](Uint32 kind){preview.event(kind,(io.MousePos.x-rect.x)/rect.w*config.width,(io.MousePos.y-rect.y)/rect.h*config.height);};if(hovered&&rect.has(io.MousePos.x,io.MousePos.y)){send(SDL_EVENT_MOUSE_MOTION);if(ImGui::IsMouseClicked(0))send(SDL_EVENT_MOUSE_BUTTON_DOWN);}if(ImGui::IsMouseReleased(0))send(SDL_EVENT_MOUSE_BUTTON_UP);}
 }
 const auto& io=ImGui::GetIO();if(hovered){if(io.MouseWheel!=0){zoom_=std::clamp(zoom_*std::pow(1.12f,io.MouseWheel),.2f,5.f);}
  if(ImGui::IsMouseClicked(2)||(io.KeyAlt&&ImGui::IsMouseClicked(0))||(ImGui::IsKeyDown(ImGuiKey_Space)&&ImGui::IsMouseClicked(0))){panning_=true;dragStart_=io.MousePos;panStart_=pan_;}
 }if(panning_){if(ImGui::IsMouseDown(2)||ImGui::IsMouseDown(0))pan_={panStart_.x+io.MousePos.x-dragStart_.x,panStart_.y+io.MousePos.y-dragStart_.y};else panning_=false;}
 if(safe_){const float sx=rect.w/config.width,sy=rect.h/config.height;draw->AddRect({rect.x+32*sx,rect.y},{rect.x+rect.w-32*sx,rect.y+rect.h-16*sy},IM_COL32(255,203,116,190),0,0,1);}
 draw->PopClipRect();ImGui::EndChild();ImGui::PopID();
}
void Studio::manipulate(Preview& preview,Rect image,bool hovered){auto& io=ImGui::GetIO();auto* draw=ImGui::GetWindowDrawList();const float sx=image.w/preview.logicalW,sy=image.h/preview.logicalH;const auto toScreen=[&](Rect r){return Rect{image.x+r.x*sx,image.y+r.y*sy,r.w*sx,r.h*sy};};
 const bool alternate=io.KeyAlt||ImGui::IsKeyDown(ImGuiKey_Space);bool handle=false;
 if(selection_.size()==1)if(auto* e=placed(selection_[0])){auto r=toScreen(e->box);handle=Rect{r.x+r.w-7,r.y+r.h-7,14,14}.has(io.MousePos.x,io.MousePos.y);}
 if(hovered&&!alternate&&ImGui::IsMouseClicked(0)&&inlineKey_.empty()){
  const UiPlaced* hit=nullptr;if(handle&&selection_.size()==1)hit=placed(selection_[0]);
  if(!hit)for(auto it=preview.layout.elements.rbegin();it!=preview.layout.elements.rend();++it){if(it->locked||it->type=="instance"||it->type=="group")continue;if(toScreen(it->box).has(io.MousePos.x,io.MousePos.y)){hit=&*it;break;}}
  if(hit){const bool chosen=std::any_of(selection_.begin(),selection_.end(),[&](const auto& s){return s.key==hit->key;});if(!chosen&&!io.KeyShift)selection_.clear();if(io.KeyShift&&chosen)std::erase_if(selection_,[&](const auto& s){return s.key==hit->key;});else if(!chosen)selection_.push_back(pick(*hit));override_=!selection_.empty()&&placed(selection_.back())?placed(selection_.back())->overridden:hit->overridden;
   if(ImGui::IsMouseDoubleClicked(0)&&!hit->node.text.empty()){
    selection_={{hit->key,hit->owner,hit->id}};inlineKey_=hit->key;inlineBuffer_=hit->node.text;const auto& values=document_.project().screen(screen_).variants.at(variant_).values;if(inlineBuffer_.starts_with("{")&&inlineBuffer_.ends_with("}")&&inlineBuffer_.find('}')==inlineBuffer_.size()-1){auto key=inlineBuffer_.substr(1,inlineBuffer_.size()-2);if(values.contains(key))inlineBuffer_=values.at(key);}inlineRect_=toScreen(hit->box);inlineFocus_=true;dragging_=false;
   }else{dragging_=!selection_.empty();resizing_=handle;dragProject_=document_.project();dragStart_=io.MousePos;dragPlaced_.clear();for(const auto& s:selection_)if(auto* e=placed(s))dragPlaced_.push_back(*e);}
  }else{if(!io.KeyShift)selection_.clear();marquee_=true;marqueeStart_=io.MousePos;}
 }
 if(dragging_&&ImGui::IsMouseDown(0)&&!alternate){float pixelX=io.MousePos.x-dragStart_.x,pixelY=io.MousePos.y-dragStart_.y;if(std::abs(pixelX)+std::abs(pixelY)>2){auto p=dragProject_;for(const auto& e:dragPlaced_){auto* base=p.node(e.owner,e.id);if(!base)continue;auto n=*base;if(override_&&!component_.empty())override_=false;if(override_){auto it=p.screen(screen_).variants[variant_].overrides.find(e.key);if(it!=p.screen(screen_).variants[variant_].overrides.end()){UiJson j=n;j.merge_patch(it->second);n=j.get<UiNode>();}}
   float dx=pixelX/(sx*e.box.w/e.reference.w),dy=pixelY/(sy*e.box.h/e.reference.h);auto round=[&](float x){return snap_?std::round(x/4)*4:x;};if(resizing_){n.box.w=std::clamp(round(n.box.w+dx),1.f,4000.f);n.box.h=std::clamp(round(n.box.h+dy),1.f,4000.f);}else{n.box.x=std::clamp(round(n.box.x+(n.layout.horizontal=="right"?-dx:dx)),-4000.f,4000.f);n.box.y=std::clamp(round(n.box.y+(n.layout.vertical=="bottom"?-dy:dy)),-4000.f,4000.f);}
   if(override_){UiJson patch=p.screen(screen_).variants[variant_].overrides[e.key];if(!patch.is_object())patch=UiJson::object();patch["box"]=n.box;p.screen(screen_).variants[variant_].overrides[e.key]=patch;}else *base=n;
  }apply(p,"canvas-drag");}}
 if(ImGui::IsMouseReleased(0)){dragging_=false;resizing_=false;document_.endEdit();if(marquee_){Rect area{std::min(marqueeStart_.x,io.MousePos.x),std::min(marqueeStart_.y,io.MousePos.y),std::abs(marqueeStart_.x-io.MousePos.x),std::abs(marqueeStart_.y-io.MousePos.y)};for(const auto& e:preview.layout.elements){if(e.locked||e.type=="group"||e.type=="instance")continue;auto r=toScreen(e.box);if(r.x>=area.x&&r.y>=area.y&&r.x+r.w<=area.x+area.w&&r.y+r.h<=area.y+area.h)selection_.push_back(pick(e));}marquee_=false;}}
 if(marquee_){draw->AddRectFilled(marqueeStart_,io.MousePos,IM_COL32(64,211,178,30));draw->AddRect(marqueeStart_,io.MousePos,IM_COL32(93,224,196,200));}
 if(hovered&&!io.WantTextInput&&!dragging_&&!selection_.empty()){float dx=0,dy=0,step=io.KeyShift?10:1;if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow))dx=-step;if(ImGui::IsKeyPressed(ImGuiKey_RightArrow))dx=step;if(ImGui::IsKeyPressed(ImGuiKey_UpArrow))dy=-step;if(ImGui::IsKeyPressed(ImGuiKey_DownArrow))dy=step;if(dx||dy){auto p=document_.project();for(const auto& s:selection_)if(auto* n=p.node(s.owner,s.id)){n->box.x+=dx;n->box.y+=dy;}apply(p,"nudge");}}
 if(guides_){for(const auto& s:selection_)if(auto* e=placed(s)){const auto r=toScreen(e->box);draw->AddRect({r.x,r.y},{r.x+r.w,r.y+r.h},IM_COL32(96,235,198,255),0,0,1.5);draw->AddRectFilled({r.x+r.w-4,r.y+r.h-4},{r.x+r.w+4,r.y+r.h+4},IM_COL32(96,235,198,255));
   if(selection_.size()==1){auto frame=toScreen(preview.layout.bounds);float mid=r.y+r.h*.5f;draw->AddLine({frame.x,mid},{r.x,mid},IM_COL32(115,209,229,170));std::string distance=std::to_string(int(std::round((r.x-frame.x)/sx/(e->box.w/e->reference.w))));draw->AddText({frame.x+4,mid-18},IM_COL32(158,220,232,255),distance.c_str());
    const float center=image.x+image.w*.5f;if(std::abs(r.x+r.w*.5f-center)<5)draw->AddLine({center,image.y},{center,image.y+image.h},IM_COL32(235,124,197,210));
   }
  }}
}
void Studio::inlineEditor(){if(inlineKey_.empty())return;ImGui::SetNextWindowPos({inlineRect_.x,inlineRect_.y});ImGui::SetNextWindowSize({std::max(280.f,inlineRect_.w),112});ImGui::Begin("Edit text",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoResize);if(inlineFocus_){ImGui::SetKeyboardFocusHere();inlineFocus_=false;}
 ImGui::SetNextItemWidth(-1);editText("##inline-text",inlineBuffer_);bool commit=ImGui::IsKeyPressed(ImGuiKey_Enter);if(ImGui::Button("Apply"))commit=true;ImGui::SameLine();if(ImGui::Button("Cancel")||ImGui::IsKeyPressed(ImGuiKey_Escape))inlineKey_.clear();
 if(commit&&!selection_.empty()){auto p=document_.project();auto n=editableNode(p);if(n.text.starts_with("{")&&n.text.ends_with("}")&&n.text.find('}')==n.text.size()-1)p.screen(screen_).variants[variant_].values[n.text.substr(1,n.text.size()-2)]=inlineBuffer_;else{n.text=inlineBuffer_;writeNode(p,n);}apply(p,"inline-text");inlineKey_.clear();}ImGui::End();}
void Studio::smoke(){
 // Exercise the visible Design/Play toggle through normal ImGui input before
 // checking the document operations used by inspectors and canvas gestures.
 auto& io=ImGui::GetIO();if(uiFrames_<2)return;
 if(smokeStep_==0){auto r=controls_.at("play");io.AddMousePosEvent(r.x+r.w*.5f,r.y+r.h*.5f);io.AddMouseButtonEvent(0,true);++smokeStep_;}
 else if(smokeStep_==1){io.AddMouseButtonEvent(0,false);++smokeStep_;}
 else if(smokeStep_==2){if(!play_)throw std::runtime_error("Smoke test: Play toggle did not respond");smokeChecks_.push_back("Design/Play responds to pointer input");play_=false;auto p=document_.project();p.styles["heading"].size+=1;apply(p,"smoke-style");selectScreen("shop-card");++smokeStep_;}
 else if(smokeStep_==3){if(screen_!="shop-card")throw std::runtime_error("Smoke test: screen navigation ignored its destination");smokeChecks_.push_back("Screen navigation retains the chosen screen");selectScreen("general-dialog");if(!document_.dirty()||!document_.undo()||!document_.redo())throw std::runtime_error("Smoke test: undo/redo failed");document_.undo();smokeChecks_.push_back("Style editing and undo/redo");comparison_=2;++smokeStep_;}
 else if(smokeStep_==4){for(const auto& p:previews_)if(!p->image||p->layout.elements.empty())throw std::runtime_error("Smoke test: comparison preview missing");smokeChecks_.push_back("Four independently rendered devices");comparison_=0;flow_=play_=true;level_=1;coins_=pearls_=0;++smokeStep_;}
 else if(smokeStep_==5){if(previews_[0]->layout.elements.empty())throw std::runtime_error("Smoke test: purchase flow missing");smokeChecks_.push_back("Actual tank purchase screen");flow_=play_=false;++smokeStep_;}
 else if(smokeStep_==6){const UiPlaced* title=nullptr;for(const auto& e:previews_[0]->layout.elements)if(e.owner=="@dialog-header"&&e.id=="label")title=&e;if(!title)throw std::runtime_error("Smoke test: editable title missing");const auto r=controls_.at("canvas-image");io.AddMousePosEvent(r.x+(title->box.x+title->box.w*.5f)*r.w/previews_[0]->logicalW,r.y+(title->box.y+title->box.h*.5f)*r.h/previews_[0]->logicalH);io.AddMouseButtonEvent(0,true);++smokeStep_;}
 else if(smokeStep_==7||smokeStep_==9){io.AddMouseButtonEvent(0,false);++smokeStep_;}
 else if(smokeStep_==8){io.AddMouseButtonEvent(0,true);++smokeStep_;}
 else if(smokeStep_==10){if(inlineKey_.empty())throw std::runtime_error("Smoke test: double click did not open text editing");io.AddKeyEvent(io.ConfigMacOSXBehaviors?ImGuiMod_Super:ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);++smokeStep_;}
 else if(smokeStep_==11){io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(io.ConfigMacOSXBehaviors?ImGuiMod_Super:ImGuiMod_Ctrl,false);io.AddInputCharactersUTF8("Edited on the canvas");++smokeStep_;}
 else if(smokeStep_==12){io.AddKeyEvent(ImGuiKey_Enter,true);++smokeStep_;}
 else if(smokeStep_==13){io.AddKeyEvent(ImGuiKey_Enter,false);++smokeStep_;}
 else if(smokeStep_==14){if(document_.project().screen(screen_).variants.at(variant_).values.at("title")!="Edited on the canvas")throw std::runtime_error("Smoke test: direct text edit did not apply");document_.undo();smokeChecks_.push_back("Double click text editing applies and can be undone");++smokeStep_;}
 else if(smokeStep_==15){smokeChecks_.push_back("Editor completed without a renderer error");++smokeStep_;if(!options_.frames)running_=false;}
}
}
