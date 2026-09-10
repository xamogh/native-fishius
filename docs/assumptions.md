# Source conflicts and implementation assumptions

## Selected sources

The current written behavior contract controls movement, care, transactions, initial state, and explicit tank prices. The supplied workbook controls remaining numerical content. The four supplied screenshots control appearance. No comparison with an original running Phaser application is claimed.

## Explicit conflicts

The pictured shop has six cards in one row. The written contract requires eight in a four-column, two-row arrangement. The implementation follows the written requirement.

A reference labels the third tank as Level 15. The explicit tank progression requires Level 16. The implementation uses 16.

The active workbook inspected in the earlier content step assigns Neon Tetra a seven-coin, 0.083-hour schedule and Guppy a 35-coin, four-hour schedule. Static notes or values from an older catalog are not substituted. The starter list remains the explicit Guppy, Ember Tetra, Platy, and Molly list.

The explicit tank price table overrides the workbook's different tank schedule. Levels above 40 and later tanks remain future content.

## Tutorial arithmetic

The previously inspected tutorial rewards total 77 XP, short of the 80-XP Level 2 threshold. The intended resolution is an additional real earning action, not an unexplained three-XP grant. The exact reward-row mapping still requires independent verification against the retained workbook audit. The importer must not pass an unrecognized reward column off as verified data.

## Local calendar

Recurring local claims use UTC day boundaries. Weekly periods start on Monday. Calendar anchors do not move backward. Event windows are inclusive and may wrap year end. The implemented event-window defaults and anniversary date need confirmation against any explicit workbook dates; defaults are not a claim that those dates came from source cells.

## Configurable local additions

Where no numeric specification was identified, the local implementation supplies decor prices and scores, an NPC daily Gift Token, and a daily egg exchange policy. These are implementation assumptions, not spreadsheet values. The current decor defaults are seaweed 25 coins/5 score, coral 60/12, shell 40/8, arch 100/20, and chest 150/30. A mastery threshold of five sales gives a cosmetic badge, not invented currency.

Unresolved quest reward mappings are not fabricated. The UI may reject their claims as unavailable until exact source values have been mapped and verified. This is an outstanding completion gap, not a completed quest system.

## Artwork and fonts

Separate assets were reused from supplied image material when identifiable. Other species use distinct local interpretations intended to form a coherent set. They are not asserted to match unseen original art. A generated aquarium backdrop is supporting artwork, not evidence of exact screenshot fidelity.

Font binaries are excluded from this delivery. A build-machine setup script installs the intended fonts and license records. System-font fallback changes the visual result.
