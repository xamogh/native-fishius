#pragma once
#include "aquarium/hud_dialog.hpp"
#include "aquarium/hud.hpp"

namespace aq {
struct FundsDialogState {
 CurrencyShortfall shortfall;
 DialogState dialog{false};
 bool shopPressed{},dragged{};
 SDL_FPoint pressPoint{};
 bool open()const{return dialog.open;}
};
struct FundsDialogLayout {
 HudDialogLayout dialog;
 Rect illustration,message,shop,scene;
};
enum class FundsDialogEvent {Ignored,Handled,OpenShop};
bool showFundsDialog(FundsDialogState&,const Result&);
Currency fundsCurrency(const FundsDialogState&);
std::string fundsDialogMessage(const FundsDialogState&);
FundsDialogLayout layoutFundsDialog(float width,float height,Insets safe,float minimumTouch=44);
void paintFundsDialog(Canvas&,const FundsDialogLayout&,const FundsDialogState&,SDL_FPoint pointer={-1,-1},bool reducedMotion=false);
FundsDialogEvent fundsDialogEvent(FundsDialogState&,const FundsDialogLayout&,const SDL_Event&,SDL_FPoint);

// Closing Treasure returns to the catalog and scroll position that opened it.
struct FundsShopReturn {
 std::optional<ShopState> previous;
 void open(const FundsDialogState&,bool& shopOpen,ShopState&);
 void close(bool& shopOpen,ShopState&);
};
}
