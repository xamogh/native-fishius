# Source decisions and remaining assumptions

## Source of truth

`design/aquarium_game_master_model.xlsx` controls the launch fish roster, economy, care schedules, XP curve, starting fish, tank schedules, dated events, quest definitions and mastery targets. `assets/content.json` records the imported values and their source cells. The screenshots guide appearance; they do not override workbook progression or fish identity.

The independent check in `evidence/workbook-catalog-check.log` passed 1,672 comparisons: 1,439 explicit values and 233 derived values. It reports no mismatches. This verifies the mapped workbook values, not details the workbook leaves unspecified.

## Resolved conflicts

- New games start with Neon Tetra, Guppy, Platy and Molly, as listed in `Unlock Plan!C4`. Ember Tetra remains a Level 4 shop fish.
- Tank unlock levels and costs now come from `Tanks & Social`, replacing the earlier local price table. The launch tank levels are 1, 7, 16, 25 and 34.
- Bubble Eye Goldfish is repeatable. The detailed note in `Premium Fish!AA2` takes precedence over the stale one-time summaries listed in the import audit.
- Mastery records fish reaching Adult, with targets of 5, 25 and 100 per species. Sales remain separate historical counters.
- The Shop shows the real roster in catalog group and unlock order. A screenshot's card order does not override progression.

## Tank capacity interpretation

The solo path requires the listed coins **plus** Gift Tokens. The friend-assist alternative is not connected, and there is no invented Pearl conversion. Existing token balances remain valid, but a new token grant has not been invented to make this path easier.

The workbook supplies four capacity and cost entries per tank. It also names Medium and Large unlock beats without clearly mapping them to every capacity step. Those beats are retained in the import as unconfirmed metadata. The runtime enforces tank unlock levels, prior tank ownership, capacity limits and costs; it does not invent missing per-expansion level gates.

## Calendar rules

Daily periods use UTC day boundaries; weekly periods start on Monday. Calendar anchors never move backward. Seasonal date ranges include both endpoints and can cross the end of the year.

Nine limited fish use explicit dates from `Limited Edition`. Anniversary Rainbowfish says one week each year but provides no dates. Its calendar remains unconfigured and its claim unavailable. The user deferred that decision.

## Deferred content

The user deferred tutorial work and rewards that lack enough source detail. The current pass does not complete or rebalance the tutorial. The Start guide entry has been removed from Settings Help. Existing unused tutorial code and saved state are retained for compatibility.

Daily quests grant their exact mapped XP. The estimated daily coin pool is retained as source data but is not copied into every quest or split without instructions. Other deferred details are:

- Collection chapter themes, species membership, and Pearl or event egg rewards.
- Neighbor Helper coin and Gift Token rewards.
- Mastery XP amounts and statue specifications.
- Decorator thresholds, titles and background rewards.
- Daily Egg Basket fish tables, probabilities, retired fish pool and resale rules.

The old invented NPC token and daily egg grants have been removed. Gift and egg actions remain unavailable until their rules are configured. Mastery badges record earned milestones without invented currency rewards.

## Local decor values

Individual decor prices and scores are not specified in the workbook. These existing local values remain explicit assumptions:

| Decor | Coins | Score |
|---|---:|---:|
| Seaweed garden | 25 | 5 |
| Sunset coral | 60 | 12 |
| Pearl shell | 40 | 8 |
| Little stone arch | 100 | 20 |
| Treasure chest | 150 | 30 |

These values should be replaced when source values are supplied. They are not part of the claim of exact workbook parity.

## Artwork and fonts

Fish artwork is an interpretation in the supplied game's visual style, not a claim to reproduce unseen original species art. Each species has a canonical asset in `assets/species`, with its source and generation notes in `assets/species/provenance`. Runtime derivatives keep the source PNGs intact.

The workspace contains the intended font binaries and license records under `assets/fonts`. System-font fallback can change appearance. Archive tooling may exclude font binaries, so a source archive and the working app are different deliverables.
