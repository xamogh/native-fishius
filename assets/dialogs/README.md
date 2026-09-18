# Funds dialog artwork

The coin and pearl dialogs share the approved orange octopus composition. Each image supplies the body artwork. The game draws the amount, frame, close control and Open Shop button separately so they remain live controls.

| Asset | Purpose | Generation prompt |
| --- | --- | --- |
| [funds-octopus-coins-v1.png](funds-octopus-coins-v1.png) | Octopus holding a gold star coin | [Provenance and prompt](funds-octopus-coins-v1.provenance.json) |
| [funds-octopus-pearls-v1.png](funds-octopus-pearls-v1.png) | Matching octopus holding an iridescent pearl | [Provenance and prompt](funds-octopus-pearls-v1.provenance.json) |
| [funds-underwater-v1.png](funds-underwater-v1.png) | Clean backdrop behind the smaller scene in compact windows | [Provenance and prompt](funds-underwater-v1.provenance.json) |

All three PNGs were made with the built-in `image_gen.imagegen` tool and copied unchanged. They are opaque images. The native renderer rounds the scene to the dialog body and feathers its edges when compact spacing requires a smaller scene.

The foreground uses about 22% to 77% of each scene's height. `src/hud_funds.cpp` fits that area between the message and button. The asset manifest records each file's dimensions and SHA-256 hash.

The [approved mockup](../../design/mockups/funds-octopus-2026-09-17/approved-coins.png) and [its prompt](../../design/mockups/funds-octopus-2026-09-17/provenance.json) are retained in the project. [Native visual checks](../../evidence/funds-octopus/design-qa.md) document the implementation and its small rendering differences.
