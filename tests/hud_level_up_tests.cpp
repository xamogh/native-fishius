#include "aquarium/hud_care.hpp"
#include "fixtures.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
SDL_FPoint center(Rect r){return {r.x+r.w*.5f,r.y+r.h*.5f};}
SDL_Event mouse(Uint32 type){SDL_Event e{};e.type=type;e.common.timestamp=1000000000;e.button.button=SDL_BUTTON_LEFT;return e;}
SDL_Event key(SDL_Keycode code,bool repeat=false){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=code;e.key.repeat=repeat;return e;}
bool inside(Rect a,Rect b){return a.w>0&&a.h>0&&a.x>=b.x-.1f&&a.y>=b.y-.1f&&a.x+a.w<=b.x+b.w+.1f&&a.y+a.h<=b.y+b.h+.1f;}
bool separate(Rect a,Rect b){return a.x+a.w<=b.x||b.x+b.w<=a.x||a.y+a.h<=b.y||b.y+b.h<=a.y;}
Event reached(const Content& c,int level){const auto reward=levelReward(c,level);return {"level",{}, {},reward.coins,0,reward.pearls,"",level};}
bool has(const std::vector<LevelUpItem>& items,std::string_view id){return std::any_of(items.begin(),items.end(),[&](const auto& i){return i.id==id;});}
void click(HudLevelUp& dialog,const LevelUpLayout& l,Rect r){
 check(dialog.event(l,mouse(SDL_EVENT_MOUSE_BUTTON_DOWN),center(r)),"Dialog did not consume press");
 check(dialog.event(l,mouse(SDL_EVENT_MOUSE_BUTTON_UP),center(r)),"Dialog did not consume release");
}
bool hasRewardPixels(Canvas& canvas,const HudLevelUp& dialog,const LevelUpLayout& level,const HudLayout& hud){
 canvas.begin();canvas.fill({0,0,canvas.width(),canvas.height()},{0,0,0,255});dialog.paintRewards(canvas,level,hud);
 auto* raw=SDL_RenderReadPixels(canvas.renderer(),nullptr);check(raw,"Cannot inspect reward pixels");
 auto* pixels=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(raw);check(pixels,"Cannot convert reward pixels");
 bool visible=false;
 for(int y=0;y<pixels->h&&!visible;++y)for(int x=0;x<pixels->w;++x){
  const auto* p=static_cast<const Uint8*>(pixels->pixels)+y*pixels->pitch+x*4;
  if(p[0]||p[1]||p[2]){visible=true;break;}
 }
 SDL_DestroySurface(pixels);return visible;
}
}
int main(int argc,char** argv){try{
 using namespace aq;
 const auto assets=std::filesystem::absolute(argc>1?argv[1]:"assets");
 std::ifstream input(assets/"content.json");const auto content=Content::fromJson(Json::parse(input));
 Domain domain(content,1000);domain.fixture("level-up");
 const auto before=encode(domain.state());const auto items=levelUpItems(domain,2);
 check(items.size()==4&&items[0].id=="zebraDanio"&&has(items,"CP-02")&&has(items,"CD-02"),"Level 2 unlocks differ from the launch catalog");
 check(has(items,"bubbleEyeGoldfish"),"Bubble Eye pearl purchase is missing from Level 2 unlocks");
 check(!has(levelUpItems(domain,3),"TK-01-15"),"Owned tank expansion still appears as a level unlock");
 check(has(levelUpItems(domain,7),"TK-02-10"),"New tank level unlock is missing");
 for(int level=1;level<=40;++level)for(const auto& t:content.tankEntitlements)
  if(t.slots>10)check(!has(levelUpItems(domain,level),t.id),"Capacity upgrade appears in level rewards");
 check(!has(levelUpItems(domain,5),"goldenSeahorse")&&!has(levelUpItems(domain,5),"heartfinTetra"),"Unreleased fish leaked into unlocks");
 auto missing=content;for(auto& s:missing.species)s.artReady=false;for(auto& d:missing.decorations)d.artReady=false;
 check(levelUpItems(Domain(missing),2).empty(),"Missing artwork leaves blank unlocks");

 const auto layout=layoutLevelUp(1440,840,{});HudLevelUp dialog;
 dialog.collect(domain,{"sale",{}, {},20,4,0,"",2});dialog.reveal(2);check(!dialog.open(),"Ordinary sale opened the level dialog");
 dialog.collect(domain,reached(content,2));dialog.reveal(1);check(!dialog.open(),"Dialog appeared before XP reached the new level");
 dialog.reveal(2);check(dialog.open()&&dialog.current()->reward.coins==500&&dialog.current()->reward.pearls==1,"Committed reward amounts were lost");
 dialog.event(layout,mouse(SDL_EVENT_MOUSE_BUTTON_UP),center(layout.continueButton));check(dialog.open(),"Unmatched release dismissed the receipt");
 dialog.event(layout,mouse(SDL_EVENT_MOUSE_BUTTON_DOWN),center(layout.continueButton));dialog.event(layout,mouse(SDL_EVENT_MOUSE_MOTION),center(layout.badge));
 dialog.event(layout,mouse(SDL_EVENT_MOUSE_BUTTON_UP),center(layout.continueButton));check(dialog.open(),"Drag back to Continue dismissed the receipt");
 for(auto type:{SDL_EVENT_WINDOW_FOCUS_LOST,SDL_EVENT_WILL_ENTER_BACKGROUND,SDL_EVENT_RENDER_DEVICE_RESET,SDL_EVENT_RENDER_TARGETS_RESET}){
  dialog.event(layout,mouse(SDL_EVENT_MOUSE_BUTTON_DOWN),center(layout.continueButton));SDL_Event cancelled{};cancelled.type=type;dialog.event(layout,cancelled,{});
  dialog.event(layout,mouse(SDL_EVENT_MOUSE_BUTTON_UP),center(layout.continueButton));check(dialog.open(),"Cancelled press dismissed the receipt");
 }
 dialog.collect(domain,reached(content,3));dialog.collect(domain,reached(content,4));dialog.reveal(4);
 click(dialog,layout,layout.continueButton);check(dialog.open()&&dialog.current()->level==3&&dialog.current()->reward.pearls==0,"Multi-level receipts are out of order");
 dialog.event(layout,key(SDLK_RETURN,true),{});check(dialog.current()->level==3,"Held Return skipped a level");
 dialog.event(layout,key(SDLK_RETURN),{});check(dialog.current()->level==4,"Return did not advance to the next earned level");
 click(dialog,layout,layout.dialog.close);check(!dialog.open(),"Close did not dismiss the final level");
 dialog.collect(domain,reached(content,2));dialog.reveal(4);check(!dialog.open(),"Repeated event reopened a credited level");
 check(encode(domain.state())==before,"Dialog changed wallet, inventory or saved progress");

 for(auto code:{SDLK_ESCAPE,SDLK_SPACE}){HudLevelUp d;d.collect(domain,reached(content,2));d.reveal(2);d.event(layout,key(code),{});check(!d.open(),"Keyboard dismissal failed");}
 HudLevelUp backdrop;backdrop.collect(domain,reached(content,2));backdrop.reveal(2);click(backdrop,layout,{0,0,1,1});check(!backdrop.open(),"Backdrop did not dismiss");
 HudLevelUp grid;grid.collect(domain,reached(content,40));grid.reveal(40);
 grid.advance(1.45,false);const HudRewardDisplay gridBalances{.coins=4000,.pearls=10};
 const auto collecting=grid.display(gridBalances);
 const auto limit=levelUpScrollLimit(layout,grid.current()->items.size());check(limit>0,"Long unlock list is truncated");
 SDL_Event wheel{};wheel.type=SDL_EVENT_MOUSE_WHEEL;wheel.wheel.y=-2;
 grid.event(layout,wheel,center(layout.badge));check(grid.scroll()==0,"Wheel outside the grid scrolled the cards");
 auto settle=[&]{for(int i=0;i<60;++i)grid.advance(1./60,false);};
 grid.event(layout,wheel,center(layout.grid));check(grid.scroll()==0,"Mouse wheel jumps before easing");settle();check(grid.scroll()>0&&grid.open(),"Mouse wheel did not scroll the unlocks");
 wheel.wheel.direction=SDL_MOUSEWHEEL_FLIPPED;grid.event(layout,wheel,center(layout.grid));settle();check(std::abs(grid.scroll())<.001f,"Natural scrolling was reversed");
 grid.event(layout,key(SDLK_END),{});check(grid.scroll()==limit,"End did not reveal the final card");
 grid.event(layout,key(SDLK_HOME),{});check(grid.scroll()==0,"Home did not restore the first cards");
 grid.event(layout,key(SDLK_RIGHT),{});check(grid.scroll()>0&&grid.open(),"Right key did not move the unlocks");
 grid.event(layout,key(SDLK_LEFT),{});check(grid.scroll()==0&&grid.open(),"Left key did not restore the first cards");
 grid.event(layout,key(SDLK_LEFT),{});check(grid.scroll()==0&&grid.open(),"Left key moved beyond the first card");
 wheel.wheel.direction=SDL_MOUSEWHEEL_NORMAL;wheel.wheel.y=0;wheel.wheel.x=1;
 grid.event(layout,wheel,center(layout.grid));settle();check(grid.scroll()>0,"Horizontal trackpad wheel did not scroll");
 grid.event(layout,key(SDLK_HOME),{});
 const auto first=levelUpCardBounds(layout,0),second=levelUpCardBounds(layout,1),third=levelUpCardBounds(layout,2);
 check(first.x<second.x&&second.x<third.x&&first.y==second.y&&second.y==third.y,"Unlocks are not arranged in a horizontal gallery");
 HudPointer touch;
 const auto start=center(layout.grid);
 for(auto type:{SDL_EVENT_FINGER_DOWN,SDL_EVENT_FINGER_MOTION,SDL_EVENT_FINGER_UP}){
  SDL_Event e{};e.type=type;e.common.timestamp=type==SDL_EVENT_FINGER_DOWN?1000000000:type==SDL_EVENT_FINGER_MOTION?1100000000:1105000000;e.tfinger.fingerID=7;e.tfinger.x=(start.x-(type==SDL_EVENT_FINGER_DOWN?0:100))/1440;e.tfinger.y=start.y/840;
  check(normalizeHudPointer(touch,e,1440,840),"Scroll touch normalization failed");
  const auto point=type==SDL_EVENT_FINGER_MOTION?SDL_FPoint{e.motion.x,e.motion.y}:SDL_FPoint{e.button.x,e.button.y};grid.event(layout,e,point);
 }
 check(grid.scroll()>0&&grid.open(),"Touch swipe did not scroll the grid");
 const float released=grid.scroll();grid.advance(.1,false);check(grid.scroll()>released,"Unlock gallery does not glide after touch release");
 check(grid.display(gridBalances).coins>=collecting.coins&&grid.display(gridBalances).pearls>=collecting.pearls,"Scrolling restarted the reward animation");
 grid.event(layout,mouse(SDL_EVENT_MOUSE_BUTTON_DOWN),start);
 SDL_Event cancelled{};cancelled.type=SDL_EVENT_WINDOW_FOCUS_LOST;grid.event(layout,cancelled,{});
 grid.event(layout,mouse(SDL_EVENT_MOUSE_BUTTON_UP),center(layout.continueButton));check(grid.open(),"Cancelled scrolling activated Continue");
 grid.event(layout,mouse(SDL_EVENT_MOUSE_BUTTON_DOWN),start);
 grid.event(layout,mouse(SDL_EVENT_MOUSE_MOTION),center(layout.continueButton));
 grid.event(layout,mouse(SDL_EVENT_MOUSE_BUTTON_UP),center(layout.continueButton));check(grid.open(),"Scrolling onto Continue dismissed the receipt");
 for(auto type:{SDL_EVENT_FINGER_DOWN,SDL_EVENT_FINGER_UP}){
  auto p=center(layout.continueButton);SDL_Event e{};e.type=type;e.tfinger.fingerID=7;e.tfinger.x=p.x/1440;e.tfinger.y=p.y/840;
  check(normalizeHudPointer(touch,e,1440,840),"Touch normalization failed");grid.event(layout,e,{e.button.x,e.button.y});
 }
 check(!grid.open(),"Touch Continue failed");
 auto largeCatalog=content;
 for(int i=0;i<60;++i){auto item=*content.findDecor("CP-02");item.id="grid-test-"+std::to_string(i);largeCatalog.decorations.push_back(item);}
 Domain crowded(largeCatalog,1000);HudLevelUp many;many.collect(crowded,reached(content,2));many.reveal(2);
 check(many.current()->items.size()>=60,"Large unlock lists were capped");many.event(layout,key(SDLK_END),{});
 const auto last=levelUpCardBounds(layout,many.current()->items.size()-1,many.scroll());
 check(inside(last,layout.grid),"Final card is unreachable in a large unlock list");
 many.collect(crowded,reached(content,3));many.reveal(3);click(many,layout,layout.continueButton);
 check(many.current()->level==3&&many.scroll()==0,"Next level kept the previous grid position");

 Canvas canvas(assets,1088,635,true);
 // Large rounded artwork must cover the body on the software renderer too.
 // This catches the wide triangle-fan holes seen in the first visual pass.
 canvas.previewViewport(PreviewViewport{1440,840,1,{}});canvas.begin();
 canvas.fill({0,0,canvas.width(),canvas.height()},{255,0,255,255});
 canvas.roundedImage("dialogs/level-up-mobile-background-v1.png",{0,0,canvas.width(),canvas.height()},20);
 auto* raw=SDL_RenderReadPixels(canvas.renderer(),nullptr);check(raw,"Cannot inspect dialog pixels");
 auto* pixels=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(raw);check(pixels,"Cannot convert dialog pixels");
 bool covered=true;
 for(int y=24;y<pixels->h-24;y+=23)for(int x=24;x<pixels->w-24;x+=29){
  const auto* p=static_cast<const Uint8*>(pixels->pixels)+y*pixels->pitch+x*4;
  if(p[0]>245&&p[1]<10&&p[2]>245)covered=false;
 }
 SDL_DestroySurface(pixels);check(covered,"Rounded dialog artwork has uncovered triangles");
 const auto flightLayout=layoutLevelUp(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
 const auto hud=layoutHud(canvas.width(),canvas.height(),canvas.safeInsets(),canvas.minimumTouchSize());
 HudLevelUp flight;flight.collect(domain,reached(content,2));flight.advance(60,false);flight.reveal(2);
 const HudRewardDisplay balances{.coins=2000,.pearls=10};
 check(flight.display(balances).coins==1500&&flight.display(balances).pearls==9,"Level bonus was visible before travel");
 flight.advance(-10,false);flight.advance(std::numeric_limits<double>::quiet_NaN(),false);flight.advance(.499,false);
 check(!hasRewardPixels(canvas,flight,flightLayout,hud),"A level reward flew before the 500 ms pause");
 flight.advance(.001,false);check(!hasRewardPixels(canvas,flight,flightLayout,hud),"Reward did not begin at its source");
 flight.advance(.05,false);check(hasRewardPixels(canvas,flight,flightLayout,hud),"Coins and pearls did not start flying after 500 ms");
 check(flight.display(balances).coins==1500&&flight.display(balances).pearls==9,"HUD counted rewards before arrival");
 flight.advance(1.65,false);
 check(!hasRewardPixels(canvas,flight,flightLayout,hud)&&flight.display(balances).coins==2000&&flight.display(balances).pearls==10,"Travel did not finish at the saved balances");
 flight.reveal(2);flight.advance(3,false);check(!hasRewardPixels(canvas,flight,flightLayout,hud),"An open dialog repeated its flight");
 flight.collect(domain,reached(content,3));flight.reveal(3);click(flight,flightLayout,flightLayout.continueButton);flight.advance(.49,false);
 check(!hasRewardPixels(canvas,flight,flightLayout,hud)&&flight.display(balances).coins==1925&&flight.display(balances).pearls==10,"Next level did not get its own pause or invented pearls");
 flight.advance(60,false);check(flight.display(balances).coins==2000,"A long frame stranded level coins");
 HudLevelUp accessible;accessible.collect(domain,reached(content,2));accessible.advance(0,true);accessible.reveal(2);
 check(accessible.display(balances).coins==2000&&accessible.display(balances).pearls==10,"Reduced motion withheld rewards");
 accessible.advance(.5,true);check(accessible.display(balances).coinPulse>0&&accessible.display(balances).pearlPulse>0,"Reduced motion omitted the delayed highlight");
 accessible.advance(.5,false);check(!hasRewardPixels(canvas,accessible,flightLayout,hud)&&accessible.display(balances).coins==2000,"Changing motion settings restarted a reward");
 check(encode(domain.state())==before,"Reward travel changed the saved wallet");
 // A real, committed sale reaches the level. A replay must not reopen it.
 Session session(content,"/tmp/fishius-level-dialog-unused.json",1000,true);HudCare care(canvas,session);
 auto near=session.domain().state();near.xp=content.levels[1]-1;near.highestRewardedLevel=1;near.tutorialStep=11;
 auto& fish=near.fish.front();fish.scripted=false;testing::stage(fish,4);fish.lastFedAt=near.simNow;const auto id=fish.id;
 session.domain().install(near);check(bool(session.command({.action=Action::Sell,.fish=id})),"Level-crossing sale failed");
 const auto committed=encode(session.domain().state());care.advance(0);check(!care.levelUp().open(),"Level dialog skipped the XP flight");
 care.advance(3);check(care.levelUp().open()&&care.levelUp().current()->level==2,"Committed sale did not open the level dialog");
 click(care.levelUp(),layout,layout.continueButton);check(encode(session.domain().state())==committed,"Continue paid a second reward");
 check(care.rewardDisplay().coins==session.domain().state().wallet.coins&&care.rewardDisplay().pearls==session.domain().state().wallet.pearls,"Early dismissal hid a committed reward");
 check(session.command({.action=Action::Sell,.fish=id}).replayed,"Replay setup failed");care.advance(3);check(!care.levelUp().open(),"Replayed settlement reopened the receipt");
 Session reduced(content,"/tmp/fishius-level-reduced-unused.json",1000,true);near.settings.reducedMotion=true;reduced.domain().install(near);HudCare reducedCare(canvas,reduced);
 check(bool(reduced.command({.action=Action::Sell,.fish=id})),"Reduced-motion sale failed");reducedCare.advance(0);check(reducedCare.levelUp().open(),"Reduced motion delayed the receipt");

 const auto output=argc>2?std::filesystem::path(argv[2]):std::filesystem::path{};
 if(!output.empty())std::filesystem::create_directories(output);
 for(const auto viewport:std::array{PreviewViewport{1642,958,1,{}},PreviewViewport{1440,840,1,{}},PreviewViewport{1088,635,1,{}},PreviewViewport{667,375,1,{}},PreviewViewport{844,390,1,{59,0,59,21}},PreviewViewport{390,844,1,{}},PreviewViewport{320,240,1,{}}}){
  canvas.previewViewport(viewport);canvas.begin();const auto margins=canvas.safeInsets();
  const auto l=layoutLevelUp(canvas.width(),canvas.height(),margins,canvas.minimumTouchSize());
  const Rect safe{margins.left,margins.top,canvas.width()-margins.left-margins.right,canvas.height()-margins.top-margins.bottom};
  check(l.rewards.y>=l.received.y+l.received.h+canvas.minimumTouchSize()/9,"Received heading crowds the currencies");
  const auto walletLayout=layoutHud(canvas.width(),canvas.height(),margins,canvas.minimumTouchSize());
  for(auto part:{HudPart::CoinIcon,HudPart::PearlIcon})check(walletLayout[part].y+walletLayout[part].h<=l.dialog.frame.y,"Dialog hides a reward destination");
  check(inside(l.dialog.frame,safe),"Level frame crosses the safe area");
  for(auto r:{l.badge,l.received,l.rewards,l.unlocks,l.continueButton,l.gallery,l.grid})check(inside(r,l.dialog.body),"Level content crosses the body at "+std::to_string(viewport.width)+"x"+std::to_string(viewport.height));
  for(auto r:{l.badge,l.received,l.rewards,l.continueButton})check(separate(l.gallery,r),"Scrollable cards overlap fixed content at "+std::to_string(viewport.width)+"x"+std::to_string(viewport.height));
  check(l.rewards.y+l.rewards.h<l.continueButton.y||l.rewards.x+l.rewards.w<l.continueButton.x,"Rewards overlap Continue");
  if(!l.compact)check(inside(l.progress,l.dialog.body)&&separate(l.progress,l.grid),"Gallery progress crosses the card or body");
  check(l.cardHeight>=canvas.minimumTouchSize()*.6f,"Unlock cards became too short to read");
  check(l.dialog.close.h>=canvas.minimumTouchSize()-.1f&&l.continueButton.h>=canvas.minimumTouchSize()-.1f,"Level controls are smaller than touch targets");
  for(int level:{2,3,10,40}){
   domain.fixture(level==40?"level-up-40":level==10?"level-up-10":"level-up");HudLevelUp d;d.collect(domain,reached(content,level));d.reveal(level);
   const auto maxScroll=levelUpScrollLimit(l,d.current()->items.size());
   for(std::size_t i=0;i<d.current()->items.size();++i){
    const auto left=levelUpCardBounds(l,i).x-l.grid.x;
    check(inside(levelUpCardBounds(l,i,std::clamp(left,0.f,maxScroll)),l.grid),"An unlock card cannot be fully scrolled into view");
   }
   for(bool bottom:{false,true}){
    if(bottom&&maxScroll==0)continue;
    if(bottom)d.event(l,key(SDLK_END),{});
    canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false);
    paintHud(canvas,domain,layoutHud(canvas.width(),canvas.height(),margins,canvas.minimumTouchSize()));d.paint(canvas,l);
    if(!output.empty())check(canvas.capture(output/("level-"+std::to_string(level)+"-"+std::to_string(viewport.width)+"x"+std::to_string(viewport.height)+(bottom?"-bottom.png":"-top.png"))),"Level capture failed");
   }
  }
  if(!output.empty()){
   domain.fixture("level-up-10");HudLevelUp d;d.collect(domain,reached(content,10));d.reveal(10);
   const auto wallet=domain.state().wallet;const HudRewardDisplay saved{.coins=wallet.coins,.pearls=wallet.pearls,.xp=double(domain.state().xp)};
   double age=0;
   for(const double time:{0.,.49,.65,.95,1.35,1.55,2.2}){
    d.advance(time-age,false);age=time;canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false);
    const auto h=layoutHud(canvas.width(),canvas.height(),margins,canvas.minimumTouchSize());const auto values=d.display(saved);
    paintHud(canvas,domain,h,{},{},&values);d.paint(canvas,l);d.paintRewards(canvas,l,h);
    check(canvas.capture(output/("flight-"+std::to_string(viewport.width)+"x"+std::to_string(viewport.height)+"-"+std::to_string(int(time*1000))+"ms.png")),"Reward flight capture failed");
   }
   if(viewport.width==1088){
    domain.fixture("level-up");HudLevelUp animation;animation.collect(domain,reached(content,2));animation.reveal(2);
    const auto balance=domain.state().wallet;const HudRewardDisplay totals{.coins=balance.coins,.pearls=balance.pearls,.xp=double(domain.state().xp)};
    std::filesystem::create_directories(output/"sequence");
    for(int frame=0;frame<90;++frame){
     if(frame)animation.advance(1./30,false);
     canvas.begin();canvas.scene(domain,1,0,Tool::Select,{},false);
     const auto h=layoutHud(canvas.width(),canvas.height(),margins,canvas.minimumTouchSize());const auto values=animation.display(totals);
     paintHud(canvas,domain,h,{},{},&values);animation.paint(canvas,l);animation.paintRewards(canvas,l,h);
     const auto number=std::to_string(frame);
     check(canvas.capture(output/"sequence"/(std::string(3-number.size(),'0')+number+".png")),"Animation capture failed");
    }
   }
  }
 }
 std::cout<<"PASS committed level receipts, exact rewards, catalog gates, horizontal mobile gallery, multi-level queue, replay, mouse, touch, keyboard, cancellation, delayed flights, reduced motion and responsive layout\n";
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
