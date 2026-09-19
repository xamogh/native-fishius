#pragma once
#include "aquarium/dialog_motion.hpp"

namespace aq {
enum class DialogSize { Small,Medium,Large,Custom };
struct DialogSpec {
 std::string title;
 DialogSize size{DialogSize::Small};
 float customWidth{960},customHeight{640};
};
struct HudDialogLayout {
 Rect backdrop,frame,header,title,close,body,content;
 float unit{};
};
struct DialogState { bool open{true},closePressed{},backdropPressed{};DialogMotion motion{}; };
enum class DialogPresentation {Modal,Popover,ModalContent};
// Custom dimensions are design units, rounded up to the four-unit grid.
// Content is the padded area where callers can place their own Clay layout.
HudDialogLayout layoutDialog(float width,float height,Insets safe,const DialogSpec&);
void paintDialog(Canvas&,const HudDialogLayout&,const DialogSpec&,bool closeHovered=false,bool closePressed=false,DialogPresentation=DialogPresentation::Modal);
// Consumes modal input, closes on X, Escape or a backdrop click, and blocks input to the screen below.
bool dialogEvent(DialogState&,const HudDialogLayout&,const SDL_Event&,SDL_FPoint canvasPoint={});
}
