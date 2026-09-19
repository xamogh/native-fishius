# Current design decisions

The active catalog source is `design/aquarium_game_design_v4.xlsx`. User decisions in this task override conflicting workbook instructions. The complete step 1 to 3 record is in `workbook-implementation.md`.

The latest 18 September direction is active play with frequent selling and faster levels. The importer now records explicit overrides for stronger fish XP, cheaper pearl eggs, smoother capacity prices and one renewable pearl per twenty eligible adult coin-fish sales. These override the earlier price assumptions below. Existing fish keep their saved quotes, existing levels and purchases remain intact, and historical sales receive no new grant. See [Active-play economy](active-economy.md) for the exact implementation choices and test scenarios.

The 18 September decor XP decision grants a first-purchase bonus for each plant or decoration type: one XP per 10 coins, rounded half up with a minimum of one, or 10 XP per pearl. Lifetime ownership prevents repeat rewards. Existing saves retain their ownership and receive no retroactive XP. This overrides the workbook's zero decor XP without changing fish purchase rewards.

The user chose a local transaction foundation, free feeding that pauses hungry fish growth, meals lasting half the total species schedule capped at 12 hours, and the existing six-second egg hatch. Earlier save schemas are unsupported; existing v4 fish and rewards are preserved.

The six-second hatch counts toward total growth and ends with a hungry Baby. Subsequent stage thresholds use the total schedule. Eggs and Babies cannot be sold; selling starts at Junior. Historical egg and Baby refund receipts remain valid, but no new refunds are allowed. Gifted starters have zero principal. These implementation choices are recorded in the generated configuration.

The 18 September decision removes the Keep action and the separate growing and display capacities. A fish stays in its tank until sold. Favoriting protects it from selling and gives no reward. All eggs and fish share the tank's capacity. Older v4 saves preserve paid adult rewards and ownership; excess adults move into free storage when the old combined occupancy exceeds the shared capacity.

The later fish decision removes the companion mechanism. Bubble Eye Goldfish costs one pearl at level 2 and permits repeat purchases. All 27 former companion species now use eggs, feeding, growth and coin/XP sales. Their growth uses the existing balanced two-hour schedule, with one-hour meals and no rarity multiplier. This schedule is an implementation assumption for later balance testing. Other pearl prices and release gates are unchanged. Pearl purchases have zero refundable coin principal and never refund pearls on sale. The importer records these overrides while preserving the workbook source rows.

Older v4 adult-only records become ordinary adult fish, preserving favorites, positions and storage. Their first sale quote uses the saved account level at conversion and is then frozen. Adults already paid by the retired Keep action retain their historical receipts and receive new fish identities with zero sale rewards, so they can be removed by selling without a second payout. No wallet or XP reward is granted during conversion.

The later tank decision keeps level gates only for new tanks. Owned tanks expand from 10 to 40 fish in steps of five, paid with coins or pearls. Existing 15- and 20-fish prices remain; the four extra steps cost 1.5, 2, 2.5 and 3 times the 20-fish upgrade, rounded half up. These new prices are an implementation assumption that needs playtesting. The importer records the rules as explicit overrides of the original workbook rows. See [tank upgrades](tank-upgrades.md).

The workbook's rounding rule takes precedence over inconsistent cached cells: positive half ties round up, and partial payouts use FLOOR on exact integer percentages. Original values remain in `cached_*` fields and the independent audit lists each correction.

Release status and available artwork both gate catalog visibility. Proposed event dates do not become live schedules. Deferred quests, mastery, projects, events and store grants do not mint currency. The new scripted onboarding remains outside these first three steps; the development game starts with its free starter fish.

Historical reports describe earlier builds. They do not override these current decisions or certify current mobile builds.
