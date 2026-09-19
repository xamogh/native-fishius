# Settings

The Settings gear opens a modal with four turquoise cards, amber ON buttons,
teal OFF buttons and the shared blue header. The approved reference is in
`design/mockups/settings-2026-09-18/approved.png`.

All Settings text uses the bundled Luckiest Guy menu font, matching Shop, fish
details and shared dialogs. Text wrapping measures that same font. This follows
the user's request to use the app's actual menu typography instead of the
mockup's mixed-case lettering.

The landscape layout uses two columns. Narrow portrait windows use one column.
All buttons and the volume slider have separate targets of at least 44 screen
points. Targets stay inside the safe area and do not overlap. Short screens use
a shorter slider to keep its target clear of the Music switch.

| Control | Behavior |
| --- | --- |
| Music | Plays or pauses Coral Promenade independently of sound effects |
| Volume | Adjusts music from 0 to 100 percent; remembers the value while music is off |
| Sound effects | Enables tap and gameplay reward sounds |
| Reduce motion | Uses the existing reduced-motion behavior for fish, scenery, rewards and overlays |
| Vibration | Enables native light impact feedback on iOS; ignored on devices without that implementation |
| About & support | Shows the version and credits; copies version, platform and preferences for a bug report |

About & support is the only footer button and is centered beneath the cards.

No support destination has been configured. Copy support info does not send a
message, upload a save or include a player's identity or local file paths.

Buttons activate after a matching press and release. Dragging away cancels a
button. The slider previews during a drag and saves once on release. Losing
focus, resizing or entering the background cancels an unfinished drag. Escape
returns from About & support, then closes Settings. The backdrop and red X also
close the menu. Settings consumes pointer and keyboard input so it cannot feed,
sell or move items underneath.

Tab and Shift+Tab move keyboard focus. Enter and Space activate focused buttons.
With the slider focused, Left and Right adjust it by five percent; Home and End
set its limits.

Preferences use the existing atomic Session save transaction. A failed save
leaves the previous value active and shows a retry message. Old v4 saves remain
valid: vibration defaults to on, and an existing reduced-motion choice of on
is preserved. On iOS, the system motion preference is read at startup and on
returning to the foreground until the player explicitly changes the game toggle.
New action values are appended to preserve saved command receipt identities.

The `settings_controls` test exercises old saves, persistence, failed saves,
pointer and touch cancellation, keyboard access, About & support, audio mute,
volume and background behavior. It checks 844x390, 667x375, 568x320, 390x844,
320x568, 1024x768 and a 2x-density 852x393 viewport with a notch and home inset.
Use `--fixture settings` for a preview that does not overwrite a player's save.

The full visual review and implementation captures are in
[the Settings review](../evidence/settings-2026-09-18/design-qa.md).
