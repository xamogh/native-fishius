# neonTetra animation atlas

Use case: identity-preserve
Asset type: animation sprite parts atlas for a 2D cartoon aquarium game.
Input reference: the attached latest eccentric fish sheet. Preserve the specified fish exactly, including its enormous eyes, odd expression, colors, rounded fish mouth, compact triangular lower fins and fish silhouette. This is asset preparation, not a new design.

Output a square 1536 x 1536 TRUE TRANSPARENT RGBA PNG arranged as exactly 3 columns and 3 rows of equal 512 x 512 cells. Each cell contains one isolated sprite. No grid, labels, text, floor, shadow, checkerboard or background paint. Every sprite entirely inside its cell with generous transparent margin. Isolated parts may be enlarged to fit their cell; keep their natural proportions. All pieces have the same viewpoint and facing direction as the specified fish in the reference. Paint complete hidden joint roots in matching solid color to permit overlap.

Cell order, left to right and top to bottom:
1: Complete assembled fish, matching the reference exactly. This is the neutral portrait and reconstruction reference.
2: The same fish BODY AND HEAD TOGETHER, including both huge eyes with pupils, eyelids, gill mark, cheeks, tiny round pink mouth, markings and tail stem. Remove only tail and ALL fins. Keep the original body silhouette and face proportions. Do not remove the eyes or mouth, do not replace them with placeholders. Complete the body surface where fins were removed. This intact face/body is the main animation sprite.
3: Detached tail fin with a short solid root for attaching to the body, preserving the exact fan or fork shape.
4: Detached dorsal fin, preserving its compact triangular shape and natural base.
5: Detached side fin, a small membrane with a broad base. No feather lobes or wing shapes. If the reference side fin is hidden, use a small blue triangular pectoral fin matching the fish.
6: Detached lower fin, a SHORT swept-back triangular fin membrane, broad base, no dangling leaf, leg or foot.
7: The large NEAR EYE fully CLOSED: same huge oval outline and proportions as the large eye in cell 2, with its entire white and pupil covered by a colored eyelid. Smooth opaque colored fill, matching thin rim, one curved closed-eye crease near the bottom. Eye only, no face or cheek patch. This will overlay the open eye at the exact same size.
8: The smaller FAR EYE fully CLOSED, same outline as the far eye in cell 2, colored lid covers the entire white and pupil. Eye only, no cheek patch. Include its rim.
9: The same small rounded pink mouth now OPEN for a feeding gulp: a round dark opening bordered by plump pink lips, clear fish mouth, no teeth, no beak or projecting duckbill, no cheek patch. Same viewpoint and short profile as cell 2.

Style invariants: huge cream eye whites and tiny pupils; strange slightly grotesque cartoon character; thin locally colored contours; simple flat color with restrained shading. No realism, scales, feathers, long fins, smaller eyes, pointed noses or heavy black borders. All areas outside the isolated sprites must have actual zero alpha, not a visible checkerboard. Do not paint background pixels. All nine cells populated.

Character: NEON TETRA, the LEFT fish in the reference. Faces RIGHT. Use the largest version from the sheet. Very large blue-rimmed near eye, tiny black pupil looking forward, smaller far eye peeking on the right, startled awkward expression. Cobalt back, luminous cyan lateral stripe, vivid red rear belly, pale rounded cheek/belly, light blue forked tail. Keep the little pursed pink O-mouth close to its blunt fish face. Closed lids are medium periwinkle blue. The detached tail's narrow root points RIGHT; tail spreads LEFT. Dorsal and lower fins match the reference's short blue membranes. Preserve the broad head with no neck. Do not change face or body proportions.
