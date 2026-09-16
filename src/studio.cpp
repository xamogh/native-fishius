#include "aquarium/studio.hpp"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace aq::studio {
Options options(int argc,char** argv){
 Options o;for(int i=1;i<argc;++i){std::string flag=argv[i];auto value=[&]{if(i+1==argc)throw std::runtime_error("Missing value for "+flag);return std::string(argv[++i]);};
  if(flag=="--assets")o.assets=value();else if(flag=="--project"||flag=="--design")o.project=value();else if(flag=="--capture")o.capture=value();else if(flag=="--export")o.exportPath=value();else if(flag=="--report")o.report=value();
  else if(flag=="--frames"){o.frames=std::stoi(value());o.recovery=false;}else if(flag=="--screen")o.screen=value();else if(flag=="--compare")o.comparison=std::stoi(value());else if(flag=="--scenario")o.scenario=std::stoi(value());
  else if(flag=="--pearls")o.variant=1;else if(flag=="--software")o.software=true;else if(flag=="--init-design")o.init=true;else if(flag=="--no-recovery")o.recovery=false;else if(flag=="--test-flow")o.flow=true;else if(flag=="--smoke"){o.smoke=true;o.recovery=false;}else throw std::runtime_error("Unknown option: "+flag);
 }o.assets=std::filesystem::absolute(o.assets);if(o.project.empty())o.project=o.assets/"ui/studio.json";return o;
}
bool editText(const char* name,std::string& value,bool multiline){std::array<char,1025> buffer{};std::copy_n(value.data(),std::min(value.size(),buffer.size()-1),buffer.data());bool edited=multiline?ImGui::InputTextMultiline(name,buffer.data(),buffer.size(),{-1,75}):ImGui::InputText(name,buffer.data(),buffer.size());if(edited)value=buffer.data();return edited;}
bool editColor(const char* name,DesignColor& color){float c[4];for(int i=0;i<4;++i)c[i]=color[i]/255.f;if(!ImGui::ColorEdit4(name,c))return false;for(int i=0;i<4;++i)color[i]=int(std::round(std::clamp(c[i],0.f,1.f)*255));return true;}
bool choice(const char* name,std::string& value,std::initializer_list<const char*> choices){bool changed=false;if(ImGui::BeginCombo(name,value.c_str())){for(auto* item:choices)if(ImGui::Selectable(item,value==item)){value=item;changed=true;}ImGui::EndCombo();}return changed;}
void label(const char* name){ImGui::Spacing();ImGui::TextDisabled("%s",name);}
void hint(const char* text){if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))ImGui::SetTooltip("%s",text);}
Content loadContent(const std::filesystem::path& assets){std::ifstream in(assets/"content.json");return Content::fromJson(Json::parse(in));}
void style(){ImGui::StyleColorsDark();auto& s=ImGui::GetStyle();s.WindowRounding=0;s.ChildRounding=5;s.FrameRounding=4;s.PopupRounding=6;s.GrabRounding=3;s.WindowPadding={14,12};s.FramePadding={9,6};s.ItemSpacing={8,8};s.ScrollbarSize=10;s.WindowBorderSize=0;s.ChildBorderSize=1;
 s.Colors[ImGuiCol_WindowBg]={.062f,.077f,.092f,1};s.Colors[ImGuiCol_ChildBg]={.077f,.094f,.110f,1};s.Colors[ImGuiCol_PopupBg]={.10f,.12f,.14f,1};s.Colors[ImGuiCol_Border]={.20f,.24f,.27f,.7f};s.Colors[ImGuiCol_Text]={.91f,.94f,.95f,1};s.Colors[ImGuiCol_TextDisabled]=muted;
 s.Colors[ImGuiCol_FrameBg]={.125f,.15f,.175f,1};s.Colors[ImGuiCol_FrameBgHovered]={.18f,.23f,.25f,1};s.Colors[ImGuiCol_FrameBgActive]={.17f,.28f,.29f,1};s.Colors[ImGuiCol_Button]={.14f,.19f,.21f,1};s.Colors[ImGuiCol_ButtonHovered]={.19f,.32f,.33f,1};s.Colors[ImGuiCol_ButtonActive]={.18f,.38f,.36f,1};s.Colors[ImGuiCol_Header]={.15f,.29f,.29f,1};s.Colors[ImGuiCol_HeaderHovered]={.17f,.34f,.33f,1};s.Colors[ImGuiCol_HeaderActive]={.20f,.39f,.37f,1};s.Colors[ImGuiCol_CheckMark]=s.Colors[ImGuiCol_SliderGrab]=accent;
 s.Colors[ImGuiCol_Tab]={.11f,.14f,.16f,1};s.Colors[ImGuiCol_TabHovered]={.17f,.30f,.29f,1};s.Colors[ImGuiCol_TabSelected]={.15f,.25f,.25f,1};s.Colors[ImGuiCol_TabSelectedOverline]=accent;
}
Studio::Studio(Options options):options_(std::move(options)),document_(options_.project,options_.assets,options_.recovery?dataDirectory()/"draft.json":std::filesystem::path{}),canvas_(options_.assets,1600,1000,options_.software),content_(loadContent(options_.assets)),screen_(options_.screen),variant_(options_.variant?"pearls":"coins"),comparison_(std::clamp(options_.comparison,0,4)),scenario_(std::clamp(options_.scenario,0,5)),flow_(options_.flow){
 SDL_SetWindowTitle(canvas_.window(),"Aquarium Studio");SDL_SetWindowMinimumSize(canvas_.window(),1180,780);density_=std::clamp(int(std::round(SDL_GetWindowPixelDensity(canvas_.window()))),1,2);
 IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;io.Fonts->AddFontFromFileTTF((options_.assets/"fonts/Nunito-SemiBold.ttf").string().c_str(),16);style();
 ImGui_ImplSDL3_InitForSDLRenderer(canvas_.window(),canvas_.renderer());ImGui_ImplSDLRenderer3_Init(canvas_.renderer());
 for(auto& preview:previews_)preview=std::make_unique<Preview>(canvas_,content_);
 for(const auto& entry:std::filesystem::recursive_directory_iterator(options_.assets))if(entry.is_regular_file()&&entry.path().extension()==".png")assets_.push_back(std::filesystem::relative(entry.path(),options_.assets).generic_string());std::sort(assets_.begin(),assets_.end());
 selectScreen(screen_);loadVersions();if(document_.recovered())status_="Recovered your draft and undo history";if(options_.variant){variant_="pearls";amount_=12;}
 livePath_=dataDirectory()/"live.json";
}
Studio::~Studio(){document_.autosave();for(auto& p:previews_)p.reset();for(auto& [_,t]:thumbnails_)SDL_DestroyTexture(t);if(previewProcess_)SDL_DestroyProcess(previewProcess_);ImGui_ImplSDLRenderer3_Shutdown();ImGui_ImplSDL3_Shutdown();ImGui::DestroyContext();}
int Studio::run(){
 while(running_){SDL_Event event{};bool input=false;
  if(uiFrames_>2&&!replaying_&&!play_&&!options_.frames&&!options_.smoke){if(SDL_WaitEventTimeout(&event,100)){ImGui_ImplSDL3_ProcessEvent(&event);input=true;if(event.type==SDL_EVENT_QUIT||event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)running_=false;}}
  while(SDL_PollEvent(&event)){input=true;ImGui_ImplSDL3_ProcessEvent(&event);if(event.type==SDL_EVENT_QUIT||event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)running_=false;}
  if(!running_){if(!document_.autosave()){running_=true;status_="Could not save draft: "+document_.error();}else break;}
  if(SDL_GetTicks()>=nextPoll_){document_.poll();if(SDL_GetTicks()>lastEdit_+400)autosaved_=document_.autosave();bridge();nextPoll_=SDL_GetTicks()+400;}
  if(!input&&!play_&&!replaying_)++idleFrames_;
  if(replaying_){elapsed_=float(SDL_GetTicks()-replayAt_)/1000;if(elapsed_>=document_.project().dialogMotion.duration){elapsed_=document_.project().dialogMotion.duration;replaying_=false;}}
  ImGui_ImplSDLRenderer3_NewFrame();ImGui_ImplSDL3_NewFrame();ImGui::NewFrame();shortcuts();draw();inlineEditor();
  if(!ImGui::IsAnyItemActive()&&!dragging_)document_.endEdit();if(options_.smoke)smoke();
  ImGui::Render();auto* renderer=canvas_.renderer();SDL_SetRenderTarget(renderer,nullptr);auto density=ImGui::GetIO().DisplayFramebufferScale;SDL_SetRenderScale(renderer,density.x,density.y);SDL_SetRenderDrawColor(renderer,16,20,24,255);SDL_RenderClear(renderer);ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(),renderer);
  ++uiFrames_;bool done=options_.frames&&int(uiFrames_)>=options_.frames;
  if(done||(!running_&&options_.smoke)){
   if(!options_.capture.empty()){std::filesystem::create_directories(options_.capture.parent_path());auto* surface=SDL_RenderReadPixels(renderer,nullptr);if(!surface||!IMG_SavePNG(surface,options_.capture.string().c_str()))throw std::runtime_error(SDL_GetError());SDL_DestroySurface(surface);}
   if(!options_.exportPath.empty())previews_[0]->capture(options_.exportPath);
   if(!options_.report.empty()){UiJson report{{"frames",uiFrames_},{"idleFrames",idleFrames_},{"revision",fingerprint()},{"screen",screen_},{"smokeChecks",smokeChecks_},{"problems",previews_[0]->layout.problems.size()}};for(const auto& preview:previews_)report["previewRenders"].push_back(preview->renders);writeUiJson(options_.report,report);}
   running_=false;
  }SDL_RenderPresent(renderer);
 }return 0;
}
std::string Studio::fingerprint(){if(fingerprintVersion_!=document_.version()){cachedFingerprint_=uiFingerprint(document_.project());fingerprintVersion_=document_.version();}return cachedFingerprint_;}
void Studio::apply(UiProject p,std::string group){uiPruneOverrides(p);if(document_.apply(std::move(p),std::move(group))){lastEdit_=SDL_GetTicks();autosaved_=false;status_="Draft updated";}else if(!document_.error().empty())status_=document_.error();}
void Studio::save(){if(document_.save()){status_="Approved design saved. Open games reload it automatically.";autosaved_=true;}else status_=document_.error();}
void Studio::selectScreen(std::string screen){screen_=std::move(screen);selectTab_=true;component_.clear();selection_.clear();const auto& s=document_.project().screen(screen_);if(!s.variants.contains(variant_))variant_=s.variants.empty()?"default":s.variants.begin()->first;pan_={};zoom_=1;inlineKey_.clear();}
void Studio::selectComponent(std::string id){component_=std::move(id);selection_.clear();pan_={};zoom_=1;inlineKey_.clear();}
Stage Studio::stage(int index)const{Stage s;s.screen=component_.empty()?screen_:"__component";s.variant=variant_;s.width=device_==4?customW_:devices[device_].w;s.height=device_==4?customH_:devices[device_].h;s.density=density_;s.safe=safe_;s.scenario=scenario_;s.amount=amount_;s.elapsed=elapsed_;s.flow=flow_&&component_.empty();s.level=level_;s.coins=coins_;s.pearls=pearls_;s.tank=tank_;s.owned=owned_;s.upgrades=upgrades_;
 if(index&&comparison_==1)s.variant=variant_=="pearls"?"coins":"pearls";
 if(comparison_==2){s.width=devices[index].w;s.height=devices[index].h;}
 if(index&&comparison_==3)s.scenario=index==1?1:index==2?2:4;
 return s;
}
UiProject Studio::previewProject(const UiProject& original)const{auto p=original;if(!component_.empty()&&p.components.contains(component_)){UiScreen s;s.id="__component";s.name=component_;s.root.id="component";s.root.name=component_;s.root.type="instance";s.root.component=component_;s.root.box=p.components.at(component_).box;s.root.box.x=s.root.box.y=0;s.maxWidth=620;s.variants=p.screen(screen_).variants;p.screens.push_back(std::move(s));}return p;}
void Studio::remember(std::string key){const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();controls_[std::move(key)]={a.x,a.y,b.x-a.x,b.y-a.y};}
void Studio::loadVersions(){versions_.clear();auto dir=dataDirectory()/"versions";if(!std::filesystem::exists(dir))return;for(auto& entry:std::filesystem::directory_iterator(dir))if(entry.path().extension()==".json")versions_.push_back(entry.path());std::sort(versions_.rbegin(),versions_.rend());}
void Studio::snapshot(){std::string safe;for(char c:versionName_)if(std::isalnum(static_cast<unsigned char>(c))||c=='-'||c=='_'||c==' ')safe+=c;if(safe.empty())safe="Version";const auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());std::ostringstream name;name<<std::put_time(std::localtime(&now),"%Y%m%d-%H%M%S")<<" "<<safe<<".json";if(document_.snapshot(dataDirectory()/"versions"/name.str())){loadVersions();versionName_.clear();status_="Named version saved";}else status_="Could not save version";}
UiJson liveDocument(const UiProject& p,const Stage& s){
 UiJson value=p.toJson();std::map<std::string,std::string> bindings;
 if(s.screen=="general-dialog")bindings["amount"]=s.scenario==1?"9,999,999":compact(s.amount);
 if(s.scenario==1&&s.screen=="currency-hud")bindings={{"coins","999,999,999"},{"pearls","999,999"}};
 if(s.scenario==1&&s.screen=="shop-card")bindings["amount"]="999,999,999";
 if(s.scenario==2)bindings={{"title","Nicht genügend Münzen"},{"detail","Du kannst zusätzliche Münzen im Shop erhalten."},{"actionLabel","Shop jetzt öffnen"},{"message","Du brauchst noch {amount} Münzen"},{"amount",compact(s.amount)}};
 if(s.scenario==3)bindings["extra"]="Requires level 12";
 value["preview"]={{"screen",s.screen},{"variant",s.variant},{"width",s.width},{"height",s.height},{"density",s.density},{"safe",s.safe},{"flow",s.flow},{"level",s.level},{"coins",s.coins},{"pearls",s.pearls},{"tank",s.tank},{"owned",s.owned},{"upgrades",s.upgrades},{"values",bindings},{"pressed",s.scenario==4},{"disabled",s.scenario==5},{"elapsed",s.elapsed}};
 return value;
}
void Studio::startGame(){
 stopGame();writeUiJson(livePath_,liveDocument(previewProject(document_.project()),stage()));sentFingerprint_=uiFingerprint(previewProject(document_.project()));sentRevision_=document_.version();
 std::filesystem::path executable=options_.assets.parent_path()/"build/desktop/aquarium";if(!std::filesystem::exists(executable))throw std::runtime_error("Build the aquarium target before starting a connected preview");
 std::vector<std::string> strings{executable.string(),"--assets",options_.assets.string(),"--fresh","--still","--ui-project",livePath_.string()};
 if(flow_){strings.insert(strings.end(),{"--fixture","tanks"});}else strings.insert(strings.end(),{"--ui-screen",screen_,"--ui-variant",variant_});
 std::vector<const char*> args;for(auto& s:strings)args.push_back(s.c_str());args.push_back(nullptr);previewProcess_=SDL_CreateProcess(args.data(),false);if(!previewProcess_)throw std::runtime_error(SDL_GetError());sentStage_=stage();connectedRevision_.clear();status_="Starting a separate game with this draft";
}
void Studio::stopGame(){if(previewProcess_){SDL_KillProcess(previewProcess_,true);SDL_DestroyProcess(previewProcess_);previewProcess_=nullptr;}connectedRevision_.clear();}
void Studio::bridge(){if(!previewProcess_)return;int code{};if(SDL_WaitProcess(previewProcess_,false,&code)){SDL_DestroyProcess(previewProcess_);previewProcess_=nullptr;status_="Preview game closed";return;}
 try{if(linkDraft_&&(document_.version()!=sentRevision_||sentStage_!=stage())){writeUiJson(livePath_,liveDocument(previewProject(document_.project()),stage()));sentRevision_=document_.version();sentFingerprint_=uiFingerprint(previewProject(document_.project()));sentStage_=stage();}
  auto ack=std::filesystem::path(livePath_.string()+".ack.json");if(std::filesystem::exists(ack)){std::ifstream in(ack);connectedRevision_=UiJson::parse(in).value("revision","");}
 }catch(const std::exception& e){status_=e.what();}
}
}
int main(int argc,char** argv){try{auto options=aq::studio::options(argc,argv);if(options.init){if(std::filesystem::exists(options.project))throw std::runtime_error("Project already exists");aq::UiDocument document(options.project,options.assets);if(!document.save())throw std::runtime_error(document.error());std::cout<<options.project<<'\n';return 0;}return aq::studio::Studio(std::move(options)).run();}catch(const std::exception& error){std::cerr<<"Aquarium Studio: "<<error.what()<<'\n';return 1;}}
