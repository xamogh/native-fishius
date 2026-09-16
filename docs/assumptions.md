# Current design decisions

The active catalog source is `design/aquarium_game_design_v4.xlsx`. User decisions in this task override conflicting workbook instructions. The complete step 1 to 3 record is in `workbook-implementation.md`.

The user chose a local transaction foundation, free feeding that pauses hungry fish growth, meals lasting half the total species schedule capped at 12 hours, and the existing six-second egg hatch. There is no backward compatibility because the game has no users.

The six-second hatch counts toward total growth and ends with a hungry Baby. Subsequent stage thresholds use the total schedule. Cancelling an egg or rehoming a Baby before Junior returns paid principal only and no XP. Gifted starters have zero principal. These implementation choices are recorded in the generated configuration.

Keep never requires the player to give up an earned adult because the display area is full. It pays once, frees the growing slot and stores the companion in Bag when the tank's eight display slots are occupied. Display fish can be stored and restored for free. They never regain a productive cycle.

The workbook's rounding rule takes precedence over inconsistent cached cells: positive half ties round up, and partial payouts use FLOOR on exact integer percentages. Original values remain in `cached_*` fields and the independent audit lists each correction.

Release status and available artwork both gate catalog visibility. Proposed event dates do not become live schedules. Deferred quests, mastery, projects, events and store grants do not mint currency. The new scripted onboarding remains outside these first three steps; the development game starts with its free starter fish.

Historical reports describe earlier builds. They do not override these current decisions or certify current mobile builds.
