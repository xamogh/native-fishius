# Aquarium Studio

Open `Run Aquarium Studio.command` to build and start the native C++ editor.
On macOS, you can also open `build/desktop/Aquarium Studio.app`.

Studio and the game use the same layout, assets, font files, text measurement,
button states and SDL renderer. The editor uses Dear ImGui for its controls.
Clay resolves the game UI hierarchy after the shared layout rules have been
calculated. The game does not load an HTML or JavaScript interface.

## Screens and components

The starting screens are General dialog, Shop card, Tank upgrade and Currency
HUD. These are connected to the real shortage dialogs, the six currency shop
cards, the tank detail purchase panel and the wallet controls. The tank list,
shop placement and other existing menus keep their current game code.

Create a screen, duplicate a screen, add text, images, buttons and groups, or
drag reusable components onto a screen. Components include the illustrated
frame, wooden header, dialog header, currency text, primary button, close
button, shop card, tank upgrade panel and wallet. Component changes reach all
instances. New screens can be designed and played in the preview game. Adding
new gameplay navigation or purchase rules still requires a C++ action handler.

Screen tabs sit across the top. The left panel separates Screens, Layers and
Assets. Search artwork by its asset path and drag it onto an image to replace
that image, or onto empty canvas to add a layer. The right panel separates
Layout, Style, Content and Action.

## Direct editing

Click a layer in the canvas or layer list. Drag it to move it and drag the lower
right handle to resize it. Double click text to edit it, then press Enter or
Apply. Text remains editable text, including button labels. Amounts, balances,
prices and level requirements are supplied by the game through bindings.

Hold Shift to select several layers. Drag across empty space to select layers
inside a rectangle. Group, ungroup, duplicate, align, distribute and change
layer order from the Layers panel or keyboard shortcuts. Group and arrange
layers in the same component. Fields in the inspector give exact coordinates.

Use the mouse wheel to zoom. Middle drag, Alt drag or Space drag pans the
canvas. Fit returns to the whole device view. The zoom percentage is relative
to that fit. Export PNG writes the full device pixels without editor guides.

| Shortcut | Action |
| --- | --- |
| Command/Ctrl + S | Save approved design |
| Command/Ctrl + Z | Undo |
| Command/Ctrl + Shift + Z or Y | Redo |
| Command/Ctrl + G | Group sibling layers |
| Command/Ctrl + Shift + G | Ungroup |
| Command/Ctrl + D | Duplicate |
| Delete or Backspace | Delete selected layers |
| Arrow keys over canvas | Move by one unit |
| Shift + arrow key | Move by ten units |
| Command/Ctrl + 0 | Fit and reset pan |
| Escape | Cancel text edit or clear selection |

## Layout, styles and variants

Groups can position children freely or in rows and columns. Layers can anchor
left, center, right, top or bottom, stretch across the parent, fill available
row width, wrap text or hug text height. Hug width sizes text within its width
limit. A Below rule follows another sibling from the layer picker and a gap. Decorative
images can keep exact coordinates. Gap and padding of `-1` inherit the shared
style's spacing; positive values override it.

Styles share fonts, sizes, colors, attention colors, spacing, outlines,
shadows, pressed scale and disabled opacity. Choose a preset, override a
layer's values, or enable Edit shared preset to change all uses. Fonts use
Baloo 2 ExtraBold, Lilita One or Nunito SemiBold from the game assets.

Coins and pearls share component definitions. Content values belong to a
variant. Enable Override this variant to change one instance's layout or
style. A star identifies an overridden layer. Reset variant overrides restores
inheritance. Use in all variants copies a content value across variants.

Actions connect buttons to existing C++ callbacks. Visibility, enabled state
and attention color can depend on named values. For example, `coinsShort`
uses the price style's attention color without disabling the purchase button.
The button can therefore explain why the player needs more currency.

## Preview and test

Compare coins with pearls, four devices, four states, or the draft beside the
approved design. Each tile has its own game session and cached texture.
Device choices include desktop, two phone sizes, tablet and custom dimensions,
with pixel density and safe insets. Scenarios cover large amounts, long sample
German text, a level requirement, pressed controls and disabled controls.
Problems identifies overflowing text, layers outside safe areas and small
touch targets. Select a problem to find its layer.

Play enables the real controls in a separate session. Test tank purchase flow
opens the actual tank menu with editable level, coin balance, pearl balance,
ownership and upgrade count. Buy uses the existing purchase validation and
transactions. It can show a shortfall, explain a level requirement, unlock or
upgrade a tank, and visit the correct shop. The player's save is not used.
Currency pack purchases retain the game's existing Coming Soon response.

The Motion tab edits the dialog's opening duration, damping, frequency,
starting scale and rise. Replay or scrub the motion. Reduced Motion in the
actual game continues to disable animation. The bottom tools can collapse to
give the canvas more room.

## Drafts, approved designs and a connected game

`assets/ui/studio.json` is the approved design used by the game. Studio imports
the previous `assets/ui/general-dialog.json` when creating a new project. The
older renderer remains available for builds without a Studio project.

Drafts autosave to `local-data/studio/draft.json`, including up to 64 undo
states. They recover after restarting Studio. A continuous edit or drag forms
one undo step. Save approved writes the game file with an atomic replacement.
Games using the source assets poll it every 250 ms. Packaged builds contain
the assets from their last build.

Named versions live in `local-data/studio/versions`. Restoring a version edits
the draft and can be undone. Draft + approved comparison shows both images.
If another process edits the approved file, Studio preserves the draft and
blocks an accidental overwrite. The Versions panel offers an explicit choice
to keep the draft or load the changed file, preserving a named version first.

Connect game starts a separate preview game using `local-data/studio/live.json`.
It receives draft designs and the current screen or tank test scenario. It
acknowledges the design revision shown in the toolbar. Send draft edits can
pause updates; Restart game resets the external test session. Disconnect closes
that preview process. Saving an approved design is a separate operation.

Invalid designs keep the last valid runtime document. Validation covers node
sizes, styles, fonts, component cycles, variant overrides, layout constraints,
asset paths and missing files. Unknown actions cannot add arbitrary code.

An unchanged Design preview reuses its texture. Comparisons redraw when their
design or scenario changes. Play and motion replay render continuously. The
editor waits for input between idle frames and shows render counts in its
status bar.

## Build and verification

```sh
cmake --preset desktop
cmake --build --preset desktop
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ctest --test-dir build/desktop --output-on-failure
build/desktop/aquarium_ui_runtime_tests assets --native
build/desktop/aquarium_studio --smoke --frames 25 --report evidence/studio/editor-check.json
```

Document tests cover migration, effective variant validation, grouping,
persistent undo, recovery, versions, conflicting edits and invalid files.
Runtime tests compare every pixel for all starting screens at four sizes and
exercise successful and unsuccessful tank purchases. Existing game tests
continue to cover price colors, touch controls, shop return state and motion.
The editor smoke check uses pointer input for Design/Play and double click text
editing, then verifies the resulting draft and undo operation.

Useful editor flags are `--screen general-dialog`, `--pearls`, `--compare 2`,
`--scenario 3`, `--test-flow`, `--frames 8`, `--capture editor.png`,
`--export preview.png` and `--project path.json`. Runs with `--frames` use a
separate in-memory draft so screenshot checks do not change your recovery file.
`--init-design --project path.json` creates a project only if it does not exist.
The game's `--ui-project`, `--ui-screen` and `--ui-variant` flags select an
explicit preview document and automatically use a separate review session.
