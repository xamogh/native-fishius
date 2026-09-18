#pragma once
#include "aquarium/canvas.hpp"
#include <functional>

namespace aq {
// Draw the main menu entry screens into the frame target to prepare their
// artwork, fonts and renderer state before the player can open them.
// Progress must redraw/present the loader, never the prepared menu frame.
// Return false from progress to cancel startup or handle an app shutdown.
bool prepareMenus(Canvas&,const Domain&,const std::function<bool(float,std::string_view)>& progress);
}
