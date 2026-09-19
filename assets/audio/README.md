# Fishius background music

`coral-promenade-loop.wav` is the current music draft. The user requested a
FishVille-like direction after reviewing the earlier ambient version. This new
piece has a playful marimba and flute melody, nylon guitar, acoustic bass and
light percussion.

Revision 3 responds to the user's report that the previous mix sounded out of
tune. It uses open chords within C major, keeps held melody notes within each
chord, removes chromatic bass pickups, corrects the bass under C/E, and removes
the extra vibraphone and pizzicato fills. Brief passing notes remain in the tune.

The melody and arrangement are original. The
[FishVille tank theme](https://www.youtube.com/watch?v=yNFSeuLjWJM) is an aesthetic
reference. No audio from that recording is included or sampled.

| Property | Value |
| --- | --- |
| Title | Coral Promenade |
| Duration | 1 minute 20 seconds |
| Tempo and meter | 96 BPM, 4/4 |
| Tonal center | C major |
| Runtime format | 48 kHz stereo, 16-bit PCM WAV |
| Loop start | Frame 0 |
| Loop end, exclusive | Frame 3,840,000 |
| Target loudness | -19 LUFS integrated |

Repeat the entire WAV buffer without a gap. Notes and reverb continue across the
boundary. The WAV has no fade at either end. Fade the playback stream when starting
or stopping music during play, rather than fading each repetition.

The listening preview is `design/audio/coral-promenade-v3-preview.mp3`. It includes
short opening and closing fades and is not the runtime loop.

The game now loads this loop through `GameAudio`. Settings controls its playback
and volume independently of tap and reward sounds. Playback fades over 80 ms,
queues at most 250 ms, and pauses when the app enters the background. A missing
audio device or WAV leaves the game usable without music. The earlier Lagoon
Daydream track is not used at runtime.

## Regenerate

The score and mix are defined in `tools/generate_coral_music.py`. The generator
requires Python 3, NumPy, FFmpeg and the macOS Swift tools. Its companion,
`tools/render_music_score.swift`, plays the score through the Mac's built-in General
MIDI instrument bank. The bank itself is not copied into this project.

```sh
python3 tools/generate_coral_music.py
```

`design/audio/coral-promenade-score.json` also records the individual notes and
instrument settings. The provenance file records the generator, renderer, sound
bank and asset hashes, loop frames and measured audio levels. The WAV is checked
for invalid samples, clipping and continuity at the repeat point. Listening review
remains subjective.

`design/audio/coral-promenade-v3-tuning.json` records the revision's pitch and
harmony checks. All 38 distinct instrument/pitch combinations were rendered
separately. Their estimated fundamental frequencies were within 1 cent of
equal temperament at A4 = 440 Hz. This measures tuning, not listener preference.

## Earlier draft

`lagoon-daydream-loop.wav` and its provenance are retained as the first draft.
The user rejected that ambient direction. Its generator is
`tools/generate_lagoon_music.py`; its preview is
`design/audio/lagoon-daydream-preview.mp3`.

`design/audio/coral-promenade-preview.mp3` retains revision 2 for comparison.
