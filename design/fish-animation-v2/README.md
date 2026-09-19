# Neon tetra and guppy animations

Animation assets for the [eccentric fish study](../mockups/fishville-art-2026-09-18/08-eccentric-fish.png), created on 18 September 2026.

The tetra has quick tail beats and short, sometimes paired blinks. The guppy has a slower tail swish and longer sleepy blinks. Both have fin movement, a slight body bob and a small feeding gulp. The large eyes and short fish fins remain part of the design.

## Preview

- [12-second video](fish-animation-preview.mp4), 1080 × 680 at 24 fps.
- [Looping GIF](fish-animation-preview.gif).
- [Pose comparison](contact-sheet.png): resting, closed eyes and feeding.
- [Interactive preview](index.html): full sequence, idle, swim, blink, gulp, pause, speed and timeline controls. Its fonts are included with their license in `fonts/`.

```sh
python3 -m http.server 4178 --bind 127.0.0.1
```

Then open `http://127.0.0.1:4178/design/fish-animation-v2/`.

You can also serve the unpacked `fish-animation-v2` folder directly and open the server's root URL.

## Animation assets

Each species has four transparent sprite sheets in `sprites/`. Frames are 384 × 256 pixels at 24 fps, laid out left to right in eight columns. The JSON files contain every frame rectangle and duration. Blank cells after the last frame are not part of a clip.

| Clip | Frames | Duration | Playback |
| --- | ---: | ---: | --- |
| idle | 48 | 2 seconds | Loop |
| swim | 48 | 2 seconds | Loop with stronger tail motion |
| blink | 12 | 0.5 seconds | Play once, then return to idle |
| feed | 18 | 0.75 seconds | Play once, then return to idle |

- [Neon tetra metadata](sprites/neonTetra.json)
- [Guppy metadata](sprites/guppy.json)
- `parts/` contains the separate painted sprites for a runtime rig.
- [rigs.json](rigs.json) contains their placement, pivots and species timing.
- [animation.mjs](animation.mjs) contains the deterministic posing code used for both the browser preview and exported frames.

Neon tetra faces right and guppy faces left. Flip an entire assembled fish or frame to reverse it. Keep the fixed frame canvas and anchor when switching clips to avoid position changes. The swim sheets are seamless loops; the full 12-second video is a review sequence.

These files are an animation package and preview. They are not connected to the game's current fish renderer.

## Artwork and prompts

The sprite parts and transparency extraction were produced with **built-in Image Gen**. The initial outputs had opaque checkerboards. A separate Image Gen extraction produced real RGBA alpha. The preparation script crops the sprites and removes isolated alpha speckles; it preserves the retained painted pixels. The rig blends the hidden attachment edges at the tail joints.

- [Neon tetra prompt](prompts/neonTetra.md)
- [Guppy prompt](prompts/guppy.md)
- [Transparency extraction prompt](prompts/transparency.md)
- [Source provenance](provenance.json)
- [Slicing bounds](extraction.json)

## Rebuild and checks

The export scripts need Node.js, `sharp`, `@napi-rs/canvas` and `ffmpeg`. Use `NODE_PATH` if those packages live outside the project.

```sh
node design/fish-animation-v2/prepare-atlases.cjs
node design/fish-animation-v2/render.cjs --all
ffmpeg -y -i design/fish-animation-v2/fish-animation-preview.mp4 -filter_complex '[0:v]fps=16,scale=720:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=3:diff_mode=rectangle' -loop 0 design/fish-animation-v2/fish-animation-preview.gif
```

The exporter checked all 252 frames for clipping at the canvas border. The four swim loop endpoints matched exactly. Every sheet has an alpha channel. Resting, closed-eye and gulp poses were visually reviewed at large and small sizes. The interactive preview's pause, blink, gulp and timeline controls were checked in the browser. See [qa.json](qa.json).
