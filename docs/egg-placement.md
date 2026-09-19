# Egg placement

The current controls are in `hud_placement.cpp`, with coverage in `hud_tests.cpp` and `hud_care_tests.cpp`.

Choosing a fish in Shop selects it without charging currency. Its egg follows the pointer, whether it costs coins or pearls. A completed primary tap in the water submits a purchase through Session using the saved quote. A drag, cancelled touch, menu tap or Done tap cannot buy a fish underneath a control.

Successful purchases add a cost receipt and keep placement active. When the tank is full, the next purchase attempt ends placement and opens the Tank full dialog with a Tank Shop button. Selecting a fish in an already full tank opens the same dialog. Currency shortages end placement and open the matching funds dialog. Other failed purchases show the domain or save error without charging. Done, Escape, Food or Rehome ends placement. Eggs hatch through the normal six-second domain timer.

The source of prices, growth times, rewards and eligibility remains the [v4 workbook implementation](workbook-implementation.md). The current Bag dialog has no restore controls; previously stored items remain in the save.
