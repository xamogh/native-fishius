// Render an original note score through macOS's General MIDI instruments.
// The sound bank remains on the host. Only the performed mix is exported.
import AVFoundation
import Foundation

struct Note: Decodable {
    let start: Double
    let duration: Double
    let pitch: UInt8
    let velocity: UInt8
}

struct Track: Decodable {
    let name: String
    let program: UInt8
    let percussion: Bool
    let gain: Float
    let pan: Float
    let notes: [Note]
}

struct Score: Decodable {
    let sampleRate: Double
    let duration: Double
    let tracks: [Track]
}

struct Event {
    let frame: Int64
    let track: Int
    let pitch: UInt8
    let velocity: UInt8
    let on: Bool
}

func render() throws {
    guard CommandLine.arguments.count == 3 else {
        throw NSError(domain: "MusicRenderer", code: 1,
                      userInfo: [NSLocalizedDescriptionKey: "Usage: render_music_score score.json output.wav"])
    }
    let scoreURL = URL(fileURLWithPath: CommandLine.arguments[1])
    let outputURL = URL(fileURLWithPath: CommandLine.arguments[2])
    let score = try JSONDecoder().decode(Score.self, from: Data(contentsOf: scoreURL))
    let bank = URL(fileURLWithPath: "/System/Library/Components/CoreAudio.component/Contents/Resources/gs_instruments.dls")
    let engine = AVAudioEngine()
    let format = AVAudioFormat(standardFormatWithSampleRate: score.sampleRate, channels: 2)!
    var samplers: [AVAudioUnitSampler] = []
    var events: [Event] = []

    for (index, track) in score.tracks.enumerated() {
        let sampler = AVAudioUnitSampler()
        engine.attach(sampler)
        engine.connect(sampler, to: engine.mainMixerNode, format: format)
        try sampler.loadSoundBankInstrument(at: bank, program: track.program,
                                            bankMSB: track.percussion ? 0x78 : 0x79, bankLSB: 0)
        sampler.overallGain = track.gain
        sampler.stereoPan = track.pan
        sampler.sendController(91, withValue: 0, onChannel: 0)
        sampler.sendController(93, withValue: 0, onChannel: 0)
        samplers.append(sampler)
        for note in track.notes {
            events.append(Event(frame: Int64((note.start * score.sampleRate).rounded()), track: index,
                                pitch: note.pitch, velocity: note.velocity, on: true))
            events.append(Event(frame: Int64(((note.start + note.duration) * score.sampleRate).rounded()),
                                track: index, pitch: note.pitch, velocity: 0, on: false))
        }
    }
    // Stop an older note before starting the next occurrence of its pitch.
    events.sort {
        if $0.frame != $1.frame { return $0.frame < $1.frame }
        return !$0.on && $1.on
    }
    try engine.enableManualRenderingMode(.offline, format: format, maximumFrameCount: 1024)
    engine.prepare()
    try engine.start()
    defer { engine.stop() }

    var fileSettings = format.settings
    fileSettings[AVLinearPCMIsNonInterleaved] = false
    let file = try AVAudioFile(forWriting: outputURL, settings: fileSettings)
    let buffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: 1024)!
    let total = Int64((score.duration * score.sampleRate).rounded())
    var position: Int64 = 0
    var nextEvent = 0
    var retries = 0
    while position < total {
        while nextEvent < events.count && events[nextEvent].frame <= position {
            let event = events[nextEvent]
            if event.on {
                samplers[event.track].startNote(event.pitch, withVelocity: event.velocity, onChannel: 0)
            } else {
                samplers[event.track].stopNote(event.pitch, onChannel: 0)
            }
            nextEvent += 1
        }
        let boundary = nextEvent < events.count ? events[nextEvent].frame : total
        let count = AVAudioFrameCount(min(1024, min(total - position, boundary - position)))
        guard count > 0 else { continue }
        let status = try engine.renderOffline(count, to: buffer)
        switch status {
        case .success:
            try file.write(from: buffer)
            position += Int64(buffer.frameLength)
            retries = 0
        case .cannotDoInCurrentContext:
            retries += 1
            if retries > 100 {
                throw NSError(domain: "MusicRenderer", code: 2,
                              userInfo: [NSLocalizedDescriptionKey: "Offline renderer stalled."])
            }
        default:
            throw NSError(domain: "MusicRenderer", code: 3,
                          userInfo: [NSLocalizedDescriptionKey: "Offline rendering failed: \(status.rawValue)"])
        }
    }
    print("Rendered \(score.tracks.count) instruments, \(Double(total) / score.sampleRate) seconds.")
}

do {
    try render()
} catch {
    fputs("\(error.localizedDescription)\n", stderr)
    exit(1)
}
