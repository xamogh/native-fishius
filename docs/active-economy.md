# Active-play economy

The user chose frequent selling and faster levels on 18 September 2026. The
importer records the following tuning in `overrides.active_play`. These values
are implementation choices for that direction. The workbook stays intact, with
its original inputs and sheets retained in generated content.

## Fish prices and XP

New fish use a base of 576 XP per productive fish-day and a 15 percent increase
per purchase level above level 1. Schedule and species factors still apply.
Coin income, coin egg prices, growth times, hunger and staged reward shares keep
their existing rules. The level thresholds and level grants stay unchanged.

| Example | Before | Active-play tuning |
| --- | ---: | ---: |
| Level-1 adult Neon Tetra | 2 XP | 10 XP |
| Level-1 Junior Neon Tetra | 0 XP | 1 XP |
| Level-10 adult Guppy | 11 XP | 147 XP |
| Level-39 adult Guppy | 14 XP | 418 XP |
| Forty level-10 adult Guppies | 440 XP | 5,880 XP |

Level 10 to 11 needs 3,790 XP. Level 39 to 40 needs 60,700 XP, covered by four
batches of forty newly purchased adult Guppies at level 39. These comparisons
assume every fish is sold as an adult and receives enough food to grow.

Bubble Eye stays at one pearl. Other pearl egg prices are the original price
divided by six, rounded up. Current launch prices are:

| Fish | Pearls |
| --- | ---: |
| Bubble Eye Goldfish | 1 |
| Koi | 3 |
| Platinum Betta | 4 |
| Mandarin Dragonet | 4 |
| Axolotl | 5 |
| Flashlight Fish | 6 |
| Golden Arowana | 8 |
| Peppermint Angelfish | 9 |
| Black Diamond Stingray | 12 |
| Platinum Arowana | 14 |

## Renewable pearls

Every twenty eligible adult coin-fish sales grant one pearl automatically.
Progress is shared across tanks, has no daily limit or reset, and continues at
level 40. Rewards shows the current count. An earned pearl flies from the sold
fish to the pearl counter, with immediate credit and a highlight under reduced
motion.

Gifted starter coin fish count. Early sales, pearl fish, scripted fish and
historical adults with zero production rewards do not count. Favorites remain
protected from selling. Buying fish, feeding and replaying a completed sale do
not advance the counter. Pearl fish never refund pearls on sale.

The new count starts at zero in old saves. Completed sales are not paid again.
Fish already owned retain their original coin and XP purchase quotes, but
eligible future adult sales of those fish count toward the pearl reward.

## Capacity

New tanks keep their level gates and ownership order. Prices are listed in
[Tank upgrades](tank-upgrades.md). Tank 1's first two five-space upgrades now
cost 300 and 900 coins. Reaching forty spaces costs 16,800 coins in total,
compared with 150,300 before this balance pass. Both currencies remain payment
alternatives. Earlier purchases are preserved without refunds.

## Validation and limits

`economy_balance` exercises the real initial wallet, starter fish, feeding,
growth, purchases and sales. With two longer-schedule starters kept, the first
batch reaches level 2 in twenty minutes. Buying the first capacity upgrade and
raising the next batch reaches level 3 and earns the first repeatable pearl by
forty minutes. No extra money, XP, capacity or growth is injected in this case.

`tools/simulate_active_economy.py` runs a separate deterministic cash-flow model.
It starts from the real wallet, grows twenty-minute Neon Tetra batches with
timely feeding, buys affordable capacity with coins while reserving egg money,
and holds two or ten fish. It does not buy decor or spend pearls. Hours below
mean accumulated active-cycle time, not calendar days or a retention forecast.

| Scenario | Level 10 | Level 20 | Level 40 |
| --- | ---: | ---: | ---: |
| One tank, eventually 40 spaces, 2 retained fish | 10.33 h | 36.33 h | 138.67 h |
| One tank, eventually 40 spaces, 10 retained fish | 16.33 h | 50.33 h | 179.67 h |
| Up to 5 tanks, capacity bought with coins, 10 retained fish | 12.67 h | 30.33 h | 61.00 h |

All three modeled scenarios reach level 40 without running out of coins. They
do not cover every spending choice, missed feeding, interaction time, longer
schedules or device performance. The full model is separate from runtime
transaction tests and does not exercise save record limits. Actual playtesting
is still needed before treating these constants as final balance.

The domain tests cover repeated milestones, the XP cap, reloads, same and
different request retries, historical settlements, failed saves, overflow and
ineligible fish. Reward presentation tests cover pearl arrival and reduced
motion. The independent content audit checks generated quotes and prices
against exact rational arithmetic and the explicit active-play overrides.

The desktop build, all 24 C++ suites and all three import tests passed in this
pass. Two native rendering suites required SDL's headless video driver in this
environment. The catalog audit passed 2,996 checks. Rewards was also rendered
at phone, tablet, desktop, portrait and notched screen sizes.
