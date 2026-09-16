#include "aquarium/ui_renderer.hpp"
#include "aquarium/theme.hpp"
#include <algorithm>
#include <array>

namespace aq {
namespace {
constexpr Color tankInk{5,34,73,255},tankBody{38,79,121,255};
std::string rowPrice(Amount value){
 if(value>=1000&&value%100==0){
  const auto decimal=value%1000/100;
  return std::to_string(value/1000)+(decimal?"."+std::to_string(decimal):"")+"K";
 }
 return compact(value);
}
}

void View::tanks(){
 const auto& domain=session_.domain();const auto& state=domain.state();
 const theme::DialogLayout l(canvas_);const float u=l.u;
 selectedTank_=std::clamp(selectedTank_,1,5);
 const auto image=[&](std::string_view name,Rect r){canvas_.image("tank-menu/"+std::string(name)+".png",r);};
 const auto title=[&](std::string_view value,float x,float y,float size,float width,bool centered=false,Color color=tankInk){
  canvas_.label(value,l.x+x*u,l.y+y*u,size*u,centered,width*u,color);
 };
 const auto body=[&](std::string_view value,float x,float y,float size,float width,bool centered=false,Color color=tankBody){
  // The source has narrower body lettering than the bundled Nunito face.
  // Retain the existing font and its height while matching that width.
  canvas_.text(value,l.x+x*u,l.y+y*u,size*u,color,centered,width*u,false,false,false,false,0,centered?.90f:.88f);
 };
 panelRect_=l.frame();
 // This is the shared Lagoon panel, sliced once at the reference border size.
 image("panel",panelRect_);
 image("well",l.rect(320,178,467,765));
 image("sign",l.rect(378,60,378,119));
 touchTarget("tank-heading",l.rect(378,60,378,119),[]{});
 const auto close=l.rect(1374,99,109,111);
 image("close",buttonVisual("panel-close",close));
 const float closePad=std::max(0.f,(canvas_.minimumTouchSize()-close.h)*.5f);
 touchTarget("panel-close",{close.x-closePad,close.y-closePad,close.w+2*closePad,close.h+2*closePad},[this]{open(Panel::None);});

 const auto purchase=[this](int id,Currency currency){
  const auto& d=session_.domain();const bool owned=d.tank({id})!=nullptr;
  const auto* next=d.nextTankEntitlement({id});std::string requirement;
  if(next){
   if(d.level()<next->level)requirement="Requires level "+std::to_string(next->level);
   else if(!next->prerequisite.empty()){
    const auto& entries=d.content().tankEntitlements;
    const auto prior=std::find_if(entries.begin(),entries.end(),[&](const auto& e){return e.id==next->prerequisite;});
    if(prior!=entries.end()){
     const auto* tank=d.tank(prior->tank);
     if(!tank||tank->slots<prior->slots)requirement="Requires Tank "+std::to_string(prior->tank.value)+" with "+std::to_string(prior->slots)+" slots";
    }
   }
   // The price button always explains its currency shortfall, including when
   // a separate unlock requirement would make the domain return earlier.
   const auto& wallet=d.state().wallet;
   const CurrencyShortfall missing{
    currency==Currency::Coins?std::max(Amount{},next->cost.coins-wallet.coins):0,
    currency==Currency::Pearls?std::max(Amount{},next->cost.pearls-wallet.pearls):0};
   if(missing.coins||missing.pearls){showFunds(missing,requirement);return;}
  }
  const auto result=command({.action=owned?Action::ExpandTank:Action::UnlockTank,.tank={id},.currency=currency});
  if(result){selectedTank_=id;return;}
  if(result.error==Error::Funds||result.error==Error::SaveFailure)return;
  MessageDialogContent notice;
  notice.title=result.error==Error::Level&&next?"Level "+std::to_string(next->level)+" Needed":result.error==Error::Maximum?"Tank Fully Upgraded":"Tank Locked";
  notice.illustration="tank-menu/preview.png";
  notice.message={{requirement.empty()?errorText(result.error):requirement}};
  if(result.error==Error::Level)notice.detail="Keep growing fish to earn XP.";
  showNotice(std::move(notice));
 };
 // Price sprites contain the source currency art. Only the amount is native,
 // and it follows the same press transform as its button.
 const auto priceButton=[&](std::string id,Rect r,Amount amount,bool pearl,bool small,int tank){
  const Rect visual=buttonVisual(id,r);const float scale=visual.w/r.w;
  image(pearl?(small?"pearl-small":"pearl-button"):(small?"coin-small":"coin-button"),visual);
  const bool shortfall=(pearl?state.wallet.pearls:state.wallet.coins)<amount;
  const auto text=small?rowPrice(amount):compact(amount);
  const Color color=shortfall?insufficientFundsColor:pearl?tankInk:theme::white;
  const float size=(small?25.f:47.f)*u*scale;
  const float center=small?.66f:.64f;
  canvas_.label(text,visual.x+visual.w*center,visual.y+visual.h*(small?.14f:.18f),size,true,visual.w*(small?.54f:.56f),color,
                pearl?0.f:.7f*u,small?Color{103,117,128,255}:Color{18,130,48,255});
  touchTarget(std::move(id),r,[purchase,tank,pearl]{purchase(tank,pearl?Currency::Pearls:Currency::Coins);});
 };

 constexpr std::array<float,5> rowY{191,335,476,638,793},rowH{135,133,154,147,145};
 constexpr std::array<float,5> priceY{0,408,572,730,884},thumbY{205,345,491,650,805};
 for(int i=0;i<5;++i){
  const int id=i+1;const auto* tank=domain.tank({id});const bool active=id==state.activeTank.value;
  const bool selected=id==selectedTank_,locked=!tank;
  const bool previousMissing=id>1&&!domain.tank({id-1});
  const float y=rowY[i],h=rowH[i];const auto r=l.rect(334,y,442,h);
  image(selected?"row-selected":"row",r);
  touchTarget("tank-select"+std::to_string(id),r,[this,id]{selectedTank_=id;});
  image(locked?"thumbnail-locked":"thumbnail",l.rect(locked?355:351,thumbY[i],locked?134:140,locked?115:111));
  title("Tank "+std::to_string(id),510,y+(i?5:11),33,244);
  if(tank){
   title(active?"Current tank":"Owned tank",510,y+49,25,241,false,active?Color{0,151,231,255}:tankBody);
   const float trackY=y+h-52;
   image("row-progress",l.rect(508,trackY,245,36));
   const auto count=domain.living({id});
   if(count){const float fraction=std::min(1.f,float(count)/tank->slots);canvas_.skin("gold",l.rect(512,trackY+4,237*fraction,27));}
   title(std::to_string(count)+"/"+std::to_string(tank->slots),630,trackY+1,28,221,true);
  }else{
   body("Unlock at Level "+std::to_string(domain.content().tankLevels[i]),512,y+37,24,244);
   if(previousMissing&&i>1){
    const auto value="Requires Tank "+std::to_string(id-1);
    // The reference uses a warm outline for the prerequisite line.
    canvas_.text(value,l.x+512*u,l.y+(y+62)*u,22*u,{161,118,39,255},false,241*u,false,false,false,false,0,.97f);
    canvas_.text(value,l.x+511*u,l.y+(y+61)*u,22*u,{247,192,57,255},false,241*u,false,false,false,false,0,.97f);
   }
   const float py=priceY[i];
   priceButton("tank-buy"+std::to_string(id),l.rect(509,py,120,47),domain.content().tankCosts[i][0],false,true,id);
   priceButton("tank-pearl"+std::to_string(id),l.rect(636,py,120,48),domain.content().tankPearlCosts[i][0],true,true,id);
  }
  if(highlightedTank_.value==id&&!selected)canvas_.outline(r,{255,203,46,255},24*u,3*u);
 }

 const int id=selectedTank_,index=id-1;const auto* tank=domain.tank({id});
 const bool active=id==state.activeTank.value,maximum=tank&&tank->slots>=20;
 const bool levelLocked=!tank&&domain.level()<domain.content().tankLevels[index];
 const bool previousMissing=id>1&&!domain.tank({id-1});
 const int slots=tank?tank->slots:10;const auto count=tank?domain.living({id}):0;
 title("Tank "+std::to_string(id),813,178,59,640);
 image("preview",l.rect(810,243,642,325));
 if(!tank)image("lock-badge",l.rect(1232,280,76,76));
 int warnings=0;
 for(const auto& fish:state.fish)if(fish.tank.value==id&&!fish.stashed&&!fish.egg){
  const auto* species=domain.content().find(fish.species);
  if(species&&careOf(*species,fish,state.simNow)!=Care::Fed)++warnings;
 }
 if(warnings){
  image("warning",l.rect(1200,279,120,71));
  title(std::to_string(warnings),1285,292,36,42,true,theme::white);
 }
 image("progress",l.rect(821,578,511,37));
 if(count){const float fraction=std::min(1.f,float(count)/slots);canvas_.skin("cyan",l.rect(826,583,501*fraction,26));}
 title(std::to_string(count)+" / "+std::to_string(slots),1393,576,39,104,true);
 if(tank){
  if(active){image("status",l.rect(987,622,279,44));title("Current Tank",1125,628,27,253,true);touchTarget("tank-current",l.rect(987,622,279,44),[]{});}
  else glassButton("tank-use",l.rect(987,622,279,44),"Use Tank",[this,id]{if(command({.action=Action::SwitchTank,.tank={id}}))open(Panel::None);},"green",27*u);
 }else{
  image("status",l.rect(936,622,380,44));
  title(levelLocked?"Unlock at Level "+std::to_string(domain.content().tankLevels[index]):previousMissing?"Requires Tank "+std::to_string(id-1):"Ready to unlock",1126,630,27,350,true);
 }
 const auto* next=domain.nextTankEntitlement({id});
 if(next&&domain.level()<next->level)title("Upgrade at Level "+std::to_string(next->level),1129,682,22,600,true);
 body(tank?maximum?"Your tank has reached maximum capacity.":"Upgrade your tank to hold more fish.":previousMissing?"Unlock Tank "+std::to_string(id-1)+" first.":"Unlock this tank to make room for more fish.",1129,668,24,626,true);
 if(uiProject()&&!maximum){
  const int step=tank?std::min(2,(slots-10)/5+1):0;
  const std::string coinId=tank?(active?"tank-buy":"tank-buy"+std::to_string(id)):"tank-detail-buy";
  const std::string pearlId=tank?"tank-pearl"+std::to_string(id):"tank-detail-pearl";
  UiBindings bindings;bindings.values={{"title",tank?"Upgrade Capacity":"Unlock Tank"},{"capacity",tank?std::to_string(slots)+" to "+std::to_string(slots+5)+" fish":std::to_string(slots)+" fish"},{"coins",compact(domain.content().tankCosts[index][step])},{"pearls",compact(domain.content().tankPearlCosts[index][step])}};
  bindings.values["coinsShort"]=domain.state().wallet.coins<domain.content().tankCosts[index][step]?"1":"0";bindings.values["pearlsShort"]=domain.state().wallet.pearls<domain.content().tankPearlCosts[index][step]?"1":"0";
  drawUi("tank-upgrade","default",l.rect(809,711,644,215),bindings,{{"buyCoins",{coinId,{},[purchase,id]{purchase(id,Currency::Coins);}}},{"buyPearls",{pearlId,{},[purchase,id]{purchase(id,Currency::Pearls);}}}});return;
 }
 image("upgrade",l.rect(809,711,644,215));
 title(tank?maximum?"Maximum Capacity":"Upgrade Capacity":"Unlock Tank",1129,718,38,610,true);
 const int step=tank?std::min(2,(slots-10)/5+1):0;
 if(tank&&!maximum){
  title(std::to_string(slots),1084,766,37,80,true);
  image("arrow",l.rect(1110,772,36,28));
  title(std::to_string(slots+5),1173,766,37,80,true);
 }else title(std::to_string(slots)+" fish",1131,766,37,400,true);
 if(maximum){title("Fully upgraded",1131,841,37,540,true,tankBody);return;}
 // Row buttons keep their own IDs. The detail pane uses distinct IDs when
 // an unowned tank also has purchase controls in the list.
 const auto coinId=tank?(active?"tank-buy":"tank-buy"+std::to_string(id)):"tank-detail-buy";
 const auto pearlId=tank?"tank-pearl"+std::to_string(id):"tank-detail-pearl";
 priceButton(coinId,l.rect(828,815,295,96),domain.content().tankCosts[index][step],false,false,id);
 priceButton(pearlId,l.rect(1139,815,296,96),domain.content().tankPearlCosts[index][step],true,false,id);
}
} // namespace aq
