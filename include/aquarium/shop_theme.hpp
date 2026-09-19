#pragma once
#include "aquarium/hud.hpp"

namespace aq::shopTheme {
inline constexpr Color ink{20,74,84,255};
inline constexpr Color white{250,255,248,255};
inline constexpr Color fishInk{126,91,53,255};
inline constexpr Color dialogPaper{235,235,222,255},dialogEdge{8,36,67,255};
enum class Surface {Tab,SelectedTab,Positive,Button,Well,Card,LockedCard,Close,FishCard,LockedFishCard,FastBadge,Buy,PearlBuy,LockedPrice,DisabledButton,PausedBadge,SettingsCard,Amber};
void panel(Canvas&,Rect,float unit,Surface,bool pressed=false);
void badge(Canvas&,Rect,float unit,std::string_view label,Surface style=Surface::FastBadge);
void lockIcon(Canvas&,Rect);
void clockIcon(Canvas&,Rect,Color clockInk=fishInk);
void blueHeader(Canvas&,Rect,float unit,float radius=0);
void backdrop(Canvas&,const ShopLayout&);
void footer(Canvas&,const ShopLayout&);
void card(Canvas&,Rect,float unit,bool locked);
void pricePanel(Canvas&,Rect,float unit,bool pressed=false);
void scrollbar(Canvas&,Rect track,Rect thumb,float unit);
void tabArt(Canvas&,Rect,int category);
}
