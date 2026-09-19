#!/usr/bin/env python3
"""Compose Fishius's original aquarium loop with NumPy and FFmpeg.

All instruments and water textures are synthesized here. No sample packs,
recordings, pretrained music model, or external sound fonts are used.
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
BPM = 80
BEAT = 60 / BPM
BARS = 32
DURATION = BARS * 4 * BEAT
FRAMES = round(DURATION * RATE)
SEED = 18092026
TAU = 2 * np.pi

# Two bars per chord. Close upper voices give each change a gentle motion.
# Each row contains the chord name, bass note, and pad/electric piano voicing.
HARMONY = [
    ("Dmaj9", 38, [57, 61, 64, 66]),
    ("A6/9/C#", 37, [57, 59, 64, 66]),
    ("Bm9", 35, [54, 57, 61, 62]),
    ("Gmaj9", 43, [54, 57, 59, 62]),
    ("Dadd9/F#", 42, [57, 62, 64, 66]),
    ("Em9", 40, [55, 59, 62, 66]),
    ("Gmaj9", 43, [54, 57, 59, 62]),
    ("A6sus4", 33, [57, 59, 62, 66]),
    ("Bm9", 35, [54, 57, 61, 62]),
    ("F#m11", 42, [57, 59, 61, 64]),
    ("Gmaj9", 43, [54, 57, 59, 62]),
    ("Dadd9/A", 45, [57, 62, 64, 66]),
    ("Em9", 40, [55, 59, 62, 66]),
    ("Gmaj9", 43, [54, 57, 59, 62]),
    ("Dadd9/F#", 42, [57, 62, 64, 66]),
    ("A6/9", 33, [57, 59, 64, 66]),
]

# Beat, pitch, and held length within each two-bar phrase. Rests are deliberate.
# Four related eight-bar sections keep the main motif familiar without a
# constant lead melody competing with the aquarium.
MELODY = [
    [(0.5, 66, 0.8), (1.75, 69, 0.8), (3, 73, 1.6), (5.5, 71, 0.8), (7, 69, 0.7)],
    [(1, 64, 1), (2.5, 66, 1.4), (5, 69, 1.8)],
    [(0.75, 66, 1), (2.25, 69, 0.8), (3.5, 73, 1.5), (6, 71, 1.5)],
    [(1, 71, 0.9), (2.5, 69, 0.9), (4, 66, 2)],
    [(0.5, 69, 0.8), (1.75, 73, 0.8), (3, 74, 1.8), (6, 73, 1.2)],
    [(1, 71, 1.2), (3.25, 69, 0.9), (5, 66, 1.8)],
    [(0.75, 67, 1), (2.25, 69, 0.9), (3.75, 71, 1.6), (6.25, 69, 0.8)],
    [(1, 66, 1.4), (4.5, 64, 1.8)],
    [(1, 73, 1.3), (3, 74, 1.5), (5.5, 78, 1.1)],
    [(0.75, 76, 1.5), (3.5, 73, 1.2), (6, 69, 1.5)],
    [(1, 71, 1.1), (2.75, 74, 1.5), (5.25, 73, 0.8), (6.5, 71, 0.9)],
    [(1.25, 69, 1.6), (4, 66, 2)],
    [(0.5, 66, 1), (2.25, 69, 1), (4, 71, 1.7)],
    [(1, 74, 1.1), (3, 71, 1.2), (5.25, 69, 1.5)],
    [(0.75, 66, 1.4), (3, 69, 1.8), (6, 64, 1.3)],
    [(1.25, 66, 1.8), (4.5, 64, 1.6)],
]


def frequency(midi: int) -> float:
    return 440 * 2 ** ((midi - 69) / 12)


def timeline(seconds: float) -> np.ndarray:
    return np.arange(round(seconds * RATE), dtype=np.float64) / RATE


def smooth_gate(t: np.ndarray, hold: float, attack: float, release: float) -> np.ndarray:
    rise = np.sin(np.minimum(t / attack, 1) * np.pi / 2) ** 2
    fall = np.cos(np.clip((t - hold) / release, 0, 1) * np.pi / 2) ** 2
    return rise * fall


def voice(kind: str, midi: int, hold: float, phase: float = 0) -> np.ndarray:
    f = frequency(midi)
    if kind == "pad":
        t = timeline(hold + 2.0)
        drift = 0.006 * np.sin(TAU * 0.17 * t + phase)
        out = np.zeros_like(t)
        for ratio in (0.9984, 1.0013):
            p = TAU * f * ratio * t + drift
            out += (np.sin(p) + 0.18 * np.sin(2 * p + 0.3)
                    + 0.055 * np.sin(3 * p + 0.7)) * 0.5
        out *= smooth_gate(t, hold, 1.1, 2.0)
        out *= 0.92 + 0.08 * np.sin(TAU * 0.12 * t + phase)
    elif kind == "piano":
        t = timeline(hold + 1.9)
        p = TAU * f * t
        tine = 0.68 * np.exp(-t / 0.27) * np.sin(2 * p)
        out = (np.sin(p + tine) * np.exp(-t / 2.7)
               + 0.18 * np.sin(2 * p + 0.2) * np.exp(-t / 0.85)
               + 0.045 * np.sin(3 * p) * np.exp(-t / 0.32))
        out *= smooth_gate(t, hold, 0.009, 1.9)
    elif kind == "pearl":
        t = timeline(hold + 2.7)
        p = TAU * f * t
        out = (np.sin(p + 0.34 * np.exp(-t / 0.2) * np.sin(2 * p))
               * np.exp(-t / 1.65)
               + 0.17 * np.sin(2 * p) * np.exp(-t / 0.55)
               + 0.10 * np.sin(4.012 * p) * np.exp(-t / 0.22)
               + 0.02 * np.sin(6.02 * p) * np.exp(-t / 0.09))
        out *= smooth_gate(t, hold, 0.007, 2.7)
    elif kind == "harp":
        t = timeline(hold + 1.25)
        p = TAU * f * t
        out = (np.sin(p) * np.exp(-t / 1.1)
               + 0.23 * np.sin(2 * p) * np.exp(-t / 0.43)
               + 0.075 * np.sin(3 * p) * np.exp(-t / 0.21)
               + 0.022 * np.sin(4 * p) * np.exp(-t / 0.10))
        out *= smooth_gate(t, hold, 0.007, 1.25)
    elif kind == "bass":
        t = timeline(hold + 0.7)
        p = TAU * f * t
        out = (np.sin(p) + 0.19 * np.sin(2 * p) + 0.035 * np.sin(3 * p))
        out *= np.exp(-t / 4) * smooth_gate(t, hold, 0.07, 0.7)
    else:
        raise ValueError(kind)
    return out.astype(np.float32)


def add_wrapped(bus: np.ndarray, signal: np.ndarray, start: float,
                level: float, pan: float) -> None:
    """Place note tails across the loop boundary instead of fading the master."""
    offset = round(start * RATE) % FRAMES
    angle = (pan + 1) * np.pi / 4
    stereo = signal[:, None] * np.array([np.cos(angle), np.sin(angle)], np.float32) * level
    first = min(len(signal), FRAMES - offset)
    bus[offset:offset + first] += stereo[:first]
    if first < len(signal):
        bus[:len(signal) - first] += stereo[first:]


def spectral_filter(samples: np.ndarray, low: float, high: float) -> np.ndarray:
    """A circular filter preserves the boundary of a periodic waveform."""
    freqs = np.fft.rfftfreq(len(samples), 1 / RATE)
    curve = 1 / np.sqrt(1 + (freqs / high) ** 6)
    curve *= 1 - np.exp(-(freqs / low) ** 4)
    return np.fft.irfft(np.fft.rfft(samples) * curve, n=len(samples)).astype(np.float32)


def reverberate(send: np.ndarray, rng: np.random.Generator) -> np.ndarray:
    """Diffuse stereo reflections, convolved around the full loop."""
    result = np.zeros_like(send)
    t = timeline(4.2)
    for ch in range(2):
        impulse = rng.normal(0, 1, len(t)).astype(np.float32)
        # Dense, softly filtered reflections bloom after the initial transients.
        impulse = spectral_filter(impulse, 220, 3700)
        impulse *= np.exp(-6.908 * t / 3.3)
        impulse *= np.clip((t - 0.027 - ch * 0.006) / 0.10, 0, 1)
        impulse /= max(np.linalg.norm(impulse), 1e-12)
        impulse *= 0.78
        for delay, gain in ((0.049, 0.20), (0.089, 0.13), (0.131, 0.10)):
            impulse[round((delay + ch * 0.011) * RATE)] += gain
        source = 0.8 * send[:, ch] + 0.2 * send[:, 1 - ch]
        result[:, ch] = np.fft.irfft(
            np.fft.rfft(source) * np.fft.rfft(impulse, n=FRAMES), n=FRAMES
        ).astype(np.float32)
    return result


def compose() -> np.ndarray:
    rng = np.random.default_rng(SEED)
    dry = np.zeros((FRAMES, 2), np.float32)
    send = np.zeros_like(dry)
    lead = np.zeros_like(dry)

    def play(kind: str, midi: int, beat: float, hold: float, level: float,
             pan: float, wet: float, destination: np.ndarray = dry) -> None:
        sound = voice(kind, midi, hold * BEAT, float(rng.uniform(0, TAU)))
        start = beat * BEAT
        add_wrapped(destination, sound, start, level, pan)
        add_wrapped(send, sound, start, level * wet, pan)

    for section, (_, bass, notes) in enumerate(HARMONY):
        base = section * 8
        phrase_gain = [0.90, 1.0, 0.94, 0.86][section // 4]
        for i, pitch in enumerate(notes):
            play("pad", pitch, base - 0.32, 7.2, 0.027 * phrase_gain,
                 (i - 1.5) * 0.38, 0.42)
            # Lightly rolled electric piano chords, with a small second response.
            for beat, gain in ((0.08, 1.0), (4.35, 0.47)):
                play("piano", pitch, base + beat + i * 0.038,
                     2.6, 0.038 * gain * phrase_gain, (i - 1.5) * 0.24, 0.45)

        for bar in range(2):
            play("bass", bass, base + bar * 4 + 0.04, 2.9,
                 0.080 * phrase_gain, 0, 0.07)
            # A quiet arpeggio suggests movement beneath the sparse melody.
            pattern = [(0.8, 0), (2.2, 2), (3.35, 1)] if bar == 0 else [(1.15, 1), (2.75, 3)]
            for beat, index in pattern:
                pitch = notes[index] + (12 if index < 2 else 0)
                play("harp", pitch, base + bar * 4 + beat + float(rng.uniform(-0.025, 0.025)),
                     0.7, float(rng.uniform(0.021, 0.031)), -0.42 if bar == 0 else 0.4, 0.58)

        for index, (beat, pitch, hold) in enumerate(MELODY[section]):
            pan = 0.12 * np.sin((section * 5 + index) * 0.8)
            play("pearl", pitch, base + beat + float(rng.uniform(-0.014, 0.014)),
                 hold, float(rng.uniform(0.064, 0.078)) * phrase_gain, pan, 0.55, lead)

    # Quiet, dark echoes widen the melody. Circular shifts keep all tails.
    dry += lead
    for delay_beats, gain in ((0.75, 0.15), (1.5, 0.07), (2.25, 0.025)):
        echo = np.roll(lead[:, ::-1], round(delay_beats * BEAT * RATE), axis=0)
        dry += echo * gain
        send += echo * gain * 0.4
    del lead

    # Soft, periodic water movement, placed well below the instruments.
    t = np.arange(FRAMES, dtype=np.float64) / RATE
    for ch in range(2):
        water = spectral_filter(rng.normal(0, 1, FRAMES).astype(np.float32), 180, 1250)
        water /= np.sqrt(np.mean(water ** 2))
        swell = 0.62 + 0.2 * np.sin(TAU * 4 * t / DURATION + ch) + 0.1 * np.sin(TAU * 7 * t / DURATION)
        dry[:, ch] += water * swell * 0.0016
    del water, t

    # Small, rounded bubbles between phrases, never a continuous bubbling layer.
    for beat in (6.5, 15, 24.5, 30.75, 39, 47.25, 57.5, 64.5, 74, 87.5, 99, 110.5, 119):
        for j in range(2):
            t = timeline(0.17)
            f = float(rng.uniform(430, 850))
            p = TAU * f * (t + 0.4 * (t - 0.045 * (1 - np.exp(-t / 0.045))))
            bubble = (np.sin(p) * np.sin(np.minimum(t / 0.015, 1) * np.pi / 2) ** 2
                      * np.exp(-t / 0.035) * np.cos(t / 0.17 * np.pi / 2) ** 2).astype(np.float32)
            pan = float(rng.uniform(-0.7, 0.7))
            add_wrapped(dry, bubble, beat * BEAT + j * 0.19, 0.006, pan)
            add_wrapped(send, bubble, beat * BEAT + j * 0.19, 0.004, pan)

    print("Rendering the stereo reverb and loop tails...", flush=True)
    dry += reverberate(send, rng) * 0.40
    del send
    for ch in range(2):
        dry[:, ch] = spectral_filter(dry[:, ch], 32, 8500)
    return dry


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_wav(path: Path, samples: np.ndarray) -> None:
    rng = np.random.default_rng(SEED + 1)
    # Triangular dither for the 16-bit runtime asset.
    dither = (rng.random(samples.shape, dtype=np.float32)
              - rng.random(samples.shape, dtype=np.float32)) / 65536
    pcm = np.rint(np.clip(samples + dither, -1, 1) * 32767).astype("<i2")
    with wave.open(str(path), "wb") as out:
        out.setnchannels(2)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(pcm.tobytes())


def loudness(ffmpeg: str, path: Path, raw: bool = False) -> dict:
    input_args = ["-f", "f32le", "-ar", str(RATE), "-ac", "2"] if raw else []
    result = subprocess.run(
        [ffmpeg, "-hide_banner", "-nostdin", *input_args, "-i", str(path),
         "-af", "loudnorm=I=-20:TP=-2:LRA=11:print_format=json", "-f", "null", "-"],
        check=True, capture_output=True, text=True,
    )
    # Some FFmpeg builds print their final progress line after the JSON object.
    return json.JSONDecoder().raw_decode(result.stderr[result.stderr.rfind("{"):])[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "assets/audio")
    parser.add_argument("--preview-dir", type=Path, default=ROOT / "design/audio")
    args = parser.parse_args()
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        parser.error("FFmpeg is required to measure loudness and encode the preview.")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    args.preview_dir.mkdir(parents=True, exist_ok=True)
    wav_path = args.output_dir / "lagoon-daydream-loop.wav"
    preview_path = args.preview_dir / "lagoon-daydream-preview.mp3"

    print(f"Composing Lagoon Daydream: {DURATION:.0f} seconds, {BPM} BPM...", flush=True)
    samples = compose()
    with tempfile.TemporaryDirectory(prefix="fishius-audio-") as temp:
        raw = Path(temp) / "mix.f32le"
        samples.astype("<f4").tofile(raw)
        measured = loudness(ffmpeg, raw, raw=True)
    # Apply one constant gain. Time-varying loudness normalization can break a loop.
    gain_db = min(-20 - float(measured["input_i"]), -2 - float(measured["input_tp"]))
    samples *= 10 ** (gain_db / 20)
    if not np.isfinite(samples).all() or np.max(np.abs(samples)) >= 1:
        raise RuntimeError("The master contains invalid or clipped samples.")
    write_wav(wav_path, samples)
    subprocess.run(
        [ffmpeg, "-hide_banner", "-loglevel", "error", "-nostdin", "-y", "-i", str(wav_path),
         "-af", f"afade=t=in:d=0.7,afade=t=out:st={DURATION - 2.5}:d=2.5",
         "-c:a", "libmp3lame", "-b:a", "192k", "-metadata", "title=Lagoon Daydream",
         "-metadata", "album=Fishius", str(preview_path)], check=True,
    )
    measured = loudness(ffmpeg, wav_path)
    seam = float(np.max(np.abs(samples[0] - samples[-1])))
    max_step = float(np.max(np.abs(np.diff(samples, axis=0))))
    record = {
        "title": "Lagoon Daydream",
        "created": "2026-09-18",
        "description": "Calm aquarium music with pearl mallets, warm electric piano, soft pads, harp, bass and quiet synthesized water.",
        "method": "Original score and procedural synthesis written for Fishius. No third-party samples, recordings, or sound fonts.",
        "generator": "tools/generate_lagoon_music.py",
        "generator_sha256": sha256(Path(__file__)),
        "seed": SEED,
        "bpm": BPM,
        "meter": "4/4",
        "tonal_center": "D major",
        "bars": BARS,
        "duration_seconds": DURATION,
        "sample_rate": RATE,
        "channels": 2,
        "pcm_bits": 16,
        "loop_start_frame": 0,
        "loop_end_frame_exclusive": FRAMES,
        "loop_method": "Note tails, delay and reverb wrap across the full buffer. No fade or silent gap in the WAV.",
        "chord_progression": [row[0] for row in HARMONY],
        "verification": {
            "integrated_loudness_lufs": float(measured["input_i"]),
            "true_peak_dbtp": float(measured["input_tp"]),
            "loudness_range_lu": float(measured["input_lra"]),
            "seam_sample_step": seam,
            "largest_sample_step": max_step,
            "finite_samples": bool(np.isfinite(samples).all()),
            "clipped_samples": int(np.count_nonzero(np.abs(samples) >= 1)),
            "review": "Objective level and continuity checks. The preview is provided for listening review.",
        },
        "files": {
            "loop": {"name": wav_path.name, "sha256": sha256(wav_path), "bytes": wav_path.stat().st_size},
            "preview": {"name": preview_path.name, "sha256": sha256(preview_path), "bytes": preview_path.stat().st_size},
        },
    }
    provenance = args.output_dir / "lagoon-daydream.provenance.json"
    provenance.write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({"wav": str(wav_path), "preview": str(preview_path),
                      "verification": record["verification"]}, indent=2))


if __name__ == "__main__":
    main()
