# Exact image-generation prompts

Mode: built-in ImageGen. Both outputs were generated from attached visual references.

## 1. Reef Home

```text
Use case: ui-mockup
Asset type: high-fidelity landscape mobile game UI concept for Fishius, a native aquarium care and collection game.
Concept: Reef Home.
Create one polished 1920 x 1080 landscape game screenshot, app content only, with clear hierarchy, strong readable typography, intentional art and purposeful spacing.
Input images: Image 1 (Clash of Clans gameplay) is the primary reference for UI design language and perimeter HUD organization. Image 2 (Clash of Clans shop) reinforces chunky outlines, simple raised controls and solid pale surfaces. Image 3 (current Fishius aquarium) is product grounding for fish designs, underwater setting, currencies and real controls. Redesign Fishius using those UI principles. Keep the Fishius aquarium identity.
Scene: an immersive side-view aquarium with bright turquoise water, gentle light rays, faceted blue stone, warm sand, and restrained lush seaweed and coral at the outer edges. Eight friendly, beautifully shaded fish swimming at natural varied depths: guppies, orange platies, black mollies and neon tetras based on the Fishius reference. Preserve a broad clear central swimming area, roughly 65 percent of the frame. Premium stylized 3D mobile-game art with broad clean shapes, attractive soft lighting, crisp silhouettes. Aquarium remains the main subject. No village, battle, characters, buildings or land map.
UI design system: Clash-inspired thick charcoal-teal outlines, rounded rectangular buttons with substantial lower bevels, small bright top highlights, pale warm limestone and ivory surfaces, legible chunky white display letters with a short dark outline/shadow, larger friendly rounded body type. Match the clear simplified graphic shapes of the references. Slightly rounded squared corners, not bubble-shaped pills everywhere. Emerald/lime green primary actions, honey gold shop emphasis, muted teal secondary controls. Limited texture. Avoid excessive ornate wood, rope borders and gold filigree.
Composition:
- Upper left: compact blue level badge with a gold star and numeral "1", beside a short blue XP bar labelled "14 / 80 XP". Small "Fishius" name above the bar.
- Upper right: two tidy horizontal dark-teal resource counters, gold star coin "250" and iridescent pearl "0", each with a small green plus button. Use coin and pearl icons from the Fishius reference, not Clash gems or elixir. Plenty of room between the counters.
- Left edge, below the XP area: one small cream square Mastery button with a chunky gold star and tiny label "Mastery", one small mail icon below it. These are secondary.
- Lower left: a tactile pale stone tank selector, small aquarium icon, label "Tank 1", capacity "8 / 10", and two compact teal arrows. A smaller rounded-square bag icon labelled "Bag" sits just to its right.
- Lower right: a substantial lime-green raised button with a cute fish-food jar illustration and the label "Food". Immediately beside it, a slightly smaller honey-gold raised button with a fish-shop awning illustration and the label "Shop". These controls resemble durable toy-like game buttons in the Clash reference. Small unobtrusive settings gear above them.
- One small unobtrusive green growth bar anchored above a guppy, labelled "Growing". No floating money rewards or invented idle-income controls.
Keep all controls inside a generous 40px safe margin. No central panel, no dialog, no marketing banner. The interface should feel usable in a real landscape phone game. Very crisp readable text, intentional restrained UI, no crowding, no overlap.
Typography: at most two faces, chunky Lilita One-like headings and rounded Nunito-like support text. Main labels optically at least 30px on the 1920px canvas.
Constraints: one screen only; no phone bezel, device body, browser frame, OS status bar, presentation board, option label, watermark or explanatory annotations. No Clash of Clans or Supercell logo. No unrelated new game mechanics. Do not reproduce old Fishius UI verbatim. This is a considered fresh mockup of Fishius using the attached Clash UI references.
```

Attached images, in order:

1. `references/clash-gameplay.png`
2. `references/clash-gems.png`
3. `evidence/lagoon-redesign/aquarium.png` from the repository root

## 2. Fish Market

