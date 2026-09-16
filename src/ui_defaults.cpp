#include "aquarium/ui_project.hpp"
namespace aq {
namespace {
UiNode makeNode(std::string id,std::string name,std::string type,DesignBox box,std::string style="body"){UiNode n;n.id=std::move(id);n.name=std::move(name);n.type=std::move(type);n.box=box;n.style=std::move(style);return n;}
UiNode image(std::string id,std::string name,DesignBox box,std::string asset){auto n=makeNode(id,name,"image",box);n.asset=std::move(asset);return n;}
UiNode text(std::string id,std::string name,DesignBox box,std::string value,std::string style){auto n=makeNode(id,name,"text",box,style);n.text=std::move(value);return n;}
UiNode instance(std::string id,std::string name,DesignBox box,std::string component){auto n=makeNode(id,name,"instance",box);n.component=std::move(component);return n;}
UiNode button(std::string id,std::string name,DesignBox box,std::string asset,std::string action,std::string value,std::string style){auto n=makeNode(id,name,"button",box,style);n.asset=std::move(asset);n.action=std::move(action);n.text=std::move(value);return n;}
}
UiProject UiProject::defaults(const DialogDesign& legacy){
 UiProject p;
 UiStyle body;body.size=38;body.stretch=.88f;body.color={76,130,158,255};p.styles["body"]=body;
 UiStyle heading;heading.size=68;heading.stretch=.89f;p.styles["heading"]=heading;
 UiStyle primary;primary.size=68;primary.color={255,255,245,255};primary.outlineWidth=2.5f;p.styles["primary"]=primary;
 UiStyle amount;amount.size=51;amount.accentSize=68;amount.stretch=.94f;p.styles["currency"]=amount;
 UiStyle extra;extra.size=32;p.styles["extra"]=extra;
 UiStyle card;card.font="lilita";card.size=25;p.styles["card-heading"]=card;card.size=31;p.styles["card-amount"]=card;
 card.color={255,255,255,255};card.outlineWidth=1;p.styles["price"]=card;
 UiStyle tank; tank.font="lilita";tank.size=38;p.styles["tank-heading"]=tank;tank.size=47;tank.color={255,255,245,255};p.styles["tank-price"]=tank;
 UiStyle hud;hud.font="lilita";hud.size=29;hud.color={255,255,255,255};p.styles["hud"]=hud;
 for(int i=1;i<=5;++i){const auto part=static_cast<DialogPart>(i);if(part==DialogPart::Illustration)continue;const auto& e=legacy.element(part);const char* name=part==DialogPart::Title?"heading":part==DialogPart::Message?"currency":part==DialogPart::Extra?"extra":"body";
  auto& s=p.styles[name];s.font=e.font;s.size=e.fontSize;s.color=e.color;s.stretch=e.stretch;
 }
 p.styles["currency"].accentSize=legacy.emphasisSize;p.styles["currency"].accent=legacy.emphasisColor;
 auto frame=image("root","Illustrated frame",{0,0,888,732},legacy.element(DialogPart::Frame).asset);p.components["dialog-frame"]=frame;
 auto header=makeNode("root","Dialog header","group",{0,0,545,100});header.children={text("label","Title",{0,0,545,100},"{title}","heading")};p.components["dialog-header"]=header;
 auto wood=makeNode("root","Wooden header","group",{0,0,560,140});wood.children={image("wood","Wood",{0,0,560,140},"lagoon/sign.png"),text("label","Title",{24,20,512,95},"{title}","heading")};p.components["wooden-header"]=wood;
 auto rich=makeNode("root","Currency amount","richtext",{0,0,674,86},"currency");rich.text="{message}";p.components["currency-text"]=rich;
 auto primaryButton=button("root","Primary button",{0,0,652,135},legacy.element(DialogPart::Action).asset,"primary","","primary");
 auto icon=image("icon","Shop icon",{126,17,96,98},"skin/icon-shop.png");icon.visibleWhen="shopIcon";
 auto label=text("label","Button label",{237,21,382,94},"{actionLabel}","primary");label.layout.horizontal="center-alone";primaryButton.children={icon,label};p.components["primary-button"]=primaryButton;
 auto close=button("root","Close button",{0,0,100,100},legacy.element(DialogPart::Close).asset,"close","","body");p.components["close-button"]=close;
 auto dialog=makeNode("root","General dialog","group",{0,0,888,732});
 dialog.children={instance("frame","Frame",legacy.element(DialogPart::Frame).box,"dialog-frame"),instance("title","Title",legacy.element(DialogPart::Title).box,"dialog-header"),image("illustration","Illustration",legacy.element(DialogPart::Illustration).box,"{illustration}"),instance("message","Message",legacy.element(DialogPart::Message).box,"currency-text"),text("extra","Requirement",legacy.element(DialogPart::Extra).box,"{extra}","extra"),text("detail","Supporting text",legacy.element(DialogPart::Detail).box,"{detail}","body"),instance("primary","Primary button",legacy.element(DialogPart::Action).box,"primary-button"),instance("close","Close",legacy.element(DialogPart::Close).box,"close-button")};
 dialog.children[4].visibleWhen="extra";
 dialog.children[4].layout.height="hug";dialog.children[4].layout.wrap=true;
 dialog.children[5].layout.after="extra";dialog.children[5].layout.gap=1;
 dialog.children[5].layout.wrap=true;dialog.children[5].layout.height="hug";
 p.components["dialog"]=dialog;
 UiScreen screen;screen.id="general-dialog";screen.name="General dialog";screen.root=instance("dialog","General dialog",{0,0,888,732},"dialog");screen.maxWidth=legacy.maxWidth;screen.safeFraction=legacy.safeFraction;
 for(bool pearl:{false,true}){const auto& v=pearl?legacy.pearls:legacy.coins;UiVariant variant;variant.values={{"title",v.title},{"illustration",v.illustration},{"message",v.prefix+"{amount}"+v.pluralSuffix},{"singularMessage",v.prefix+"{amount}"+v.singularSuffix},{"detail",v.detail},{"actionLabel",v.actionLabel},{"shopIcon","1"},{"extra",""}};
  screen.variants[pearl?"pearls":"coins"]=variant;
 }p.screens.push_back(screen);
 auto shop=makeNode("root","Shop card","group",{0,0,244,239});shop.children={image("background","Card",{0,0,244,239},"{cardArt}"),text("title","Pack name",{10,22,224,42},"{title}","card-heading"),image("art","Pack illustration",{27,57,190,101},"{art}"),text("amount","Pack amount",{12,155,220,42},"{amount}","card-amount"),instance("buy","Price button",{16,195,212,44},"primary-button")};
 p.components["shop-card"]=shop;UiScreen shopScreen;shopScreen.id="shop-card";shopScreen.name="Shop card";shopScreen.root=instance("card","Shop card",{0,0,244,239},"shop-card");shopScreen.maxWidth=340;
 shopScreen.variants["coins"].values={{"title","Small Coin Pack"},{"amount","2,500"},{"art","currency-shop/coins-small.png"},{"cardArt","currency-shop/card.png"},{"actionLabel","$0.99"},{"shopIcon",""}};
 shopScreen.variants["pearls"].values={{"title","Small Pearl Pack"},{"amount","15"},{"art","currency-shop/pearls-small.png"},{"cardArt","currency-shop/pearl-card.png"},{"actionLabel","$1.99"},{"shopIcon",""}};
 // These changes belong to the card's button instance; its shared component
 // still provides the background, font, colors and pressed appearance.
 for(auto& [_,v]:shopScreen.variants)v.overrides["card/buy|@primary-button|label"]={{"box",DesignBox{25,5,602,123}},{"styleOverrides",{{"size",85},{"outlineWidth",2.5}}}};
 p.screens.push_back(shopScreen);
 auto upgrade=makeNode("root","Tank upgrade","group",{0,0,644,215});upgrade.children={image("background","Upgrade panel",{0,0,644,215},"tank-menu/upgrade.png"),text("title","Upgrade title",{16,7,612,50},"{title}","tank-heading"),text("capacity","Capacity",{18,55,608,47},"{capacity}","tank-heading"),button("coins","Coin purchase",{19,104,295,96},"tank-menu/coin-button.png","buyCoins","{coins}","tank-price"),button("pearls","Pearl purchase",{330,104,296,96},"tank-menu/pearl-button.png","buyPearls","{pearls}","tank-price")};
 for(int i=3;i<=4;++i){auto& b=upgrade.children[i];auto priceLabel=text(i==3?"coinLabel":"pearlLabel","Price",{89,22,180,65},b.text,"tank-price");if(i==4)priceLabel.styleOverrides={{"color",DesignColor{5,34,73,255}}};priceLabel.alertWhen=i==3?"coinsShort":"pearlsShort";b.text.clear();b.children.push_back(priceLabel);}

 p.components["tank-upgrade"]=upgrade;UiScreen ts;ts.id="tank-upgrade";ts.name="Tank upgrade";ts.root=instance("upgrade","Tank upgrade",{0,0,644,215},"tank-upgrade");ts.maxWidth=620;ts.variants["default"].values={{"title","Upgrade Capacity"},{"capacity","10 to 15 fish"},{"coins","300"},{"pearls","3"}};p.screens.push_back(ts);
 auto wallet=makeNode("root","Currency HUD","group",{0,0,395,77});
 for(bool pearl:{false,true}){float x=pearl?214.f:0.f,width=pearl?181.f:197.f;std::string id=pearl?"pearls":"coins";
  auto balance=button(id,pearl?"Pearl balance":"Coin balance",{x,16,width,54},"skin/nav.png",pearl?"pearlBalance":"coinBalance","","hud");balance.slice=24;
  auto amountLabel=text(id+"-amount","Balance",{48,7,width-110,42},"{"+id+"}","hud");balance.children.push_back(amountLabel);wallet.children.push_back(balance);
  wallet.children.push_back(image(id+"-icon",pearl?"Pearl":"Coin",{x-12,5,65,66},pearl?"skin/icon-pearl.png":"skin/icon-coin.png"));
  wallet.children.push_back(button(id+"-add","Add currency",{x+width-57,8,60,62},"skin/control-add.png",id,"","hud"));
 }
 p.components["currency-hud"]=wallet;UiScreen hs;hs.id="currency-hud";hs.name="Currency HUD";hs.root=instance("wallet","Wallet",{0,0,395,77},"currency-hud");hs.maxWidth=560;hs.variants["default"].values={{"coins","250"},{"pearls","0"}};p.screens.push_back(hs);
 return p;
}
}
