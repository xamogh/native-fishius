# Economy audit, 18 September 2026

The build before this balance pass could not fund several pearl fish through normal play. XP was
earnable, but short fish schedules can require many sales and frequent feeding.
The prices and level curve need to be evaluated together against the rules that
are actually available to players.

This records the findings before the changes. The user then chose active play,
frequent selling and faster levels. The implemented changes and validation are
in [Active-play economy](active-economy.md). All figures below refer to the prior
configuration, `v4-fed-1-ac014fb342bf`.

## Pearl income cannot cover the catalog

Players start with zero pearls. Level rewards from levels 2 through 40 grant
**40 pearls in total**. The generated supplement is empty, and quests, mastery,
projects, events, purchases and ads are disabled. Normal fish sales grant no
pearls. There is no renewable pearl source in the current game.

These are best-case balances that assume the player spends no pearls:

| Fish | Unlock level | Pearl price | Pearls earned by unlock | First affordable level |
| --- | ---: | ---: | ---: | --- |
| Bubble Eye Goldfish | 2 | 1 | 1 | 2 |
| Koi | 5 | 18 | 5 | 20 |
| Platinum Betta | 8 | 24 | 7 | 25 |
| Mandarin Dragonet | 10 | 24 | 10 | 25 |
| Axolotl | 12 | 30 | 11 | 30 |
| Flashlight Fish | 16 | 35 | 16 | 35 |
| Golden Arowana | 20 | 45 | 20 | Never |
| Peppermint Angelfish | 25 | 50 | 25 | Never |
| Black Diamond Stingray | 35 | 70 | 35 | Never |
| Platinum Arowana | 40 | 80 | 40 | Never |

One of each of these ten fish costs **377 pearls**. Pearl decor and capacity
upgrades compete for the same 40 earnable pearls.

The workbook's Pearl Budget assumes weekly rewards, collection chapters and
events. Those sources are absent from the running game. Its pearl forecasts
therefore do not describe the current player experience.

There is a second mismatch. These prices came from permanent collectible fish.
All pearl fish now follow the ordinary two-hour growth and sale cycle, with
one-hour meals. Only Bubble Eye's price was changed. Selling a pearl fish removes
it and grants ordinary coins and XP, with no pearl refund. At level 40, a newly
bought Platinum Arowana costs 80 pearls and sells as an adult for 107 coins and
14 XP before the account XP cap is applied. Its price buys its appearance, not a
better production rate.

## Forty spaces do not guarantee steady XP

Fish grant XP when sold. Feeding, buying fish, growth, retaining adults and
favoriting do not grant XP. The first purchase of each decor type also grants
XP, but that is a finite source. A full tank of retained adults produces no
further XP.

Examples below use 40 newly purchased fish at the stated account level, all
sold as adults. The XP quote stays fixed at purchase.

| Level transition | XP needed | Fish | XP per adult | XP per 40-fish batch | Batches needed |
| --- | ---: | --- | ---: | ---: | ---: |
| 1 to 2 | 80 | Neon Tetra | 2 | 80 | 1 |
| 5 to 6 | 830 | Guppy | 11 | 440 | 2 |
| 10 to 11 | 3,790 | Guppy | 11 | 440 | 9 |
| 20 to 21 | 11,480 | Guppy | 12 | 480 | 24 |
| 39 to 40 | 60,700 | Guppy | 14 | 560 | 109 |

Forty spaces in the first row are a hypothetical comparison. A new game starts
with ten spaces and four gifted fish, including only two Neon Tetras.

Growth time means fed time. A two-hour Guppy gets one hour of growth from each
meal. With two check-ins spaced twelve hours apart, a new batch takes about one
day to reach an adult sale, assuming the player feeds it just after hatching.
The level 10 example then takes nine days if the player uses only Guppies and
earns no other XP. Keeping ten of the forty fish reduces sale capacity to thirty
and raises that example to twelve batches.

Longer schedules are substantially better for infrequent visits. At level 10,
40 new Mollies give 7,040 XP after four twelve-hour feeding intervals, enough for
the next level in two days. At level 39, 40 new Longhorn Cowfish give 9,840 XP per
two-day batch. Seven batches, or fourteen days under that schedule, cover the
60,700 XP gap. These are illustrative batch calculations, not a complete
progression simulation. They assume the player already owns the spaces, can
afford every egg, visits consistently and sells every fish.

Early sales can feel especially unrewarding. A level-1 Neon Tetra gives 2 XP as
an adult. Its Junior and Young rewards both round down to zero; Mature gives
1 XP. A simple minimum of 1 XP at Junior would also change the fastest XP
strategy, so this needs rate testing rather than a display-only correction.

The level curve uses reference capacity that rises from 10 to 100 spaces across
five tanks, plus daily and weekly quest rewards in the source design. Current
care pauses, shared capacity, retained fish, unavailable quests and changed
tank upgrades all affect actual earnings. The workbook's older simulation
explicitly requires a rerun for shared capacity and favorites.

## Capacity has a sharp price jump

Tank 1's first five extra spaces cost 300 coins. Its next five cost 15,000 coins,
a 50-fold price increase for the same amount of space. All six upgrades from
10 to 40 spaces cost 150,300 coins, or 154 pearls if every step uses pearls.
The pearl route costs nearly four times the game's total earned pearl supply.

## Initial recommendations

The initial proposal used two daily check-ins as a baseline. The user selected
active play instead. Test some fish retained as favorites and missed visits. Keep hunger pauses and
shared capacity in the model because those are current design decisions.

1. Define and implement a renewable pearl reward that works through an available
   player action and has a visible progress and claim path. Set fish prices
   against measured days of earning after ordinary spending. Raising only the
   finite level grants would leave the same problem after level 40.
2. Reprice pearl eggs for the current ownership and sale rules. Test the ability
   to buy an early pearl fish near its unlock and to replace a sold pearl fish.
3. Tune the XP curve or sale rewards around actual feeding and sale intervals,
   with 20 to 40 spaces and a reserve for favorites. Include early-sale rounding,
   first-session progression and the value of longer schedules.
4. Smooth the capacity prices and include the cost of eggs, decor and upgrades
   in a full cash-flow simulation. Do not assume unlocked space has been bought.
5. Check both new and existing saves. Fish already owned have immutable purchase
   quotes. Changing future quotes will not immediately improve their sale XP;
   changing level thresholds requires explicit handling of prior level grants.

The balance pass should report days to each level, days to each pearl purchase,
productive and retained fish counts, required check-ins, and the lowest coin
balance. Exact replacement values should come from those scenarios.

## Evidence and verification

- `assets/content.json`: economy inputs, species, level thresholds, level grants,
  capacity prices, feature flags and retained workbook sheets.
- `tools/import_v4.py`: unchanged collectible prices except Bubble Eye, the
  shared two-hour pearl schedule, disabled features and empty supplement.
- `src/economy.cpp`: purchase snapshots, staged sale rewards and no pearl sale
  payout.
- `src/domain.cpp`: fed growth, XP and level grants, and unavailable rewards.
- `docs/assumptions.md` and `docs/tank-upgrades.md`: current decisions and known
  pricing assumptions.

Rebuilt the current desktop domain libraries and ran
`v4_lifecycle_and_transactions` and `level_rewards`. Both passed. They verify
the current rules and transaction safety; they do not establish that ordinary
players can afford the catalog or that progression feels rewarding.