```text
Use case: ui-mockup
Asset type: high-fidelity landscape mobile game UI concept for Fishius, a native aquarium care and collection game.
Concept: Fish Market.
Create one polished 1920 x 1080 landscape in-game fish shop screenshot, app content only. Create a realistic production-quality UI design with clear hierarchy, strong readable typography, intentional fish illustrations, and purposeful spacing.
Input images: Image 1 (Clash of Clans Gems shop) is the primary reference for the simple pale panel, oversized tabs, bold outlined lettering, framed merchandise cards and chunky purchase controls. Image 2 (current Fishius shop) grounds existing fish species, aquarium identity and actual shop categories. Its old prices and durations must be replaced with the exact copy below. Image 3 (new Fishius aquarium mockup) is a supporting design-system reference: match its charcoal outlines, lower bevels, turquoise/ivory/lime/gold color system, lettering, gold star coins and iridescent pearls, but make this a fish-shopping screen with a distinct shop layout.
Scene/backdrop: the same cyan underwater aquarium softly dimmed behind a nearly full-screen shop overlay. Thin strips of aquarium remain visible around the panel. Keep the two resource counters visible at top-right, matching Image 3: gold star coin "250" with small green plus, iridescent pearl "0" with small green plus. Use clearly readable sample balances.
Layout and hierarchy:
- A broad pale ivory stone-like shop panel fills roughly x=65..1855, y=150..1020, with slightly rounded rectangular corners, a charcoal-teal edge and convincing lower bevel. It should feel like a clear contained game menu in the Clash shop reference.
- Five large chunky tabs connect to the top edge of this panel. Their exact labels: "Fish", "Plants", "Decor", "Environment", "Food". Each has one immediately readable illustrated icon, respectively fish, seaweed, coral, landscape rock, food jar. Active Fish tab is creamy ivory and visibly joins the main surface. Inactive tabs are muted teal-gray. Keep tab labels modest and legible, no additional categories.
- Large bold heading "Fish Shop" centered on the panel. A single raised red rounded-square button with a white X at the upper-right corner. No long marketing banner or extra promotional panel.
- Under the heading, one evenly aligned row of four generous independent collectible fish cards. Each card has a rounded rectangular pale aqua background with a restrained radial light burst behind the fish; heavy but clean charcoal outline, subtle inner highlight, and short cast shadow. Each card represents one actual purchasable fish so distinct cards are appropriate here.
- Fish art is large and centered, filling most of each card's upper area, glossy stylized dimensional aquarium fish based on the reference species. Friendly and beautifully shaded without losing species identity. Neon Tetra: bright blue horizontal stripe and red rear stripe. Guppy: blue-silver body and flowing orange fan tail. Platy: compact orange fish. Molly: black fish with a tall dorsal fin.
- Card name appears in large chunky white letters with dark outline at the top. Below fish art use a small neutral summary area with just two readable rows. Each card has a large green beveled price button at the bottom, showing the gold star coin icon and its number. The button buys an egg for placement, so the footer explains the action.
Exact card copy left to right, no extras:
"Neon Tetra"
"Adult in 20m"
"Rehome: 11 coins"
price button coin icon + "5"

"Guppy"
"Adult in 2h"
"Rehome: 49 coins"
price button coin icon + "10"

"Platy"
"Adult in 20h"
"Rehome: 375 coins"
price button coin icon + "75"

"Molly"
"Adult in 44h"
"Rehome: 759 coins"
price button coin icon + "152"
- Keep consistent alignment and spacing for names, fish, body text and price buttons on every card. 4 cards only, all fully visible with neither horizontal cropping nor clipped labels.
- Small teal previous/next arrow controls at the row's outer edges and four small page dots below, first dot selected gold.
- Along the panel bottom, concise centered instructional line "Choose a fish, then place its egg in your tank." At bottom left a small capacity label "Tank 1: 8 / 10". No other footer actions.
Visual system: take the clear graphic restraint and charm of the Clash reference. Broad slightly rounded squared silhouettes, thick dark outlines, pale matte panels, toy-like raised beveled green controls, short intentional shadows, clean top-face highlights. White outlined display headings and dark teal body text on pale backgrounds. Main colours cyan, warm ivory, charcoal-teal, lime green and restrained gold. Use just two font styles, Lilita One-like playful display lettering and rounded Nunito-like body type. Main labels optically at least 30px on the 1920px canvas. Focus on browsing fish and their prices, with generous spacing, no distracting feature inventory.
Constraints: match Image 3's Fishius UI family while showing a distinct shop composition inspired by Image 1. One independent screen only. No device bezel, phone body, notch, OS status bar, browser chrome, collage, presentation board, headings outside the UI, numerical option label, watermark, fake QR code or logo for Clash/Supercell. No store payments, discount badges, countdown offers, imaginary mechanics, extra categories or extra fish cards. Do not paste the old Fishius screen into this result; compose a new refined mockup. Do not overdecorate with carved wood, rope, vines or ornate gold trim. All text clean and legible.
```

Attached images, in order:

1. `references/clash-gems.png`
2. `evidence/shop-dialog-size/desktop.png` from the repository root
3. The first generated image, saved here as `01-reef-home.png`
