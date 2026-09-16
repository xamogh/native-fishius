#include "aquarium/studio.hpp"
#include <algorithm>
#include <fstream>
#include <numeric>

namespace aq::studio {
void Studio::draw(){const auto* viewport=ImGui::GetMainViewport();ImGui::SetNextWindowPos(viewport->WorkPos);ImGui::SetNextWindowSize(viewport->WorkSize);
 ImGui::Begin("Aquarium Studio",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);toolbar();
 const float height=ImGui::GetContentRegionAvail().y-33,left=238,right=300,gap=8;
 ImGui::BeginChild("Navigation",{left,height},ImGuiChildFlags_Borders);navigation();ImGui::EndChild();ImGui::SameLine(0,gap);
 const float center=std::max(400.f,ImGui::GetContentRegionAvail().x-right-gap);
 ImGui::BeginChild("Work area",{center,height});workspace();ImGui::EndChild();ImGui::SameLine(0,gap);
 ImGui::BeginChild("Properties",{right,height},ImGuiChildFlags_Borders);inspector();ImGui::EndChild();
 if(!document_.error().empty())ImGui::TextColored({1,.65f,.43f,1},"%s",document_.error().c_str());else ImGui::TextDisabled("%s",status_.c_str());
 ImGui::SameLine(std::max(750.f,ImGui::GetWindowWidth()-515));std::uint64_t renders=0;for(const auto& p:previews_)renders+=p->renders;
 ImGui::TextDisabled("%s  |  %llu renders  |  %s",fingerprint().c_str(),static_cast<unsigned long long>(renders),autosaved_?"Draft stored":"Draft autosave");ImGui::End();
}
void Studio::toolbar(){
 ImGui::TextColored(accent,"AQUARIUM STUDIO");ImGui::SameLine();ImGui::TextDisabled("/ %s",document_.project().name.c_str());
 ImGui::SameLine(std::max(350.f,ImGui::GetWindowWidth()-835));
 if(ImGui::Button(play_?"Design":"Play",{76,0})){play_=!play_;if(play_&&screen_=="tank-upgrade"&&component_.empty())flow_=true;for(auto& p:previews_)p->invalid=true;}remember("play");hint("Play uses a separate game session. Design enables selection and direct editing.");ImGui::SameLine();
 ImGui::BeginDisabled(!document_.canUndo());if(ImGui::Button("Undo"))document_.undo();remember("undo");ImGui::EndDisabled();ImGui::SameLine();
 ImGui::BeginDisabled(!document_.canRedo());if(ImGui::Button("Redo"))document_.redo();remember("redo");ImGui::EndDisabled();ImGui::SameLine();
 if(ImGui::Button("Export PNG")){try{auto path=options_.assets.parent_path()/"evidence/studio"/(screen_+"-export.png");previews_[0]->capture(path);status_="Exported "+path.filename().string();}catch(const std::exception& e){status_=e.what();}}ImGui::SameLine();
 if(ImGui::Button(previewProcess_?"Restart game":"Connect game")){try{startGame();}catch(const std::exception& e){status_=e.what();}}ImGui::SameLine();
 ImGui::PushStyleColor(ImGuiCol_Button,{.16f,.39f,.35f,1});if(ImGui::Button(document_.dirty()?"Save approved *":"Save approved"))save();remember("save");ImGui::PopStyleColor();hint("Publishes the draft to the local game design file. Connected preview uses its own draft file.");
 if(previewProcess_){ImGui::TextColored(connectedRevision_==sentFingerprint_?accent:ImVec4{1,.7f,.4f,1},"Game: %s",connectedRevision_.empty()?"waiting for first frame":connectedRevision_.c_str());ImGui::SameLine();ImGui::Checkbox("Send draft edits",&linkDraft_);ImGui::SameLine();if(ImGui::SmallButton("Disconnect"))stopGame();}
 if(ImGui::BeginTabBar("Open screens",ImGuiTabBarFlags_FittingPolicyScroll)){
  const auto desired=screen_;const bool requested=selectTab_;std::string activated;bool tabClicked=false;
  for(const auto& screen:document_.project().screens){const bool selected=desired==screen.id;
   if(ImGui::BeginTabItem(screen.name.c_str(),nullptr,selected&&requested?ImGuiTabItemFlags_SetSelected:0)){
    if(!requested||selected){activated=screen.id;tabClicked=ImGui::IsItemClicked();}ImGui::EndTabItem();
   }
  }
  ImGui::EndTabBar();if(!activated.empty()&&(screen_!=activated||(!component_.empty()&&tabClicked)))selectScreen(activated);selectTab_=false;
 }
}
void Studio::navigation(){
 if(ImGui::BeginTabBar("Navigator tabs")){
  if(ImGui::BeginTabItem("Screens")){leftTab_=0;
   label("SCREENS");for(const auto& s:document_.project().screens)if(ImGui::Selectable(s.name.c_str(),screen_==s.id&&component_.empty()))selectScreen(s.id);
   if(ImGui::Button("+ New screen",{-1,0})){auto p=document_.project();UiScreen s;s.id=p.newId("screen");s.name="New screen";s.root.id="root";s.root.name="Screen";s.root.box={0,0,888,732};s.variants["default"].values={{"title","New screen"},{"actionLabel","Continue"}};p.screens.push_back(s);apply(p);selectScreen(s.id);}
   if(ImGui::Button("Duplicate screen",{-1,0})){auto p=document_.project();auto s=p.screen(screen_);s.id=p.newId("screen");s.name+=" copy";p.screens.push_back(s);apply(p);selectScreen(s.id);}
   ImGui::Separator();label("REUSABLE COMPONENTS");editText("##component-filter",componentFilter_);
   for(const auto& [id,n]:document_.project().components){if(!componentFilter_.empty()&&n.name.find(componentFilter_)==std::string::npos&&id.find(componentFilter_)==std::string::npos)continue;
    if(ImGui::Selectable(n.name.c_str(),component_==id))selectComponent(id);
    if(ImGui::BeginDragDropSource()){ImGui::SetDragDropPayload("AQ_COMPONENT",id.c_str(),id.size()+1);ImGui::TextUnformatted(n.name.c_str());ImGui::EndDragDropSource();}
   }ImGui::TextWrapped("Drag a component onto the canvas. Changes to a component reach every instance.");ImGui::EndTabItem();
  }
  if(ImGui::BeginTabItem("Layers")){leftTab_=1;
   if(!component_.empty()){ImGui::TextColored(accent,"Editing %s",component_.c_str());if(ImGui::SmallButton("Back to screen"))selectScreen(screen_);}
   if(ImGui::Button("+ Layer"))ImGui::OpenPopup("Add layer");remember("add-layer");
   if(ImGui::BeginPopup("Add layer")){for(const auto* type:{"text","image","group","button"})if(ImGui::MenuItem(type))addNode(type);ImGui::EndPopup();}
   ImGui::SameLine();if(ImGui::Button("Group"))transformSelection("group");remember("group");hint("Select sibling layers with Shift, then group. Command/Ctrl + G.");
   if(ImGui::BeginChild("Layer tree",{0,std::max(120.f,ImGui::GetContentRegionAvail().y-152)})){
    for(const auto& e:previews_[0]->layout.elements){if(e.id=="root"&&e.owner.starts_with("@"))continue;ImGui::PushID(e.key.c_str());int depth=int(std::count(e.key.begin(),e.key.end(),'/'))+(e.owner.starts_with("@")?1:0);ImGui::Indent(float(depth)*10);
     bool chosen=std::any_of(selection_.begin(),selection_.end(),[&](const auto& s){return s.key==e.key;});
     std::string title=e.name.empty()?e.id:e.name;if(e.overridden)title+=" *";if(e.locked)title+=" (locked)";
     if(ImGui::Selectable(title.c_str(),chosen)){if(!ImGui::GetIO().KeyShift)selection_.clear();if(chosen&&ImGui::GetIO().KeyShift)std::erase_if(selection_,[&](const auto& s){return s.key==e.key;});else selection_.push_back(pick(e));override_=e.overridden;}
     if(e.type=="instance"&&ImGui::IsItemHovered()&&ImGui::IsMouseDoubleClicked(0))selectComponent(e.node.component);
     ImGui::Unindent(float(depth)*10);ImGui::PopID();
    }
   }ImGui::EndChild();
   label("ARRANGE");for(const auto* op:{"left","center","right","top","middle","bottom"}){if(ImGui::SmallButton(op))transformSelection(op);if(std::string(op)!="right"&&std::string(op)!="bottom")ImGui::SameLine();}
   if(ImGui::SmallButton("Distribute X"))transformSelection("distribute-x");ImGui::SameLine();if(ImGui::SmallButton("Distribute Y"))transformSelection("distribute-y");
   if(ImGui::SmallButton("Bring forward"))transformSelection("front");ImGui::SameLine();if(ImGui::SmallButton("Send back"))transformSelection("back");
   ImGui::EndTabItem();
  }
  if(ImGui::BeginTabItem("Assets")){leftTab_=2;ImGui::TextDisabled("%zu PNG assets",assets_.size());editText("##asset-filter",assetFilter_);ImGui::TextWrapped("Drag artwork onto a selected image or onto empty canvas.");
   ImGui::BeginChild("Asset list");int shown=0;
   for(const auto& asset:assets_){if(!assetFilter_.empty()&&asset.find(assetFilter_)==std::string::npos)continue;
    ImGui::PushID(asset.c_str());auto pos=ImGui::GetCursorScreenPos();const bool visible=ImGui::IsRectVisible({194,106});
    if(visible){auto*& texture=thumbnails_[asset];if(!texture){auto* source=IMG_Load((options_.assets/asset).string().c_str());if(source){float factor=std::min({1.f,380.f/source->w,144.f/source->h});auto* small=SDL_CreateSurface(std::max(1,int(source->w*factor)),std::max(1,int(source->h*factor)),SDL_PIXELFORMAT_RGBA32);if(small){SDL_BlitSurfaceScaled(source,nullptr,small,nullptr,SDL_SCALEMODE_LINEAR);texture=SDL_CreateTextureFromSurface(canvas_.renderer(),small);SDL_DestroySurface(small);}SDL_DestroySurface(source);}}if(texture){float w,h;SDL_GetTextureSize(texture,&w,&h);float scale=std::min(190.f/w,72.f/h);ImGui::GetWindowDrawList()->AddImage(ImTextureRef((ImTextureID)(intptr_t)texture),{pos.x+(194-w*scale)*.5f,pos.y},{pos.x+(194+w*scale)*.5f,pos.y+h*scale});}}
    ImGui::InvisibleButton("Asset",{194,76});if(ImGui::BeginDragDropSource()){ImGui::SetDragDropPayload("AQ_ASSET",asset.c_str(),asset.size()+1);ImGui::TextUnformatted(asset.c_str());ImGui::EndDragDropSource();}hint(asset.c_str());
    ImGui::TextDisabled("%.28s",std::filesystem::path(asset).stem().string().c_str());ImGui::PopID();++shown;
   }if(!shown)ImGui::TextDisabled("No matching assets");ImGui::EndChild();ImGui::EndTabItem();
  }ImGui::EndTabBar();
 }
}
const UiPlaced* Studio::placed(const Selection& s)const{for(const auto& e:previews_[0]->layout.elements)if(e.key==s.key)return &e;return nullptr;}
Selection Studio::pick(const UiPlaced& e)const{
 if(e.id=="root"&&e.owner.starts_with("@")&&component_.empty()){
  const auto chain=e.key.substr(0,e.key.find('|'));
  for(const auto& candidate:previews_[0]->layout.elements)if(candidate.type=="instance"&&candidate.node.component==e.owner.substr(1)){
   auto prefix=candidate.key.substr(0,candidate.key.find('|'));auto nested=prefix.empty()?candidate.id:prefix+"/"+candidate.id;
   if(nested==chain)return {candidate.key,candidate.owner,candidate.id};
  }
 }
 return {e.key,e.owner,e.id};
}
std::string Studio::owner()const{return !selection_.empty()?selection_.front().owner:component_.empty()?screen_:"@"+component_;}
UiNode* Studio::ownerRoot(UiProject& p)const{auto name=owner();if(name.starts_with("@")){auto it=p.components.find(name.substr(1));return it==p.components.end()?nullptr:&it->second;}return &p.screen(name).root;}
UiNode Studio::editableNode(const UiProject& p)const{if(selection_.empty())return {};auto* n=p.node(selection_[0].owner,selection_[0].id);if(!n)return {};auto copy=*n;if(override_&&component_.empty()){const auto& s=p.screen(screen_);if(auto v=s.variants.find(variant_);v!=s.variants.end())if(auto patch=v->second.overrides.find(selection_[0].key);patch!=v->second.overrides.end()){UiJson j=copy;j.merge_patch(patch->second);copy=j.get<UiNode>();}}return copy;}
void Studio::writeNode(UiProject& p,const UiNode& n)const{if(selection_.empty())return;auto* base=p.node(selection_[0].owner,selection_[0].id);if(!base)return;
 if(override_&&component_.empty()){UiJson original=*base,next=n,patch=UiJson::object();for(auto it=next.begin();it!=next.end();++it)if(it.key()!="id"&&it.key()!="children"&&it.key()!="component"&&it.value()!=original[it.key()])patch[it.key()]=it.value();auto& overrides=p.screen(screen_).variants[variant_].overrides;if(patch.empty())overrides.erase(selection_[0].key);else overrides[selection_[0].key]=patch;
 }else *base=n;
}
void Studio::editNode(const UiNode& n,std::string group){auto p=document_.project();writeNode(p,n);apply(std::move(p),std::move(group));}
void Studio::inspector(){
 if(selection_.empty()){
  ImGui::TextUnformatted(component_.empty()?document_.project().screen(screen_).name.c_str():component_.c_str());ImGui::TextWrapped("Select a layer to change its layout, style, content or action.");
  auto p=document_.project();auto& s=p.screen(screen_);bool changed=false;label("SCREEN");ImGui::SetNextItemWidth(-1);changed|=editText("##screen-name",s.name);changed|=ImGui::SliderFloat("Max width",&s.maxWidth,100,1600,"%.0f pt");changed|=ImGui::SliderFloat("Safe coverage",&s.safeFraction,.3f,1,"%.2f");if(changed)apply(p,"screen-settings");
  label("SHARED STYLES");for(const auto& [id,style]:p.styles)if(ImGui::TreeNode(id.c_str())){auto draft=document_.project();auto& v=draft.styles[id];bool edited=choice("Font",v.font,{"baloo","nunito","lilita"});edited|=ImGui::SliderFloat("Size",&v.size,8,160);edited|=editColor("Color",v.color);edited|=ImGui::SliderFloat("Spacing",&v.gap,0,100);edited|=ImGui::SliderFloat("Padding",&v.padding,0,100);if(edited)apply(draft,"style-"+id);ImGui::TreePop();}
  return;
 }
 auto p=document_.project();auto n=editableNode(p);if(n.id.empty()){selection_.clear();return;}
 ImGui::TextUnformatted(n.name.c_str());ImGui::TextDisabled("%s%s",n.type.c_str(),selection_.size()>1?" / multiple selection":"");
 if(component_.empty()){
  if(ImGui::Checkbox("Override this variant",&override_)){}
  hint("Off edits the shared layer. On changes only this instance in the selected variant.");
  if(override_&&ImGui::SmallButton("Reset variant overrides")){p.screen(screen_).variants[variant_].overrides.erase(selection_[0].key);apply(p);override_=false;}
 }
 if(selection_[0].owner.starts_with("@"))ImGui::TextWrapped("Shared component: %s",selection_[0].owner.substr(1).c_str());
 ImGui::Separator();bool changed=false;ImGui::PushItemWidth(-1);
 if(ImGui::BeginTabBar("Inspector tabs")){
  if(ImGui::BeginTabItem("Layout")){inspectLayout(p,n,changed);ImGui::EndTabItem();}
  if(ImGui::BeginTabItem("Style")){inspectStyle(p,n,changed);ImGui::EndTabItem();}
  if(ImGui::BeginTabItem("Content")){inspectContent(p,n,changed);ImGui::EndTabItem();}
  if(ImGui::BeginTabItem("Action")){inspectBehavior(p,n,changed);ImGui::EndTabItem();}
  ImGui::EndTabBar();
 }ImGui::PopItemWidth();if(changed){writeNode(p,n);apply(p,"inspector-"+selection_[0].key);}
}
void Studio::inspectLayout(UiProject& p,UiNode& n,bool& changed){label("POSITION");float xy[]{n.box.x,n.box.y};changed|=ImGui::DragFloat2("##position",xy,.5f,-4000,4000,"%.1f");n.box.x=xy[0];n.box.y=xy[1];remember("position");label("WIDTH / HEIGHT");float wh[]{n.box.w,n.box.h};changed|=ImGui::DragFloat2("##size",wh,.5f,1,4000,"%.1f");n.box.w=wh[0];n.box.h=wh[1];
 label("ANCHORS");changed|=choice("##horizontal",n.layout.horizontal,{"left","center","right","stretch","center-alone"});changed|=choice("##vertical",n.layout.vertical,{"top","center","bottom"});
 label("LAYOUT");changed|=choice("##layout",n.layout.mode,{"absolute","row","column"});changed|=choice("##width-rule",n.layout.width,{"fixed","fill","hug"});changed|=choice("##height-rule",n.layout.height,{"fixed","hug"});changed|=ImGui::Checkbox("Wrap text",&n.layout.wrap);
 label("GAP / PADDING");changed|=ImGui::DragFloat("##gap",&n.layout.gap,.5f,-1,300,"Gap %.1f");changed|=ImGui::DragFloat("##padding",&n.layout.padding,.5f,-1,300,"Padding %.1f");
 ImGui::TextDisabled("-1 inherits shared style spacing");
 label("BELOW LAYER");
 UiNode* parent=nullptr;auto* root=ownerRoot(p);std::function<void(UiNode&)> findParent=[&](UiNode& current){for(auto& child:current.children){if(child.id==n.id)parent=&current;findParent(child);}};if(root)findParent(*root);
 std::string targetName="None";if(parent)for(const auto& sibling:parent->children)if(sibling.id==n.layout.after)targetName=sibling.name;
 if(ImGui::BeginCombo("##after",targetName.c_str())){if(ImGui::Selectable("None",n.layout.after.empty())){n.layout.after.clear();changed=true;}if(parent)for(const auto& sibling:parent->children)if(sibling.id!=n.id&&ImGui::Selectable(sibling.name.c_str(),n.layout.after==sibling.id)){n.layout.after=sibling.id;changed=true;}ImGui::EndCombo();}
 hint("Gap is the distance below this layer. A hidden target uses the original position.");
 ImGui::TextDisabled("ID: %s",n.id.c_str());changed|=ImGui::Checkbox("Lock layer",&n.locked);changed|=ImGui::Checkbox("Hide layer",&n.hidden);
 if(n.type=="group"){if(ImGui::Button("Ungroup",{-1,0}))transformSelection("ungroup");}
 if(n.type=="instance"){if(ImGui::Button("Edit component",{-1,0}))selectComponent(n.component);}
}
void Studio::inspectStyle(UiProject& p,UiNode& n,bool& changed){label("STYLE PRESET");if(ImGui::BeginCombo("##preset",n.style.c_str())){for(const auto& [id,_]:p.styles)if(ImGui::Selectable(id.c_str(),n.style==id)){n.style=id;changed=true;}ImGui::EndCombo();}
 static bool shared=false;ImGui::Checkbox("Edit shared preset",&shared);auto s=uiStyle(p,n);if(shared)s=p.styles[n.style];bool edited=false;
 label("FONT");edited|=choice("##font",s.font,{"baloo","nunito","lilita"});edited|=choice("Alignment",s.align,{"left","center","right"});label("SIZE / WIDTH");edited|=ImGui::SliderFloat("##font-size",&s.size,8,160,"%.0f");edited|=ImGui::SliderFloat("##stretch",&s.stretch,.5f,1.6f,"%.2f");edited|=ImGui::SliderFloat("Line height",&s.lineHeight,.8f,2,"%.2f");edited|=editColor("Text",s.color);edited|=editColor("Attention",s.alert);
 if(n.type=="richtext"||n.component=="currency-text"){edited|=ImGui::SliderFloat("Amount size",&s.accentSize,8,180);edited|=editColor("Amount",s.accent);}
 if(ImGui::CollapsingHeader("Outline and shadow")){edited|=ImGui::SliderFloat("Stroke",&s.outlineWidth,0,8);edited|=editColor("Outline",s.outline);edited|=editColor("Shadow",s.shadow);edited|=ImGui::DragFloat("Shadow X",&s.shadowX,.2f,-50,50);edited|=ImGui::DragFloat("Shadow Y",&s.shadowY,.2f,-50,50);}
 if(n.type=="button"){edited|=ImGui::SliderFloat("Pressed scale",&s.pressedScale,.7f,1);edited|=ImGui::SliderFloat("Disabled alpha",&s.disabledAlpha,.1f,1);}
 if(edited){if(shared)p.styles[n.style]=s;else{UiJson base=p.styles[n.style],value=s,patch=UiJson::object();for(auto it=value.begin();it!=value.end();++it)if(it.value()!=base[it.key()])patch[it.key()]=it.value();n.styleOverrides=patch;}changed=true;}
 if(!n.styleOverrides.empty()&&ImGui::Button("Inherit all preset values",{-1,0})){n.styleOverrides=UiJson::object();changed=true;}
 if(ImGui::Button("Save as new preset",{-1,0})){auto id=p.newId("style");p.styles[id]=s;n.style=id;n.styleOverrides=UiJson::object();changed=true;}
 label("ARTWORK");changed|=ImGui::SliderFloat("Rotation",&n.rotation,-180,180);changed|=ImGui::SliderFloat("9-slice border",&n.slice,0,200);
}
void Studio::inspectContent(UiProject& p,UiNode& n,bool& changed){label("LAYER NAME");changed|=editText("##name",n.name);
 auto& variant=p.screen(screen_).variants[variant_];
 if(!n.text.empty()||n.type=="text"||n.type=="richtext"||n.type=="button"){
  const bool bound=n.text.starts_with("{")&&n.text.ends_with("}")&&n.text.find('}')==n.text.size()-1;
  if(bound){auto key=n.text.substr(1,n.text.size()-2);label(("CONTENT / "+key).c_str());changed|=editText("##bound-content",variant.values[key],true);if(ImGui::Button("Use in all variants",{-1,0})){auto value=variant.values[key];for(auto& [_,v]:p.screen(screen_).variants)v.values[key]=value;changed=true;}}
  else{label("TEXT");changed|=editText("##text",n.text,true);}
  if(ImGui::CollapsingHeader("Binding template")){changed|=editText("##template",n.text);ImGui::TextWrapped("Templates accept {amount}, {title}, {detail}, {coins}, {pearls}, {capacity} and your own values.");}
 }
 if(n.type=="image"||n.type=="button"||!n.asset.empty()){
  label("IMAGE");std::string* asset=&n.asset;if(n.asset.starts_with("{")&&n.asset.ends_with("}"))asset=&variant.values[n.asset.substr(1,n.asset.size()-2)];changed|=editText("##asset",*asset);
  if(ImGui::BeginCombo("Choose asset",std::filesystem::path(*asset).filename().string().c_str())){for(const auto& name:assets_)if(ImGui::Selectable(name.c_str(),name==*asset)){*asset=name;changed=true;}ImGui::EndCombo();}
 }
 label("VARIANT VALUES");for(auto& [key,value]:variant.values){if(key=="art"||key=="illustration"||key=="cardArt")continue;ImGui::PushID(key.c_str());ImGui::TextDisabled("%s",key.c_str());changed|=editText("##value",value);ImGui::PopID();}
}
void Studio::inspectBehavior(UiProject&,UiNode& n,bool& changed){label("ACTION");changed|=choice("##action",n.action,{"","primary","close","coins","pearls","buyCoins","buyPearls","coinBalance","pearlBalance"});ImGui::TextWrapped("The game owns purchases and navigation. This field connects the layer to the screen's action.");label("VISIBLE WHEN VALUE IS SET");changed|=editText("##visible",n.visibleWhen);label("ATTENTION COLOR WHEN VALUE IS SET");changed|=editText("##alert",n.alertWhen);label("ENABLED WHEN VALUE IS SET");changed|=editText("##enabled",n.enabledWhen);ImGui::TextWrapped("Empty uses the normal state. A named value of 0 or an empty value switches the condition off.");}
void Studio::bottomPanel(){
 if(ImGui::BeginTabBar("Bottom tools")){
  if(ImGui::BeginTabItem("Scenarios")){bottomTab_=0;
   ImGui::SetNextItemWidth(190);ImGui::Combo("##scenario",&scenario_,"Normal\0Large amount\0Long translation\0Level requirement\0Pressed\0Disabled\0");remember("scenario");ImGui::SameLine();ImGui::SetNextItemWidth(105);ImGui::InputScalar("Amount",ImGuiDataType_S64,&amount_);amount_=std::clamp<Amount>(amount_,1,999999999);
   if(ImGui::Checkbox("Test tank purchase flow",&flow_)){for(auto& p:previews_)p->invalid=true;play_=flow_;}remember("flow");hint("Uses the actual tank menu and purchase callbacks in an isolated session.");
   if(flow_){ImGui::SetNextItemWidth(80);ImGui::SliderInt("Level",&level_,1,40);ImGui::SameLine();ImGui::SetNextItemWidth(100);ImGui::InputScalar("Coins",ImGuiDataType_S64,&coins_);ImGui::SameLine();ImGui::SetNextItemWidth(80);ImGui::InputScalar("Pearls",ImGuiDataType_S64,&pearls_);
    ImGui::SetNextItemWidth(80);ImGui::SliderInt("Tank",&tank_,1,5);ImGui::SameLine();ImGui::Checkbox("Owned",&owned_);ImGui::SameLine();ImGui::SetNextItemWidth(80);ImGui::SliderInt("Upgrades",&upgrades_,0,2);ImGui::SameLine();if(ImGui::Button("Reset test"))for(auto& p:previews_)p->initialized=false;
   }else ImGui::TextWrapped("Change the state without editing the design. Play lets you use the real controls.");ImGui::EndTabItem();
  }
  if(ImGui::BeginTabItem("Motion")){bottomTab_=1;auto p=document_.project();auto& m=p.dialogMotion;bool changed=false;
   if(ImGui::Button("Replay")){elapsed_=0;replaying_=true;replayAt_=SDL_GetTicks();}ImGui::SameLine();ImGui::SetNextItemWidth(280);if(ImGui::SliderFloat("Time",&elapsed_,0,m.duration,"%.2f s"))replaying_=false;
   ImGui::SetNextItemWidth(115);changed|=ImGui::SliderFloat("Duration",&m.duration,.1f,2,"%.2f s");ImGui::SameLine();ImGui::SetNextItemWidth(115);changed|=ImGui::SliderFloat("Damping",&m.damping,4,35,"%.1f");
   ImGui::SetNextItemWidth(115);changed|=ImGui::SliderFloat("Frequency",&m.frequency,1,40,"%.1f");ImGui::SameLine();ImGui::SetNextItemWidth(115);changed|=ImGui::SliderFloat("Start scale",&m.scale,.5f,1,"%.2f");ImGui::SameLine();ImGui::SetNextItemWidth(90);changed|=ImGui::SliderFloat("Rise",&m.rise,-100,150,"%.0f");if(changed)apply(p,"motion");ImGui::EndTabItem();
  }
  std::string problems="Problems ("+std::to_string(previews_[0]->layout.problems.size())+")";
  if(ImGui::BeginTabItem(problems.c_str())){bottomTab_=2;ImGui::BeginChild("Problems list");if(previews_[0]->layout.problems.empty())ImGui::TextColored(accent,"No layout problems in this preview.");for(const auto& item:previews_[0]->layout.problems){ImGui::PushID(item.key.c_str());if(ImGui::Selectable((std::string(item.error?"! ":"  ")+item.message).c_str()))for(const auto& e:previews_[0]->layout.elements)if(e.key==item.key){selection_={pick(e)};override_=e.overridden;}ImGui::PopID();}ImGui::EndChild();ImGui::EndTabItem();}
  if(ImGui::BeginTabItem("Versions")){bottomTab_=3;versionsPanel();ImGui::EndTabItem();}
  ImGui::EndTabBar();
 }
}
void Studio::versionsPanel(){ImGui::SetNextItemWidth(220);editText("##version-name",versionName_);ImGui::SameLine();if(ImGui::Button("Save named version"))snapshot();ImGui::SameLine();if(ImGui::Button("Compare approved"))comparison_=4;
 if(ImGui::BeginCombo("Restore", "Choose a named version")){for(const auto& path:versions_)if(ImGui::Selectable(path.stem().string().c_str())){try{std::ifstream in(path);apply(UiProject::fromJson(UiJson::parse(in)),"restore-version");status_="Version restored into draft. Undo remains available.";}catch(const std::exception& e){status_=e.what();}}ImGui::EndCombo();}
 ImGui::TextWrapped("Drafts and undo history recover after restart. Save approved updates the game. Named versions keep milestones.");
 if(document_.conflict()){
  if(ImGui::Button("Keep draft over changed file")){auto draft=document_.project();snapshot();document_.reload(true);apply(draft);save();}ImGui::SameLine();if(ImGui::Button("Load changed file")){snapshot();document_.reload(true);}
 }
}
void Studio::addNode(std::string type,std::string component,std::string asset){auto p=document_.project();const auto targetOwner=component_.empty()?screen_:"@"+component_;auto* root=component_.empty()?&p.screen(screen_).root:&p.components.at(component_);UiNode n;n.id=p.newId(type);n.name=component.empty()?type:component;n.type=type;n.box={120,160,240,90};
 if(type=="text"){n.text="New text";n.style="body";}if(type=="image"){n.asset=asset.empty()?"general-dialog/coins.png":asset;n.box.h=160;}if(type=="button"){n.asset="general-dialog/button.png";n.text="Continue";n.style="primary";n.action="primary";n.box.h=70;}
 if(!component.empty()){n.type="instance";n.component=component;const auto b=p.components.at(component).box;n.box.w=b.w*.6f;n.box.h=b.h*.6f;}
 if(root->type=="instance"){
  auto original=*root;root->type="group";root->component.clear();root->children.clear();root->id=p.newId("root");original.box.x=original.box.y=0;root->children.push_back(original);
 }
 root->children.push_back(n);apply(p);selection_={{(component_.empty()?"":"component")+std::string("|")+targetOwner+"|"+n.id,targetOwner,n.id}};override_=false;leftTab_=1;
}
void Studio::transformSelection(const std::string& op){if(selection_.empty())return;auto p=document_.project();auto selectedOwner=selection_[0].owner;std::set<std::string> ids;for(auto& s:selection_){if(s.owner!=selectedOwner){status_="Choose sibling layers from one component";return;}ids.insert(s.id);}
 bool changed=false;if(op=="group")changed=uiGroup(p,selectedOwner,ids);else if(op=="ungroup")changed=uiUngroup(p,selectedOwner,selection_[0].id);else if(op=="delete")changed=uiDelete(p,selectedOwner,ids);else if(op=="duplicate")changed=uiDuplicate(p,selectedOwner,ids);
 else if(op=="front"||op=="back"){
  auto* root=ownerRoot(p);std::function<void(UiNode&)> reorder=[&](UiNode& n){std::stable_partition(n.children.begin(),n.children.end(),[&](const auto& c){return op=="front"?!ids.contains(c.id):ids.contains(c.id);});for(auto& c:n.children)reorder(c);};if(root){reorder(*root);changed=true;}
 }else{std::vector<UiNode*> nodes;for(auto& s:selection_)if(auto* n=p.node(s.owner,s.id))nodes.push_back(n);if(nodes.size()<2)return;float left=4000,top=4000,right=-4000,bottom=-4000;for(auto* n:nodes){left=std::min(left,n->box.x);top=std::min(top,n->box.y);right=std::max(right,n->box.x+n->box.w);bottom=std::max(bottom,n->box.y+n->box.h);}
  if(op=="distribute-x"||op=="distribute-y"){bool x=op=="distribute-x";std::sort(nodes.begin(),nodes.end(),[&](auto* a,auto* b){return x?a->box.x<b->box.x:a->box.y<b->box.y;});float total=0;for(auto* n:nodes)total+=x?n->box.w:n->box.h;float gap=((x?right-left:bottom-top)-total)/float(nodes.size()-1),at=x?left:top;for(auto* n:nodes){if(x)n->box.x=at;else n->box.y=at;at+=(x?n->box.w:n->box.h)+gap;}}
  else for(auto* n:nodes){if(op=="left")n->box.x=left;else if(op=="center")n->box.x=(left+right-n->box.w)*.5f;else if(op=="right")n->box.x=right-n->box.w;else if(op=="top")n->box.y=top;else if(op=="middle")n->box.y=(top+bottom-n->box.h)*.5f;else if(op=="bottom")n->box.y=bottom-n->box.h;}changed=true;
 }if(changed){apply(p,op);if(op=="group"||op=="ungroup"||op=="delete"||op=="duplicate")selection_.clear();}else status_="This operation needs sibling layers inside a group";
}
void Studio::shortcuts(){const auto& io=ImGui::GetIO();const bool mod=io.KeyCtrl||io.KeySuper;if(mod&&ImGui::IsKeyPressed(ImGuiKey_S))save();if(io.WantTextInput)return;
 if(mod&&ImGui::IsKeyPressed(ImGuiKey_Z)){if(io.KeyShift)document_.redo();else document_.undo();}if(mod&&ImGui::IsKeyPressed(ImGuiKey_Y))document_.redo();if(mod&&ImGui::IsKeyPressed(ImGuiKey_G))transformSelection(io.KeyShift?"ungroup":"group");if(mod&&ImGui::IsKeyPressed(ImGuiKey_D))transformSelection("duplicate");if(!play_&&(ImGui::IsKeyPressed(ImGuiKey_Delete)||ImGui::IsKeyPressed(ImGuiKey_Backspace)))transformSelection("delete");
 if(ImGui::IsKeyPressed(ImGuiKey_Escape)){selection_.clear();inlineKey_.clear();}if(mod&&ImGui::IsKeyPressed(ImGuiKey_0)){zoom_=1;pan_={};}
}
}
