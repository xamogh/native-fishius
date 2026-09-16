# Egg placement

Current economy and lifecycle rules are in [v4 implementation, steps 1 to 3](workbook-implementation.md). That record supersedes older prices, purchase XP, care penalties, capacity and save compatibility statements below.

The native game follows the local FishX Phaser interaction in `/Users/amoghrijal/fishx/src/ui/ui.ts`, functions `setupEggPlacement`, `startPlacing` and `stopPlacing`. Water bounds follow `clampToWater` in `/Users/amoghrijal/fishx/src/game/tank.ts`.

Choosing a fish in Shop selects its species and closes Shop. It does not charge currency, grant XP, claim a gift or create an egg. An egg follows the mouse pointer. A hint shows the species and cost per egg, with a Done button.

Each primary press buys and places one egg. The purchase, XP, claims, fish identity and six-second hatch timer are saved together. Placement continues after successful purchases. Done, Escape or another tool ends placement. A failed purchase also ends placement without changing the save. One-time and annual claims end after one successful placement.

The placement layer owns taps over the aquarium and its navigation. It clamps drop points to the water, from 0 to 1088 horizontally and 0 to 512 vertically. Done owns its press and never buys an egg beneath the button. A 24-point dead zone around the Shop activation prevents accidental purchases from double taps. Right-clicks, secondary fingers and synthetic mouse events do not buy extra eggs.

There is no new paid egg queue or resume button. Loading an older save converts `pendingEggs` into paused inventory eggs in Bag under Fish. Their previous payment and XP remain unchanged. Stored eggs do not count toward tank capacity and can be placed later without paying again. Saving the recovered state removes the old field, so loading again cannot duplicate the eggs.

Regression coverage includes repeated mouse and touch presses, free arming and cancellation, the arming dead zone, pointer and button ownership, coin and pearl costs, full tanks, one-time and annual gifts, hatching, save round trips and recovery through Bag. Native screenshots are saved under `evidence/egg-placement`.

Validation on 12 September 2026: the desktop build passed. The full run passed 15 of 17 suites. After correcting the new test's arming setup and rebuilding the shared level-up changes, all five targeted suites passed. Together these runs cover all 17 desktop suites. The final egg placement tests passed at 667x375, 852x393 and 1024x768, and the phone and tablet captures were visually reviewed. Logs are retained in `evidence/egg-placement`.
