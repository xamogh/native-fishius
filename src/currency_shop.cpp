#include "aquarium/theme.hpp"
#include "aquarium/ui_renderer.hpp"
#include <algorithm>
#include <array>

namespace aq {

void View::showFunds(CurrencyShortfall missing,std::string requirement){
 cancelGesture();buttons_.clear();fundsCurrency_.reset();fundsDialog_=missing;
 noticeDialog_.reset();fundsRequirement_=std::move(requirement);
 currencyOfferTitle_.clear();currencyOfferDetail_.clear();
 fundsAnimation_.motion.show(true,now_);
}

void View::showNotice(MessageDialogContent content){
 cancelGesture();buttons_.clear();fundsDialog_.reset();fundsCurrency_.reset();fundsRequirement_.clear();
 noticeDialog_=std::move(content);fundsAnimation_.motion.show(true,now_);
}

void View::showCurrencyFunds(Currency currency){
 resetActionTool();
 cancelGesture();buttons_.clear();fundsCurrency_=currency;fundsDialog_=CurrencyShortfall{};
 noticeDialog_.reset();fundsRequirement_.clear();
 fundsAnimation_.motion.show(true,now_);
}

void View::dismissFunds(){
 cancelGesture();buttons_.clear();fundsDialog_.reset();noticeDialog_.reset();fundsRequirement_.clear();fundsAnimation_.motion.show(false,now_);
}

void View::openCurrencyShop(Currency currency){
 if(panel_!=Panel::CurrencyShop)currencyReturn_={panel_,selected_,category_,page_};
 dismissFunds();currencyCategory_=currency;
 open(Panel::CurrencyShop);
 buttons_.clear();panelButtonStart_=0;
}

void View::closeCurrencyShop(){
 const auto back=currencyReturn_;open(back.panel);
 selected_=back.fish;category_=back.category;page_=back.page;
}

void View::fundsDialog(){
 if(noticeDialog_){
  fundsBounds_=fundsAnimation_.bounds=messageDialog(*noticeDialog_,"notice-ok","notice-close",[this]{dismissFunds();},[this]{dismissFunds();});
  return;
 }
 const auto missing=*fundsDialog_;const bool direct=fundsCurrency_.has_value();
 const bool pearls=direct?*fundsCurrency_==Currency::Pearls:missing.pearls>0&&!missing.coins;
 const bool canShop=direct||missing.coins||missing.pearls,preview=!canShop;
 const auto& variant=pearls?dialogDesign().pearls:dialogDesign().coins;
 MessageDialogContent content;
 content.actionLabel=variant.actionLabel;
 content.title=preview?"Coming Soon":direct?(pearls?"More Pearls":"More Coins"):variant.title;
 content.illustration=preview?"theme/starter.png":variant.illustration;
 if(preview){
  content.message={{currencyOfferTitle_.empty()?"Currency packs":currencyOfferTitle_}};
  content.extra=currencyOfferDetail_;
  content.detail="Purchases are not available yet.";
  content.actionLabel="Back to Shop";
 }else if(direct){
  content.message={{pearls?"Get more pearls":"Get more coins"}};
  content.detail="Find a pack in the shop!";
 }else{
  const auto amount=pearls?missing.pearls:missing.coins;
  const auto& suffix=amount==1?variant.singularSuffix:variant.pluralSuffix;
  content.message={{variant.prefix},{compact(amount),true},{suffix}};
  content.detail=variant.detail;
  if(!pearls&&missing.pearls)content.extra="Also need "+compact(missing.pearls)+(missing.pearls==1?" pearl":" pearls");
  if(!fundsRequirement_.empty())content.extra+=(content.extra.empty()?"":" • ")+fundsRequirement_;
 }
 if(uiProject()&&!direct&&!preview){
  const auto& variants=uiProject()->screen("general-dialog").variants;auto it=variants.find(pearls?"pearls":"coins");
  if(it!=variants.end()){const auto& v=it->second.values;const auto get=[&](const char* key,const std::string& fallback){auto found=v.find(key);return found==v.end()?fallback:found->second;};
   content.title=get("title",content.title);content.illustration=get("illustration",content.illustration);content.detail=get("detail",content.detail);content.actionLabel=get("actionLabel",content.actionLabel);
   const auto amount=pearls?missing.pearls:missing.coins;const auto pattern=get(amount==1?"singularMessage":"message","You need {amount} more");content.message.clear();for(const auto& run:uiAmountRuns(pattern,compact(amount)))content.message.push_back({run.text,run.emphasis});
  }
 }
 fundsBounds_=fundsAnimation_.bounds=messageDialog(content,"funds-shop","funds-close",[this,canShop,pearls]{
  if(canShop)openCurrencyShop(pearls?Currency::Pearls:Currency::Coins);else dismissFunds();
 },[this]{dismissFunds();});
}

void View::currencyShop(){
 const theme::Layout l(canvas_);const float u=l.u;
 panelRect_=l.rect(278,168,1124,729);canvas_.skin("panel",panelRect_);
 titleSign(l.rect(321,103,460,126),"Shop","lagoon/shop.png");
 canvas_.text("Get Coins & Pearls",l.x+816*u,l.y+194*u,27*u,theme::body,false,415*u,true);
 closeButton("panel-close",l.rect(1287,134,102,101),[this]{closeCurrencyShop();});
 struct Offer {const char* asset;const char* title;const char* amount;const char* price;const char* detail;};
 static constexpr std::array<Offer,7> offers{{
  {"currency-shop/coins-small.png","Small Coin Pack","2,500","$0.99","2,500 Coins • $0.99"},
  {"currency-shop/coins-medium.png","Medium Coin Pack","12,000","$3.99","12,000 Coins • $3.99"},
  {"currency-shop/coins-chest.png","Large Coin Pack","35,000","$9.99","35,000 Coins • $9.99"},
  {"currency-shop/pearls-small.png","Small Pearl Pack","15","$1.99","15 Pearls • $1.99"},
  {"currency-shop/pearls-chest.png","Medium Pearl Pack","40","$4.99","40 Pearls • $4.99"},
  {"currency-shop/pearls-shell.png","Large Pearl Pack","90","$9.99","90 Pearls • $9.99"},
  {"theme/starter.png","Starter Bundle","25,000 Coins + 30 Pearls","$4.99","25,000 Coins + 30 Pearls • $4.99"}
 }};
 for(int i=0;i<7;++i){
  const auto& offer=offers[i];const bool bundle=i==6,coins=i<3;
  const auto r=bundle?l.rect(1100,257,274,500):l.rect(326+(i%3)*258,i<3?257:518,244,239);
  const std::string id=bundle?"currency-bundle":"currency-pack"+std::to_string(i);
  const auto v=buttonVisual(id,r);const float scale=v.w/(bundle?274.f:244.f);
  const auto local=[&](float x,float y,float w,float h){return Rect{v.x+x*scale,v.y+y*scale,w*scale,h*scale};};
  if(uiProject()&&!bundle){
   const auto buy=[this,i]{showFunds({});currencyOfferTitle_=offers[i].title;currencyOfferDetail_=offers[i].detail;};
   UiBindings bindings;bindings.values={{"title",offer.title},{"amount",offer.amount},{"art",offer.asset},{"actionLabel",offer.price},{"shopIcon",""}};
   drawUi("shop-card",coins?"coins":"pearls",v,bindings,{{"primary",{id+"-price",{},buy}}});
   const char* ribbon=i%3==1?"POPULAR":i%3==2?"BEST VALUE":"";
   if(*ribbon){auto badge=local(47,-15,151,35);canvas_.skin("ribbon",badge);canvas_.label(ribbon,badge.x+badge.w*.5f,badge.y+8*scale,20*scale,true,badge.w-9*scale,theme::white);}
   touchTarget(id,r,buy);continue;
  }
  canvas_.skin(bundle?"bundle-card":coins?"coin-card":"pearl-card",v);
  canvas_.label(offer.title,v.x+v.w*.5f,v.y+22*scale,(bundle?30.f:25.f)*scale,true,v.w-20*scale,theme::ink);
  const char* ribbon=bundle?"ONE TIME":i%3==1?"POPULAR":i%3==2?"BEST VALUE":"";
  if(*ribbon){
   const Rect badge=bundle?local(49,64,176,37):local(47,-15,151,35);
   canvas_.skin("ribbon",badge);canvas_.label(ribbon,badge.x+badge.w*.5f,badge.y+8*scale,20*scale,true,badge.w-9*scale,theme::white);
  }
  if(bundle){
   canvas_.icon(offer.asset,local(11,109,252,226));
   canvas_.icon("skin/icon-coin.png",local(44,342,44,44));canvas_.label("25,000",v.x+160*scale,v.y+350*scale,31*scale,true,156*scale,theme::ink);
   canvas_.icon("skin/icon-pearl.png",local(44,391,44,44));canvas_.label("30",v.x+160*scale,v.y+398*scale,31*scale,true,156*scale,theme::ink);
  }else{
   canvas_.icon(offer.asset,local(27,57,190,101));
   canvas_.label(offer.amount,v.x+v.w*.5f,v.y+160*scale,31*scale,true,v.w-25*scale,theme::ink);
  }
  const auto buy=bundle?local(16,444,242,47):local(16,195,212,44);
  glassButton(id+"-price",buy,offer.price,[]{},"green",31*scale,"",1,false);
  touchTarget(id,r,[this,i]{showFunds({});currencyOfferTitle_=offers[i].title;currencyOfferDetail_=offers[i].detail;});
 }
 const auto earn=l.rect(326,780,1048,79);canvas_.skin("card",earn);
 canvas_.icon("currency-shop/coins-small.png",l.rect(405,800,95,50));
 const auto video=l.rect(346,790,69,58);canvas_.skin("blue",video);canvas_.symbol("next",{video.x+18*u,video.y+14*u,34*u,31*u});
 canvas_.label("Earn Coins",l.x+521*u,l.y+788*u,31*u,false,400*u,theme::ink);
 canvas_.text("Complete goals to earn free coins!",l.x+521*u,l.y+824*u,21*u,theme::body,false,500*u,true);
 // The native game has quest rewards. An unconnected ad purchase is never implied.
 glassButton("currency-earn",l.rect(1113,793,244,57),"VIEW GOALS",[this]{open(Panel::Quests);},"blue",26*u);
}
}
