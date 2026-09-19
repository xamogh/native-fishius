# Error toast designs

Created on 18 September 2026 with the built-in Image Gen tool. These are design mockups for the error shown when a player tries to sell a favourite fish.

Latest: [More creative revision with three new options](creative-v2/README.md). Use that set for the current design selection.

## Designs and prompts

The numbers follow the order in which the generated images appeared in the conversation.

| Option | Mockup | Exact prompt |
| --- | --- | --- |
| 1 | [Heart shield](01-heart-shield.png) | [Prompt](heart-shield-prompt.md) |
| 2 | [Ocean ribbon](02-ocean-ribbon.png) | [Prompt](ocean-ribbon-prompt.md) |
| 3 | [Guided recovery](03-guided-recovery.png) | [Prompt](guided-recovery-prompt.md) |

All three use the actual [favourite fish screen](../../../evidence/fish-actions-badges/final/1672x941/favorite-locked.png) as the attached image reference. The existing [fish details screen](../../../evidence/fish-actions-badges/final/1672x941/favorite-popover.png) was inspected to match the cream, blue, gold and coral palette.

## Intended behaviour

- Keep the fish and its favourite setting when the sale fails.
- Show the reason and the next step in one compact toast.
- Keep the aquarium, fish and main controls visible.
- Allow dismissal with the close control.
- For option 3, View fish opens the existing fish details so the player can change the favourite. It does not sell the fish or change its favourite setting.
- Reuse the chosen container for other errors, with an appropriate icon and message.

## Visual review

All three generated images were reviewed in the conversation. Each shows one toast with readable copy, clear spacing, an error or protection icon, and a close control. The aquarium and main controls remain visible. Option 3 includes the View fish button. The generated dimensions are approximate design references, not measured runtime layouts.

The game already blocks selling favourites in the fish details and sell-net flows. These mockups do not change that behaviour. No app code was changed or runtime tests run in this design pass. The chosen design still needs native implementation and checks at desktop and phone sizes.
