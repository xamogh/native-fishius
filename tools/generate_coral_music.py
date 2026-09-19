#!/usr/bin/env python3
"""Create Coral Promenade, an original playful aquarium theme.

Python 3, NumPy, FFmpeg and the macOS Swift tools are required. The companion
Swift renderer plays the score through the host's General MIDI instruments.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import wave

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RATE = 48_000
BPM = 96
BEAT = 60 / BPM
DURATION = 80.0
FRAMES = round(DURATION * RATE)
SEED = 18092602
REVISION = 3
REFERENCE = "https://www.youtube.com/watch?v=yNFSeuLjWJM"
BANK = Path("/System/Library/Components/CoreAudio.component/Contents/Resources/gs_instruments.dls")
HELPER = ROOT / "tools/render_music_score.swift"

# One chord per bar. Keep the harmony open and within C major. In the previous
# draft, close seventh/extension voicings and chromatic pickups sounded sour.
CHORDS = {
    "C6": (36, [55, 60, 64, 69]),
    "C": (36, [55, 60, 64, 67]),
    "Dm": (38, [57, 62, 65, 69]),
    "G": (43, [55, 59, 62, 67]),
    "C/E": (40, [55, 60, 64, 67]),
    "Am": (45, [57, 60, 64, 69]),
    "F": (41, [53, 57, 60, 65]),
    "Em": (40, [55, 59, 64, 67]),
}
PROGRESSION = [
    "C6", "C6", "Dm", "G", "C/E", "Am", "Dm", "G",
    "C6", "Em", "Am", "Am", "Dm", "G", "C6", "C",
    "F", "F", "Em", "Am", "Dm", "G", "C", "Am",
    "Dm", "G", "Em", "Am", "Dm", "G", "C6", "G",
]

# Original melody, expressed as (beat within bar, MIDI pitch, held beats).
# Short, repeated rhythmic ideas give the track its light conversational feel.
MELODY = [
    [(0.5, 76, .45), (1.25, 79, .4), (2, 81, .65), (3, 79, .65)],
    [(0, 76, .8), (1.75, 74, .22), (2.25, 72, 1.15)],
    [(0.5, 74, .45), (1.25, 77, .4), (2.6, 76, .22), (3, 74, .65)],
    [(0, 71, .8), (1.75, 69, .22), (2.25, 67, .9), (3.5, 71, .35)],
    [(0.5, 72, .45), (1.25, 76, .4), (2, 79, 1.25)],
    [(0, 76, .8), (1.5, 72, .4), (2.25, 69, 1.1)],
    [(0.5, 69, .4), (1.25, 74, .4), (2, 77, .55), (3, 74, .65)],
    [(0, 74, .8), (1.5, 71, .4), (2.25, 67, .8)],
    [(0.25, 76, .65), (1.25, 79, .55), (2.25, 81, .55), (3, 79, .65)],
    [(0.25, 79, .85), (1.5, 76, .5), (2.25, 71, 1)],
    [(0.25, 72, .6), (1.6, 71, .22), (2, 69, 1.5)],
    [(0.5, 72, .6), (1.5, 76, .55), (2.5, 81, .9)],
    [(0.25, 77, 1.2), (2.4, 76, .22), (3, 74, .65)],
    [(0.25, 71, .6), (1.25, 74, .5), (2, 79, .5), (3, 74, .55)],
    [(0.25, 72, 1.35), (2.25, 76, .5), (3.25, 79, .45)],
    [(0.25, 76, .65), (1.25, 79, .5), (2.25, 72, 1.05)],
    [(0.5, 77, .55), (1.5, 81, 1), (3, 84, .55)],
    [(0.25, 81, 1), (1.75, 77, .55), (3, 72, .65)],
    [(0.5, 71, .55), (1.5, 76, .75), (2.75, 79, .75)],
    [(0.25, 76, .65), (1.5, 72, .55), (2.5, 69, 1)],
    [(0.5, 74, .55), (1.5, 77, .75), (2.75, 81, .75)],
    [(0.25, 79, .65), (1.5, 71, .45), (2.25, 74, 1.1)],
    [(0.5, 76, 1.1), (2.4, 74, .22), (3, 72, .6)],
    [(0.25, 72, .65), (1.5, 76, .55), (2.5, 81, .85)],
    [(0.5, 77, .45), (1.6, 76, .22), (2, 74, 1.3)],
    [(0, 71, .8), (1.5, 74, .45), (2.25, 79, .8)],
    [(0.5, 79, .55), (1.5, 76, .6), (2.5, 71, .7)],
    [(0.25, 72, .75), (1.85, 71, .22), (2.25, 69, 1.1)],
    [(0.5, 69, .45), (1.25, 74, .4), (2, 77, .6), (3, 74, .65)],
    [(0, 74, .8), (1.5, 71, .4), (2.25, 67, 1.1)],
    [(0.25, 72, 1.35), (2.25, 67, .55), (3.25, 69, .45)],
    [(0.25, 71, .7), (1.5, 74, .5), (2.5, 67, .8)],
]


def score() -> dict:
    rng = np.random.default_rng(SEED)
    tracks = [
        dict(name="Marimba melody", program=12, gain=-5.5, pan=12, percussion=False, notes=[]),
        dict(name="Flute reply", program=73, gain=-13.5, pan=-8, percussion=False, notes=[]),
        dict(name="Nylon guitar", program=24, gain=-7.0, pan=-28, percussion=False, notes=[]),
        dict(name="Acoustic bass", program=32, gain=-6.0, pan=0, percussion=False, notes=[]),
        dict(name="Light percussion", program=0, gain=-12.0, pan=8, percussion=True, notes=[]),
    ]

    def note(track: int, beat: float, pitch: int, held: float, velocity: int,
             timing: float = .007) -> None:
        start = max(0, beat * BEAT + float(rng.uniform(-timing, timing)))
        tracks[track]["notes"].append(dict(
            start=round(start, 6), duration=round(held * BEAT, 6), pitch=pitch,
            velocity=int(np.clip(velocity + rng.integers(-3, 4), 1, 127))))

    for bar, chord in enumerate(PROGRESSION):
        bass, harmony = CHORDS[chord]
        base = bar * 4
        # Mostly fingerpicked, with a soft, brushed chord on the syncopations.
        for beat, index, held, vel in ((0, 0, .8, 61), (.75, 2, .55, 51),
                                       (2, 1, .75, 55), (2.75, 3, .5, 50)):
            note(2, base + beat, harmony[index], held, vel)
        for beat, velocity in ((1.5, 44), (3.5, 38)):
            for index, pitch in enumerate(harmony):
                note(2, base + beat + index * .022, pitch, .32, velocity)

        note(3, base + .012, bass, 1.25, 70)
        fifth = 43 if chord == "C/E" else bass + 7
        note(3, base + 2.08, fifth, 1.05, 59)
        if bar % 4 == 3:
            note(3, base + 3.5, bass, .32, 43)

        lead = 1 if 8 <= bar < 16 or 20 <= bar < 24 else 0
        for beat, pitch, held in MELODY[bar]:
            # Softer flute dynamics preserve the melody's contour without a sharp lead.
            note(lead, base + beat, pitch, held, 70 if lead == 0 else 61)

        # Quiet brush/rim, shakers and occasional bongos create the gentle bounce.
        for beat in (.5, 1.5, 2.5, 3.5):
            note(4, base + beat + .025, 70, .15, 31 if beat in (.5, 2.5) else 25)
        for beat in (1, 3):
            note(4, base + beat + .008, 37, .15, 37 if beat == 1 else 31)
        note(4, base, 36, .16, 31)
        if bar % 2:
            note(4, base + 2.75, 60, .16, 36)
            note(4, base + 3.35, 61, .16, 30)

    return dict(title="Coral Promenade", sampleRate=RATE, duration=DURATION + 4,
                loopDuration=DURATION, bpm=BPM, meter="4/4", key="C major",
                progression=PROGRESSION, tracks=tracks)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def measure(path: Path, raw: bool = False) -> dict:
    options = ["-f", "f32le", "-ar", str(RATE), "-ac", "2"] if raw else []
    run = subprocess.run(["ffmpeg", "-hide_banner", "-nostdin", *options, "-i", str(path),
                          "-af", "loudnorm=I=-19:TP=-2:LRA=11:print_format=json",
                          "-f", "null", "-"], check=True, capture_output=True, text=True)
    return json.JSONDecoder().raw_decode(run.stderr[run.stderr.rfind("{"):])[0]


def master(rendered: Path) -> None:
    audio_dir = ROOT / "assets/audio"
    preview_dir = ROOT / "design/audio"
    audio_dir.mkdir(parents=True, exist_ok=True)
    preview_dir.mkdir(parents=True, exist_ok=True)
    loop = audio_dir / "coral-promenade-loop.wav"
    preview = preview_dir / f"coral-promenade-v{REVISION}-preview.mp3"
    score_path = preview_dir / "coral-promenade-score.json"
    score_path.write_text(json.dumps(score(), indent=2) + "\n")
    decoded = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", str(rendered),
                              "-f", "f32le", "-ar", str(RATE), "-ac", "2", "-"],
                             check=True, capture_output=True).stdout
    source = np.frombuffer(decoded, "<f4").reshape(-1, 2)
    if len(source) != FRAMES + 4 * RATE:
        raise RuntimeError("The rendered score has the wrong duration.")
    mix = source[:FRAMES].copy()
    mix[:len(source) - FRAMES] += source[FRAMES:]
    rng = np.random.default_rng(SEED)
    freqs = np.fft.rfftfreq(FRAMES, 1 / RATE)
    curve = (1 - np.exp(-(freqs / 32) ** 4)) / np.sqrt(1 + (freqs / 10_000) ** 8)

    # A small stereo room keeps the plucked instruments close and clear.
    for channel in range(2):
        t = np.arange(round(RATE * 1.8)) / RATE
        impulse = rng.normal(0, 1, len(t)) * np.exp(-6.908 * t / 1.05)
        impulse *= np.clip((t - .019 - channel * .004) / .022, 0, 1)
        impulse = np.convolve(impulse, np.ones(12) / 12, mode="same")
        impulse *= .12 / np.linalg.norm(impulse)
        impulse[round((.037 + channel * .006) * RATE)] += .055
        room = np.fft.rfft(impulse, n=FRAMES)
        spectrum = np.fft.rfft(mix[:, channel])
        mix[:, channel] = np.fft.irfft(spectrum * (1 + room) * curve, n=FRAMES)

    with tempfile.TemporaryDirectory(prefix="fishius-coral-master-") as temp:
        raw = Path(temp) / "mix.f32le"
        mix.astype("<f4").tofile(raw)
        levels = measure(raw, raw=True)
    gain_db = min(-19 - float(levels["input_i"]), -2 - float(levels["input_tp"]))
    mix *= 10 ** (gain_db / 20)
    if not np.isfinite(mix).all() or np.max(np.abs(mix)) >= 1:
        raise RuntimeError("The exported mix must have finite, unclipped samples.")
    dither = (rng.random(mix.shape) - rng.random(mix.shape)) / 65536
    pcm = np.rint((mix + dither) * 32767).astype("<i2")
    with wave.open(str(loop), "wb") as out:
        out.setnchannels(2)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(pcm.tobytes())
    subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-y", "-i", str(loop),
                    "-af", "afade=t=in:d=0.15,afade=t=out:st=78:d=2",
                    "-c:a", "libmp3lame", "-b:a", "192k", "-metadata", "title=Coral Promenade",
                    "-metadata", "album=Fishius", str(preview)], check=True)
    levels = measure(loop)
    samples = pcm.astype(np.float32) / 32768
    seam = float(np.max(np.abs(samples[0] - samples[-1])))
    local = np.concatenate([samples[-4800:], samples[:4800]])
    typical_large_step = float(np.percentile(np.abs(np.diff(local, axis=0)), 99))
    assert seam < max(typical_large_step, .001), "Review the loop boundary before using the asset."
    record = dict(
        title="Coral Promenade", created="2026-09-18", revision=REVISION,
        direction="Playful, relaxed aquarium music inspired by the user's FishVille reference.",
        description="An original marimba and flute melody with nylon guitar, acoustic bass and light percussion. The revised harmony stays in C major with open chords and brief passing notes.",
        reference_url=REFERENCE,
        reference_use="Aesthetic reference only. No audio from FishVille is included or sampled. The melody and arrangement are newly written.",
        rendering="Original note score rendered offline with AVAudioUnitSampler and the macOS General MIDI sound bank, followed by circular room reverb and mastering.",
        sound_bank=dict(path=str(BANK), sha256=digest(BANK), bundled=False),
        generator=dict(path="tools/generate_coral_music.py", sha256=digest(Path(__file__))),
        renderer=dict(path="tools/render_music_score.swift", sha256=digest(HELPER)),
        seed=SEED, bpm=BPM, meter="4/4", tonal_center="C major", bars=32,
        duration_seconds=DURATION, sample_rate=RATE, channels=2, pcm_bits=16,
        loop_start_frame=0, loop_end_frame_exclusive=FRAMES,
        loop_method="Full WAV buffer. Instrument tails and room reflections wrap across the boundary without a fade or gap.",
        verification=dict(integrated_loudness_lufs=float(levels["input_i"]),
                          true_peak_dbtp=float(levels["input_tp"]),
                          loudness_range_lu=float(levels["input_lra"]),
                          seam_sample_step=seam, nearby_99th_percentile_step=typical_large_step,
                          clipped_samples=int(np.count_nonzero(np.abs(samples) >= 1)),
                          review="File, level and continuity checks. Preview supplied for listening review."),
        files={"loop": dict(name=loop.name, bytes=loop.stat().st_size, sha256=digest(loop)),
               "preview": dict(name=preview.name, bytes=preview.stat().st_size, sha256=digest(preview)),
               "score": dict(name=score_path.name, sha256=digest(score_path))})
    (audio_dir / "coral-promenade.provenance.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps(dict(wav=str(loop), preview=str(preview), verification=record["verification"]), indent=2))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--prepare", type=Path, help="Write the note score for a separate native render.")
    mode.add_argument("--master", type=Path, help="Master a WAV already rendered from this score.")
    args = parser.parse_args()
    if args.prepare:
        args.prepare.write_text(json.dumps(score(), indent=2) + "\n")
        print(args.prepare)
        return
    if not shutil.which("ffmpeg"):
        parser.error("FFmpeg is required.")
    if args.master:
        master(args.master)
        return
    if not shutil.which("swiftc") or not BANK.exists():
        parser.error("Full rendering requires macOS, its General MIDI bank, and the Swift compiler.")
    with tempfile.TemporaryDirectory(prefix="fishius-coral-render-") as temp:
        temp_dir = Path(temp)
        score_path = temp_dir / "score.json"
        score_path.write_text(json.dumps(score(), indent=2) + "\n")
        renderer = temp_dir / "render-score"
        subprocess.run(["swiftc", "-module-cache-path", str(temp_dir / "swift-cache"),
                        str(HELPER), "-o", str(renderer)], check=True)
        raw = temp_dir / "performance.wav"
        subprocess.run([str(renderer), str(score_path), str(raw)], check=True)
        master(raw)


if __name__ == "__main__":
    main()
